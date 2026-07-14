#include "TiledGraphicsView.h"
#include <QOpenGLWidget>
#include <QScrollBar>
#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include <QFileInfo>
#include <QtMath>
#include <QMenu>
#include <QMessageBox>
#include <QSurfaceFormat>

#include "./tools/DefectDrawTool.h"
#include "./items/DefectShapeItem.h"

static bool isSameImageNameForSdkView(const QString& itemName, const QString& queryName)
{
	if (itemName.isEmpty() || queryName.isEmpty())
	{
		return false;
	}

	if (itemName == queryName)
	{
		return true;
	}

	const QFileInfo itemInfo(itemName);
	const QFileInfo queryInfo(queryName);
	return itemInfo.fileName() == queryInfo.fileName()
		|| itemInfo.completeBaseName() == queryInfo.completeBaseName()
		|| itemName == queryInfo.fileName()
		|| itemName == queryInfo.completeBaseName()
		|| queryName == itemInfo.fileName()
		|| queryName == itemInfo.completeBaseName();
}

static QRectF sdkVisibleLocalRectToSourceRect(const QRectF& visibleLocalRect, const QRectF& itemLocalRect, const AbstractTileSource* source)
{
	QRectF sourceRect = visibleLocalRect;
	if (source && source->isHMirrored())
	{
		sourceRect.setLeft(itemLocalRect.width() - visibleLocalRect.right());
		sourceRect.setRight(itemLocalRect.width() - visibleLocalRect.left());
	}
	if (source && source->isVMirrored())
	{
		sourceRect.setTop(itemLocalRect.height() - visibleLocalRect.bottom());
		sourceRect.setBottom(itemLocalRect.height() - visibleLocalRect.top());
	}
	return sourceRect.normalized().intersected(itemLocalRect);
}

static void drawSdkTileImage(QPainter& painter, const QRectF& itemSceneRect, const QRectF& tileLocalRect,
	const QImage& tile, const AbstractTileSource* source)
{
	if (!source || (!source->isHMirrored() && !source->isVMirrored()))
	{
		painter.drawImage(QRectF(itemSceneRect.x() + tileLocalRect.x(), itemSceneRect.y() + tileLocalRect.y(),
			tileLocalRect.width(), tileLocalRect.height()), tile);
		return;
	}

	painter.save();
	painter.translate(itemSceneRect.left(), itemSceneRect.top());
	painter.translate(source->isHMirrored() ? itemSceneRect.width() : 0.0,
		source->isVMirrored() ? itemSceneRect.height() : 0.0);
	painter.scale(source->isHMirrored() ? -1.0 : 1.0, source->isVMirrored() ? -1.0 : 1.0);
	painter.drawImage(tileLocalRect, tile);
	painter.restore();
}

TiledGraphicsView::TiledGraphicsView(QWidget* parent) : QGraphicsView(parent),m_scrollSpeed(50), m_orientation(LayoutOrientation::Vertical)
{ 
	m_curDrawShape = Shape_Line;

    // ?? 看门狗： 
    if (this->devicePixelRatio() > 1.0) {
        if (!QCoreApplication::testAttribute(Qt::AA_EnableHighDpiScaling)) {
            qCritical() << "High DPI scaling is not enabled.";
            qCritical() << "Please enable QApplication::setAttribute(Qt::AA_EnableHighDpiScaling) before QApplication is created.";
            // 这里仅仅打印日志，不弹窗也不崩溃，起到提示作用即�?
        }
    }
    // 配置 GraphicsView (硬件加速、事件拦�?
    setupGraphicsView();
    // 绑定信号槽、快捷键、定时器
    setupConnections();
	setViewMode(Mode_Browse);
	// ?? 挂载万能绘制工具，并告诉它画什么形�?
	m_currentTool = new DefectDrawTool(m_curDrawShape);
	m_currentTool->setView(this);
}

TiledGraphicsView::~TiledGraphicsView()
{ 
    clear();
	delete m_currentTool;
	m_currentTool = nullptr;
}

// 设置视图名称
void TiledGraphicsView::setViewName(QString strName)
{
	m_strViewName = strName;
}

// 获取视图名称
QString TiledGraphicsView::getViewName()
{
	return m_strViewName;
}

QPointF TiledGraphicsView::mapToGlobalScene(const QString & imageName, qreal localX, qreal localY)
{
	if (!imageName.isEmpty())
	{
		TunnelSectionItem* cachedItem = m_imageItemCache.value(imageName, nullptr);
		if (cachedItem && isSameImageNameForSdkView(cachedItem->getImageName(), imageName))
		{
			return cachedItem->mapToScene(QPointF(localX, localY));
		}
		if (cachedItem)
		{
			qWarning().noquote() << "[HN_SDK_IMAGE_LOOKUP_CACHE_REJECT]"
				<< "query=" << imageName
				<< "cachedItem=" << cachedItem->getImageName();
			m_imageItemCache.remove(imageName);
		}
	}

	for (TunnelSectionItem* item : m_items)
	{
		if (item && isSameImageNameForSdkView(item->getImageName(), imageName))
		{
			m_imageItemCache.insert(imageName, item);
			m_imageItemCache.insert(item->getImageName(), item);
			const QFileInfo itemInfo(item->getImageName());
			if (!itemInfo.fileName().isEmpty())
			{
				m_imageItemCache.insert(itemInfo.fileName(), item);
			}
			if (!itemInfo.completeBaseName().isEmpty())
			{
				m_imageItemCache.insert(itemInfo.completeBaseName(), item);
			}
			return item->mapToScene(QPointF(localX, localY));
		}
	}

	if (m_items.isEmpty())
	{
		return QPointF(0, 0);
	}

	// Keep all points for one unresolved image alias on the same item. The old
	// localY-based fallback sent y == 0 to the first item and the other corners
	// to the last item, stretching one border cell across the complete scene.
	TunnelSectionItem* stableFallbackItem = m_imageItemCache.value(imageName, nullptr);
	if (!stableFallbackItem)
	{
		stableFallbackItem = m_items.last();
		if (!imageName.isEmpty())
		{
			m_imageItemCache.insert(imageName, stableFallbackItem);
		}
	}

	// If image name cannot be resolved, keep a stable fallback item for this alias.
	TunnelSectionItem* fallbackItem = localY <= 0 ? m_items.first() : m_items.last();
	fallbackItem = stableFallbackItem;
	if (!fallbackItem)
	{
		return QPointF(0, 0);
	}
	const QRectF localRect = fallbackItem->boundingRect();
	return fallbackItem->mapToScene(QPointF(qBound(localRect.left(), localX, localRect.right()),
		qBound(localRect.top(), localY, localRect.bottom())));
}

void TiledGraphicsView::clearImageItemLookupCache()
{
	m_imageItemCache.clear();
}

void TiledGraphicsView::focusOnPosition(const QPointF& scenePos, double targetScale)
{
    // 1. 如果指定了缩放级别，先应用缩�?
    if (targetScale > 0) {
        // [限制上下限逻辑] 保持和你 wheelEvent 里一样的逻辑
        double minScale = getFitScale();
        double maxScale = 5.0;
        if (targetScale < minScale) targetScale = minScale;
        if (targetScale > maxScale) targetScale = maxScale;

        resetTransform();
        scale(targetScale, targetScale);
        m_currentScale = targetScale;
    }

    // 自动计算滚动条并居中目标点！
    centerOn(scenePos);


	// ??直接�?Scene 谁在这个点上，不用自己遍�?list
	// items() 返回的是 Z 值从上到下的列表，第一个通常就是最上面�?
	QList<QGraphicsItem*> items = m_scene->items(scenePos);
	TunnelSectionItem* hoverItem = nullptr;
	for (auto item : items) {
		// 使用 dynamic_cast 确认是不是我们要找的切片�?
		hoverItem = dynamic_cast<TunnelSectionItem*>(item);
		if (hoverItem) break;
	}

	// 3. 发送坐标信号给外部 HUD
	if (hoverItem) {
		QPointF localPos = hoverItem->mapFromScene(scenePos);
		emit sigCursorInfoChanged(
			hoverItem->getImageName(),
			(int)localPos.x(),
			(int)localPos.y()
		);
	}
	else {
		emit sigCursorInfoChanged("", 0, 0);
	}

    // 3. 强制触发一次高清切片加载和 LOD 刷新
    updateVisibleTiles();
	emitViewCenterChanged();
}

QPointF TiledGraphicsView::currentCenterScenePos() const
{
	// �?viewport 的几何中心反�?scene 坐标，比读滚动条更稳，缩放后也不会跑偏�?
	if (!viewport())
	{
		return QPointF();
	}

	return mapToScene(viewport()->rect().center());
}

double TiledGraphicsView::currentCenterSceneY() const
{
	// 纵向长图联动只关�?Y，这里单独给一个入口，调用侧代码会更直白�?
	return currentCenterScenePos().y();
}

QPointF TiledGraphicsView::currentBottomCenterScenePos() const
{
	// 用视口底边中点做锚点，用户看到的底部位置就是业务联动的当前位置�?
	if (!viewport())
	{
		return QPointF();
	}

	QRect viewportRect = viewport()->rect();
	QPoint bottomCenter(viewportRect.center().x(), qMax(0, viewportRect.bottom()));
	QPointF scenePos = mapToScene(bottomCenter);

	if (m_scene && !m_scene->sceneRect().isEmpty())
	{
		const QRectF sceneRect = m_scene->sceneRect();
		scenePos.setX(qBound(sceneRect.left(), scenePos.x(), sceneRect.right()));
		scenePos.setY(qBound(sceneRect.top(), scenePos.y(), sceneRect.bottom()));
	}

	return scenePos;
}

TiledViewAnchor TiledGraphicsView::currentBottomAnchor() const
{
	TiledViewAnchor anchor;
	if (m_items.isEmpty())
	{
		return anchor;
	}

	QPointF scenePos = currentBottomCenterScenePos();
	TunnelSectionItem* matchedItem = nullptr;
	int matchedIndex = -1;

	for (int i = 0; i < m_items.size(); ++i)
	{
		TunnelSectionItem* item = m_items.at(i);
		const QRectF itemSceneRect = item->sceneBoundingRect();
		if (itemSceneRect.contains(scenePos) ||
			(scenePos.y() >= itemSceneRect.top() && scenePos.y() <= itemSceneRect.bottom()))
		{
			matchedItem = item;
			matchedIndex = i;
			break;
		}
	}

	if (!matchedItem)
	{
		// 滚到场景边界时，底边可能正好落在最后一张图外一点点，按最近一张兜底�?
		const QRectF firstRect = m_items.first()->sceneBoundingRect();
		const QRectF lastRect = m_items.last()->sceneBoundingRect();
		if (scenePos.y() < firstRect.top())
		{
			matchedItem = m_items.first();
			matchedIndex = 0;
			scenePos.setY(firstRect.top());
		}
		else
		{
			matchedItem = m_items.last();
			matchedIndex = m_items.size() - 1;
			scenePos.setY(lastRect.bottom());
		}
	}

	QPointF localPos = matchedItem->mapFromScene(scenePos);
	const QRectF localRect = matchedItem->boundingRect();
	localPos.setX(qBound(localRect.left(), localPos.x(), localRect.right()));
	localPos.setY(qBound(localRect.top(), localPos.y(), localRect.bottom()));

	anchor.valid = true;
	anchor.imageName = matchedItem->getImageName();
	anchor.imageIndex = matchedIndex;
	anchor.scenePos = scenePos;
	anchor.imagePixelPos = localPos;
	return anchor;
}

void TiledGraphicsView::scrollToSceneY(double sceneY)
{
	if (!m_scene)
	{
		return;
	}

	const QRectF sceneRect = m_scene->sceneRect();
	if (!sceneRect.isEmpty())
	{
		sceneY = qBound(sceneRect.top(), sceneY, sceneRect.bottom());
	}

	// 只换 Y，不主动�?X，避免用户横向查看某一车道时被联动逻辑拉回中间�?
	QPointF centerPos = currentCenterScenePos();
	if (centerPos.isNull() && !sceneRect.isEmpty())
	{
		centerPos = sceneRect.center();
	}
	centerPos.setY(sceneY);
	centerOn(centerPos);
	updateVisibleTiles();
	emitViewCenterChanged();
}

void TiledGraphicsView::scrollToImagePixel(int imageIndex, double pixelY, bool anchorBottom)
{
	if (!m_scene || imageIndex < 0 || imageIndex >= m_items.size())
	{
		return;
	}

	TunnelSectionItem* item = m_items.at(imageIndex);
	const QRectF localRect = item->boundingRect();
	pixelY = qBound(localRect.top(), pixelY, localRect.bottom());

	const QPointF currentCenter = currentCenterScenePos();
	const QPointF targetScenePos = item->mapToScene(QPointF(localRect.center().x(), pixelY));
	QPointF newCenter = currentCenter.isNull() ? targetScenePos : currentCenter;

	if (anchorBottom && viewport())
	{
		const QRectF visibleSceneRect = mapToScene(viewport()->rect()).boundingRect();
		newCenter.setY(targetScenePos.y() - visibleSceneRect.height() / 2.0);
	}
	else
	{
		newCenter.setY(targetScenePos.y());
	}

	centerOn(newCenter);
	updateVisibleTiles();
	emitViewCenterChanged();
}

void TiledGraphicsView::scrollToImagePixel(const QString& imageName, double pixelY, bool anchorBottom)
{
	if (!m_scene || imageName.isEmpty())
	{
		return;
	}

	TunnelSectionItem* targetItem = nullptr;
	for (TunnelSectionItem* item : qAsConst(m_items))
	{
		if (item && isSameImageNameForSdkView(item->getImageName(), imageName))
		{
			targetItem = item;
			break;
		}
	}

	if (!targetItem)
	{
		return;
	}

	const QRectF localRect = targetItem->boundingRect();
	pixelY = qBound(localRect.top(), pixelY, localRect.bottom());

	const QPointF currentCenter = currentCenterScenePos();
	const QPointF targetScenePos = targetItem->mapToScene(QPointF(localRect.center().x(), pixelY));
	QPointF newCenter = currentCenter.isNull() ? targetScenePos : currentCenter;
	newCenter.setX(targetScenePos.x());

	if (anchorBottom && viewport())
	{
		const QRectF visibleSceneRect = mapToScene(viewport()->rect()).boundingRect();
		newCenter.setY(targetScenePos.y() - visibleSceneRect.height() / 2.0);
	}
	else
	{
		newCenter.setY(targetScenePos.y());
	}

	centerOn(newCenter);
	updateVisibleTiles();
	emitViewCenterChanged();
}

void TiledGraphicsView::addLayer(AbstractTileSource* source)
{
    TunnelSectionItem* item = new TunnelSectionItem(source);
	connect(item, &TunnelSectionItem::sigTilesUpdated, this, &TiledGraphicsView::updateHUD);
    m_scene->addItem(item);

    // 自动拼接逻辑：方向由 LayoutOrientation 决定，不再默认所有项目都从左到右拼�?
	QPointF itemPos(0, 0);
	if (!m_items.isEmpty()) {
		if (isReverseLayout(m_orientation)) {
			TunnelSectionItem* first = m_items.first();
			if (isVerticalLayout(m_orientation)) {
				itemPos = QPointF(0, first->pos().y() - item->boundingRect().height());
			}
			else {
				itemPos = QPointF(first->pos().x() - item->boundingRect().width(), 0);
			}
		}
		else {
			TunnelSectionItem* last = m_items.last();
			if (isVerticalLayout(m_orientation)) {
				itemPos = QPointF(0, last->pos().y() + last->boundingRect().height());
			}
			else {
				itemPos = QPointF(last->pos().x() + last->boundingRect().width(), 0);
			}
		}
	}

    item->setPos(itemPos);
	item->m_senceRect = QRectF(item->pos(), item->boundingRect().size());
	if (isReverseLayout(m_orientation)) {
		m_items.prepend(item);
	}
	else {
		m_items.append(item);
	}
	m_imageItemCache.clear();

    // 更新场景边界
    QRectF allRect = m_scene->itemsBoundingRect();
    m_scene->setSceneRect(allRect);

    // 如果是第一张图，自动适应高度
    if (m_items.size() == 1) {

         resetTransform();
        double startScale = getFitScale();
        scale(startScale, startScale);
        m_currentScale = startScale;
        
    }
}

void TiledGraphicsView::clear()
{
	if (m_vecCp3Manager) m_vecCp3Manager->clearDefects();
	if (m_vecPlatformManager) m_vecPlatformManager->clearDefects();
	if (m_vecChainManager) m_vecChainManager->clearDefects();
	if (m_defectManager) m_defectManager->clearDefects();
	if (m_vecTunnelLocManager) m_vecTunnelLocManager->clearDefects();
	if (m_vecRingInfoManager) m_vecRingInfoManager->clearDefects();
	if (m_vecSectionManager) m_vecSectionManager->clearDefects();
	if (m_vecReAutoRingManager) m_vecReAutoRingManager->clearDefects();
	if (m_currentTool) m_currentTool->deactivate();

    m_scene->clear(); // 这会 delete 掉所有的 item
    m_items.clear();
	m_imageItemCache.clear();
	m_exportBoxItem = nullptr;
    m_scene->setSceneRect(0, 0, 0, 0);
}


// ?? 1. 实现公开的重置接�?
void TiledGraphicsView::resetToFit()
{ 
    double s = getFitScale();
    resetTransform();
    scale(s, s);
    m_currentScale = s;
    updateVisibleTiles();
	emitViewCenterChanged();
}


void TiledGraphicsView::updateHUD()
{
	int loadedTiles = 0;
	for (auto item : m_items) {
	 
		loadedTiles += item->getLoadedTileCount();
	}
 bool	isHighResMode = false;
	if (loadedTiles > 0)
	{
		isHighResMode = true;
	}

	
	QString quality = isHighResMode ? QString::fromLocal8Bit("高清") : QString::fromLocal8Bit("预览");

	QString stats = QString::fromLocal8Bit(
		"绘制模式: %1 | 切片: %2 | 缩放: %3% | 画质: %4 |LOD系数: %5"
	).arg(DrawModelStr).arg(loadedTiles)
		.arg(m_currentScale * 100, 0, 'f', 0).arg(quality)
		.arg(m_lodThresholdMultiplier, 0, 'f', 2);

	emit sigStatsUpdated(stats);
}

void TiledGraphicsView::setViewMode(ViewMode mode)
{
    m_currentMode = mode; 
 	setDragMode(QGraphicsView::NoDrag);
	if (m_exportBoxItem)
	{
		m_exportBoxItem->hide();
	}
    if (mode == Mode_Browse) {
        setDragMode(QGraphicsView::NoDrag);
        setCursor(Qt::ArrowCursor);
		DrawModelStr = QString::fromLocal8Bit("浏览模式");
    }
    else if (mode == Mode_Draw)
    {
		setDragMode(QGraphicsView::NoDrag);
		setCursor(Qt::CrossCursor); // 绘图模式用十字光�?
		DrawModelStr = QString::fromLocal8Bit("绘图模式");

		startDrawingDefect(m_curDrawShape);
    }
	else if (mode == Mode_ExportBox)
     {

		setCursor(Qt::BlankCursor);
		DrawModelStr = QString::fromLocal8Bit("定焦截图模式");
		if (!m_exportBoxItem)
		{
			m_exportBoxItem = new QGraphicsRectItem;
			QPen pen(Qt::yellow, 3, Qt::DashLine);
			pen.setCosmetic(true);
			m_exportBoxItem->setPen(pen);
			m_exportBoxItem->setZValue(9999);
			m_scene->addItem(m_exportBoxItem);
		}
		m_exportBoxItem->setRect(0, 0, exportBoxSize, exportBoxSize);
		m_exportBoxItem->show();
     
    }

    // 可以在这里触�?updateVisibleTiles 以刷新可能的 UI 状�?
	updateVisibleTiles();
}

// =========================================================
// ?? 核心交互逻辑
// =========================================================

void TiledGraphicsView::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx, dy);
	emitViewCenterChanged();
	if (horizontalScrollBar()->isSliderDown() || verticalScrollBar()->isSliderDown())
	{
		emitUserViewBottomAnchorChanged();
		m_debounceTimer->start();
		return;
	} 
	
    // 触发防抖更新 (LOD 计算)
    m_debounceTimer->start();
}

void TiledGraphicsView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);

     double minScale = getFitScale();
    if (m_currentScale < minScale || qAbs(m_currentScale - minScale) < 0.001) {
        resetTransform();
        scale(minScale, minScale);
        m_currentScale = minScale;
    } 
    updateVisibleTiles();
	emitViewCenterChanged();
}

void TiledGraphicsView::wheelEvent(QWheelEvent* event) {
    // ---------------------------------------------------------
     // ?? 1. 缩放逻辑 (Ctrl + 滚轮)
     // ---------------------------------------------------------
    if (event->modifiers() & Qt::ControlModifier) {

        int angle = event->angleDelta().y();
        if (angle == 0) return;
		const QPoint viewportPoint = event->pos();
		const QPointF scenePointUnderMouseBeforeZoom = mapToScene(viewportPoint);

        // 1. 获取当前缩放系数 (m11 �?x 轴缩放，通常 xy 一�?
        double currentScale = transform().m11();

        // 2. 计算缩放因子 (滚轮向上放大 1.1 倍，向下缩小 0.9 �?
        double scaleFactor = (angle > 0) ? 1.15 : (1.0 / 1.15);

        // 3. 预测下一次的缩放�?
        double nextScale = currentScale * scaleFactor;

        // =====================================================
        // ?? 核心修改：动态计算边�?
        // =====================================================

        // [下限]：不允许缩得比“适应窗口”还�?
        // 这样用户狂滚滚轮，最后一定会停在刚好铺满屏幕的状态，非常舒服
        double minScale = getFitScale();

        // [上限]：最大允许放大到 5.0 �?(�?1 个像素变 5 个像素大)
        // 隧道病害一般看清裂缝即可，5.0 足够了，太大全是锯齿
        double maxScale = 20.0;

      
        // 如果下一次缩放会超出边界，就只缩放到边界�?
        if (nextScale < minScale) {
            scaleFactor = minScale / currentScale;
            nextScale = minScale;
        }
        else if (nextScale > maxScale) {
            scaleFactor = maxScale / currentScale;
            nextScale = maxScale;
        }

        // 5. 应用缩放
        // 只有当实际上发生了变化，且变化不过分微小的时候才执行
        if (qAbs(scaleFactor - 1.0) > 0.001) {
			// AnchorUnderMouse uses the global cursor and can be disturbed by the
			// bottom-anchor restoration used by the old linkage code. Preserve the
			// event position explicitly so Ctrl+wheel zooms around this mouse point.
			setTransformationAnchor(QGraphicsView::NoAnchor);
            scale(scaleFactor, scaleFactor);
            m_currentScale = nextScale; // 更新成员变量
			const QPointF scenePointUnderMouseAfterZoom = mapToScene(viewportPoint);
			const QPointF centerAfterZoom = mapToScene(viewport()->rect().center());
			centerOn(centerAfterZoom + scenePointUnderMouseBeforeZoom - scenePointUnderMouseAfterZoom);
			setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
            updateVisibleTiles();       // 触发 LOD
			emitViewCenterChanged();
			emitUserViewBottomAnchorChanged();
        }

        event->accept();
        return;
    }

    // 2. 滚动逻辑 (修复 Shift 支持)
    int delta = event->angleDelta().y();
    if (delta == 0) return;

    // 获取 Shift 键状�?
    bool isShiftPressed = (event->modifiers() & Qt::ShiftModifier);

    if (isVerticalLayout(m_orientation)) {
        // === 纵向模式 (地铁) ===
        if (isShiftPressed) {
            // Shift: 滚水平条 (左右�?
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta);
        }
        else {
            // 普�? 滚垂直条 (上下跑里�?
            verticalScrollBar()->setValue(verticalScrollBar()->value() - delta);
        }
    }
    else {
        // === 横向模式 (公路) ===
        if (isShiftPressed) {
            // ?? 修复点：Shift -> 滚垂直条 (上下看墙�?
            verticalScrollBar()->setValue(verticalScrollBar()->value() - delta);
        }
        else {
            // 普�? 滚水平条 (左右跑里�?
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta);
        }
    } 
	emitUserViewBottomAnchorChanged();
    event->accept(); 
}

void TiledGraphicsView::keyPressEvent(QKeyEvent* event) {

    // 如果外部禁用了导航（比如正在输入文字，或处于编辑模式），直接透传给父�?
    if (!m_enableKeyNav) {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    // 小优化：处理 Shift 加�?
    int speed = m_scrollSpeed;
    bool isShift = (event->modifiers() & Qt::ShiftModifier);
    if (isShift) {
        speed *= 4;
        m_isFastScrolling = true; // ?? 进入飙车模式
    }
    bool handled = true;

    switch (event->key()) {
    case Qt::Key_Space:
        resetToFit();   // 调用之前封装好的公有槽函�?
        event->accept(); // 标记事件已处理，防止传递给父类导致冲突
        break;
    case Qt::Key_W:
    case Qt::Key_Up:
        // W 键：只减小垂直滚动条 (向上)
        verticalScrollBar()->setValue(verticalScrollBar()->value() - speed);
        event->accept();
        break;

    case Qt::Key_S:
    case Qt::Key_Down:
        // S 键：只增加垂直滚动条 (向下)
        verticalScrollBar()->setValue(verticalScrollBar()->value() + speed);
        event->accept();
        break;

    case Qt::Key_A:
    case Qt::Key_Left:
        // A 键：只减小水平滚动条 (向左)
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - speed);
        event->accept();
        break;

    case Qt::Key_D:
    case Qt::Key_Right:
        // D 键：只增加水平滚动条 (向右)
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + speed);
        event->accept();
        break;

    default:
        handled = false;
        break;
    }

    if (!handled) {
        QGraphicsView::keyPressEvent(event);
        return;
    }

	emitUserViewBottomAnchorChanged();

    //  如果是快速滚动，不要立即触发重绘逻辑，或者只做轻量级更新
    if (!isShift) {
        // 普通移动已经改过滚动条了，这里只收下事件，避免父类再处理一遍快捷键�?
        event->accept();
    }
    else {
        // 飙车模式下，我们 accept 事件，让滚动条动，但暂时不让 View 疯狂加载
        event->accept();
    }
}

void TiledGraphicsView::keyReleaseEvent(QKeyEvent* event)
{ 
    if (event->key() == Qt::Key_Shift) {
        m_isFastScrolling = false; // ?? 飙车结束

        // 立即触发一次全量加载，把高清图刷出�?
        updateVisibleTiles();
		emitViewCenterChanged();
    }

    QGraphicsView::keyReleaseEvent(event);
}

void TiledGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
	QPointF scensPos = mapToScene(event->pos());

	if (m_currentMode == Mode_ExportBox && m_exportBoxItem)
	{
		QRectF boxRect(0,0, exportBoxSize, exportBoxSize);
		boxRect.moveCenter(scensPos);
		m_exportBoxItem->setRect(boxRect);
	}
	// ?? 1. 处理中键拖拽
	if (m_isPanning) {
		// 计算鼠标移动的差�?
		int dx = event->pos().x() - m_lastMousePos.x();
		int dy = event->pos().y() - m_lastMousePos.y();

		// 拨动滚动�?(方向相反，鼠标往右划，内容往右走，滚动条其实是往左减)
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() - dx);
		verticalScrollBar()->setValue(verticalScrollBar()->value() - dy); 
		emitUserViewBottomAnchorChanged();
		// 更新坐标
		m_lastMousePos = event->pos();
		event->accept();
		return;  
	}

	// ?? 绘图模式拦截：让工具画出跟随的虚�?
	if (m_currentMode == Mode_Draw && m_currentTool) {
		QPointF scenePos = mapToScene(event->pos());
		m_currentTool->handleMouseMove(scenePos);
	}
	// ?? 3. 核心引擎：自定义病害集群精准拖拽�?
	// ==========================================
	// 如果左键按下并处于拖拽状态，接管坐标换算
	if (m_isDraggingDefects) {
		QPointF currentScenePos = mapToScene(event->pos());
		// 计算鼠标在真实的物理世界里移动了多少距离
		QPointF delta = currentScenePos - m_lastDragScenePos;

		// 让所有被选中的病害跟着走，指哪打哪，绝对不乱飘�?
		for (QGraphicsItem* item : m_scene->selectedItems()) {
			if (DefectShapeItem* defect = dynamic_cast<DefectShapeItem*>(item)) {
				defect->setPos(defect->pos() + delta);
			}
		}

		m_lastDragScenePos = currentScenePos; // 更新坐标
		event->accept();
		return; //  
	}
	// ==========================================
	// ?? 4. 浏览模式下的光标“雷达反馈�?
	// ==========================================
	// 只有在没按任何鼠标键（纯移动探测）时，才触发嗅探
	if (m_currentMode == Mode_Browse && event->buttons() == Qt::NoButton) {

		// 调用我们强大的动态物理雷达探测器
		QList<QGraphicsItem*> items = getVisualItems(event->pos());

		bool hoverOnDefect = false;
		for (auto item : items) {
			if (dynamic_cast<DefectShapeItem*>(item)) {
				hoverOnDefect = true;
				break; // 只要探测到范围里有病害，立刻标记
			}
		}

		// ?? 动态切换光标：碰到病害变“点击小�??”，离开变“普通箭头↖�?
		if (hoverOnDefect) {
			setCursor(Qt::PointingHandCursor);
		}
		else {
			setCursor(Qt::ArrowCursor);
		}
	}




    QGraphicsView::mouseMoveEvent(event);

    // 1. 获取鼠标�?Scene 中的坐标
    QPointF scenePos = mapToScene(event->pos());

    // ??直接�?Scene 谁在这个点上，不用自己遍�?list
    // items() 返回的是 Z 值从上到下的列表，第一个通常就是最上面�?
    QList<QGraphicsItem*> items = m_scene->items(scenePos);
    TunnelSectionItem* hoverItem = nullptr;
    for (auto item : items) {
        // 使用 dynamic_cast 确认是不是我们要找的切片�?
        hoverItem = dynamic_cast<TunnelSectionItem*>(item);
        if (hoverItem) break;
    }

    // 3. 发送坐标信号给外部 HUD
    if (hoverItem) {
        QPointF localPos = hoverItem->mapFromScene(scenePos);
        QString info = QString::fromLocal8Bit("图名: %1 | 坐标: (%2, %3)")
            .arg(hoverItem->getImageName())
            .arg((int)localPos.x())
            .arg((int)localPos.y());
        emit sigCursorInfoChanged(
            hoverItem->getImageName(),
            (int)localPos.x(),
            (int)localPos.y()
        );
    }
    else {
        emit sigCursorInfoChanged("", 0, 0);
    }
}

void TiledGraphicsView::mousePressEvent(QMouseEvent* event)
{ 
	// 1. 中键拖拽背景
	if (event->button() == Qt::MiddleButton) {
		m_isPanning = true;
		m_lastMousePos = event->pos();
		setCursor(Qt::ClosedHandCursor);
		event->accept();
		return;
	}

	// 2. 绘图模式
	if (m_currentMode == Mode_Draw && m_currentTool) {
		m_currentTool->handleMousePress(mapToScene(event->pos()), event->button());
		event->accept();
		return;
	}
	 if (m_currentMode == Mode_ExportBox && event->button() == Qt::RightButton)
	 {
		 if (m_exportBoxItem)
		 {
			 QRectF exprotRect = m_exportBoxItem->rect();

			 QPixmap resultpix = exportRegionData(exprotRect, Export_HighRes, false);
			 if (!resultpix.isNull())
			 {
				 emit sigRegionExported(resultpix);
			 }
		 }
		 event->accept();
		 return;
	 }
  
	if (m_currentMode == Mode_Browse && event->button() == Qt::LeftButton)
	{
		if (event->modifiers() & Qt::ShiftModifier) {
			setDragMode(QGraphicsView::RubberBandDrag);
			QGraphicsView::mousePressEvent(event);
			return;
		}
		else {
			setDragMode(QGraphicsView::NoDrag);

			QList<QGraphicsItem*> items = getVisualItems(event->pos());
			DefectShapeItem* hitDefect = nullptr;
			for (auto item : items) {
				if (DefectShapeItem* defect = dynamic_cast<DefectShapeItem*>(item)) {
					hitDefect = defect;
					break;
				}
			}

			if (hitDefect) {
				if (!hitDefect->isSelected()) {
					m_scene->clearSelection();
					hitDefect->setSelected(true);
				}
				// ?? 核心开启：激活我们自己的上帝拖拽引擎�?
				m_isDraggingDefects = true;
				m_lastDragScenePos = mapToScene(event->pos());
				event->accept();
				return;
			}
			else {
				m_scene->clearSelection();
			}
		}
	}

	if (event->button() == Qt::RightButton) {
		QList<QGraphicsItem*> items = getVisualItems(event->pos());
		emit sigContextMenuRequested(event->globalPos(), items);
		event->accept();
		return;
	}

    QGraphicsView::mousePressEvent(event);
}

void TiledGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{

	if (m_isDraggingDefects && event->button() == Qt::LeftButton) {
		m_isDraggingDefects = false;

		// 遍历所有选中的兄弟，挨个结算最终坐标写入数据库
		for (QGraphicsItem* item : m_scene->selectedItems()) {
			if (DefectShapeItem* defect = dynamic_cast<DefectShapeItem*>(item)) {
				defect->finalizeMove();
			}
		}
		event->accept();
		return;
	} 

	//  必须在最前面调用父类，让 Qt 引擎优先结算框选逻辑
	QGraphicsView::mouseReleaseEvent(event);

	//  松开中键
	if (event->button() == Qt::MiddleButton) {
		m_isPanning = false;


		if (m_currentMode == Mode_Browse) {
			setCursor(Qt::ArrowCursor);
		}
		else {
			setCursor(Qt::CrossCursor);
		}
		event->accept();
		return;
	}

	// ==========================================
	//   浏览模式松开左键：结算并抛出框选结�?
	// ==========================================
	if (m_currentMode == Mode_Browse && event->button() == Qt::LeftButton)
	{
		if (dragMode() == QGraphicsView::RubberBandDrag) {

			QList<QGraphicsItem*> allSelected = m_scene->selectedItems();
			QList<DefectShapeItem*> selectedDefects;

			for (auto item : allSelected) {
				if (DefectShapeItem* defect = dynamic_cast<DefectShapeItem*>(item)) {

					if (defect->m_elementType != Type_Disease)
					{
						item->setSelected(false);
						continue;
					}
					selectedDefects.append(defect);
				}
				else {
					item->setSelected(false);
				}
			}

			// 发送信号给 MainWindow
			if (!selectedDefects.isEmpty()) {

				QTimer::singleShot(0, this, [this, selectedDefects]() {
					emit sigDefectsSelected(selectedDefects);
				});
			}
		}
		setDragMode(QGraphicsView::NoDrag);
	}
}

// =========================================================
// ?? 核心逻辑与辅助函�?
// =========================================================

void TiledGraphicsView::setupGraphicsView()
{
    setFrameShape(QFrame::NoFrame);
    // 1. 初始化场�?
    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(QColor(40, 40, 40)); // 深灰色背�?
    setScene(m_scene);

    // =========================================================
    // ?? 渲染配置
    // =========================================================

	// 默认用普�?QWidget viewport。QOpenGLWidget 嵌进复杂 QWidget 界面时，
	// 有些显卡/远程桌面环境会把窗口其它区域残留到视图里，所�?OpenGL 改成显式开启�?
	applyRenderBackend();

    // 强制全屏重绘，避免局部刷新留下的伪影
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // 开启鼠标追�?(即使不按键也能收�?MouseMove，用于显示坐�?
    setMouseTracking(true);
    viewport()->setMouseTracking(true);

    // 变换锚点设为鼠标中心 (缩放时以鼠标为中�?
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);

    // 抗锯�?
    setAlignment(Qt::AlignLeft | Qt::AlignTop); 
    setRenderHint(QPainter::Antialiasing);

    // 滚动条策�?
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 默认模式
    setDragMode(QGraphicsView::NoDrag);
    //setDragMode(QGraphicsView::ScrollHandDrag);
    setFocusPolicy(Qt::StrongFocus);
    setFocus(); // 启动时获取焦�?

	m_vecCp3Manager = new DefectManager(m_scene, this);
	m_vecPlatformManager = new DefectManager(m_scene, this);
	m_vecChainManager = new DefectManager(m_scene, this);
	m_defectManager = new DefectManager(m_scene, this);
	m_vecTunnelLocManager = new DefectManager(m_scene, this);
	m_vecRingInfoManager = new DefectManager(m_scene, this);
	m_vecSectionManager = new DefectManager(m_scene, this);
	m_vecReAutoRingManager = new DefectManager(m_scene, this);

}

void TiledGraphicsView::setRenderBackend(RenderBackend backend)
{
	if (m_renderBackend == backend)
	{
		return;
	}

	m_renderBackend = backend;
	applyRenderBackend();
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	if (viewport())
	{
		viewport()->setMouseTracking(true);
		viewport()->update();
	}
}

void TiledGraphicsView::applyRenderBackend()
{
	QWidget* newViewport = nullptr;

	if (m_renderBackend == RenderBackend_OpenGL)
	{
		QSurfaceFormat format;
		format.setDepthBufferSize(24);
		format.setStencilBufferSize(8);
		format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);

		QOpenGLWidget* glViewport = new QOpenGLWidget(this);
		glViewport->setFormat(format);
		glViewport->setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
		glViewport->setAutoFillBackground(false);
		glViewport->setAttribute(Qt::WA_OpaquePaintEvent, true);
		glViewport->setAttribute(Qt::WA_NoSystemBackground, true);
		newViewport = glViewport;
	}
	else
	{
		QWidget* rasterViewport = new QWidget(this);
		rasterViewport->setAutoFillBackground(true);
		rasterViewport->setAttribute(Qt::WA_OpaquePaintEvent, true);
		newViewport = rasterViewport;
	}

	setViewport(newViewport);
}

void TiledGraphicsView::setupConnections()
{
    // 2. 初始化防抖定时器
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(15); // 15ms 延迟

    auto triggerScroll = [this]() {
		m_debounceTimer->start();
        };

    // 3. 监听滚动条变�?
    connect( horizontalScrollBar(), &QScrollBar::valueChanged, this, triggerScroll);
    connect( verticalScrollBar(), &QScrollBar::valueChanged, this, triggerScroll); 
    connect(m_debounceTimer, &QTimer::timeout, this, [this]()
	{
		updateVisibleTiles();
		emitViewCenterChanged();
	});

	auto onSilderReleased = [this]()
	{ 
		m_debounceTimer->start();
	};
	connect(horizontalScrollBar(), &QScrollBar::sliderReleased, this, onSilderReleased);
	connect(verticalScrollBar(), &QScrollBar::sliderReleased, this, onSilderReleased);

	 

	// 获取视图窗口区间
	connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [=]() {
		QRect viewPortRect = viewport()->rect();

		// 1. 获取鼠标�?Scene 中的坐标
		QPointF sceneTopLPos = mapToScene(viewPortRect.topLeft());
		QPointF sceneBottomRPos = mapToScene(viewPortRect.bottomRight());
		QPointF centerPos = QPointF((sceneTopLPos.x() + sceneBottomRPos.x()) / 2.0, sceneTopLPos.y());

		QString strNameL, strNameR;
		int xL, yL, xR, yR;
		bool bL = GlobalSceneToMap(sceneTopLPos, strNameL, xL, yL);
		bool bR = GlobalSceneToMap(sceneBottomRPos, strNameR, xR, yR);

		if (bL && bR)
		{
			emit sigViewInfoChanged(strNameL, xL, yL,
				strNameR, xR, yR);
		}

	});

}

void TiledGraphicsView::emitViewCenterChanged()
{
	// 这个信号只描�?SDK 自己的连续位置，不掺业务里程，保�?SDK 和项目业务解耦�?
	emit sigViewCenterSceneChanged(currentCenterScenePos());
	emit sigViewBottomAnchorChanged(currentBottomAnchor());
}

void TiledGraphicsView::emitUserViewBottomAnchorChanged()
{
	// 这条线只表示“用户真的在浏览”，上层拿它�?2D/3D 联动，避免程序定位后自己打回自己�?
	emit sigUserViewBottomAnchorChanged(currentBottomAnchor());
}

void TiledGraphicsView::updateVisibleTiles()
{
	
	QElapsedTimer timer;
	timer.start();

	// 1. 获取可视区域
	QRect viewportRect = viewport()->rect();
	QRectF visibleSceneRect = mapToScene(viewportRect).boundingRect();

	// =========================================================
	// ?? 终极丝滑魔法：双层空间结界！
	// =========================================================

	// 【内层结界】：高清图的 Buffer (稍微外扩一点点，防边缘穿帮)
	double highResBuffer = 50.0;
	QRectF highResRect = visibleSceneRect.adjusted(-highResBuffer, -highResBuffer, highResBuffer, highResBuffer);

	// 【外层结界】：缩略图的“雷达预警区�?(向外狂扩 1.5 个屏幕的宽度�?
	// 这意味着用户还没滚到那里，提�?1.5 个屏幕的缩略图就已经在后台悄悄解压了
	double prefetchX = visibleSceneRect.width() * 1.5;
	double prefetchY = visibleSceneRect.height() * 1.5;
	QRectF prefetchRect = visibleSceneRect.adjusted(-prefetchX, -prefetchY, prefetchX, prefetchY);

	// 整图源一张图片就是一个完整块。沿�?50px 高清窗口会导致图片刚进入视口才开始解码，
	// 普通滚动稍快就容易露出黑底，所以整图模式单独放大预加载窗口，但仍不做全量常驻�?
	const double wholeThumbX = visibleSceneRect.width() * 1.0;
	const double wholeThumbY = visibleSceneRect.height() * 3.0;
	const QRectF wholeThumbRect = visibleSceneRect.adjusted(-wholeThumbX, -wholeThumbY, wholeThumbX, wholeThumbY);
	const double wholeImageScreenCount = m_isFastScrolling ? 0.5 : 2.0;
	const double wholeImageX = visibleSceneRect.width() * 1.0;
	const double wholeImageY = visibleSceneRect.height() * wholeImageScreenCount;
	const QRectF wholeImageRect = visibleSceneRect.adjusted(-wholeImageX, -wholeImageY, wholeImageX, wholeImageY);

	m_currentScale = transform().m11();

	// 2. 遍历大管家：统一调度所�?Item 的生死与预加�?
	for (auto item : m_items) {
		if (!item)
		{
			continue;
		}

		// --- ?? A. 缩略图雷达预加载逻辑 ---
		QRectF itemRect = item->sceneBoundingRect();
		AbstractTileSource* source = item->getSource();
		const bool isWholeImage = source && source->sourceMode() == ImageSourceMode::WholeImage;
		/*
		
		*/
		
		if (isWholeImage)
		{
			if (wholeThumbRect.intersects(itemRect))
			{
				item->ensureThumbnailRequested();
			}
			else
			{
				item->releaseThumbnail();
			}

			if (wholeImageRect.intersects(itemRect))
			{
				item->updateVisibleTiles(wholeImageRect, m_currentScale, m_lodThresholdMultiplier);
			}
			else
			{
				item->unloadAll();
			}
			continue;
		}

		if (prefetchRect.intersects(itemRect)) {
			// 只要进入雷达区，立刻发起异步请求�?
			item->ensureThumbnailRequested();
		}
		else {
			// 如果连雷达区都跌出去了，立刻销毁缩略图，严控内存！
			item->releaseThumbnail();
		}
		if (m_isFastScrolling) {
			item->unloadAll();
		}
		else
		{
			// --- ?? B. 高清�?LOD 降级逻辑 ---
			// 高清图绝不能�?prefetchRect，必须用极其克制�?highResRect
			item->updateVisibleTiles(highResRect, m_currentScale, m_lodThresholdMultiplier);
		} 
	}
	viewport()->update();
}

void TiledGraphicsView::undoLastDrawPoint()
{
    // 既然画图�?Tool 接管，撤销当然也直接甩锅给 Tool 去做�?
    if (m_currentMode == Mode_Draw && m_currentTool) {
        m_currentTool->undoLastPoint();
    }
}

void TiledGraphicsView::cancelCurrentDrawing()
{
    // 只有在绘图模式下，且工具存在时，才通知工具清空画面
    if (m_currentMode == Mode_Draw && m_currentTool) {
        m_currentTool->cancelDrawing();
    }
}

 

double TiledGraphicsView::getFitScale() const
{
    if (m_items.isEmpty()) return 1.0;

    // 获取图片真实的物理边�?
    QRectF itemsRect = m_scene->itemsBoundingRect();
    if (itemsRect.isEmpty()) return 1.0;

    QSize viewSize = viewport()->size();

    if (!isVerticalLayout(m_orientation)) {
        // 横向拼接：进度方向是 X，初始按高度铺满�?
		int availableHeight = viewSize.height();
		if (itemsRect.width() * (double)availableHeight / itemsRect.height() > viewSize.width()
			&& horizontalScrollBar()
			&& !horizontalScrollBar()->isVisible())
		{
			// viewport() 已经扣掉可见滚动条；这里只在滚动条尚未出现但即将出现时预扣一次�?
			availableHeight = qMax(1, availableHeight - horizontalScrollBar()->sizeHint().height());
		}
        return (double)availableHeight / itemsRect.height();
    }
    else {
        // 纵向拼接：进度方向是 Y，初始按宽度铺满�?
		int availableWidth = viewSize.width();
		if (itemsRect.height() * (double)availableWidth / itemsRect.width() > viewSize.height()
			&& verticalScrollBar()
			&& !verticalScrollBar()->isVisible())
		{
			// viewport() 已经扣掉可见滚动条；这里只在滚动条尚未出现但即将出现时预扣一次�?
			availableWidth = qMax(1, availableWidth - verticalScrollBar()->sizeHint().width());
		}
        return (double)availableWidth / itemsRect.width();
    }
}

QList<QGraphicsItem*> TiledGraphicsView::getVisualItems(QPoint viewPos)
{
	// ?? 1. 获取设备像素�?(关键修改)
	// 如果宿主程序没开缩放，高分屏下这里会返回 1.25, 1.5, 2.0 �?
	// 如果开了缩放，或者普通屏，这里通常�?1.0
	qreal ratio = viewport()->devicePixelRatio();

	// 2. 设定基础容差 (逻辑像素)
	int baseTolerance = 5;

	// 缩放很小时（上帝视角），线条很细，很难点中，所以要大幅扩大容差
	if (m_currentScale < 0.2) {
		baseTolerance = 10;
	}

	// ?? 3. 计算最终容�?
	// 这样无论在什么屏幕上，物理点击面积都是差不多大的，手感一�?
	int finalTolerance = static_cast<int>(baseTolerance * ratio);

	// 4. 构造点击矩�?
	QRect viewRect(
		viewPos.x() - finalTolerance,
		viewPos.y() - finalTolerance,
		finalTolerance * 2,
		finalTolerance * 2
	);

	//
	//
	return m_scene->items(mapToScene(viewRect));
}
//
 //

void TiledGraphicsView::setDrawingGeometry(DrawShape shapeType)
{
	if (m_curDrawShape != shapeType)
	{
		m_curDrawShape = shapeType;
		startDrawingDefect(shapeType);
	}

}

void TiledGraphicsView::startDrawingDefect(DrawShape shapeType)
{
	// 切换到绘图模�?
	m_currentMode = Mode_Draw;

	if (m_currentTool) {
		m_currentTool->deactivate();
	//delete m_currentTool;
	}

	m_currentTool->ChangeShapeType(shapeType);
	setDragMode(QGraphicsView::NoDrag);
	setCursor(Qt::CrossCursor);

	

	// ==========================================
	// ?? 修复点：动态更新左上角提示文字
	// ==========================================
	if (shapeType == Shape_Point) {
		DrawModelStr = QString::fromLocal8Bit("绘图模式 [点]");
	}
	else if (shapeType == Shape_Line) {
		DrawModelStr = QString::fromLocal8Bit("绘图模式 [线]");
	}
	else if (shapeType == Shape_Polygon) {
		DrawModelStr = QString::fromLocal8Bit("绘图模式 [面]");
	}

	// ?? 强制触发一次可见性更新，立刻刷新左上角的 HUD 文字
	updateVisibleTiles();
}

void TiledGraphicsView::setLodLevel(int level) {
    // 1. 限制范围 1~10
    if (level < 1) level = 1;
    if (level > 10) level = 10;

    // 2. �?1~10 映射�?1.5 ~ 0.5
    // Level 1  -> 1.5 (最晚显�?
    // Level 10 -> 0.5 (最早显�?

    // 步长 = (最大系�?- 最小系�? / (最大等�?- 1)
    // 步长 = (1.5 - 0.5) / 9.0 �?0.1111

    double step = 1.25 / 9.0;

    m_lodThresholdMultiplier = 1.5 - (level - 1) * step;

    // 3. 立即刷新
    updateVisibleTiles();
}



bool TiledGraphicsView::GlobalSceneToMap(QPointF pt, QString & imageName, int& localX, int& localY)
{
	for (TunnelSectionItem* item : qAsConst(m_items))
	{
		if (!item || !item->sceneBoundingRect().contains(pt))
		{
			continue;
		}

		QPointF localPos = item->mapFromScene(pt);
		const QRectF localRect = item->boundingRect();
		if (!localRect.contains(localPos))
		{
			continue;
		}

		imageName = item->getImageName();
		localX = qBound((int)localRect.left(), qRound(localPos.x()), (int)localRect.right());
		localY = qBound((int)localRect.top(), qRound(localPos.y()), (int)localRect.bottom());

		return true;
	}

	return false;
}

void TiledGraphicsView::setHighLightElement(int uuid, ElementType ele)
{

	switch (ele)
	{
	case Type_Cp3:
	{

		break;
	}
	case Type_Chain:
	{

		break;
	}
	case Type_Disease:
	{
		DefectShapeItem * item = m_defectManager->getItemUseId(uuid);

		if (item == NULL)
		{
			return;
		}

		QRectF sceneRect = item->sceneBoundingRect();
		qreal margin = 1.85;

		QRectF targetRect(0, 0, sceneRect.width()* margin, sceneRect.height()* margin);
		targetRect.moveCenter(sceneRect.center());

		// 确保图像不失心变�?
		fitInView(targetRect, Qt::KeepAspectRatio);

		item->setSelected(true);

		break;
	}
	case Type_Ring:
	{

		break;
	}
	case Type_Section:
	{

		break;
	}
	case Type_Platform:
	{

		break;
	}
	default:
		break;
	}

	updateVisibleTiles();      
}

void TiledGraphicsView::mouseDoubleClickEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (event->button() == Qt::MouseEventCreatedDoubleClick)
		{
			// 获取鼠标下的物体
			QList<QGraphicsItem*> items = getVisualItems(event->pos());

			emit sigDoubleClickedLeft(mapToScene(event->pos()), items);
		}

	}
}

//TODO 新增功能 20260310 增加图像导出功能�?
QPixmap TiledGraphicsView::exportRegionData(const QRectF& sceneRect, ExportQuality quality, bool drawDefects)
{
	if (sceneRect.isEmpty() || m_items.isEmpty()) return QPixmap();

	QSize targetSize(qCeil(sceneRect.width()), qCeil(sceneRect.height()));
	/*if (targetSize.width() > 16384 || targetSize.height() > 16384) {
		qWarning() << QString::fromLocal8Bit("?? 截取区域过大，已自动等比缩小�?); 
		targetSize.scale(16384, 16384, Qt::KeepAspectRatio);
	}*/ 
	//QImage resultImage(targetSize, QImage::Format_ARGB32_Premultiplied);
	QImage resultImage(targetSize, QImage::Format_RGB888); 
	resultImage.fill(Qt::white);

	QPainter painter(&resultImage);
	painter.setRenderHint(QPainter::Antialiasing,false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform,false);
	 
	painter.setWindow(sceneRect.toRect());
	painter.setViewport(resultImage.rect());

	for (TunnelSectionItem* item : m_items) {
		QRectF itemRect = item->sceneBoundingRect();
		if (!sceneRect.intersects(itemRect)) continue; 
		AbstractTileSource* source = item->getSource();

		if (quality == Export_Thumbnail) {
			QImage thumb(source->getThumbnailImage());
			if (!thumb.isNull()) { 
				painter.drawImage(itemRect, thumb);
			}
		}
		else {
			int tSize = item->getTileSize();
			QRectF localIntersect = item->mapRectFromScene(sceneRect.intersected(itemRect)); 
			const QRectF itemLocalRect = item->boundingRect();
			const QRectF sourceIntersect = sdkVisibleLocalRectToSourceRect(localIntersect, itemLocalRect, source);
			int startCol = qMax(0, qFloor(sourceIntersect.left() / tSize));
			int endCol = qFloor(sourceIntersect.right() / tSize);
			int startRow = qMax(0, qFloor(sourceIntersect.top() / tSize));
			int endRow = qFloor(sourceIntersect.bottom() / tSize);


			const QList<TileImageData> tiles = source->tileDataRange(startCol, endCol, startRow, endRow);
			for (const TileImageData& tileData : tiles) {
				QImage tile = QImage::fromData(tileData.data, "JPG");
				if (tile.isNull()) continue;

				const QRectF tileLocalRect(
					tileData.col * tSize,
					tileData.row * tSize,
					tile.width(),
					tile.height()
				);
				drawSdkTileImage(painter, itemRect, tileLocalRect, tile, source);
			}
		}
	}

	// =========================================================
	// 绘制矢量病害�?
	// =========================================================
	if (drawDefects) {
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		// 1. 隐藏隧道底图
		for (TunnelSectionItem* item : m_items) {
			item->hide();
		}

		// ?? 2. 核心修复：临时抽�?Scene 的“黑背景”，防止它覆盖我们拼好的图片�?
		QBrush oldBgBrush = m_scene->backgroundBrush();
		m_scene->setBackgroundBrush(Qt::NoBrush);

		// 3. 充当翻译官：统一两边的坐标系
		painter.setWindow(0, 0, targetSize.width(), targetSize.height());
		painter.setViewport(resultImage.rect());

		// 4. 画病害！此时因为背景�?NoBrush，病害会直接以透明底盖在我们的图片�?
		m_scene->render(&painter, resultImage.rect(), sceneRect);

		// ?? 5. 打扫战场：把背景色和底图全还给界面，做到神不知鬼不觉
		m_scene->setBackgroundBrush(oldBgBrush);
		for (TunnelSectionItem* item : m_items) {
			item->show();
		}
	}

	painter.end();
	return QPixmap::fromImage(resultImage);
}





//TODO 新增功能 增加图像导出功能，返�?cv::Mat 灰度图格�?(极速单通道零拷贝版)
cv::Mat TiledGraphicsView::exportRegionGrayMat(const QRectF& sceneRect, ExportQuality quality, bool drawDefects)
{
	if (sceneRect.isEmpty() || m_items.isEmpty()) return cv::Mat();

	QSize targetSize(qCeil(sceneRect.width()), qCeil(sceneRect.height()));
	//if (targetSize.width() > 16384 || targetSize.height() > 16384) {
	//	qWarning() << QString::fromLocal8Bit("?? 截取区域过大，已自动等比缩小�?);
	//	targetSize.scale(16384, 16384, Qt::KeepAspectRatio);
	//}

	QImage resultImage(targetSize, QImage::Format_Grayscale8);
	resultImage.fill(Qt::white);

	QPainter painter(&resultImage);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

	painter.setWindow(sceneRect.toRect());
	painter.setViewport(resultImage.rect());

	for (TunnelSectionItem* item : m_items) {
		QRectF itemRect = item->sceneBoundingRect();
		if (!sceneRect.intersects(itemRect)) continue;

		AbstractTileSource* source = item->getSource();

		if (quality == Export_Thumbnail) {
			// 缩略图逻辑（按需放开�?
		}
		else {
			int tSize = item->getTileSize();
			QRectF localIntersect = item->mapRectFromScene(sceneRect.intersected(itemRect));
			const QRectF itemLocalRect = item->boundingRect();
			const QRectF sourceIntersect = sdkVisibleLocalRectToSourceRect(localIntersect, itemLocalRect, source);

			int startCol = qMax(0, qFloor(sourceIntersect.left() / tSize));
			int endCol = qFloor(sourceIntersect.right() / tSize);
			int startRow = qMax(0, qFloor(sourceIntersect.top() / tSize));
			int endRow = qFloor(sourceIntersect.bottom() / tSize);

			const QList<TileImageData> tiles = source->tileDataRange(startCol, endCol, startRow, endRow);
			for (const TileImageData& tileData : tiles) {
				QImage tile = QImage::fromData(tileData.data, "JPG");
				if (tile.isNull()) continue;

				const QRectF tileLocalRect(
					tileData.col * tSize,
					tileData.row * tSize,
					tile.width(),
					tile.height()
				);
				drawSdkTileImage(painter, itemRect, tileLocalRect, tile, source);
			}
		}
	}

	// 绘制矢量病害�?
	if (drawDefects) {
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

		for (TunnelSectionItem* item : m_items) {
			item->hide();
		}

		QBrush oldBgBrush = m_scene->backgroundBrush();
		m_scene->setBackgroundBrush(Qt::NoBrush);

		painter.setWindow(0, 0, targetSize.width(), targetSize.height());
		painter.setViewport(resultImage.rect());

		// ?? 这里哪怕你界面上画的是大红大黄的线段，最终砸�?finalMat 也会自动变成灰阶线段
		m_scene->render(&painter, resultImage.rect(), sceneRect);

		m_scene->setBackgroundBrush(oldBgBrush);
		for (TunnelSectionItem* item : m_items) {
			item->show();
		}
	}

	painter.end();

	cv::Mat finalMat(resultImage.height(),
		resultImage.width(),
		CV_8UC1,
		resultImage.bits(),
		resultImage.bytesPerLine());
	return finalMat.clone();
}


cv::Mat TiledGraphicsView::exportRegionMat(const QRectF& sceneRect,
	ExportQuality quality,
	bool drawDefects,
	bool returnBgr)
{
	if (sceneRect.isEmpty() || m_items.isEmpty()) {
		return cv::Mat();
	}

	QSize targetSize(qCeil(sceneRect.width()), qCeil(sceneRect.height()));
	if (targetSize.width() <= 0 || targetSize.height() <= 0) {
		return cv::Mat();
	}

	//if (targetSize.width() > 16384 || targetSize.height() > 16384) {
	//	qWarning() << QString::fromLocal8Bit("?? 截取区域过大，已自动等比缩小�?);
	//	targetSize.scale(16384, 16384, Qt::KeepAspectRatio);
	//}

	QImage resultImage(targetSize, QImage::Format_RGB888);
	resultImage.fill(Qt::white);

	QPainter painter(&resultImage);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

	painter.setWindow(sceneRect.toRect());
	painter.setViewport(resultImage.rect());

	for (TunnelSectionItem* item : m_items) {
		QRectF itemRect = item->sceneBoundingRect();
		if (!sceneRect.intersects(itemRect)) {
			continue;
		}

		AbstractTileSource* source = item->getSource();
		if (!source) {
			continue;
		}

		if (quality == Export_Thumbnail) {
			// 你当前原函数里缩略图逻辑也是注释掉的，这里先保持一�?
			continue;
		}

		int tSize = item->getTileSize();
		QRectF localIntersect = item->mapRectFromScene(sceneRect.intersected(itemRect));
		const QRectF itemLocalRect = item->boundingRect();
		const QRectF sourceIntersect = sdkVisibleLocalRectToSourceRect(localIntersect, itemLocalRect, source);

		int startCol = qMax(0, qFloor(sourceIntersect.left() / tSize));
		int endCol = qFloor(sourceIntersect.right() / tSize);
		int startRow = qMax(0, qFloor(sourceIntersect.top() / tSize));
		int endRow = qFloor(sourceIntersect.bottom() / tSize);

		const QList<TileImageData> tiles = source->tileDataRange(startCol, endCol, startRow, endRow);
		for (const TileImageData& tileData : tiles) {
			QImage tile = QImage::fromData(tileData.data, "JPG");
			if (tile.isNull()) {
				continue;
			}

			if (tile.format() != QImage::Format_RGB888) {
				tile = tile.convertToFormat(QImage::Format_RGB888);
			}

			const QRectF tileLocalRect(
				tileData.col * tSize,
				tileData.row * tSize,
				tile.width(),
				tile.height()
			);
			drawSdkTileImage(painter, itemRect, tileLocalRect, tile, source);
		}
	}

	// =========================================================
	// 绘制矢量病害�?
	// 这里仍然是画�?resultImage，但 resultImage 背后就是 matRgb.data
	// =========================================================
	if (drawDefects) {
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

		for (TunnelSectionItem* item : m_items) {
			item->hide();
		}

		QBrush oldBgBrush = m_scene->backgroundBrush();
		m_scene->setBackgroundBrush(Qt::NoBrush);

		painter.setWindow(0, 0, targetSize.width(), targetSize.height());
		painter.setViewport(resultImage.rect());

		m_scene->render(&painter, resultImage.rect(), sceneRect);

		m_scene->setBackgroundBrush(oldBgBrush);

		for (TunnelSectionItem* item : m_items) {
			item->show();
		}
	}

	painter.end();

	cv::Mat matRgb(resultImage.height(),
		resultImage.width(),
		CV_8UC3,
		resultImage.bits(),
		resultImage.bytesPerLine());

	// 如果后续还要 Qt 显示、或者你只是转灰度识别，可以直接 return matRgb，最�?
	if (!returnBgr) {
		return matRgb.clone();
	}

	// OpenCV �?imwrite / 大多数算法默认按 BGR 解释彩色�?
	// 需要保存正常颜色时，再�?BGR
	cv::Mat matBgr;
	cv::cvtColor(matRgb, matBgr, cv::COLOR_RGB2BGR);
	return matBgr;
}


QPixmap TiledGraphicsView::exportRegionDataToWord(const QRectF& sceneRect, ExportQuality quality /*= Export_HighRes*/, bool drawDefects /*= true*/)
{
	if (sceneRect.isEmpty() || m_items.isEmpty()) return QPixmap();

	QSize targetSize(qCeil(sceneRect.width()), qCeil(sceneRect.height()));
	/*if (targetSize.width() > 16384 || targetSize.height() > 16384) {
	qWarning() << QString::fromLocal8Bit("?? 截取区域过大，已自动等比缩小�?);
	targetSize.scale(16384, 16384, Qt::KeepAspectRatio);
	}*/

	//QImage resultImage(targetSize, QImage::Format_ARGB32_Premultiplied);
	QImage resultImage(targetSize, QImage::Format_RGB888);

	// ?? 修复�?1：JPG不支持透明底，一律填成干净的白�?(或你需要的底图颜色)
	resultImage.fill(Qt::white);

	QPainter painter(&resultImage);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

	// ?? 修复�?2：上帝级坐标系映射！
	// 告诉画家：“你现在的画板代表的是真实的物理 SceneRect�?
	painter.setWindow(sceneRect.toRect());
	painter.setViewport(resultImage.rect());

	for (TunnelSectionItem* item : m_items) {
		QRectF itemRect = item->sceneBoundingRect();
		if (!sceneRect.intersects(itemRect)) continue;

		AbstractTileSource* source = item->getSource();

		if (quality == Export_Thumbnail) {
			/*	QImage thumb = source->getThumbnailImage();
				if (!thumb.isNull()) {
					painter.drawImage(itemRect, thumb);
				}*/
		}
		else {
			int tSize = item->getTileSize();
			QRectF localIntersect = item->mapRectFromScene(sceneRect.intersected(itemRect));
			const QRectF itemLocalRect = item->boundingRect();
			const QRectF sourceIntersect = sdkVisibleLocalRectToSourceRect(localIntersect, itemLocalRect, source);

			int startCol = qMax(0, qFloor(sourceIntersect.left() / tSize));
			int endCol = qFloor(sourceIntersect.right() / tSize);
			int startRow = qMax(0, qFloor(sourceIntersect.top() / tSize));
			int endRow = qFloor(sourceIntersect.bottom() / tSize);

			const QList<TileImageData> tiles = source->tileDataRange(startCol, endCol, startRow, endRow);
			for (const TileImageData& tileData : tiles) {
				QImage tile = QImage::fromData(tileData.data, "JPG");
				if (tile.isNull()) continue;

				const QRectF tileLocalRect(
					tileData.col * tSize,
					tileData.row * tSize,
					tile.width(),
					tile.height()
				);
				drawSdkTileImage(painter, itemRect, tileLocalRect, tile, source);
			}
		}
	}

	// =========================================================
	// 绘制矢量病害�?
	// =========================================================
	if (drawDefects) {
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		// 1. 隐藏隧道底图
		for (TunnelSectionItem* item : m_items) {
			item->hide();
		}

		// ?? 2. 核心修复：临时抽�?Scene 的“黑背景”，防止它覆盖我们拼好的图片�?
		QBrush oldBgBrush = m_scene->backgroundBrush();
		m_scene->setBackgroundBrush(Qt::NoBrush);

		// 3. 充当翻译官：统一两边的坐标系
		painter.setWindow(0, 0, targetSize.width(), targetSize.height());
		painter.setViewport(resultImage.rect());

		// 4. 画病害！此时因为背景�?NoBrush，病害会直接以透明底盖在我们的图片�?
		m_scene->render(&painter, resultImage.rect(), sceneRect);

		// ?? 5. 打扫战场：把背景色和底图全还给界面，做到神不知鬼不觉
		m_scene->setBackgroundBrush(oldBgBrush);
		for (TunnelSectionItem* item : m_items) {
			item->show();
		}
	}

	painter.end();

	if (resultImage.width()>1500)
	{
		resultImage = resultImage.scaledToWidth(1500, Qt::SmoothTransformation);
	}
	return QPixmap::fromImage(resultImage);
}

// 指定某图片名获取该Mat指针
cv::Mat TiledGraphicsView::getMatByImageName(QString qstrImageName)
{
	QString qstrFloderPath;
	QString qstrFindName = qstrImageName.remove(".jpg");
	for (TunnelSectionItem* item : m_items)
	{
		if (item->getSource()->oriImageName() == qstrFindName)
		{
			qstrFloderPath = item->getSource()->getDbPath();
			break;
		}
	}


	// 获取所有X_X.jpg文件
	QDir dir(qstrFloderPath);
	if (!dir.exists())
	{
		return cv::Mat();
	}

	QFileInfoList fileList = dir.entryInfoList(QStringList() << "*.jpg", QDir::Files | QDir::NoSymLinks, QDir::NoSort);

	// 每列一个数�?
	vector<vector<pair<QString,QString>>> vecVecColsImages;
	// 21680宽度是固定的
	vecVecColsImages.resize(22);
	for (const QFileInfo& fileInfo : fileList)
	{
		QString qstrFileName = fileInfo.fileName();
		qstrFileName = qstrFileName.remove(".jpg");
		QString absoluPath = fileInfo.absoluteFilePath();

		if (qstrFileName.contains("_"))
		{
			QStringList qstrList = qstrFileName.split("_");
			if (qstrList.size()==0)
			{
				continue;
			}
			bool isOk = false;
			int nIndex = qstrList[0].toInt(&isOk);
			if (isOk && nIndex < vecVecColsImages.size())
			{
				vecVecColsImages[nIndex].push_back({ qstrFileName, absoluPath });
			}
		}
	}

	// 排序
	for (size_t i = 0; i < vecVecColsImages.size(); i++)
	{
		sort(vecVecColsImages[i].begin(), vecVecColsImages[i].end(), [](pair<QString, QString>& a, pair<QString, QString>&b)
		{
			return  a.first.split("_")[1].toInt() < b.first.split("_")[1].toInt();
		});
	}


	vector<cv::Mat> vecVconcatMat;
	// 先按列读�?
	for (size_t i = 0; i < vecVecColsImages.size(); i++)
	{
		vector<cv::Mat> vecMats;
		for (size_t j = 0; j < vecVecColsImages[i].size(); j++)
		{
			string strPath = vecVecColsImages[i][j].second.toLocal8Bit();
			cv::Mat srcImg = cv::imread(strPath, cv::IMREAD_UNCHANGED);
			vecVconcatMat.push_back(srcImg);
		}
		cv::Mat vconcatMat;
		cv::vconcat(vecMats, vconcatMat);
		vecVconcatMat.push_back(vconcatMat);
	}

	// 检查高度是否都一�?
	bool isSame = true;
	for (int i = 1; i < vecVconcatMat.size(); i++)
	{
		if (vecVconcatMat[i].rows != vecVconcatMat[0].rows || vecVconcatMat[i].cols != vecVconcatMat[0].cols)
		{
			isSame = false;
		}
	}
	if (!isSame)
	{
		return cv::Mat();
	}

	cv::Mat result;
	cv::hconcat(vecVconcatMat, result);
	return result;

}

double TiledGraphicsView::getImageHeight()
{
	if (m_items.size() > 0)
	{
		return (double)m_items[0]->boundingRect().height() - 10;
	}

	return 0.0;
}

double TiledGraphicsView::getImageWidth()
{ 
	if (m_items.size() > 0)
	{ 
		return (double)m_items[0]->boundingRect().width();
	}

	return 0.0;
}

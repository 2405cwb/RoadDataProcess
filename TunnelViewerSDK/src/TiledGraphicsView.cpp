#include "TiledGraphicsView.h"
#include "../include/tools/AbstractTool.h"
#include "../include/tools/DefectDrawTool.h"
#include <QOpenGLWidget>
#include <QScrollBar>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QDebug>
#include <QApplication>
#include <QFileInfo>
#include <QtMath>
#include <QMenu>
#include <QMessageBox>
#include <QSurfaceFormat>
#include <QPointer>
#include <QRunnable>
#include <QThread>
#include <QMetaObject>
#include <QStyleOptionGraphicsItem>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <algorithm>

#include "./tools/DefectDrawTool.h"
#include "./items/DefectShapeItem.h"
#include "VirtualImageSequence.h"
#include "AsyncImageLoader.h"

// TODO 卷帘分析新增：该图元位于基准图层与病害图层之间，
// 从而不改变现有病害、环片等成果图元的绘制逻辑。
class CurtainCompareItem : public QGraphicsItem
{
public:
	explicit CurtainCompareItem(TiledGraphicsView* view)
		: m_view(view)
	{
		setAcceptedMouseButtons(Qt::NoButton);
		setZValue(-500.0);
	}

	QRectF boundingRect() const override { return m_rect; }

	void setRect(const QRectF& rect)
	{
		if (m_rect == rect) return;
		prepareGeometryChange();
		m_rect = rect;
	}

	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) override
	{
		if (m_view) m_view->paintCurtainCompare(painter, option->exposedRect);
	}

private:
	TiledGraphicsView* m_view = nullptr;
	QRectF m_rect;
};

namespace {

	double mileageMin(const DbImageInfo& info)
	{
		return qMin(info.realStartMileage, info.realEndMileage);
	}

	double mileageMax(const DbImageInfo& info)
	{
		return qMax(info.realStartMileage, info.realEndMileage);
	}

	double mileageAtSceneX(const DbImageInfo& info, const QRectF& sceneRect, double sceneX)
	{
		if (sceneRect.width() <= 0.0) return info.realStartMileage;
		const double ratio = qBound(0.0,
			(sceneX - sceneRect.left()) / sceneRect.width(), 1.0);
		return info.realStartMileage
			+ ratio * (info.realEndMileage - info.realStartMileage);
	}

	double sceneXAtMileage(const DbImageInfo& info, const QRectF& sceneRect, double mileage)
	{
		const double delta = info.realEndMileage - info.realStartMileage;
		if (qFuzzyIsNull(delta)) return sceneRect.left();
		return sceneRect.left()
			+ ((mileage - info.realStartMileage) / delta) * sceneRect.width();
	}

	double imageXAtMileage(const DbImageInfo& info, double mileage)
	{
		const double delta = info.realEndMileage - info.realStartMileage;
		if (qFuzzyIsNull(delta)) return 0.0;
		return ((mileage - info.realStartMileage) / delta) * info.width;
	}

}


namespace {

/*
 * 导出时只临时隐藏明确标记为不可导出的标注，析构后恢复原可见状态。
 * 不改变 Scene、不改业务数据，也不影响旧 drawDefects=false 逻辑。
 */
class AnnotationExportGuard
{
public:
    explicit AnnotationExportGuard(const QMap<QString, AnnotationManager*>& managers)
    {
        QSet<DefectShapeItem*> visited;
        for (AnnotationManager* manager : managers) {
            if (!manager) continue;
            const bool layerExportable = manager->layerState().exportable;
            const QList<DefectShapeItem*> items = manager->items();
            for (DefectShapeItem* item : items) {
                if (!item || visited.contains(item)) continue;
                visited.insert(item);
                if (layerExportable && item->isExportableByPolicy()) continue;
                VisibilityState state;
                state.item = item;
                state.visible = item->isVisible();
                m_states.append(state);
                if (state.visible) item->setVisible(false);
            }
        }
    }

    ~AnnotationExportGuard()
    {
        for (const VisibilityState& state : m_states) {
            if (state.item) state.item->setVisible(state.visible);
        }
    }

private:
    struct VisibilityState {
        QPointer<DefectShapeItem> item;
        bool visible = false;
    };
    QList<VisibilityState> m_states;
};

bool sequenceTraceEnabled()
{
	static const bool enabled = qgetenv("HN_SEQUENCE_TRACE") == QByteArrayLiteral("1");
	return enabled;
}

void applyImageAdjustments(QImage& image, const ImageDisplayAdjustments& adjustments)
{
	if (image.isNull()) return;
	const int brightness = qBound(-100, adjustments.brightness, 100);
	const int contrast = qBound(0, adjustments.contrast, 200);
	const int sharpen = qBound(0, adjustments.sharpen, 100);
	if (brightness == 0 && contrast == 100 && sharpen == 0) return;
	if (image.format() != QImage::Format_RGB888)
		image = image.convertToFormat(QImage::Format_RGB888);

	const double contrastFactor = contrast / 100.0;
	for (int y = 0; y < image.height(); ++y) {
		uchar* row = image.scanLine(y);
		for (int x = 0; x < image.width() * 3; ++x) {
			const int value = qRound((row[x] - 128) * contrastFactor + 128 + brightness);
			row[x] = static_cast<uchar>(qBound(0, value, 255));
		}
	}

	if (sharpen == 0 || image.width() < 3 || image.height() < 3) return;
	// OpenCV unsharp masking: subtract a Gaussian-blurred image from the source.
	// A full slider maps to 3.0 so road texture changes remain visibly apparent.
	cv::Mat source(image.height(), image.width(), CV_8UC3, image.bits(), image.bytesPerLine());
	cv::Mat blurred;
	cv::GaussianBlur(source, blurred, cv::Size(), 1.2, 1.2, cv::BORDER_REPLICATE);
	const double amount = sharpen * 3.0 / 100.0;
	cv::addWeighted(source, 1.0 + amount, blurred, -amount, 0.0, source);
}

class SequenceDecodeTask : public QRunnable
{
public:
    SequenceDecodeTask(TiledGraphicsView* view, const QSharedPointer<ISequenceFrameSource>& source,
        int sourceIndex, bool highResolution, int generation, int requestSerial,
		int thumbnailMaxEdge, const ImageDisplayAdjustments& adjustments)
        : m_view(view), m_source(source), m_sourceIndex(sourceIndex),
		  m_highResolution(highResolution), m_generation(generation), m_requestSerial(requestSerial),
		  m_thumbnailMaxEdge(thumbnailMaxEdge),
		  m_adjustments(adjustments)
    {
        setAutoDelete(true);
    }

    void run() override
    {
		QElapsedTimer decodeTimer;
		decodeTimer.start();
		if (sequenceTraceEnabled())
			qInfo().noquote() << "[HN_SEQUENCE_TRACE][DECODE_BEGIN]"
				<< "index=" << m_sourceIndex << "high=" << m_highResolution
				<< "layoutGen=" << m_generation << "request=" << m_requestSerial;
        QImage image;
        if (!m_source.isNull()) {
            image = m_highResolution
                ? m_source->decodeFullImage(static_cast<quint64>(m_sourceIndex))
                : m_source->decodeThumbnail(static_cast<quint64>(m_sourceIndex), QSize(m_thumbnailMaxEdge, m_thumbnailMaxEdge));
        }
		applyImageAdjustments(image, m_adjustments);
		if (sequenceTraceEnabled())
			qInfo().noquote() << "[HN_SEQUENCE_TRACE][DECODE_END]"
				<< "index=" << m_sourceIndex << "high=" << m_highResolution
				<< "layoutGen=" << m_generation << "request=" << m_requestSerial
				<< "null=" << image.isNull() << "size=" << image.size()
				<< "elapsedMs=" << decodeTimer.elapsed();
        if (!m_view.isNull()) {
            QMetaObject::invokeMethod(m_view.data(), "onSequenceImageDecoded", Qt::QueuedConnection,
                Q_ARG(int, m_sourceIndex), Q_ARG(bool, m_highResolution),
				Q_ARG(int, m_generation), Q_ARG(int, m_requestSerial), Q_ARG(QImage, image));
        }
    }

private:
    QPointer<TiledGraphicsView> m_view;
    QSharedPointer<ISequenceFrameSource> m_source;
    int m_sourceIndex = -1;
    bool m_highResolution = false;
    int m_generation = 0;
	int m_requestSerial = 0;
    int m_thumbnailMaxEdge = 1024;
	ImageDisplayAdjustments m_adjustments;
};
}

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

TiledGraphicsView::TiledGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
    , m_vecSleeperManager(nullptr)
    , m_orientation(LayoutOrientation::Vertical)
    , m_scrollSpeed(50)
    , m_isSynchronizingViewPosition(false)
{ 
	qRegisterMetaType<AnnotationDrawContext>("AnnotationDrawContext");
	m_sequencePreviewCache.setMemoryLimitBytes(64LL * 1024LL * 1024LL);
	m_sequenceFullCache.setMemoryLimitBytes(192LL * 1024LL * 1024LL);
	m_sequenceTraceClock.start();
	m_curDrawShape = Shape_Line;

    // ?? 看门狗： 
    if (this->devicePixelRatio() > 1.0) {
        if (!QCoreApplication::testAttribute(Qt::AA_EnableHighDpiScaling)) {
            qCritical() << "High DPI scaling is not enabled.";
            qCritical() << "Please enable QApplication::setAttribute(Qt::AA_EnableHighDpiScaling) before QApplication is created.";
            // 这里仅仅打印日志，不弹窗也不崩溃，起到提示作用即?
        }
    }
    // 配置 GraphicsView (硬件加速、事件拦?
    setupGraphicsView();
    // 绑定信号槽、快捷键、定时器
    setupConnections();
	setViewMode(Mode_Browse);
	// ?? 挂载万能绘制工具，并告诉它画什么形?
	m_defaultDrawTool = new DefectDrawTool(m_curDrawShape);
	m_defaultDrawTool->setView(this);
	m_currentTool = m_defaultDrawTool;
	m_ownsCurrentTool = false;

    // 卷帘比较复用同一个异步加载器，只缓存当前需要的比较图，避免另外两个软件增加额外线程。
    connect(AsyncImageLoader::instance(), &AsyncImageLoader::sigThumbnailLoaded, this,
        [this](const QString& path, const QPixmap& pix) {
            for (int i = 0; i < m_compareCurtainFrames.size(); ++i) {
                CompareCurtainFrame& frame = m_compareCurtainFrames[i];
                if (frame.info.dbFilePath != path) continue;
                frame.thumbnailLoading = false;
                if (m_curtainEnabled && m_curtainNeededFrames.contains(i) && !pix.isNull()) { frame.thumbnail = pix; viewport()->update(); }
            }
        });
    connect(AsyncImageLoader::instance(), &AsyncImageLoader::sigImageLoaded, this,
        [this](const QString& path, const QPixmap& pix) {
            if (!m_curtainPendingTiles.contains(path)) return;
            const CurtainPendingTile pending = m_curtainPendingTiles.take(path);
            if (!m_curtainEnabled || !m_curtainNeededTiles.contains(path)
                || pending.frameIndex < 0 || pending.frameIndex >= m_compareCurtainFrames.size() || pix.isNull()) return;
            CompareCurtainFrame& frame = m_compareCurtainFrames[pending.frameIndex];
            if (frame.loadedTiles.size() >= 200) return;
            frame.loadedTiles.insert(pending.key, pix);
            updateCurtainCompareResources();
            viewport()->update();
        });
}

TiledGraphicsView::~TiledGraphicsView()
{ 
    clear();
	m_sequenceDecodePool.waitForDone();
	if (m_ownsCurrentTool && m_currentTool && m_currentTool != m_defaultDrawTool) {
		delete m_currentTool;
	}
	m_currentTool = nullptr;
	m_ownsCurrentTool = false;
	delete m_defaultDrawTool;
	m_defaultDrawTool = nullptr;
}

static QString normalizedImageBaseName(const QString& imageName)
{
    const QString fileName = QFileInfo(imageName.trimmed()).fileName();
    return fileName.isEmpty() ? QString() : QFileInfo(fileName).completeBaseName();
}

DefectManager* TiledGraphicsView::sleeperManager() const
{
    return m_vecSleeperManager;
}

AnnotationManager* TiledGraphicsView::registerAnnotationManager(const QString& layerKey, AnnotationManager* manager)
{
    const QString normalizedKey = layerKey.trimmed();
    if (normalizedKey.isEmpty() || !m_scene) return nullptr;

    AnnotationManager* existing = m_annotationManagers.value(normalizedKey, nullptr);
    if (existing) return existing;

    AnnotationManager* actual = manager;
    if (!actual) actual = new AnnotationManager(m_scene, this);
    actual->setLayerKey(normalizedKey);
    m_annotationManagers.insert(normalizedKey, actual);

    // 外部传入且不由 View 持有的 Manager 如果先销毁，自动清理注册表。
    // View 自己创建的 child Manager 不需要该连接，避免析构阶段访问已经释放的成员容器。
    if (actual->parent() != this) {
        connect(actual, &QObject::destroyed, this, [this, normalizedKey]() {
            m_annotationManagers.remove(normalizedKey);
        });
    }
    return actual;
}

AnnotationManager* TiledGraphicsView::annotationManager(const QString& layerKey) const
{
    return m_annotationManagers.value(layerKey.trimmed(), nullptr);
}

QStringList TiledGraphicsView::annotationLayerKeys() const
{
    return m_annotationManagers.keys();
}

bool TiledGraphicsView::clearAnnotationLayer(const QString& layerKey)
{
    AnnotationManager* manager = annotationManager(layerKey);
    if (!manager) return false;
    manager->clearDefects();
    return true;
}

void TiledGraphicsView::clearAnnotations()
{
    // 一个 Manager 只允许注册一个 key；仍做去重，避免未来兼容别名时被重复 clear。
    // 同时把历史 public 指针也纳入，兼容极少数旧业务直接替换 m_vecXXXManager 的做法。
    QSet<AnnotationManager*> managers;
    for (AnnotationManager* manager : m_annotationManagers) if (manager) managers.insert(manager);
    if (m_vecCp3Manager) managers.insert(m_vecCp3Manager);
    if (m_vecPlatformManager) managers.insert(m_vecPlatformManager);
    if (m_vecChainManager) managers.insert(m_vecChainManager);
    if (m_vecSleeperManager) managers.insert(m_vecSleeperManager);
    if (m_defectManager) managers.insert(m_defectManager);
    if (m_vecTunnelLocManager) managers.insert(m_vecTunnelLocManager);
    if (m_vecRingInfoManager) managers.insert(m_vecRingInfoManager);
    if (m_vecSectionManager) managers.insert(m_vecSectionManager);
    if (m_vecReAutoRingManager) managers.insert(m_vecReAutoRingManager);
    if (m_vecArchitraveLineManager) managers.insert(m_vecArchitraveLineManager);

    for (AnnotationManager* manager : managers) manager->clearDefects();
}

bool TiledGraphicsView::registerAnnotationItemFactory(const QString& layerKey, const QString& typeKey, const AnnotationItemCreator& creator)
{
    AnnotationManager* manager = annotationManager(layerKey);
    if (!manager || typeKey.trimmed().isEmpty() || !creator) return false;
    manager->registerItemFactory(typeKey, creator);
    return true;
}

bool TiledGraphicsView::unregisterAnnotationItemFactory(const QString& layerKey, const QString& typeKey)
{
    AnnotationManager* manager = annotationManager(layerKey);
    if (!manager || typeKey.trimmed().isEmpty()) return false;
    manager->unregisterItemFactory(typeKey);
    return true;
}

bool TiledGraphicsView::setAnnotationLayerState(const QString& layerKey, const AnnotationLayerState& state)
{
    AnnotationManager* manager = annotationManager(layerKey);
    if (!manager) return false;
    manager->setLayerState(state);
    if (viewport()) viewport()->update();
    return true;
}

AnnotationLayerState TiledGraphicsView::annotationLayerState(const QString& layerKey) const
{
    AnnotationManager* manager = annotationManager(layerKey);
    return manager ? manager->layerState() : AnnotationLayerState();
}

bool TiledGraphicsView::setAnnotationLayerVisible(const QString& layerKey, bool visible)
{
    AnnotationManager* manager = annotationManager(layerKey);
    if (!manager) return false;
    manager->setLayerVisible(visible);
    if (viewport()) viewport()->update();
    return true;
}

bool TiledGraphicsView::setAnnotationLayerLocked(const QString& layerKey, bool locked)
{
    AnnotationManager* manager = annotationManager(layerKey);
    if (!manager) return false;
    manager->setLayerLocked(locked);
    return true;
}

bool TiledGraphicsView::setAnnotationLayerSelectable(const QString& layerKey, bool selectable)
{
    AnnotationManager* manager = annotationManager(layerKey);
    if (!manager) return false;
    manager->setLayerSelectable(selectable);
    return true;
}

bool TiledGraphicsView::setAnnotationLayerExportable(const QString& layerKey, bool exportable)
{
    AnnotationManager* manager = annotationManager(layerKey);
    if (!manager) return false;
    manager->setLayerExportable(exportable);
    return true;
}

void TiledGraphicsView::setActiveTool(AbstractTool* tool, bool takeOwnership)
{
    if (tool == m_currentTool) {
        m_ownsCurrentTool = takeOwnership && tool != m_defaultDrawTool;
        if (tool) tool->setView(this);
        return;
    }

    if (m_currentTool) m_currentTool->deactivate();
    if (m_ownsCurrentTool && m_currentTool && m_currentTool != m_defaultDrawTool) {
        delete m_currentTool;
    }

    m_currentTool = tool ? tool : m_defaultDrawTool;
    m_ownsCurrentTool = tool && tool != m_defaultDrawTool && takeOwnership;
    if (m_currentTool) m_currentTool->setView(this);

    // 自定义工具需要能直接工作，不能再调用 setViewMode(Mode_Draw)，
    // 因为旧 setViewMode 会按历史语义切回内置 DefectDrawTool。
    if (tool) {
        m_currentMode = Mode_Draw;
        setDragMode(QGraphicsView::NoDrag);
        setCursor(Qt::CrossCursor);
        DrawModelStr = QString::fromLocal8Bit("绘图模式 [自定义工具]");
        updateVisibleTiles();
    }
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

QList<TunnelSectionItem*> TiledGraphicsView::databaseItemsIntersecting(const QRectF& sceneRect) const
{
    QList<TunnelSectionItem*> result;
    if (m_items.isEmpty() || sceneRect.isEmpty()) return result;

    const bool vertical = isVerticalLayout(m_databaseLayoutOrientation);
    const qreal queryStart = vertical ? sceneRect.top() : sceneRect.left();
    const qreal queryEnd = vertical ? sceneRect.bottom() : sceneRect.right();

    // m_items 由 addLayer() 始终按 Scene 主轴坐标升序维护，包括 Reverse 布局的 prepend 情况。
    int low = 0;
    int high = m_items.size();
    while (low < high) {
        const int mid = low + (high - low) / 2;
        TunnelSectionItem* item = m_items.at(mid);
        const QRectF rect = item ? item->sceneBoundingRect() : QRectF();
        const qreal itemEnd = vertical ? rect.bottom() : rect.right();
        if (itemEnd < queryStart) low = mid + 1;
        else high = mid;
    }

    for (int i = low; i < m_items.size(); ++i) {
        TunnelSectionItem* item = m_items.at(i);
        if (!item) continue;
        const QRectF rect = item->sceneBoundingRect();
        const qreal itemStart = vertical ? rect.top() : rect.left();
        if (itemStart > queryEnd) break;
        if (rect.intersects(sceneRect)) result.append(item);
    }
    return result;
}

int TiledGraphicsView::databaseItemIndexAtScenePos(const QPointF& scenePos, bool clampToEdge) const
{
    if (m_items.isEmpty()) return -1;
    const bool vertical = isVerticalLayout(m_databaseLayoutOrientation);
    const qreal primary = vertical ? scenePos.y() : scenePos.x();

    int low = 0;
    int high = m_items.size();
    while (low < high) {
        const int mid = low + (high - low) / 2;
        TunnelSectionItem* item = m_items.at(mid);
        const QRectF rect = item ? item->sceneBoundingRect() : QRectF();
        const qreal itemEnd = vertical ? rect.bottom() : rect.right();
        if (itemEnd < primary) low = mid + 1;
        else high = mid;
    }

    if (low < m_items.size()) {
        TunnelSectionItem* item = m_items.at(low);
        if (item) {
            const QRectF rect = item->sceneBoundingRect();
            const qreal start = vertical ? rect.top() : rect.left();
            const qreal end = vertical ? rect.bottom() : rect.right();
            if (primary >= start && primary <= end) {
                if (clampToEdge || rect.contains(scenePos)) return low;
            }
        }
    }

    if (!clampToEdge) return -1;
    const QRectF firstRect = m_items.first()->sceneBoundingRect();
    const qreal firstStart = vertical ? firstRect.top() : firstRect.left();
    return primary < firstStart ? 0 : m_items.size() - 1;
}

void TiledGraphicsView::onDatabaseItemTilesUpdated(TunnelSectionItem* item)
{
    if (!item) return;
    const int oldCount = m_databaseLoadedTileCounts.value(item, 0);
    const int newCount = qMax(0, item->getLoadedTileCount());
    if (oldCount != newCount) {
        m_databaseLoadedTileCounts.insert(item, newCount);
        m_databaseTotalLoadedTiles += (newCount - oldCount);
        if (m_databaseTotalLoadedTiles < 0) m_databaseTotalLoadedTiles = 0;
    }
    updateHUD();
}

bool TiledGraphicsView::hasExactImageLayer(const QString& imageName) const
{
	const QString targetName = normalizedImageBaseName(imageName);
	if (targetName.isEmpty())
	{
		return false;
	}

	for (TunnelSectionItem* item : m_items)
	{
		if (!item || !item->getSource())
		{
			continue;
		}

		if (normalizedImageBaseName(item->getSource()->oriImageName()) == targetName)
		{
			return true;
		}
	}

	return false;
}

bool TiledGraphicsView::tryMapToExactImageLayer(const QString& imageName, qreal localX, qreal localY, QPointF& scenePos)
{
	scenePos = QPointF(0, 0);
	const QString targetName = normalizedImageBaseName(imageName);
	if (targetName.isEmpty())
	{
		qWarning() << QString::fromLocal8Bit("精确坐标映射失败：图片名称为空");
		return false;
	}

	for (TunnelSectionItem* item : m_items)
	{
		if (!item || !item->getSource())
		{
			continue;
		}

		if (normalizedImageBaseName(item->getSource()->oriImageName()) == targetName)
		{
			scenePos = item->mapToScene(QPointF(localX, localY));
			return true;
		}
	}

	qWarning() << QString::fromLocal8Bit("精确坐标映射失败：当前图层未找到图片") << imageName;
	return false;
}

bool TiledGraphicsView::tryMapToGlobalScene(const QString & imageName, qreal localX, qreal localY, QPointF& scenePos)
{
	scenePos = QPointF(0, 0);
	if (m_contentMode == ContentMode::VirtualSequence && m_coordinateMapper.isValid())
	{
		return m_coordinateMapper.imageNameToScene(imageName, QPointF(localX, localY), scenePos);
	}
	if (m_items.isEmpty())
	{
		qWarning() << QString::fromLocal8Bit("坐标映射失败：当前未加载图层");
		return false;
	}

	if (imageName.trimmed().isEmpty())
	{
		qWarning() << QString::fromLocal8Bit("坐标映射失败：图片名称为空");
		return false;
	}

	for (TunnelSectionItem* item : m_items)
	{
		if (item && item->getImageName().contains(imageName))
		{
			scenePos = item->mapToScene(QPointF(localX, localY));
			return true;
		}
	}
	TunnelSectionItem* firstItem = m_items.first();
	TunnelSectionItem* lastItem = m_items.last();
	if (!firstItem || !lastItem)
	{
		qWarning() << QString::fromLocal8Bit("坐标映射失败：图层数据无效");
		return false;
	}

	bool currentMileOk = false;
	bool beginMileOk = false;
	double curImageMile = imageName.section('-', 0, 0).mid(4).toDouble(&currentMileOk);
	double BegImageMile = firstItem->getImageName().section('-', 0, 0).mid(4).toDouble(&beginMileOk);
	if (!currentMileOk || !beginMileOk)
	{
		qWarning() << QString::fromLocal8Bit("坐标映射失败：图片名称格式无效") << imageName;
		return false;
	}
	if (curImageMile <= BegImageMile)
	{
		scenePos = firstItem->mapToScene(QPointF(0, localY));
	}
	else
	{
		scenePos = lastItem->mapToScene(QPointF(lastItem->boundingRect().width(), localY));
	}

	return true;
}

QPointF TiledGraphicsView::mapToGlobalScene(const QString & imageName, qreal localX, qreal localY)
{
	if (m_contentMode == ContentMode::VirtualSequence && !m_sequenceModel.isNull())
	{
		const int sourceIndex = m_sequenceModel->sourceIndexForName(imageName);
		if (sourceIndex < 0) return QPointF();
		const QRectF frameRect = m_sequenceModel->frameRect(sourceIndex);
		return QPointF(frameRect.left() + qBound<qreal>(0.0, localX, frameRect.width()),
			frameRect.top() + qBound<qreal>(0.0, localY, frameRect.height()));
	}
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
    // 1. 如果指定了缩放级别，先应用缩?
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


	// ??直接?Scene 谁在这个点上，不用自己遍?list
	// items() 返回的是 Z 值从上到下的列表，第一个通常就是最上面?
	if (m_contentMode == ContentMode::VirtualSequence && !m_sequenceModel.isNull())
	{
		QString imageName;
		int x = 0, y = 0;
		if (GlobalSceneToMap(scenePos, imageName, x, y)) emit sigCursorInfoChanged(imageName, x, y);
		else emit sigCursorInfoChanged("", 0, 0);
		updateVisibleTiles();
		emitViewCenterChanged();
		return;
	}

	QList<QGraphicsItem*> items = m_scene->items(scenePos);
	TunnelSectionItem* hoverItem = nullptr;
	for (auto item : items) {
		// 使用 dynamic_cast 确认是不是我们要找的切片?
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
	// ?viewport 的几何中心反?scene 坐标，比读滚动条更稳，缩放后也不会跑偏?
	if (!viewport())
	{
		return QPointF();
	}

	return mapToScene(viewport()->rect().center());
}

double TiledGraphicsView::currentCenterSceneY() const
{
	// 纵向长图联动只关?Y，这里单独给一个入口，调用侧代码会更直白?
	return currentCenterScenePos().y();
}

QPointF TiledGraphicsView::currentBottomCenterScenePos() const
{
	// 用视口底边中点做锚点，用户看到的底部位置就是业务联动的当前位置?
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
	if (m_contentMode == ContentMode::VirtualSequence && !m_sequenceModel.isNull() && !m_sequenceModel->isEmpty())
	{
		QPointF scenePos = currentBottomCenterScenePos();
		int sourceIndex = m_sequenceModel->sourceIndexAt(scenePos);
		if (sourceIndex < 0)
		{
			const QRectF sceneBounds = m_sequenceModel->sceneRect();
			scenePos.setX(qBound(sceneBounds.left(), scenePos.x(), sceneBounds.right()));
			scenePos.setY(qBound(sceneBounds.top(), scenePos.y(), sceneBounds.bottom() - 0.001));
			sourceIndex = m_sequenceModel->sourceIndexAt(scenePos);
		}
		if (sourceIndex < 0) return anchor;
		const QRectF frameRect = m_sequenceModel->frameRect(sourceIndex);
		anchor.valid = true;
		anchor.imageName = m_sequenceModel->descriptor(sourceIndex).imageName;
		anchor.imageIndex = m_sequenceModel->visualIndexForSourceIndex(sourceIndex);
		anchor.scenePos = scenePos;
		anchor.imagePixelPos = QPointF(qBound<qreal>(0.0, scenePos.x() - frameRect.left(), frameRect.width()),
			qBound<qreal>(0.0, scenePos.y() - frameRect.top(), frameRect.height()));
		return anchor;
	}
	if (m_items.isEmpty())
	{
		return anchor;
	}

	QPointF scenePos = currentBottomCenterScenePos();
	const int matchedIndex = databaseItemIndexAtScenePos(scenePos, true);
	if (matchedIndex < 0 || matchedIndex >= m_items.size()) return anchor;
	TunnelSectionItem* matchedItem = m_items.at(matchedIndex);
	if (!matchedItem) return anchor;

	// 到达场景首尾时 viewport 底边可能略超出图片，沿主轴夹回最近一张图。
	const QRectF matchedSceneRect = matchedItem->sceneBoundingRect();
	if (isVerticalLayout(m_databaseLayoutOrientation))
		scenePos.setY(qBound(matchedSceneRect.top(), scenePos.y(), matchedSceneRect.bottom()));
	else
		scenePos.setX(qBound(matchedSceneRect.left(), scenePos.x(), matchedSceneRect.right()));

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

	// 只换 Y，不主动?X，避免用户横向查看某一车道时被联动逻辑拉回中间?
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
	if (m_contentMode == ContentMode::VirtualSequence && !m_sequenceModel.isNull())
	{
		const int sourceIndex = m_sequenceModel->sourceIndexForVisualIndex(imageIndex);
		if (sourceIndex < 0) return;
		const QRectF frameRect = m_sequenceModel->frameRect(sourceIndex);
		pixelY = qBound<qreal>(0.0, pixelY, frameRect.height());
		QPointF target(frameRect.center().x(), frameRect.top() + pixelY);
		QPointF center = currentCenterScenePos();
		center.setX(target.x());
		center.setY(anchorBottom && viewport()
			? target.y() - mapToScene(viewport()->rect()).boundingRect().height() / 2.0 : target.y());
		centerOn(center); updateVisibleTiles(); emitViewCenterChanged(); return;
	}
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

void TiledGraphicsView::synchronizeToImagePixel(int imageIndex, double pixelY, bool anchorBottom)
{
    // 双视图同步只修改纵向位置，保留目标视图当前的横向观察位置。
    m_isFastScrolling = true;
    if (m_navigationSettleTimer != nullptr)
    {
        m_navigationSettleTimer->start();
    }

    if (m_contentMode == ContentMode::VirtualSequence && !m_sequenceModel.isNull())
    {
        const int sourceIndex = m_sequenceModel->sourceIndexForVisualIndex(imageIndex);
        if (sourceIndex < 0)
        {
            return;
        }

        const QRectF frameRect = m_sequenceModel->frameRect(sourceIndex);
        pixelY = qBound<qreal>(0.0, pixelY, frameRect.height());
        QPointF centerPosition = currentCenterScenePos();
        if (centerPosition.isNull())
        {
            centerPosition = frameRect.center();
        }

        const double targetSceneY = frameRect.top() + pixelY;
        centerPosition.setY(anchorBottom && viewport()
            ? targetSceneY - mapToScene(viewport()->rect()).boundingRect().height() / 2.0
            : targetSceneY);

        // centerOn 会触发滚动条防抖刷新，避免此处再次同步调度解码任务。
        m_isSynchronizingViewPosition = true;
        centerOn(centerPosition);
        m_isSynchronizingViewPosition = false;
        return;
    }

    if (m_scene == nullptr || imageIndex < 0 || imageIndex >= m_items.size())
    {
        return;
    }

    TunnelSectionItem* imageItem = m_items.at(imageIndex);
    const QRectF localRect = imageItem->boundingRect();
    pixelY = qBound(localRect.top(), pixelY, localRect.bottom());
    const QPointF targetScenePosition = imageItem->mapToScene(
        QPointF(localRect.center().x(), pixelY));
    QPointF centerPosition = currentCenterScenePos();
    if (centerPosition.isNull())
    {
        centerPosition = targetScenePosition;
    }

    const QRectF visibleSceneRect = viewport()
        ? mapToScene(viewport()->rect()).boundingRect()
        : QRectF();
    centerPosition.setY(anchorBottom && viewport()
        ? targetScenePosition.y() - visibleSceneRect.height() / 2.0
        : targetScenePosition.y());
    m_isSynchronizingViewPosition = true;
    centerOn(centerPosition);
    m_isSynchronizingViewPosition = false;
}


// 轻量同步另一个视图的位置，瓦片调度由滚动条防抖计时器统一执行。
void TiledGraphicsView::synchronizeToImagePixel(QString qstrImageName, double pixelY, bool anchorBottom)
{
    // 先判断当前内容模式再访问 m_sequenceModel，修复旧 0831 版本的空指针风险。
    m_isFastScrolling = true;
    if (m_navigationSettleTimer != nullptr) m_navigationSettleTimer->start();

    if (m_contentMode == ContentMode::VirtualSequence)
    {
        if (m_sequenceModel.isNull()) return;
        const int sourceIndex = m_sequenceModel->sourceIndexForName(qstrImageName);
        if (sourceIndex < 0) return;

        const QRectF frameRect = m_sequenceModel->frameRect(sourceIndex);
        pixelY = qBound<qreal>(0.0, pixelY, frameRect.height());
        QPointF centerPosition = currentCenterScenePos();
        if (centerPosition.isNull()) centerPosition = frameRect.center();
        const double targetSceneY = frameRect.top() + pixelY;
        centerPosition.setY(anchorBottom && viewport()
            ? targetSceneY - mapToScene(viewport()->rect()).boundingRect().height() / 2.0
            : targetSceneY);
        m_isSynchronizingViewPosition = true;
        centerOn(centerPosition);
        m_isSynchronizingViewPosition = false;
        return;
    }

    // DB 瓦片模式按图片名查找，不再错误地借用虚拟序列索引。
    TunnelSectionItem* imageItem = nullptr;
    for (TunnelSectionItem* item : m_items)
    {
        if (item && isSameImageNameForSdkView(item->getSource()->oriImageName(), qstrImageName))
        {
            imageItem = item;
            break;
        }
    }
    if (m_scene == nullptr || imageItem == nullptr) return;

    const QRectF localRect = imageItem->boundingRect();
    pixelY = qBound(localRect.top(), pixelY, localRect.bottom());
    const QPointF targetScenePosition = imageItem->mapToScene(QPointF(localRect.center().x(), pixelY));
    QPointF centerPosition = currentCenterScenePos();
    if (centerPosition.isNull()) centerPosition = targetScenePosition;
    const QRectF visibleSceneRect = viewport() ? mapToScene(viewport()->rect()).boundingRect() : QRectF();
    centerPosition.setY(anchorBottom && viewport()
        ? targetScenePosition.y() - visibleSceneRect.height() / 2.0
        : targetScenePosition.y());
    m_isSynchronizingViewPosition = true;
    centerOn(centerPosition);
    m_isSynchronizingViewPosition = false;
}

void TiledGraphicsView::scrollToImagePixel(const QString& imageName, double pixelY, bool anchorBottom)
{
	if (!m_scene || imageName.isEmpty())
	{
		return;
	}
	if (m_contentMode == ContentMode::VirtualSequence && !m_sequenceModel.isNull())
	{
		const int sourceIndex = m_sequenceModel->sourceIndexForName(imageName);
		if (sourceIndex < 0) return;
		scrollToImagePixel(m_sequenceModel->visualIndexForSourceIndex(sourceIndex), pixelY, anchorBottom);
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

bool TiledGraphicsView::loadVirtualSequence(const QSharedPointer<ISequenceFrameSource>& source,
	const SequenceLoadOptions& options, LayoutOrientation orientation,
	bool horizontalMirror, bool verticalMirror)
{
	if (source.isNull() || source->frameCount() == 0) return false;
	clear();
	m_contentMode = ContentMode::VirtualSequence;
	m_orientation = orientation;
	m_sequenceSource = source;
	m_sequenceOptions = options;
	m_sequenceOptions.chunkFrameCount = qMax(1, options.chunkFrameCount);
	m_sequenceOptions.thumbnailMaxEdge = qMax(64, options.thumbnailMaxEdge);
	m_sequenceOptions.decodedCacheBytes = qMax<qint64>(16LL * 1024LL * 1024LL, options.decodedCacheBytes);
	m_sequenceHorizontalMirror = horizontalMirror;
	m_sequenceVerticalMirror = verticalMirror;
	m_sequenceModel.reset(new ImageLayoutManager());
	if (!m_sequenceModel->build(source, orientation))
	{
		clear();
		return false;
	}
	m_coordinateMapper.setModel(m_sequenceModel);

	const int idealJobs = qBound(1, QThread::idealThreadCount() / 2, 4);
	m_sequenceDecodePool.setMaxThreadCount(options.maxDecodeJobs > 0 ? options.maxDecodeJobs : idealJobs);
	// 预览和高清分开预算，避免少量大高清图把高速浏览所需的缩略图全部逐出。
	const qint64 totalBudget = m_sequenceOptions.decodedCacheBytes;
	const qint64 previewBudget = qMax<qint64>(4LL * 1024LL * 1024LL, totalBudget / 4);
	const qint64 fullBudget = qMax<qint64>(1LL * 1024LL * 1024LL, totalBudget - previewBudget);
	m_sequencePreviewCache.setMemoryLimitBytes(previewBudget);
	m_sequenceFullCache.setMemoryLimitBytes(fullBudget);
	m_isFastScrolling = true;
	if (m_navigationSettleTimer) m_navigationSettleTimer->start();

	const int count = m_sequenceModel->count();
	for (int first = 0; first < count; first += m_sequenceOptions.chunkFrameCount)
	{
		const int last = qMin(count - 1, first + m_sequenceOptions.chunkFrameCount - 1);
		SequenceChunkItem* chunk = new SequenceChunkItem(this, m_sequenceModel, first, last);
		m_scene->addItem(chunk);
		m_sequenceChunks.append(chunk);
	}
	m_scene->setSceneRect(m_sequenceModel->sceneRect());
	resetToFit();
	return true;
}

int TiledGraphicsView::imageCount() const
{
	return m_contentMode == ContentMode::VirtualSequence && !m_sequenceModel.isNull()
		? m_sequenceModel->count() : m_items.size();
}

SequenceFrameDescriptor TiledGraphicsView::imageDescriptor(int sourceIndex) const
{
	if (m_contentMode == ContentMode::VirtualSequence && !m_sequenceModel.isNull()
		&& sourceIndex >= 0 && sourceIndex < m_sequenceModel->count())
	{
		return m_sequenceModel->descriptor(sourceIndex);
	}
	return SequenceFrameDescriptor();
}

QStringList TiledGraphicsView::visibleImageFileNames() const
{
	QStringList result;
	if (!viewport()) return result;

	const QRectF visibleSceneRect = mapToScene(viewport()->rect()).boundingRect();
	if (visibleSceneRect.isEmpty()) return result;

	auto appendFileName = [&result](const QString& imagePathOrName)
	{
		const QString fileName = QFileInfo(imagePathOrName).fileName();
		if (!fileName.isEmpty() && !result.contains(fileName)) result.append(fileName);
	};

	if (m_contentMode == ContentMode::VirtualSequence && !m_sequenceModel.isNull())
	{
		const int firstVisualIndex = m_sequenceModel->firstVisualIndexIntersecting(visibleSceneRect);
		const int lastVisualIndex = m_sequenceModel->lastVisualIndexIntersecting(visibleSceneRect);
		if (firstVisualIndex < 0 || lastVisualIndex < firstVisualIndex) return result;

		for (int visualIndex = firstVisualIndex; visualIndex <= lastVisualIndex; ++visualIndex)
		{
			const int sourceIndex = m_sequenceModel->sourceIndexForVisualIndex(visualIndex);
			const SequenceFrameDescriptor descriptor = m_sequenceModel->descriptor(sourceIndex);
			// Pack frames have no original file path; retain their stable imageName.
			appendFileName(descriptor.filePath.isEmpty() ? descriptor.imageName : descriptor.filePath);
		}
		return result;
	}

	// Database-tile mode keeps one TunnelSectionItem for each original image.
	// m_items is maintained in scene display order by addLayer().
	for (TunnelSectionItem* item : databaseItemsIntersecting(visibleSceneRect))
	{
		if (item) appendFileName(item->getImageName());
	}
	return result;
}

QString TiledGraphicsView::sequenceCacheKey(int sourceIndex, bool highResolution) const
{
	return QString::number(sourceIndex) + (highResolution ? QStringLiteral(":H") : QStringLiteral(":T"));
}

bool TiledGraphicsView::sequenceUsesThumbnailPreview() const
{
	return !m_sequenceSource.isNull() && m_sequenceSource->usesThumbnailPreview();
}

bool TiledGraphicsView::sequenceShouldUseHighResolution(qreal lod) const
{
	if (m_sequenceSource.isNull()) return false;
	if (!sequenceUsesThumbnailPreview()) return true;
	if (m_sequenceSource->requiresFullResolutionWhenIdle()) return !m_isFastScrolling;
	return !m_isFastScrolling && lod >= 0.45;
}

void TiledGraphicsView::requestSequenceImage(int sourceIndex, bool highResolution, int priority)
{
	if (m_contentMode != ContentMode::VirtualSequence || m_sequenceSource.isNull()
		|| sourceIndex < 0 || sourceIndex >= imageCount()) return;
	const QString key = sequenceCacheKey(sourceIndex, highResolution);
	ImageCacheManager& targetCache = highResolution ? m_sequenceFullCache : m_sequencePreviewCache;
	if (targetCache.contains(key)
		|| m_sequencePendingRequests.value(key, -1) == m_sequenceRequestSerial) return;
	m_sequencePendingRequests.insert(key, m_sequenceRequestSerial);
	if (sequenceTraceEnabled())
	{
		const SequenceFrameDescriptor descriptor = imageDescriptor(sourceIndex);
		qInfo().noquote() << "[HN_SEQUENCE_TRACE][REQUEST]"
			<< "index=" << sourceIndex << "high=" << highResolution
			<< "name=" << descriptor.imageName
			<< "request=" << m_sequenceRequestSerial << "priority=" << priority
			<< "active=" << m_sequenceDecodePool.activeThreadCount();
	}
	m_sequenceDecodePool.start(new SequenceDecodeTask(this, m_sequenceSource, sourceIndex,
		highResolution, m_sequenceGeneration, m_sequenceRequestSerial,
		m_sequenceOptions.thumbnailMaxEdge, m_imageAdjustments), priority);
}

QImage TiledGraphicsView::sequenceImageForPaint(int sourceIndex, bool highResolution)
{
	if (m_contentMode != ContentMode::VirtualSequence) return QImage();
	// 已经缓存的原图始终优先使用，进入快速浏览不能把清晰图降级成缩略图。
	const QString fullKey = sequenceCacheKey(sourceIndex, true);
	if (QImage* full = m_sequenceFullCache.object(fullKey)) return *full;
	if (highResolution)
	{
		requestSequenceImage(sourceIndex, true, 2);
	}
	if (!sequenceUsesThumbnailPreview()) return QImage();
	// 原图尚未完成时继续显示缩略图，解码完成后由回调触发无缝重绘。
	const QString thumbKey = sequenceCacheKey(sourceIndex, false);
	if (QImage* thumb = m_sequencePreviewCache.object(thumbKey)) return *thumb;
	if (sequenceTraceEnabled())
	{
		const qint64 now = m_sequenceTraceClock.elapsed();
		const QString traceKey = QString::number(sourceIndex) + QStringLiteral(":MISS");
		if (now - m_sequenceLastTraceMs.value(traceKey, -1000) >= 250)
		{
			m_sequenceLastTraceMs.insert(traceKey, now);
			qInfo().noquote() << "[HN_SEQUENCE_TRACE][PAINT_MISS]"
				<< "index=" << sourceIndex << "request=" << m_sequenceRequestSerial
				<< "name=" << imageDescriptor(sourceIndex).imageName
				<< "wantHigh=" << highResolution
				<< "pendingH=" << m_sequencePendingRequests.value(sequenceCacheKey(sourceIndex, true), -1)
				<< "pendingT=" << m_sequencePendingRequests.value(thumbKey, -1)
				<< "cacheKiB=" << (m_sequencePreviewCache.totalCost() + m_sequenceFullCache.totalCost())
				<< "active=" << m_sequenceDecodePool.activeThreadCount();
		}
	}
	requestSequenceImage(sourceIndex, false, 1);
	return QImage();
}

void TiledGraphicsView::onSequenceImageDecoded(int sourceIndex, bool highResolution, int generation,
	int requestSerial, QImage image)
{
	// A result from a cleared/reloaded sequence must not remove the pending flag
	// of a newer request that happens to use the same frame index.
	if (generation != m_sequenceGeneration || m_contentMode != ContentMode::VirtualSequence)
	{
		if (sequenceTraceEnabled())
			qInfo().noquote() << "[HN_SEQUENCE_TRACE][CALLBACK_DROP_LAYOUT]"
				<< "index=" << sourceIndex << "high=" << highResolution
				<< "layoutGen=" << generation << "current=" << m_sequenceGeneration;
		return;
	}
	const QString key = sequenceCacheKey(sourceIndex, highResolution);
	const bool ownsPendingRequest =
		m_sequencePendingRequests.value(key, -1) == requestSerial;
	// 普通文件图片不随视口变化。滚动换批次时任务可能已经开始执行，允许已完成的
	// 缩略图和原图进入缓存，避免读完后被丢弃，下一屏又重复读取同一文件。
	const bool canCacheCompletedFileImage = !m_sequenceSource.isNull()
		&& m_sequenceSource->requiresFullResolutionWhenIdle();
	if (!ownsPendingRequest && !canCacheCompletedFileImage)
	{
		if (sequenceTraceEnabled())
			qInfo().noquote() << "[HN_SEQUENCE_TRACE][CALLBACK_STALE_REQUEST]"
				<< "index=" << sourceIndex << "high=" << highResolution
				<< "request=" << requestSerial
				<< "currentOwner=" << m_sequencePendingRequests.value(key, -1);
		// The viewport has already scheduled a newer batch.  Caching this old
		// result can evict the thumbnail/full image that the user is looking at
		// now, and accepting old full decodes is the main source of cache churn
		// while a navigation key is held down.
		return;
	}
	if (ownsPendingRequest)
	{
		m_sequencePendingRequests.remove(key);
	}
	if (image.isNull())
	{
		const SequenceFrameDescriptor descriptor = imageDescriptor(sourceIndex);
		qWarning().noquote() << "[HN_SEQUENCE_DECODE_FAIL]"
			<< "index=" << sourceIndex << "high=" << highResolution
			<< "name=" << descriptor.imageName << "path=" << descriptor.filePath;
		return;
	}
	const qint64 byteCost = qMax<qint64>(1, static_cast<qint64>(image.bytesPerLine()) * image.height());
	ImageCacheManager& targetCache = highResolution ? m_sequenceFullCache : m_sequencePreviewCache;
	targetCache.insert(key, new QImage(image),
		static_cast<int>(qMin<qint64>(INT_MAX, (byteCost + 1023) / 1024)));
	emit sigSequenceImageReady(sourceIndex, highResolution);
	const int chunkIndex = m_sequenceOptions.chunkFrameCount > 0
		? m_sequenceModel->visualIndexForSourceIndex(sourceIndex) / m_sequenceOptions.chunkFrameCount : 0;
	const bool validChunk = chunkIndex >= 0 && chunkIndex < m_sequenceChunks.size()
		&& m_sequenceChunks.at(chunkIndex);
	if (validChunk) m_sequenceChunks.at(chunkIndex)->update();
	const QRectF visibleSceneRect = viewport()
		? mapToScene(viewport()->rect()).boundingRect() : QRectF();
	const bool visible = !m_sequenceModel.isNull()
		&& m_sequenceModel->frameRect(sourceIndex).intersects(visibleSceneRect);
	// A decoded visible frame must repaint the viewport even if Qt loses or
	// coalesces the chunk's local dirty region. This is the same repaint that a
	// minimize/restore was previously forcing externally.
	if (visible && viewport()) viewport()->update();
	if (sequenceTraceEnabled())
		qInfo().noquote() << "[HN_SEQUENCE_TRACE][CACHE_INSERT]"
			<< "index=" << sourceIndex << "high=" << highResolution
			<< "request=" << requestSerial << "chunk=" << chunkIndex
			<< "validChunk=" << validChunk << "visible=" << visible
			<< "cacheKiB=" << (m_sequencePreviewCache.totalCost() + m_sequenceFullCache.totalCost());
}

void TiledGraphicsView::requestSequenceRange(const QRectF& sceneRect, bool highResolution, int priority)
{
	if (m_sequenceModel.isNull() || sceneRect.isEmpty()) return;
	const int first = m_sequenceModel->firstVisualIndexIntersecting(sceneRect);
	const int last = m_sequenceModel->lastVisualIndexIntersecting(sceneRect);
	if (first < 0 || last < 0) return;

	// 同一批任务按“离当前视口中心的距离”细分优先级；接口保持不变。
	QRectF visibleRect;
	if (viewport()) visibleRect = mapToScene(viewport()->rect()).boundingRect();
	const QPointF focus = visibleRect.isEmpty() ? sceneRect.center() : visibleRect.center();
	const qreal span = qMax<qreal>(1.0, isVerticalLayout(m_orientation)
		? (visibleRect.isEmpty() ? sceneRect.height() : visibleRect.height())
		: (visibleRect.isEmpty() ? sceneRect.width() : visibleRect.width()));

	for (int visual = first; visual <= last; ++visual) {
		const int sourceIndex = m_sequenceModel->sourceIndexForVisualIndex(visual);
		const QRectF frameRect = m_sequenceModel->frameRect(sourceIndex);
		const qreal distance = qAbs(isVerticalLayout(m_orientation)
			? frameRect.center().y() - focus.y()
			: frameRect.center().x() - focus.x());
		int framePriority = priority;
		if (distance <= span * 0.75) framePriority += 2;
		else if (distance <= span * 1.5) framePriority += 1;
		requestSequenceImage(sourceIndex, highResolution, framePriority);
	}
}

void TiledGraphicsView::clearVirtualSequence()
{
	++m_sequenceGeneration;
	++m_sequenceRequestSerial;
	m_sequenceDecodePool.clear();
	m_sequencePendingRequests.clear();
	m_sequencePreviewCache.clear();
	m_sequenceFullCache.clear();
	m_sequenceScheduleAnchorVisibleRect = QRectF();
	m_sequenceScheduleAnchorValid = false;
	m_sequenceScheduleAnchorHighResolution = false;
	m_sequenceLastVisibleCenter = QPointF();
	m_sequenceLastVisibleCenterValid = false;

	// 旧 clear() 最终会 scene->clear()，但新 clearImages() 不会。
	// 因此虚拟序列必须自己释放 Chunk Item，避免只清 QList 后 Scene 仍保留旧图片。
	const QList<SequenceChunkItem*> chunks = m_sequenceChunks;
	m_sequenceChunks.clear();
	for (SequenceChunkItem* chunk : chunks) {
		if (!chunk) continue;
		if (m_scene && chunk->scene() == m_scene) m_scene->removeItem(chunk);
		delete chunk;
	}
	m_coordinateMapper.clear();
	m_sequenceModel.clear();
	m_sequenceSource.clear();
	m_sequenceHorizontalMirror = false;
	m_sequenceVerticalMirror = false;
}

void TiledGraphicsView::addLayer(AbstractTileSource* source)
{
    // 兼容所有旧软件：没有真实里程时走普通路径。
    addLayer(source, DbImageInfo());
}

void TiledGraphicsView::addLayer(AbstractTileSource* source, const DbImageInfo& imageInfo)
{
	m_contentMode = ContentMode::DatabaseTiles;
	if (m_items.isEmpty()) m_databaseLayoutOrientation = m_orientation;
    TunnelSectionItem* item = new TunnelSectionItem(source);
    // 基准图片放在卷帘层之下，标注仍保持在上层。
    item->setZValue(-1000.0);
	connect(item, &TunnelSectionItem::sigTilesUpdated, this, [this, item]() {
		onDatabaseItemTilesUpdated(item);
	});
    m_scene->addItem(item);

    // 自动拼接逻辑：方向由 LayoutOrientation 决定，不再默认所有项目都从左到右拼?
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
	// 极少数数据源可能在 addLayer 完成前同步触发一次 tilesUpdated；避免初始计数被重复累计。
	if (!m_databaseLoadedTileCounts.contains(item)) {
		const int initialLoadedCount = qMax(0, item->getLoadedTileCount());
		m_databaseLoadedTileCounts.insert(item, initialLoadedCount);
		m_databaseTotalLoadedTiles += initialLoadedCount;
	}

    BaseCurtainFrame baseFrame;
    baseFrame.item = item;
    baseFrame.info = imageInfo;
    if (baseFrame.info.dbFilePath.isEmpty() && source) baseFrame.info.dbFilePath = source->getDbPath();
    if (baseFrame.info.originalName.isEmpty() && source) baseFrame.info.originalName = source->oriImageName();
    if (baseFrame.info.width <= 0) baseFrame.info.width = qRound(item->boundingRect().width());
    if (baseFrame.info.height <= 0) baseFrame.info.height = qRound(item->boundingRect().height());
    m_baseCurtainFrames.append(baseFrame);

    // 场景边界增量维护：旧实现每 add 一张都扫描全部图片，累计为 O(N²)。
    const QRectF itemSceneRect = item->sceneBoundingRect();
    m_databaseImageSceneRect = m_databaseImageSceneRect.isEmpty()
        ? itemSceneRect : m_databaseImageSceneRect.united(itemSceneRect);
    m_scene->setSceneRect(m_databaseImageSceneRect);
    recreateCurtainItem();
    if (m_curtainItem) m_curtainItem->setRect(m_databaseImageSceneRect);

    // 如果是第一张图，自动适应高度
    if (m_items.size() == 1) {

         resetTransform();
        double startScale = getFitScale();
        scale(startScale, startScale);
        m_currentScale = startScale;
        
    }

}

void TiledGraphicsView::clearImages()
{
    clearCurtainCompare();
    clearVirtualSequence();

    const QList<TunnelSectionItem*> imageItems = m_items;
    m_items.clear();
    m_databaseScheduledItems.clear();
    m_databaseLoadedTileCounts.clear();
    m_databaseTotalLoadedTiles = 0;
    m_databaseImageSceneRect = QRectF();
    for (TunnelSectionItem* item : imageItems) {
        if (!item) continue;
        if (m_scene && item->scene() == m_scene) m_scene->removeItem(item);
        delete item;
    }

    m_baseCurtainFrames.clear();
    m_imageItemCache.clear();
    m_contentMode = ContentMode::DatabaseTiles;
    if (m_curtainItem) m_curtainItem->setRect(QRectF());

    // Annotation 仍可保留，所以 SceneRect 只按剩余内容重新计算。
    if (m_scene) m_scene->setSceneRect(m_scene->itemsBoundingRect());
    if (viewport()) viewport()->update();
}

void TiledGraphicsView::clear()
{
    clearCurtainCompare();
	clearVirtualSequence();
	clearAnnotations();
	if (m_currentTool) m_currentTool->deactivate();

    m_scene->clear(); // 这会 delete 掉所有的 item（包括 CurtainCompareItem）
    m_curtainItem = nullptr; // scene 已经释放该对象，必须清空悬空指针后再重建。
    m_items.clear();
    m_databaseScheduledItems.clear();
    m_databaseLoadedTileCounts.clear();
    m_databaseTotalLoadedTiles = 0;
    m_databaseImageSceneRect = QRectF();
    m_baseCurtainFrames.clear();
	m_imageItemCache.clear();
	m_exportBoxItem = nullptr;
    m_scene->setSceneRect(0, 0, 0, 0);
	m_contentMode = ContentMode::DatabaseTiles;
    recreateCurtainItem();
}


// ?? 1. 实现公开的重置接?
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
	// DB 切片数量由 sigTilesUpdated 增量维护，避免每次 HUD 更新扫描全部图片。
	const int loadedTiles = qMax(0, m_databaseTotalLoadedTiles);
    bool isHighResMode = loadedTiles > 0;

	
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
    else if (mode == Mode_Move)
    {
        setDragMode(QGraphicsView::NoDrag);
        setCursor(Qt::SizeVerCursor);
        DrawModelStr = QString::fromLocal8Bit("移动模式");
    }
    else if (mode == Mode_Draw)
    {
		setDragMode(QGraphicsView::NoDrag);
		setCursor(Qt::CrossCursor); // 绘图模式用十字光?
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

    // 可以在这里触?updateVisibleTiles 以刷新可能的 UI 状?
	updateVisibleTiles();
}

// =========================================================
// ?? 核心交互逻辑
// =========================================================

void TiledGraphicsView::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx, dy);
    // 双视图程序同步由防抖计时器统一发布位置，避免同一位置连续计算两次。
    if (!m_isSynchronizingViewPosition)
    {
        emitViewCenterChanged();
    }
	if (horizontalScrollBar()->isSliderDown() || verticalScrollBar()->isSliderDown())
	{
		emitUserViewBottomAnchorChanged();
		m_debounceTimer->start();
		return;
	}
	
    // 触发防抖更新 (LOD 计算)
    m_debounceTimer->start();
}

void TiledGraphicsView::setDxfMeasureEnabled(bool enabled)
{
	m_dxfMeasureEnabled = enabled;
}

void TiledGraphicsView::setTunnelLocationMoveEnabled(bool enabled)
{
	m_tunnelLocationMoveEnabled = enabled;
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

    if (m_curtainLinePosition < 0) {
        m_curtainLinePosition = (m_curtainOrientation == CurtainOrientation::Vertical)
            ? viewport()->width() / 2 : viewport()->height() / 2;
    } else {
        const int maxPosition = (m_curtainOrientation == CurtainOrientation::Vertical)
            ? viewport()->width() : viewport()->height();
        m_curtainLinePosition = qBound(0, m_curtainLinePosition, maxPosition);
    }
    if (m_curtainItem) m_curtainItem->setRect(m_scene->sceneRect());
}

void TiledGraphicsView::paintEvent(QPaintEvent* event)
{
	QElapsedTimer paintTimer;
	if (sequenceTraceEnabled()) paintTimer.start();
	QGraphicsView::paintEvent(event);
	if (sequenceTraceEnabled() && m_contentMode == ContentMode::VirtualSequence)
	{
		const qint64 now = m_sequenceTraceClock.elapsed();
		if (now - m_sequenceLastTraceMs.value(QStringLiteral("VIEW_PAINT"), -1000) >= 100)
		{
			m_sequenceLastTraceMs.insert(QStringLiteral("VIEW_PAINT"), now);
			qInfo().noquote() << "[HN_SEQUENCE_TRACE][VIEW_PAINT]"
				<< "request=" << m_sequenceRequestSerial
				<< "visibleScene=" << mapToScene(viewport()->rect()).boundingRect()
				<< "dirty=" << (event ? event->region().boundingRect() : QRect())
				<< "cacheKiB=" << (m_sequencePreviewCache.totalCost() + m_sequenceFullCache.totalCost())
				<< "pending=" << m_sequencePendingRequests.size()
				<< "elapsedMs=" << paintTimer.elapsed();
		}
	}
}

bool TiledGraphicsView::stepVirtualSequenceFrame(int visualDelta)
{
	if (m_contentMode != ContentMode::VirtualSequence || m_sequenceModel.isNull()
		|| m_sequenceModel->isEmpty() || visualDelta == 0)
	{
		return false;
	}

	QPointF center = currentCenterScenePos();
	const QRectF sceneBounds = m_sequenceModel->sceneRect();
	center.setX(qBound(sceneBounds.left(), center.x(), sceneBounds.right() - 0.001));
	center.setY(qBound(sceneBounds.top(), center.y(), sceneBounds.bottom() - 0.001));

	const int currentVisual = m_sequenceModel->firstVisualIndexIntersecting(
		QRectF(center, QSizeF(0.001, 0.001)));
	const int sourceIndex = m_sequenceModel->sourceIndexForVisualIndex(currentVisual);
	if (currentVisual < 0 || sourceIndex < 0)
	{
		return false;
	}

	const int targetVisual = qBound(0, currentVisual + visualDelta, m_sequenceModel->count() - 1);
	if (targetVisual == currentVisual)
	{
		return false;
	}
	const int targetSource = m_sequenceModel->sourceIndexForVisualIndex(targetVisual);
	if (targetSource < 0)
	{
		return false;
	}

	const QRectF currentRect = m_sequenceModel->frameRect(sourceIndex);
	const QRectF targetRect = m_sequenceModel->frameRect(targetSource);
	if (isVerticalLayout(m_orientation))
	{
		const qreal ratio = currentRect.height() > 0.0
			? qBound<qreal>(0.0, (center.y() - currentRect.top()) / currentRect.height(), 1.0) : 0.5;
		center.setY(targetRect.top() + ratio * targetRect.height());
	}
	else
	{
		const qreal ratio = currentRect.width() > 0.0
			? qBound<qreal>(0.0, (center.x() - currentRect.left()) / currentRect.width(), 1.0) : 0.5;
		center.setX(targetRect.left() + ratio * targetRect.width());
	}

	centerOn(center);
	updateVisibleTiles();
	emitViewCenterChanged();
	emitUserViewBottomAnchorChanged();
	return true;
}

bool TiledGraphicsView::stepSingleFrame(int visualDelta)
{
	if (visualDelta == 0)
	{
		return false;
	}
	if (m_contentMode == ContentMode::VirtualSequence)
	{
		return stepVirtualSequenceFrame(visualDelta);
	}
	if (m_items.isEmpty())
	{
		return false;
	}

	const QPointF center = currentCenterScenePos();
	int currentIndex = -1;
	for (int i = 0; i < m_items.size(); ++i)
	{
		const QRectF rect = m_items.at(i)->sceneBoundingRect();
		const bool onPrimaryAxis = isVerticalLayout(m_orientation)
			? center.y() >= rect.top() && center.y() <= rect.bottom()
			: center.x() >= rect.left() && center.x() <= rect.right();
		if (onPrimaryAxis)
		{
			currentIndex = i;
			break;
		}
	}
	if (currentIndex < 0)
	{
		currentIndex = isVerticalLayout(m_orientation)
			? (center.y() < m_items.first()->sceneBoundingRect().top() ? 0 : m_items.size() - 1)
			: (center.x() < m_items.first()->sceneBoundingRect().left() ? 0 : m_items.size() - 1);
	}

	const int targetIndex = qBound(0, currentIndex + visualDelta, m_items.size() - 1);
	if (targetIndex == currentIndex)
	{
		return false;
	}
	const QRectF currentRect = m_items.at(currentIndex)->sceneBoundingRect();
	const QRectF targetRect = m_items.at(targetIndex)->sceneBoundingRect();
	QPointF targetCenter = center;
	if (isVerticalLayout(m_orientation))
	{
		const qreal ratio = currentRect.height() > 0.0
			? qBound<qreal>(0.0, (center.y() - currentRect.top()) / currentRect.height(), 1.0) : 0.5;
		targetCenter.setY(targetRect.top() + ratio * targetRect.height());
	}
	else
	{
		const qreal ratio = currentRect.width() > 0.0
			? qBound<qreal>(0.0, (center.x() - currentRect.left()) / currentRect.width(), 1.0) : 0.5;
		targetCenter.setX(targetRect.left() + ratio * targetRect.width());
	}
	centerOn(targetCenter);
	updateVisibleTiles();
	emitViewCenterChanged();
	emitUserViewBottomAnchorChanged();
	return true;
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

        // 1. 获取当前缩放系数 (m11 ?x 轴缩放，通常 xy 一?
        double currentScale = transform().m11();

        // 2. 计算缩放因子 (滚轮向上放大 1.1 倍，向下缩小 0.9 ?
        double scaleFactor = (angle > 0) ? 1.15 : (1.0 / 1.15);

        // 3. 预测下一次的缩放?
        double nextScale = currentScale * scaleFactor;

        // =====================================================
        // ?? 核心修改：动态计算边?
        // =====================================================

        // [下限]：不允许缩得比“适应窗口”还?
        // 这样用户狂滚滚轮，最后一定会停在刚好铺满屏幕的状态，非常舒服
        double minScale = getFitScale();

        // [上限]：最大允许放大到 5.0 ?(?1 个像素变 5 个像素大)
        // 隧道病害一般看清裂缝即可，5.0 足够了，太大全是锯齿
        double maxScale = 20.0;

      
        // 如果下一次缩放会超出边界，就只缩放到边界?
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
	// 滚轮没有“松开”事件，每次滚动都重新计时；最后一次事件后再切回高清图。
	m_isFastScrolling = true;
	if (m_navigationSettleTimer != nullptr) m_navigationSettleTimer->start();

    // 获取 Shift 键状?
    bool isShiftPressed = (event->modifiers() & Qt::ShiftModifier);
	if (m_singleFrameNavigationEnabled && !isShiftPressed
		&& m_contentMode == ContentMode::VirtualSequence
		&& stepVirtualSequenceFrame(delta > 0 ? -1 : 1))
	{
		event->accept();
		return;
	}

    // 连续滚动期间优先使用缩略图，停止滚动后再恢复高清解码。
    if (m_contentMode == ContentMode::VirtualSequence)
    {
        m_isFastScrolling = true;
        if (m_navigationSettleTimer != nullptr)
        {
            m_navigationSettleTimer->start();
        }
    }

    if (isVerticalLayout(m_orientation)) {
        // === 纵向模式 (地铁) ===
        if (isShiftPressed) {
            // Shift: 滚水平条 (左右?
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta);
        }
        else {
            // 普? 滚垂直条 (上下跑里?
            verticalScrollBar()->setValue(verticalScrollBar()->value() - delta);
        }
    }
    else {
        // === 横向模式 (公路) ===
        if (isShiftPressed) {
            // ?? 修复点：Shift -> 滚垂直条 (上下看墙?
            verticalScrollBar()->setValue(verticalScrollBar()->value() - delta);
        }
        else {
            // 普? 滚水平条 (左右跑里?
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta);
        }
    } 
	emitUserViewBottomAnchorChanged();
    event->accept(); 
}

void TiledGraphicsView::keyPressEvent(QKeyEvent* event) {

    // 如果外部禁用了导航（比如正在输入文字，或处于编辑模式），直接透传给父?
    if (!m_enableKeyNav) {
        QGraphicsView::keyPressEvent(event);
        return;
    }
	const bool isNavigationKey =
		event->key() == Qt::Key_W || event->key() == Qt::Key_S
		|| event->key() == Qt::Key_A || event->key() == Qt::Key_D
		|| event->key() == Qt::Key_Up || event->key() == Qt::Key_Down
		|| event->key() == Qt::Key_Left || event->key() == Qt::Key_Right;
	if (isNavigationKey)
	{
		// A normal held W/A/S/D key is just as much a fast browse as Shift.
		// Restarting this timer on every auto-repeat prevents full JPEG decodes
		// from being launched for frames that are already leaving the viewport.
		m_isFastScrolling = true;
		if (m_navigationSettleTimer) m_navigationSettleTimer->start();
	}
	if (m_singleFrameNavigationEnabled && m_contentMode == ContentMode::VirtualSequence)
	{
		int visualDelta = 0;
		if (isVerticalLayout(m_orientation))
		{
			if (event->key() == Qt::Key_W || event->key() == Qt::Key_Up) visualDelta = -1;
			else if (event->key() == Qt::Key_S || event->key() == Qt::Key_Down) visualDelta = 1;
		}
		else
		{
			if (event->key() == Qt::Key_A || event->key() == Qt::Key_Left) visualDelta = -1;
			else if (event->key() == Qt::Key_D || event->key() == Qt::Key_Right) visualDelta = 1;
		}
		if (visualDelta != 0 && stepVirtualSequenceFrame(visualDelta))
		{
			event->accept();
			return;
		}
	}
    // 小优化：处理 Shift 加?
    int speed = m_scrollSpeed;
    bool isShift = (event->modifiers() & Qt::ShiftModifier);
    if (isShift) {
        speed *= 4;
        m_isFastScrolling = true; // ?? 进入飙车模式
    }
    bool handled = true;

    switch (event->key()) {
    case Qt::Key_C:
        // 老 TunnelViewer 快捷键：只有已经加载对比期时才接管 C 键。
        if (!m_compareCurtainFrames.isEmpty()) {
            setCurtainCompareEnabled(!m_curtainEnabled);
            event->accept();
            return;
        }
        handled = false;
        break;
    case Qt::Key_Space:
        resetToFit();   // 调用之前封装好的公有槽函?
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
        // 普通移动已经改过滚动条了，这里只收下事件，避免父类再处理一遍快捷键?
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
		if (m_navigationSettleTimer) m_navigationSettleTimer->start();
    }
	else if ((event->key() == Qt::Key_W || event->key() == Qt::Key_S
		|| event->key() == Qt::Key_A || event->key() == Qt::Key_D
		|| event->key() == Qt::Key_Up || event->key() == Qt::Key_Down
		|| event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
		&& !event->isAutoRepeat())
	{
		if (m_navigationSettleTimer) m_navigationSettleTimer->start();
    }

    QGraphicsView::keyReleaseEvent(event);
}

void TiledGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDraggingCurtain) {
        m_curtainLinePosition = (m_curtainOrientation == CurtainOrientation::Vertical) ? event->pos().x() : event->pos().y();
        updateCurtainCompareResources();
        viewport()->update();
        event->accept();
        return;
    }
    if (m_currentMode == Mode_Browse && m_curtainEnabled && event->buttons() == Qt::NoButton && isNearCurtainLine(event->pos())) {
        setCursor(m_curtainOrientation == CurtainOrientation::Vertical ? Qt::SplitHCursor : Qt::SplitVCursor);
    }

	QPointF scensPos = mapToScene(event->pos());

	if (m_currentMode == Mode_ExportBox && m_exportBoxItem)
	{
		QRectF boxRect(0,0, exportBoxSize, exportBoxSize);
		boxRect.moveCenter(scensPos);
		m_exportBoxItem->setRect(boxRect);
	}
	// ?? 1. 处理中键拖拽
	if (m_isPanning) {
		// 计算鼠标移动的差?
		int dx = event->pos().x() - m_lastMousePos.x();
		int dy = event->pos().y() - m_lastMousePos.y();

		// 拨动滚动?(方向相反，鼠标往右划，内容往右走，滚动条其实是往左减)
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() - dx);
		verticalScrollBar()->setValue(verticalScrollBar()->value() - dy); 
		emitUserViewBottomAnchorChanged();
		// 更新坐标
		m_lastMousePos = event->pos();
		event->accept();
		return;  
	}

	// ?? 绘图模式拦截：让工具画出跟随的虚?
	if (m_currentMode == Mode_Draw && m_currentTool) {
		QPointF scenePos = mapToScene(event->pos());
		m_currentTool->handleMouseMove(scenePos);
	}
	// ?? 3. 核心引擎：自定义病害集群精准拖拽?
	// ==========================================
	// 如果左键按下并处于拖拽状态，接管坐标换算
	if (m_isDraggingDefects) {
		QPointF currentScenePos = mapToScene(event->pos());
		// 计算鼠标在真实的物理世界里移动了多少距离
		QPointF delta = currentScenePos - m_lastDragScenePos;

		// 让所有被选中的病害跟着走，指哪打哪，绝对不乱飘?
		for (QGraphicsItem* item : m_scene->selectedItems()) {
			if (DefectShapeItem* defect = dynamic_cast<DefectShapeItem*>(item)) {
                    if (!defect->isMovableByPolicy()) continue;
				defect->setPos(defect->pos() + delta);
			}
		}

		m_lastDragScenePos = currentScenePos; // 更新坐标
		event->accept();
		return; //  
	}
	// ==========================================
	// ?? 4. 浏览模式下的光标“雷达反馈?
	// ==========================================
	// 只有在没按任何鼠标键（纯移动探测）时，才触发嗅探
	if (m_currentMode == Mode_Browse && event->buttons() == Qt::NoButton) {

		// 调用我们强大的动态物理雷达探测器
		QList<QGraphicsItem*> items = getVisualItems(event->pos());

		bool hoverOnDefect = false;
		for (auto item : items) {
			if (DefectShapeItem* defect = dynamic_cast<DefectShapeItem*>(item)) {
				if (!defect->isVisible() || !defect->flags().testFlag(QGraphicsItem::ItemIsSelectable)) continue;
				hoverOnDefect = true;
				break; // 只要探测到范围里有病害，立刻标记
			}
		}

		// ?? 动态切换光标：碰到病害变“点击小??”，离开变“普通箭头↖?
		if (hoverOnDefect) {
			setCursor(Qt::PointingHandCursor);
		}
		else {
			setCursor(Qt::ArrowCursor);
		}
	}




    QGraphicsView::mouseMoveEvent(event);

    // 1. 获取鼠标?Scene 中的坐标
    QPointF scenePos = mapToScene(event->pos());
	if (m_contentMode == ContentMode::VirtualSequence)
	{
		QString imageName;
		int localX = 0, localY = 0;
		if (GlobalSceneToMap(scenePos, imageName, localX, localY))
			emit sigCursorInfoChanged(imageName, localX, localY);
		else
			emit sigCursorInfoChanged("", 0, 0);
		return;
	}

    // ??直接?Scene 谁在这个点上，不用自己遍?list
    // items() 返回的是 Z 值从上到下的列表，第一个通常就是最上面?
    QList<QGraphicsItem*> items = m_scene->items(scenePos);
    TunnelSectionItem* hoverItem = nullptr;
    for (auto item : items) {
        // 使用 dynamic_cast 确认是不是我们要找的切片?
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
    // DXF 测量优先处理，避免与绘图/拖动模式抢事件。
    if (m_dxfMeasureEnabled) {
        if (event->button() == Qt::LeftButton) { emit sigDxfMeasurePoint(mapToScene(event->pos())); event->accept(); return; }
        if (event->button() == Qt::RightButton) { setDxfMeasureEnabled(false); event->accept(); return; }
    }
    if (m_currentMode == Mode_Browse && m_curtainEnabled
        && event->button() == Qt::LeftButton && isNearCurtainLine(event->pos())) {
        m_isDraggingCurtain = true;
        setCursor(m_curtainOrientation == CurtainOrientation::Vertical ? Qt::SplitHCursor : Qt::SplitVCursor);
        event->accept();
        return;
    }
 
	// 1. 中键拖拽背景
	if (event->button() == Qt::MiddleButton) {
		m_isPanning = true;
		m_isFastScrolling = true;
		if (m_navigationSettleTimer != nullptr) m_navigationSettleTimer->stop();
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
  
    if ((m_currentMode == Mode_Browse || m_currentMode == Mode_Move) && event->button() == Qt::LeftButton)
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
                    // 新业务通过 attributes 控制交互；未配置时内部保持旧 Sleeper/拱脚线语义。
                    if (!defect->isInteractiveInViewMode(m_currentMode, m_tunnelLocationMoveEnabled)) continue;

					hitDefect = defect;
					break;
				}
			}

            // 移动模式允许业务层先按屏幕容差选中图元，避免细横线难以直接命中。
            if (hitDefect == nullptr && m_currentMode == Mode_Move)
            {
                const QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();

                for (QGraphicsItem* selectedItem : selectedItems)
                {
                    hitDefect = dynamic_cast<DefectShapeItem*>(selectedItem);

                    if (hitDefect != nullptr
                        && hitDefect->isInteractiveInViewMode(m_currentMode, m_tunnelLocationMoveEnabled))
                    {
                        break;
                    }

                    hitDefect = nullptr;
                }
            }

			if (hitDefect) {
				if (!hitDefect->isSelected()) {
					m_scene->clearSelection();
					hitDefect->setSelected(true);
				}
				// ?? 核心开启：激活我们自己的上帝拖拽引擎?
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
    if (m_isDraggingCurtain && event->button() == Qt::LeftButton) {
        m_isDraggingCurtain = false;
        unsetCursor();
        updateCurtainCompareResources();
        viewport()->update();
        event->accept();
        return;
    }


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
		if (m_navigationSettleTimer != nullptr) m_navigationSettleTimer->start();


		if (m_currentMode == Mode_Browse) {
			setCursor(Qt::ArrowCursor);
		}
		else {
			setCursor(Qt::CrossCursor);
		}
		event->accept();
		return;
	}

	// 自定义 AbstractTool 的完整事件闭环。内置 DefectDrawTool 没有重写 release，
	// 因此这里不会改变旧点击式绘图语义，但允许外部实现按下-拖动-释放型工具。
	// 中键平移已经在上面优先结算，避免绘图模式下释放中键后 m_isPanning 残留。
	if (m_currentMode == Mode_Draw && m_currentTool) {
		m_currentTool->handleMouseRelease(mapToScene(event->pos()), event->button());
		event->accept();
		return;
	}

	// ==========================================
	//   浏览模式松开左键：结算并抛出框选结?
	// ==========================================
	if (m_currentMode == Mode_Browse && event->button() == Qt::LeftButton)
	{
		if (dragMode() == QGraphicsView::RubberBandDrag) {

			QList<QGraphicsItem*> allSelected = m_scene->selectedItems();
			QList<DefectShapeItem*> selectedDefects;

			for (auto item : allSelected) {
				if (DefectShapeItem* defect = dynamic_cast<DefectShapeItem*>(item)) {

					if (!defect->isRubberBandSelectableByPolicy())
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
// ?? 核心逻辑与辅助函?
// =========================================================

void TiledGraphicsView::setupGraphicsView()
{
    setFrameShape(QFrame::NoFrame);
    // 1. 初始化场?
    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(QColor(40, 40, 40)); // 深灰色背?
    setScene(m_scene);

    // =========================================================
    // ?? 渲染配置
    // =========================================================

	// 默认用普?QWidget viewport。QOpenGLWidget 嵌进复杂 QWidget 界面时，
	// 有些显卡/远程桌面环境会把窗口其它区域残留到视图里，所?OpenGL 改成显式开启?
	applyRenderBackend();

    // 强制全屏重绘，避免局部刷新留下的伪影
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // 开启鼠标追?(即使不按键也能收?MouseMove，用于显示坐?
    setMouseTracking(true);
    viewport()->setMouseTracking(true);

    // 变换锚点设为鼠标中心 (缩放时以鼠标为中?
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);

    // 抗锯?
    setAlignment(Qt::AlignLeft | Qt::AlignTop); 
    setRenderHint(QPainter::Antialiasing);

    // 滚动条策?
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 默认模式
    setDragMode(QGraphicsView::NoDrag);
    //setDragMode(QGraphicsView::ScrollHandDrag);
    setFocusPolicy(Qt::StrongFocus);
    setFocus(); // 启动时获取焦?

    // 旧业务 Manager 统一注册到新 Annotation Registry。
    // 旧公开成员仍指向同一对象，因此现有三个软件无需改调用代码。
    m_vecCp3Manager = registerAnnotationManager(QStringLiteral("cp3"));
    m_vecCp3Manager->setLegacyElementType(Type_Cp3);

    m_vecPlatformManager = registerAnnotationManager(QStringLiteral("platform"));
    m_vecPlatformManager->setLegacyElementType(Type_Platform);

    m_vecChainManager = registerAnnotationManager(QStringLiteral("chain"));
    m_vecChainManager->setLegacyElementType(Type_Chain);

    m_vecSleeperManager = registerAnnotationManager(QStringLiteral("sleeper"));
    m_vecSleeperManager->setLegacyElementType(Type_Sleeper);

    m_defectManager = registerAnnotationManager(QStringLiteral("disease"));
    m_defectManager->setLegacyElementType(Type_Disease);

    m_vecTunnelLocManager = registerAnnotationManager(QStringLiteral("tunnel_location"));
    m_vecTunnelLocManager->setLegacyElementType(Type_SPRINGING_LOC);
    m_vecTunnelLocManager->setPersistenceProfile(ElementTypePersistenceProfile::TunnelViewerLegacy);

    m_vecRingInfoManager = registerAnnotationManager(QStringLiteral("ring"));
    m_vecRingInfoManager->setLegacyElementType(Type_Ring);

    m_vecSectionManager = registerAnnotationManager(QStringLiteral("section"));
    m_vecSectionManager->setLegacyElementType(Type_Section);

    m_vecReAutoRingManager = registerAnnotationManager(QStringLiteral("auto_ring"));
    m_vecReAutoRingManager->setLegacyElementType(Type_AUTORING);

    m_vecArchitraveLineManager = registerAnnotationManager(QStringLiteral("user_line"));
    m_vecArchitraveLineManager->setLegacyElementType(Type_UserLine);
    m_vecArchitraveLineManager->setPersistenceProfile(ElementTypePersistenceProfile::TunnelViewerLegacy);

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

	m_navigationSettleTimer = new QTimer(this);
	m_navigationSettleTimer->setSingleShot(true);
	// A short pause between key repeats or wheel steps is still navigation.
	// Waiting a little longer prevents 50 MB full-image decodes from competing
	// with the thumbnails that must keep the moving viewport filled.
	m_navigationSettleTimer->setInterval(350);
	connect(m_navigationSettleTimer, &QTimer::timeout, this, [this]()
	{
		m_isFastScrolling = false;
		updateVisibleTiles();
		emitViewCenterChanged();
	});

    auto triggerScroll = [this]() {
		m_debounceTimer->start();
        };

    // 3. 监听滚动条变?
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
		if (m_navigationSettleTimer != nullptr) m_navigationSettleTimer->start();
	};
	connect(horizontalScrollBar(), &QScrollBar::sliderReleased, this, onSilderReleased);
	connect(verticalScrollBar(), &QScrollBar::sliderReleased, this, onSilderReleased);
	connect(horizontalScrollBar(), &QScrollBar::sliderPressed, this, [this]()
	{
		m_isFastScrolling = true;
		if (m_navigationSettleTimer != nullptr) m_navigationSettleTimer->stop();
	});
	connect(verticalScrollBar(), &QScrollBar::sliderPressed, this, [this]()
	{
		m_isFastScrolling = true;
		if (m_navigationSettleTimer != nullptr) m_navigationSettleTimer->stop();
	});

	 

	// 获取视图窗口区间
	connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [=]() {
		QRect viewPortRect = viewport()->rect();

		// 1. 获取鼠标?Scene 中的坐标
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
	// 这个信号只描?SDK 自己的连续位置，不掺业务里程，保?SDK 和项目业务解耦?
	emit sigViewCenterSceneChanged(currentCenterScenePos());
	emit sigViewBottomAnchorChanged(currentBottomAnchor());

	if (!viewport())
	{
		return;
	}

	const QRect viewportRect = viewport()->rect();
	const QPointF sceneTopLeft = mapToScene(viewportRect.topLeft());
	const QPointF sceneBottomRight = mapToScene(viewportRect.bottomRight());
	QString topImageName;
	QString bottomImageName;
	int topX = 0;
	int topY = 0;
	int bottomX = 0;
	int bottomY = 0;

	if (GlobalSceneToMap(sceneTopLeft, topImageName, topX, topY) &&
		GlobalSceneToMap(sceneBottomRight, bottomImageName, bottomX, bottomY))
	{
		emit sigViewInfoChanged(
			topImageName,
			topX,
			topY,
			bottomImageName,
			bottomX,
			bottomY);
	}
}

void TiledGraphicsView::emitUserViewBottomAnchorChanged()
{
	// 这条线只表示“用户真的在浏览”，上层拿它?2D/3D 联动，避免程序定位后自己打回自己?
	emit sigUserViewBottomAnchorChanged(currentBottomAnchor());
}

void TiledGraphicsView::updateVisibleTiles()
{
	
	QElapsedTimer timer;
	timer.start();

	// 1. 获取可视区域
	QRect viewportRect = viewport()->rect();
	QRectF visibleSceneRect = mapToScene(viewportRect).boundingRect();
	if (m_contentMode == ContentMode::VirtualSequence)
	{
		m_currentScale = transform().m11();
		const bool highResolution = sequenceShouldUseHighResolution(m_currentScale);
		const qreal anchorWidth = qMax<qreal>(1.0, m_sequenceScheduleAnchorVisibleRect.width());
		const qreal anchorHeight = qMax<qreal>(1.0, m_sequenceScheduleAnchorVisibleRect.height());
		const bool movedPastBatch =
			!m_sequenceScheduleAnchorValid
			|| qAbs(visibleSceneRect.center().x() - m_sequenceScheduleAnchorVisibleRect.center().x()) >= anchorWidth * 0.5
			|| qAbs(visibleSceneRect.center().y() - m_sequenceScheduleAnchorVisibleRect.center().y()) >= anchorHeight * 0.5
			|| qAbs(visibleSceneRect.width() - anchorWidth) >= anchorWidth * 0.1
			|| qAbs(visibleSceneRect.height() - anchorHeight) >= anchorHeight * 0.1
			|| highResolution != m_sequenceScheduleAnchorHighResolution;
		if (movedPastBatch)
		{
			// Keep one request batch alive across small scroll/mouse updates.
			// Previously every 15 ms update cleared the queue and immediately
			// requested the same frames again, causing duplicate HDD reads and
			// making the cache progressively less useful.  Rotate only after half
			// a viewport (or a quality-mode change).
			m_sequenceDecodePool.clear();
			++m_sequenceRequestSerial;
			m_sequencePendingRequests.clear();
			m_sequenceScheduleAnchorVisibleRect = visibleSceneRect;
			m_sequenceScheduleAnchorValid = true;
			m_sequenceScheduleAnchorHighResolution = highResolution;
		}
		if (sequenceTraceEnabled())
		{
			const qint64 now = m_sequenceTraceClock.elapsed();
			if (now - m_sequenceLastViewportTraceMs >= 100)
			{
				m_sequenceLastViewportTraceMs = now;
				qInfo().noquote() << "[HN_SEQUENCE_TRACE][VIEWPORT_UPDATE]"
					<< "request=" << m_sequenceRequestSerial
					<< "visibleScene=" << visibleSceneRect
					<< "scale=" << transform().m11()
					<< "active=" << m_sequenceDecodePool.activeThreadCount()
					<< "cacheKiB=" << (m_sequencePreviewCache.totalCost() + m_sequenceFullCache.totalCost());
			}
		}
		const QPointF visibleCenter = visibleSceneRect.center();
		qreal primaryDelta = 0.0;
		if (m_sequenceLastVisibleCenterValid) {
			primaryDelta = isVerticalLayout(m_orientation)
				? visibleCenter.y() - m_sequenceLastVisibleCenter.y()
				: visibleCenter.x() - m_sequenceLastVisibleCenter.x();
		}
		m_sequenceLastVisibleCenter = visibleCenter;
		m_sequenceLastVisibleCenterValid = true;

		requestSequenceRange(visibleSceneRect, highResolution, 3);
		qreal beforeScreens = qMax(0, m_sequenceOptions.prefetchBackwardScreens);
		qreal afterScreens = qMax(0, m_sequenceOptions.prefetchForwardScreens);
		// 快速浏览缓冲区统一预取缩略图；不支持缩略图的数据源才预取原图。
		if (isReverseLayout(m_orientation)) qSwap(beforeScreens, afterScreens);

		// 根据最近一次实际滚动方向把更多预算放到即将进入视口的一侧。
		const qreal primarySpan = qMax<qreal>(1.0, isVerticalLayout(m_orientation)
			? visibleSceneRect.height() : visibleSceneRect.width());
		if (qAbs(primaryDelta) >= primarySpan * 0.02) {
			const qreal extra = m_isFastScrolling ? 1.5 : 0.5;
			if (primaryDelta > 0.0) {
				afterScreens += extra;
				beforeScreens *= 0.5;
			}
			else {
				beforeScreens += extra;
				afterScreens *= 0.5;
			}
		}
		QRectF prefetch = visibleSceneRect;
		if (isVerticalLayout(m_orientation))
			prefetch.adjust(0.0, -visibleSceneRect.height() * beforeScreens, 0.0, visibleSceneRect.height() * afterScreens);
		else
			prefetch.adjust(-visibleSceneRect.width() * beforeScreens, 0.0, visibleSceneRect.width() * afterScreens, 0.0);
		requestSequenceRange(prefetch, !sequenceUsesThumbnailPreview(), 1);
		viewport()->update();
		return;
	}

	// =========================================================
	// ?? 终极丝滑魔法：双层空间结界！
	// =========================================================

	// 【内层结界】：高清图的 Buffer (稍微外扩一点点，防边缘穿帮)
	double highResBuffer = 50.0;
	QRectF highResRect = visibleSceneRect.adjusted(-highResBuffer, -highResBuffer, highResBuffer, highResBuffer);

	// 【外层结界】：缩略图的“雷达预警区?(向外狂扩 1.5 个屏幕的宽度?
	// 这意味着用户还没滚到那里，提?1.5 个屏幕的缩略图就已经在后台悄悄解压了
	double prefetchX = visibleSceneRect.width() * 1.5;
	double prefetchY = visibleSceneRect.height() * 1.5;
	QRectF prefetchRect = visibleSceneRect.adjusted(-prefetchX, -prefetchY, prefetchX, prefetchY);

	// 整图源一张图片就是一个完整块。沿?50px 高清窗口会导致图片刚进入视口才开始解码，
	// 普通滚动稍快就容易露出黑底，所以整图模式单独放大预加载窗口，但仍不做全量常驻?
	// 整图预取沿当前 DB 主轴扩展更多屏，横向工程不再错误地只向 Y 方向预取。
	const bool databaseHorizontal = (m_databaseLayoutOrientation == LayoutOrientation::Horizontal ||
		m_databaseLayoutOrientation == LayoutOrientation::HorizontalReverse);
	const double wholeThumbX = visibleSceneRect.width() * (databaseHorizontal ? 3.0 : 1.0);
	const double wholeThumbY = visibleSceneRect.height() * (databaseHorizontal ? 1.0 : 3.0);
	const QRectF wholeThumbRect = visibleSceneRect.adjusted(-wholeThumbX, -wholeThumbY, wholeThumbX, wholeThumbY);
	const double wholeImageScreenCount = m_isFastScrolling ? 0.5 : 2.0;
	const double wholeImageX = visibleSceneRect.width() * (databaseHorizontal ? wholeImageScreenCount : 1.0);
	const double wholeImageY = visibleSceneRect.height() * (databaseHorizontal ? 1.0 : wholeImageScreenCount);
	const QRectF wholeImageRect = visibleSceneRect.adjusted(-wholeImageX, -wholeImageY, wholeImageX, wholeImageY);

	m_currentScale = transform().m11();

	// 2. DB 图片按主轴有序，二分找到当前预取区域，只调度附近 Item。
	//    旧实现每次滚动扫描全部 m_items，图片越多成本线性增长。
	const QRectF scheduleRect = prefetchRect.united(wholeThumbRect).united(wholeImageRect);
	const QList<TunnelSectionItem*> candidates = databaseItemsIntersecting(scheduleRect);
	QSet<TunnelSectionItem*> currentScheduled;
	for (TunnelSectionItem* item : candidates) if (item) currentScheduled.insert(item);

	// 只清理“上一批在调度区、这一批已经离开”的 Item，避免为了卸载资源再次扫描全部图片。
	for (TunnelSectionItem* oldItem : qAsConst(m_databaseScheduledItems)) {
		if (!oldItem || currentScheduled.contains(oldItem)) continue;
		oldItem->releaseThumbnail();
		oldItem->unloadAll();
	}

	for (TunnelSectionItem* item : candidates) {
		if (!item) continue;

		const QRectF itemRect = item->sceneBoundingRect();
		AbstractTileSource* source = item->getSource();
		const bool isWholeImage = source && source->sourceMode() == ImageSourceMode::WholeImage;

		if (isWholeImage)
		{
			if (wholeThumbRect.intersects(itemRect)) item->ensureThumbnailRequested();
			else item->releaseThumbnail();

			if (wholeImageRect.intersects(itemRect))
				item->updateVisibleTiles(wholeImageRect, m_currentScale, m_lodThresholdMultiplier);
			else
				item->unloadAll();
			continue;
		}

		if (prefetchRect.intersects(itemRect)) item->ensureThumbnailRequested();
		else item->releaseThumbnail();

		if (m_isFastScrolling) item->unloadAll();
		else item->updateVisibleTiles(highResRect, m_currentScale, m_lodThresholdMultiplier);
	}

	m_databaseScheduledItems = currentScheduled;
	viewport()->update();

    updateCurtainCompareResources();
    if (viewport()) viewport()->update();
}

void TiledGraphicsView::undoLastDrawPoint()
{
    // 既然画图?Tool 接管，撤销当然也直接甩锅给 Tool 去做?
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
    if (m_contentMode == ContentMode::VirtualSequence)
    {
		if (m_sequenceModel.isNull() || m_sequenceModel->isEmpty()) return 1.0;
		const QRectF itemsRect = m_sequenceModel->sceneRect();
		const QSize viewSize = viewport()->size();
		return isVerticalLayout(m_orientation)
			? static_cast<double>(qMax(1, viewSize.width())) / qMax<qreal>(1.0, itemsRect.width())
			: static_cast<double>(qMax(1, viewSize.height())) / qMax<qreal>(1.0, itemsRect.height());
	}
    if (m_items.isEmpty()) return 1.0;

    // 获取图片真实的物理边?
    QRectF itemsRect = m_scene->itemsBoundingRect();
    if (itemsRect.isEmpty()) return 1.0;

    QSize viewSize = viewport()->size();

    if (!isVerticalLayout(m_orientation)) {
        // 横向拼接：进度方向是 X，初始按高度铺满?
		int availableHeight = viewSize.height();
		if (itemsRect.width() * (double)availableHeight / itemsRect.height() > viewSize.width()
			&& horizontalScrollBar()
			&& !horizontalScrollBar()->isVisible())
		{
			// viewport() 已经扣掉可见滚动条；这里只在滚动条尚未出现但即将出现时预扣一次?
			availableHeight = qMax(1, availableHeight - horizontalScrollBar()->sizeHint().height());
		}
        return (double)availableHeight / itemsRect.height();
    }
    else {
        // 纵向拼接：进度方向是 Y，初始按宽度铺满?
		int availableWidth = viewSize.width();
		if (itemsRect.height() * (double)availableWidth / itemsRect.width() > viewSize.height()
			&& verticalScrollBar()
			&& !verticalScrollBar()->isVisible())
		{
			// viewport() 已经扣掉可见滚动条；这里只在滚动条尚未出现但即将出现时预扣一次?
			availableWidth = qMax(1, availableWidth - verticalScrollBar()->sizeHint().width());
		}
        return (double)availableWidth / itemsRect.width();
    }
}

QList<QGraphicsItem*> TiledGraphicsView::getVisualItems(QPoint viewPos)
{
	// ?? 1. 获取设备像素?(关键修改)
	// 如果宿主程序没开缩放，高分屏下这里会返回 1.25, 1.5, 2.0 ?
	// 如果开了缩放，或者普通屏，这里通常?1.0
	qreal ratio = viewport()->devicePixelRatio();

	// 2. 设定基础容差 (逻辑像素)
	int baseTolerance = 5;

	// 缩放很小时（上帝视角），线条很细，很难点中，所以要大幅扩大容差
	if (m_currentScale < 0.2) {
		baseTolerance = 10;
	}

	// ?? 3. 计算最终容?
	// 这样无论在什么屏幕上，物理点击面积都是差不多大的，手感一?
	int finalTolerance = static_cast<int>(baseTolerance * ratio);

	// 4. 构造点击矩?
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
	// 老入口不携带新业务上下文，避免上一次自定义 Tool 的 context 泄漏到旧信号语义。
	clearAnnotationDrawContext();
	// 切换到绘图模?
	m_currentMode = Mode_Draw;

	// 旧绘图入口永远使用内置 DefectDrawTool；外部自定义工具可通过 setActiveTool() 单独启用。
	if (m_currentTool) m_currentTool->deactivate();
	if (m_ownsCurrentTool && m_currentTool && m_currentTool != m_defaultDrawTool) {
		delete m_currentTool;
	}
	m_currentTool = m_defaultDrawTool;
	m_ownsCurrentTool = false;
	if (!m_defaultDrawTool) return;
	m_defaultDrawTool->ChangeShapeType(shapeType);
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
	else if (shapeType == Shape_Rectangle)
	{
		DrawModelStr = QString::fromLocal8Bit("绘图模式 [矩形]");
	}
    else if (shapeType == Shape_ObliqueRectangle)
    {
        DrawModelStr = QString::fromLocal8Bit("绘图模式 [斜矩形]");
    }
	else if (shapeType == Shape_ThreePointRectangle)
	{
		DrawModelStr = QString::fromLocal8Bit("绘图模式 [三点矩形]");
	}

	// ?? 强制触发一次可见性更新，立刻刷新左上角的 HUD 文字
	updateVisibleTiles();
}

void TiledGraphicsView::setLodLevel(int level) {
    // 1. 限制范围 1~10
    if (level < 1) level = 1;
    if (level > 10) level = 10;

    // 2. ?1~10 映射?1.5 ~ 0.5
    // Level 1  -> 1.5 (最晚显?
    // Level 10 -> 0.5 (最早显?

    // 步长 = (最大系?- 最小系? / (最大等?- 1)
    // 步长 = (1.5 - 0.5) / 9.0 ?0.1111

    double step = 1.25 / 9.0;

    m_lodThresholdMultiplier = 1.5 - (level - 1) * step;

    // 3. 立即刷新
    updateVisibleTiles();
}



bool TiledGraphicsView::GlobalSceneToMap(QPointF pt, QString & imageName, int& localX, int& localY)
{
	if (m_contentMode == ContentMode::VirtualSequence && m_coordinateMapper.isValid())
	{
		ImageCoordinate coordinate;
		if (!m_coordinateMapper.sceneToImage(pt, coordinate)) return false;
		const int sourceIndex = m_sequenceModel.isNull()
			? -1 : m_sequenceModel->sourceIndexForVisualIndex(coordinate.imageIndex);
		if (sourceIndex < 0) return false;
		const SequenceFrameDescriptor descriptor = imageDescriptor(sourceIndex);
		imageName = coordinate.imageName;
		localX = qBound(0, qRound(coordinate.localPos.x()), descriptor.imageSize.width());
		localY = qBound(0, qRound(coordinate.localPos.y()), descriptor.imageSize.height());
		return true;
	}
	const int itemIndex = databaseItemIndexAtScenePos(pt, false);
	if (itemIndex < 0 || itemIndex >= m_items.size()) return false;
	TunnelSectionItem* item = m_items.at(itemIndex);
	if (!item) return false;

	QPointF localPos = item->mapFromScene(pt);
	const QRectF localRect = item->boundingRect();
	if (!localRect.contains(localPos)) return false;

	imageName = item->getImageName();
	localX = qBound((int)localRect.left(), qRound(localPos.x()), (int)localRect.right());
	localY = qBound((int)localRect.top(), qRound(localPos.y()), (int)localRect.bottom());
	return true;
}

bool TiledGraphicsView::imageCoordinateToScene(const ImageCoordinate& coordinate, QPointF& scenePos) const
{
    scenePos = QPointF();
    if (!coordinate.valid && coordinate.imageIndex < 0 && coordinate.imageName.isEmpty()) return false;

    if (m_contentMode == ContentMode::VirtualSequence && m_coordinateMapper.isValid()) {
        // index 优先；工程重载后 index 失效时允许用稳定 imageName 兜底，与 DB 模式语义一致。
        if (coordinate.imageIndex >= 0
            && m_coordinateMapper.imageToScene(coordinate.imageIndex, coordinate.localPos, scenePos))
            return true;
        if (!coordinate.imageName.isEmpty())
            return m_coordinateMapper.imageNameToScene(coordinate.imageName, coordinate.localPos, scenePos);
        return false;
    }

    if (coordinate.imageIndex >= 0 && coordinate.imageIndex < m_items.size()) {
        TunnelSectionItem* item = m_items.at(coordinate.imageIndex);
        if (!item) return false;
        scenePos = item->mapToScene(coordinate.localPos);
        return true;
    }

    const QString targetName = normalizedImageBaseName(coordinate.imageName);
    for (TunnelSectionItem* item : m_items) {
        if (!item) continue;
        if (normalizedImageBaseName(item->getImageName()) == targetName) {
            scenePos = item->mapToScene(coordinate.localPos);
            return true;
        }
    }
    return false;
}

bool TiledGraphicsView::sceneToImageCoordinate(const QPointF& scenePos, ImageCoordinate& coordinate) const
{
    coordinate = ImageCoordinate();
    if (m_contentMode == ContentMode::VirtualSequence && m_coordinateMapper.isValid()) {
        return m_coordinateMapper.sceneToImage(scenePos, coordinate);
    }

    const int index = databaseItemIndexAtScenePos(scenePos, false);
    if (index < 0 || index >= m_items.size()) return false;
    TunnelSectionItem* item = m_items.at(index);
    if (!item) return false;
    const QPointF localPos = item->mapFromScene(scenePos);
    if (!item->boundingRect().contains(localPos)) return false;
    coordinate.valid = true;
    coordinate.imageIndex = index;
    coordinate.imageName = item->getImageName();
    coordinate.localPos = localPos;
    return true;
}

bool TiledGraphicsView::setHighLightAnnotation(const QString& layerKey, int uuid, qreal margin)
{
    AnnotationManager* manager = annotationManager(layerKey);
    if (!manager) return false;
    DefectShapeItem* item = manager->getItemUseId(uuid);
    if (!item || !item->isVisible()) return false;

    const QRectF sceneRect = item->sceneBoundingRect();
    const qreal safeMargin = qMax<qreal>(1.0, margin);
    QRectF targetRect(0, 0, qMax<qreal>(1.0, sceneRect.width() * safeMargin),
        qMax<qreal>(1.0, sceneRect.height() * safeMargin));
    targetRect.moveCenter(sceneRect.center());

    // 线图元某一维可能接近 0，给一个最小范围避免 fitInView 放大到异常比例。
    if (targetRect.width() < 20.0) targetRect.adjust(-10.0, 0.0, 10.0, 0.0);
    if (targetRect.height() < 20.0) targetRect.adjust(0.0, -10.0, 0.0, 10.0);

    fitInView(targetRect, Qt::KeepAspectRatio);
    m_currentScale = transform().m11();
    m_scene->clearSelection();
    item->setSelected(true);
    updateVisibleTiles();
    return true;
}

void TiledGraphicsView::setHighLightElement(int uuid, ElementType ele)
{
    // 旧接口严格保持原实现语义：仅 Disease 执行定位；其它历史类型虽然不处理，
    // 但仍会在函数末尾触发一次 updateVisibleTiles()。不要把这里改成新通用接口，
    // 否则会额外清空旧选择集、改写 m_currentScale，造成旧工程交互差异。
    if (ele == Type_Disease) {
        if (!m_defectManager) return;
        DefectShapeItem* item = m_defectManager->getItemUseId(uuid);
        if (!item) return;

        const QRectF sceneRect = item->sceneBoundingRect();
        const qreal margin = 1.85;
        QRectF targetRect(0, 0, sceneRect.width() * margin, sceneRect.height() * margin);
        targetRect.moveCenter(sceneRect.center());
        fitInView(targetRect, Qt::KeepAspectRatio);
        item->setSelected(true);
    }

    updateVisibleTiles();
}

void TiledGraphicsView::mouseDoubleClickEvent(QMouseEvent * event)
{
    // 老 TunnelViewer 交互：双击卷帘分割线，在横/竖方向之间切换。
    if (m_curtainEnabled && event->button() == Qt::LeftButton && isNearCurtainLine(event->pos())) {
        setCurtainOrientation(m_curtainOrientation == CurtainOrientation::Vertical
            ? CurtainOrientation::Horizontal : CurtainOrientation::Vertical);
        event->accept();
        return;
    }

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

QImage TiledGraphicsView::renderVirtualSequenceRegion(const QRectF& sceneRect, ExportQuality quality, bool drawDefects)
{
	if (sceneRect.isEmpty() || m_sequenceModel.isNull() || m_sequenceSource.isNull()) return QImage();
	const QSize targetSize(qCeil(sceneRect.width()), qCeil(sceneRect.height()));
	if (targetSize.width() <= 0 || targetSize.height() <= 0) return QImage();
	QImage result(targetSize, QImage::Format_RGB888);
	result.fill(Qt::white);
	QPainter painter(&result);
	painter.setWindow(sceneRect.toRect());
	painter.setViewport(result.rect());
	const int first = m_sequenceModel->firstVisualIndexIntersecting(sceneRect);
	const int last = m_sequenceModel->lastVisualIndexIntersecting(sceneRect);
	if (first >= 0 && last >= first)
	{
		for (int visual = first; visual <= last; ++visual)
		{
			const int sourceIndex = m_sequenceModel->sourceIndexForVisualIndex(visual);
			const QRectF frameRect = m_sequenceModel->frameRect(sourceIndex);
			if (!sceneRect.intersects(frameRect)) continue;
			QImage image = quality == Export_Thumbnail
				? m_sequenceSource->decodeThumbnail(static_cast<quint64>(sourceIndex), QSize(m_sequenceOptions.thumbnailMaxEdge, m_sequenceOptions.thumbnailMaxEdge))
				: m_sequenceSource->decodeFullImage(static_cast<quint64>(sourceIndex));
			if (image.isNull()) continue;
			painter.save();
			if (m_sequenceHorizontalMirror || m_sequenceVerticalMirror)
			{
				painter.translate(frameRect.left(), frameRect.top());
				painter.translate(m_sequenceHorizontalMirror ? frameRect.width() : 0.0,
					m_sequenceVerticalMirror ? frameRect.height() : 0.0);
				painter.scale(m_sequenceHorizontalMirror ? -1.0 : 1.0,
					m_sequenceVerticalMirror ? -1.0 : 1.0);
				painter.drawImage(QRectF(0, 0, frameRect.width(), frameRect.height()), image);
			}
			else painter.drawImage(frameRect, image);
			painter.restore();
		}
	}
	if (drawDefects)
	{
		for (SequenceChunkItem* chunk : qAsConst(m_sequenceChunks)) if (chunk) chunk->hide();
		const QBrush oldBrush = m_scene->backgroundBrush();
		m_scene->setBackgroundBrush(Qt::NoBrush);
		painter.setWindow(QRect(0, 0, targetSize.width(), targetSize.height()));
		painter.setViewport(result.rect());
		AnnotationExportGuard annotationGuard(m_annotationManagers);
		m_scene->render(&painter, result.rect(), sceneRect);
		m_scene->setBackgroundBrush(oldBrush);
		for (SequenceChunkItem* chunk : qAsConst(m_sequenceChunks)) if (chunk) chunk->show();
	}
	painter.end();
	return result;
}

QImage TiledGraphicsView::renderCachedVirtualSequencePreview(const QRectF& sceneRect)
{
	if (sceneRect.isEmpty() || m_sequenceModel.isNull() || m_sequenceSource.isNull())
	{
		return QImage();
	}
	const QSize targetSize(qCeil(sceneRect.width()), qCeil(sceneRect.height()));
	if (targetSize.width() <= 0 || targetSize.height() <= 0)
	{
		return QImage();
	}

	QImage result(targetSize, QImage::Format_RGB888);
	result.fill(Qt::white);
	QPainter painter(&result);
	painter.setWindow(sceneRect.toRect());
	painter.setViewport(result.rect());

	const int first = m_sequenceModel->firstVisualIndexIntersecting(sceneRect);
	const int last = m_sequenceModel->lastVisualIndexIntersecting(sceneRect);
	if (first >= 0 && last >= first)
	{
		for (int visual = first; visual <= last; ++visual)
		{
			const int sourceIndex = m_sequenceModel->sourceIndexForVisualIndex(visual);
			const QRectF frameRect = m_sequenceModel->frameRect(sourceIndex);
			if (!sceneRect.intersects(frameRect))
			{
				continue;
			}

			QImage image;
			if (QImage* full = m_sequenceFullCache.object(sequenceCacheKey(sourceIndex, true)))
			{
				image = *full;
			}
			else
			{
				if (QImage* thumbnail = m_sequencePreviewCache.object(sequenceCacheKey(sourceIndex, false)))
				{
					image = *thumbnail;
				}
				else
				{
					requestSequenceImage(sourceIndex, false, 3);
				}
				// One asynchronous full decode replaces repeated synchronous
				// full-file reads previously performed on every mouse move.
				requestSequenceImage(sourceIndex, true, 4);
			}
			if (image.isNull())
			{
				continue;
			}

			painter.save();
			if (m_sequenceHorizontalMirror || m_sequenceVerticalMirror)
			{
				painter.translate(frameRect.left(), frameRect.top());
				painter.translate(m_sequenceHorizontalMirror ? frameRect.width() : 0.0,
					m_sequenceVerticalMirror ? frameRect.height() : 0.0);
				painter.scale(m_sequenceHorizontalMirror ? -1.0 : 1.0,
					m_sequenceVerticalMirror ? -1.0 : 1.0);
				painter.drawImage(QRectF(0, 0, frameRect.width(), frameRect.height()), image);
			}
			else
			{
				painter.drawImage(frameRect, image);
			}
			painter.restore();
		}
	}
	painter.end();
	return result;
}

void TiledGraphicsView::setImageAdjustments(const ImageDisplayAdjustments& adjustments)
{
	ImageDisplayAdjustments bounded;
	bounded.brightness = qBound(-100, adjustments.brightness, 100);
	bounded.contrast = qBound(0, adjustments.contrast, 200);
	bounded.sharpen = qBound(0, adjustments.sharpen, 100);
	if (m_imageAdjustments.brightness == bounded.brightness
		&& m_imageAdjustments.contrast == bounded.contrast
		&& m_imageAdjustments.sharpen == bounded.sharpen) return;
	m_imageAdjustments = bounded;

	if (m_contentMode == ContentMode::VirtualSequence)
	{
		// Brightness is applied during decode. Invalidate only the virtual sequence
		// generation so stale results cannot overwrite the new brightness setting.
		++m_sequenceGeneration;
		m_sequenceDecodePool.clear();
		m_sequencePendingRequests.clear();
		m_sequencePreviewCache.clear();
		m_sequenceFullCache.clear();
		updateVisibleTiles();
		return;
	}

	// Preserve the existing DB thumbnail/tile pipeline; only ask it to rebuild
	// currently visible items using its established brightness-aware cache keys.
	AsyncImageLoader::instance()->setImageAdjustments(
		bounded.brightness, bounded.contrast, bounded.sharpen);
	for (TunnelSectionItem* item : m_items)
	{
		if (item) item->unloadAll();
	}
	updateVisibleTiles();
}

void TiledGraphicsView::setImageBrightness(int value)
{
    // 保留 SDK0831 API，只修改亮度；对比度和锐化维持调用者当前设置。
    ImageDisplayAdjustments next = m_imageAdjustments;
    next.brightness = qBound(-100, value, 100);
    setImageAdjustments(next);
}

//TODO 新增功能 20260310 增加图像导出功能?
QPixmap TiledGraphicsView::exportRegionData(const QRectF& sceneRect, ExportQuality quality, bool drawDefects)
{
	if (m_contentMode == ContentMode::VirtualSequence)
		return QPixmap::fromImage(renderVirtualSequenceRegion(sceneRect, quality, drawDefects));
	if (sceneRect.isEmpty() || m_items.isEmpty()) return QPixmap();
	const QList<TunnelSectionItem*> exportItems = databaseItemsIntersecting(sceneRect);

	QSize targetSize(qCeil(sceneRect.width()), qCeil(sceneRect.height()));
	/*if (targetSize.width() > 16384 || targetSize.height() > 16384) {
		qWarning() << "Capture region is too large; scaling it proportionally.";
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

	for (TunnelSectionItem* item : exportItems) {
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
	// 绘制矢量病害?
	// =========================================================
	if (drawDefects) {
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		// 1. 隐藏隧道底图
		for (TunnelSectionItem* item : exportItems) {
			item->hide();
		}

		// ?? 2. 核心修复：临时抽?Scene 的“黑背景”，防止它覆盖我们拼好的图片?
		QBrush oldBgBrush = m_scene->backgroundBrush();
		m_scene->setBackgroundBrush(Qt::NoBrush);

		// 3. 充当翻译官：统一两边的坐标系
		painter.setWindow(0, 0, targetSize.width(), targetSize.height());
		painter.setViewport(resultImage.rect());

		// 4. 画病害！此时因为背景?NoBrush，病害会直接以透明底盖在我们的图片?
		AnnotationExportGuard annotationGuard(m_annotationManagers);
		m_scene->render(&painter, resultImage.rect(), sceneRect);

		// ?? 5. 打扫战场：把背景色和底图全还给界面，做到神不知鬼不觉
		m_scene->setBackgroundBrush(oldBgBrush);
		for (TunnelSectionItem* item : exportItems) {
			item->show();
		}
	}

	painter.end();
	return QPixmap::fromImage(resultImage);
}

QPixmap TiledGraphicsView::previewRegionData(const QRectF& sceneRect)
{
	if (m_contentMode == ContentMode::VirtualSequence)
	{
		return QPixmap::fromImage(renderCachedVirtualSequencePreview(sceneRect));
	}
	return exportRegionData(sceneRect, Export_HighRes, false);
}







// ==================== TunnelViewer 可选卷帘模块 ====================
void TiledGraphicsView::updateCurtainCompareResources()
{
	if (!m_curtainEnabled || m_compareCurtainFrames.isEmpty()) return;

	const QRect clipViewport = curtainViewportRect();
	if (clipViewport.isEmpty()) return;

	const QRectF visibleSceneRect = mapToScene(clipViewport).boundingRect();
	QSet<int> neededFrames;
	QSet<QString> neededTiles;
	QList<QPair<int, QPair<int, int>>> tileRequests;
	const bool highResolution = m_currentScale >= (0.15 * m_lodThresholdMultiplier);

	for (const BaseCurtainFrame& baseFrame : m_baseCurtainFrames) {
		if (!baseFrame.item || !baseFrame.info.hasRealMileage) continue;

		const QRectF baseRect = baseFrame.item->sceneBoundingRect();
		const QRectF targetVisibleRect = baseRect.intersected(visibleSceneRect);
		if (targetVisibleRect.isEmpty() || baseRect.width() <= 0.0 || baseRect.height() <= 0.0) {
			continue;
		}

		const double visibleMileage1 =
			mileageAtSceneX(baseFrame.info, baseRect, targetVisibleRect.left());
		const double visibleMileage2 =
			mileageAtSceneX(baseFrame.info, baseRect, targetVisibleRect.right());
		const double visibleMin = qMin(visibleMileage1, visibleMileage2);
		const double visibleMax = qMax(visibleMileage1, visibleMileage2);

		for (int frameIndex = 0; frameIndex < m_compareCurtainFrames.size(); ++frameIndex) {
			const DbImageInfo& compareInfo = m_compareCurtainFrames[frameIndex].info;
			const double overlapStart = qMax(visibleMin, mileageMin(compareInfo));
			const double overlapEnd = qMin(visibleMax, mileageMax(compareInfo));
			if (overlapEnd <= overlapStart) continue;

			neededFrames.insert(frameIndex);
			if (!highResolution) continue;

			const double sourceX1 = imageXAtMileage(compareInfo, overlapStart);
			const double sourceX2 = imageXAtMileage(compareInfo, overlapEnd);
			const double sourceLeft = qBound(0.0, qMin(sourceX1, sourceX2),
				static_cast<double>(compareInfo.width));
			const double sourceRight = qBound(0.0, qMax(sourceX1, sourceX2),
				static_cast<double>(compareInfo.width));

			const double sourceTop = qBound(0.0,
				((targetVisibleRect.top() - baseRect.top()) / baseRect.height())
				* compareInfo.height,
				static_cast<double>(compareInfo.height));
			const double sourceBottom = qBound(0.0,
				((targetVisibleRect.bottom() - baseRect.top()) / baseRect.height())
				* compareInfo.height,
				static_cast<double>(compareInfo.height));

			const int tileSize = qMax(1, compareInfo.tileSize);
			const int startCol = qMax(0, qFloor(sourceLeft / tileSize));
			const int endCol = qMax(startCol, qFloor(qMax(sourceLeft, sourceRight - 0.01) / tileSize));
			const int startRow = qMax(0, qFloor(sourceTop / tileSize));
			const int endRow = qMax(startRow, qFloor(qMax(sourceTop, sourceBottom - 0.01) / tileSize));

			for (int row = startRow; row <= endRow; ++row) {
				for (int col = startCol; col <= endCol; ++col) {
					const QString requestUri = QString("%1|%2|%3")
						.arg(compareInfo.dbFilePath).arg(col).arg(row);
					neededTiles.insert(requestUri);
					tileRequests.append(qMakePair(frameIndex, qMakePair(col, row)));
				}
			}
		}
	}

	m_curtainNeededFrames = neededFrames;
	m_curtainNeededTiles = neededTiles;

	for (int frameIndex = 0; frameIndex < m_compareCurtainFrames.size(); ++frameIndex) {
		CompareCurtainFrame& frame = m_compareCurtainFrames[frameIndex];
		if (!neededFrames.contains(frameIndex)) {
			frame.thumbnail = QPixmap();
			frame.loadedTiles.clear();
			continue;
		}

		auto tileIt = frame.loadedTiles.begin();
		while (tileIt != frame.loadedTiles.end()) {
			const int col = tileIt.key().x / qMax(1, frame.info.tileSize);
			const int row = tileIt.key().y / qMax(1, frame.info.tileSize);
			const QString requestUri = QString("%1|%2|%3")
				.arg(frame.info.dbFilePath).arg(col).arg(row);
			if (!neededTiles.contains(requestUri)) {
				tileIt = frame.loadedTiles.erase(tileIt);
			}
			else {
				++tileIt;
			}
		}

		requestCurtainThumbnail(frameIndex);
	}

	for (const auto& request : tileRequests) {
		if (m_curtainPendingTiles.size() >= 50) break;
		requestCurtainTile(request.first, request.second.first, request.second.second);
	}
}

void TiledGraphicsView::paintCurtainCompare(QPainter* painter, const QRectF& exposedRect)
{
	if (!m_curtainEnabled || !painter) return;

	const QRect clipViewport = curtainViewportRect();
	if (clipViewport.isEmpty()) return;

	QPainterPath clipPath;
	clipPath.addPolygon(mapToScene(clipViewport));

	painter->save();
	painter->setClipPath(clipPath, Qt::IntersectClip);
	painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

	auto drawMappedPixmap = [painter](const QRectF& targetRect, const QPixmap& pixmap,
		const QRectF& sourceRect, bool reverseHorizontally) {
		if (targetRect.isEmpty() || sourceRect.isEmpty() || pixmap.isNull()) return;
		if (!reverseHorizontally) {
			painter->drawPixmap(targetRect, pixmap, sourceRect);
			return;
		}

		painter->save();
		painter->translate(targetRect.left() + targetRect.right(), 0.0);
		painter->scale(-1.0, 1.0);
		painter->drawPixmap(targetRect, pixmap, sourceRect);
		painter->restore();
	};

	for (const BaseCurtainFrame& baseFrame : m_baseCurtainFrames) {
		if (!baseFrame.item || !baseFrame.info.hasRealMileage) continue;

		const QRectF baseRect = baseFrame.item->sceneBoundingRect();
		const QRectF visibleBaseRect = baseRect.intersected(exposedRect);
		if (visibleBaseRect.isEmpty()) continue;

		const double baseMin = mileageMin(baseFrame.info);
		const double baseMax = mileageMax(baseFrame.info);

		for (const CompareCurtainFrame& compareFrame : m_compareCurtainFrames) {
			const DbImageInfo& compareInfo = compareFrame.info;
			const double overlapStart = qMax(baseMin, mileageMin(compareInfo));
			const double overlapEnd = qMin(baseMax, mileageMax(compareInfo));
			if (overlapEnd <= overlapStart) continue;

			const double targetX1 = sceneXAtMileage(baseFrame.info, baseRect, overlapStart);
			const double targetX2 = sceneXAtMileage(baseFrame.info, baseRect, overlapEnd);
			const double sourceX1 = imageXAtMileage(compareInfo, overlapStart);
			const double sourceX2 = imageXAtMileage(compareInfo, overlapEnd);

			QRectF targetRect(qMin(targetX1, targetX2), baseRect.top(),
				qAbs(targetX2 - targetX1), baseRect.height());
			const double clippedTargetLeft = qMax(targetRect.left(), visibleBaseRect.left());
			const double clippedTargetRight = qMin(targetRect.right(), visibleBaseRect.right());
			if (clippedTargetRight <= clippedTargetLeft) continue;
			targetRect = QRectF(clippedTargetLeft, baseRect.top(),
				clippedTargetRight - clippedTargetLeft, baseRect.height());

			const double targetDelta = targetX2 - targetX1;
			const double sourceDelta = sourceX2 - sourceX1;
			if (qFuzzyIsNull(targetDelta) || qFuzzyIsNull(sourceDelta)) continue;

			auto sourceXFromTarget = [&](double targetX) {
				return sourceX1 + ((targetX - targetX1) / targetDelta) * sourceDelta;
			};

			const double clippedSourceX1 = sourceXFromTarget(targetRect.left());
			const double clippedSourceX2 = sourceXFromTarget(targetRect.right());
			const QRectF sourceRect(qMin(clippedSourceX1, clippedSourceX2), 0.0,
				qAbs(clippedSourceX2 - clippedSourceX1), compareInfo.height);
			const bool reverseHorizontally = targetDelta * sourceDelta < 0.0;

			if (!compareFrame.thumbnail.isNull()) {
				// 缩略图像素尺寸通常小于原图，原图裁剪坐标需按比例换算。
				const qreal thumbnailScaleX = static_cast<qreal>(compareFrame.thumbnail.width())
					/ compareInfo.width;
				const QRectF thumbnailSourceRect(
					sourceRect.left() * thumbnailScaleX,
					0.0,
					sourceRect.width() * thumbnailScaleX,
					compareFrame.thumbnail.height());
				drawMappedPixmap(targetRect, compareFrame.thumbnail, thumbnailSourceRect,
					reverseHorizontally);
			}

			for (auto tileIt = compareFrame.loadedTiles.constBegin();
				tileIt != compareFrame.loadedTiles.constEnd(); ++tileIt) {
				const QPixmap& tile = tileIt.value();
				if (tile.isNull()) continue;

				const QRectF tileSourceRect(tileIt.key().x, tileIt.key().y,
					tile.width(), tile.height());
				const QRectF sourceIntersection = sourceRect.intersected(tileSourceRect);
				if (sourceIntersection.isEmpty()) continue;

				auto targetXFromSource = [&](double sourceX) {
					return targetX1 + ((sourceX - sourceX1) / sourceDelta) * targetDelta;
				};

				const double tileTargetX1 = targetXFromSource(sourceIntersection.left());
				const double tileTargetX2 = targetXFromSource(sourceIntersection.right());
				const double targetTop = baseRect.top()
					+ (sourceIntersection.top() / compareInfo.height) * baseRect.height();
				const double targetBottom = baseRect.top()
					+ (sourceIntersection.bottom() / compareInfo.height) * baseRect.height();

				const QRectF tileTargetRect(qMin(tileTargetX1, tileTargetX2), targetTop,
					qAbs(tileTargetX2 - tileTargetX1), targetBottom - targetTop);
				const QRectF tileLocalSource = sourceIntersection.translated(
					-tileSourceRect.left(), -tileSourceRect.top());
				drawMappedPixmap(tileTargetRect, tile, tileLocalSource,
					reverseHorizontally);
			}
		}
	}

	painter->restore();
}

void TiledGraphicsView::drawForeground(QPainter* painter, const QRectF& rect)
{
	QGraphicsView::drawForeground(painter, rect);
	if (!m_curtainEnabled || !painter) return;

	painter->save();
	painter->resetTransform();
	QPen linePen(QColor(255, 196, 0), 2.0);
	linePen.setCosmetic(true);
	painter->setPen(linePen);

	if (m_curtainOrientation == CurtainOrientation::Vertical) {
		painter->drawLine(m_curtainLinePosition, 0,
			m_curtainLinePosition, viewport()->height());
	}
	else {
		painter->drawLine(0, m_curtainLinePosition,
			viewport()->width(), m_curtainLinePosition);
	}
	painter->restore();
}

bool TiledGraphicsView::setCurtainCompareImages(const QList<DbImageInfo>& compareImages)
{
	clearCurtainCompare();

	for (const DbImageInfo& info : compareImages) {
		if (!info.hasRealMileage || info.width <= 0 || info.height <= 0
			|| info.dbFilePath.isEmpty()) {
			continue;
		}

		CompareCurtainFrame frame;
		frame.info = info;
		m_compareCurtainFrames.append(frame);
	}

	std::sort(m_compareCurtainFrames.begin(), m_compareCurtainFrames.end(),
		[](const CompareCurtainFrame& left, const CompareCurtainFrame& right) {
		return mileageMin(left.info) < mileageMin(right.info);
	});

	qDebug() << "[CurtainCompare] compare gray images mileageCount"
		<< m_compareCurtainFrames.size();
	return !m_compareCurtainFrames.isEmpty();
}

bool TiledGraphicsView::setCurtainCompareEnabled(bool enabled)
{
	if (enabled) {
		bool hasBaseMileage = false;
		for (const BaseCurtainFrame& frame : m_baseCurtainFrames) {
			if (frame.item && frame.info.hasRealMileage) {
				hasBaseMileage = true;
				break;
			}
		}

		if (!hasBaseMileage || m_compareCurtainFrames.isEmpty()) {
			qWarning() << "[CurtainCompare] enable failed: base or compare mileage missing";
			m_curtainEnabled = false;
			return false;
		}

		m_curtainEnabled = true;
		if (m_curtainLinePosition < 0) {
			m_curtainLinePosition = (m_curtainOrientation == CurtainOrientation::Vertical)
				? viewport()->width() / 2 : viewport()->height() / 2;
		}
		updateCurtainCompareResources();
	}
	else {
		m_curtainEnabled = false;
		m_isDraggingCurtain = false;
		m_curtainNeededFrames.clear();
		m_curtainNeededTiles.clear();
		m_curtainPendingTiles.clear();
		for (CompareCurtainFrame& frame : m_compareCurtainFrames) {
			frame.thumbnail = QPixmap();
			frame.loadedTiles.clear();
			frame.thumbnailLoading = false;
		}
	}

	viewport()->update();
	return true;
}

void TiledGraphicsView::setCurtainOrientation(CurtainOrientation orientation)
{
	m_curtainOrientation = orientation;
	m_curtainLinePosition = (orientation == CurtainOrientation::Vertical)
		? viewport()->width() / 2 : viewport()->height() / 2;
	updateCurtainCompareResources();
	viewport()->update();
}

void TiledGraphicsView::setCurtainPosition(qreal ratio)
{
	ratio = qBound<qreal>(0.0, ratio, 1.0);
	m_curtainLinePosition = qRound(ratio
		* (m_curtainOrientation == CurtainOrientation::Vertical
			? viewport()->width() : viewport()->height()));
	updateCurtainCompareResources();
	viewport()->update();
}

qreal TiledGraphicsView::curtainPosition() const
{
	const int length = (m_curtainOrientation == CurtainOrientation::Vertical)
		? viewport()->width() : viewport()->height();
	if (length <= 0 || m_curtainLinePosition < 0) return 0.5;
	return qBound<qreal>(0.0, static_cast<qreal>(m_curtainLinePosition) / length, 1.0);
}

void TiledGraphicsView::clearCurtainCompare()
{
	setCurtainCompareEnabled(false);
	m_compareCurtainFrames.clear();
}

QRect TiledGraphicsView::curtainViewportRect() const
{
	const QRect fullRect = viewport()->rect();
	if (!m_curtainEnabled) return QRect();

	if (m_curtainOrientation == CurtainOrientation::Vertical) {
		return QRect(fullRect.left(), fullRect.top(),
			qBound(0, m_curtainLinePosition, fullRect.width()), fullRect.height());
	}

	return QRect(fullRect.left(), fullRect.top(), fullRect.width(),
		qBound(0, m_curtainLinePosition, fullRect.height()));
}

bool TiledGraphicsView::isNearCurtainLine(const QPoint& viewportPos) const
{
	if (!m_curtainEnabled || m_curtainLinePosition < 0) return false;
	const int tolerance = qMax(5, qRound(5.0 * viewport()->devicePixelRatio()));
	return m_curtainOrientation == CurtainOrientation::Vertical
		? qAbs(viewportPos.x() - m_curtainLinePosition) <= tolerance
		: qAbs(viewportPos.y() - m_curtainLinePosition) <= tolerance;
}

int TiledGraphicsView::findCompareFrame(double mileage) const
{
	for (int i = 0; i < m_compareCurtainFrames.size(); ++i) {
		const DbImageInfo& info = m_compareCurtainFrames[i].info;
		if (mileage >= mileageMin(info) && mileage <= mileageMax(info)) return i;
	}
	return -1;
}

void TiledGraphicsView::requestCurtainThumbnail(int frameIndex)
{
	if (frameIndex < 0 || frameIndex >= m_compareCurtainFrames.size()) return;
	CompareCurtainFrame& frame = m_compareCurtainFrames[frameIndex];
	if (!frame.thumbnail.isNull() || frame.thumbnailLoading) return;

	frame.thumbnailLoading = true;
	AsyncImageLoader::instance()->requestThumbnail(frame.info.dbFilePath, QSize(2048, 2048));
}

void TiledGraphicsView::requestCurtainTile(int frameIndex, int col, int row)
{
	if (frameIndex < 0 || frameIndex >= m_compareCurtainFrames.size()) return;
	CompareCurtainFrame& frame = m_compareCurtainFrames[frameIndex];
	const TileKey key = { col * frame.info.tileSize, row * frame.info.tileSize };
	if (frame.loadedTiles.contains(key) || frame.loadedTiles.size() >= 200) return;

	const QString requestUri = QString("%1|%2|%3")
		.arg(frame.info.dbFilePath).arg(col).arg(row);
	if (m_curtainPendingTiles.contains(requestUri)) return;

	CurtainPendingTile pending;
	pending.frameIndex = frameIndex;
	pending.key = key;
	m_curtainPendingTiles.insert(requestUri, pending);
	AsyncImageLoader::instance()->requestImage(requestUri);
}

void TiledGraphicsView::recreateCurtainItem()
{
	if (!m_scene || m_curtainItem) return;
	m_curtainItem = new CurtainCompareItem(this);
	m_scene->addItem(m_curtainItem);
	m_curtainItem->setRect(m_scene->sceneRect());
}

//TODO 新增功能 增加图像导出功能，返?cv::Mat 灰度图格?(极速单通道零拷贝版)
cv::Mat TiledGraphicsView::exportRegionGrayMat(const QRectF& sceneRect, ExportQuality quality, bool drawDefects)
{
	if (m_contentMode == ContentMode::VirtualSequence)
	{
		QImage image = renderVirtualSequenceRegion(sceneRect, quality, drawDefects).convertToFormat(QImage::Format_Grayscale8);
		if (image.isNull()) return cv::Mat();
		return cv::Mat(image.height(), image.width(), CV_8UC1, image.bits(), image.bytesPerLine()).clone();
	}
	if (sceneRect.isEmpty() || m_items.isEmpty()) return cv::Mat();
	const QList<TunnelSectionItem*> exportItems = databaseItemsIntersecting(sceneRect);

	QSize targetSize(qCeil(sceneRect.width()), qCeil(sceneRect.height()));
	//if (targetSize.width() > 16384 || targetSize.height() > 16384) {
	//	qWarning() << "Capture region is too large; scaling it proportionally.";
	//	targetSize.scale(16384, 16384, Qt::KeepAspectRatio);
	//}

	QImage resultImage(targetSize, QImage::Format_Grayscale8);
	resultImage.fill(Qt::white);

	QPainter painter(&resultImage);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

	painter.setWindow(sceneRect.toRect());
	painter.setViewport(resultImage.rect());

	for (TunnelSectionItem* item : exportItems) {
		QRectF itemRect = item->sceneBoundingRect();
		if (!sceneRect.intersects(itemRect)) continue;

		AbstractTileSource* source = item->getSource();

		if (quality == Export_Thumbnail) {
			// 缩略图逻辑（按需放开?
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

	// 绘制矢量病害?
	if (drawDefects) {
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

		for (TunnelSectionItem* item : exportItems) {
			item->hide();
		}

		QBrush oldBgBrush = m_scene->backgroundBrush();
		m_scene->setBackgroundBrush(Qt::NoBrush);

		painter.setWindow(0, 0, targetSize.width(), targetSize.height());
		painter.setViewport(resultImage.rect());

		// ?? 这里哪怕你界面上画的是大红大黄的线段，最终砸?finalMat 也会自动变成灰阶线段
		AnnotationExportGuard annotationGuard(m_annotationManagers);
		m_scene->render(&painter, resultImage.rect(), sceneRect);

		m_scene->setBackgroundBrush(oldBgBrush);
		for (TunnelSectionItem* item : exportItems) {
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
	if (m_contentMode == ContentMode::VirtualSequence)
	{
		QImage image = renderVirtualSequenceRegion(sceneRect, quality, drawDefects).convertToFormat(QImage::Format_RGB888);
		if (image.isNull()) return cv::Mat();
		cv::Mat rgb(image.height(), image.width(), CV_8UC3, image.bits(), image.bytesPerLine());
		if (!returnBgr) return rgb.clone();
		cv::Mat bgr; cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR); return bgr;
	}
	if (sceneRect.isEmpty() || m_items.isEmpty()) {
		return cv::Mat();
	}
	const QList<TunnelSectionItem*> exportItems = databaseItemsIntersecting(sceneRect);

	QSize targetSize(qCeil(sceneRect.width()), qCeil(sceneRect.height()));
	if (targetSize.width() <= 0 || targetSize.height() <= 0) {
		return cv::Mat();
	}

	//if (targetSize.width() > 16384 || targetSize.height() > 16384) {
	//	qWarning() << "Capture region is too large; scaling it proportionally.";
	//	targetSize.scale(16384, 16384, Qt::KeepAspectRatio);
	//}

	QImage resultImage(targetSize, QImage::Format_RGB888);
	resultImage.fill(Qt::white);

	QPainter painter(&resultImage);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

	painter.setWindow(sceneRect.toRect());
	painter.setViewport(resultImage.rect());

	for (TunnelSectionItem* item : exportItems) {
		QRectF itemRect = item->sceneBoundingRect();
		if (!sceneRect.intersects(itemRect)) {
			continue;
		}

		AbstractTileSource* source = item->getSource();
		if (!source) {
			continue;
		}

		if (quality == Export_Thumbnail) {
			// 你当前原函数里缩略图逻辑也是注释掉的，这里先保持一?
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
	// 绘制矢量病害?
	// 这里仍然是画?resultImage，但 resultImage 背后就是 matRgb.data
	// =========================================================
	if (drawDefects) {
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

		for (TunnelSectionItem* item : exportItems) {
			item->hide();
		}

		QBrush oldBgBrush = m_scene->backgroundBrush();
		m_scene->setBackgroundBrush(Qt::NoBrush);

		painter.setWindow(0, 0, targetSize.width(), targetSize.height());
		painter.setViewport(resultImage.rect());

		AnnotationExportGuard annotationGuard(m_annotationManagers);
		m_scene->render(&painter, resultImage.rect(), sceneRect);

		m_scene->setBackgroundBrush(oldBgBrush);

		for (TunnelSectionItem* item : exportItems) {
			item->show();
		}
	}

	painter.end();

	cv::Mat matRgb(resultImage.height(),
		resultImage.width(),
		CV_8UC3,
		resultImage.bits(),
		resultImage.bytesPerLine());

	// 如果后续还要 Qt 显示、或者你只是转灰度识别，可以直接 return matRgb，最?
	if (!returnBgr) {
		return matRgb.clone();
	}

	// OpenCV ?imwrite / 大多数算法默认按 BGR 解释彩色?
	// 需要保存正常颜色时，再?BGR
	cv::Mat matBgr;
	cv::cvtColor(matRgb, matBgr, cv::COLOR_RGB2BGR);
	return matBgr;
}


QPixmap TiledGraphicsView::exportRegionDataToWord(const QRectF& sceneRect, ExportQuality quality /*= Export_HighRes*/, bool drawDefects /*= true*/)
{
	if (m_contentMode == ContentMode::VirtualSequence)
	{
		QImage image = renderVirtualSequenceRegion(sceneRect, quality, drawDefects);
		if (image.width() > 1500) image = image.scaledToWidth(1500, Qt::SmoothTransformation);
		return QPixmap::fromImage(image);
	}
	if (sceneRect.isEmpty() || m_items.isEmpty()) return QPixmap();
	const QList<TunnelSectionItem*> exportItems = databaseItemsIntersecting(sceneRect);

	QSize targetSize(qCeil(sceneRect.width()), qCeil(sceneRect.height()));
	/*if (targetSize.width() > 16384 || targetSize.height() > 16384) {
	qWarning() << "Capture region is too large; scaling it proportionally.";
	targetSize.scale(16384, 16384, Qt::KeepAspectRatio);
	}*/

	//QImage resultImage(targetSize, QImage::Format_ARGB32_Premultiplied);
	QImage resultImage(targetSize, QImage::Format_RGB888);

	// ?? 修复?1：JPG不支持透明底，一律填成干净的白?(或你需要的底图颜色)
	resultImage.fill(Qt::white);

	QPainter painter(&resultImage);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

	// ?? 修复?2：上帝级坐标系映射！
	// 告诉画家：“你现在的画板代表的是真实的物理 SceneRect?
	painter.setWindow(sceneRect.toRect());
	painter.setViewport(resultImage.rect());

	for (TunnelSectionItem* item : exportItems) {
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
	// 绘制矢量病害?
	// =========================================================
	if (drawDefects) {
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		// 1. 隐藏隧道底图
		for (TunnelSectionItem* item : exportItems) {
			item->hide();
		}

		// ?? 2. 核心修复：临时抽?Scene 的“黑背景”，防止它覆盖我们拼好的图片?
		QBrush oldBgBrush = m_scene->backgroundBrush();
		m_scene->setBackgroundBrush(Qt::NoBrush);

		// 3. 充当翻译官：统一两边的坐标系
		painter.setWindow(0, 0, targetSize.width(), targetSize.height());
		painter.setViewport(resultImage.rect());

		// 4. 画病害！此时因为背景?NoBrush，病害会直接以透明底盖在我们的图片?
		AnnotationExportGuard annotationGuard(m_annotationManagers);
		m_scene->render(&painter, resultImage.rect(), sceneRect);

		// ?? 5. 打扫战场：把背景色和底图全还给界面，做到神不知鬼不觉
		m_scene->setBackgroundBrush(oldBgBrush);
		for (TunnelSectionItem* item : exportItems) {
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
	if (m_contentMode == ContentMode::VirtualSequence && !m_sequenceModel.isNull() && !m_sequenceSource.isNull())
	{
		const int sourceIndex = m_sequenceModel->sourceIndexForName(qstrImageName);
		if (sourceIndex < 0) return cv::Mat();
		QImage image = m_sequenceSource->decodeFullImage(static_cast<quint64>(sourceIndex)).convertToFormat(QImage::Format_RGB888);
		if (image.isNull()) return cv::Mat();
		cv::Mat rgb(image.height(), image.width(), CV_8UC3, image.bits(), image.bytesPerLine());
		cv::Mat bgr; cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR); return bgr;
	}
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

	// 每列一个数?
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
	// 先按列读?
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

	// 检查高度是否都一?
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
	if (m_contentMode == ContentMode::VirtualSequence && !m_sequenceModel.isNull() && !m_sequenceModel->isEmpty())
		return m_sequenceModel->descriptor(0).imageSize.height() - 10.0;
	if (m_items.size() > 0)
	{
		return (double)m_items[0]->boundingRect().height() - 10;
	}

	return 0.0;
}

double TiledGraphicsView::getImageWidth()
{ 
	if (m_contentMode == ContentMode::VirtualSequence && !m_sequenceModel.isNull() && !m_sequenceModel->isEmpty())
		return m_sequenceModel->descriptor(0).imageSize.width();
	if (m_items.size() > 0)
	{ 
		return (double)m_items[0]->boundingRect().width();
	}

	return 0.0;
}

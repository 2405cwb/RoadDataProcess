#include "./items/DefectShapeItem.h"
#include <QPen>
#include <QUuid>
#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
// 构造函数保持不变
DefectShapeItem::DefectShapeItem(const QPainterPath& path, const QString& name, const DrawShape shape, int uuid)
    : QGraphicsPathItem(path), m_name(name), m_shape(shape) {
  
    m_isClosed =
        shape == Shape_Polygon ||
        shape == Shape_Rectangle ||
        shape == Shape_ObliqueRectangle ||
        shape == Shape_ThreePointRectangle;

	ringEndPt = QPointF(0, 0);
     
	setFlags(ItemIsSelectable  | ItemSendsGeometryChanges);
	//setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
    m_uuid = uuid;

    // 动画定时器 (保持之前的闪烁逻辑)
    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        m_dashOffset++;
        if (m_dashOffset > 10) m_dashOffset = 0;
        update();
        });
}

void DefectShapeItem::finalizeMove()
{
	if (pos() != QPointF(0,0))
	{
		QTransform t;
		t.translate(pos().x(), pos().y());
		setPath(t.map(path()));
		setPos(0, 0);
		emit sigDefectMoved(this);
	}
}

 

// 选中动画保持不变
QVariant DefectShapeItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemSelectedHasChanged) {
        if (value.toBool()) m_animTimer->start(50);
        else m_animTimer->stop();
    }
    
    else if (change == ItemPositionChange && scene()) {
		if (isLocked())
		{
			return pos();
		}

        QPointF newPos = value.toPointF(); 

        //  限制一：轴向锁定 (轨道锁)
        // 因为我们在 mouseReleaseEvent 里把偏移量合并到了 path 中并把 pos 归零了
        // 所以这里的 newPos 就是相对于原始 path 的【纯净偏移量】
        if (m_moveAxis == Axis_Horizontal) {
            newPos.setY(0); // 锁死 Y 轴偏移
        }
        else if (m_moveAxis == Axis_Vertical) {
            newPos.setX(0); // 锁死 X 轴偏移
        }

        // 🛡️ 限制二：活动范围锁定 (物理结界)
        if (m_moveBounds.isValid()) {
            // 获取图形本身的物理边界
            QRectF rect = path().boundingRect();
            // 预测移动后的物理边界
            QRectF nextRect = rect.translated(newPos);

            // X 轴碰壁检测：如果预测边界超出了给定的结界，就把 newPos 硬拽回来
            if (nextRect.left() < m_moveBounds.left()) {
                newPos.setX(m_moveBounds.left() - rect.left());
            }
            else if (nextRect.right() > m_moveBounds.right()) {
                newPos.setX(m_moveBounds.right() - rect.right());
            }

            // Y 轴碰壁检测
            if (nextRect.top() < m_moveBounds.top()) {
                newPos.setY(m_moveBounds.top() - rect.top());
            }
            else if (nextRect.bottom() > m_moveBounds.bottom()) {
                newPos.setY(m_moveBounds.bottom() - rect.bottom());
            }
        }

       
        return newPos;
    }

    return QGraphicsPathItem::itemChange(change, value);
}

// 完美的防遮挡 paint 保持不变
QPainterPath DefectShapeItem::shape() const
{
    QPainterPath base = QGraphicsPathItem::shape();
    if (m_shape == Shape_Line || path().elementCount() <= 2) {
        QPainterPathStroker stroker;
        // 老 TunnelViewer 的拱脚线需要较宽点击区域；只扩大 hit-test，不改 path 本身。
        const qreal width = hitTolerance();
        stroker.setWidth(qMax(width, pen().widthF() + 8.0));
        return base.united(stroker.createStroke(path()));
    }
    return base;
}

void DefectShapeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    // 1. 绘制病害线条/图形本身
	double exportScale = 1.0;
	if (widget == nullptr)
	{
		exportScale = qMax(1.0, (double)painter->device()->width() / 1500.0);
	}
    QPen drawPen = pen();
    if (isSelected()) {
        drawPen.setColor(QColor(153,51,255));
        drawPen.setStyle(Qt::CustomDashLine);
        drawPen.setDashPattern({ 4, 4 });
        drawPen.setDashOffset(m_dashOffset);
    }
	if (widget == nullptr)
	{
		drawPen.setCosmetic(false);
		double baseWidth = qMax(3.0, drawPen.widthF());
		drawPen.setWidthF(baseWidth * exportScale * 3.0);
		drawPen.setCapStyle(Qt::RoundCap);
		drawPen.setJoinStyle(Qt::RoundJoin);
	}
	else
	{
		drawPen.setCosmetic(true);

	}
    painter->setPen(drawPen);
    painter->setBrush(brush());

    painter->drawPath(path());

    // 2. 绘制文字标签 (防遮挡 + 防出界版)
    if (m_name.isEmpty()) return;
    // 新属性优先；旧 TunnelViewer 拱脚线仍沿用不显示文字的历史语义。
    if (!shouldShowLabel()) return;

    const double lod = option->levelOfDetailFromTransform(painter->worldTransform());
    if (lod < labelMinLod()) return;

    painter->save();

	int fontSize = 12;
	//TODO 新增优化 20260227 全局保存字体防止重复创建，提升性能 
	static  QFont f = painter->font();
	static bool isFontInit = false;
	if (!isFontInit) {
		f.setPointSize(12);
		f.setBold(true);
		isFontInit = true;
	}

	QFontMetrics fm(f);
	int tw = fm.width(m_name) + 10;
	int th = fm.height() + 4;

    QPointF scenePos;
    if (path().elementCount() > 0) {

		// boundingRect() 对复杂 path 也有计算成本，一次 paint 只计算一次。
		const QRectF pathBounds = path().boundingRect();
		scenePos = pathBounds.center();

		if (m_elementType == Type_Ring && ringEndPt.x() != 0)
		{
			scenePos.rx() = (scenePos.x() + ringEndPt.x()) / 2 - tw;
		}

        //scenePos = QPointF(path().elementAt(0).x, path().elementAt(0).y);
    }
    else {
        scenePos = boundingRect().topLeft();
    }

    QPointF pixelPos = painter->worldTransform().map(scenePos);
    painter->setWorldTransform(QTransform());



	// ==========================================
	//  动态感知环境并计算放大倍数
	// ==========================================
//	double exportScale = 1.0;

	// 如果 widget 为 nullptr，说明现在没有屏幕，正在向后台的 QImage 里导出数据！
	/*if (widget == nullptr) {
		 painter->device()->width() 能获取当前生成图片的总像素宽度
		 假设正常屏幕宽 1500，如果导出图是 6000 像素宽，exportScale 就是 4.0 倍！
		exportScale = qMax(1.0, (double)painter->device()->width() / 1500.0);
	}*/

    qreal targetX = pixelPos.x();
    qreal targetY = pixelPos.y() - 20;

    if (widget) {
        QRect viewportRect = widget->rect();
        if (targetX + tw > viewportRect.right())  targetX = viewportRect.right() - tw - 5;
        if (targetX < viewportRect.left())        targetX = viewportRect.left() + 5;
        if (targetY < viewportRect.top())         targetY = pixelPos.y() + 20;
        if (targetY + th > viewportRect.bottom()) targetY = viewportRect.bottom() - th - 5;
    }

    QRectF bgRect(targetX, targetY, tw, th); 
    //TODO 新增功能 20260227 修改病害文字绘制效果 
    QColor defectColor = pen().color();                 // 获取病害自身的颜色 (红/黄/蓝)
    painter->setPen(QPen(defectColor, 1));              // 用病害颜色做 1 像素精细描边
    //painter->setBrush(QColor(15, 15, 15, 210));         // 加深黑底的浓度 (210)，让白色文字对比度极高
    //painter->drawRoundedRect(bgRect, 4, 4);
    painter->setFont(f);
    painter->drawText(bgRect, Qt::AlignCenter, m_name);

    painter->restore();
}
 
// =========================================================
// 通用行为属性：显式属性优先，未配置时保持历史 ElementType 行为。
// =========================================================
bool DefectShapeItem::isSelectableByPolicy() const
{
    const QVariant value = m_attributes.value(AnnotationAttributeKey::Selectable);
    return value.isValid() ? value.toBool() : true;
}

bool DefectShapeItem::isMovableByPolicy() const
{
    const QVariant value = m_attributes.value(AnnotationAttributeKey::Movable);
    if (value.isValid()) return value.toBool() && !isLocked();
    return !isLocked();
}

bool DefectShapeItem::isRubberBandSelectableByPolicy() const
{
    const QVariant value = m_attributes.value(AnnotationAttributeKey::RubberBandSelectable);
    if (value.isValid()) return value.toBool();
    // 保持旧逻辑：历史框选只针对病害，其他业务图元默认不参与。
    return m_elementType == Type_Disease;
}

bool DefectShapeItem::isExportableByPolicy() const
{
    const QVariant value = m_attributes.value(AnnotationAttributeKey::Exportable);
    return value.isValid() ? value.toBool() : true;
}

bool DefectShapeItem::shouldShowLabel() const
{
    const QVariant value = m_attributes.value(AnnotationAttributeKey::ShowLabel);
    if (value.isValid()) return value.toBool();
    return m_elementType != Type_SPRINGING_LOC;
}

qreal DefectShapeItem::labelMinLod() const
{
    const QVariant value = m_attributes.value(AnnotationAttributeKey::LabelMinLod);
    // 0.01 是旧版本硬编码阈值，作为默认值可以保证旧工程显示效果不变。
    return value.isValid() ? qMax<qreal>(0.0, value.toDouble()) : 0.01;
}

bool DefectShapeItem::isInteractiveInViewMode(ViewMode mode, bool legacyTunnelLocationMoveEnabled) const
{
    // Manager 的图层 selectable=false 会清掉 ItemIsSelectable；这里也尊重最终 Flag，
    // 防止 View 的自定义拖动引擎绕过图层级禁选状态。
    if (!flags().testFlag(QGraphicsItem::ItemIsSelectable) || !isSelectableByPolicy()) return false;

    if (mode == Mode_Move) {
        const QVariant value = m_attributes.value(AnnotationAttributeKey::InteractiveInMoveMode);
        if (value.isValid()) return value.toBool();
        // 保持旧逻辑：Move 模式历史上只用于轨枕。
        return m_elementType == Type_Sleeper;
    }

    if (mode == Mode_Browse) {
        const QVariant value = m_attributes.value(AnnotationAttributeKey::InteractiveInBrowseMode);
        if (value.isValid()) return value.toBool();
        // 保持旧 TunnelViewer 逻辑：拱脚定位线默认只可选，显式开启后才参与拖动。
        return m_elementType != Type_SPRINGING_LOC || legacyTunnelLocationMoveEnabled;
    }

    return false;
}

qreal DefectShapeItem::hitTolerance() const
{
    const QVariant value = m_attributes.value(AnnotationAttributeKey::HitTolerance);
    if (value.isValid()) return qMax<qreal>(1.0, value.toDouble());
    return (m_elementType == Type_SPRINGING_LOC || m_elementType == Type_UserLine) ? 50.0 : 14.0;
}

// =========================================================
// 把图元转换成纯数据。
// 注意：AnnotationData 结构体保持旧布局，新元数据全部落到既有 attributes。
// =========================================================
DefectData DefectShapeItem::toData() const {
    DefectData data;
    data.uuid = m_uuid;
    data.name = m_name;
    data.type = m_shape;
    data.shape = path();
    data.attributes = m_attributes;

    data.attributes[AnnotationAttributeKey::FormatVersion] = 2;
    data.attributes[AnnotationAttributeKey::LegacyElementType] = static_cast<int>(m_elementType);
    if (!m_layerKey.isEmpty()) data.attributes[AnnotationAttributeKey::LayerKey] = m_layerKey;
    if (!m_typeKey.isEmpty()) data.attributes[AnnotationAttributeKey::TypeKey] = m_typeKey;
    return data;
}

// =========================================================
// 从纯数据恢复。旧数据没有 SDK 元数据时，继续依赖 Manager/旧 ElementType 兜底。
// =========================================================
void DefectShapeItem::fromData(const DefectData& data) {
    fromData(data, ElementTypePersistenceProfile::UnifiedModern);
}

void DefectShapeItem::fromData(const DefectData& data, ElementTypePersistenceProfile profile) {
    m_uuid = data.uuid;
    m_name = data.name;
    m_shape = static_cast<DrawShape>(data.type);
    m_isClosed =
        m_shape == Shape_Polygon ||
        m_shape == Shape_Rectangle ||
        m_shape == Shape_ObliqueRectangle ||
        m_shape == Shape_ThreePointRectangle;

    m_attributes = data.attributes;
    m_layerKey = m_attributes.value(AnnotationAttributeKey::LayerKey).toString();
    m_typeKey = m_attributes.value(AnnotationAttributeKey::TypeKey).toString();

    const QVariant legacyType = m_attributes.value(AnnotationAttributeKey::LegacyElementType);
    if (legacyType.isValid()) {
        m_elementType = elementTypeFromPersistedValue(legacyType.toInt(), profile);
    }
    if (m_typeKey.isEmpty()) m_typeKey = annotationTypeKeyFromLegacyElement(m_elementType);
    if (m_layerKey.isEmpty()) m_layerKey = annotationLayerKeyFromLegacyElement(m_elementType);

    m_attributes[AnnotationAttributeKey::TypeKey] = m_typeKey;
    m_attributes[AnnotationAttributeKey::LayerKey] = m_layerKey;
    setPath(data.shape);
}

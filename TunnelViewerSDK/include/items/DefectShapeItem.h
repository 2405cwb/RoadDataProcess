#pragma once

#include <QObject>
#include <QGraphicsPathItem>
#include <QTimer>
#include <QVariantMap>
#include <QtWidgets>

#include "TunnelGlobal.h"

/* 标注拖动方向 老名字保留 */ enum MoveAxis {
    Axis_Free = 0,
    Axis_Horizontal,
    Axis_Vertical,
};

class DefectShapeItem : public QObject, public QGraphicsPathItem {
    Q_OBJECT
public:
    /* 创建一个可绘制可选择的标注图形 */ DefectShapeItem(const QPainterPath& path, const QString& name, const DrawShape shape, int uuid);
    /* 设置拖动方向 比如环片只准横向拖 */ void setMoveAxis(MoveAxis axis) { m_moveAxis = axis; }
    /* 设置拖动边界 QRectF 无效时就是不限制 */ void setMoveBounds(const QRectF& bounds) { m_moveBounds = bounds; }
    /* 图层锁定与图元自身 locked 分开，避免切换图层锁后丢失图元原有锁定状态。 */ void setLayerLocked(bool locked) { m_layerLocked = locked; update(); }
    bool isLayerLocked() const { return m_layerLocked; }
    /* 鼠标释放后把临时偏移合并进 path */ void finalizeMove();
    /* 锁住后还能显示和选中 但不能拖动 */ void setLocked(bool locked) {
        m_isLocked = locked;
        update();
    }
    /* 查询当前是不是锁住了；图元自身锁定或所属图层锁定任一成立即不可移动。 */ bool isLocked() const { return m_isLocked || m_layerLocked; }
    /* 把图元转成纯数据 存储层只拿这个 */ DefectData toData() const;
    /* 从纯数据恢复图元；默认按统一新格式解释 ElementType。 */ void fromData(const DefectData& data);
    /* 兼容历史格式：显式指定持久化 profile，解决老 TunnelViewer 7/8 冲突。 */
    void fromData(const DefectData& data, ElementTypePersistenceProfile profile);
    /* 写入一个业务属性 SDK 不解释 key */ void setAttribute(const QString& key, const QVariant& value) { m_attributes[key] = value; }
    /* 读取一个业务属性 没有就返回空 QVariant */ QVariant getAttribute(const QString& key) const { return m_attributes.value(key); }
    /* 一次性取出全部业务属性 */ QVariantMap getAttributes() const { return m_attributes; }
    /* 批量覆盖业务属性，主要供兼容加载和默认配置合并使用。 */ void setAttributes(const QVariantMap& attributes) { m_attributes = attributes; }

    /* 新通用类型键。旧业务仍可继续使用 m_elementType。 */
    void setTypeKey(const QString& typeKey) { m_typeKey = typeKey; m_attributes[AnnotationAttributeKey::TypeKey] = typeKey; }
    QString typeKey() const { return m_typeKey; }

    /* 新通用图层键。用于 Manager Registry 与外部业务分层。 */
    void setLayerKey(const QString& layerKey) { m_layerKey = layerKey; m_attributes[AnnotationAttributeKey::LayerKey] = layerKey; }
    QString layerKey() const { return m_layerKey; }

    /* 统一行为查询：显式 attributes 优先，未配置时继续执行旧 ElementType 语义。 */
    bool isSelectableByPolicy() const;
    bool isMovableByPolicy() const;
    bool isRubberBandSelectableByPolicy() const;
    bool isExportableByPolicy() const;
    bool shouldShowLabel() const;
    qreal labelMinLod() const;
    qreal hitTolerance() const;
    /* 当前 View 模式下是否允许作为拖动目标。显式属性优先，缺省保持旧业务规则。 */
    bool isInteractiveInViewMode(ViewMode mode, bool legacyTunnelLocationMoveEnabled = false) const;

    /* 返回标注唯一 id */ int getUuid() const { return m_uuid; }
    /* 返回显示名 */ QString getName() const { return m_name; }
    /* 返回点线面类型 */ DrawShape getShapeType() const { return m_shape; }
    /* 返回当前图形是不是闭合面 */ bool isClosed() const { return m_isClosed; }
    // 扩大细线鼠标命中区域，但绝不改写真实 path，避免保存/导出几何被污染。
    QPainterPath shape() const override;

    /* 老业务元素类型 新业务建议逐步改用 attributes/category */ ElementType m_elementType = Type_Disease;
    /* 环片旧逻辑需要的结束点 先保留 */ QPointF ringEndPt;

signals:
    /* 拖动结束后通知 Manager */ void sigDefectMoved(DefectShapeItem* item);

protected:
    /* Qt 图元变化入口 拖动限制和选中动画都在这里 */ QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    /* 自己负责画几何和文字标签 导出时也复用 */ void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    int m_uuid = 0;
    QString m_name;
    DrawShape m_shape = Shape_Line;
    bool m_isClosed = false;
    QVariantMap m_attributes;
    QString m_typeKey;
    QString m_layerKey;
    QTimer* m_animTimer = nullptr;
    int m_dashOffset = 0;
    MoveAxis m_moveAxis = Axis_Free;
    QRectF m_moveBounds;
    bool m_isLocked = false;
    bool m_layerLocked = false;
};

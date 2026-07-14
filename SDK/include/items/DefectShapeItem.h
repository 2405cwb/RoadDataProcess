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
    /* 鼠标释放后把临时偏移合并进 path */ void finalizeMove();
    /* 锁住后还能显示和选中 但不能拖动 */ void setLocked(bool locked) {
        m_isLocked = locked;
        update();
    }
    /* 查询当前是不是锁住了 */ bool isLocked() const { return m_isLocked; }
    /* 把图元转成纯数据 存储层只拿这个 */ DefectData toData() const;
    /* 从纯数据恢复图元 */ void fromData(const DefectData& data);
    /* 写入一个业务属性 SDK 不解释 key */ void setAttribute(const QString& key, const QVariant& value) { m_attributes[key] = value; }
    /* 读取一个业务属性 没有就返回空 QVariant */ QVariant getAttribute(const QString& key) const { return m_attributes.value(key); }
    /* 一次性取出全部业务属性 */ QVariantMap getAttributes() const { return m_attributes; }
    /* 返回标注唯一 id */ int getUuid() const { return m_uuid; }
    /* 返回显示名 */ QString getName() const { return m_name; }
    /* 返回点线面类型 */ DrawShape getShapeType() const { return m_shape; }
    /* 返回当前图形是不是闭合面 */ bool isClosed() const { return m_isClosed; }

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
    QTimer* m_animTimer = nullptr;
    int m_dashOffset = 0;
    MoveAxis m_moveAxis = Axis_Free;
    QRectF m_moveBounds;
    bool m_isLocked = false;
};

#pragma once

#include "hnapplication_global.h"
#include <QList>
#include <QColor>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QString>

class QGraphicsItem;
class QGraphicsScene;

class HNAPPLICATION_EXPORT hnSdkDiseaseGraphicsLayer
{
public:
	// 构造 SDK 病害图层适配器；真正的 scene 由 setScene() 注入。
	hnSdkDiseaseGraphicsLayer();

	// 析构时清理当前图层创建的所有 QGraphicsItem，避免 scene 中残留悬空 item。
	~hnSdkDiseaseGraphicsLayer();

	// 设置当前承载病害、临时框、打标标识的 QGraphicsScene；切换 scene 时会先清理旧 item。
	void setScene(QGraphicsScene* scene);

	// 清空本图层维护的所有内容：临时绘制、已提交临时 item、正式病害、材质打标。
	void clearAll();

	// 清空正在绘制过程中的临时 item，例如鼠标拖动时的虚线框、轨迹线。
	void clearTemporary();

	// 清空已经从临时层提交但尚未归入正式病害集合的 item；当前主要作为兼容接口保留。
	void clearCommitted();

	// 清空正式病害 item 和当前选中 key，不影响临时绘制和材质打标。
	void clearDiseases();

	// 清空路面材质、标准、等级等打标标识 item，不影响病害 item。
	void clearMaterialMarks();

	// 把当前临时 item 转入 committed 集合；SDK 正式病害迁移后通常不再依赖该流程。
	void commitTemporary();

	// 丢弃当前临时绘制 item，用于取消绘制或病害成功入库后清掉预览框。
	void discardTemporary();

	// 在 scene 中添加一个临时矩形，坐标必须是 SDK scene 坐标。
	void addTemporaryRect(const QRectF& rect, const QColor& color, int width, Qt::PenStyle style);

	// 在 scene 中添加一个临时路径，坐标必须是 SDK scene 坐标，常用于折线或轨迹预览。
	void addTemporaryPath(const QPainterPath& path, const QColor& color, int width, Qt::PenStyle style);

	// 添加正式病害路径；diseaseKey 是应用层生成的不透明唯一键，本图层不理解数据库表含义。
	void addDiseasePath(const QString& diseaseKey, const QPainterPath& path, const QString& label,
		const QColor& color, int width, Qt::PenStyle style);

	// 设置当前选中的病害 key，用于命中、标签字号和高亮状态判断。
	void setSelectedDiseaseKey(const QString& diseaseKey);

	// 设置标签避让所需的当前可见 scene 区域和视图缩放比例，保证文字保持屏幕可读大小。
	void setLabelPlacementContext(const QRectF& visibleSceneRect, qreal viewScaleX, qreal viewScaleY, int gap);

	// 清空标签布局上下文；未设置上下文时标签按默认 scene 坐标规则放置。
	void clearLabelPlacementContext();

	// 添加路面材质/标准等打标边界标识：横向色带、边界线、可选标签和箭头。
	void addMaterialBoundaryMark(qreal sceneY, qreal sceneWidth, const QString& label,
		const QColor& lineColor, const QColor& fillColor, int lineWidth = 2,
		Qt::PenStyle lineStyle = Qt::SolidLine, qreal bandHeightPixels = 36.0,
		int labelRow = 0);

	// 根据 SDK scene 坐标命中正式病害 item，返回应用层 diseaseKey；未命中返回空字符串。
	QString diseaseKeyAt(const QPointF& scenePos) const;

	// 查询指定病害 key 对应 item 的 scene 包围盒，用于列表跳转后居中显示。
	bool diseaseSceneRect(const QString& diseaseKey, QRectF& rect) const;

private:
	// 从 scene 删除并释放指定集合中的所有 item，同时清空集合。
	void clearItems(QList<QGraphicsItem*>& items);

	// 按当前颜色、线宽、线型生成画笔；内部会处理线宽下限和缩放下的可读性。
	QPen makePen(const QColor& color, int width, Qt::PenStyle style) const;

	// 从命中的子 item 向上寻找正式病害根 item，保证点击标签或引线也能选中同一个病害。
	QGraphicsItem* diseaseRootItem(QGraphicsItem* item) const;

	// 判断某个 scene 矩形是否与当前可见区域相交，用于避免不可见病害的标签贴到窗口边缘。
	bool isSceneRectVisible(const QRectF& sceneRect) const;

	// 根据病害包围盒、标签尺寸和可见区域计算标签位置，并返回引线目标点。
	QPointF labelScenePosForPath(const QRectF& pathRect, const QSizeF& textSceneSize, QPointF& labelAnchor) const;

	QGraphicsScene* m_scene = nullptr;
	QList<QGraphicsItem*> m_temporaryItems;
	QList<QGraphicsItem*> m_committedItems;
	QList<QGraphicsItem*> m_diseaseItems;
	QList<QGraphicsItem*> m_materialMarkItems;
	QString m_selectedDiseaseKey;
	QColor m_selectedDiseaseColor = Qt::red;
	QRectF m_visibleSceneRect;
	qreal m_viewScaleX = 1.0;
	qreal m_viewScaleY = 1.0;
	int m_labelGap = 12;
	bool m_hasLabelPlacementContext = false;
};
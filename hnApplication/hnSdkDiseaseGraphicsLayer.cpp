#include "hnSdkDiseaseGraphicsLayer.h"

#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QLineF>
#include <QtGlobal>

namespace
{
	const int DiseaseKeyRole = 0;
	const int DiseaseKindRole = 1;
	const int DiseaseColorRole = 2;
	const int DiseaseWidthRole = 3;
	const int DiseaseStyleRole = 4;
	const char* DiseaseKind = "disease";
}

hnSdkDiseaseGraphicsLayer::hnSdkDiseaseGraphicsLayer()
{
}

hnSdkDiseaseGraphicsLayer::~hnSdkDiseaseGraphicsLayer()
{
	clearAll();
}

void hnSdkDiseaseGraphicsLayer::setScene(QGraphicsScene* scene)
{
	if (m_scene == scene)
	{
		return;
	}

	clearAll();
	m_scene = scene;
}

void hnSdkDiseaseGraphicsLayer::clearAll()
{
	clearTemporary();
	clearCommitted();
	clearDiseases();
	clearMaterialMarks();
}

void hnSdkDiseaseGraphicsLayer::clearTemporary()
{
	clearItems(m_temporaryItems);
}

void hnSdkDiseaseGraphicsLayer::clearCommitted()
{
	clearItems(m_committedItems);
}

void hnSdkDiseaseGraphicsLayer::clearDiseases()
{
	clearItems(m_diseaseItems);
	m_selectedDiseaseKey.clear();
}

void hnSdkDiseaseGraphicsLayer::clearMaterialMarks()
{
	clearItems(m_materialMarkItems);
}

void hnSdkDiseaseGraphicsLayer::commitTemporary()
{
	m_committedItems.append(m_temporaryItems);
	m_temporaryItems.clear();
}

void hnSdkDiseaseGraphicsLayer::discardTemporary()
{
	clearTemporary();
}

void hnSdkDiseaseGraphicsLayer::setLabelPlacementContext(const QRectF& visibleSceneRect, qreal viewScaleX, qreal viewScaleY, int gap)
{
	m_visibleSceneRect = visibleSceneRect.normalized();
	m_viewScaleX = qMax<qreal>(0.001, qAbs(viewScaleX));
	m_viewScaleY = qMax<qreal>(0.001, qAbs(viewScaleY));
	m_labelGap = qMax(0, gap);
	m_hasLabelPlacementContext = m_visibleSceneRect.isValid() && !m_visibleSceneRect.isEmpty();
}

void hnSdkDiseaseGraphicsLayer::clearLabelPlacementContext()
{
	m_visibleSceneRect = QRectF();
	m_viewScaleX = 1.0;
	m_viewScaleY = 1.0;
	m_labelGap = 12;
	m_hasLabelPlacementContext = false;
}

void hnSdkDiseaseGraphicsLayer::addTemporaryRect(const QRectF& rect, const QColor& color, int width, Qt::PenStyle style)
{
	if (!m_scene || rect.isNull() || !rect.isValid())
	{
		return;
	}

	QGraphicsRectItem* item = new QGraphicsRectItem(rect.normalized());
	item->setPen(makePen(color, width, style));
	item->setBrush(Qt::NoBrush);
	item->setAcceptedMouseButtons(Qt::NoButton);
	item->setFlag(QGraphicsItem::ItemIsSelectable, false);
	item->setZValue(100000.0);
	m_scene->addItem(item);
	m_temporaryItems.append(item);
}

void hnSdkDiseaseGraphicsLayer::addTemporaryPath(const QPainterPath& path, const QColor& color, int width, Qt::PenStyle style)
{
	if (!m_scene || path.isEmpty())
	{
		return;
	}

	QGraphicsPathItem* item = new QGraphicsPathItem(path);
	item->setPen(makePen(color, width, style));
	item->setAcceptedMouseButtons(Qt::NoButton);
	item->setFlag(QGraphicsItem::ItemIsSelectable, false);
	item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	item->setZValue(100001.0);
	m_scene->addItem(item);
	m_temporaryItems.append(item);
}

void hnSdkDiseaseGraphicsLayer::addMaterialBoundaryMark(qreal sceneY, qreal sceneWidth, const QString& label,
	const QColor& lineColor, const QColor& fillColor, int lineWidth, Qt::PenStyle lineStyle, qreal bandHeightPixels,
	int labelRow)
{
	if (!m_scene || sceneWidth <= 0.0)
	{
		return;
	}

	const qreal bandHeight = qMax<qreal>(8.0, bandHeightPixels) / m_viewScaleY;
	const QRectF bandRect(0.0, sceneY - bandHeight / 2.0, sceneWidth, bandHeight);
	const bool markVisible = isSceneRectVisible(bandRect);

	QGraphicsRectItem* band = new QGraphicsRectItem(bandRect);
	band->setPen(Qt::NoPen);
	band->setBrush(QBrush(fillColor));
	band->setAcceptedMouseButtons(Qt::NoButton);
	band->setZValue(15000.0);
	m_scene->addItem(band);
	m_materialMarkItems.append(band);

	QGraphicsLineItem* line = new QGraphicsLineItem(QLineF(0.0, sceneY, sceneWidth, sceneY));
	line->setPen(makePen(lineColor, qMax(1, lineWidth), lineStyle));
	line->setAcceptedMouseButtons(Qt::NoButton);
	line->setZValue(15001.0);
	m_scene->addItem(line);
	m_materialMarkItems.append(line);

	if (!label.isEmpty() && markVisible)
	{
		QFont font(QStringLiteral("SimHei"));
		font.setBold(true);
		font.setPixelSize(14);
		QFontMetrics metrics(font);
		const qreal gapX = m_labelGap / m_viewScaleX;
		const qreal rowGap = (8.0 + qMax(0, labelRow) * 18.0) / m_viewScaleY;
		const qreal textHeightScene = metrics.height() / m_viewScaleY;
		const qreal x = gapX;
		const qreal y = sceneY - textHeightScene - rowGap;

		QGraphicsSimpleTextItem* text = new QGraphicsSimpleTextItem(label);
		text->setFont(font);
		text->setBrush(QBrush(lineColor));
		text->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
		text->setAcceptedMouseButtons(Qt::NoButton);
		text->setZValue(15003.0);
		text->setPos(QPointF(x, y));
		m_scene->addItem(text);
		m_materialMarkItems.append(text);

		const qreal arrowStartX = x + qMax<qreal>(120.0, metrics.width(label) + 10.0) / m_viewScaleX;
		const qreal arrowLen = 70.0 / m_viewScaleX;
		const qreal arrowSizeX = 8.0 / m_viewScaleX;
		const qreal arrowSizeY = 5.0 / m_viewScaleY;
		const qreal arrowEndX = qMin(sceneWidth, arrowStartX + arrowLen);
		QPainterPath callout;
		callout.moveTo(arrowStartX, y + textHeightScene * 0.5);
		callout.lineTo(arrowEndX, sceneY);
		callout.moveTo(arrowEndX, sceneY);
		callout.lineTo(arrowEndX - arrowSizeX, sceneY - arrowSizeY);
		callout.moveTo(arrowEndX, sceneY);
		callout.lineTo(arrowEndX - arrowSizeX, sceneY + arrowSizeY);

		QGraphicsPathItem* calloutItem = new QGraphicsPathItem(callout);
		calloutItem->setPen(makePen(lineColor, 2, Qt::SolidLine));
		calloutItem->setAcceptedMouseButtons(Qt::NoButton);
		calloutItem->setZValue(15002.0);
		m_scene->addItem(calloutItem);
		m_materialMarkItems.append(calloutItem);
	}
}
void hnSdkDiseaseGraphicsLayer::addDiseasePath(const QString& diseaseKey, const QPainterPath& path, const QString& label,
	const QColor& color, int width, Qt::PenStyle style)
{
	if (!m_scene || diseaseKey.isEmpty() || path.isEmpty())
	{
		return;
	}

	QGraphicsPathItem* item = new QGraphicsPathItem(path);
	item->setPen(makePen(color, width, style));
	// Dense little-frame diseases can be rendered as one bounding rect.
	// Transparent fill keeps the rect interior hittable without changing visual appearance.
	item->setBrush(QBrush(QColor(0, 0, 0, 0)));
	item->setAcceptedMouseButtons(Qt::NoButton);
	item->setFlag(QGraphicsItem::ItemIsSelectable, false);
	item->setData(DiseaseKeyRole, diseaseKey);
	item->setData(DiseaseKindRole, QString::fromLatin1(DiseaseKind));
	item->setData(DiseaseColorRole, color);
	item->setData(DiseaseWidthRole, qMax(1, width));
	item->setData(DiseaseStyleRole, static_cast<int>(style));
	item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	item->setZValue(20000.0);
	m_scene->addItem(item);

	if (!label.isEmpty() && isSceneRectVisible(path.boundingRect()))
	{
		const QRectF pathRect = path.boundingRect();
		QFont font(QStringLiteral("SimHei"));
		font.setBold(true);
		font.setPixelSize(diseaseKey == m_selectedDiseaseKey ? 18 : 13);

		QFontMetrics metrics(font);
		const QStringList lines = label.split(QLatin1Char('\n'));
		int textWidth = 0;
		for (const QString& lineText : lines)
		{
			textWidth = qMax(textWidth, metrics.width(lineText));
		}
		const int lineHeight = metrics.height();
		const int textHeight = qMax(lineHeight, lineHeight * lines.size());
		const QSizeF textSceneSize(textWidth / m_viewScaleX, textHeight / m_viewScaleY);
		QPointF labelAnchor;
		const QPointF textPos = labelScenePosForPath(pathRect, textSceneSize, labelAnchor);
		const QPointF anchor(qBound(pathRect.left(), labelAnchor.x(), pathRect.right()),
			qBound(pathRect.top(), labelAnchor.y(), pathRect.bottom()));

		QGraphicsLineItem* line = new QGraphicsLineItem(QLineF(anchor, labelAnchor), item);
		line->setPen(makePen(QColor(Qt::yellow), 1, Qt::SolidLine));
		line->setAcceptedMouseButtons(Qt::NoButton);
		line->setData(DiseaseKeyRole, diseaseKey);
		line->setData(DiseaseKindRole, QString::fromLatin1(DiseaseKind));
		line->setZValue(1.0);

		QGraphicsSimpleTextItem* text = new QGraphicsSimpleTextItem(label, item);
		text->setFont(font);
		text->setBrush(QBrush(Qt::yellow));
		text->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
		text->setAcceptedMouseButtons(Qt::NoButton);
		text->setData(DiseaseKeyRole, diseaseKey);
		text->setData(DiseaseKindRole, QString::fromLatin1(DiseaseKind));
		text->setZValue(2.0);
		text->setPos(textPos);
	}

	m_diseaseItems.append(item);
	if (diseaseKey == m_selectedDiseaseKey)
	{
		setSelectedDiseaseKey(m_selectedDiseaseKey);
	}
}

void hnSdkDiseaseGraphicsLayer::setSelectedDiseaseKey(const QString& diseaseKey)
{
	m_selectedDiseaseKey = diseaseKey;
	for (QGraphicsItem* rawItem : qAsConst(m_diseaseItems))
	{
		QGraphicsPathItem* item = dynamic_cast<QGraphicsPathItem*>(rawItem);
		if (!item)
		{
			continue;
		}

		const QColor baseColor = item->data(DiseaseColorRole).value<QColor>();
		const int baseWidth = qMax(1, item->data(DiseaseWidthRole).toInt());
		const Qt::PenStyle baseStyle = static_cast<Qt::PenStyle>(item->data(DiseaseStyleRole).toInt());
		const bool selected = item->data(DiseaseKeyRole).toString() == diseaseKey;
		// Selected state changes color only; keep original width and pen style.
		QPen pen = makePen(selected ? m_selectedDiseaseColor : baseColor,
			baseWidth,
			baseStyle);
		item->setPen(pen);
		item->setZValue(selected ? 30000.0 : 20000.0);
		item->update();
	}
}

QString hnSdkDiseaseGraphicsLayer::diseaseKeyAt(const QPointF& scenePos) const
{
	if (!m_scene)
	{
		return QString();
	}

	const QList<QGraphicsItem*> items = m_scene->items(scenePos);
	for (QGraphicsItem* item : items)
	{
		QGraphicsItem* root = diseaseRootItem(item);
		if (root)
		{
			return root->data(DiseaseKeyRole).toString();
		}
	}
	return QString();
}

bool hnSdkDiseaseGraphicsLayer::diseaseSceneRect(const QString& diseaseKey, QRectF& rect) const
{
	bool found = false;
	QRectF unitedRect;
	for (QGraphicsItem* item : qAsConst(m_diseaseItems))
	{
		if (item && item->data(DiseaseKeyRole).toString() == diseaseKey)
		{
			const QRectF itemRect = item->sceneBoundingRect();
			unitedRect = found ? unitedRect.united(itemRect) : itemRect;
			found = true;
		}
	}

	if (!found)
	{
		return false;
	}
	rect = unitedRect;
	return rect.isValid() && !rect.isEmpty();
}

bool hnSdkDiseaseGraphicsLayer::isSceneRectVisible(const QRectF& sceneRect) const
{
	if (!sceneRect.isValid() || sceneRect.isEmpty())
	{
		return false;
	}
	if (!m_hasLabelPlacementContext)
	{
		return true;
	}
	return m_visibleSceneRect.intersects(sceneRect);
}

QPointF hnSdkDiseaseGraphicsLayer::labelScenePosForPath(const QRectF& pathRect, const QSizeF& textSceneSize, QPointF& labelAnchor) const
{
	QRectF visibleRect = m_hasLabelPlacementContext ? m_visibleSceneRect : QRectF();
	if ((!visibleRect.isValid() || visibleRect.isEmpty()) && m_scene)
	{
		visibleRect = m_scene->sceneRect();
	}
	if (!visibleRect.isValid() || visibleRect.isEmpty())
	{
		visibleRect = pathRect.adjusted(-100000.0, -100000.0, 100000.0, 100000.0);
	}

	const qreal gapX = m_labelGap / m_viewScaleX;
	const qreal gapY = m_labelGap / m_viewScaleY;
	qreal x = pathRect.right() + gapX;
	qreal y = pathRect.top() - textSceneSize.height() / 2.0;

	if (x + textSceneSize.width() > visibleRect.right() - gapX)
	{
		x = pathRect.left() - gapX - textSceneSize.width();
	}
	if (x < visibleRect.left() + gapX)
	{
		x = pathRect.left();
		y = pathRect.bottom() + gapY;
	}

	x = qBound(visibleRect.left() + gapX, x, visibleRect.right() - textSceneSize.width() - gapX);
	y = qBound(visibleRect.top() + gapY, y, visibleRect.bottom() - textSceneSize.height() - gapY);

	const QRectF textRect(QPointF(x, y), textSceneSize);
	if (textRect.left() >= pathRect.right())
	{
		labelAnchor = QPointF(textRect.left(), textRect.center().y());
	}
	else if (textRect.right() <= pathRect.left())
	{
		labelAnchor = QPointF(textRect.right(), textRect.center().y());
	}
	else
	{
		labelAnchor = QPointF(textRect.center().x(), textRect.top());
	}
	return QPointF(x, y);
}

void hnSdkDiseaseGraphicsLayer::clearItems(QList<QGraphicsItem*>& items)
{
	if (!m_scene)
	{
		qDeleteAll(items);
		items.clear();
		return;
	}

	for (QGraphicsItem* item : qAsConst(items))
	{
		if (!item)
		{
			continue;
		}

		m_scene->removeItem(item);
		delete item;
	}
	items.clear();
}

QPen hnSdkDiseaseGraphicsLayer::makePen(const QColor& color, int width, Qt::PenStyle style) const
{
	QPen pen(color, qMax(1, width), style);
	pen.setCosmetic(true);
	pen.setJoinStyle(Qt::RoundJoin);
	pen.setCapStyle(Qt::RoundCap);
	return pen;
}

QGraphicsItem* hnSdkDiseaseGraphicsLayer::diseaseRootItem(QGraphicsItem* item) const
{
	while (item)
	{
		if (item->data(DiseaseKindRole).toString() == QString::fromLatin1(DiseaseKind))
		{
			QGraphicsItem* root = item;
			while (root->parentItem() &&
				root->parentItem()->data(DiseaseKindRole).toString() == QString::fromLatin1(DiseaseKind))
			{
				root = root->parentItem();
			}
			return root;
		}
		item = item->parentItem();
	}
	return nullptr;
}

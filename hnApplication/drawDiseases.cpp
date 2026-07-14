#include "drawDiseases.h"  
#include "hnDataManager.h"
#include <QFontMetrics>
#include <QStringList>
#include <qmath.h>
#include <QLineF>
drawDiseases::drawDiseases()
{
	m_encoderMile = -1;
}


drawDiseases::~drawDiseases()
{
}

double drawDiseases::getEncoderMile()
{
	return m_encoderMile;
}

void drawDiseases::drawRectsOnImage(QImage & image, const QVector<QRect>&rects , int boarderWidth, const QColor &rectColor, Qt::PenStyle style)
{
	QPainter painter(&image);
	  
	QPen pen;
	pen.setWidth(boarderWidth);
	pen.setColor(rectColor);
	pen.setStyle(style);
	painter.setPen(pen);
	
	painter.drawRects(rects);
	painter.save();


	QPen boundingPen;
	boundingPen.setStyle(style);
	boundingPen.setWidth(boarderWidth);
	boundingPen.setColor(Qt::blue);

	//QVector<qreal> dashes = { 15.0,8.0 };
	//boundingPen.setDashPattern(dashes);

	painter.setPen(boundingPen);

	if (!rects.isEmpty())
	{
		QRect boundingRect = rects.first();
		for (const QRect& rect : rects)
		{
			boundingRect = boundingRect.united(rect);
		}
		painter.drawRect(boundingRect);
	}
	painter.restore();



}


void drawDiseases::drawRectOnImageByStyle(
	QImage &image,
	const QRect &rect,
	int borderWidth,
	const QColor &rectColor,
	Qt::PenStyle style)
{
	if (rect.isNull())
	{
		return;
	}

	QPainter painter(&image);
	QPen pen;
	pen.setWidth(borderWidth);
	pen.setColor(rectColor);
	pen.setStyle(style);
	painter.setPen(pen);
	painter.drawRect(rect.normalized());
}



QRect drawDiseases::unitedRectOfPoints(const QVector<QPoint> &points) const
{
	if (points.isEmpty())
	{
		return QRect();
	}

	QRect result(points.first(), QSize(1, 1));
	for (const QPoint &point : points)
	{
		result = result.united(QRect(point, QSize(1, 1)));
	}

	return result.adjusted(-5, -5, 5, 5);
}

 

QString drawDiseases::buildDiseaseMileLabel(const hnRoadDiseaseInfo &disease) const
{
	int mile = qRound(hnApp::hnDataManager::getDataManager()
		->getCurrentProject()
		->enclToTrueMile(disease.dMileage));

	int qian = mile / 1000;
	int bai = mile - qian * 1000;

	return QStringLiteral("桩号：")
		+ "K"
		+ QString::number(qian)
		+ "+"
		+ QString::number(bai).rightJustified(3, '0');
}

QString drawDiseases::buildBigFrameDiseaseDetailLabel(
	const hnRoadDiseaseInfo &disease,
	bool includeDepth) const
{
	QString text = buildShortDiseaseLabel(disease)
		+ "\n" + buildDiseaseMileLabel(disease)
		+ "\n" + QStringLiteral("计算长度：") + QString::number(disease.dRealLen, 'f', 2)
		+ "\n" + QStringLiteral("计算宽度：") + QString::number(disease.dReaWidth, 'f', 2)
		+ "\n" + QStringLiteral("计算面积：") + QString::number(disease.dArea, 'f', 2);

	if (includeDepth && disease.dDepth != 0)
	{
		text += "\n" + QStringLiteral("深度：") + QString::number(disease.dDepth, 'f', 2);
	}

	const QString mark = QString::fromLocal8Bit(disease.strRemark);
	if (!mark.isEmpty())
	{
		text += "\n" + QStringLiteral("病害备注：") + mark;
	}

	return text;
}

QString drawDiseases::buildLittleFrameDiseaseDetailLabel(
	const hnRoadDiseaseInfo &disease,
	bool includeDepth) const
{
	const double displayLength = disease.dRealLen > 0.0 ? disease.dRealLen : disease.dLength;
	const double displayWidth = disease.dReaWidth > 0.0 ? disease.dReaWidth : disease.dWidth;
	QString text = buildShortDiseaseLabel(disease)
		+ "\n" + buildDiseaseMileLabel(disease)
		+ "\n" + QStringLiteral("计算长度：") + QString::number(displayLength, 'f', 2)
		+ "\n" + QStringLiteral("计算宽度：") + QString::number(displayWidth, 'f', 2)
		+ "\n" + QStringLiteral("计算面积：") + QString::number(disease.dArea, 'f', 2);

	if (includeDepth && disease.dDepth != 0)
	{
		text += "\n" + QStringLiteral("深度：") + QString::number(disease.dDepth, 'f', 2);
	}

	const QString mark = QString::fromLocal8Bit(disease.strRemark);
	if (!mark.isEmpty())
	{
		text += "\n" + QStringLiteral("病害备注：") + mark;
	}

	return text;
}
QString drawDiseases::buildFrameDiseaseLabel(
	const hnRoadDiseaseInfo &disease,
	bool selected,
	bool bigFrame,
	bool includeDepth) const
{
	if (selected && m_diseaseDrawStyle.showSelectedDiseaseDetail)
	{
		if (bigFrame)
		{
			return buildBigFrameDiseaseDetailLabel(disease, includeDepth);
		}

		return buildLittleFrameDiseaseDetailLabel(disease, includeDepth);
	}

	if (m_diseaseDrawStyle.showNormalDiseaseLabel)
	{
		return buildShortDiseaseLabel(disease);
	}

	return QString();
}

QString drawDiseases::buildLineDiseaseLabel(const hnRoadDiseaseInfo &disease) const
{
	return buildShortDiseaseLabel(disease)
		+ "_" + QString::number(disease.dArea, 'f', 2);
}

void drawDiseases::drawDiseaseCalloutLabel(
	QImage &image,
	const QRect &diseaseRect,
	const QString &text,
	int fontSize,
	const QColor &textColor,
	const QColor &lineColor) const
{
	if (text.isEmpty() || diseaseRect.isEmpty())
	{
		return;
	}

	QPainter painter(&image);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	painter.setWorldMatrixEnabled(false);

	QFont font = painter.font();
	font.setBold(true);
	font.setFamily("SimHei");
	font.setPixelSize(fontSize);
	painter.setFont(font);

	QFontMetrics metrics(font);
	QStringList lines = text.split('\n');

	int textWidth = 0;
	for (const QString &line : lines)
	{
		textWidth = qMax(textWidth, metrics.width(line));
	}

	const int lineHeight = metrics.height();
	const int textHeight = qMax(lineHeight, lineHeight * lines.size());

	const QRect rect = diseaseRect.normalized();
	const int gap = m_diseaseDrawStyle.calloutGap;

	int x = rect.right() + gap;
	int y = rect.top() - textHeight / 2;

	if (x + textWidth > image.width() - gap)
	{
		x = rect.left() - gap - textWidth;
	}

	if (x < gap)
	{
		x = rect.left();
		y = rect.bottom() + gap;
	}

	const int maxX = qMax(gap, image.width() - textWidth - gap);
	const int maxY = qMax(gap, image.height() - textHeight - gap);

	x = qBound(gap, x, maxX);
	y = qBound(gap, y, maxY);

	const QRect textRect(x, y, textWidth, textHeight);

	QPen linePen(lineColor);
	linePen.setWidth(m_diseaseDrawStyle.calloutLineWidth);
	painter.setPen(linePen);

	QPoint labelAnchor;
	if (textRect.left() > rect.right())
	{
		labelAnchor = QPoint(textRect.left(), textRect.center().y());
	}
	else if (textRect.right() < rect.left())
	{
		labelAnchor = QPoint(textRect.right(), textRect.center().y());
	}
	else
	{
		labelAnchor = textRect.center();
	}

	QPoint calloutStart(
		qBound(rect.left(), labelAnchor.x(), rect.right()),
		qBound(rect.top(), labelAnchor.y(), rect.bottom()));

	if (QLineF(calloutStart, labelAnchor).length() > 2.0)
	{
		painter.drawLine(calloutStart, labelAnchor);
	}

	QPen textPen(textColor);
	textPen.setWidth(1);
	painter.setPen(textPen);

	for (int i = 0; i < lines.size(); ++i)
	{
		const QPoint pos(
			textRect.left(),
			textRect.top() + metrics.ascent() + i * lineHeight);

		painter.drawText(pos, lines.at(i));
	}
}

 

void drawDiseases::drawDiseaseRectWithCallout(
	QImage &image,
	const QRect &diseaseRect,
	const QString &text,
	int borderWidth,
	const QColor &rectColor,
	Qt::PenStyle style,
	int fontSize)
{
	drawRectOnImageByStyle(image, diseaseRect, borderWidth, rectColor, style);

	drawDiseaseCalloutLabel(
		image,
		diseaseRect,
		text, 
		fontSize,
		m_diseaseDrawStyle.labelTextColor,
		m_diseaseDrawStyle.calloutLineColor);
}

QRect drawDiseases::unitedRectOfRects(const QVector<QRect>& rects) const
{
	if (rects.isEmpty())
	{
		return QRect();
	}

	QRect result = rects.first().normalized();
	for (const QRect& rect : rects)
	{
		result = result.united(rect.normalized());
	}

	return result;
}

 

QString drawDiseases::buildShortDiseaseLabel(const hnRoadDiseaseInfo& disease) const
{
	return QString::fromLocal8Bit(disease.strDisName)
		+ "_" + QString::number(disease.nID);
}

QString drawDiseases::buildLittleFrameDiseaseDetailLabel(
	const hnRoadDiseaseInfo& disease,
	const QString& mileStr,
	bool includeDepth) const
{
	QString text = buildShortDiseaseLabel(disease)
		+ "\n" + mileStr
		+ "\n" + QStringLiteral("长度：") + QString::number(disease.dLength, 'f', 2)
		+ "\n" + QStringLiteral("宽度：") + QString::number(disease.dWidth, 'f', 2)
		+ "\n" + QStringLiteral("面积：") + QString::number(disease.dArea, 'f', 2);

	if (includeDepth && disease.dDepth != 0)
	{
		text += "\n" + QStringLiteral("深度：") + QString::number(disease.dDepth, 'f', 2);
	}

	const QString mark = QString::fromLocal8Bit(disease.strRemark);
	if (!mark.isEmpty())
	{
		text += "\n" + QStringLiteral("备注：") + mark;
	}

	return text;
}







bool drawDiseases::isSeclectedMergeDisease(const hnRoadDiseaseInfo & disease)
{
	for (auto seclectedDisease : qAsConst(m_seclectedDiseases))
	{
		if (seclectedDisease.nID == disease.nID &&
			QString::fromLocal8Bit(seclectedDisease.strDiseaseTableName) == QString::fromLocal8Bit(disease.strDiseaseTableName))
		{
			return true;
		}
	 
	}

	return false;
}



void drawDiseases::claerSelectPoint()
{
    m_encoderMile = -1;
    m_seclectPoint.pixName.clear();
    m_seclectPoint.pixPoint.setX(-1);
    m_seclectPoint.pixPoint.setY(-1);
}

void drawDiseases::drawLineOnImage(const QLine &line,int lineWidth, const QColor & color, QImage & image)
{
	QPainter painter(&image);
	QPen pen;
	pen.setColor(color);
	pen.setWidth(lineWidth);
	painter.setPen(pen);
	painter.drawLine(line);
}

#include "drawDiseases.h"



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

void drawDiseases::drawRectsOnImage(QImage & image, const QVector<QRect> rects , int boarderWidth, const QColor &rectColor, Qt::PenStyle style)
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
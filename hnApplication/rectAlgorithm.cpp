#include "rectAlgorithm.h"
#include <qalgorithms.h>


rectAlgorithm::rectAlgorithm()
{
}


rectAlgorithm::~rectAlgorithm()
{
}

QVector<QRect> rectAlgorithm::mergeRects(QVector<QRect> rect1, QVector<QRect> rect2, QVector<QRect> acrossRects)
{
	//计算两个自动化模式数组的顶点坐标
	QVector<QPoint> vertectPoints1 = this->caculateVertexPoints(rect1);
	QVector<QPoint> vertectPoints2 = this->caculateVertexPoints(rect2);

	//计算坐标之间的最短线段
	QLineF minLine = this->findMinLenthLine(vertectPoints1, vertectPoints2);

	//计算线段穿过的数组
	auto newRects = this->caculateIntersectedRects(minLine, acrossRects);

	for (auto rect : qAsConst(rect1))
	{
		if (!newRects.contains(rect))
		{
			newRects.append(rect);
		}
	}

	for (auto rect : qAsConst(rect2))
	{
		if (!newRects.contains(rect))
		{
			newRects.append(rect);
		}
	}

	return newRects;
}

QPoint rectAlgorithm::findMinPoint(const QVector<QRect>& rects)
{
	QPoint dstPoint;

	int minX = 0;
	int minY = 0;

	for (const QRect &rect : qAsConst(rects))
	{
		if (rect.x() < minX || minX == 0)
		{
			minX = rect.x();
		}

		if (rect.y() < minY || minY == 0)
		{
			minY = rect.y();
		}
	}

	dstPoint.setX(minX);
	dstPoint.setY(minY);

	return dstPoint;
}

int rectAlgorithm::findMaxY(const QVector<QRect>& rects)
{
	QMap<int, int> lines;
	for (auto rect : qAsConst(rects))
	{
		lines.insert(rect.bottom(), rect.bottom());
	}

	if (!lines.isEmpty())
	{
		int maxY = lines.lastKey();
		return maxY;
	}

	return 0;

}

int rectAlgorithm::findMinY(const QVector<QRect>& rects)
{
	QMap<int,int> lines;
	for (auto rect : qAsConst(rects))
	{
		lines.insert(rect.top(),rect.top());
	}

	if (!lines.isEmpty())
	{
		int minY = lines.firstKey();
		return minY;
	}

	return 0;
}

bool rectAlgorithm::isintersectBetweenRectAndLine(const QRect & rect, const QLineF & line)
{
	QLineF leftLine = QLineF(rect.topLeft(), rect.bottomLeft());
	QLineF rightLine = QLineF(rect.topRight(), rect.bottomRight());
	QLineF topLine = QLineF(rect.topLeft(), rect.topRight());
	QLineF bottomLine = QLineF(rect.bottomLeft(), rect.bottomRight());

	QVector<QLineF> lines;
	lines.push_back(leftLine);
	lines.push_back(rightLine);
	lines.push_back(topLine);
	lines.push_back(bottomLine);

	for (const QLineF &tmpLine : qAsConst(lines))
	{
		if (line.intersect(tmpLine, nullptr) == QLineF::BoundedIntersection)
		{
			return true;
		}
	}

	return false;
}

QVector<QRect> rectAlgorithm::caculateIntersectedRects(const QPoint & startPoint, const QPoint & endPoint, const QVector<QRect>& rects)
{
	QVector<QRect> dstRects;

	//开始点所在的矩形
	for (const QRect &rect : qAsConst(rects))
	{
		if (rect.contains(startPoint))
		{
			dstRects.push_back(rect);
			break;
		}
	}

	//结束点所在的矩形
	for (const QRect &rect : qAsConst(rects))
	{
		if (rect.contains(endPoint))
		{
			dstRects.push_back(rect);
			break;
		}
	}

	//找两点之间的直线穿过的矩形
	QLineF line(startPoint, endPoint);
	for (const QRect& rect : qAsConst(rects))
	{
		if (this->isintersectBetweenRectAndLine(rect, line))
		{
			dstRects.push_back(rect);
		}
	}

	return dstRects;
}

QVector<QRect> rectAlgorithm::caculateIntersectedRects(const QLineF & line, const QVector<QRect>& rects)
{
	QVector<QRect> dstRects;

	for (const QRect& rect : qAsConst(rects))
	{
		if (this->isintersectBetweenRectAndLine(rect, line))
		{
			dstRects.push_back(rect);
		}
	}

	return dstRects;
}

QVector<QRect> rectAlgorithm::littleRects(const QPoint & point, QVector<QRect> rects)
{
	//遍历大张图片上的所有自动化模式矩形，找到点所在的矩形
	QVector<QRect> resultRects;

	for (const QRect rect : qAsConst(rects))
	{
		if (rect.contains(point) && !resultRects.contains(rect))
		{
			resultRects.append(rect);
		}
	}

	return resultRects;
}

int rectAlgorithm::caculateLittleFrameSideLenth(const double imageWidthScale, const double rectWidth)
{
	int sideLenth = rectWidth / imageWidthScale;

	return sideLenth;
}

void rectAlgorithm::caculateVertexPoints(const QVector<QRect> rects, QPoint & topLeft, QPoint & topRight, QPoint & bottomLeft, QPoint & bottomRight)
{
	if (rects.size() == 0)
	{
		return;
	}

	//最上面的中心点y
	int topY = rects.at(0).center().y();
	//最下面的中心点y
	int bottomY = rects.at(0).center().y();
	//遍历数组，找到上下两个极值
	for (auto rect : rects)
	{
		int y = rect.center().y();
		if (y < topY)
		{
			topY = y;
		}
		if (y > bottomY)
		{
			bottomY = y;
		}
	}

	//最上面中心点x数组
	QVector<int> topXVector;

	//最下面的中心点x数组
	QVector<int> bottomXVector;

	for (auto rect : rects)
	{
		int y = rect.center().y();
		int x = rect.center().x();
		if (y == topY)
		{
			topXVector.append(x);
		}
		if (y == bottomY)
		{
			bottomY = y;
			bottomXVector.append(x);
		}
	}
	
	qSort(topXVector);
	qSort(bottomXVector);

	topLeft.setX(topXVector.at(0));
	topLeft.setY(topY);

	topRight.setX(topXVector.at(topXVector.size() - 1));
	topRight.setY(topY);

	bottomLeft.setX(bottomXVector.at(0));
	bottomLeft.setY(bottomY);

	bottomRight.setX(bottomXVector.at(bottomXVector.size() - 1));
	bottomRight.setY(bottomY);
}

QVector<QPoint> rectAlgorithm::caculateVertexPoints(const QVector<QRect> rects)
{
	QVector<QPoint> resultPoints;

	if (rects.size() == 0)
	{
		return resultPoints;
	}

	//最上面的中心点y
	int topY = rects.at(0).center().y();
	//最下面的中心点y
	int bottomY = rects.at(0).center().y();
	//遍历数组，找到上下两个极值
	for (auto rect : rects)
	{
		int y = rect.center().y();
		if (y < topY)
		{
			topY = y;
		}
		if (y > bottomY)
		{
			bottomY = y;
		}
	}

	//最上面中心点x数组
	QVector<int> topXVector;

	//最下面的中心点x数组
	QVector<int> bottomXVector;

	for (auto rect : rects)
	{
		int y = rect.center().y();
		int x = rect.center().x();
		if (y == topY)
		{
			topXVector.append(x);
		}
		if (y == bottomY)
		{
			bottomY = y;
			bottomXVector.append(x);
		}
	}

	qSort(topXVector);
	qSort(bottomXVector);

	QPoint topLeft, topRight, bottomLeft, bottomRight;

	topLeft.setX(topXVector.at(0));
	topLeft.setY(topY);

	topRight.setX(topXVector.at(topXVector.size() - 1));
	topRight.setY(topY);

	bottomLeft.setX(bottomXVector.at(0));
	bottomLeft.setY(bottomY);

	bottomRight.setX(bottomXVector.at(bottomXVector.size() - 1));
	bottomRight.setY(bottomY);

	resultPoints << topLeft << topRight << bottomLeft << bottomRight;

	return resultPoints;
}

QLineF rectAlgorithm::findMinLenthLine(QVector<QPoint> points1, QVector<QPoint> points2)
{
	QLineF resultLine;
	const int pointSize = 4;

	if (pointSize != points1.size() || pointSize != points2.size())
	{
		return resultLine;
	}

	QMap<double, QLineF> lineMap;

	for (auto point1 : qAsConst(points1))
	{
		for (auto point2 : qAsConst(points2))
		{
			QLineF line(point1, point2);
			lineMap.insert(line.length(), line);
		}
	}

	if (lineMap.isEmpty())
	{
		return resultLine;
	}
	else
	{
		return lineMap.first();
	}

	return QLineF();
}

QRect rectAlgorithm::mergeRects(QVector<QRect> rects)
{
	QRect destRect;

	if (rects.empty())
	{
		return destRect;
	}

	if (1 == rects.size())
	{
		return rects.at(0);
	}

	for (auto rect : qAsConst(rects))
	{
		destRect = destRect.united(rect);
	}

	return destRect;

}



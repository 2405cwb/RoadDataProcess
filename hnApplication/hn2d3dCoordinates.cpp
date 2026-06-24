#include "hn2d3dCoordinates.h"

bool hn2d3dCoordinates::isIntersectBetweenRectAndLine(const QRect & rect, const QLineF & line)
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

int hn2d3dCoordinates::single2dXToSingle3dX(const int single2dX)
{
	int  single3dx = -1;

	//如果没有打开工程，就返回传进来的值
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return single2dX;
	}

	const int image2dWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelX;
	const int image3dWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelWidth();
	const double road2dWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRoadWidth;
	const double road3dWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getRoadWidth();
	const double widthScale3d = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageWidthScale();

	//计算截完之后的3d图片宽度
	int new3dImageWidth = image3dWidth * (road2dWidth / road3dWidth);
	//把单张2d的x先映射到截完了之后3d图片宽度上
	single3dx = single2dX * (new3dImageWidth * 1.0 / image2dWidth);

	//把计算完的3维点 要加上左边边上的宽度
	int leftPixelWidth = ((road3dWidth - road2dWidth) / 2) / widthScale3d;

	single3dx += leftPixelWidth;

	return single3dx;
}

int hn2d3dCoordinates::single3dXToSingle2dX(const int single3dx)
{
	int  single2dx = -1;

	//如果没有打开工程，就返回传进来的值
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return single3dx;
	}

	const int image2dWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelX;
	const int image3dWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelWidth();
	const double road2dWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRoadWidth;
	const double road3dWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getRoadWidth();
	const double widthScale3d = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageWidthScale();

	//先把3d图裁切成2d图的宽度
	int newImage3dWidth = image3dWidth * (road2dWidth / road3dWidth);

	//裁切完之后，3d图的x要减去上，左边的宽度
	int leftPixelWidth = ((road3dWidth - road2dWidth) / 2) / widthScale3d;
	int newSingle3dx = single3dx - leftPixelWidth;

	//映射到 2维
	single2dx = newSingle3dx * (image2dWidth * 1.0 / newImage3dWidth);

	return single2dx;
}

hn2d3dCoordinates::hn2d3dCoordinates(QObject *parent)
	: QObject(parent)
{
}

hn2d3dCoordinates::~hn2d3dCoordinates()
{
}

//判断某个折线经过了哪些给定的矩形
QVector<QRect> hn2d3dCoordinates::crossOver(const QVector<QPoint> &points, const QVector<QRect> &rects)
{
	QVector<QRect> dstRects;

	for (int i = 0; i < points.size() - 1; i++)
	{
		QPoint p1 = points[i];
		QPoint p2 = points[i + 1];
		QLineF line(p1, p2);
		for (const QRect &rect : qAsConst(rects))
		{
			if (this->isIntersectBetweenRectAndLine(rect,line) && !dstRects.contains(rect))
			{
				dstRects.append(rect);
			}
		}
	}

	return dstRects;
}

//判断某个折线经过了哪些给定的矩形
QVector<QRect> hn2d3dCoordinates::crossLineOver(const QLineF &line, const QVector<QRect> &rects)
{
	QVector<QRect> dstRects;
	for (const QRect &rect : qAsConst(rects))
	{
		if (this->isIntersectBetweenRectAndLine(rect, line) && !dstRects.contains(rect))
		{
			dstRects.append(rect);
		}
	}

	return dstRects;
}

//判断某个折线经过了哪些给定的矩形
QVector<QRect> hn2d3dCoordinates::crossRectOver(const QRect& rect, const QVector<QRect> &rects)
{
	QVector<QRect> dstRects;
	dstRects.reserve(qMin(rects.size(), 4096));
	const QRect normalizedRect = rect.normalized();

	for (const QRect &smallRect : qAsConst(rects))
	{
		if (normalizedRect.contains(smallRect))
		{
			dstRects.append(smallRect);
		}
	}
	/*for (const QRect &smallRect :rects)
   {
		QPoint center = smallRect.center();
		bool inside = rect.contains(center);
		if (inside )
		{
			dstRects.append(smallRect);

		}*/ 
  // }

	return dstRects;
}



std::vector<hn3dRectI> hn2d3dCoordinates::get3dSingleImageLittleFrames(const double mile)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return std::vector<hn3dRectI>();
	}
	//这个接口就是为了自动化模式模式二维视图映射三维视图而设计的。如果二三维其中有一个工程不打开，则返回空。
	if (!hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2DProject() ||
		!hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		return std::vector<hn3dRectI>();
	}

	//获取二维路面宽度
	double roadWidth2d = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRoadWidth;
	//获取三维路面宽度
	double roadWidth3d = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getRoadWidth();


	std::vector<hn3dRectI> dstRects;

	//算出每个像素代表多少米
	const double widthScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageWidthScale();
	const double heightScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageHeightScale();
	//矩形宽度（米） 每个小矩形宽度是10cm 也就是0.1米
	const double rectWidth = 0.1;

	//算出自动化模式边长 单位：像素
	int widthSideLenth = this->caculateLittleFrameSideLenth(widthScale, rectWidth);
	int heightSideLenth = this->caculateLittleFrameSideLenth(heightScale, rectWidth);

	//三维图片像素宽度
	int image3dPixelWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelWidth();

	//三维图片像素高度
	int image3dPixelHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelHeight();

	//二维在三维视图上映射的宽度
	int image2dMapPixelWidth = image3dPixelWidth *(roadWidth2d / roadWidth3d);

	//横向矩形的个数
	int widthRectCount = image2dMapPixelWidth / widthSideLenth;

	//纵向矩形的个数
	int heightRectCount = image3dPixelHeight / heightSideLenth;

	//在三维视图上截取二维，左边边距的像素值
	int leftSidePixel = image3dPixelWidth*(((roadWidth3d - roadWidth2d) / 2) / roadWidth3d);

	//循环向目标的数据里面添加矩形
	hn3dRectI rect3d;
	QRect rect;					//单个自动化模式的矩形
	QPoint topLeftPoint;		//左上角的点
	QPoint bottomRightPoint;	//右下角的点
	for (int i = 0; i < heightRectCount; i++)
	{
		for (int j = 0; j < widthRectCount; j++)
		{
			//计算左上角的点
			topLeftPoint = QPoint(j * widthSideLenth + leftSidePixel, i * heightSideLenth);
			//计算右下角的点
			bottomRightPoint = QPoint(j * widthSideLenth + widthSideLenth + leftSidePixel, i * heightSideLenth + heightSideLenth);
			//得到矩形
			rect = QRect(topLeftPoint, bottomRightPoint);
			//插入数组
			//dstRects.push_back(rect);
			rect3d.p0.x = rect.topLeft().x();
			rect3d.p0.y = rect.topLeft().y();
			rect3d.p1.x = rect.topRight().x();
			rect3d.p1.y = rect.topRight().y();
			rect3d.p2.x = rect.bottomRight().x();
			rect3d.p2.y = rect.bottomRight().y();
			rect3d.p3.x = rect.bottomLeft().x();
			rect3d.p3.y = rect.bottomLeft().y();

			rect3d.p0.bottomEncoderMile = mile;
			rect3d.p1.bottomEncoderMile = mile;
			rect3d.p2.bottomEncoderMile = mile;
			rect3d.p3.bottomEncoderMile = mile;

			dstRects.push_back(rect3d);
		}
	}

	return dstRects;
}

std::vector<hn3dRectI> hn2d3dCoordinates::get3dLittleRects(hn3dPointWithMileI point)
{
	std::vector<hn3dRectI> allRects = this->get3dSingleImageLittleFrames(point.bottomEncoderMile);
	std::vector<hn3dRectI> dstRects;

	for (hn3dRectI &hnRect : allRects)
	{
		if (isContains3dPoint(hnRect, point))
		{
			dstRects.push_back(hnRect);
		}
	}
	return dstRects;
}

bool hn2d3dCoordinates::isContains3dPoint(hn3dRectI rect, hn3dPointWithMileI point)
{
	QPoint qpoint;
	qpoint.setX(point.x);
	qpoint.setY(point.y);

	QRect qrect;
	qrect.setTopLeft(QPoint(rect.p0.x, rect.p0.y));
	qrect.setTopRight(QPoint(rect.p1.x, rect.p1.y));
	qrect.setBottomRight(QPoint(rect.p2.x, rect.p2.y));
	qrect.setBottomLeft(QPoint(rect.p3.x, rect.p3.y));

	return qrect.contains(qpoint);
}

std::vector<hn2dRectI> hn2d3dCoordinates::get2dSingleImageLittleFrames(const double dmi)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return std::vector<hn2dRectI>();
	}

	std::vector<hn2dRectI> dstRects;

	//算出每个像素代表多少米
	const double widthScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioX;
	const double heightScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;
	//矩形宽度（米） 每个小矩形宽度是10cm 也就是0.1米
	const double rectWidth = 0.1;

	//算出自动化模式边长 单位：像素
	int widthSideLenth = this->caculateLittleFrameSideLenth(widthScale, rectWidth);
	int heightSideLenth = this->caculateLittleFrameSideLenth(heightScale, rectWidth);

	//图片像素宽度
	int imagePixelWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelX;

	//图片像素高度
	int imagePixelHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;

	//横向矩形的个数
	int widthRectCount = imagePixelWidth / widthSideLenth;

	//纵向矩形的个数
	int heightRectCount = imagePixelHeight / heightSideLenth;

	//循环向目标的数据里面添加矩形
	hn2dRectI rect2d;
	QRect rect;					//单个自动化模式的矩形
	QPoint topLeftPoint;		//左上角的点
	QPoint bottomRightPoint;	//右下角的点
	for (int i = 0; i < heightRectCount; i++)
	{
		for (int j = 0; j < widthRectCount; j++)
		{
			//计算左上角的点
			topLeftPoint = QPoint(j * widthSideLenth, i * heightSideLenth);
			//计算右下角的点
			bottomRightPoint = QPoint(j * widthSideLenth + widthSideLenth, i * heightSideLenth + heightSideLenth);
			//得到矩形
			rect = QRect(topLeftPoint, bottomRightPoint);
			//插入数组
			rect2d.p0.x = rect.topLeft().x();
			rect2d.p0.y = rect.topLeft().y();
			rect2d.p1.x = rect.topRight().x();
			rect2d.p1.y = rect.topRight().y();
			rect2d.p2.x = rect.bottomRight().x();
			rect2d.p2.y = rect.bottomRight().y();
			rect2d.p3.x = rect.bottomLeft().x();
			rect2d.p3.y = rect.bottomLeft().y();

			rect2d.p0.m_dmi =dmi;
			rect2d.p1.m_dmi =dmi;
			rect2d.p2.m_dmi =dmi;
			rect2d.p3.m_dmi =dmi;

			dstRects.push_back(rect2d);
		}
	}

	return dstRects;
}

std::vector<hn2dRectI> hn2d3dCoordinates::get2dLittleRects(hn2dPointWithMileI point)
{
	std::vector<hn2dRectI> allRects = this->get2dSingleImageLittleFrames(point.m_dmi);
	std::vector<hn2dRectI> dstRects;

	for (hn2dRectI &hnRect : allRects)
	{
		if (isContains2dPoint(hnRect, point))
		{
			dstRects.push_back(hnRect);
		}
	}
	return dstRects;
}

bool hn2d3dCoordinates::isContains2dPoint(hn2dRectI rect, hn2dPointWithMileI point)
{
	QPoint qpoint;
	qpoint.setX(point.x);
	qpoint.setY(point.y);

	QRect qrect;
	qrect.setTopLeft(QPoint(rect.p0.x, rect.p0.y));
	qrect.setTopRight(QPoint(rect.p1.x, rect.p1.y));
	qrect.setBottomRight(QPoint(rect.p2.x, rect.p2.y));
	qrect.setBottomLeft(QPoint(rect.p3.x, rect.p3.y));

	return qrect.contains(qpoint);
}

int hn2d3dCoordinates::caculateLittleFrameSideLenth(const double imageWidthScale, const double rectWidth)
{
	int sideLenth = rectWidth / imageWidthScale;

	return sideLenth;
}


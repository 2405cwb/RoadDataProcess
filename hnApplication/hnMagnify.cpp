#include "hnMagnify.h"

hnMagnify::hnMagnify()
{
	//设置是否放大图片
	this->m_isMagnifyPix = false;

	//设置图片大小
	this->m_pixLenOfSide = 400;

	//设置偏置距离
	this->m_offsetDistance = 0;

	//设置放大倍数
	this->m_magnification = 2;
}


bool hnMagnify::getIsMagnification()
{
	return m_isMagnifyPix;
}

void hnMagnify::setIsMagnification(const bool isMagnify)
{
	m_isMagnifyPix = isMagnify;
}

int hnMagnify::getMagnification()
{
	return m_magnification;
}

void hnMagnify::setMagnification(int magnification)
{
	m_magnification = magnification;
}

int hnMagnify::getOffsetDistance()
{
	return m_offsetDistance;
}

void hnMagnify::setOffsetDistance(int distance)
{
	m_offsetDistance = distance;
}

int hnMagnify::getPixLenOfSide()
{
	return m_pixLenOfSide;
}

void hnMagnify::setPixLenOfSide(int side)
{
	m_pixLenOfSide = side;
}


QImage hnMagnify::drawMagnifyPixRectangle(const QPoint &point, const QImage &imageWithoutDisease,
	const QImage &imageWithDisease)
{
	//如果没有放大 ，就不做处理
	if (m_isMagnifyPix == false)
	{
		return imageWithDisease;
	}
	//获取鼠标位置
	QPoint mousePosPoint = point;

	//正方形边长转换为image中的边长
	int sideLen = this->m_pixLenOfSide;

	//左上角坐标
	QPoint leftTopPoint;
	leftTopPoint.setX(mousePosPoint.x() - sideLen * 1.0 / 2);
	leftTopPoint.setY(mousePosPoint.y() - sideLen * 1.0 / 2);

	//右下角坐标
	QPoint rightButtonPoint;
	rightButtonPoint.setX(mousePosPoint.x() + sideLen * 1.0 / 2 - 1);
	rightButtonPoint.setY(mousePosPoint.y() + sideLen * 1.0 / 2 - 1);

	//放大之后的左上角和右下角坐标
	QPoint newMousePosPoint = mousePosPoint;
	QPoint multiLeftTopPoint, multiRightButtonPoint;
	//计算放大后左上角的坐标
	multiLeftTopPoint.setX(newMousePosPoint.x() - this->m_pixLenOfSide * m_magnification * 1.0 / 2);
	multiLeftTopPoint.setY(newMousePosPoint.y() - this->m_pixLenOfSide * m_magnification * 1.0 / 2);
	//计算放大后右下角的坐标
	multiRightButtonPoint.setX(newMousePosPoint.x() + this->m_pixLenOfSide * m_magnification * 1.0 / 2);
	multiRightButtonPoint.setY(newMousePosPoint.y() + this->m_pixLenOfSide * m_magnification * 1.0 / 2);
	//算完后，会多取一个点 ，就是newMousePosPoint这个点  我们把右下角的坐标x和y都减去1
	multiRightButtonPoint.setX(multiRightButtonPoint.x() - 1);
	multiRightButtonPoint.setY(multiRightButtonPoint.y() - 1);

	//从label中把image提取出来
	QImage image = imageWithoutDisease;

	//先截图，再放大
	QImage copyImage = image.copy(QRect(leftTopPoint, rightButtonPoint));

	//image放大
	QImage multiImage = copyImage.scaled(QSize(copyImage.width() *m_magnification, copyImage.height() * m_magnification));

	if (multiImage.isNull())
	{
		return imageWithDisease;
	}

	//截图
	QImage newImage = multiImage;

	//截图完了画十字线上去
	QPoint centerPoint = newImage.rect().center();	//算正方形的中心点

	QLine hLine; //横线
	hLine.setP1(QPoint(centerPoint.x(), centerPoint.y() - 50));
	hLine.setP2(QPoint(centerPoint.x(), centerPoint.y() + 50));

	QLine wLine; //纵线
	wLine.setP1(QPoint(centerPoint.x() - 50, centerPoint.y()));
	wLine.setP2(QPoint(centerPoint.x() + 50, centerPoint.y()));

	//往小方块内画线
	QPainter newImagePainter(&newImage);
	QPen pen;
	pen.setWidth(20);		//设置画笔宽度
	pen.setColor(Qt::blue);	//设置画笔颜色
	newImagePainter.setPen(pen);
	newImagePainter.drawLine(hLine);
	newImagePainter.drawLine(wLine);

	//偏移距离
	const int offset = m_pixLenOfSide * 1.0 * m_magnification / 2 + m_offsetDistance;

	//算新的坐标 正方形移动后的坐标  左上角
	QPoint newLeftTopPoint;
	newLeftTopPoint.setX(multiLeftTopPoint.x() - offset);
	newLeftTopPoint.setY(multiLeftTopPoint.y() + offset);
	//算新的坐标 正方形移动后的坐标  右下角
	QPoint newRightButtonPoint;
	newRightButtonPoint.setX(multiRightButtonPoint.x() - offset);
	newRightButtonPoint.setY(multiRightButtonPoint.y() + offset);

	QRect newRect(newLeftTopPoint, newRightButtonPoint);
	//如果鼠标点的x < 新矩形的边长（这里按宽表示），把矩形的“左边”移动到当前鼠标x的位置
	if (mousePosPoint.x() < newRect.width())
	{
		newRect.moveLeft(mousePosPoint.x());
	}
	//如果鼠标点距离最下方的位置 小于 新矩形的边长（这里按宽表示），把矩形的“底边”移动到鼠标y的位置
	if ((imageWithDisease.height() - mousePosPoint.y()) < newRect.width())
	{
		newRect.moveBottom(mousePosPoint.y());
	}

	//算完画上去，往带病害的临时image上面画
	QImage labelImage = imageWithDisease;
	QPainter painter(&labelImage);
	painter.drawPixmap(newRect, QPixmap::fromImage(newImage));
	
	//返回画好的Image
	return labelImage;
}

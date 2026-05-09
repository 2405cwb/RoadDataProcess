#pragma once
#include <QImage>
#include <QPainter>
#include "hnapplication_global.h"

//放大镜
class HNAPPLICATION_EXPORT hnMagnify
{
public:
	hnMagnify();

public:
	//获取是否放大
	bool getIsMagnification();

	//设置是否放大
	void setIsMagnification(const bool isMagnify);

	//获取放大倍数
	int getMagnification();

	//设置放大倍数
	void setMagnification(int magnification);

	//获取鼠标偏置距离
	int getOffsetDistance();

	//设置鼠标偏置距离
	void setOffsetDistance(int distance);

	//获取放大区域边长  
	int getPixLenOfSide();

	//设置放大区域边长
	void setPixLenOfSide(int side);

protected:
	/*
	* 往图片上画放大镜矩形
	* 参数一：point [const QPoint&] 大image上的点！
	* 参数二：imageWithoutDisease[const QImage &] 不带病害的，干净的image，在这个上面取放大镜矩形内容
	* 参数三：imageWithDisease[const QImage &]带病害的image，在这个上面画放大镜的矩形框
	* 返回值：画好的带有放大镜矩形的图片
	*/

	QImage drawMagnifyPixRectangle(const QPoint &point,const QImage &imageWithoutDisease,
		const QImage &imageWithDisease);

protected:
	//是否放大图片
	bool m_isMagnifyPix;

	//放大倍数
	int m_magnification;

	//鼠标偏置距离
	int m_offsetDistance;

	//放大镜视图下的图片大小 （正方形边长）
	int m_pixLenOfSide;

	//当前鼠标位置 针对大image的
	QPoint m_magnifyBigImagePos;
};

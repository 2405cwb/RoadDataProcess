#pragma once

#include "hnapplication_global.h"
#include <QPoint>
#include <QRect>
#include <QtGlobal>
#include <qglobal.h>
#include <QVector>
#include <QLineF>
#include <QMap>

class HNAPPLICATION_EXPORT rectAlgorithm
{
public:
	rectAlgorithm();
	~rectAlgorithm();
	//自动化模式合并病害用
protected:

	//给定两个自动化模式的矩形数组，根据算法合并两个自动化模式数组，成为一个新的数组
	QVector<QRect> mergeRects(QVector<QRect> rect1, QVector<QRect> rect2 ,QVector<QRect> acrossRects);

public:
	//找出矩形数组中最小的x和最小y的点
	QPoint findMinPoint(const QVector<QRect> &rects);

	//找出很多矩形中找出最下面的线的y坐标
	int findMaxY(const QVector<QRect> &rects);

	//找出很多矩形中找出最上面的线的y坐标
	int findMinY(const QVector<QRect> &rects);

	//判断一个矩形和一条线是否相交,直接相交，不包含延长线相交
	bool isintersectBetweenRectAndLine(const QRect &rect, const QLineF &line);

	//在一堆矩形中找到两点中间线穿过的矩形 这里的点都是大张图的点
	//包括两个点所在的矩形
	QVector<QRect> caculateIntersectedRects(const QPoint &startPoint, const QPoint &endPoint, const QVector<QRect> &rects);

	//在一堆矩形中找到两点中间线穿过的矩形 这里的点都是大张图的点
	//不包括两个点所在的矩形
	QVector<QRect> caculateIntersectedRects(const QLineF &line, const QVector<QRect> &rects);

	//判断一个点在很多矩形中，所在的自动化模式矩形  针对大image坐标系
	QVector<QRect> littleRects(const QPoint &point, QVector<QRect> rects);

	//接口含义：计算自动化模式边长像素大小
	int caculateLittleFrameSideLenth(const double imageWidthScale, const double rectWidth);

	// 计算QRect数组中 最上面和最下面四个QRect(左上、右上、左下、右下)的中心点坐标
	// 如果最下面或者最上面只有一个QRect，则上面的两个或者下面的两个相同。
	// 如果数组为空，返回值的坐标都为QPoint(0,0)
	void caculateVertexPoints(const QVector<QRect> rects, QPoint &topLeft, QPoint &topRight, QPoint &bottomLeft, QPoint &bottomRight);
	// 计算QRect数组中 最上面和最下面四个QRect(左上、右上、左下、右下)的中心点坐标的数组集合
	// 如果最下面或者最上面只有一个QRect，则上面的两个或者下面的两个相同。
	// 如果数组为空，返回值的坐标数组为空
	QVector<QPoint> caculateVertexPoints(const QVector<QRect> rects);

	/*
	* 接口名：findMinLenthLine
	* 接口含义：给定两个四个点的数组，两个数组的点两两相连，找到最小的那个线
	* 参数一：第一个数组，必须有四个点
	* 参数二：第二个数组，必须有四个点
	* 返回值：找到的线段，如果异常，返回空的线段
	*/
	QLineF findMinLenthLine(QVector<QPoint> points1, QVector<QPoint> points2);

	//计算编码器里程
	virtual double caculateEncoderMileByScreenPoint(const QPoint &screenPoint) = 0;

	//合并矩形
	QRect mergeRects(QVector<QRect> rects);
};


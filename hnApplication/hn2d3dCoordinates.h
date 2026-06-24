#pragma once

#include <QObject>
#include "../hnCommon/hnRoadStruct.h"
#include "../hnCommon/hn3dRect.h"
#include "hnDataManager.h"
#include "../hnProject/hnProject.h"
#include "../hnProject/hn3DProject.h"
#include <QVector>
#include <QPointer>
#include <QRect>
#include <QLineF>
#include "hnapplication_global.h"

using namespace hnCommon;

class HNAPPLICATION_EXPORT hn2d3dCoordinates : public QObject
{
	Q_OBJECT

public:
	//判断某个折线经过了哪些给定的矩形
	QVector<QRect> crossOver(const QVector<QPoint> &points, const QVector<QRect> &rects);

	//判断某个线段经过了哪些给定的矩形
	QVector<QRect> crossLineOver(const QLineF &line, const QVector<QRect> &rects);

	//判断大矩形内的哪些点经过了小矩形
	QVector<QRect> crossRectOver(const QRect& rect ,const QVector<QRect> &rects);

	//判断一个矩形和一条线是否相交,直接相交，不包含延长线相交
	bool isIntersectBetweenRectAndLine(const QRect &rect, const QLineF &line);

	// 单张2d图上的x转单张三维图片上的x
	int single2dXToSingle3dX(const int single2dX);

	//单张3d图的x转单张2d图的x
	int single3dXToSingle2dX(const int single3dx);

public:
	hn2d3dCoordinates(QObject *parent = nullptr);
	~hn2d3dCoordinates();

public:
	//获取3d视图的单张图片的所有自动化模式  
	std::vector<hn3dRectI> get3dSingleImageLittleFrames(const double mile);

	//获取3D某个点所在的自动化模式
	std::vector<hn3dRectI> get3dLittleRects(hn3dPointWithMileI point);

	//判断某个点在不在矩形内 3d 前提是必须是同一个mile！
	bool isContains3dPoint(hn3dRectI rect, hn3dPointWithMileI point);

public:
	//获取2d视图的单张图片的所有自动化模式  
	std::vector<hn2dRectI> get2dSingleImageLittleFrames(const double dmi);

	//获取2D某个点所在的自动化模式
	std::vector<hn2dRectI> get2dLittleRects(hn2dPointWithMileI point);

	//判断某个点在不在矩形内 2d 前提是必须是同一个mile！
	bool isContains2dPoint(hn2dRectI rect, hn2dPointWithMileI point);

private:
	/*
	*接口含义：计算自动化模式边长像素大小
	*接口名称：caculateLittleFrameSideLenth
	*参数一：imageWidthScale 图片横向每个像素代表的米数
	*参数二：自动化模式每个矩形的宽度
	*/
	int caculateLittleFrameSideLenth(const double imageWidthScale, const double rectWidth);

};

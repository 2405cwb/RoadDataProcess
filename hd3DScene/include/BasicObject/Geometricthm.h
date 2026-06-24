/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdBasicObject
文件名		：Geometricthm.h
相关文件	: base中的基本图形
文件实现功能：一些矩形多边形求交拓扑关系方法集合
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/6/10	1.0			马振明		  实现
</PRE>
******************************************************************************************************/
#pragma once
#include "hdBasicObject.h"
#include "hdPolyline2D.h"
#include "BaseRect.h"
using namespace base;

class BASICOBJECT_API CGeometricthm
{
public:
	CGeometricthm(void);
	~CGeometricthm(void);

	// 判断4个点组成的2个线段是否相交，前2个点构成一个线段，后2个点构成一个线段,并求出交点
	static bool IsLineIntersectLine(const Point2dd& pFirst1,const Point2dd& pFirst2,const Point2dd& pSecond1,const Point2dd& pSecond2, Point2dd& pCrossPoint);

	// 判断2个线段是否相交
	static bool IsIntersect(const CPolyline2Dd& pPolyline1,const CPolyline2Dd& pPolyLine2,vector<Point2dd>& vectCrossPoint);

	// 判断线段和矩形是否相交
	static bool IsIntersect(const CPolyline2Dd& pPolyline1,const CHdRectd& pRect,vector<Point2dd>& vectCrossPoint);
};


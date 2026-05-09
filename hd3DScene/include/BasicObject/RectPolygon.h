/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdBasicObject
文件名		：Polyline2D.h
相关文件	: BasicStruct.h
文件实现功能：定义一个2D点构成的矩形镶嵌构成的多边形,暂时只考虑double类型
			  该图形形如:
					 -------
					|		|
				----		 -----
				|				 |
				|----		-----|
					|		|
					 -------
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/6/11	1.0			马振明		  新建
</PRE>
******************************************************************************************************/
#pragma once
#include "BaseRect.h"
#include "polygon.h"
using namespace std;
using namespace base;
using namespace tal;

// 定义一个由矩形镶嵌而成的多边形
class BASICOBJECT_API CRectPolygon
{
public:
	CRectPolygon(void);
	~CRectPolygon(void);

	// 插入矩形,矩形点左为x最小，右为x最大，上为y最大，下为y最小
	void InsertRect(const CHdRectd& rectd);

	// 求交
	CHdRectd Intersect(const CHdRectd& rectd);

	// 判断一个点是否在内部
	bool IsInside(double dx,double dy);

private:
	// 判断拓扑关系
	CPolygon RectToPolygon(const CHdRectd& rectd);

private:
	CPolygon m_polygon;				// 多边形
};


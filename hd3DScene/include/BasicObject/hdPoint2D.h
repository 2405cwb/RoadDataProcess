/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdBasicObject
文件名		：Point2D.h
相关文件	: CBasePoint.h
文件实现功能：定义一个2D点模板类
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/4/22	1.0			马振明		  移植
</PRE>
******************************************************************************************************/

#pragma once
#include "hdBasicObject.h"

template <class T>
class  CHdPoint2D : public CHdBasicObject
{
public:
	CHdPoint2D(void);
	virtual ~CHdPoint2D(void);

public:
	POINT2D<T>		m_point;
};

typedef CHdPoint2D<double> CHdPoint2Dd;
typedef CHdPoint2D<int> CHdPoint2Di;
typedef CHdPoint2D<float> CHdPoint2Df;
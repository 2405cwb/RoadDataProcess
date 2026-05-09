/*!@file
*******************************************************************************************************
<PRE>
模块名		：HD2DPoint.h
文件名		：CHD2DPoint.h
相关文件	: CHD2DPoint.cpp	HD2DObject.h	HDBaseStruct.h
文件实现功能：二维点对象，从CHD2DObject继承。
作者		：任高强
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/1/6	1.0			任高强		创建
</PRE>
******************************************************************************************************/


#pragma once
#include "HD2DObject.h"
#include "HDBaseStruct.h"

namespace hd
{

	class HDCOMMON_API CHD2DPoint:public CHD2DObject
	{
	public:
		CHD2DPoint(void);
		CHD2DPoint(double dX, double dY);
		CHD2DPoint(const HD_2DPOINT& pt);
		virtual ~CHD2DPoint(void);
		
	public:
		double m_x;
		double m_y;

	public:
		// 赋值运算
		CHD2DPoint& operator=(const CHD2DPoint& other); 
		// 取反运算
		CHD2DPoint operator-() const;
		// 减法运算
		CHD2DPoint operator-(const CHD2DPoint& other) const;
		// 加法运算
		CHD2DPoint operator+(const CHD2DPoint& other) const;
		// 除法运算
		CHD2DPoint operator/(double dVal) const;
		// 乘法运算
		CHD2DPoint operator*(double dVal) const;

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_2DPOINT;}
		virtual CHD2DBoundingBox GetBoundingBox()  const;
		double Distace(HD_2DPOINT& pt);
		double Distace(CHD2DPoint& pt);
	};

}
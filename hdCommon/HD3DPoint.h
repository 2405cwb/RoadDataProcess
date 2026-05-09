/*!@file
*******************************************************************************************************
<PRE>
模块名		：HD3DPoint.h
文件名		：CHD3DPoint.h
相关文件	: CHD3DPoint.cpp	HD3DObject.h	HDBaseStruct.h
文件实现功能：三维点对象，从CHD3DObject继承。
作者		：孙文
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/1/6	1.0			孙文		创建
</PRE>
******************************************************************************************************/

#pragma once
#include "HD3DObject.h"
#include "HDBaseStruct.h"

namespace hd
{

	class HDCOMMON_API CHD3DPoint : public CHD3DObject
	{
	public:
		CHD3DPoint(void);
		CHD3DPoint(double dX, double dY, double dZ);
		CHD3DPoint::CHD3DPoint(double dX, double dY, double dZ,int index);
		virtual ~CHD3DPoint(void);

	public:
		double m_x;
		double m_y;
		double m_z;

		int m_index; // 点索引,为了在点云中编辑轨迹点的索引
	public:
		// 返回当前的Box
		virtual CHD3DBoundingBox GetBoundingBox() const;
		// 判断点是否在球内
		virtual bool IsInSphere(double dX, double dY, double dZ, double dR) const;
		//只通过x,y进行判断 【add by mzm 2013.10.16】
		virtual bool IsInSphere(double dX, double dY, double dR) const;
		// 返回当前类型
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_3DPOINT;}
		// 赋值运算
		CHD3DPoint& operator=(const	CHD3DPoint& other);
		// 减法运算
		CHD3DPoint operator-(const	CHD3DPoint& other) const;
		// 取反运算
		CHD3DPoint operator-() const;
		// 加法运算
		CHD3DPoint operator+(const	CHD3DPoint& other) const;
		// 乘法运算
		CHD3DPoint operator*(double dVal) const;
		// 除法运算
		CHD3DPoint operator/(double dVal) const;
	};

}
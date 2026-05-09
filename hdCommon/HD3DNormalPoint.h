/*!@file
*******************************************************************************************************
<PRE>
模块名		：HD3DNormalPoint.h
文件名		：CHD3DNormalPoint.h
相关文件	: CHD3DNormalPoint.cpp	HD3DPoint.h
文件实现功能：带法向量的三维点对象，从CHD3DPoint继承。
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
#include "HD3DPoint.h"


namespace hd
{

	class HDCOMMON_API CHD3DNormalPoint : public CHD3DPoint
	{
	public:
		CHD3DNormalPoint(void);
		CHD3DNormalPoint(double dX, double dY, double dZ, double dNX, double dNY, double dNZ);
		virtual ~CHD3DNormalPoint(void);

	public:
		// 返回当前类型
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_3DNORMALPOINT;}
		virtual CHD3DBoundingBox GetBoundingBox() const;
		// 判断是否在球内
		virtual bool IsInSphere(double dX, double dY, double dZ, double dR) const;
		// 赋值运算
		CHD3DNormalPoint& operator=(const CHD3DNormalPoint& other);

	public:
		HD_3DPOINT m_normal;	// 点的法向量
	};

}
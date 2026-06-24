/*!@file
*******************************************************************************************************
<PRE>
模块名		：HD2DBoundingBox.h
文件名		：CHD2DBoundingBox.h
相关文件	: CHD2DBoundingBox.cpp	hdCommon.h
文件实现功能：二维对象的最小外包围盒。
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
#include "hdCommon.h"

namespace hd
{

	class HDCOMMON_API CHD2DBoundingBox
	{
	public:
		CHD2DBoundingBox(void);	
		virtual ~CHD2DBoundingBox(void); 

	public:
		// 插入一个点，使当前Box扩大
		void InsertPoint(double dx, double dy);
		// 插入一个Box，使当前Box扩大
		void InsertBoundingBox(const CHD2DBoundingBox& bBox);
		// 赋值运算
		CHD2DBoundingBox& operator=(const CHD2DBoundingBox& other);
		// 加一个ResetBoundingBox函数
		void ResetBoundingBox();
		// Box扩展
		void Extend(double dExt);
		// Box扩展
		void Extend(double dXExt, double dYExt);

		// 判断是否在包围盒内
		bool IsInBox(double dx, double dy,double dError=0.0);
	public:
		double   m_dMinX;		// Box的最小X值
		double   m_dMinY;		// Box的最小Y值
		double   m_dMaxX;		// Box的最大X值
		double   m_dMaxY;		// Box的最大Y值
	};

}
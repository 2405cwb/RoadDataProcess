/*!@file
*******************************************************************************************************
<PRE>
模块名		：HD3DBoundingBox.h
文件名		：CHD3DBoundingBox.h
相关文件	: CHD3DBoundingBox.cpp	hdCommon.h
文件实现功能：三维对象的最小外包围盒。
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
#include "hdCommon.h"
#include <float.h>

namespace hd
{

	class HDCOMMON_API CHD3DBoundingBox
	{
	public:
		CHD3DBoundingBox(void);
		virtual ~CHD3DBoundingBox(void);

	public:
		// 输入一个点，来更新Box的范围
		void InsertPoint(double dX, double dY, double dZ);
		// 输入一个矩形，来更新Box的范围
		void InsertBoundingBox(CHD3DBoundingBox bBox);
		// "="运算符重载
		CHD3DBoundingBox& operator=(const CHD3DBoundingBox& other);

	public:
		inline bool IsValid() const
		{
			if ((m_dMaxX < m_dMinX) || (m_dMaxY < m_dMinY) || (m_dMaxZ < m_dMinZ))
			{
				return false;
			}

			return true;
		}

		inline void Extend(double fExt)
		{
			Extend(fExt, fExt, fExt);
		}

		inline void Extend(double xExt, double yExt, double zExt)
		{
			if (IsValid())
			{
				m_dMinX -= xExt;
				m_dMaxX += xExt;
				m_dMinY -= yExt;
				m_dMaxY += yExt;
				m_dMinZ -= zExt;
				m_dMaxZ += zExt;
			}
		}

	public:
		double m_dMinX;		// Box的X最小值
		double m_dMinY;		// Box的Y最小值
		double m_dMinZ;		// Box的Z最小值
		double m_dMaxX;		// Box的X最大值
		double m_dMaxY;		// Box的Y最大值
		double m_dMaxZ;		// Box的Z最大值
	};

}
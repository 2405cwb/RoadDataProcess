/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：CHD2DRect.h
相关文件	: CHD2DRect.cpp	HD2DObject.h	HDBaseStruct.h
文件实现功能：二维矩形对象。
作者		：姚立
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2013/06/13	1.0			任高强		创建
</PRE>
******************************************************************************************************/
#include "StdAfx.h"
#include "HD2DRect.h"

namespace hd
{
	CHD2DRect::CHD2DRect(void)
	{
	}


	CHD2DRect::~CHD2DRect(void)
	{
	}

	CHD2DBoundingBox CHD2DRect::GetBoundingBox() const
	{
		CHD2DBoundingBox box;
		box.m_dMinX = m_ptLB.m_x;
		box.m_dMinY = m_ptLB.m_y;
		box.m_dMaxX = m_ptRT.m_x;
		box.m_dMaxY = m_ptRT.m_y;

		return box;
	}

	CHD2DRect& CHD2DRect::operator=(const	CHD2DRect& other)
	{
		m_ptLB.m_x = other.m_ptLB.m_x;
		m_ptLB.m_y = other.m_ptLB.m_y;
		m_ptRT.m_x = other.m_ptRT.m_x;
		m_ptRT.m_y = other.m_ptRT.m_y;
		return *this;
	}

	// pt是否在rect中
	bool CHD2DRect::IsPointInRect(const CHD2DPoint& pt) const
	{
		if (pt.m_x > m_ptLB.m_x && pt.m_x < m_ptRT.m_x && pt.m_y > m_ptLB.m_y && pt.m_y < m_ptRT.m_y)
		{
			return true;
		}

		return false;
	}
}
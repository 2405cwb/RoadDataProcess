/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：CHDCrossLine.h
相关文件	: CHDCrossLine.cpp	HD2DObject.h	HDBaseStruct.h
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
#include "HDCrossLine.h"

namespace hd
{
	CHDCrossLine::CHDCrossLine(void)
	{
	}


	CHDCrossLine::~CHDCrossLine(void)
	{
	}

	CHD3DBoundingBox CHDCrossLine::GetBoundingBox() const
	{
		CHD3DBoundingBox box;
		box.m_dMinX = m_ptDown.m_x;
		box.m_dMaxX = m_ptDown.m_x;
		if (m_ptUp.m_x < box.m_dMinX)
		{
			box.m_dMinX = m_ptUp.m_x;
		}
		else if (m_ptUp.m_x > box.m_dMaxX)
		{
			box.m_dMaxX = m_ptUp.m_x;
		}
		if (m_ptRight.m_x < box.m_dMinX)
		{
			box.m_dMinX = m_ptRight.m_x;
		}
		else if (m_ptRight.m_x > box.m_dMaxX)
		{
			box.m_dMaxX = m_ptRight.m_x;
		}
		if (m_ptLeft.m_x < box.m_dMinX)
		{
			box.m_dMinX = m_ptLeft.m_x;
		}
		else if (m_ptLeft.m_x > box.m_dMaxX)
		{
			box.m_dMaxX = m_ptLeft.m_x;
		}

		box.m_dMinY = m_ptDown.m_y;
		box.m_dMaxY = m_ptDown.m_y;
			if (m_ptUp.m_y < box.m_dMinY)
			{
				box.m_dMinY = m_ptUp.m_y;
			}
			else if (m_ptUp.m_y > box.m_dMaxY)
			{
				box.m_dMaxY = m_ptUp.m_y;
			}
			if (m_ptRight.m_y < box.m_dMinY)
			{
				box.m_dMinY = m_ptRight.m_y;
			}
			else if (m_ptRight.m_y > box.m_dMaxY)
			{
				box.m_dMaxY = m_ptRight.m_y;
			}
			if (m_ptLeft.m_y < box.m_dMinY)
			{
				box.m_dMinY = m_ptLeft.m_y;
			}
			else if (m_ptLeft.m_y > box.m_dMaxY)
			{
				box.m_dMaxY = m_ptLeft.m_y;
			}
			box.m_dMinZ = m_ptDown.m_z;
			box.m_dMaxZ = m_ptDown.m_z;
			if (m_ptUp.m_z < box.m_dMinZ)
			{
				box.m_dMinZ = m_ptUp.m_z;
			}
			else if (m_ptUp.m_z > box.m_dMaxZ)
			{
				box.m_dMaxZ = m_ptUp.m_z;
			}
			if (m_ptRight.m_z < box.m_dMinZ)
			{
				box.m_dMinZ = m_ptRight.m_z;
			}
			else if (m_ptRight.m_z > box.m_dMaxZ)
			{
				box.m_dMaxZ = m_ptRight.m_z;
			}
			if (m_ptLeft.m_z < box.m_dMinZ)
			{
				box.m_dMinZ = m_ptLeft.m_z;
			}
			else if (m_ptLeft.m_z > box.m_dMaxZ)
			{
				box.m_dMaxZ = m_ptLeft.m_z;
			}
		return box;
	}

	CHDCrossLine& CHDCrossLine::operator=(const	CHDCrossLine& other)
	{
		m_ptDown = other.m_ptDown;
		m_ptLeft = other.m_ptLeft;
		m_ptUp = other.m_ptUp;
		m_ptRight = other.m_ptRight;
		m_ptCenter = other.m_ptCenter;
		return *this;
	}

}
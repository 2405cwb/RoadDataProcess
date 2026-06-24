#include "StdAfx.h"
#include "HD2DBoundingBox.h"
#include <float.h>
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{

	CHD2DBoundingBox::CHD2DBoundingBox(void)
	{
		m_dMinX =  DBL_MAX;		// 设置Box横坐标最小值为DBL_MAX
		m_dMinY =  DBL_MAX;		// 设置Box纵坐标最小值为DBL_MAX
		m_dMaxX = -DBL_MAX;		// 设置Box横坐标最大值为-DBL_MAX
		m_dMaxY = -DBL_MAX;		// 设置Box纵坐标最大值为-DBL_MAX
	}


	CHD2DBoundingBox::~CHD2DBoundingBox(void)
	{
	}

	void CHD2DBoundingBox::InsertPoint(double dx, double dy)
	{
		m_dMinX = dx < m_dMinX ? dx : m_dMinX;
		m_dMinY = dy < m_dMinY ? dy : m_dMinY;
		m_dMaxX = dx > m_dMaxX ? dx : m_dMaxX;
		m_dMaxY = dy > m_dMaxY ? dy : m_dMaxY;
	}

	void CHD2DBoundingBox::InsertBoundingBox(const CHD2DBoundingBox& bBox)
	{
		m_dMinX = bBox.m_dMinX < m_dMinX ? bBox.m_dMinX : m_dMinX;
		m_dMinY = bBox.m_dMinY < m_dMinY ? bBox.m_dMinY : m_dMinY;
		m_dMaxX = bBox.m_dMaxX > m_dMaxX ? bBox.m_dMaxX : m_dMaxX;
		m_dMaxY = bBox.m_dMaxY > m_dMaxY ? bBox.m_dMaxY : m_dMaxY;
	}

	CHD2DBoundingBox& CHD2DBoundingBox::operator=(const CHD2DBoundingBox& other)
	{
		m_dMinX = other.m_dMinX;
		m_dMinY = other.m_dMinY;
		m_dMaxX = other.m_dMaxX;
		m_dMaxY = other.m_dMaxY;
		return *this;
	}
	void CHD2DBoundingBox::ResetBoundingBox()
	{
		m_dMinX =  DBL_MAX;		
		m_dMinY =  DBL_MAX;		
		m_dMaxX = -DBL_MAX;		
		m_dMaxY = -DBL_MAX;	
	}

	// Box扩展
	void CHD2DBoundingBox::Extend(double dExt)
	{
		Extend(dExt, dExt);
	}

	// Box扩展
	void CHD2DBoundingBox::Extend(double dXExt, double dYExt)
	{
		m_dMinX -= dXExt;		
		m_dMinY -= dYExt;		
		m_dMaxX += dXExt;
		m_dMaxY += dYExt;	
	}

	bool CHD2DBoundingBox::IsInBox(double dx, double dy,double dError)
	{
		if (dx>= m_dMinX-dError && dx <= m_dMaxX+dError 
			&& dy>= m_dMinY-dError && dy <= m_dMaxY+dError)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}
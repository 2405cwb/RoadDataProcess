#include "StdAfx.h"
#include "HD3DNormalPoint.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
namespace hd
{

	CHD3DNormalPoint::CHD3DNormalPoint(void)
	{
		m_normal.X = 0;
		m_normal.Y = 0;
		m_normal.Z = 0;
	}

	CHD3DNormalPoint::CHD3DNormalPoint(double dX, double dY, double dZ, double dNX, double dNY, double dNZ)
		:CHD3DPoint(dX, dY, dZ),m_normal(dNX, dNY, dNZ)
	{

	}


	CHD3DNormalPoint::~CHD3DNormalPoint(void)
	{
	}

	CHD3DBoundingBox CHD3DNormalPoint::GetBoundingBox() const
	{
		CHD3DBoundingBox bBox;
		bBox.InsertPoint(m_x, m_y, m_z);
		return bBox;
	}

	bool CHD3DNormalPoint::IsInSphere(double dX, double dY, double dZ, double dR) const
	{
		return CHD3DPoint::IsInSphere(dX, dY, dZ, dR);
	}

	CHD3DNormalPoint& CHD3DNormalPoint::operator=(const CHD3DNormalPoint& other)
	{
		m_x = other.m_x;
		m_y = other.m_y;
		m_z = other.m_z;
		return *this;
	}
}
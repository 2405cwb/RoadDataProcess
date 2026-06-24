#include "StdAfx.h"
#include "HD3DPoint.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	CHD3DPoint::CHD3DPoint(void)
		:m_x(0.0), m_y(0.0), m_z(0.0),m_index(0)
	{
	}

	CHD3DPoint::CHD3DPoint(double dX, double dY, double dZ)
		:m_x(dX), m_y(dY), m_z(dZ),m_index(0)
	{
	}

	CHD3DPoint::CHD3DPoint(double dX, double dY, double dZ,int index)
		:m_x(dX), m_y(dY), m_z(dZ),m_index(index)
	{
	}

	CHD3DPoint::~CHD3DPoint(void)
	{
	}

	CHD3DBoundingBox CHD3DPoint::GetBoundingBox() const
	{
		CHD3DBoundingBox bBox;
		bBox.InsertPoint(m_x, m_y, m_z);
		return bBox;
	}

	bool CHD3DPoint::IsInSphere(double dX, double dY, double dZ, double dR) const
	{
		double dSphDistance, dSphR;
		// 点到球心的距离
		dSphDistance = (m_x - dX) * (m_x - dX) + (m_y - dY) * (m_y - dY) + (m_z - dZ) * (m_z - dZ);
		// 球心的半径
		dSphR = dR * dR;
		return dSphDistance < dSphR ? true : false;
	}

	bool CHD3DPoint::IsInSphere(double dX, double dY, double dR) const
	{
		double dSphDistance, dSphR;
		// 点到球心的距离
		dSphDistance = (m_x - dX) * (m_x - dX) + (m_y - dY) * (m_y - dY);
		// 球心的半径
		dSphR = dR * dR;
		return dSphDistance < dSphR ? true : false;
	}

	CHD3DPoint& CHD3DPoint::operator=(const	CHD3DPoint& other)
	{
		m_x = other.m_x;
		m_y = other.m_y;
		m_z = other.m_z;
		return *this;
	}

	CHD3DPoint CHD3DPoint::operator-(const	CHD3DPoint& other) const
	{
		CHD3DPoint point(m_x - other.m_x, m_y - other.m_y, m_z - other.m_z);
		return point;
	}

	CHD3DPoint CHD3DPoint::operator-() const
	{
		return CHD3DPoint(-m_x, -m_y, -m_z);
	}

	CHD3DPoint CHD3DPoint::operator+(const	CHD3DPoint& other) const
	{
		CHD3DPoint point(m_x + other.m_x, m_y + other.m_y, m_z + other.m_z);
		return point;
	}

	CHD3DPoint	CHD3DPoint::operator*(double dVal) const
	{
		CHD3DPoint point(m_x * dVal, m_y * dVal, m_z * dVal);
		return point;
	}

	CHD3DPoint	CHD3DPoint::operator/(double dVal) const
	{
		CHD3DPoint point(m_x / dVal, m_y / dVal, m_z / dVal);
		return point;
	}

}
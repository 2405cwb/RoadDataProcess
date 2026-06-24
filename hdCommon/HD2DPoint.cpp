#include "StdAfx.h"
#include "HD2DPoint.h"
#include <math.h>
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{

	CHD2DPoint::CHD2DPoint(void)
		:m_x(0.0),m_y(0.0)
	{
	}

	CHD2DPoint::CHD2DPoint(double dX, double dY)
		:m_x(dX),m_y(dY)
	{
	}

	CHD2DPoint::CHD2DPoint(const HD_2DPOINT& pt)
		:m_x(pt.X),m_y(pt.Y)
	{
	}

	CHD2DPoint::~CHD2DPoint(void)
	{
	}

	CHD2DPoint& CHD2DPoint::operator=(const	CHD2DPoint& other)
	{
		m_x = other.m_x;
		m_y = other.m_y;
		return *this;
	}

	CHD2DBoundingBox CHD2DPoint::GetBoundingBox() const
	{
		CHD2DBoundingBox bBox;
		bBox.InsertPoint(m_x, m_y);
		return bBox;
	}

	CHD2DPoint CHD2DPoint::operator-() const
	{
		return CHD2DPoint(-m_x, -m_x);
	}

	CHD2DPoint CHD2DPoint::operator-(const	CHD2DPoint& other) const
	{
		return CHD2DPoint(m_x - other.m_x, m_y - other.m_y);
	}

	CHD2DPoint CHD2DPoint::operator+(const	CHD2DPoint& other) const
	{
		CHD2DPoint point;
		point.m_x = m_x + other.m_x;
		point.m_y = m_y + other.m_y;
		return point;
	}

	CHD2DPoint	CHD2DPoint::operator*(double dVal) const
	{
		CHD2DPoint point;
		point.m_x = m_x * dVal;
		point.m_y = m_y * dVal;
		return point;
	}

	CHD2DPoint CHD2DPoint::operator/(double dVal) const
	{
		CHD2DPoint point;
		point.m_x = m_x / dVal;
		point.m_y = m_y / dVal;
		return point;
	}

	double CHD2DPoint::Distace(HD_2DPOINT& pt)
	{
		return sqrt((pt.X - m_x)*(pt.X - m_x) + (pt.Y - m_y)*(pt.Y - m_y));
	}

	double CHD2DPoint::Distace(CHD2DPoint& pt)
	{
		return sqrt((pt.m_x - m_x)*(pt.m_x - m_x) + (pt.m_y - m_y)*(pt.m_y - m_y));
	}

}
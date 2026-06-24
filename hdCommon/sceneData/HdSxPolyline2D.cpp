#include "StdAfx.h"
#include "HdSxPolyline2D.h"
#include <string>
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

using namespace std;

namespace hd
{
	CHdSxPolyline2D::CHdSxPolyline2D(void)
	{
		//m_eType = ESDT_OBJECT_POLYLINE2D;
	}
	
	CHdSxPolyline2D::~CHdSxPolyline2D(void)
	{
	}
		
	//多线段编辑接口
	void CHdSxPolyline2D::AddPoint(const CHdSxPoint2D& point)
	{
		m_Polyline2D.push_back(point);

		if (m_Polyline2D.size() == 1)
		{
			m_MaxPt = m_MinPt = point;
		}
		else
		{
			UpdateBox(point);
		}
	}

	void CHdSxPolyline2D::AddPoint(double x, double y)
	{
		CHdSxPoint2D pt(x,y);
		m_Polyline2D.push_back(pt);

		if (m_Polyline2D.size() == 1)
		{
			m_MaxPt = m_MinPt = pt;
		}
		else
		{
			UpdateBox(pt);
		}
	}

	void CHdSxPolyline2D::DeletePoint(unsigned int nIndex)
	{
		if (nIndex >=0 && nIndex < m_Polyline2D.size())
		{
			m_Polyline2D.erase(m_Polyline2D.begin() + nIndex);

			RecalclateBox();
		}
	}

	void CHdSxPolyline2D::InsertAfter(unsigned int nIndex,const CHdSxPoint2D& point)
	{
		if (nIndex >= 0 && nIndex < m_Polyline2D.size())
		{
			m_Polyline2D.insert(m_Polyline2D.begin() + nIndex + 1, point);

			if (m_Polyline2D.size() == 1)
			{
				m_MaxPt = m_MinPt = point;
			}
			else
			{
				UpdateBox(point);
			}
		}
	}

	bool CHdSxPolyline2D::SetLastPoint(double x, double y)
	{
		if (m_Polyline2D.size() > 0)
		{
			unsigned int nCount = m_Polyline2D.size();
			if (nCount > 0)
			{
				CHdSxPoint2D& pt = m_Polyline2D.at(nCount - 1);
				pt.m_x = x;
				pt.m_y = y;

				RecalclateBox();
				return true;
			}
		}
		return false;
	}

	bool CHdSxPolyline2D::SetLastPoint(float colScale,float rowScale,double x, double y)
	{
		unsigned int nCount = m_Polyline2D.size();
		if (nCount > 0)
		{
			CHdSxPoint2D& pt = m_Polyline2D.at(nCount - 1);
			pt.m_x = x;
			pt.m_y = y;
			pt.m_fRow = rowScale;
			pt.m_fCol = colScale;

			RecalclateBox();
			return true;
		}

		return false;
	}

	void CHdSxPolyline2D::DeleteLastPoint()
	{
		unsigned int nCount = m_Polyline2D.size();
		DeletePoint(nCount - 1);
	}

	//闭合二维多段线
	void CHdSxPolyline2D::Close()
	{
		const CHdSxPoint2D& firstPoint = GetPoint(0);
		InsertAfter(m_Polyline2D.size()-1,firstPoint);
	}

	void CHdSxPolyline2D::ClosePlanar()
	{
		const CHdSxPoint2D& firstPoint = GetPoint(0);		
		InsertAfter(m_Polyline2D.size()-1,firstPoint);

	}

	unsigned int CHdSxPolyline2D::GetPointCount() const
	{
		return m_Polyline2D.size();
	}

	const CHdSxPoint2D& CHdSxPolyline2D::GetPoint(unsigned int nIndex) const
	{
		return m_Polyline2D.at(nIndex);
	}

	void CHdSxPolyline2D::UpdateBox(const CHdSxPoint2D& point)
	{
		if (point.m_x < m_MinPt.m_x)
			m_MinPt.m_x = point.m_x;
		else if (point.m_x > m_MaxPt.m_x)
			m_MaxPt.m_x = point.m_x;

		if (point.m_y < m_MinPt.m_y)
			m_MinPt.m_y = point.m_y;
		else if (point.m_y > m_MaxPt.m_y)
			m_MaxPt.m_y = point.m_y;
	}

	void CHdSxPolyline2D::RecalclateBox()
	{
		unsigned int i;
		unsigned int nCount = m_Polyline2D.size();
		if (nCount > 0)
		{
			m_MinPt = m_MaxPt = m_Polyline2D.at(0);
			for (i = 1; i<nCount; i++)
			{
				UpdateBox(m_Polyline2D.at(i));
			}
		}
		else
		{
			m_MinPt = m_MaxPt = CHdSxPoint2D(0.0, 0.0);
		}
	}

	void CHdSxPolyline2D::SetPoint( unsigned int nIndex,const CHdSxPoint2D& pPoint, bool bReCalcBox /*= false*/ )
	{
		if (nIndex >= 0 && nIndex < m_Polyline2D.size())
		{
			CHdSxPoint2D& pt = m_Polyline2D.at(nIndex);
			pt.m_x = pPoint.m_x;
			pt.m_y = pPoint.m_y;

			if (bReCalcBox)
			{
				RecalclateBox();
			}
		}
	}

}
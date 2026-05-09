#include "StdAfx.h"
#include "HD2DPolygon.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{

	CHD2DPolygon::CHD2DPolygon(void)
	{
	}


	CHD2DPolygon::~CHD2DPolygon(void)
	{

	}

	unsigned int CHD2DPolygon::GetVertexCount() const
	{
		return m_vecVertexs.size();
	}

	HD_2DPOINT CHD2DPolygon::GetVertex(unsigned int nIndex) const
	{
		if (nIndex < m_vecVertexs.size())
		{
			return m_vecVertexs.at(nIndex);	
		}
		else
		{
			HD_2DPOINT NullIndex;
			return NullIndex;
		}
	}

	void CHD2DPolygon::AddVertex(const HD_2DPOINT& pt)
	{
		m_BBox.InsertPoint(pt.X, pt.Y);
		m_vecVertexs.push_back(pt);
	}

	void CHD2DPolygon::AddVertex(double dX, double dY)
	{
		HD_2DPOINT pt;
		pt.X = dX;
		pt.Y = dY;
		m_BBox.InsertPoint(pt.X, pt.Y);
		m_vecVertexs.push_back(pt);
	}

	bool CHD2DPolygon::SetVertex(unsigned nIndex, const HD_2DPOINT& pt)
	{
		vector<HD_2DPOINT>& vertexs = m_vecVertexs;
		//判断nIndex是否为有效区间
		if(nIndex < m_vecVertexs.size())
		{
			vertexs[nIndex].X = pt.X;
			vertexs[nIndex].Y = pt.Y;
			m_BBox.InsertPoint(vertexs[nIndex].X, vertexs[nIndex].Y);
			return true;
		}
		else 
			return false;
	}

	bool CHD2DPolygon::InsertVertex(const HD_2DPOINT& pt, unsigned int nIndex )
	{
		unsigned int nSize = m_vecVertexs.size();
		if(nIndex < nSize)
		{
			m_vecVertexs.insert(m_vecVertexs.begin () + nIndex, pt);
		}

		//判断是否成功添加点
		if((nSize + 1) == m_vecVertexs.size())
		{
			m_BBox.InsertPoint(pt.X, pt.Y);
			return true;
		}
		else 
			return false;
	}

	bool CHD2DPolygon::DeleteVertex(unsigned int nIndex)
	{
		unsigned int nSize = m_vecVertexs.size();
		if(nIndex < nSize)
		{
			m_vecVertexs.erase(m_vecVertexs.begin() + nIndex);
		}

		//判断是否删除成功
		if((nSize - 1) == m_vecVertexs.size())
		{
			ReCalcBoundingBox();
			return true;
		}
		else
			return false;
	}

	void CHD2DPolygon::ClearALL()
	{
		m_vecVertexs.clear();
		m_BBox.ResetBoundingBox();
	}

	void CHD2DPolygon::ReCalcBoundingBox()
	{
		CHD2DBoundingBox bBox;
		for(unsigned int i = 0; i < m_vecVertexs.size(); i++)
		{
			bBox.InsertPoint(m_vecVertexs[i].X, m_vecVertexs[i].Y);
		}
		m_BBox = bBox;
	}

	CHD2DPolygon& CHD2DPolygon::operator=(const CHD2DPolygon& other)
	{
		m_vecVertexs = other.m_vecVertexs;
		m_BBox = other.m_BBox;
		return *this;
	}

	bool CHD2DPolygon::IsPointIn(const HD_2DPOINT& pt) const
	{
		int nCrossCount = 0;		// 统计单边交点数
		int nSize = m_vecVertexs.size();

		for (int i=0; i<nSize; i++)
		{
			HD_2DPOINT pt1 = m_vecVertexs[i];
			HD_2DPOINT pt2 = m_vecVertexs[(i+1) % nSize];

			// 求y=pt.Y与多边形边的交点
			if(pt1.Y == pt2.Y)       // 多边形边与y=pt.Y平行
				continue;
			if (pt.Y < min(pt1.Y, pt2.Y))	// 小于边的两个顶点纵坐标的最小值
				continue;
			if (pt.Y > max(pt1.Y, pt2.Y))	// 大于边的两个顶点纵坐标的最大值
				continue;
			// 交点的X坐标
			double X = (pt.Y - pt1.Y)*(pt2.X - pt1.X)/(pt2.Y - pt1.Y) + pt1.X;

			if (X > pt.X)
				nCrossCount++;	// 只统计单边交点个数
		}

		// 单边交点为奇数，交点在多变形内部，为偶数则在多边形外部
		if (nCrossCount % 2 == 1)
			return true;
		else
			return false;
	}

	bool CHD2DPolygon::IsPointIn(double dX, double dY) const
	{
		HD_2DPOINT pt;
		pt.X = dX;
		pt.Y = dY;
		int nCrossCount = 0;		// 统计单边交点数
		int nSize = m_vecVertexs.size();

		for (int i=0; i<nSize; i++)
		{
			HD_2DPOINT pt1 = m_vecVertexs[i];
			HD_2DPOINT pt2 = m_vecVertexs[(i+1) % nSize];

			// 求y=pt.Y与多边形边的交点
			if(pt1.Y == pt2.Y)       // 多边形边与y=pt.Y平行
				continue;
			if (pt.Y < min(pt1.Y, pt2.Y))	// 小于边的两个顶点纵坐标的最小值
				continue;
			if (pt.Y > max(pt1.Y, pt2.Y))	// 大于边的两个顶点纵坐标的最大值
				continue;
			// 交点的X坐标
			double X = (pt.Y - pt1.Y)*(pt2.X - pt1.X)/(pt2.Y - pt1.Y) + pt1.X;

			if (X > pt.X)
				nCrossCount++;	// 只统计单边交点个数
		}

		// 单边交点为奇数，交点在多变形内部，为偶数则在多边形外部
		if (nCrossCount % 2 == 1)
		{
			return true;
		}
		else
			return false;
	}

}
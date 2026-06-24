#include "StdAfx.h"
#include "HD3DPolyline.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
namespace hd
{

	CHD3DPolyline::CHD3DPolyline(void)
	{
	}


	CHD3DPolyline::~CHD3DPolyline(void)
	{
	}

	unsigned int CHD3DPolyline::GetVertexCount() const
	{
		return m_vecVertexs.size();
	}

	HD_3DPOINT CHD3DPolyline::GetVertex(unsigned int nIndex) const
	{
		// 判断nIndex是否合法
		if(m_vecVertexs.empty() || nIndex > m_vecVertexs.size())
		{
			HD_3DPOINT NullPoint(0, 0, 0);
			return NullPoint;
		}
		return m_vecVertexs.at(nIndex);
	}

	void CHD3DPolyline::AddVertex(const HD_3DPOINT& pt)
	{
		m_bBox.InsertPoint(pt.X, pt.Y, pt.Z);
		m_vecVertexs.push_back(pt);
	}

	void CHD3DPolyline::AddVertex(double dX, double dY, double dZ)
	{
		HD_3DPOINT pt(dX, dY, dZ);
		m_bBox.InsertPoint(pt.X, pt.Y, pt.Z);
		m_vecVertexs.push_back(pt);
	}

	bool CHD3DPolyline::SetVertex(unsigned int nIndex, const HD_3DPOINT& pt)
	{
		// 判断nIndex是否为有效区间
		if(nIndex < m_vecVertexs.size())
		{
			m_vecVertexs[nIndex].X = pt.X;
			m_vecVertexs[nIndex].Y = pt.Y;
			m_vecVertexs[nIndex].Z = pt.Z;
			RecalcBoundingBox();
			return true;
		}
		else 
		{
			return false;
		}
	}

	// 更改指定索引位置的节点的值
	bool CHD3DPolyline::SetVertex(unsigned int nIndex, double dX, double dY, double dZ)
	{
		// 判断nIndex是否为有效区间
		if(nIndex < m_vecVertexs.size())
		{
			m_vecVertexs[nIndex].X = dX;
			m_vecVertexs[nIndex].Y = dY;
			m_vecVertexs[nIndex].Z = dZ;
			RecalcBoundingBox();
			return true;
		}
		else 
		{
			return false;
		}
	}

	bool CHD3DPolyline::InsertVertex(unsigned int nIndex, const HD_3DPOINT& pt)
	{
		// 判断nIndex是否合法
		if(nIndex > m_vecVertexs.size())
		{
			return false;
		}

		int nSize = m_vecVertexs.size();
		m_vecVertexs.insert(m_vecVertexs.begin () + nIndex - 1, pt);
		// 判断是否成功添加点
		if((nSize + 1) == m_vecVertexs.size())
		{
			m_bBox.InsertPoint(pt.X, pt.Y, pt.Z);
			return true;
		}
		else 
		{
			return false;
		}
	}

	bool CHD3DPolyline::DeleteVertex(unsigned int nIndex)
	{
		// 判断nIndex是否合法
		if(nIndex > m_vecVertexs.size())
		{
			return false;
		}

		int nSize = m_vecVertexs.size();
		m_vecVertexs.erase(m_vecVertexs.begin() + nIndex - 1);
		// 判断是否删除成功
		if((nSize - 1) == m_vecVertexs.size())
		{
			RecalcBoundingBox();
			return true;
		}
		else
		{
			return false;
		}
	}

	void CHD3DPolyline::ClearAll()
	{

		m_vecVertexs.clear();
		RecalcBoundingBox();
	}

	CHD3DBoundingBox CHD3DPolyline::GetBoundingBox() const
	{
		return m_bBox;
	}

	void CHD3DPolyline::RecalcBoundingBox()
	{
		CHD3DBoundingBox bBox;
		for(unsigned int i = 0; i < m_vecVertexs.size(); i++)
		{
			bBox.InsertPoint(m_vecVertexs[i].X, m_vecVertexs[i].Y, m_vecVertexs[i].Z);
		}
		m_bBox = bBox;
	}

	CHD3DPolyline& CHD3DPolyline::operator=(const CHD3DPolyline& other)
	{
		m_vecVertexs = other.m_vecVertexs;
		m_bBox = other.m_bBox;
		return *this;
	}

	HD_3DPOINT CHD3DPolyline::GetClosestPoint(const HD_3DPOINT* pStartPt, const HD_3DPOINT* pEndPt, const HD_3DPOINT* pPoint) const
	{
		HD_3DPOINT c;
		c.X = pPoint->X - pStartPt->X; 
		c.Z	= pPoint->Z - pStartPt->Z;

		HD_3DPOINT v;
		v.X = pEndPt->X - pStartPt->X;
		v.Y = pEndPt->Y - pStartPt->Y;
		v.Z = pEndPt->Z - pStartPt->Z;

		double d = sqrt((v.X*v.X) + (v.Y*v.Y) + (v.Z*v.Z));

		v.X /= d;
		v.Y /= d;
		v.Z /= d;

		double t = v.X*c.X + v.Y*c.Y + v.Z*c.Z;

		HD_3DPOINT PointRet;
		if (t < 0.0)
		{
			PointRet.X = pStartPt->X;
			PointRet.Y = pStartPt->Y;
			PointRet.Z = pStartPt->Z;
			return PointRet;
		}
		if (t > d)
		{
			PointRet.X = pEndPt->X;
			PointRet.Y = pEndPt->Y;
			PointRet.Z = pEndPt->Z;
			return PointRet;
		}

		v.X *= t;
		v.Y *= t;
		v.Z *= t;
		PointRet.X = pStartPt->X + v.X;
		PointRet.Y = pStartPt->Y + v.Y;
		PointRet.Z = pStartPt->Z + v.Z;
		return PointRet;
	}

	double CHD3DPolyline::GetClosestPoint(const HD_3DPOINT* pPoint, HD_3DPOINT& closestPt) const
	{
		double fMinDist = FLT_MAX;

		int i;
		int nPtCount = GetVertexCount();

		//分别计算多线段上每条线段和点之间的最短距离
		for (i = 0; i < nPtCount-1; i++)
		{
			HD_3DPOINT pt = GetClosestPoint(&m_vecVertexs[i], &m_vecVertexs[(i+1)%nPtCount], pPoint);
			HD_3DPOINT c ;
			c.X = pt.X -pPoint->X;
			c.Y = pt.Y -pPoint->Y;
			c.Z = pt.Z -pPoint->Z;
			double fCurDist = sqrt(c.X*c.X + c.Y*c.Y + c.Z*c.Z);
			if (fMinDist > fCurDist)
			{
				fMinDist = fCurDist;
				closestPt = pt;
			}
		}

		return fMinDist;
	}

	// 判断是否在多段线的包围盒内
	bool CHD3DPolyline::IsInPolyLineBox(double dX, double dY, double dZ, double dError)
	{
		// 先扩展一部分
		m_bBox.Extend(dError);
		if(dX>=m_bBox.m_dMinX && dX<=m_bBox.m_dMaxX && dY>=m_bBox.m_dMinY && dY<=m_bBox.m_dMaxY && dZ>=m_bBox.m_dMinZ && dZ<m_bBox.m_dMaxZ)
		{
			return true;
		}
		else
		{
			return false;
		}
		// 再回复原样
		m_bBox.Extend(-dError);
	}

	bool CHD3DPolyline::IsInSphere(double dX, double dY, double dZ, double dR) const
	{
		HD_3DPOINT centerPt(dX, dY, dZ);
		HD_3DPOINT closestPt;
		double fMinDist = GetClosestPoint(&centerPt, closestPt);
		return (fMinDist <= dR);
	}

	// 判断一个点是否在多边形的边上，如果在，返回边的点索引
	void CHD3DPolyline::PointIsOnEdge(double dX,double dY,double dZ, int& nStartIndex, int& nEndIndex,double fError)
	{
		// 先判断是否在包围盒内
		int nPtCount = m_vecVertexs.size();
		HD_3DPOINT* pPoint= new HD_3DPOINT();
		for (int i=0;i<nPtCount;i++)
		{
			CHD3DPolyline pPolyline;
			pPolyline.AddVertex(m_vecVertexs[i]);
			pPolyline.AddVertex(m_vecVertexs[(i+1)%nPtCount]);
			if (pPolyline.IsInPolyLineBox(dX,dY,dZ,5.0))
			{
				nStartIndex = i;
				nEndIndex = (i+1)%nPtCount;
				break;
			}
		}

		if (pPoint)
		{
			delete pPoint;
			pPoint = NULL;
		}
		nStartIndex = nEndIndex = -1;	
	}
}
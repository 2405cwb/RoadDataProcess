#include "StdAfx.h"
#include "HD2DPolyline.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{

	CHD2DPolyline::CHD2DPolyline(void)
	{
	}

	CHD2DPolyline::~CHD2DPolyline(void)
	{
	}

	unsigned int CHD2DPolyline::GetVertexCount() const
	{
		return m_vecVertexs.size();
	}

	HD_2DPOINT CHD2DPolyline::GetVertex(unsigned int nIndex) const
	{
		return m_vecVertexs.at(nIndex);
	}

	void CHD2DPolyline::AddVertex(const HD_2DPOINT& pt)
	{
		m_BBox.InsertPoint(pt.X, pt.Y);
		m_vecVertexs.push_back(pt);
	}

	void CHD2DPolyline::AddVertex(double dX, double dY)
	{
		HD_2DPOINT pt;
		pt.X = dX;
		pt.Y = dY;
		m_BBox.InsertPoint(pt.X, pt.Y);
		m_vecVertexs.push_back(pt);

	}

	bool CHD2DPolyline::SetVertex(unsigned int nIndex, const HD_2DPOINT& pt)
	{
		// 判断nIndex是否为有效区间
		if(nIndex < m_vecVertexs.size())
		{
			m_vecVertexs[nIndex].X = pt.X;
			m_vecVertexs[nIndex].Y = pt.Y;
			
			ReCalcBoundingBox();
			return true;
		}
		else 
		{
			return false;
		}
	}

	// 更改指定索引位置的值
	bool CHD2DPolyline::SetVertex(unsigned int nIndex, double dX, double dY)
	{
		// 判断nIndex是否为有效区间
		if(nIndex < m_vecVertexs.size())
		{
			m_vecVertexs[nIndex].X = dX;
			m_vecVertexs[nIndex].Y = dY;
			
			ReCalcBoundingBox();
			return true;
		}
		else 
		{
			return false;
		}
	}

	bool CHD2DPolyline::InsertVertex(unsigned int nIndex, const HD_2DPOINT& pt)
	{
		// 判断是否在有效范围内
		unsigned int nSize = m_vecVertexs.size();
		if (nIndex < nSize)
		{
			m_vecVertexs.insert(m_vecVertexs.begin() + nIndex, pt);
		}

		// 判断是否成功添加点*****************
		if((nSize + 1) == m_vecVertexs.size())
		{
			m_BBox.InsertPoint(pt.X, pt.Y);
			return true;
		}
		else 
		{
			return false;
		}
	}

	bool CHD2DPolyline::DeleteVertex(unsigned int nIndex)
	{
		unsigned int nSize = m_vecVertexs.size();

		if (nIndex < nSize)
		{
			m_vecVertexs.erase(m_vecVertexs.begin() + nIndex);
		}

		// 判断是否删除成功
		if((nSize - 1) == m_vecVertexs.size())
		{
			ReCalcBoundingBox();
			return true;
		}
		else
		{
			return false;
		}
	}

	void CHD2DPolyline::ClearALL()
	{
		m_vecVertexs.clear();
		m_BBox.ResetBoundingBox();
	}

	void CHD2DPolyline::ReCalcBoundingBox()
	{
		CHD2DBoundingBox bBox;
		vector<HD_2DPOINT>& vertexs = m_vecVertexs;

		for(unsigned int i = 0; i < m_vecVertexs.size(); i++)
		{
			bBox.InsertPoint(vertexs[i].X, vertexs[i].Y);
		}
		m_BBox = bBox;
	}

	CHD2DPolyline& CHD2DPolyline::operator=(const CHD2DPolyline& other)
	{
		m_vecVertexs = other.m_vecVertexs;
		m_BBox = other.m_BBox;
		return *this;
	}

	//判断点到多线段之间的最短距离是否小于fRadius
	bool CHD2DPolyline::IsInRange(const HD_2DPOINT* pPoint, float fRadius) const
	{
		float fRadiusSQ = fRadius*fRadius;

		int i;
		int nPtCount = GetVertexCount();

		//分别计算多线段上每条线段和点之间的最短距离
		for (i = 0; i < nPtCount-1; i++)
		{
			//得到离当前线段最近的点
			HD_2DPOINT pt = GetClosestPoint(&m_vecVertexs[i], &m_vecVertexs[(i+1)%nPtCount], pPoint);

			HD_2DPOINT c ;
			c.X = pt.X - pPoint->X;
			c.Y = pt.Y - pPoint->Y;
			double fCurDist = c.X*c.X + c.Y*c.Y;
			if (fRadiusSQ > fCurDist)
			{
				return true;
			}
		}

		return false;
	}

	double CHD2DPolyline::GetClosestPoint(const HD_2DPOINT* pPoint, HD_2DPOINT& closestPt) const
	{
		double fMinDist = FLT_MAX;

		int i;
		int nPtCount = GetVertexCount();

		//分别计算多线段上每条线段和点之间的最短距离
		for (i = 0; i < nPtCount-1; i++)
		{
			HD_2DPOINT pt = GetClosestPoint(&m_vecVertexs[i], &m_vecVertexs[(i+1)%nPtCount], pPoint);

			HD_2DPOINT c ;
			c.X = pt.X - pPoint->X;
			c.Y = pt.Y - pPoint->Y;
			double fCurDist = sqrt(c.X*c.X + c.Y*c.Y);
			if (fMinDist > fCurDist)
			{
				fMinDist = fCurDist;
				closestPt = pt;
			}
		}

		return fMinDist;
	}

	HD_2DPOINT CHD2DPolyline::GetClosestPoint(const HD_2DPOINT* pStartPt, const HD_2DPOINT* pEndPt, const HD_2DPOINT* pPoint) const
	{
		HD_2DPOINT c;
		c.X = pPoint->X - pStartPt->X;
		c.Y = pPoint->Y - pStartPt->Y;

		HD_2DPOINT v;
		v.X = pEndPt->X - pStartPt->X;
		v.Y = pEndPt->Y - pStartPt->Y;

		double d = sqrt((v.X*v.X) + (v.Y*v.Y));

		v.X /= d;
		v.Y /= d;

		double t = v.X*c.X + v.Y*c.Y;

		HD_2DPOINT PointRet;
		if (t < 0.0)
		{
			PointRet.X = pStartPt->X;
			PointRet.Y = pStartPt->Y;
			return PointRet;
		}
		if (t > d)
		{
			PointRet.X = pEndPt->X;
			PointRet.Y = pEndPt->Y;
			return PointRet;
		}

		v.X *= t;
		v.Y *= t;
		PointRet.X = pStartPt->X + v.X;
		PointRet.Y = pStartPt->Y + v.Y;
		return PointRet;
	}

	// 判断点是否在线上
	bool CHD2DPolyline::IsOnLine(double dX,double dY,double dError)
	{
		// 首先判断是否在包围盒范围内，防止延长线的点
		if (m_BBox.IsInBox(dX,dY,5))
		{
			// 计算三个点构成的三角形的面积，共线时面积趋向于0
			double dArea(0);
			double dLength(0);
			/*
			面积公式：
				Area（ABC） = 1/2*(向量AC 叉乘 向量BC) = 1/2*(ax-cx)(by-cy)-(bx-cx)(ay-cy)
			*/
			int nPointCount = m_vecVertexs.size();
			if (nPointCount<2)
			{
				return false;
			}

			HD_2DPOINT pPoint;
			// 遍历,判断该点是否与该线的每个线段共线
			for (int i=0;i<nPointCount;i++)
			{
				dArea = (dX - m_vecVertexs[i].X)*(m_vecVertexs[(i+1)%nPointCount].Y-m_vecVertexs[i].Y)
					-(m_vecVertexs[(i+1)%nPointCount].X-m_vecVertexs[i].X)*(dY-m_vecVertexs[i].Y);

				// 2个端点,x相减,y相减，求其最大的值，再放大1.5倍，作为容差
				dLength = max(fabs(m_vecVertexs[i].X-m_vecVertexs[(i+1)%nPointCount].X),fabs(m_vecVertexs[i].Y-m_vecVertexs[(i+1)%nPointCount].Y));

				//dError = max(dLength,dError);

				// 是否有效
				if (fabs(dArea)<=(dLength*5))
				{
					return true;
				}
			}

			return false;
		}

		return false;
	}

	// 判断是否相交,并求出交点，add by mzm 2014.11.26
	bool CHD2DPolyline::IsIntersect(const CHD2DPolyline* pPolyLine, HD_2DPOINT& pCrossPoint)
	{
		// 判断指针是否为空
		if (!m_vecVertexs.empty() && pPolyLine)
		{
			// 当前线的点数
			int nPointCount = m_vecVertexs.size();
			// 被比较线的点
			int nOtherPointCount = pPolyLine->GetVertexCount();

			if (nOtherPointCount>1)
			{
				// 定义点
				HD_2DPOINT pFirst1, pFirst2,pSecond1,pSecond2;

				// 遍历当前线的所有点
				for (int i=0;i<nPointCount-1;i++)
				{
					pFirst1 = m_vecVertexs.at(i);
					pFirst2 = m_vecVertexs.at(i+1);

					// 遍历被比较线的所有点
					for (int j=0;j<nOtherPointCount-1;j++)
					{
						pSecond1 = pPolyLine->GetVertex(j);
						pSecond2 = pPolyLine->GetVertex(j+1);

						// 判断是否相交
						if (IsLineIntersectLine(pFirst1,pFirst2,pSecond1,pSecond2,pCrossPoint))
						{
							return true;
						}
					}
				}
			}
			else
			{
				return false;
			}
		}

		return false;
	}

	// 判断2个线段是否相交
	bool CHD2DPolyline::IsLineIntersectLine(const HD_2DPOINT& pFirst1,const HD_2DPOINT& pFirst2,const HD_2DPOINT& pSecond1,const HD_2DPOINT& pSecond2, HD_2DPOINT& pCrossPoint)
	{
		//每个线段的两点都在另一个线段的左右不同侧，则能断定线段相交   
		//公式对于向量(x1,y1)->(x2,y2),判断点(x3,y3)在向量的左边,右边,还是线上.   
		//p=x1(y3-y2)+x2(y1-y3)+x3(y2-y1). p<0 左侧,    p=0 线上, p>0 右侧   
		double Linep1,Linep2;

		//判断pSecond1和pSecond2是否在pFirst1->pFirst2两侧  

		Linep1 = pFirst1.X*(pSecond1.Y-pFirst2.Y)+pFirst2.X*(pFirst1.Y-pSecond1.Y)
				+pSecond1.X*(pFirst2.Y-pFirst1.Y);

		Linep2 = pFirst1.X*(pSecond2.Y-pFirst2.Y)+pFirst2.X*(pFirst1.Y-pSecond2.Y)
			+ pSecond2.X*(pFirst2.Y -pFirst1.Y);

		long l1 = Linep1;
		long l2 = Linep2;

		// 符号位异或为0:pSecond1和pSecond2在pFirst1->pFirst2同侧
		if (((l1^l2)>=0) && !(Linep1 == 0 && Linep2 ==0))
		{
			return false;
		}

		 //判断pFirst1和pFirst2是否在pSecond1->pSecond2两侧 

		Linep1 = pSecond1.X*(pFirst1.Y-pSecond2.Y)+pSecond2.X*(pSecond1.Y-pFirst1.Y)
			+pFirst1.X*(pSecond2.Y-pSecond1.Y);

		Linep2 = pSecond1.X*(pFirst2.Y-pSecond2.Y)+pSecond2.X*(pSecond1.Y-pFirst2.Y)
			+ pFirst2.X*(pSecond2.Y -pSecond1.Y);

		l1 = Linep1;
		l2 = Linep2;

		// 符号位异或为0:pSecond1和pSecond2在pFirst1->pFirst2同侧
		if (((l1^l2)>=0) && !(Linep1 == 0 && Linep2 ==0))
		{
			return false;
		}

		// 求交点
		// 根据两点化为标准式，进而求线性方程组
		double tmpLeft,tmpRight;
		tmpLeft =  (pSecond2.X- pSecond1.X) * (pFirst1.Y - pFirst2.Y) - (pFirst2.X - pFirst1.X) * (pSecond1.Y - pSecond2.Y);

		tmpRight = (pFirst1.Y - pSecond1.Y) * (pFirst2.X - pFirst1.X) * (pSecond2.X - pSecond1.X) + pSecond1.X * (pSecond2.Y - pSecond1.Y) * (pFirst2.X - pFirst1.X) - pFirst1.X * (pFirst2.Y - pFirst1.Y) * (pSecond2.X - pSecond1.X);  
		pCrossPoint.X=tmpRight / tmpLeft;

		//求Y坐标     
		tmpLeft = (pFirst1.X - pFirst2.X) * (pSecond2.Y - pSecond1.Y) - (pFirst2.Y - pFirst1.Y) * (pSecond1.X - pSecond2.X);  
		tmpRight = pFirst2.Y * (pFirst1.X - pFirst2.X) * (pSecond2.Y - pSecond1.Y) + (pSecond2.X- pFirst2.X) * (pSecond2.Y - pSecond1.Y) * (pFirst1.Y - pFirst2.Y) - pSecond2.Y * (pSecond1.X - pSecond2.X) * (pFirst2.Y - pFirst1.Y);  

		pCrossPoint.Y =tmpRight/tmpLeft; 

		return true;
	}

	// 通过距离获得多段线上某一点
	// 思路为：先判断传入的距离在哪2个点之间的距离范围内，然后再在对应的线段上计算出点
	bool CHD2DPolyline::GetPointByDis(double dis,HD_2DPOINT& point)
	{
		int nCount = m_vecVertexs.size();

		// 如果没有点或者小于2，都直接返回false
		if (m_vecVertexs.empty() || nCount<2)
		{
			return false;
		}
		else
		{
			// 记录每段距离之和
			double dSum(0);
			// 每段距离值
			double ds(0);

			// 获取4个点
			double dFirstX,dFirstY,dSecX,dSecY;
			// 遍历
			for (int i=0;i<nCount-1;i++)
			{
				// 获取第一个点坐标
				dFirstX = m_vecVertexs.at(i).X;
				dFirstY = m_vecVertexs.at(i).Y;

				// 获取第2个点坐标
				dSecX = m_vecVertexs.at(i+1).X;
				dSecY = m_vecVertexs.at(i+1).Y;
				
				// 计算距离
				ds = sqrt((dFirstX-dSecX)* (dFirstX-dSecX)+ 
					(dFirstY-dSecY)* (dFirstY-dSecY));

				// 判断距离是否为0
				if (ds==0)
				{
					continue;
				}

				// 如果距离在范围内，说明找到了，然后求出点
				if (dis >dSum && dis <= (dSum+ds))
				{
					double d1 = dis- dSum;
					point.X = (d1*dSecX + (ds - d1)*dFirstX)/ds;
					point.Y = (d1*dSecY + (ds - d1)*dFirstY)/ds;
					return true;
				}

				// 记录距离
				dSum += ds;
			}

			return false;
		}
	}

	// 获取长度
	double CHD2DPolyline::GetLength()
	{
		int nCount = m_vecVertexs.size();

		// 如果没有点或者小于2，都直接返回false
		if (m_vecVertexs.empty() || nCount<2)
		{
			return 0;
		}
		else
		{
			// 记录每段距离之和
			double dSum(0);
			// 每段距离值
			double ds(0);

			// 获取4个点
			double dFirstX,dFirstY,dSecX,dSecY;
			// 遍历
			for (int i=0;i<nCount-1;i++)
			{
				// 获取第一个点坐标
				dFirstX = m_vecVertexs.at(i).X;
				dFirstY = m_vecVertexs.at(i).Y;

				// 获取第2个点坐标
				dSecX = m_vecVertexs.at(i+1).X;
				dSecY = m_vecVertexs.at(i+1).Y;

				// 计算距离
				ds = sqrt((dFirstX-dSecX)* (dFirstX-dSecX)+ 
					(dFirstY-dSecY)* (dFirstY-dSecY));

				// 记录距离
				dSum += ds;
			}

			return dSum;
		}
	}
}

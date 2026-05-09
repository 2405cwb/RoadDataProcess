#include "StdAfx.h"
#include "HDMarkerFacade.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
namespace hd
{
	CHDMarkerFacade::CHDMarkerFacade(void)
	{
	}


	CHDMarkerFacade::~CHDMarkerFacade(void)
	{
	}

	CHD3DBoundingBox CHDMarkerFacade::GetBoundingBox() const
	{
		return m_Polyline.GetBoundingBox();
	}

	bool CHDMarkerFacade::IsInSphere(double dX, double dY, double dZ, double dR) const
	{
		// m_Polyline的每个顶点都在球体内，则整个面片在球体内
		HD_2DPOINT pt;
		if (GetCenterPt(pt))
		{
			double dist = (dX - pt.X)*(dX - pt.X) + (dY - pt.Y)*(dY - pt.Y);
			if (dist <= dR*dR)
			{
				return true;
			}
		}

		return false;
	}

	bool CHDMarkerFacade::IsInBox(CHD3DBoundingBox& bBox)
	{
		CHD3DBoundingBox m_bBox = GetBoundingBox();
		// bBox的任意一个角的顶点在包围盒内就返回true
		if(BoxCollision(m_bBox, bBox.m_dMinX, bBox.m_dMinY, bBox.m_dMinZ)
			|| BoxCollision(m_bBox, bBox.m_dMinX, bBox.m_dMaxY, bBox.m_dMinZ)
			|| BoxCollision(m_bBox, bBox.m_dMinX, bBox.m_dMaxY, bBox.m_dMaxZ)
			|| BoxCollision(m_bBox, bBox.m_dMinX, bBox.m_dMinY, bBox.m_dMaxZ)
			|| BoxCollision(m_bBox, bBox.m_dMaxX, bBox.m_dMaxY, bBox.m_dMaxZ)
			|| BoxCollision(m_bBox, bBox.m_dMaxX, bBox.m_dMaxY, bBox.m_dMinZ)
			|| BoxCollision(m_bBox, bBox.m_dMaxX, bBox.m_dMinY, bBox.m_dMaxZ)
			|| BoxCollision(m_bBox, bBox.m_dMaxX, bBox.m_dMinY, bBox.m_dMinZ)
			)
		{
			return true;
		}
		// bBox包含包围盒范围则返回true
		else if((m_bBox.m_dMaxX < bBox.m_dMaxX)
			&& (m_bBox.m_dMaxY < bBox.m_dMaxY)
			&& (m_bBox.m_dMaxZ < bBox.m_dMaxZ)
			&& (m_bBox.m_dMinX > bBox.m_dMinX)
			&& (m_bBox.m_dMinY > bBox.m_dMinY)
			&& (m_bBox.m_dMinZ > bBox.m_dMinZ)
			)
		{
			return true;
		}
		return false;
	}

	bool CHDMarkerFacade::BoxCollision(CHD3DBoundingBox bBox, double dX, double dY, double dZ)
	{

		if(bBox.m_dMinX < dX && bBox.m_dMaxX > dX)
		{
			if(bBox.m_dMinY < dY && bBox.m_dMaxY > dY)
			{
				if(bBox.m_dMinZ < dZ && bBox.m_dMaxZ > dZ)
				{
					return true;
				}
			}
		}
		return false;
	}

	HD_3DPOINT CHDMarkerFacade::GetNormal()
	{
		// 获取点的个数
		unsigned int nVertexCount = m_Polyline.GetVertexCount();
		// 定义Nab，Nac用来存储两个平面向量,Normal用来存储法向量
		HD_3DPOINT Nab, Nac, Normal;
		// PointA用来存储平面第一个点的坐标
		HD_3DPOINT PointA = m_Polyline.GetVertex(1);
		// PointB用来存储平面第二个点的坐标
		HD_3DPOINT PointB = m_Polyline.GetVertex(2);
		// PointC用来存储平面最后一个点的坐标
		HD_3DPOINT PointC = m_Polyline.GetVertex(nVertexCount);

		Nab.X = PointB.X - PointA.X;
		Nab.Y = PointB.Y - PointA.Y;
		Nab.Z = PointB.Z - PointA.Z;
		Nac.X = PointC.X - PointA.X;
		Nac.Y = PointC.Y - PointA.Y;
		Nac.Z = PointC.Z - PointA.Z;

		Normal.X = Nab.Y * Nac.Z - Nac.Y * Nab.Z;
		Normal.Y = Nab.Z * Nac.X - Nac.Z * Nab.X;
		Normal.Z = Nab.X * Nac.Y - Nac.X * Nab.Y;
		return Normal;
	}

	bool CHDMarkerFacade::CreateFacade(int nVertexCount, double* pVertexs)
	{
		m_Polyline.ClearAll();

		// 判断是否大于等于4个点，且能够被2整除
		if(nVertexCount >= 4 && (nVertexCount % 2 == 0))
		{
			//// 从第二个点开始遍历
			//for(int i = 1; i < nVertexCount; i++)
			//{
			//	// 判断每个点的Z值是否相等，如果不等则返回false
			//	if(pVertexs[i*3+2] != pVertexs[2])
			//	{
			//		return false;
			//	}
			//}

			// 如果合法则建立立面
			for(int i = 0; i < nVertexCount; i++)
			{
				m_Polyline.AddVertex(pVertexs[i*3], pVertexs[i*3+1], pVertexs[i*3+2]);
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	// 根据底边和高创建立面
	bool CHDMarkerFacade::CreateFacade(int nVertexCount, double* pVertexs, double dZMin, double dZMax)
	{
		if (nVertexCount < 2 || !pVertexs || (dZMax <= dZMin))
		{
			return false;
		}

		m_Polyline.ClearAll();

		// 添加底边上的点
		for (int i = 0; i<nVertexCount; i++)
		{
			m_Polyline.AddVertex(pVertexs[2*i], pVertexs[2*i+1], dZMin);
		}

		// 添加顶边上的点
		for (int i = nVertexCount-1; i>=0; i--)
		{
			m_Polyline.AddVertex(pVertexs[2*i], pVertexs[2*i+1], dZMax);
		}

		return true;
	}

	// 获取当前面的中心点
	bool CHDMarkerFacade::GetCenterPt(HD_2DPOINT& pt) const
	{
		// 获取点的个数
		unsigned int nVertexCount = m_Polyline.GetVertexCount();
		if (nVertexCount > 0)
		{
			unsigned int nHalfCount = nVertexCount / 2;
			double dx = 0.0f, dy = 0.0f;
			for (unsigned int i = 0; i< nHalfCount; i++)
			{
				dx += m_Polyline.GetVertex(i).X;
				dy += m_Polyline.GetVertex(i).Y;
			}

			pt.X = dx/nHalfCount;
			pt.Y = dy/nHalfCount;
			return true;
		}

		return false;
	}

	// 获取面片类型
	int CHDMarkerFacade::GetFacadeType()
	{
		// 获取节点个数
		unsigned int nCount = m_Polyline.GetVertexCount();
		// 大于4为多片面,小于4为单面片
		if (nCount > 4)
		{
			return 2;
		}
		else
		{
			return 1;
		}
	}

	// 得到多线段节点个数
	unsigned int CHDMarkerFacade::GetVertexCount() const
	{
		return m_Polyline.GetVertexCount();
	}

	// 得到指定索引位置的节点
	HD_3DPOINT CHDMarkerFacade::GetVertex( unsigned int nIndex ) const
	{
		return m_Polyline.GetVertex(nIndex);
	}

	// 在多线段尾部添加一个节点
	void CHDMarkerFacade::AddVertex( const HD_3DPOINT& pt )
	{
		m_Polyline.AddVertex(pt);
	}

	// 在多线段尾部添加一个节点
	void CHDMarkerFacade::AddVertex( double dX, double dY, double dZ )
	{
		m_Polyline.AddVertex(dX, dY, dZ);
	}

	//更改立面的节点坐标
	bool CHDMarkerFacade::UpdateVertex(int nIndex, double dX, double dY, double dZ)
	{
		switch (nIndex)
		{
		case 0:
			{
				// 屏蔽掉判断条件，面片导入有反的可能，yf 20130828
				//if (GetVertex(3).Z > dZ)
				{
					HD_3DPOINT pt0(dX, dY, dZ);
					HD_3DPOINT pt01(m_Polyline.GetVertex(0).X, m_Polyline.GetVertex(0).Y, m_Polyline.GetVertex(0).Z);
					HD_3DPOINT dpt0(pt0.X-pt01.X, pt0.Y-pt01.Y, pt0.Z-pt01.Z);					
					
					m_Polyline.SetVertex(0, pt0);
					HD_3DPOINT pt1(GetVertex(1).X, GetVertex(1).Y, GetVertex(1).Z);
					HD_3DPOINT pt2(GetVertex(2).X, GetVertex(2).Y, GetVertex(2).Z);
					HD_3DPOINT dpt02(pt01.X-pt2.X, pt01.Y-pt2.Y, pt01.Z-pt2.Z);
					HD_3DPOINT dpt12(pt1.X-pt2.X, pt1.Y-pt2.Y, pt1.Z-pt2.Z);
					HD_3DPOINT dpt1(dpt0.X*dpt12.X/dpt02.X, dpt0.Y*dpt12.Y/dpt02.Y, dpt0.Z*dpt12.Z/dpt02.Z);

					HD_3DPOINT pt11(pt1.X+dpt1.X, pt1.Y+dpt1.Y, pt1.Z+dpt1.Z);
					m_Polyline.SetVertex(1, pt11);
					HD_3DPOINT mpt(pt0.X+pt2.X, pt0.Y+pt2.Y, pt0.Z+pt2.Z);
					HD_3DPOINT pt3(mpt.X-pt11.X, mpt.Y-pt11.Y, mpt.Z-pt11.Z);
					m_Polyline.SetVertex(3, pt3);

					return true;
				}
				//else
				//	return false;				
			}
		case 1:
			{

				//if (GetVertex(3).Z > dZ)
				{
					HD_3DPOINT pt1(dX, dY, dZ);
					HD_3DPOINT pt11(m_Polyline.GetVertex(1).X, m_Polyline.GetVertex(1).Y, m_Polyline.GetVertex(1).Z);
					HD_3DPOINT dpt1(pt1.X-pt11.X, pt1.Y-pt11.Y, pt1.Z-pt11.Z);					

					m_Polyline.SetVertex(1, pt1);
					HD_3DPOINT pt0(GetVertex(0).X, GetVertex(0).Y, GetVertex(0).Z);
					HD_3DPOINT pt3(GetVertex(3).X, GetVertex(3).Y, GetVertex(3).Z);
					HD_3DPOINT dpt13(pt11.X-pt3.X, pt11.Y-pt3.Y, pt11.Z-pt3.Z);
					HD_3DPOINT dpt03(pt0.X-pt3.X, pt0.Y-pt3.Y, pt0.Z-pt3.Z);
					HD_3DPOINT dpt0(dpt1.X*dpt03.X/dpt13.X, dpt1.Y*dpt03.Y/dpt13.Y, dpt1.Z*dpt03.Z/dpt13.Z);

					HD_3DPOINT pt00(pt0.X+dpt0.X, pt0.Y+dpt0.Y, pt0.Z+dpt0.Z);
					m_Polyline.SetVertex(0, pt00);
					HD_3DPOINT mpt(pt1.X+pt3.X, pt1.Y+pt3.Y, pt1.Z+pt3.Z);
					HD_3DPOINT pt2(mpt.X-pt00.X, mpt.Y-pt00.Y, mpt.Z-pt00.Z);
					m_Polyline.SetVertex(2, pt2);

					return true;
				}
				//else
				//	return false;
			}
		case 2:
			{
				//if (dZ > GetVertex(0).Z)
				{
					HD_3DPOINT pt2(dX, dY, dZ);
				HD_3DPOINT pt22(m_Polyline.GetVertex(2).X, m_Polyline.GetVertex(2).Y, m_Polyline.GetVertex(2).Z);
				HD_3DPOINT dpt2(pt2.X-pt22.X, pt2.Y-pt22.Y, pt2.Z-pt22.Z);					

				m_Polyline.SetVertex(2, pt2);
				HD_3DPOINT pt0(GetVertex(0).X, GetVertex(0).Y, GetVertex(0).Z);
				HD_3DPOINT pt1(GetVertex(1).X, GetVertex(1).Y, GetVertex(1).Z);
				HD_3DPOINT dpt20(pt22.X-pt0.X, pt22.Y-pt0.Y, pt22.Z-pt0.Z);
				HD_3DPOINT dpt10(pt1.X-pt0.X, pt1.Y-pt0.Y, pt1.Z-pt0.Z);
				HD_3DPOINT dpt1(dpt2.X*dpt10.X/dpt20.X, dpt2.Y*dpt10.Y/dpt20.Y, dpt2.Z*dpt10.Z/dpt20.Z);

				HD_3DPOINT pt11(pt1.X+dpt1.X, pt1.Y+dpt1.Y, pt1.Z+dpt1.Z);
				m_Polyline.SetVertex(1, pt11);
				HD_3DPOINT mpt(pt0.X+pt2.X, pt0.Y+pt2.Y, pt0.Z+pt2.Z);
				HD_3DPOINT pt3(mpt.X-pt11.X, mpt.Y-pt11.Y, mpt.Z-pt11.Z);
				m_Polyline.SetVertex(3, pt3);

					return true;
				}
				//else
				//	return false;
			}
		case 3:
			{
				//if (dZ > GetVertex(0).Z)
				{
					HD_3DPOINT pt3(dX, dY, dZ);
					HD_3DPOINT pt33(m_Polyline.GetVertex(3).X, m_Polyline.GetVertex(3).Y, m_Polyline.GetVertex(3).Z);
					HD_3DPOINT dpt3(pt3.X-pt33.X, pt3.Y-pt33.Y, pt3.Z-pt33.Z);					

					m_Polyline.SetVertex(3, pt3);
					HD_3DPOINT pt0(GetVertex(0).X, GetVertex(0).Y, GetVertex(0).Z);
					HD_3DPOINT pt1(GetVertex(1).X, GetVertex(1).Y, GetVertex(1).Z);
					HD_3DPOINT dpt31(pt33.X-pt1.X, pt33.Y-pt1.Y, pt33.Z-pt1.Z);
					HD_3DPOINT dpt01(pt0.X-pt1.X, pt0.Y-pt1.Y, pt0.Z-pt1.Z);
					HD_3DPOINT dpt0(dpt3.X*dpt01.X/dpt31.X, dpt3.Y*dpt01.Y/dpt31.Y, dpt3.Z*dpt01.Z/dpt31.Z);

					HD_3DPOINT pt00(pt0.X+dpt0.X, pt0.Y+dpt0.Y, pt0.Z+dpt0.Z);
					m_Polyline.SetVertex(0, pt00);
					HD_3DPOINT mpt(pt1.X+pt3.X, pt1.Y+pt3.Y, pt1.Z+pt3.Z);
					HD_3DPOINT pt2(mpt.X-pt00.X, mpt.Y-pt00.Y, mpt.Z-pt00.Z);
					m_Polyline.SetVertex(2, pt2);

					return true;
				}
				//else
				//	return false;
			}
		default:
			return false;
		}

		return true;
	}

}
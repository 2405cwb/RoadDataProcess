#include "StdAfx.h"
#include "HDFacade.h"


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
namespace hd
{
	CHDFacade::CHDFacade(void)
		:m_nMapID(0)
		,m_bFacType(1)
	{
	}


	CHDFacade::~CHDFacade(void)
	{
	}

	CHD3DBoundingBox CHDFacade::GetBoundingBox() const
	{
		return m_Polyline.GetBoundingBox();
	}

	bool CHDFacade::IsInSphere(double dX, double dY, double dZ, double dR) const
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

	bool CHDFacade::IsInBox(CHD3DBoundingBox& bBox)
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

	bool CHDFacade::BoxCollision(CHD3DBoundingBox bBox, double dX, double dY, double dZ)
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

	HD_3DPOINT CHDFacade::GetNormal()
	{
		// 获取点的个数
		unsigned int nVertexCount = m_Polyline.GetVertexCount();
		// 定义Nab，Nac用来存储两个平面向量,Normal用来存储法向量
		HD_3DPOINT Nab, Nbc, Normal;
		// PointA用来存储平面第一个点的坐标
		HD_3DPOINT PointA = m_Polyline.GetVertex(0);
		// PointB用来存储平面第二个点的坐标
		HD_3DPOINT PointB = m_Polyline.GetVertex(1);
		// PointC用来存储平面最后一个点的坐标
		HD_3DPOINT PointC = m_Polyline.GetVertex(2);

		Nab.X = PointB.X - PointA.X;
		Nab.Y = PointB.Y - PointA.Y;
		Nab.Z = PointB.Z - PointA.Z;
		Nbc.X = PointC.X - PointB.X;
		Nbc.Y = PointC.Y - PointB.Y;
		Nbc.Z = PointC.Z - PointB.Z;

		Normal.X = Nab.Y * Nbc.Z - Nbc.Y * Nab.Z;
		Normal.Y = Nab.Z * Nbc.X - Nbc.Z * Nab.X;
		Normal.Z = Nab.X * Nbc.Y - Nbc.X * Nab.Y;
		return Normal;
	}

	bool CHDFacade::CreateFacade(int nVertexCount, double* pVertexs)
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
	bool CHDFacade::CreateFacade(int nVertexCount, double* pVertexs, double dZMin, double dZMax)
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

	// 根据CHD3DPolyline创建立面
	bool CHDFacade::CreateFacade(const CHD3DPolyline& poly)
	{
		m_Polyline.ClearAll();

		unsigned int nVertexCount = poly.GetVertexCount();

		// 判断是否大于等于4个点，且能够被2整除
		if(nVertexCount >= 4 && (nVertexCount % 2 == 0))
		{
			// 如果合法则建立立面
			for (unsigned int i = 0; i < nVertexCount; i++)
			{
				m_Polyline.AddVertex(poly.GetVertex(i));
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	// 获取当前面的中心点
	bool CHDFacade::GetCenterPt(HD_2DPOINT& pt) const
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

	bool CHDFacade::GetCenterPt(HD_3DPOINT& pt) const
	{
		// 获取点的个数
		unsigned int nVertexCount = m_Polyline.GetVertexCount();
		if (nVertexCount > 0)
		{
			double dx = 0.0f, dy = 0.0f, dz = 0.0f;
			for (unsigned int i = 0; i< nVertexCount; i++)
			{
				dx += m_Polyline.GetVertex(i).X;
				dy += m_Polyline.GetVertex(i).Y;
				dz += m_Polyline.GetVertex(i).Z;
			}

			pt.X = dx/nVertexCount;
			pt.Y = dy/nVertexCount;
			pt.Z = dz/nVertexCount;
			return true;
		}

		return false;
	}

	// 获取面片类型
	int CHDFacade::GetFacadeType()
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
	unsigned int CHDFacade::GetVertexCount() const
	{
		return m_Polyline.GetVertexCount();
	}

	// 得到指定索引位置的节点
	HD_3DPOINT CHDFacade::GetVertex( unsigned int nIndex ) const
	{
		return m_Polyline.GetVertex(nIndex);
	}

	// 在多线段尾部添加一个节点
	void CHDFacade::AddVertex( const HD_3DPOINT& pt )
	{
		m_Polyline.AddVertex(pt);
	}

	// 在多线段尾部添加一个节点
	void CHDFacade::AddVertex( double dX, double dY, double dZ )
	{
		m_Polyline.AddVertex(dX, dY, dZ);
	}

	// 设置点
	void CHDFacade::SetVertex(int index, double x, double y, double z)
	{
		m_Polyline.SetVertex(index, x, y, z);
	}
	//更改立面的节点坐标
	bool CHDFacade::UpdateVertex(int nIndex, double dX, double dY, double dZ)
	{
		switch (nIndex)
		{
		case 0:
			{
				// 屏蔽掉判断条件，面片导入有反的可能，yf 20130828
				//if (GetVertex(3).Z > dZ)
				{
					HD_3DPOINT pt0(dX, dY, dZ);
					m_Polyline.SetVertex(0, pt0);
					HD_3DPOINT pt1(GetVertex(1).X, GetVertex(1).Y, dZ);
					m_Polyline.SetVertex(1, pt1);
					HD_3DPOINT pt3(dX, dY, GetVertex(3).Z);
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
					m_Polyline.SetVertex(1, pt1);
					HD_3DPOINT pt0(GetVertex(0).X, GetVertex(0).Y, dZ);
					m_Polyline.SetVertex(0, pt0);
					HD_3DPOINT pt2(dX, dY, GetVertex(2).Z);
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
					m_Polyline.SetVertex(2, pt2);
					HD_3DPOINT pt3(GetVertex(3).X, GetVertex(3).Y, dZ);
					m_Polyline.SetVertex(3, pt3);
					HD_3DPOINT pt1(dX, dY, GetVertex(1).Z);
					m_Polyline.SetVertex(1, pt1);

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
					m_Polyline.SetVertex(3, pt3);
					HD_3DPOINT pt2(GetVertex(2).X, GetVertex(2).Y, dZ);
					m_Polyline.SetVertex(2, pt2);
					HD_3DPOINT pt0(dX, dY, GetVertex(0).Z);
					m_Polyline.SetVertex(0, pt0);

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

	// 更改立面的顺序，03互换，12互换
	void CHDFacade::SwapVertex()
	{
		HD_3DPOINT pt0 = m_Polyline.GetVertex(0);
		HD_3DPOINT pt1 = m_Polyline.GetVertex(1);
		HD_3DPOINT pt2 = m_Polyline.GetVertex(2);
		HD_3DPOINT pt3 = m_Polyline.GetVertex(3);

		m_Polyline.SetVertex(0, pt3);
		m_Polyline.SetVertex(1, pt2);
		m_Polyline.SetVertex(2, pt1);
		m_Polyline.SetVertex(3, pt0);
	}

	// 判断面片的高差,dOffSet高度偏移（向上的偏移） 2014/12/23 lwm
	bool CHDFacade::CheckHigh(double dCenterHigh ,double dOffSet /*= 5*/, double dOffSetLow /*= -1*/)
	{
		// 获取点的个数
		unsigned int nVertexCount = m_Polyline.GetVertexCount();
		if (nVertexCount > 0)
		{
			double dz = 0.0f;
			for (unsigned int i = 0; i< nVertexCount; i++)
			{
				dz += m_Polyline.GetVertex(i).Z;
			}

			dz = dz/nVertexCount;

			dz = dz - dCenterHigh;

			// 判断是否在范围内 
			if (dz>= dOffSet || dz <= dOffSetLow)
			{
				return false;
			}
			else
			{
				return true;
			}
		}
		return false;
	}

}
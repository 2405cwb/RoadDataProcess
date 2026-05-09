#include "StdAfx.h"
#include "hdFilterSelectPoly.h"
#include "../hdCommon/point_types2.h"

using namespace irr::core;

namespace hd
{
	namespace ptcloud
	{
		hdFilterSelectPoly::hdFilterSelectPoly(E_Select_Mode select_type, 
			vector2df* screen_pts, int pt_count, dimension2du view_size, f32* view_prj)
			:hdFilterSelection(select_type, view_size, view_prj), m_geo_ring(NULL), m_geo_point(NULL)
		{
			if (pt_count >= 3)
			{
				m_geo_ring = OGRGeometryFactory::createGeometry(wkbLinearRing);
				for (int i=0; i<pt_count; i++)
				{
					((OGRLineString*)m_geo_ring)->addPoint((double)screen_pts[i].X, (double)screen_pts[i].Y);
				}
				((OGRLinearRing*)m_geo_ring)->closeRings();

				m_geo_point = OGRGeometryFactory::createGeometry(wkbPoint);
			}
		}

		hdFilterSelectPoly::hdFilterSelectPoly(E_Select_Mode select_type, irr::core::vector2df* angleCoords, int pt_count):
		    hdFilterSelection(select_type), m_geo_ring(NULL), m_geo_point(NULL)
		{
			if (pt_count >= 3)
			{
				m_geo_ring = OGRGeometryFactory::createGeometry(wkbLinearRing);
				for (int i=0; i<pt_count; i++)
				{
					((OGRLineString*)m_geo_ring)->addPoint((double)angleCoords[i].X, (double)angleCoords[i].Y);
				}
				((OGRLinearRing*)m_geo_ring)->closeRings();

				m_geo_point = OGRGeometryFactory::createGeometry(wkbPoint);
			}
		}

		hdFilterSelectPoly::~hdFilterSelectPoly()
		{
			// 释放geo对象内存
			if (m_geo_ring)
			{
				OGRGeometryFactory::destroyGeometry(m_geo_ring);
				m_geo_ring = NULL;
			}
			if (m_geo_point)
			{
				OGRGeometryFactory::destroyGeometry(m_geo_point);
				m_geo_point = NULL;
			}
		}

		//! 过滤处理，更新选中状态
		void hdFilterSelectPoly::doFilter(hdVector<PointXYZIPRGBA>* pt_array,
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
		{
			if (!pt_array)
			{
				return;
			}

			if (m_select_envi == E_SELECT_SCENE)  // 三维场景
			{
				OGRLinearRing* geo_ring = (OGRLinearRing*)m_geo_ring;
				OGRPoint* geo_pt = (OGRPoint*)m_geo_point;
				//遍历块中所有点
				u32 count_in_block = pt_array->size();
				for (u32 pt_index = 0; pt_index < count_in_block; ++pt_index)
				{
					// 获得每个点
					PointXYZIPRGBA& pt = (*pt_array)[pt_index];
					if (!pt.isValid())
					{
						continue;
					}

					// 新建选择模式下，先设置为未选中状态
					if (m_select_mode == E_SELECT_MODE_NEW)
					{
						pt.setUnSelected();
					}

					// 转到屏幕坐标
					vector2df screen_pos = getScreenCoordinatesFrom3DPositionf(irr::core::vector3df(pt.x, pt.y, pt.z));
					if (screen_pos.X<0 || screen_pos.Y<0 ||	screen_pos.X>m_view_size.Width || screen_pos.Y>m_view_size.Height)
					{
						continue;
					}

					//判断点是否在多边形内
					geo_pt->setX(screen_pos.X);
					geo_pt->setY(screen_pos.Y);
					if (!(geo_ring->isPointInRing(geo_pt)))
					{
						continue;
					}

					// 增加、减少选择
					if (m_select_mode == E_SELECT_MODE_ADD || m_select_mode == E_SELECT_MODE_NEW)
					{
						pt.setSelected();
					}
					else //if (m_select_mode == E_SELECT_MODE_MINUS)
					{
						pt.setUnSelected();
					}
				}
			}
			else   // 灰度图
			{
				OGRLinearRing* geo_ring = (OGRLinearRing*)m_geo_ring;
				OGRPoint* geo_pt = (OGRPoint*)m_geo_point;
				//遍历块中所有点
				u32 count_in_block = pt_array->size();
				for (u32 pt_index = 0; pt_index < count_in_block; ++pt_index)
				{
					// 获得每个点
					PointXYZIPRGBA& pt = *(pt_array->_Myfirst + pt_index);
					if (!pt.isValid())
					{
						continue;
					}

					// 新建选择模式下，先设置为未选中状态
					if (m_select_mode == E_SELECT_MODE_NEW)
					{
						pt.setUnSelected();
					}

					// 转到角度坐标，单位：度
					double X = pt.x, Y = pt.y, Z = pt.z;
					Y = - Y;   // 右手系转左手系坐标
					double dfHorizAngle = atan2(Y, X) * RADTODEG64;
					double dfVertAngle = atan2(Z, sqrt(X * X + Y * Y)) * RADTODEG64;
					if (dfHorizAngle < 0)
					{
						dfHorizAngle += 360.0;
					}

					//判断点是否在多边形内
					geo_pt->setX(dfHorizAngle);
					geo_pt->setY(dfVertAngle);
					if (!(geo_ring->isPointInRing(geo_pt)))
					{
						continue;
					}

					// 增加、减少选择
					if (m_select_mode == E_SELECT_MODE_ADD || m_select_mode == E_SELECT_MODE_NEW)
					{
						pt.setSelected();
					}
					else //if (m_select_mode == E_SELECT_MODE_MINUS)
					{
						pt.setUnSelected();
					}
				}
			}
		}

		//! 过滤处理，更新选中状态
		void hdFilterSelectPoly::doFilter(CHdParcelBase* pt_array,
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
		{
			if (m_select_envi == E_SELECT_GRAYIMAGE)
			{
				return;
			}

			if (!pt_array || !pt_array->m_pHlzPoint)
			{
				return;
			}

			OGRLinearRing* geo_ring = (OGRLinearRing*)m_geo_ring;
			OGRPoint* geo_pt = (OGRPoint*)m_geo_point;
			//遍历块中所有点
			u32 count_in_block = pt_array->GetPtCount();
			for (u32 pt_index = 0; pt_index < count_in_block; ++pt_index)
			{
				// 获得每个点
				HlzPoint& pt = *(pt_array->m_pHlzPoint + pt_index);
				if (!pt.isValid())
				{
					continue;
				}

				// 新建选择模式下，先设置为未选中状态
				if (m_select_mode == E_SELECT_MODE_NEW)
				{
					pt.setUnSelected();
				}

				// 转到屏幕坐标
				vector2df screen_pos = getScreenCoordinatesFrom3DPositionf(irr::core::vector3df(pt.x, pt.y, pt.z));
				if (screen_pos.X<0 || screen_pos.Y<0 ||	screen_pos.X>m_view_size.Width || screen_pos.Y>m_view_size.Height)
				{
					continue;
				}

				//判断点是否在多边形内
				geo_pt->setX(screen_pos.X);
				geo_pt->setY(screen_pos.Y);
				if (!(geo_ring->isPointInRing(geo_pt)))
				{
					continue;
				}

				// 增加、减少选择
				if (m_select_mode == E_SELECT_MODE_ADD || m_select_mode == E_SELECT_MODE_NEW)
				{
					pt.setSelected();
				}
				else //if (m_select_mode == E_SELECT_MODE_MINUS)
				{
					pt.setUnSelected();
				}
			}
		}

		bool hdFilterSelectPoly::isIntersects(CHdBox3df& box)
		{
			return true;
		}
	}
}
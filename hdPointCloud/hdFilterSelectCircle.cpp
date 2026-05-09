#include "StdAfx.h"
#include "hdFilterSelectCircle.h"
#include "../hdCommon/point_types2.h"

using namespace irr::core;

namespace hd
{
	namespace ptcloud
	{
		hdFilterSelectCircle::hdFilterSelectCircle(E_Select_Mode select_type, 
			vector2df center, f32 radius, dimension2du view_size, f32* view_prj)
			:hdFilterSelection(select_type, view_size, view_prj), m_center(center), m_radius(radius)
		{
		}

		hdFilterSelectCircle::hdFilterSelectCircle(E_Select_Mode select_type, irr::core::vector2df center, f32 radius):
		    hdFilterSelection(select_type), m_center(center), m_radius(radius)
		{
		}

		hdFilterSelectCircle::~hdFilterSelectCircle(void)
		{
		}

		//! 过滤处理，更新选中状态
		void hdFilterSelectCircle::doFilter(hdVector<PointXYZIPRGBA>* pt_array,
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
		{
			if (!pt_array)
			{
				return;
			}

			if (m_select_envi == E_SELECT_SCENE)
			{
				f32 radius2 = m_radius*m_radius;
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

					// 转到屏幕坐标
					vector2df screen_pos = getScreenCoordinatesFrom3DPositionf(irr::core::vector3df(pt.x, pt.y, pt.z));
					if (screen_pos.X<0 || screen_pos.Y<0 ||	screen_pos.X>m_view_size.Width || screen_pos.Y>m_view_size.Height)
					{
						continue;
					}

					//判断点是否在圆形内
					f32 dx = screen_pos.X - m_center.X;
					f32 dy = screen_pos.Y - m_center.Y;
					bool bInside = (dx*dx+dy*dy) < radius2;
					if (!bInside)
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
			else
			{
				f32 radius2 = m_radius*m_radius;
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

					//判断点是否在圆形内
					f32 dx = dfHorizAngle - m_center.X;
					f32 dy = dfVertAngle - m_center.Y;
					bool bInside = (dx*dx+dy*dy) < radius2;
					if (!bInside)
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
		void hdFilterSelectCircle::doFilter(CHdParcelBase* pt_array,
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

			f32 radius2 = m_radius*m_radius;
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

				//判断点是否在圆形内
				f32 dx = screen_pos.X - m_center.X;
				f32 dy = screen_pos.Y - m_center.Y;
				bool bInside = (dx*dx+dy*dy) < radius2;
				if (!bInside)
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

		bool hdFilterSelectCircle::isIntersects(CHdBox3df& box)
		{
			return true;
		}
	}
}
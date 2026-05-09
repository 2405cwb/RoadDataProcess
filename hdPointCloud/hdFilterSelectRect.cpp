#include "StdAfx.h"
#include "hdFilterSelectRect.h"
#include "../hdCommon/point_types2.h"

using namespace irr::core;

namespace hd
{
	namespace ptcloud
	{
		hdFilterSelectRect::hdFilterSelectRect(E_Select_Mode select_type, 
			vector<vector2df>& rect_pts, dimension2du view_size, f32* view_prj)
			:hdFilterSelection(select_type, view_size, view_prj), m_rect_pts(rect_pts)
		{
			m_min_pt = m_rect_pts[0];
			m_max_pt = m_rect_pts[0];
			for (int i=1; i<4; i++)
			{
				if (m_rect_pts[i].X < m_min_pt.X)
				{
					m_min_pt.X = m_rect_pts[i].X;
				}
				if (m_rect_pts[i].Y < m_min_pt.Y)
				{
					m_min_pt.Y = m_rect_pts[i].Y;
				}
				if (m_rect_pts[i].X > m_max_pt.X)
				{
					m_max_pt.X = m_rect_pts[i].X;
				}
				if (m_rect_pts[i].Y > m_max_pt.Y)
				{
					m_max_pt.Y = m_rect_pts[i].Y;
				}
			}
		}

		hdFilterSelectRect::hdFilterSelectRect(E_Select_Mode select_type, vector<irr::core::vector2df>& rect_pts):
		    hdFilterSelection(select_type), m_rect_pts(rect_pts)
		{
			m_min_pt = m_rect_pts[0];
			m_max_pt = m_rect_pts[0];
			for (int i=1; i<4; i++)
			{
				if (m_rect_pts[i].X < m_min_pt.X)
				{
					m_min_pt.X = m_rect_pts[i].X;
				}
				if (m_rect_pts[i].Y < m_min_pt.Y)
				{
					m_min_pt.Y = m_rect_pts[i].Y;
				}
				if (m_rect_pts[i].X > m_max_pt.X)
				{
					m_max_pt.X = m_rect_pts[i].X;
				}
				if (m_rect_pts[i].Y > m_max_pt.Y)
				{
					m_max_pt.Y = m_rect_pts[i].Y;
				}
			}
		}

		hdFilterSelectRect::~hdFilterSelectRect(void)
		{
		}

		//! 过滤处理，更新选中状态
		void hdFilterSelectRect::doFilter(hdVector<PointXYZIPRGBA>* pt_array,
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
		{
			//if (m_select_mode==E_SELECT_MODE_ADD || m_select_mode==E_SELECT_MODE_MINUS)
			//{
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

					//判断点是否在矩形内
					if (!isPtInRect(screen_pos))
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
			//}
		}

		//! 过滤处理，更新选中状态
		void hdFilterSelectRect::doFilter(CHdParcelBase* pt_array,
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
		{
			//if (m_select_mode==E_SELECT_MODE_ADD || m_select_mode==E_SELECT_MODE_MINUS)
			//{
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

					//判断点是否在矩形内
					if (!isPtInRect(screen_pos))
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
			//}
		}

		//! 判断点是否在矩形内
		bool hdFilterSelectRect::isPtInRect(vector2df& pt)
		{
			if (pt.X<=m_min_pt.X || pt.X>=m_max_pt.X ||pt.Y<=m_min_pt.Y || pt.Y>=m_max_pt.Y)
			{
				return false;
			}

			vector<vector2df> d(4);
			for (int i=0; i<4; i++)
			{
				d[i].X = m_rect_pts[i].X - pt.X;
				d[i].Y = m_rect_pts[i].Y - pt.Y;
			}

			//矩形相对两边
			f32 s01 = d[0].X*d[1].Y - d[0].Y*d[1].X;
			f32 s32 = d[3].X*d[2].Y - d[3].Y*d[2].X;
			if (s01*s32 >= 0)
			{
				return false;
			}

			//矩形相对两边
			f32 s03 = d[0].X*d[3].Y - d[0].Y*d[3].X;
			f32 s12 = d[1].X*d[2].Y - d[1].Y*d[2].X;
			if (s03*s12 >= 0)
			{
				return false;
			}

			return true;
		}
	}
}
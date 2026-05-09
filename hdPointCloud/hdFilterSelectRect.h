#ifndef __HD_FILTER_SELECT_RECT_H_INCLUDED__
#define __HD_FILTER_SELECT_RECT_H_INCLUDED__
#include "hdFilterSelection.h"

namespace hd
{
	namespace ptcloud
	{
		class HDPOINTCLOUD_API hdFilterSelectRect : public hdFilterSelection
		{
		public:
			// 构造三维场景选择器
			hdFilterSelectRect(E_Select_Mode select_type, vector<irr::core::vector2df>& rect_pts, 
				irr::core::dimension2du view_size, f32* view_prj);

			// 构造灰度图选择器
			hdFilterSelectRect(E_Select_Mode select_type, vector<irr::core::vector2df>& rect_pts);

			virtual ~hdFilterSelectRect();

			//! 获取过滤器类型
			virtual E_Filter_Type getFilterType(){return E_FILTER_TYPE_SELECT_RECT;}

			//! 过滤处理，更新选中状态
			virtual void doFilter(hdVector<PointXYZIPRGBA>* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);

			//! 过滤处理，更新选中状态--处理所有过滤器
			virtual void doFilter(CHdParcelBase* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);

		private:
			//! 选择矩形--屏幕坐标
			vector<irr::core::vector2df> m_rect_pts;
			
			//! 矩形最大最小角点
			irr::core::vector2df  m_min_pt;
			irr::core::vector2df  m_max_pt;

			//! 判断点在矩形内
			bool isPtInRect(irr::core::vector2df& pt);
		};
	}
}

#endif
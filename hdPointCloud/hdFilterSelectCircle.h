#ifndef __HD_FILTER_SELECT_CIRCLE_H_INCLUDED__
#define __HD_FILTER_SELECT_CIRCLE_H_INCLUDED__
#include "hdFilterSelection.h"

namespace hd
{
	namespace ptcloud
	{
		class HDPOINTCLOUD_API hdFilterSelectCircle : public hdFilterSelection
		{
		public:
			// 构造三维场景选择器
			// center: 圆心的屏幕坐标
			hdFilterSelectCircle(E_Select_Mode select_type, irr::core::vector2df center, f32 radius, 
				irr::core::dimension2du view_size, f32* view_prj);

			// 构造灰度图选择器
			// center: 圆心的角度坐标（单位：度）
			hdFilterSelectCircle(E_Select_Mode select_type, irr::core::vector2df center, f32 radius);

			virtual ~hdFilterSelectCircle();

			//! 获取过滤器类型
			virtual E_Filter_Type getFilterType(){return E_FILTER_TYPE_SELECT_CIRCLE;}

			//! 过滤处理，更新选中状态
			virtual void doFilter(hdVector<PointXYZIPRGBA>* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);

			//! 过滤处理，更新选中状态
			virtual void doFilter(CHdParcelBase* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);

			//! 判断一个包围盒是否与选择区域相交，相交返回 true
			virtual bool isIntersects(CHdBox3df& box);

		private:
			//! 选择圆形中心点--屏幕坐标
			irr::core::vector2df m_center;
			//! 选择圆形半径
			f32         m_radius;
		};
	}
}

#endif
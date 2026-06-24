#ifndef __HD_FILTER_SELECT_POLY_H_INCLUDED__
#define __HD_FILTER_SELECT_POLY_H_INCLUDED__
#include "hdFilterSelection.h"
//#include "ogr_api.h"
#include "ogr_geometry.h"

class OGRGeometry;

namespace hd
{
	namespace ptcloud
	{
		class HDPOINTCLOUD_API hdFilterSelectPoly : public hdFilterSelection
		{
		public:
			// 构造三维场景选择器
			// screen_pts: 屏幕坐标点序列
			hdFilterSelectPoly(E_Select_Mode select_type, irr::core::vector2df* screen_pts, int pt_count, 
				irr::core::dimension2du view_size, f32* view_prj);

			// 构造灰度图选择器
			// angleCoords: 角度坐标点序列（单位：度）
			hdFilterSelectPoly(E_Select_Mode select_type, irr::core::vector2df* angleCoords, int pt_count);
			
			virtual ~hdFilterSelectPoly();

			//! 获取过滤器类型
			virtual E_Filter_Type getFilterType(){return E_FILTER_TYPE_SELECT_POLY;}

			//! 过滤处理，更新选中状态
			virtual void doFilter(hdVector<PointXYZIPRGBA>* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);

			//! 过滤处理，更新选中状态--处理所有过滤器
			virtual void doFilter(CHdParcelBase* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);

			//! 判断一个包围盒是否与选择区域相交，相交返回 true
			virtual bool isIntersects(CHdBox3df& box);

		private:
			//! 选择多边形
			OGRGeometry* m_geo_ring;
			OGRGeometry* m_geo_point;
		};
	}
}

#endif

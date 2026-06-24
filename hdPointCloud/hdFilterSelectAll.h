#ifndef __HD_FILTER_SELECT_ALL_H_INCLUDED__
#define __HD_FILTER_SELECT_ALL_H_INCLUDED__
#include "hdFilter.h"

namespace hd
{
	namespace ptcloud
	{
		class HDPOINTCLOUD_API hdFilterSelectAll : public hdFilter
		{
		public:
			hdFilterSelectAll();
			
			virtual ~hdFilterSelectAll();

			//! 获取过滤器类型
			virtual E_Filter_Type getFilterType(){return E_FILTER_TYPE_SELECT_ALL;}

			//! 过滤处理，更新选中状态--处理所有过滤器
			virtual void doFilter(hdVector<PointXYZIPRGBA>* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);

			//! 过滤处理，更新选中状态--处理所有过滤器
			virtual void doFilter(CHdParcelBase* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);
		};
	}
}

#endif

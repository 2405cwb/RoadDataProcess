#ifndef __HD_FILTER_HEIGHT_H_INCLUDED__
#define __HD_FILTER_HEIGHT_H_INCLUDED__
#include "hdFilter.h"

namespace hd
{
	namespace ptcloud
	{
		class HDPOINTCLOUD_API hdFilterHeight : public hdFilter
		{
		public:
			hdFilterHeight(int mode, float height, float height2);
			virtual ~hdFilterHeight();

			//! 获取过滤器类型
			virtual E_Filter_Type getFilterType(){return E_FILTER_TYPE_HEIGHT;}

			//! 过滤处理，更新选中状态
			virtual void doFilter(hdVector<PointXYZIPRGBA>* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);

		private:
			//! 过滤方式
			int   m_mode;
			//! 高度参数
			float m_height;
			float m_height2;
		};
	}
}

#endif
#ifndef __HD_FILTER_POS_DIST_H_INCLUDED__
#define __HD_FILTER_POS_DIST_H_INCLUDED__
#include "hdFilter.h"

namespace hd
{
	namespace ptcloud
	{
		class HDPOINTCLOUD_API hdFilterPosDist : public hdFilter
		{
		public:
			hdFilterPosDist(float fMinDis, float fMaxDis);
			virtual ~hdFilterPosDist();

			//! 获取过滤器类型
			virtual E_Filter_Type getFilterType(){return E_FILTER_TYPE_POS_DIST;}

			//! 过滤处理，更新选中状态
			virtual void doFilter(hdVector<PointXYZIPRGBA>* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);

			//! 过滤处理，更新选中状态
			virtual void doFilter(CHdParcelBase* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);

		private:

			float     m_fMinDis;
			float     m_fMaxDis;
		};
	}
}

#endif
#ifndef __HD_FILTER_MANAGER_H_INCLUDED__
#define __HD_FILTER_MANAGER_H_INCLUDED__
#include "hdFilter.h"
#include <vector>

using namespace std;

namespace hd
{
	namespace ptcloud
	{	
		/*! @class hdFilterManager hdFilterManager.h
		* @brief 定义过滤器管理类
		* @ingroup framework
		* @details hdFilterManager 过滤器管理类
		*/
		class HDPOINTCLOUD_API hdFilterManager
		{
		public:
			hdFilterManager();
			~hdFilterManager();

			//!添加过滤器
			void addFilter(hdFilter* filter);

			//!移除过滤器
			bool removeFilter(hdFilter* filter);

			//!移除过滤器--按类型
			bool removeFilter(E_Filter_Type filter_type);

			//!清空所有过滤器
			bool removeFilterAll();

			//!获取过滤器个数
			int getFilterCount(){return m_filter_list.size();}

			//!过滤处理--重新加载时
			void doFilter(hdVector<PointXYZIPRGBA>* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);

			//!过滤处理--重新加载时
			void doFilter(CHdParcelBase* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);
			
			//! 过滤处理--即刻刷新时
			void doFilterFresh(hdVector<PointXYZIPRGBA>* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);

			//! 过滤处理--即刻刷新时
			void doFilterFresh(CHdParcelBase* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans);

			//! 重置使处于未选择状态
			void reset(hdVector<PointXYZIPRGBA>* pt_array, CBursaWolfModel* render_trans);

			//! 重置使处于未选择状态
			void reset(CHdParcelBase* pt_array, CBursaWolfModel* render_trans);

			//! 判断一个包围盒是否与选择区域相交，相交返回 true
			bool isIntersects(CHdBox3df& box);

		private:
			//! 记录非选择型过滤器列表
			vector<hdFilter*> m_filter_list;
			//! 刷新模式：0-不刷新，1-叠加刷新，2-重置刷新
			int  m_refresh_mode;
			////!加锁
			//hdMutex m_thread_mutex;
		};
	}
}

#endif
#include "StdAfx.h"
#include "hdFilterManager.h"
#include "../hdCommon/point_types2.h"

namespace hd
{
	namespace ptcloud
	{
		hdFilterManager::hdFilterManager(void):m_refresh_mode(0)
		{
		}


		hdFilterManager::~hdFilterManager(void)
		{
			removeFilterAll();
		}

		//!添加过滤器
		void hdFilterManager::addFilter( hdFilter* filter )
		{
			if (filter == NULL)
			{
				return;
			}

			//hdLockGuard<hdMutex> lock_guard(m_thread_mutex);
			
			//判断选择模式：更新过滤器队列
			if (filter->getFilterType() == E_FILTER_TYPE_SELECT_ALL)
			{
				//全选则清空之前所有过滤器
				for (vector<hdFilter*>::iterator it=m_filter_list.begin(); it!=m_filter_list.end(); it++)
				{
					delete (*it);
				}
				m_filter_list.clear();
			}
			else if (filter->getFilterType() == E_FILTER_TYPE_SELECT_INVERSE)
			{
				//检查到之前连续两个反选则清除之，反选+反选=未选
				int count = m_filter_list.size();
				for (int i=1; i<count; i++)
				{
					if (m_filter_list[i]->getFilterType()==E_FILTER_TYPE_SELECT_INVERSE
						&&m_filter_list[i-1]->getFilterType()==E_FILTER_TYPE_SELECT_INVERSE)
					{
						delete m_filter_list[i];
						m_filter_list.erase(m_filter_list.begin()+i);
						delete m_filter_list[i-1];
						m_filter_list.erase(m_filter_list.begin()+i-1);
						break;
					}
				}
			}

			//添加新过滤器
			m_filter_list.push_back(filter);
			//记录刷新方式--叠加刷新
			m_refresh_mode = 1;
		}

		//!移除过滤器
		bool hdFilterManager::removeFilter( hdFilter* filter )
		{
			if (filter == NULL)
			{
				return false;
			}

			//hdLockGuard<hdMutex> lock_guard(m_thread_mutex);
			bool is_removed = false;

			//从后往前删除以适应队列变动
			int count = m_filter_list.size();
			for (int i=count-1; i>=0; i--)
			{
				if (m_filter_list[i] == filter)
				{
					delete m_filter_list[i];
					m_filter_list.erase(m_filter_list.begin()+i);
					//记录刷新方式--先重置再刷新
					m_refresh_mode = 2;
					is_removed = true;
					break;
				}
			}

			//移除成功则返回true
			return is_removed;
		}

		//!移除过滤器--按类型
		bool hdFilterManager::removeFilter( E_Filter_Type filter_type )
		{
			if (filter_type == E_FILTER_TYPE_UNKNOWN)
			{
				return false;
			}

			//hdLockGuard<hdMutex> lock_guard(m_thread_mutex);
			bool is_removed = false;

			//从后往前删除以适应队列变动
			int count = m_filter_list.size();
			for (int i=count-1; i>=0; i--)
			{
				if (m_filter_list[i]->getFilterType() == filter_type)
				{
					delete m_filter_list[i];
					m_filter_list.erase(m_filter_list.begin()+i);
					//记录刷新方式--先重置再刷新
					m_refresh_mode = 2;
					is_removed = true;
				}
			}

			//移除成功则返回true
			return is_removed;
		}


		//!清空所有过滤器
		bool hdFilterManager::removeFilterAll()
		{
			//hdLockGuard<hdMutex> lock_guard(m_thread_mutex);
			bool is_removed = false;

			//清空所有过滤器
			for (vector<hdFilter*>::iterator it=m_filter_list.begin(); it!=m_filter_list.end(); it++)
			{
				delete (*it);
			}
			m_filter_list.clear();

			//记录刷新方式--先重置再刷新
			m_refresh_mode = 2;
			is_removed = true;

			//移除成功则返回true
			return is_removed;
		}

		//!过滤处理--重新加载时
		void hdFilterManager::doFilter(hdVector<PointXYZIPRGBA>* pt_array,
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
		{
			//hdLockGuard<hdMutex> lock_guard(m_thread_mutex);

			//过滤器
			for (vector<hdFilter*>::iterator it=m_filter_list.begin(); it!=m_filter_list.end(); it++)
			{
				(*it)->doFilter(pt_array, render_trans, pcd_trans);
			}
		}
		
		//!过滤处理--重新加载时
		void hdFilterManager::doFilter(CHdParcelBase* pt_array,
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
		{
			//hdLockGuard<hdMutex> lock_guard(m_thread_mutex);

			//过滤器
			for (vector<hdFilter*>::iterator it=m_filter_list.begin(); it!=m_filter_list.end(); it++)
			{
				(*it)->doFilter(pt_array, render_trans, pcd_trans);
			}
		}

		//! 过滤处理--即刻刷新时
		void hdFilterManager::doFilterFresh(hdVector<PointXYZIPRGBA>* pt_array,
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
		{
			//hdLockGuard<hdMutex> lock_guard(m_thread_mutex);

			//过滤器个数
			int count = m_filter_list.size();

			//叠加刷新
			if (m_refresh_mode==1)
			{
				if (count > 0)
				{
					m_filter_list[count-1]->doFilter(pt_array, render_trans, pcd_trans);
				}
			}
			else if (m_refresh_mode==2)
			{
				//先重置选择状态
				reset(pt_array, render_trans);
				//再依次过滤
				for (int i=0; i<count; i++)
				{
					m_filter_list[i]->doFilter(pt_array, render_trans, pcd_trans);
				}
			}
		}

		//! 过滤处理--即刻刷新时
		void hdFilterManager::doFilterFresh(CHdParcelBase* pt_array,
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
		{
			//hdLockGuard<hdMutex> lock_guard(m_thread_mutex);

			//过滤器个数
			int count = m_filter_list.size();

			//叠加刷新
			if (m_refresh_mode==1)
			{
				if (count > 0)
				{
					m_filter_list[count-1]->doFilter(pt_array, render_trans, pcd_trans);
				}
			}
			else if (m_refresh_mode==2)
			{
				//先重置选择状态
				reset(pt_array, render_trans);
				//再依次过滤
				for (int i=0; i<count; i++)
				{
					m_filter_list[i]->doFilter(pt_array, render_trans, pcd_trans);
				}
			}
		}

		//! 重置使处于未选择状态
		void hdFilterManager::reset(hdVector<PointXYZIPRGBA>* pt_array, CBursaWolfModel* render_trans)
		{
			if (!pt_array)
			{
				return;
			}

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
				// 取消选择
				pt.setUnSelected();
			}
		}

		//! 重置使处于未选择状态
		void hdFilterManager::reset(CHdParcelBase* pt_array, CBursaWolfModel* render_trans)
		{
			if (!pt_array || !pt_array->m_pHlzPoint)
			{
				return;
			}

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
				// 取消选择
				pt.setUnSelected();
			}
		}

		bool hdFilterManager::isIntersects(CHdBox3df& box)
		{
			return true;
		}
	}
}
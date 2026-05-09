#ifndef __HD_FILTER_H_INCLUDED__
#define __HD_FILTER_H_INCLUDED__

#include "../hdCore/hdBlkArray.h"
#include "../hdCommon/point_types.h"
#include "../hdCommon/BursaWolfModel.h"
#include "hdPointCloud.h"
#include "../hdHlslib/HdParcelBase.h"

namespace hd
{
	namespace ptcloud
	{	
		enum E_Filter_Type
		{
			//! 未知类型
			E_FILTER_TYPE_UNKNOWN = 0,

			//! 全选
			E_FILTER_TYPE_SELECT_ALL,
			//! 反选
			E_FILTER_TYPE_SELECT_INVERSE,

			//! 选择多边形
			E_FILTER_TYPE_SELECT_POLY,
			//! 选择矩形
			E_FILTER_TYPE_SELECT_RECT,
			//! 选择圆形
			E_FILTER_TYPE_SELECT_CIRCLE,

			//! 高度过滤
			E_FILTER_TYPE_HEIGHT,
			//! POS高度过滤
			E_FILTER_TYPE_POS_HEIGHT,
			//! POS距离过滤
			E_FILTER_TYPE_POS_DIST,
			//! 统计过滤
			E_FILTER_TYPE_STATIC
		};

		/*! @class hdFilter hdFilter.h
		* @brief 定义过滤器基类
		* @ingroup framework
		* @details hdFilter 过滤器基类
		*/
		class HDPOINTCLOUD_API hdFilter
		{
		private:
			//! 禁止拷贝
			hdFilter(const hdFilter &filter)
			{
			}

			//! 禁止复制
			const hdFilter& operator=(const hdFilter& other)
			{
				return *this;
			}

		public:
			//! 构造函数
			hdFilter(){}

			//! 析构函数
			virtual ~hdFilter(){};

			//! 获取过滤器类型
			virtual E_Filter_Type getFilterType() = 0;
			
			//! 过滤处理，更新选中状态--处理所有过滤器
			virtual void doFilter(hdVector<PointXYZIPRGBA>* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans) = 0;

			//! 过滤处理，更新选中状态--处理所有过滤器
			virtual void doFilter(CHdParcelBase* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans) = 0;
		};
	}
}

#endif
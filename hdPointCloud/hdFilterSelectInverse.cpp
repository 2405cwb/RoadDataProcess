#include "StdAfx.h"
#include "hdFilterSelectInverse.h"
#include "../hdCommon/point_types2.h"

namespace hd
{
	namespace ptcloud
	{
		hdFilterSelectInverse::hdFilterSelectInverse()
		{
		}

		hdFilterSelectInverse::~hdFilterSelectInverse()
		{
		}

		//! 过滤处理，更新选中状态
		void hdFilterSelectInverse::doFilter(hdVector<PointXYZIPRGBA>* pt_array,
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
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

				// 反选设置选择标记
				pt.setXorSelected();
			}
		}

		//! 过滤处理，更新选中状态
		void hdFilterSelectInverse::doFilter(CHdParcelBase* pt_array,
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
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

				// 反选设置选择标记
				pt.setXorSelected();
			}
		}
	}
}
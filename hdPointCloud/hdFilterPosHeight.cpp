#include "StdAfx.h"
#include "hdFilterPosHeight.h"


namespace hd
{
	namespace ptcloud
	{
		hdFilterPosHeight::hdFilterPosHeight(int mode, float height, float height2)
		{
			m_mode = mode;
			m_height = height;
			m_height2 = height2;
		}


		hdFilterPosHeight::~hdFilterPosHeight()
		{
		}

		void hdFilterPosHeight::doFilter(hdVector<PointXYZIPRGBA>* pt_array, 
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
		{
			//遍历块中所有点
			u32 count_in_block = pt_array->size();
			for (u32 pt_index = 0; pt_index < count_in_block; ++pt_index)
			{
				// 获得每个点
				PointXYZIPRGBA& pt = (*pt_array)[pt_index];					
				if (!pt.isValid())
				{
					continue;
				}

				// 更新选中状态
				if (m_mode == 0)//between
				{
					if (pt.z > m_height && pt.z < m_height2)
					{
						pt.setSelected();
					}
				}
				else if (m_mode == 1)//above
				{
					if (pt.z > m_height)
					{
						pt.setSelected();
					}
				}
				else if (m_mode == 2)//below
				{
					if (pt.z < m_height)
					{
						pt.setSelected();
					}
				}
			}
		}
	}
}
#include "StdAfx.h"
#include "hdFilterHeight.h"


namespace hd
{
	namespace ptcloud
	{
		hdFilterHeight::hdFilterHeight(int mode, float height, float height2)
			:m_mode(mode), m_height(height), m_height2(height2)
		{
			if (m_mode==0)//between
			{
				if (height2 < height)
				{
					float tmp = height;
					height = height2;
					height2 = tmp;
				}
			}
		}


		hdFilterHeight::~hdFilterHeight()
		{
		}

		//! 过滤处理，更新选中状态
		void hdFilterHeight::doFilter(hdVector<PointXYZIPRGBA>* pt_array,
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
		{
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

				// 转换绝对坐标
				float fx=pt.x, fy=pt.y, fz=pt.z;
				if (pcd_trans)
				{
					pcd_trans->Translate(fx, fy, fz);
				}				

				// 更新选中状态
				if (m_mode == 0)//between
				{
					if (fz>m_height && fz<m_height2)
					{
						pt.setSelected();
					}
				}
				else if (m_mode == 1)//above
				{
					if (fz > m_height)
					{
						pt.setSelected();
					}
				}
				else if (m_mode == 2)//below
				{
					if (fz < m_height)
					{
						pt.setSelected();
					}
				}
			}
		}
	}
}
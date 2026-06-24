#include "StdAfx.h"
#include "hdFilterPosDist.h"

namespace hd
{
	namespace ptcloud
	{
		hdFilterPosDist::hdFilterPosDist(float fMinDis, float fMaxDis): m_fMinDis(fMinDis), m_fMaxDis(fMaxDis)
		{
		}


		hdFilterPosDist::~hdFilterPosDist()
		{
		}

		void hdFilterPosDist::doFilter(hdVector<PointXYZIPRGBA>* pt_array, 
			CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
		{
			float fMaxDis_2 = m_fMaxDis * m_fMaxDis;
			float fMinDis_2 = m_fMinDis * m_fMinDis;

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

				float fDis_2 = pt.x * pt.x + pt.y * pt.y + pt.z * pt.z;
				if (fDis_2 <= fMaxDis_2 && fDis_2 >= fMinDis_2)
				{
					pt.setSelected();
				}
				else
				{
					pt.setUnSelected();
				}
			}
		}

		void hdFilterPosDist::doFilter(CHdParcelBase* pt_array, CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans)
		{

		}
	}
}
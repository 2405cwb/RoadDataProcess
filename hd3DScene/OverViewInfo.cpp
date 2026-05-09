#include "stdafx.h"
#include "OverViewInfo.h"
#include <algorithm>

using namespace hd::scene;

COverViewInfo::COverViewInfo(void)
	:m_pImageInfo(NULL)
	,m_bIsUsed(false)
	,m_nUseLevel(0)
{
}


COverViewInfo::~COverViewInfo(void)
{
	int nSize = m_pLevelList.size();
	
	for (int i=0;i<nSize;++i)
	{
		OVERVIEWLEVEL* pLevel = m_pLevelList.at(i);
		if (pLevel)
		{
			delete pLevel;
			pLevel = NULL;
		}
	}

	m_pLevelList.clear();
}

// 添加级别
void COverViewInfo::SetLevel(const vector<OVERVIEWLEVEL*>& vectLevels)
{
	// 设置
	m_pLevelList = vectLevels;
}

// 通过索引获取
OVERVIEWLEVEL* COverViewInfo::GetLevel(int nLevelIndex) const
{
	if (m_pLevelList.empty())
	{
		return NULL;
	}
	else
	{
		// 防止无效值
		int nSize = m_pLevelList.size();
		if (nLevelIndex<0 || nLevelIndex>=nSize)
		{
			return NULL;
		}

		return m_pLevelList.at(nLevelIndex);
	}
}
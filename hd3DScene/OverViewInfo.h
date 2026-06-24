#pragma once
#include "stdafx.h"
#include "BaseStruct.h"
#include <vector>
using namespace std;

namespace hd
{
	namespace scene
	{
	class HD3DSCENE_API COverViewInfo
	{
	public:
		COverViewInfo(void);
		~COverViewInfo(void);

		// 返回影像基本信息
		ImageExtInfoSPtr GetImageInfo() const
		{
			return m_pImageInfo;
		}

		// 设置信息
		void SetImageInfo(const ImageExtInfoSPtr& imageinfo)
		{
			m_pImageInfo = imageinfo;
		}

		// 获取当前显示级别
		int GetUseLevel() const
		{
			return m_nUseLevel;
		}

		// 设置当前所用级别
		void SetUseLevel(int nUseLevel)
		{
			m_nUseLevel= nUseLevel;
		}

		// 得到级别总数
		int GetLevelCount() const
		{
			return m_pLevelList.size();
		}

		// 添加级别
		void SetLevel(const vector<OVERVIEWLEVEL*>& vectLevels);

		// 通过索引获取
		OVERVIEWLEVEL* GetLevel(int nLevelIndex) const;

		// 设置是否可用
		void SetUsed(bool bUsed)
		{
			m_bIsUsed = bUsed;
		}

		// 当前是否可用
		bool GetUsed() const
		{
			return m_bIsUsed;
		}
	private:
		ImageExtInfoSPtr	m_pImageInfo;		// 影像信息 
		int m_nUseLevel;						// 当前显示所用级别
		vector<OVERVIEWLEVEL*> m_pLevelList;	// 级别列表
		bool m_bIsUsed;							// 是否在用
	};
	}
}

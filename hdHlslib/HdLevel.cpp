#include "HdLevel.h"
#include "HdBlockset.h"
#include "HdBlock.h"
#include "HdParcel.h"
#include "HlzDefs.h"
#include "..\hdCommon\point_types2.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{

CHdLevel::CHdLevel(void)
{
	
}

CHdLevel::~CHdLevel(void)
{
	Clear();
}

// 清除对象
void CHdLevel::Clear()
{
	map<I32,CHdBlockset*>::iterator it;
	for (it = m_pListBlockset.begin();it != m_pListBlockset.end();++it)
	{
		if(it->second)
		{
			delete it->second;
			it->second = NULL;
		}
	}
	m_pListBlockset.clear();
	m_level.numBlockset = 0;
	m_level.pointNum = 0;
}

u64 CHdLevel::GetPtCount()
{
	return m_level.pointNum;
}

// 更新内部点数
void CHdLevel::Update()
{
	if (m_pListBlockset.size() > 0)
	{
		m_level.pointNum = 0;
	}
	
	map<I32,CHdBlockset*>::iterator it;
	for (it = m_pListBlockset.begin();it != m_pListBlockset.end();++it)
	{
		m_level.pointNum += it->second->m_blockSet.numPoint;
	}
	m_level.numBlockset = m_pListBlockset.size();
}

// 更据单位面积点数,更新计算比例尺
void CHdLevel::UpdateScale()
{
	return;
	// 根据叶子节点数据包面积和点数估算显示比例尺		
	F64 dArea = 0.0;		// 面积
	U64 countInArea = 0;	// 点数
	U32 ignoreBS = 512;    // 小于512点的块集不统计
	U32 ignoreBK = 128;	   // 小于128点的包不统计
	U32 ignorePcl = 32;	   // 小于32点的包不统计
	CHdVector3df ext;
	map<I32,CHdBlockset*>::iterator it;
	for (it = m_pListBlockset.begin();it != m_pListBlockset.end();it++)
	{
		CHdBlockset* pHdBlockset = it->second;

		// 如果当前块集不包含子块,则计算其范围,
		if (!pHdBlockset->m_blockSet.hasSubBlock)
		{
			if(pHdBlockset->m_blockSet.numPoint > ignoreBS)
			{
				countInArea += pHdBlockset->m_blockSet.numPoint;
				ext = pHdBlockset->m_blockSet.box.getExtent();
				dArea += (ext.X * ext.Y);
			}
			continue;
		}

		map<I32,CHdBlock*>::iterator itBlk;
		for (itBlk = pHdBlockset->m_pListBlock.begin();itBlk != pHdBlockset->m_pListBlock.end();itBlk++)
		{
			CHdBlock* pBlock = itBlk->second;
				
			// 如果当前块集不包含子块,则计算其范围,
			if (!pBlock->m_block.hasSubParcel)
			{
				if(pBlock->m_block.numPoint > ignoreBK)
				{
					countInArea += pBlock->m_block.numPoint;
					ext = pBlock->m_block.box.getExtent();
					dArea += (ext.X * ext.Y);
				}
				continue;
			}

			map<I32,CHdParcel*>::iterator itPcl;
			for (itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
			{
				CHdParcel* pParcel = itPcl->second;
				if(pParcel->m_parcel.numPoint > ignorePcl)
				{
					countInArea += pParcel->m_parcel.numPoint;
					ext = pParcel->m_parcel.box.getExtent();
					dArea += (ext.X * ext.Y);
				}
			}
		}
	}

	// 根据范围面积和点数估算比例尺 100万/平方米 1/500
	F64 adjCount = countInArea / dArea;
	
	m_level.dispScale =  sqrt(1000000 / adjCount) * 500;
}

BOOL CHdLevel::AddBlockSetRec( CHdBlockset* pBlockSetRec )
{
	m_pListBlockset.insert(HdBlockSetPair(pBlockSetRec->GetIndex(),pBlockSetRec));
	return TRUE;
}

// 添加块集记录
BOOL CHdLevel::AddBlockSetRec(I32 nBlockSetNo,CHdBlockset* pBlockSetRec)
{
	map<I32,CHdBlockset*>::iterator it = m_pListBlockset.find(nBlockSetNo);
	if (it != m_pListBlockset.end())
	{
		return FALSE;
	}
	m_pListBlockset.insert(HdBlockSetPair(nBlockSetNo,pBlockSetRec));
	return TRUE;
}

CHdBlockset* CHdLevel::GetBlockSetRec( I32 nBlockSetNo )
{
	map<I32,CHdBlockset*>::iterator it = m_pListBlockset.find(nBlockSetNo);
	if (it != m_pListBlockset.end())
	{
		return it->second;
	}
	else
	{
		return NULL;
	}	 
}


CHdBox3df CHdLevel::GetExtent()
{
	// 初始化box
	CHdBox3df boxTmp;
	boxTmp.MaxEdge.set(F32_MIN,F32_MIN,F32_MIN);
	boxTmp.MinEdge.set(F32_MAX,F32_MAX,F32_MAX);

	map<I32,CHdBlockset*>::iterator it;
	for (it = m_pListBlockset.begin();it != m_pListBlockset.end();++it)
	{
		CHdBlockset* pHdBlockSet = it->second;

		if (pHdBlockSet)
		{
			boxTmp.addInternalBox(pHdBlockSet->GetExtent());

		}
		
	}

	return boxTmp;

}

}
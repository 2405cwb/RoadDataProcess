#include "HdBlockset.h"
#include "HdBlock.h"
#include "HlzDefs.h"
#include "..\hdCommon\point_types2.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{

	CHdBlockset::~CHdBlockset()
	{
		Clear();
	}

	// 清除对象
	void CHdBlockset::Clear()
	{		
		map<I32,CHdBlock*>::iterator it;
		for (it = m_pListBlock.begin();it != m_pListBlock.end();++it)
		{
			if(it->second)
			{
				delete it->second;
				it->second = NULL;
			}
		}
		m_pListBlock.clear();
		m_blockSet.numBlock = 0;
		m_blockSet.numPoint = 0;
	}

	// 更新内部点数
	void CHdBlockset::Update()
	{
		if (m_pListBlock.size() > 0)
		{
			m_blockSet.numPoint = 0;
		}
		map<I32,CHdBlock*>::iterator it;
		for (it = m_pListBlock.begin();it != m_pListBlock.end();++it)
		{
			m_blockSet.numPoint += it->second->GetPtCount();
		}
		m_blockSet.numBlock = m_pListBlock.size();
	}

	BOOL CHdBlockset::AddBlockRec( CHdBlock* pBlockRec )
	{
		m_pListBlock.insert(HdBlockPair(pBlockRec->GetIndex(),pBlockRec));
		return TRUE;
	}

	CHdBlock* CHdBlockset::GetBlockRec( I32 nBlockSetNo )
	{
		map<I32,CHdBlock*>::iterator it = m_pListBlock.find(nBlockSetNo);
		if (it != m_pListBlock.end())
		{
			return it->second;
		}
		else
		{
			return NULL;
		}	 
	}

	I32 CHdBlockset::GetIndex()
	{		
		//return GridNo2Index(m_blockSet.numBlockX,m_blockSet.numBlockY,m_blockSet.numBlockZ,
		//	m_xNo,m_yNo,m_zNo);	

		return m_nBlockSetNo;
	}

	int CHdBlockset::GetPtCount()
	{
		return m_blockSet.numPoint;
	}

	hd::HdAddr CHdBlockset::GetCoordAddr() const
	{
		return m_blockSet.addrCoord;
	}

	hd::HdAddr CHdBlockset::GetIntensityAddr()
	{
		return m_blockSet.addrIntensity;
	}

	hd::HdAddr CHdBlockset::GetTimeAddr()
	{
		return m_blockSet.addrTime;
	}

	hd::HdAddr CHdBlockset::GetColorAddr()
	{
		return m_blockSet.addrColor;
	}

	hd::HdAddr CHdBlockset::GetClassAddr()
	{
		return m_blockSet.addrClass;
	}

	bool CHdBlockset::IsCompress()
	{
		return m_blockSet.cmpSize > 0;
	}

	void CHdBlockset::SetUnCompress()
	{
		m_blockSet.cmpSize = 0;
	}

	U32 CHdBlockset::GetCmpCoordLen()
	{
		return m_blockSet.cmpSize;
	}

	U32 CHdBlockset::GetCmpIntenLen()
	{
		return 0;
	}

	// 外部设置坐标文件地址
	void CHdBlockset::SetCoordAddr( U64 addrs )
	{
		m_blockSet.addrCoord.Prase(addrs);
	}

	// 外部设置强度地址
	void CHdBlockset::SetIntensityAddr( U64 addrs )
	{
		m_blockSet.addrIntensity.Prase(addrs);
	}

	// 外部设置颜色数据地址   袁亮  20160625
	void CHdBlockset::SetColorAddr( U64 addrs )
	{
		m_blockSet.addrColor.Prase(addrs);
	}
}

#include "HdBlock.h"
#include "HdParcel.h"
#include "HlzDefs.h"
#include "..\hdCommon\point_types2.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{

	CHdBlock::~CHdBlock()
	{
		Clear();
	}
	// 清除对象
	void CHdBlock::Clear()
	{		
		map<I32,CHdParcel*>::iterator it;
		for (it = m_pListParcel.begin();it != m_pListParcel.end();++it)
		{
			if(it->second)
			{
				delete it->second;
				it->second = NULL;
			}
		}
		m_pListParcel.clear();
		m_block.numParcel = 0;
		m_block.numPoint = 0;
	}

	// 更新内部点数
	void CHdBlock::Update()
	{
		if (m_pListParcel.size() > 0)
		{
			m_block.numPoint = 0;
		}
		
		map<I32,CHdParcel*>::iterator it;
		for (it = m_pListParcel.begin();it != m_pListParcel.end();++it)
		{
			m_block.numPoint += it->second->GetPtCount();
		}
		m_block.numParcel = m_pListParcel.size();
	}

	BOOL CHdBlock::AddParcel(CHdParcel* pParcelRec )
	{
		m_pListParcel.insert(HdParcelPair(pParcelRec->GetIndex(),pParcelRec));
		return TRUE;
	}

	CHdParcel* CHdBlock::GetParcel( I32 nParcelNo )
	{
		map<I32,CHdParcel*>::iterator it = m_pListParcel.find(nParcelNo);
		if (it != m_pListParcel.end())
		{
			return it->second;
		}
		else
		{
			return NULL;
		}	 
	}

	I32 CHdBlock::GetIndex()
	{
		//return GridNo2Index(m_block.numParcelX,m_block.numParcelY,m_block.numParcelZ,
		//	m_xNo,m_yNo,m_zNo);

		return m_nBlockNo;
	}

	int CHdBlock::GetPtCount()
	{
		return m_block.numPoint;
	}

	hd::HdAddr CHdBlock::GetCoordAddr() const
	{
		return m_block.addrCoord;
	}

	hd::HdAddr CHdBlock::GetIntensityAddr()
	{
		return m_block.addrIntensity;
	}

	hd::HdAddr CHdBlock::GetTimeAddr()
	{
		return m_block.addrTime;
	}

	hd::HdAddr CHdBlock::GetColorAddr() 
	{
		return m_block.addrColor;
	}

	hd::HdAddr CHdBlock::GetClassAddr()
	{
		return m_block.addrClass;
	}

	bool CHdBlock::IsCompress()
	{
		return m_block.cmpSize > 0;
	}

	void CHdBlock::SetUnCompress()
	{
		m_block.cmpSize = 0;
	}

	U32 CHdBlock::GetCmpCoordLen()
	{
		return m_block.cmpSize;
	}

	U32 CHdBlock::GetCmpIntenLen()
	{
		return 0;
	}

	// 外部设置坐标数据地址
	void CHdBlock::SetCoordAddr( U64 addrs )
	{
		m_block.addrCoord.Prase(addrs);
	}

	// 外部设置强度数据地址
	void CHdBlock::SetIntensityAddr( U64 addrs )
	{
		m_block.addrIntensity.Prase(addrs);
	}

	// 外部设置颜色数据地址   袁亮  20160625
	void CHdBlock::SetColorAddr( U64 addrs )
	{
		m_block.addrColor.Prase(addrs);
	}
}
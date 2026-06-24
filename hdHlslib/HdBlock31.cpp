#include "HdBlock31.h"
#include "HdParcel31.h"
#include "..\hdCommon\point_types2.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	CHdBlock31::~CHdBlock31()
	{
	}

	// 更新内部点数
	void CHdBlock31::Update()
	{
		if (m_pListParcel.size() > 0)
		{
			m_block31.numPoint = 0;
		}

		map<I32,CHdParcel*>::iterator it;
		for (it = m_pListParcel.begin();it != m_pListParcel.end();++it)
		{
			m_block31.numPoint += it->second->GetPtCount();
		}
		m_block31.numParcel = m_pListParcel.size();
		//保持hlz 3.0块索引与hlz 3.1块索引基本信息一直，达到兼容的目的
		m_block.hasSubParcel = m_pListParcel.empty() ? 0 : 1;
		m_block.numParcel = m_block31.numParcel;
		m_block.numPoint = m_block31.numPoint;
	}

	int CHdBlock31::GetPtCount()
	{
		return m_block31.numPoint;
	}

	hd::HdAddr CHdBlock31::GetCoordAddr() const
	{
		return m_block31.addrBaseAttri;
	}

	hd::HdAddr CHdBlock31::GetIntensityAddr()
	{
		U32 coordLen = IsCompress() ? GetCmpCoordLen() : GetPtCount() * sizeof(HdPointXYZ);
		U64 intensityAddr = m_block31.addrBaseAttri.GetAddress() + coordLen;

		HdAddr addrInten;
		addrInten.Prase(intensityAddr);
		return addrInten;
	}

	hd::HdAddr CHdBlock31::GetTimeAddr()
	{
		return m_block31.addrTime;
	}

	hd::HdAddr CHdBlock31::GetColorAddr() 
	{
		return m_block31.addrColor;
	}

	hd::HdAddr CHdBlock31::GetClassAddr()
	{
		U64 classAddr = GetIntensityAddr().GetAddress();
		classAddr += IsCompress()? GetCmpIntenLen() : GetPtCount() * sizeof(HdIntensity);

		HdAddr addrClass;
		addrClass.Prase(classAddr);
		return addrClass;
	}

	bool CHdBlock31::IsCompress()
	{
		return m_block31.isCompress();
	}

	void CHdBlock31::SetUnCompress()
	{
		m_block31.setCompress(false);
	}

	U32 CHdBlock31::GetCmpCoordLen()
	{
		return m_block31.getCoordLen();
	}

	U32 CHdBlock31::GetCmpIntenLen()
	{
		return m_block31.getIntenLen();
	}

	// 外部设置坐标数据地址
	void CHdBlock31::SetCoordAddr( U64 addrs )
	{
		//m_block.addrCoord.Prase(addrs);
		m_block31.addrBaseAttri.Prase(addrs);
	}

	// 外部设置强度数据地址
	void CHdBlock31::SetIntensityAddr( U64 addrs )
	{
		//m_block.addrIntensity.Prase(addrs);
	}

	// 外部设置颜色数据地址   袁亮  20160625
	void CHdBlock31::SetColorAddr( U64 addrs )
	{
		//m_block.addrColor.Prase(addrs);
		m_block31.addrColor.Prase(addrs);
	}
}



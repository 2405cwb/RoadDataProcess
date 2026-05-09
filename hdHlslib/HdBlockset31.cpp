#include "HdBlockset31.h"
#include "HdBlock31.h"
#include "..\hdCommon\point_types2.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	CHdBlockset31::~CHdBlockset31(void)
	{
	}

	// 更新内部点数
	void CHdBlockset31::Update()
	{
		if (!m_pListBlock.empty())
		{
			m_blockSet31.numPoint = 0;
		}

		for (auto it = m_pListBlock.begin();it != m_pListBlock.end();++it)
		{
			m_blockSet31.numPoint += it->second->GetPtCount();
		}

		m_blockSet31.setBlockNum(m_pListBlock.size());
		//保持hlz 3.0块索引与hlz 3.1块集索引基本信息一直，达到兼容的目的
		m_blockSet.hasSubBlock = m_pListBlock.empty() ? 0 : 1;
		m_blockSet.numBlock = m_blockSet31.getBlockNum();
		m_blockSet.numPoint = m_blockSet31.numPoint;
	}

	int CHdBlockset31::GetPtCount()
	{
		return m_blockSet31.numPoint;
	}

	hd::HdAddr CHdBlockset31::GetCoordAddr() const
	{
		return m_blockSet31.addrBaseAttri;
	}

	hd::HdAddr CHdBlockset31::GetIntensityAddr()
	{
		U32 coordLen = IsCompress() ? GetCmpCoordLen() : GetPtCount()* sizeof(HdPointXYZ);
		U64 intensityAddr = m_blockSet31.addrBaseAttri.GetAddress() +  coordLen;

		HdAddr addrInten;
		addrInten.Prase(intensityAddr);
		return addrInten;
	}

	hd::HdAddr CHdBlockset31::GetTimeAddr()
	{
		return m_blockSet31.addrTime;
	}

	hd::HdAddr CHdBlockset31::GetColorAddr()
	{
		return m_blockSet31.addrColor;
	}

	hd::HdAddr CHdBlockset31::GetClassAddr()
	{
		U64 classAddr = GetIntensityAddr().GetAddress();
		classAddr += IsCompress() ?  GetCmpIntenLen() : GetPtCount() * sizeof(HdIntensity);

		HdAddr addrClass;
		addrClass.Prase(classAddr);
		return addrClass;
	}

	bool CHdBlockset31::IsCompress()
	{
		return m_blockSet31.isCompress();
	}

	void CHdBlockset31::SetUnCompress()
	{
		m_blockSet31.setCompress(false);
	}

	U32 CHdBlockset31::GetCmpCoordLen()
	{
		return m_blockSet31.getCoordLen();
	}

	U32 CHdBlockset31::GetCmpIntenLen()
	{
		return m_blockSet31.getIntenLen();
	}

	// 外部设置坐标文件地址
	void CHdBlockset31::SetCoordAddr( U64 addrs )
	{
		//m_blockSet.addrCoord.Prase(addrs);
		m_blockSet31.addrBaseAttri.Prase(addrs);
	}

	// 外部设置强度地址
	void CHdBlockset31::SetIntensityAddr( U64 addrs )
	{
		//m_blockSet.addrIntensity.Prase(addrs);
	}

	// 外部设置颜色数据地址   袁亮  20160625
	void CHdBlockset31::SetColorAddr( U64 addrs )
	{
		//m_blockSet.addrColor.Prase(addrs);
		m_blockSet31.addrColor.Prase(addrs);
	}
}

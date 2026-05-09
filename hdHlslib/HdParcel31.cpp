#include "HdParcel31.h"
#include "..\hdCommon\point_types2.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	int CHdParcel31::GetPtCount()
	{
		return m_parcel31.numPoint;
	}

	bool CHdParcel31::IsCompress()
	{
		return m_parcel31.isCompress();
	}

	void CHdParcel31::SetUnCompress()
	{
		m_parcel31.setCompress(false);
	}

	U32 CHdParcel31::GetCmpCoordLen()
	{
		return m_parcel31.getCoordLen();
	}

	U32 CHdParcel31::GetCmpIntenLen()
	{
		return m_parcel31.getIntenLen();
	}

	hd::HdAddr CHdParcel31::GetCoordAddr() const
	{
		return m_parcel31.addrBaseAttri;
	}

	hd::HdAddr CHdParcel31::GetIntensityAddr()
	{
		U32 coordLen = IsCompress() ? GetCmpCoordLen() : GetPtCount() * sizeof(HdPointXYZ);
		U64 intensityAddr = m_parcel31.addrBaseAttri.GetAddress()  + coordLen;
		
		HdAddr addrInten;
		addrInten.Prase(intensityAddr);
		return addrInten;
	}

	hd::HdAddr CHdParcel31::GetTimeAddr()
	{
		return m_parcel31.addrTime;
	}

	hd::HdAddr CHdParcel31::GetColorAddr()
	{
		return m_parcel31.addrColor;
	}

	hd::HdAddr CHdParcel31::GetClassAddr()
	{
		U64 classAddr = GetIntensityAddr().GetAddress();
		classAddr += IsCompress() ? GetCmpIntenLen() : GetPtCount() * sizeof(HdIntensity);

		HdAddr addrClass;
		addrClass.Prase(classAddr);
		return addrClass;
	}

	// 外部设置坐标文件地址
	void CHdParcel31::SetCoordAddr( U64 addrs )
	{
		m_parcel31.addrBaseAttri.Prase(addrs);
	}

	// 外部设置强度地址
	void CHdParcel31::SetIntensityAddr( U64 addrs )
	{
		//m_parcel.addrIntensity.Prase(addrs);
	}

	// 外部设置颜色数据地址   袁亮  20160625
	void CHdParcel31::SetColorAddr( U64 addrs )
	{
		m_parcel31.addrColor.Prase(addrs);
	}
}
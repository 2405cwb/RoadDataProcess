#include "HdParcel.h"
#include "HlzDefs.h"
#include "..\hdCommon\point_types2.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{

	CHdParcel::~CHdParcel()
	{
		
	}

	// 清除对象
	void CHdParcel::Clear()
	{
		
	}

	int CHdParcel::GetPtCount()
	{
		return m_parcel.numPoint;
	}

	hd::HdAddr CHdParcel::GetCoordAddr() const
	{
		return m_parcel.addrCoord;
	}

	hd::HdAddr CHdParcel::GetIntensityAddr()
	{
		return m_parcel.addrIntensity;
	}

	hd::HdAddr CHdParcel::GetTimeAddr()
	{
		return m_parcel.addrTime;
	}

	hd::HdAddr CHdParcel::GetColorAddr()
	{
		return m_parcel.addrColor;
	}

	hd::HdAddr CHdParcel::GetClassAddr()
	{
		return m_parcel.addrClass;
	}

	bool CHdParcel::IsCompress()
	{
		return m_parcel.cmpSize > 0;
	}

	void CHdParcel::SetUnCompress()
	{
		m_parcel.cmpSize = 0;
	}

	U32 CHdParcel::GetCmpCoordLen()
	{
		return m_parcel.cmpSize;
	}

	U32 CHdParcel::GetCmpIntenLen()
	{
		return 0;
	}

	// 外部设置坐标文件地址
	void CHdParcel::SetCoordAddr( U64 addrs )
	{
		m_parcel.addrCoord.Prase(addrs);
	}

	// 外部设置强度地址
	void CHdParcel::SetIntensityAddr( U64 addrs )
	{
		m_parcel.addrIntensity.Prase(addrs);
	}

	// 外部设置颜色数据地址   袁亮  20160625
	void CHdParcel::SetColorAddr( U64 addrs )
	{
		m_parcel.addrColor.Prase(addrs);
	}
}
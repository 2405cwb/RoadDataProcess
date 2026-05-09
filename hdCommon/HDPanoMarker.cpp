#include "StdAfx.h"
#include "HDPanoMarker.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
namespace hd
{
	CHDPanoMarker::CHDPanoMarker(void)
	{
		memset(m_strName, 0, NAME_LEN);
		memset(m_strSymbolID, 0, OBJECT_ID_LEN);
		m_dScaleX = 1.0;
		m_dScaleY = 1.0;
		m_dScaleZ = 1.0;
	}

	CHDPanoMarker::CHDPanoMarker(const char* strName, const char* strSymbolID,  CHD3DNormalPoint& normalPt, double dScaleX, double dScaleY, double dScaleZ)
	{
		strcpy(m_strName, strName);
		strcpy(m_strSymbolID, strSymbolID);
		m_Pos = normalPt;
		m_dScaleX = dScaleX;
		m_dScaleY = dScaleY;
		m_dScaleZ = dScaleZ;
	}

	CHDPanoMarker::~CHDPanoMarker(void)
	{
	}

	CHD3DBoundingBox CHDPanoMarker::GetBoundingBox() const
	{
		return m_Pos.GetBoundingBox();
	}

	bool CHDPanoMarker::IsInSphere(double dX, double dY, double dZ, double dR) const
	{
		return m_Pos.IsInSphere(dX, dY, dZ, dR);
	}

	CHD3DNormalPoint CHDPanoMarker::GetPosition() const
	{
		return m_Pos;
	}

	void CHDPanoMarker::SetPosition(CHD3DNormalPoint& normalPt)
	{
		m_Pos = normalPt;
	}

	const char* CHDPanoMarker::GetName() const
	{
		return m_strName;
	}

	void CHDPanoMarker::SetName(const char* strName)
	{
		strcpy(m_strName, strName);
	}

	const char* CHDPanoMarker::GetSymbolID() const
	{
		return m_strSymbolID;
	}

	void CHDPanoMarker::SetSymbolID(const char* strSymbolID)
	{
		strcpy(m_strSymbolID, strSymbolID);
	}

	void CHDPanoMarker::GetScale(double& dScaleX, double& dScaleY, double& dScaleZ) const
	{
		dScaleX = m_dScaleX;
		dScaleY = m_dScaleY;
		dScaleZ = m_dScaleZ;
	}

	void CHDPanoMarker::SetScale(double dScaleX, double dScaleY, double dScaleZ)
	{
		m_dScaleX = dScaleX;
		m_dScaleY = dScaleY;
		m_dScaleZ = dScaleZ;
	}}
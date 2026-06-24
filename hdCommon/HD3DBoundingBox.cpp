#include "StdAfx.h"
#include "HD3DBoundingBox.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
namespace hd
{

	CHD3DBoundingBox::CHD3DBoundingBox(void)
	{
		m_dMinX = DBL_MAX;
		m_dMinY = DBL_MAX;
		m_dMinZ = DBL_MAX;
		m_dMaxX = -DBL_MAX;
		m_dMaxY = -DBL_MAX;
		m_dMaxZ = -DBL_MAX;
	}


	CHD3DBoundingBox::~CHD3DBoundingBox(void)
	{
	}

	void CHD3DBoundingBox::InsertPoint(double dX, double dY, double dZ)
	{
		// 取dX与m_dMinX的最小值作为m_dMinX
		m_dMinX = dX < m_dMinX ? dX : m_dMinX;
		// 取dY与m_dMinY的最小值作为m_dMinY
		m_dMinY = dY < m_dMinY ? dY : m_dMinY;
		// 取dZ与m_dMinZ的最小值作为m_dMinZ
		m_dMinZ = dZ < m_dMinZ ? dZ : m_dMinZ;
		// 取dX与m_dMaxX的最大值作为m_dMaxX
		m_dMaxX = dX > m_dMaxX ? dX : m_dMaxX;
		// 取dY与m_dMaxY的最大值作为m_dMaxY
		m_dMaxY = dY > m_dMaxY ? dY : m_dMaxY;
		// 取dZ与m_dMaxZ的最大值作为m_dMaxZ
		m_dMaxZ = dZ > m_dMaxZ ? dZ : m_dMaxZ;
	}

	void CHD3DBoundingBox::InsertBoundingBox(CHD3DBoundingBox bBox)
	{
		m_dMinX = bBox.m_dMinX < m_dMinX ? bBox.m_dMinX : m_dMinX; 
		m_dMinY = bBox.m_dMinY < m_dMinY ? bBox.m_dMinY : m_dMinY; 
		m_dMinZ = bBox.m_dMinZ < m_dMinZ ? bBox.m_dMinZ : m_dMinZ; 
		m_dMaxX = bBox.m_dMaxX > m_dMaxX ? bBox.m_dMaxX : m_dMaxX; 
		m_dMaxY = bBox.m_dMaxY > m_dMaxY ? bBox.m_dMaxY : m_dMaxY; 
		m_dMaxZ = bBox.m_dMaxZ > m_dMaxZ ? bBox.m_dMaxZ : m_dMaxZ; 
	}

	CHD3DBoundingBox& CHD3DBoundingBox::operator=(const CHD3DBoundingBox& other)
	{
		m_dMinX = other.m_dMinX;
		m_dMinY = other.m_dMinY;
		m_dMinZ = other.m_dMinZ;
		m_dMaxX = other.m_dMaxX;
		m_dMaxY = other.m_dMaxY;
		m_dMaxZ = other.m_dMaxZ;
		return *this;
	}

}
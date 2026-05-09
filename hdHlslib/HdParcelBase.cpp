#include "HdParcelBase.h"
#include "..\hdCommon\point_types2.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{

	CHdParcelBase::CHdParcelBase(void)
	{
		m_pParent = NULL;
		m_pHlzPoint = NULL;
		m_distance = 0.f;
		m_SimpleInval = 1;
	}


	CHdParcelBase::~CHdParcelBase(void)
	{
		Release();
	}

	void CHdParcelBase::Release()
	{	
		if (m_pHlzPoint != NULL)
		{
			delete[] m_pHlzPoint;
			m_pHlzPoint = NULL;
		}
	}
}
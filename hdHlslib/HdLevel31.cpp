#include "HdLevel31.h"
#include "HdBlockset.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd 
{
	CHdLevel31::CHdLevel31(void)
		: CHdLevel()
	{
	}

	CHdLevel31::~CHdLevel31(void)
	{
	}

	void CHdLevel31::Update()
	{
		if(!m_pListBlockset.empty())
		{
			m_level31.pointNum = 0;
		}

		for(auto it = m_pListBlockset.begin(); it != m_pListBlockset.end(); ++it)
		{
			m_level31.pointNum += it->second->GetPtCount();
		}
		m_level31.numBlockset = m_pListBlockset.size();
	}

	u64 CHdLevel31::GetPtCount()
	{
		return m_level31.pointNum;
	}
}


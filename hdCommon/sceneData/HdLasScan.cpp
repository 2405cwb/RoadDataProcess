#include "stdafx.h"
#include "HdLasScan.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
CHdLasScan::CHdLasScan(void)
{
	
}

CHdLasScan::~CHdLasScan(void)
{
}

CHdLasScan::CHdLasScan( _HD_LAS_DATA& hdLas )
{
	LasData.lasPath = hdLas.lasPath;
}

CHdLasScan::CHdLasScan( CHdLasScan& hdLas )
{
	LasData.lasPath = hdLas.LasData.lasPath;
}


void CHdLasScan::Serialize( TiXmlElement* element, bool bSave )
{
	if (bSave)
	{
		LasData.Serialize(element, true);
	}
	else
	{
		LasData.Serialize(element, false);
	}
}

}
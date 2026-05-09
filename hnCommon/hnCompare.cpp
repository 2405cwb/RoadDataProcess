#include "stdafx.h"
#include "hnCompare.h"

namespace hnCommon
{
	bool compareMilepileByEnclMile(hnMilePile begMilePile, hnMilePile endMilePile)
	{
		return begMilePile.dEnclMile < endMilePile.dEnclMile;
	}

	// 里程桩按照真实里程排序
	bool compareMilepileByTrueMile(hnMilePile begMilePile, hnMilePile endMilePile)
	{
		return begMilePile.dTrueMile < endMilePile.dTrueMile;
	}

	// 按点的Z值排序
	bool compareZBy3dPoint(hn3dPointD ptBeg, hn3dPointD ptEnd)
	{
		return ptBeg.z < ptEnd.z;
	}

	// 按点的Y值排序
	bool compareYBy3dPoint(hn3dPointD ptBeg, hn3dPointD ptEnd)
	{
		return ptBeg.y < ptEnd.y;
	}

	// 按点的X值排序
	bool compareXBy3dPoint(hn3dPointD ptBeg, hn3dPointD ptEnd)
	{
		return ptBeg.x < ptEnd.x;
	}
}

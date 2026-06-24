#ifndef _HN_COMPARE_H_
#define _HN_COMPARE_H_
#include "stdafx.h"
#include "hnRoadStruct.h"

namespace hnCommon
{
	// 里程桩按照相对里程排序
	HNCOMMOMAPI bool compareMilepileByEnclMile(hnMilePile begMilePile, hnMilePile endMilePile);

	// 里程桩按照真实里程排序
	HNCOMMOMAPI bool compareMilepileByTrueMile(hnMilePile begMilePile, hnMilePile endMilePile);

	// 按点的Z值排序
	HNCOMMOMAPI bool compareZBy3dPoint(hn3dPointD ptBeg, hn3dPointD ptEnd);

	// 按点的Y值排序
	HNCOMMOMAPI bool compareYBy3dPoint(hn3dPointD ptBeg, hn3dPointD ptEnd);

	// 按点的X值排序
	HNCOMMOMAPI bool compareXBy3dPoint(hn3dPointD ptBeg, hn3dPointD ptEnd);
}

#endif



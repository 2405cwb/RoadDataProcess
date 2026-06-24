//---------------------------------------------------------------------------
//	 emap_prjtrans_cassinisoldner.h
//   中海达：坐标投影转换
//
//
//---------------------------------------------------------------------------

#ifndef _EMAP_PRJTRANS_CASSINISOLDNER_H_
#define _EMAP_PRJTRANS_CASSINISOLDNER_H_

#include <math.h>
#include "emap_prjtrans_base.h"

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif
	//----------------------------------------------------------------------------
	void	CassiniSoldner(double Origin_Lat, double Origin_Lon, double inF, double a);
	void	TransformValue(double B, double L, double *North, double *East);
	void	ReverseValue(double North, double East, double *B, double *L);
	//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
#endif	//	_EMAP_PRJTRANS_CASSINISOLDNER_H_
// EOF emap_prjtrans_cassinisoldner.h

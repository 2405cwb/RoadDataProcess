//
//
//
//
//
//
//
//
//----------------------------------------------------------------------------

#ifndef _IHD_PJTRANSLATOR_BASE_H_
#define _IHD_PJTRANSLATOR_BASE_H_

//----------------------------------------------------------------------------

#include <stdio.h>
#include <windows.h>
#pragma warning(once:4996) //仅显示一个

////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
////	this is for dll or com support
//#define _inline_
//#define __EXPORT__ __declspec(dllexport)
//#define __IMPORT__ __declspec(dllimport)
//
//#ifdef EMAP_DEV
//#	ifdef EMAP_DEV_LIB
//#	define DEV_EXPORT	__EXPORT__
//#	else
//#	define DEV_EXPORT	__IMPORT__
//#	endif
//#else
//#	define DEV_EXPORT 
//#endif
//
////----------------------------------------------------------------------------
////----------------------------------------------------------------------------

//四参数
typedef struct
{
	double Dx;
	double Dy;
	double T;
	double K;
}E_PJTFourPar_T;

//七参数结构
typedef struct
{
	double DX;
	double DY;
	double DZ;
	double WX;
	double WY;
	double WZ;
	double K;
}E_PJTSevenPar_T;

//高程拟合参数
typedef struct
{
	double A;
	double B;
	double C;
	double D;
	double E;
	double F;
	double X0;
	double Y0;
}E_PJHeightFixPar_T;

//投影参数
typedef struct
{
	double Rc;
	double Ac;
	double Lo;	//中心点经度
	double Bo;	//中心点纬度
	double NF;	//假原点北坐标
	double EF;	//假原点东坐标
	double EC;	//平均东坐标
	double NC;	//平均北坐标
	double FE;	//东偏移
	double FN;	//北偏移
	double B1;	//第一纬线
	double B2;	//第一纬线
	double Bf;
	double Lf;
	double Bc;
	double Lc;	//平均经度
	double Bp;	//标准纬线
	double Li;	//最初的经线
	double Ko;	//尺度缩放比	
	double Kc;
	double Kp;
	double PH;	//投影高
	int	   W;	//度 带宽
	int	   Add;	//带号
	int	   bAdd;	//添加带号
	int	   North;	//坐标轴X正向是北向
	int	   East;	//坐标轴Y正向是东向
	int	   Unused;	//保留位
}E_PJTProjPars_T;

//----------------------------------------------------------------------------
#endif	//	_IHD_PJTRANSLATOR_BASE_H_

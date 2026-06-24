//---------------------------------------------------------------------------
//   emap_prjtrans_base.h
//   中海达：坐标投影转换
//
//
//---------------------------------------------------------------------------

#ifndef _EMAP_PRJTRANS_BASE_H_
#define _EMAP_PRJTRANS_BASE_H_

#include <stdio.h>
#include <windows.h>
#pragma warning(once:4996) //仅显示一个

////----------------------------------------------------------------------------
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
////------------------------------------------------------------------------------

#define Math_E  2.718281828459
#define PI      3.141592653589793238462643383279
#define EPS     1.0E-12
#define LOGE    2.71828182845904

/*转换模型枚举 */ 
#define ZHD_ConvertEnum_None		0 
#define ZHD_ConvertEnum_Bursa		1     // 布尔莎（Bursa-Wolf）法 
#define ZHD_ConvertEnum_Modensky	2     // 莫洛金斯基（Molodensky）法  
#define ZHD_ConvertEnum_Onetouch	3     // 四参数法 
#define ZHD_ConvertEnum_PolynomialRegression  4   // 基于地心的七参数法

/*投影方法枚举 */ 
#define ZHD_ProjectionEnum_Guas3	0    // 高斯投影 3
#define ZHD_ProjectionEnum_Guass6	1    // 高斯投影 6
#define ZHD_ProjectionEnum_Guass_Userdefine  2  // 高斯投影 自定义
#define ZHD_ProjectionEnum_Mecator  3   //墨卡托投影
#define ZHD_ProjectionEnum_UTM      4   //确切的说是横轴莫卡托
#define ZHD_ProjectionEnum_TM_South			5   // UTM南半球投影
#define ZHD_ProjectionEnum_Lambert_1CCP		6   // 兰伯托斜轴切圆锥投影
#define ZHD_ProjectionEnum_Lambert_2CCP		7   // 兰伯托斜轴割圆锥投影
#define ZHD_ProjectionEnum_Oblique_Stereo	8   // 倾斜赤平投影反算
#define ZHD_ProjectionEnum_Mecator_Oblique  9   // 马来西亚 (倾斜墨卡托投影)
#define ZHD_ProjectionEnum_Mecator_Hotine_Oblique	10  // 倾斜赤平投影反算
#define ZHD_ProjectionEnum_Double_Stereographic		11  // Hotine倾斜墨卡托投影正算
#define ZHD_ProjectionEnum_CassiniSoldner			12 

/*高程拟合模型枚举 */ 
#define ZHD_HighEnum_None  0                 
#define ZHD_HighEnum_HFix  1           
#define ZHD_HighEnum_TGO  2              
#define ZHD_HighEnum_Grid  3  
#define ZHD_HighEnum_FreeSurvey  4  

/*高程拟合模型枚举 */ 
#define ZHD_PaneEnum_None  0  
#define ZHD_PaneEnum_Four  1    
#define ZHD_PaneEnum_TGO  2    
#define ZHD_PaneEnum_Grid  3  
#define ZHD_PaneEnum_FreeSurvey  4 
#define ZHD_PaneEnum_PolynomialFitting  5 

/*几何高程拟合模型枚举 */ 
#define ZHD_HFixEnum_None  0                 
#define ZHD_HFixEnum_Constant  1           
#define ZHD_HFixEnum_Pane  2              
#define ZHD_HFixEnum_Curve  3 

/*带高的投影椭球变形方法枚举 */ 
#define ZHD_ExpandMethodEnum_unknow		-1                 
#define ZHD_ExpandMethodEnum_expand		0    
#define ZHD_ExpandMethodEnum_translation  1
#define ZHD_ExpandMethodEnum_morph		2

// 点结构
typedef struct
{
	 double X;
	 double Y;
	 double Z;
}ZHDPT;

//四参数
typedef struct 
{
	double Dx;
	double Dy;
	double T;
	double K;
}ZHDFourPar;


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
}ZHDSevenPar;

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
}ZHDHFixPar;

//投影参数
typedef struct
{
	double Rc;
	double Ac;
	double Lo;//中心点经度
	double Bo;//中心点纬度
	double NF;//假原点北坐标
	double EF;//假原点东坐标
	double EC;//平均东坐标
	double NC;//平均北坐标
	double FE;//东偏移
	double FN;//北偏移
	double B1;//第一纬线
	double B2;//第二纬线
	double Bf;
	double Lf;
	double Bc;
	double Lc;//平均经度
	double Bp;//标准纬线
	double Li;//最初的经线
	double Ko;//尺度缩放比	
	double Kc;
	double Kp;
	double PH;//投影高
	int	W;//度 带宽
	int	Add;//带号
	int	bAdd;//添加带号
	int	North;//坐标轴X正向是北向
	int	East;//坐标轴Y正向是东向
	int	Unused;	//保留位
}ZHDProjPars;

//----------------------------------------------------------------------------
#endif	//	_EMAP_PRJTRANS_BASE_H_
// EOF emap_prjtrans_base.h

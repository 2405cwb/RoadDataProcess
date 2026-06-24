#include "StdAfx.h"
#include "hnPositionApi.h"

#include "IHdPJTranslator.h"

const double PI64		= 3.1415926535897932384626433832795028841971693993751;
namespace hn
{
	hnPositionApi::hnPositionApi(void)
	{
	}


	hnPositionApi::~hnPositionApi(void)
	{
	}

	void hnPositionApi::wgs84BlhToGaussPrjXyz( double B,double L,double H,double centre,double& proj_x,double& proj_y,double& proj_z, int w/*=3*/,double east_offset /*= 500000*/ )
	{
		double           center_b = 0;
		Spatial_Ref_t    src_param;				// 原始数据转换参数;
		Spatial_Ref_t    dst_param;				// 目标数据转换参数;

		// 数据转换对象;
		IHdPJTranslator* ptr_pj_translator = NULL;	
		CreateIHdPJTranslator(&ptr_pj_translator);

		// 定义两个临时变量;
		double dZ_IN = 0;
		double dZ_OUT = 0;

		// 设置原始数据属性;
		memset(&src_param, 0, sizeof(src_param));
		src_param.coorSystem = E_COOR_SYSTEM_TYPE_GEO;
		src_param.coorUnit = E_COOR_UNIT_TYPE_DEGREE;
		src_param.earthType = E_EARTH_TYPE_WGS84;
		ptr_pj_translator->SetSrcSpatialRef(&src_param);

		// 设置目标数据属性;
		memset(&dst_param, 0, sizeof(dst_param));
		dst_param.coorSystem = E_COOR_SYSTEM_TYPE_PRJ;
		dst_param.coorUnit = E_COOR_UNIT_TYPE_METER;
		dst_param.earthType = E_EARTH_TYPE_WGS84;
		dst_param.prjType = E_PROJECT_TYPE_Gauss_Kruger;
		dst_param.Lo = centre * PI64 / 180.0;
		dst_param.Ko = 1.0;
		dst_param.FE = east_offset;
		dst_param.W = w;
		ptr_pj_translator->SetDstSpatialRef(&dst_param);

		// 坐标转换;
		ptr_pj_translator->Translator(L,B,H, &proj_x, &proj_y, &proj_z);
		DestroyIHdPJTranslator(ptr_pj_translator);
	}

	void hnPositionApi::wgs84GaussPrjXyzToBlh( double proj_x,double proj_y,double proj_z,double centre,double& B,double& L,double& H,int w/*=3*/,double east_offset /*= 500000*/ )
	{
		Spatial_Ref_t    src_param;				// 原始数据转换参数
		Spatial_Ref_t    dst_param;				// 目标数据转换参数

		IHdPJTranslator* ptr_pj_translator = NULL;	// 数据转换对象
		CreateIHdPJTranslator(&ptr_pj_translator);

		// 设置原始数据属性;
		memset(&src_param, 0, sizeof(src_param));
		src_param.coorSystem = E_COOR_SYSTEM_TYPE_PRJ;
		src_param.coorUnit = E_COOR_UNIT_TYPE_METER;
		src_param.earthType = E_EARTH_TYPE_WGS84;
		src_param.prjType = E_PROJECT_TYPE_Gauss_Kruger;
		src_param.Lo = centre * PI64 / 180;
		src_param.Ko = 1.0;
		src_param.FE = east_offset;
		dst_param.W = w;
		ptr_pj_translator->SetSrcSpatialRef(&src_param);

		// 设置目标数据属性;
		memset(&dst_param, 0, sizeof(dst_param));
		dst_param.coorSystem = E_COOR_SYSTEM_TYPE_GEO;
		dst_param.coorUnit = E_COOR_UNIT_TYPE_DEGREE;
		dst_param.earthType = E_EARTH_TYPE_WGS84;
		ptr_pj_translator->SetDstSpatialRef(&dst_param);

		// 坐标转换
		ptr_pj_translator->Translator(proj_x, proj_y, proj_z, &L, &B, &H);
		DestroyIHdPJTranslator(ptr_pj_translator);
	}

}
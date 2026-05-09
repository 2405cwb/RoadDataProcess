//
//
//
//
//
//
//
//
//----------------------------------------------------------------------------

#ifndef _IHD_PJTRANSLATOR_H_
#define _IHD_PJTRANSLATOR_H_
//----------------------------------------------------------------------------

#include "emap_gisdb_base.h"
#include "IHdPJTranslatorBase.h"
#include "hnPositionProj.h"

//----------------------------------------------------------------------------

class HNPOSITIONPROJ_API IHdPJTranslator
{
public:
	// 设置源坐标投影信息
	//------------------------------------------------------------------------------------
	// 设置模式一：
	// 设置参考椭球 E_EARTH_TYPE
	virtual	int		SetSrcEarthType(int EarthType) = 0;
	virtual	int		GetSrcEarthType(int *EarthType) = 0;
	// 设置坐标系类型与单位 E_COOR_SYSTEM_TYPE  E_COOR_UNIT_TYPE
	virtual	int		SetSrcCoorSysType(int CoorType, int  Unit) = 0;
	virtual	int		GetSrcCoorSysType(int *CoorType, int  *Unit) = 0;
	// 设置投影方式与相关参数 E_PROJECT_TYPE
	virtual	int		SetSrcPorjectParam(int PrjType, E_PJTProjPars_T *Param) = 0;
	virtual	int		GetSrcPorjectParam(int *PrjType, E_PJTProjPars_T *Param) = 0;
	//------------------------------------------------------------------------------------
	// 设置模式二：
	virtual	int		SetSrcSpatialRef(Spatial_Ref_t *pSpatialRef) = 0;
	virtual	int		GetSrcSpatialRef(Spatial_Ref_t *pSpatialRef) = 0;


	// 设置目的坐标投影信息
	//------------------------------------------------------------------------------------
	// 设置模式一：
	virtual	int		SetDstEarthType(int EarthType) = 0;
	virtual	int		GetDstEarthType(int *EarthType) = 0;
	virtual	int		SetDstCoorSysType(int CoorType, int  Unit) = 0;
	virtual	int		GetDstCoorSysType(int *CoorType, int  *Unit) = 0;
	virtual	int		SetDstPorjectParam(int PrjType, E_PJTProjPars_T *Param) = 0;
	virtual	int		GetDstPorjectParam(int *PrjType, E_PJTProjPars_T *Param) = 0;
	//------------------------------------------------------------------------------------
	// 设置模式二：
	virtual	int		SetDstSpatialRef(Spatial_Ref_t *pSpatialRef) = 0;
	virtual	int		GetDstSpatialRef(Spatial_Ref_t *pSpatialRef) = 0;


	// 设置椭球变换计算方法与参数 E_CONVERT_TYPE （4参数 7参数）
	virtual	int		SetFourParam(int ConvertType, E_PJTFourPar_T *Param) = 0;
	virtual	int		SetSevenParam(int ConvertType, E_PJTSevenPar_T *Param) = 0;
	virtual	int		GetFourParam(int *ConvertType, E_PJTFourPar_T *Param) = 0;
	virtual	int		GetSevenParam(int *ConvertType, E_PJTSevenPar_T *Param) = 0;
	virtual int		SetHeightFitParam(int model,E_PJHeightFixPar_T* par) = 0;

	// 投影变换
	virtual int		Translator(double in_x, double in_y, double in_z, double *out_x, double *out_y, double *out_z) = 0;
	virtual int		TranslatorReverse(double in_x, double in_y, double in_z, double *out_x, double *out_y, double *out_z) = 0;
	virtual int     TransLators_BLToNE( int ncount,double* degree_latitude, double* degree_longtitude ,double* north,double* east) = 0;
	virtual int     TransLators_NEToBL( int ncount,double* north, double* east ,double* degree_latitude,double* degree_longtitude) = 0;

	virtual void    HFixCalus(double x, double y, double *dResult) = 0;
	virtual int     TranslatorByParam(double in_x, double in_y, double in_z, double *out_x, double *out_y, double *out_z) = 0; // 仅进行四参数或七参数转换

	// 导入导出设置信息
	virtual	int		SaveData(char* DataBuf, int DataLen) = 0;
	virtual	int		LoadData(char* DataBuf, int DataLen) = 0;
};

//----------------------------------------------------------------------------
#ifndef __cplusplus
extern "C"{
#endif
//----------------------------------------------------------------------------
	int	HNPOSITIONPROJ_API	CreateIHdPJTranslator( IHdPJTranslator **pIHdPJTranslator );
	void HNPOSITIONPROJ_API	DestroyIHdPJTranslator( IHdPJTranslator *pIHdPJTranslator );
//----------------------------------------------------------------------------
#ifndef __cplusplus
}
#endif
//----------------------------------------------------------------------------
#endif	//	_IHD_PJTRANSLATOR_H_

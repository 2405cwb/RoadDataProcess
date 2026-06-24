//
//   HdPJTranslator.h
//   坐标投影转换
//   charlin.luo
//   2012-10-24
//
//
//
//----------------------------------------------------------------------------

#ifndef _HD_PJTRANSLATOR_H_
#define _HD_PJTRANSLATOR_H_
//----------------------------------------------------------------------------

#include "emap_prjtrans_api.h"
#include "IHdPJTranslator.h"
//#include "hdGeoProj/hdGeoProj.h"

//----------------------------------------------------------------------------

class CHdPJTranslator : public IHdPJTranslator
{
public:
	CHdPJTranslator();
	virtual ~CHdPJTranslator();

	// 设置源坐标投影信息;
	// 设置参考椭球 E_EARTH_TYPE;
	int		SetSrcEarthType(int EarthType);
	int		GetSrcEarthType(int *EarthType);
	// 设置坐标系类型与单位 E_COOR_SYSTEM_TYPE  E_COOR_UNIT_TYPE;
	int		SetSrcCoorSysType(int CoorType, int  Unit);
	int		GetSrcCoorSysType(int *CoorType, int  *Unit);
	// 设置投影方式与相关参数 E_PROJECT_TYPE;
	int		SetSrcPorjectParam(int PrjType, E_PJTProjPars_T *Param);
	int		GetSrcPorjectParam(int *PrjType, E_PJTProjPars_T *Param);

	int		SetSrcSpatialRef(Spatial_Ref_t *pSpatialRef);
	int		GetSrcSpatialRef(Spatial_Ref_t *pSpatialRef);

	// 设置目的坐标投影信息;
	int		SetDstEarthType(int EarthType);
	int		GetDstEarthType(int *EarthType);
	int		SetDstCoorSysType(int CoorType, int  Unit);
	int		GetDstCoorSysType(int *CoorType, int  *Unit);
	int		SetDstPorjectParam(int PrjType, E_PJTProjPars_T *Param);
	int		GetDstPorjectParam(int *PrjType, E_PJTProjPars_T *Param);

	int		SetDstSpatialRef(Spatial_Ref_t *pSpatialRef);
	int		GetDstSpatialRef(Spatial_Ref_t *pSpatialRef);

	// 设置椭球变换计算方法与参数 E_CONVERT_TYPE （4参数 7参数）;
	int		SetFourParam(int ConvertType, E_PJTFourPar_T *Param);
	int		SetSevenParam(int ConvertType, E_PJTSevenPar_T *Param);

	int		GetFourParam(int *ConvertType, E_PJTFourPar_T *Param);
	int		GetSevenParam(int *ConvertType, E_PJTSevenPar_T *Param);

	// 设置高程拟合参数信息
	int		SetHeightFitParam(int model,E_PJHeightFixPar_T* par);

	// 投影变换;
	int		Translator(double in_x, double in_y, double in_z, double *out_x, double *out_y, double *out_z);
	int		TranslatorReverse(double in_x, double in_y, double in_z, double *out_x, double *out_y, double *out_z);
	int     TransLators_BLToNE(int ncount,double* degree_latitude, double* degree_longtitude,double* north,double* east);
	int     TransLators_NEToBL( int ncount,double* north, double* east ,double* degree_latitude,double* degree_longtitude);

	void    HFixCalus(double x, double y, double *dResult);
	int		TranslatorByParam(double in_x, double in_y, double in_z, double *out_x, double *out_y, double *out_z);

	// 导入导出投影设置信息;
	int		SaveData(char* DataBuf, int DataLen);
	int		LoadData(char* DataBuf, int DataLen);

	// 经纬度转换投影坐标，输入为弧度;
	void BLtoxy_geo(int nModel,int EarthType, ZHDProjPars par, double dB, double dL, 
		double &H, double &dx, double &dy);

	// 投影坐标转换经纬度,输出为弧度;
	void xytoBL_geo(int nModel, int EarthType, ZHDProjPars par, double dx, double dy, double dh, 
		double &dB, double &dL, double &dH);

private:
	// 单位标准化与反标准化  （ 地理坐标--弧度  投影坐标--米）;
	int		StandardUnit(int CoorType, int  Unit, double &x, double &y);
	int		UnStandardUnit(int CoorType, int  Unit, double &x, double &y);

public:
	//	源参考坐标系;
	int		m_SrcEarthType;
	int		m_SrcCoorType;
	int		m_SrcUnit;
	int		m_SrcPrjType;
	int		m_SrcZHDPrjType;
	double	m_SrcEa;
	double	m_SrcEf;
	ZHDProjPars  m_SrcPrjPars;

	// 目的参考坐标系;
	int		m_DstEarthType;
	int		m_DstCoorType;
	int		m_DstUnit;
	int		m_DstPrjType;
	int		m_DstZHDPrjType;
	double	m_DstEa;
	double	m_DstEf;
	ZHDProjPars  m_DstPrjPars;

	int		m_FourType;
	int		m_SevenType;
	ZHDFourPar  m_FourPar;
	ZHDSevenPar m_SevenPar;

	int		   m_HeightFitType;
	ZHDHFixPar m_HeightFixPar;

	//hdGeographicCoordinateSystem m_geographic_coord_system;
};

//----------------------------------------------------------------------------
#endif	//	_HD_PJTRANSLATOR_H_

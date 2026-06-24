//
//   HdPJTranslator.cpp
//   坐标投影转换
//   charlin.luo
//   2012-10-24
//
//
//
//----------------------------------------------------------------------------

// 北京54	 Krassovsky 1940	6378245	298.3
// 国家80	 IAG		78		6378140	298.257
// WGS84	 WGS		1984	6378137	298.2572236
// 国家2000	 CN			2000	6378137	298.2572221
// WGS72	 WGS		1972	6378135	298.26

#include "stdafx.h"
#include "HdPJTranslator.h"
#include "emap_prjtrans_api.h"
//#include "hdGeoProj/hdSpatialReference.h"
#include <ObjBase.h>
//#include "HDCoord.h"

#pragma comment(lib,"ole32.lib")

#define   EARTH_BJ54_EA		6378245
#define   EARTH_BJ54_EF		298.3

#define   EARTH_CN80_EA		6378140
#define   EARTH_CN80_EF		298.257

#define   EARTH_WGS84_EA	6378137
#define   EARTH_WGS84_EF	298.2572236
//#define   EARTH_WGS84_EF	298.257224

#define   EARTH_CN2000_EA	6378137
#define   EARTH_CN2000_EF	298.2572221

#define   EARTH_WGS72_EA	6378135
#define   EARTH_WGS72_EF	298.26

#define	  HDPHTRAN_VERSION  100   // (版本1.00)

#define PI64	3.1415926535897932384626433832795028841971693993751
 
//----------------------------------------------------------------------------
int CreateIHdPJTranslator( IHdPJTranslator **pIHdPJTranslator )
{
	IHdPJTranslator *_pIHdPJTranslator = NULL;
	if( pIHdPJTranslator == NULL )
		return FALSE;

	*pIHdPJTranslator = NULL;
	_pIHdPJTranslator = new CHdPJTranslator;
	if( _pIHdPJTranslator == NULL )
		return FALSE;

	*pIHdPJTranslator = _pIHdPJTranslator;
	return TRUE;
}

void DestroyIHdPJTranslator( IHdPJTranslator *pIHdPJTranslator )
{
	CHdPJTranslator *_pIHdPJTranslator = NULL;
	if( pIHdPJTranslator == NULL )
		return;

	_pIHdPJTranslator = (CHdPJTranslator*)pIHdPJTranslator;
	delete _pIHdPJTranslator;
	pIHdPJTranslator = NULL;
}


//----------------------------------------------------------------------------
CHdPJTranslator::CHdPJTranslator()
{
	m_SrcEarthType = 0;
	m_SrcCoorType = 0;
	m_SrcUnit = 0;
	m_SrcPrjType =0;
	m_SrcEa = 0.0;
	m_SrcEf = 0.0;
	memset(&m_SrcPrjPars, 0, sizeof(ZHDProjPars));

	m_DstEarthType = 0;
	m_DstCoorType = 0;
	m_DstUnit = 0;
	m_DstPrjType =0;
	m_DstEa = 0.0;
	m_DstEf = 0.0;
	memset(&m_DstPrjPars, 0, sizeof(ZHDProjPars));

	m_FourType = E_CONVERT_TYPE_UNKNOWN;
	m_SevenType = E_CONVERT_TYPE_UNKNOWN;
	memset(&m_FourPar, 0, sizeof(E_PJTFourPar_T));
	memset(&m_SevenPar, 0, sizeof(E_PJTSevenPar_T));
	::CoInitialize(NULL);
	
}

CHdPJTranslator::~CHdPJTranslator()
{
	m_SrcEarthType = 0;
	m_SrcCoorType = 0;
	m_SrcUnit = 0;
	m_SrcPrjType =0;
	m_SrcEa = 0.0;
	m_SrcEf = 0.0;
	memset(&m_SrcPrjPars, 0, sizeof(ZHDProjPars));

	m_DstEarthType = 0;
	m_DstCoorType = 0;
	m_DstUnit = 0;
	m_DstPrjType =0;
	m_DstEa = 0.0;
	m_DstEf = 0.0;
	memset(&m_DstPrjPars, 0, sizeof(ZHDProjPars));

	m_FourType = E_CONVERT_TYPE_UNKNOWN;
	m_SevenType = E_CONVERT_TYPE_UNKNOWN;
	memset(&m_FourPar, 0, sizeof(E_PJTFourPar_T));
	memset(&m_SevenPar, 0, sizeof(E_PJTSevenPar_T));

	//if (m_phdCoord)
	//{
	//	delete m_phdCoord;
	//	m_phdCoord = NULL;
	//}
	//if (m_param)
	//{
	//	delete m_param;
	//	m_param = NULL;
	//}


	::CoUninitialize();
}

// 设置源坐标投影信息
// 设置参考椭球 E_EARTH_TYPE
int	CHdPJTranslator::SetSrcEarthType(int EarthType)
{
	switch(EarthType){
		case E_EARTH_TYPE_Beijing54:
			m_SrcEa = EARTH_BJ54_EA;
			m_SrcEf = EARTH_BJ54_EF;
			break;
		case E_EARTH_TYPE_Xian80:
			m_SrcEa = EARTH_CN80_EA;
			m_SrcEf = EARTH_CN80_EF;
			break;
		case E_EARTH_TYPE_China2000:
			m_SrcEa = EARTH_CN2000_EA;
			m_SrcEf = EARTH_CN2000_EF;
			break;
		case E_EARTH_TYPE_WGS84:
			m_SrcEa = EARTH_WGS84_EA;
			m_SrcEf = EARTH_WGS84_EF;
			break;
		case E_EARTH_TYPE_WGS72:
			m_SrcEa = EARTH_WGS72_EA;
			m_SrcEf = EARTH_WGS72_EF;
			break;
		default:
			return FALSE;
	}
	m_SrcEarthType = EarthType;
	return TRUE;
}

int	CHdPJTranslator::GetSrcEarthType(int *EarthType)
{
	if (EarthType == NULL)
		return FALSE;

	*EarthType = m_SrcEarthType;
	return TRUE;
}

// 设置坐标系类型与单位 E_COOR_SYSTEM_TYPE  E_COOR_UNIT_TYPE
int	CHdPJTranslator::SetSrcCoorSysType(int CoorType, int Unit)
{
	switch(CoorType){
		case E_COOR_SYSTEM_TYPE_UNKNOWN:
			break;
		case E_COOR_SYSTEM_TYPE_GEO:
			break;
		case E_COOR_SYSTEM_TYPE_PRJ:
			break;
		default:
			return FALSE;
	}

	m_SrcCoorType = CoorType;
	m_SrcUnit = Unit;
	return TRUE;
}

int	CHdPJTranslator::GetSrcCoorSysType(int *CoorType, int  *Unit)
{
	if (CoorType == NULL || Unit == NULL)
		return FALSE;

	*CoorType = m_SrcCoorType;
	*Unit = m_SrcUnit;
	return TRUE;
}

// 设置投影方式与相关参数 E_PROJECT_TYPE
int	CHdPJTranslator::SetSrcPorjectParam(int PrjType, E_PJTProjPars_T *Param)
{
	switch(PrjType){
		case E_PROJECT_TYPE_UNKNOWN:
			m_SrcZHDPrjType = ZHD_ProjectionEnum_Guass_Userdefine;
			break;
		case E_PROJECT_TYPE_Gauss_Kruger:
			m_SrcZHDPrjType = ZHD_ProjectionEnum_Guass_Userdefine;
			break;
		case E_PROJECT_TYPE_UTM:
			m_SrcZHDPrjType = ZHD_ProjectionEnum_UTM;
			break;
		case E_PROJECT_TYPE_Mercator:
			m_SrcZHDPrjType = ZHD_ProjectionEnum_Mecator;
			break;
		case E_PROJECT_TYPE_Lambert_Conformal_Conic:
			m_SrcZHDPrjType = ZHD_ProjectionEnum_Lambert_1CCP;
			break;
		case E_PROJECT_TYPE_Oblique_Mercator:
			m_SrcZHDPrjType = ZHD_ProjectionEnum_Mecator_Hotine_Oblique;
			break;
		case E_PROJECT_TYPE_StereoGraphic:
			m_SrcZHDPrjType = ZHD_ProjectionEnum_Double_Stereographic;
			break;
		default:
			m_SrcZHDPrjType = ZHD_ProjectionEnum_Guass_Userdefine;
			return FALSE;
	}

	m_SrcPrjType = PrjType;
	memcpy(&m_SrcPrjPars, Param, sizeof(ZHDProjPars));
	return TRUE;
}

int	CHdPJTranslator::GetSrcPorjectParam(int *PrjType, E_PJTProjPars_T *Param)
{
	if (PrjType == NULL || Param == NULL)
		return FALSE;

	*PrjType = m_SrcPrjType;
	memcpy( Param, &m_SrcPrjPars, sizeof(ZHDProjPars));
	return TRUE;
}

int	CHdPJTranslator::SetSrcSpatialRef(Spatial_Ref_t *pSpatialRef)
{
	int ret1, ret2, ret3;
	char *ptr = NULL;
	E_PJTProjPars_T  PrjParam = {0};
	if (pSpatialRef == NULL)
		return FALSE;

	ptr = (char*)pSpatialRef;
	memcpy(&PrjParam, ptr+sizeof(long)*4, sizeof(ZHDProjPars));
	ret1 = SetSrcEarthType(pSpatialRef->earthType);
	ret2 = SetSrcCoorSysType(pSpatialRef->coorSystem, pSpatialRef->coorUnit);
	ret3 = SetSrcPorjectParam(pSpatialRef->prjType, &PrjParam);
	return (ret1&ret2&ret3);
}

int	CHdPJTranslator::GetSrcSpatialRef(Spatial_Ref_t *pSpatialRef)
{
	char *ptr = NULL;
	if ( pSpatialRef == NULL)
		return FALSE;

	ptr = (char*)pSpatialRef;
	pSpatialRef->earthType = m_SrcEarthType;
	pSpatialRef->prjType = m_SrcPrjType;
	pSpatialRef->coorSystem = m_SrcCoorType;
	pSpatialRef->coorUnit = m_SrcUnit;
	memcpy(ptr+sizeof(int)*4, &m_SrcPrjPars, sizeof(ZHDProjPars));
	return TRUE;
}


// 设置目的坐标投影信息
int	CHdPJTranslator::SetDstEarthType(int EarthType)
{
	switch(EarthType){
		case E_EARTH_TYPE_Beijing54:
			m_DstEa = EARTH_BJ54_EA;
			m_DstEf = EARTH_BJ54_EF;
			break;
		case E_EARTH_TYPE_Xian80:
			m_DstEa = EARTH_CN80_EA;
			m_DstEf = EARTH_CN80_EF;
			break;
		case E_EARTH_TYPE_China2000:
			m_DstEa = EARTH_CN2000_EA;
			m_DstEf = EARTH_CN2000_EF;
			break;
		case E_EARTH_TYPE_WGS84:
			m_DstEa = EARTH_WGS84_EA;
			m_DstEf = EARTH_WGS84_EF;
			break;
		case E_EARTH_TYPE_WGS72:
			m_DstEa = EARTH_WGS72_EA;
			m_DstEf = EARTH_WGS72_EF;
			break;
		default:
			return FALSE;
	}
	m_DstEarthType = EarthType;
	return TRUE;
}

int	CHdPJTranslator::GetDstEarthType(int *EarthType)
{
	if (EarthType==NULL)
		return FALSE;

	*EarthType = m_DstEarthType;
	return TRUE;
}

int	CHdPJTranslator::SetDstCoorSysType(int CoorType, int  Unit)
{
	switch(CoorType){
		case E_COOR_SYSTEM_TYPE_UNKNOWN:
			break;
		case E_COOR_SYSTEM_TYPE_GEO:
			break;
		case E_COOR_SYSTEM_TYPE_PRJ:
			break;
		default:
			return FALSE;
	}

	m_DstCoorType = CoorType;
	m_DstUnit = Unit;
	return TRUE;
}

int	CHdPJTranslator::GetDstCoorSysType(int *CoorType, int *Unit)
{
	if (CoorType==NULL || Unit==NULL)
		return FALSE;

	*CoorType = m_DstCoorType;
	*Unit = m_DstUnit;
	return TRUE;
}

int	CHdPJTranslator::SetDstPorjectParam(int PrjType, E_PJTProjPars_T *Param)
{
	if (Param == NULL)	return FALSE;
	
	switch(PrjType){
		case E_PROJECT_TYPE_UNKNOWN:
			m_DstZHDPrjType = ZHD_ProjectionEnum_Guass_Userdefine;
			break;
		case E_PROJECT_TYPE_Gauss_Kruger:
			m_DstZHDPrjType = ZHD_ProjectionEnum_Guass_Userdefine;
			break;
		case E_PROJECT_TYPE_UTM:
			m_DstZHDPrjType = ZHD_ProjectionEnum_UTM;
			break;
		case E_PROJECT_TYPE_Mercator:
			m_DstZHDPrjType = ZHD_ProjectionEnum_Mecator;
			break;
		case E_PROJECT_TYPE_Lambert_Conformal_Conic:
			m_DstZHDPrjType = ZHD_ProjectionEnum_Lambert_1CCP;
			break;
		case E_PROJECT_TYPE_Oblique_Mercator:
			m_DstZHDPrjType = ZHD_ProjectionEnum_Mecator_Hotine_Oblique;
			break;
		case E_PROJECT_TYPE_StereoGraphic:
			m_DstZHDPrjType = ZHD_ProjectionEnum_Double_Stereographic;
			break;
		default:
			m_DstZHDPrjType = ZHD_ProjectionEnum_Guass_Userdefine;
			return FALSE;
	}

	m_DstPrjType = PrjType;
	memcpy(&m_DstPrjPars, Param, sizeof(ZHDProjPars));
	return TRUE;
}

int	CHdPJTranslator::GetDstPorjectParam(int *PrjType, E_PJTProjPars_T *Param)
{
	if (PrjType == NULL || Param == NULL)
		return FALSE;

	*PrjType = m_DstPrjType;
	memcpy( Param, &m_DstPrjPars, sizeof(ZHDProjPars));
	return TRUE;
}

int	CHdPJTranslator::SetDstSpatialRef(Spatial_Ref_t *pSpatialRef)
{
	int ret1, ret2, ret3;
	char *ptr = NULL;
	E_PJTProjPars_T  PrjParam = {0};
	if (pSpatialRef == NULL)
		return FALSE;

	ptr = (char*)pSpatialRef;
	memcpy(&PrjParam, ptr+sizeof(long)*4, sizeof(ZHDProjPars));
	ret1 = SetDstEarthType(pSpatialRef->earthType);
	ret2 = SetDstCoorSysType(pSpatialRef->coorSystem, pSpatialRef->coorUnit);
	ret3 = SetDstPorjectParam(pSpatialRef->prjType, &PrjParam);

	//// 采用新的接口设置相关参数;
	//m_param = new WGS84ToLacal;
	//m_phdCoord = new CHDCoord();

	//m_param->m_SrcEllipsoid.m_a = m_SrcEa;
	//m_param->m_SrcEllipsoid.m_f = m_SrcEf;
	//m_param->m_DstEllipsoid.m_a = m_DstEa;
	//m_param->m_DstEllipsoid.m_f = m_DstEf;

	//if ( m_SevenType == E_CONVERT_TYPE_7)
	//{
	//	m_param->m_ConvertType = E_CONVERT_TYPE_7PARAM;
	//	m_param->m_bWGS84ToLocal = true;

	//	// 设置七参数;
	//	TransCoord7Param param_7;
	//	param_7.m_lfDX = m_SevenPar.DX;
	//	param_7.m_lfDY = m_SevenPar.DY;
	//	param_7.m_lfDZ = m_SevenPar.DZ;
	//	param_7.m_lfRX = m_SevenPar.WX * (180.0 * 3600.0) / PI64;
	//	param_7.m_lfRY = m_SevenPar.WY * (180.0 * 3600.0) / PI64;
	//	param_7.m_lfRZ = m_SevenPar.WZ * (180.0 * 3600.0) / PI64;
	//	param_7.m_lfK = m_SevenPar.K;

	//	m_param->m_TransCoord7Param = param_7;
	//}
	//else if (m_FourType == E_CONVERT_TYPE_4)  // 四参数;
	//{
	//	m_param->m_ConvertType = E_CONVERT_TYPE_PLANE_HEIGHT;
	//	m_param->m_bWGS84ToLocal = true;

	//	TransParams4 param_4;
	//	param_4.m_lfE = m_FourPar.Dy;
	//	param_4.m_lfN = m_FourPar.Dx;
	//	param_4.m_lfScale = m_FourPar.K;
	//	param_4.m_lfRotation = m_FourPar.T;

	//	m_param->m_PlaneTrans4Para = param_4;

	//	TransHeightParams param_height;
	//	param_height.m_lfA = m_HeightFixPar.A;
	//	param_height.m_lfB = m_HeightFixPar.B;
	//	param_height.m_lfC = m_HeightFixPar.C;
	//	param_height.m_lfD = m_HeightFixPar.D;
	//	param_height.m_lfE = m_HeightFixPar.E;
	//	param_height.m_lfF = m_HeightFixPar.F;
	//	param_height.m_lfN0 = m_HeightFixPar.X0;
	//	param_height.m_lfE0 = m_HeightFixPar.Y0;

	//	switch (m_HeightFitType)
	//	{
	//	case 1:
	//		param_height.m_model = CONSTANT; // 固定差改正
	//		break;
	//	case 2:
	//		param_height.m_model = PLANEFIT; // 平面拟合
	//		break;
	//	case 3:
	//		param_height.m_model = QUADRATIC_SURFACE; // 曲面拟合
	//		break;
	//	}

	//	m_param->m_HeightFit = param_height;
	//}
	//else
	//{
	//	m_param->m_bWGS84ToLocal = false;
	//}

	//// 设置投影类型;
	//switch (m_DstZHDPrjType)
	//{
	//case ZHD_ProjectionEnum_Guas3://高斯3
	//case ZHD_ProjectionEnum_Guass_Userdefine://自定义高斯
	//	m_param->m_ProjParam.m_eProjType = GAUSS3;
	//	m_param->m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
	//	break;
	//case ZHD_ProjectionEnum_Guass6://高斯6
	//	m_param->m_ProjParam.m_eProjType = GAUSS6;
	//	m_param->m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
	//	break;
	//case ZHD_ProjectionEnum_Mecator://莫卡托投影
	//	m_param->m_ProjParam.m_eProjType = Mercator;
	//	m_param->m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
	//	break;
	//case ZHD_ProjectionEnum_UTM://UTM投影
	//	m_param->m_ProjParam.m_eProjType = UTM;
	//	m_param->m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
	//	break;
	//}

	//// 设置其他中央经线，偏移量等信息;
	//m_param->m_ProjParam.m_lfCentralMeridian = m_DstPrjPars.Lo * 180.0 / PI64;
	//m_param->m_ProjParam.m_lfEastAdd = m_DstPrjPars.FE;
	//m_param->m_ProjParam.m_lfNorthAdd = m_DstPrjPars.FN;

	//m_param->m_ProjParam.m_lfBm = m_DstPrjPars.Bc;
	//m_param->m_ProjParam.m_bAddZoneNo = false;
	//m_param->m_ProjParam.m_lfProjHeight = m_DstPrjPars.PH;

	return (ret1&ret2&ret3);
}

int	CHdPJTranslator::GetDstSpatialRef(Spatial_Ref_t *pSpatialRef)
{
	char *ptr = NULL;
	if ( pSpatialRef == NULL)
		return FALSE;

	ptr = (char*)pSpatialRef;
	pSpatialRef->earthType = m_DstEarthType;
	pSpatialRef->prjType = m_DstPrjType;
	pSpatialRef->coorSystem = m_DstCoorType;
	pSpatialRef->coorUnit = m_DstUnit;
	memcpy(ptr+sizeof(long)*4, &m_DstPrjPars, sizeof(ZHDProjPars));
	return TRUE;
}


// 设置椭球变换计算方法与参数 E_CONVERT_TYPE （4参数 7参数）
int	CHdPJTranslator::SetFourParam(int ConvertType, E_PJTFourPar_T *Param)
{
	m_FourType = ConvertType;
	if (Param == NULL || m_FourType==E_CONVERT_TYPE_UNKNOWN)	
		return TRUE;

	memcpy(&m_FourPar, Param, sizeof(ZHDFourPar));
	return TRUE;
}

int	CHdPJTranslator::SetSevenParam(int ConvertType, E_PJTSevenPar_T *Param)
{
	m_SevenType = ConvertType;
	if (Param == NULL || m_SevenType==E_CONVERT_TYPE_UNKNOWN)	
		return TRUE;

	memcpy(&m_SevenPar, Param, sizeof(ZHDSevenPar));
	return TRUE;
}

int	CHdPJTranslator::GetFourParam(int *ConvertType, E_PJTFourPar_T *Param)
{
	*ConvertType = m_FourType;
	if (Param == NULL || m_FourType==E_CONVERT_TYPE_UNKNOWN)	
		return TRUE;

	memcpy(Param, &m_FourPar, sizeof(ZHDFourPar));
	return TRUE;
}

int	CHdPJTranslator::GetSevenParam(int *ConvertType, E_PJTSevenPar_T *Param)
{
	*ConvertType = m_SevenType;
	if (Param == NULL || m_SevenType==E_CONVERT_TYPE_UNKNOWN)	
		return TRUE;

	memcpy(Param, &m_SevenPar, sizeof(ZHDSevenPar));
	return TRUE;
}


// 投影变换
int	CHdPJTranslator::Translator(double in_x, double in_y, double in_z, double *out_x, double *out_y, double *out_z)
{
	return TranslatorBLToNeH(in_x,in_y,in_z,out_x,out_y,out_z);

	double _inx, _iny, _outx, _outy;
	double _inh = 0, _outh = 0;

	// 不成功情况下，直接返回传入值
	*out_x = in_x;
	*out_y = in_y;
	*out_z = in_z;

	if (m_SrcEarthType == 0 || m_DstEarthType == 0)
		return FALSE;

	if (m_SrcCoorType == 0 || m_DstCoorType == 0)
		return FALSE;

	// 测量坐标系与数字坐标系XY轴是相反的，古此处需要转换
	_inx = in_y;  _iny = in_x; _inh = in_z;

	if( StandardUnit(m_SrcCoorType, m_SrcUnit, _inx, _iny) == FALSE)
		return FALSE;

	// 换算坐标单位
	if (m_SrcCoorType == E_COOR_SYSTEM_TYPE_GEO)   // 地理坐标
	{
		//// 椭球参数是否相同
		//if (m_SrcEarthType == m_DstEarthType)
		//{
		//	if (m_DstCoorType == E_COOR_SYSTEM_TYPE_GEO)
		//	{
		//		_outx = _inx; _outy = _iny; _outh = _inh;
		//	}
		//	else
		//	{
		//		//BLtoxy(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh, 
		//		//	_outx, _outy, TRUE, TRUE);
		//		BLtoxy_geo(m_DstZHDPrjType, m_DstEarthType, m_DstPrjPars, _inx, _iny, _inh,_outx, _outy);

		//		_outh = _inh;

		//		// 如果存在四参数，则继续转换
		//		if (m_FourType == E_CONVERT_TYPE_4)
		//		{
		//			xtox(m_FourPar, &_outx, &_outy);
		//		}
		//	}

		//	UnStandardUnit(m_DstCoorType, m_DstUnit, _outx, _outy);

		//	// 测量坐标系与数字坐标系XY轴是相反的，古此处需要转换
		//	*out_x = _outy;
		//	*out_y = _outx;
		//	*out_z = _outh;
		//	return TRUE;
		//}
		//else
		{
			// 优先选用七参数转换
			if ( m_SevenType == E_CONVERT_TYPE_7)
			{
				// 地理到投影坐标系, 转换后的标准单位为米
				BtoX(m_SrcEa, m_SrcEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);
				// 7参数转换
				XtoX_Bursa_Simple__(m_SevenPar, &_outx, &_outy, &_outh);
				// 投影坐标系到的坐标
				_inx = _outx;  _iny = _outy;  _inh = _outh;
				XtoB(m_DstEa, m_DstEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);

				if (m_DstCoorType == E_COOR_SYSTEM_TYPE_GEO)
				{ ; }
				else
				{
					_inx = _outx;  _iny = _outy;  _inh = _outh;
					BLtoxy(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh, 
						_outx, _outy, TRUE, TRUE);
					//BLtoxy(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh, 
					//	_outx, _outy, TRUE, TRUE);
					//double tempH = _outy;
					//BLtoxy(m_DstZHDPrjType, m_DstEarthType, m_DstPrjPars, _inx, _iny, _inh,
					//	_outx, _outy);
					_outh = _inh;
					//_outy = tempH;

					// 如果存在四参数，则继续转换
					if (m_FourType == E_CONVERT_TYPE_4)
					{
						xtox(m_FourPar, &_outx, &_outy);
					}
				}
			}
			else if (m_FourType == E_CONVERT_TYPE_4)
			{
				// 投影变换
				BLtoxy(m_SrcZHDPrjType, m_SrcEa, m_SrcEf, m_SrcPrjPars, _inx, _iny, _inh, 
					_outx, _outy, TRUE, TRUE);
				//BLtoxy(m_SrcZHDPrjType, m_SrcEarthType, m_DstPrjPars, _inx, _iny, _inh,
				//	_outx, _outy);
				_outh = _inh;

				// 4参数转换
				xtox(m_FourPar, &_outx, &_outy);

				// 反投影变换
				if (m_DstCoorType == E_COOR_SYSTEM_TYPE_GEO)
				{
					_inx = _outx;  _iny = _outy;
					//xytoBL(m_DstZHDPrjType, m_DstEarthType, m_DstPrjPars, _inx, _iny, _inh, 
					//	_outx, _outy, _outh);
					xytoBL(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh,
						_outx, _outy, _outh, TRUE, TRUE);
				}
				else
				{ ; }
			}
			else
			{
				if (m_SrcEarthType == m_DstEarthType)
				{
					BLtoxy(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh, _outx, _outy, TRUE, TRUE);
					//BLtoxy(m_DstZHDPrjType, m_DstEarthType, m_DstPrjPars, _inx, _iny, _inh,_outx, _outy);

					_outh = _inh;

					// 如果存在四参数，则继续转换
					if (m_FourType == E_CONVERT_TYPE_4)
					{
						xtox(m_FourPar, &_outx, &_outy);
					}
					
					UnStandardUnit(m_DstCoorType, m_DstUnit, _outx, _outy);
				}
				else
				{
					// 大地坐标转空间直角坐标系
					BtoX(m_SrcEa, m_SrcEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);
					//之间因为不存在四参数和七参数，所以不同椭球之间空间直角坐标系值一样
					// 空间直角坐标系转大地坐标
					_inx = _outx;  _iny = _outy;  _inh = _outh;
					XtoB(m_DstEa, m_DstEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);

					// 无七参数或四参数 则直接投影
					if (m_DstCoorType == E_COOR_SYSTEM_TYPE_GEO)
					{ ; }
					else
					{
						_inx = _outx;  _iny = _outy;  _inh = _outh;
						BLtoxy(m_DstZHDPrjType, m_SrcEa, m_SrcEf, m_DstPrjPars, _inx, _iny, _inh, 
							_outx, _outy, TRUE, TRUE);
						//BLtoxy(m_DstZHDPrjType, m_SrcEarthType, m_DstPrjPars, _inx, _iny, _inh,
						//	_outx, _outy);
						_outh = _inh;
					}
					UnStandardUnit(m_DstCoorType, m_DstUnit, _outx, _outy);
				}

				*out_x = _outy;
				*out_y = _outx;
				*out_z = _outh;
				return FALSE;
			}

			UnStandardUnit(m_DstCoorType, m_DstUnit, _outx, _outy);

			// 测量坐标系与数字坐标系XY轴是相反的，古此处需要转换
			*out_x = _outy;
			*out_y = _outx;
			*out_z = _outh;
			return TRUE;
		}
	}
	else
	{
		// 椭球参数是否相同
		if (m_SrcEarthType == m_DstEarthType)
		{
			// 如果存在四参数，则继续转换
			if (m_FourType == E_CONVERT_TYPE_4)
			{
				xtox(m_FourPar, &_inx, &_iny);
			}

			if (m_DstCoorType == E_COOR_SYSTEM_TYPE_GEO)
			{
				xytoBL(m_SrcZHDPrjType, m_SrcEa, m_SrcEf, m_SrcPrjPars, _inx, _iny, _inh, 
					_outx, _outy, _outh, TRUE, TRUE);
				//xytoBL(m_SrcZHDPrjType, m_SrcEarthType, m_SrcPrjPars, _inx, _iny, _inh,
				//	_outx, _outy, _outh);
			}
			else
			{
				// 判断投影类型是否相同
				if (m_SrcPrjType == m_DstPrjType)
				{
					_outx = _inx;
					_outy = _iny;
				}
				else
				{
					xytoBL(m_SrcZHDPrjType, m_SrcEa, m_SrcEf, m_SrcPrjPars, _inx, _iny, _inh, 
						_outx, _outy, _outh, TRUE, TRUE);
					//xytoBL(m_SrcZHDPrjType, m_SrcEarthType, m_SrcPrjPars, _inx, _iny, _inh,
					//	_outx, _outy, _outh);

					_inx = _outx; _iny = _outy; _inh = _outh;
					BLtoxy(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh, 
						_outx, _outy, TRUE, TRUE);
					//BLtoxy(m_DstZHDPrjType, m_DstEarthType, m_DstPrjPars, _inx, _iny, _inh,
					//	_outx, _outy);
					_outh = _inh;

					// 如果存在四参数，则继续转换
					if (m_FourType == E_CONVERT_TYPE_4)
					{
						xtox(m_FourPar, &_outx, &_outy);
					}
				}
			}

			UnStandardUnit(m_DstCoorType, m_DstUnit, _outx, _outy);

			// 测量坐标系与数字坐标系XY轴是相反的，古此处需要转换
			*out_x = _outy;
			*out_y = _outx;
			*out_z = _outh;
			return TRUE;
		}
		// 椭球参数不同
		else
		{
			if ( m_SevenType == E_CONVERT_TYPE_7) // 七参数
			{
				xytoBL(m_SrcZHDPrjType, m_SrcEa, m_SrcEf, m_SrcPrjPars, _inx, _iny, _inh, 
					_outx, _outy, _outh, TRUE, TRUE);
				//xytoBL(m_SrcZHDPrjType, m_SrcEarthType, m_SrcPrjPars, _inx, _iny, _inh,
				//	_outx, _outy, _outh);

				_inx = _outx; _iny = _outy; _inh = _outh;
				BtoX(m_SrcEa, m_SrcEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);

				// 7参数
				XtoX_Bursa_Simple__(m_SevenPar, &_outx, &_outy, &_outh);
				
				_inx = _outx;  _iny = _outy;  _inh = _outh;
				XtoB(m_DstEa, m_DstEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);

				if (m_DstCoorType == E_COOR_SYSTEM_TYPE_GEO)
				{ ; }
				else
				{
					_inx = _outx;  _iny = _outy;  _inh = _outh;
					BLtoxy(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh, 
						_outx, _outy, TRUE, TRUE);
					//BLtoxy(m_DstZHDPrjType, m_DstEarthType, m_DstPrjPars, _inx, _iny, _inh,
					//	_outx, _outy);
					_outh = _inh;

					// 如果存在四参数，则继续转换
					if (m_FourType == E_CONVERT_TYPE_4)
					{
						xtox(m_FourPar, &_outx, &_outy);
					}
				}
			}
			else if (m_FourType == E_CONVERT_TYPE_4)  // 四参数
			{
				xtox(m_FourPar, &_inx, &_iny);
				if (m_DstCoorType == E_COOR_SYSTEM_TYPE_GEO)
				{
					xytoBL(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh, 
						_outx, _outy, _outh, TRUE, TRUE);
					//xytoBL(m_DstZHDPrjType, m_DstEarthType, m_DstPrjPars, _inx, _iny, _inh,
					//	_outx, _outy, _outh);
				}
				else
				{ ; }

			}
			else
			{
				xytoBL(m_SrcZHDPrjType, m_SrcEa, m_SrcEf, m_SrcPrjPars, _inx, _iny, _inh, 
					_outx, _outy, _outh, TRUE, TRUE);
				//xytoBL(m_SrcZHDPrjType, m_SrcEarthType, m_SrcPrjPars, _inx, _iny, _inh,
				//	_outx, _outy, _outh);

				_inx = _outx; _iny = _outy; _inh = _outh;
				BtoX(m_SrcEa, m_SrcEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);

				_inx = _outx;  _iny = _outy;  _inh = _outh;
				XtoB(m_DstEa, m_DstEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);

				// 无四参数或其参数 则直接投影
				if (m_DstCoorType == E_COOR_SYSTEM_TYPE_GEO)
				{;}
				else
				{ 
					_inx = _outx;  _iny = _outy;  _inh = _outh;
					BLtoxy(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh, 
						_outx, _outy, TRUE, TRUE);
					//BLtoxy(m_DstZHDPrjType, m_DstEarthType, m_DstPrjPars, _inx, _iny, _inh,
					//	_outx, _outy);
					_outh = _inh;
				}
				UnStandardUnit(m_DstCoorType, m_DstUnit, _outx, _outy);
				*out_x = _outy;
				*out_y = _outx;
				*out_z = _outh;
				return FALSE;
			}

			UnStandardUnit(m_DstCoorType, m_DstUnit, _outx, _outy);

			// 测量坐标系与数字坐标系XY轴是相反的，古此处需要转换
			*out_x = _outy;
			*out_y = _outx;
			*out_z = _outh;
			return TRUE;
		}
	}
	return TRUE;
}

int	CHdPJTranslator::TranslatorReverse(double in_x, double in_y, double in_z, double *out_x, double *out_y, double *out_z)
{
	double _inx, _iny, _outx, _outy;
	double _inh = 0, _outh = 0;;

	// 不成功情况下，直接返回传入值
	*out_x = in_x;
	*out_y = in_y;
	*out_z = in_z;

	if (m_SrcEarthType == 0 || m_DstEarthType == 0)
		return FALSE;

	if (m_SrcCoorType == 0 || m_DstCoorType == 0)
		return FALSE;

	// 测量坐标系与数字坐标系XY轴是相反的，古此处需要转换
	_inx = in_y;  _iny = in_x; _inh = in_z;

	if( StandardUnit(m_DstCoorType, m_DstUnit, _inx, _iny) == FALSE)
		return FALSE;

	// 换算坐标单位
	if (m_DstCoorType == E_COOR_SYSTEM_TYPE_GEO)   // 地理坐标
	{
		// 椭球参数是否相同
		if (m_DstEarthType == m_SrcEarthType)
		{
			if (m_SrcCoorType == E_COOR_SYSTEM_TYPE_GEO)
			{
				_outx = _inx; 
				_outy = _iny; 
				_outh = _inh;
			}
			else
			{
				BLtoxy(m_SrcZHDPrjType, m_SrcEa, m_SrcEf, m_SrcPrjPars, _inx, _iny, _inh, 
					_outx, _outy, TRUE, TRUE);
				//BLtoxy_geo(m_SrcZHDPrjType, m_SrcEarthType, m_SrcPrjPars, _inx, _iny, _inh, 
				//	_outx, _outy);
				_outh = _inh;
			}

			UnStandardUnit(m_SrcCoorType, m_SrcUnit, _outx, _outy);

			// 测量坐标系与数字坐标系XY轴是相反的，古此处需要转换
			*out_x = _outy;
			*out_y = _outx;
			*out_z = _outh;
			return TRUE;
		}
		else
		{
			if ( m_SevenType == E_CONVERT_TYPE_7)
			{
				// 地理到投影坐标系, 转换后的标准单位为米
				BtoX(m_DstEa, m_DstEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);
				// 7参数转换
				XtoX_Bursa_Simple_False__(m_SevenPar, &_outx, &_outy, &_outh);
				// 投影坐标系到的坐标
				_inx = _outx;  _iny = _outy;  _inh = _outh;
				XtoB(m_SrcEa, m_SrcEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);

				if (m_SrcCoorType == E_COOR_SYSTEM_TYPE_GEO)
				{ ; }
				else
				{
					_inx = _outx;  _iny = _outy;  _inh = _outh;
					BLtoxy(m_SrcZHDPrjType, m_SrcEa, m_SrcEf, m_SrcPrjPars, _inx, _iny, _inh, 
						_outx, _outy, TRUE, TRUE);
					//BLtoxy_geo(m_SrcZHDPrjType, m_SrcEarthType, m_SrcPrjPars, _inx, _iny, _inh, 
					//	_outx, _outy);
					_outh = _inh;
				}
			}
			else if (m_FourType == E_CONVERT_TYPE_4)
			{
				// 投影变换
				BLtoxy(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh, 
					_outx, _outy, TRUE, TRUE);
				//BLtoxy_geo(m_DstZHDPrjType, m_DstEarthType, m_DstPrjPars, _inx, _iny, _inh, 
				//	_outx, _outy);
				_outh = _inh;

				// 4参数转换
				xtox_false(m_FourPar, &_outx, &_outy);

				// 反投影变换
				if (m_SrcCoorType == E_COOR_SYSTEM_TYPE_GEO)
				{
					_inx = _outx;  _iny = _outy;  _inh = _outh;
					xytoBL(m_SrcZHDPrjType, m_SrcEa, m_SrcEf, m_SrcPrjPars, _inx, _iny, _inh, 
						_outx, _outy, _outh, TRUE, TRUE);
					//xytoBL_geo(m_SrcZHDPrjType, m_SrcEarthType, m_SrcPrjPars, _inx, _iny, _inh, 
					//	_outx, _outy, _outh);
				}
				else
				{ ; }
			}
			else
			{
				// 地理到投影坐标系, 转换后的标准单位为米
				BtoX(m_DstEa, m_DstEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);
				// 投影坐标系到的坐标
				_inx = _outx;  _iny = _outy;  _inh = _outh;
				XtoB(m_SrcEa, m_SrcEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);

				// 无四参数或其参数 则直接投影
				if (m_SrcCoorType == E_COOR_SYSTEM_TYPE_GEO)
				{ ; }
				else
				{ 
					BLtoxy(m_SrcZHDPrjType, m_DstEa, m_DstEf, m_SrcPrjPars, _inx, _iny, _inh, 
						_outx, _outy, TRUE, TRUE);
					//BLtoxy_geo(m_SrcZHDPrjType, m_DstEarthType, m_SrcPrjPars, _inx, _iny, _inh, 
					//	_outx, _outy);
					_outh = _inh;
				}
				UnStandardUnit(m_SrcCoorType, m_SrcUnit, _outx, _outy);
				*out_x = _outy;
				*out_y = _outx;
				*out_z = _outh;
				return FALSE;
			}

			UnStandardUnit(m_SrcCoorType, m_SrcUnit, _outx, _outy);

			// 测量坐标系与数字坐标系XY轴是相反的，古此处需要转换
			*out_x = _outy;
			*out_y = _outx;
			*out_z = _outh;
			return TRUE;
		}
	}
	else
	{
		// 椭球参数是否相同
		if (m_DstEarthType == m_SrcEarthType)
		{
			// 如果存在四参数，则先转换
			if (m_FourType == E_CONVERT_TYPE_4)
			{
				xtox_false(m_FourPar, &_inx, &_iny);
			}

			if (m_SrcCoorType == E_COOR_SYSTEM_TYPE_GEO)
			{
				xytoBL(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh, 
					_outx, _outy, _outh, TRUE, TRUE);
				//xytoBL_geo(m_DstZHDPrjType, m_DstEarthType, m_DstPrjPars, _inx, _iny, _inh, 
				//	_outx, _outy, _outh);
			}
			else
			{
				// 判断投影类型是否相同
				if (m_DstPrjType == m_SrcPrjType)
				{
					_outx = _inx;
					_outy = _iny;
					_outh = _inh;
				}
				else
				{
					xytoBL(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh, 
						_outx, _outy, _outh, TRUE, TRUE);
					//xytoBL_geo(m_DstZHDPrjType, m_DstEarthType, m_DstPrjPars, _inx, _iny, _inh, 
					//	_outx, _outy, _outh);

					_inx = _outx; _iny = _outy;
					BLtoxy(m_SrcZHDPrjType, m_SrcEa, m_SrcEf, m_SrcPrjPars, _inx, _iny, _inh, 
						_outx, _outy, TRUE, TRUE);
					//BLtoxy_geo(m_SrcZHDPrjType, m_SrcEarthType, m_SrcPrjPars, _inx, _iny, _inh, 
					//	_outx, _outy);
					_outh = _inh;
				}
			}

			UnStandardUnit(m_SrcCoorType, m_SrcUnit, _outx, _outy);

			// 测量坐标系与数字坐标系XY轴是相反的，古此处需要转换
			*out_x = _outy;
			*out_y = _outx;
			*out_z = _outh;
			return TRUE;
		}
		// 椭球参数不同
		else
		{
			if ( m_SevenType == E_CONVERT_TYPE_7) // 七参数
			{
				// 如果存在四参数，则先转换
				if (m_FourType == E_CONVERT_TYPE_4)
				{
					xtox_false(m_FourPar, &_inx, &_iny);
				}

				xytoBL(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh, 
					_outx, _outy, _outh, TRUE, TRUE);
				//xytoBL_geo(m_DstZHDPrjType, m_DstEarthType, m_DstPrjPars, _inx, _iny, _inh, 
				//	_outx, _outy, _outh);

				_inx = _outx; _iny = _outy; _inh = _outh;
				BtoX(m_DstEa, m_DstEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);
				// 7参数
				XtoX_Bursa_Simple_False__(m_SevenPar, &_outx, &_outy, &_outh);
				_inx = _outx;  _iny = _outy;  _inh = _outh;
				XtoB(m_SrcEa, m_SrcEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);

				if (m_SrcCoorType == E_COOR_SYSTEM_TYPE_GEO)
				{;}
				else
				{
					_inx = _outx;  _iny = _outy;  _inh = _outh;
					BLtoxy(m_SrcZHDPrjType, m_SrcEa, m_SrcEf, m_SrcPrjPars, _inx, _iny, _inh, 
						_outx, _outy, TRUE, TRUE);
					//BLtoxy_geo(m_SrcZHDPrjType, m_SrcEarthType, m_SrcPrjPars, _inx, _iny, _inh, 
					//	_outx, _outy);
					_outh = _inh;
				}
			}
			else if (m_FourType == E_CONVERT_TYPE_4)  // 四参数
			{
				xtox_false(m_FourPar, &_inx, &_iny);
				if (m_SrcCoorType == E_COOR_SYSTEM_TYPE_GEO)
				{
					xytoBL(m_SrcZHDPrjType, m_SrcEa, m_SrcEf, m_SrcPrjPars, _inx, _iny, _inh, 
						_outx, _outy, _outh, TRUE, TRUE);
					//xytoBL_geo(m_SrcZHDPrjType, m_SrcEarthType, m_SrcPrjPars, _inx, _iny, _inh, 
					//	_outx, _outy, _outh);
				}
				else
				{ ; }
			}
			else
			{
				xytoBL(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _inx, _iny, _inh, 
					_outx, _outy, _outh, TRUE, TRUE);
				//xytoBL_geo(m_DstZHDPrjType, m_DstEarthType, m_DstPrjPars, _inx, _iny, _inh, 
				//	_outx, _outy, _outh);

				_inx = _outx; _iny = _outy; _inh = _outh;
				BtoX(m_DstEa, m_DstEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);
				_inx = _outx;  _iny = _outy;  _inh = _outh;
				XtoB(m_SrcEa, m_SrcEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);

				// 无四参数或其参数 则直接投影
				if (m_SrcCoorType == E_COOR_SYSTEM_TYPE_GEO)
				{;}
				else
				{
					_inx = _outx;  _iny = _outy;  _inh = _outh;
					BLtoxy(m_SrcZHDPrjType, m_SrcEa, m_SrcEf, m_SrcPrjPars, _inx, _iny, _inh, 
						_outx, _outy, TRUE, TRUE);
					//BLtoxy_geo(m_SrcZHDPrjType, m_SrcEarthType, m_SrcPrjPars, _inx, _iny, _inh, 
					//	_outx, _outy);
					_outh = _inh;
				}
				UnStandardUnit(m_SrcCoorType, m_SrcUnit, _outx, _outy);
				*out_x = _outy;
				*out_y = _outx;
				*out_z = _outh;
				return FALSE;
			}

			UnStandardUnit(m_SrcCoorType, m_SrcUnit, _outx, _outy);

			// 测量坐标系与数字坐标系XY轴是相反的，古此处需要转换
			*out_x = _outy;
			*out_y = _outx;
			*out_z = _outh;
			return TRUE;
		}
	}
	return TRUE;
}

// 导入导出投影设置信息
int	CHdPJTranslator::SaveData(char* DataBuf, int DataLen)
{
	int Offset = 0;
	int Version = HDPHTRAN_VERSION;
	if (DataBuf == NULL || DataLen < 580)
		return FALSE;

	// 源参考坐标系
	memcpy(DataBuf+Offset, &Version, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_SrcEarthType, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_SrcCoorType, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_SrcUnit, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_SrcPrjType, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_SrcEarthType, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_SrcZHDPrjType, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_SrcEa, sizeof(double));
	Offset +=  sizeof(double);
	memcpy(DataBuf+Offset, &m_SrcEf, sizeof(double));
	Offset +=  sizeof(double);
	memcpy(DataBuf+Offset, &m_SrcPrjPars, sizeof(ZHDProjPars));
	Offset +=  sizeof(ZHDProjPars);

	// 目的参考坐标系
	memcpy(DataBuf+Offset, &m_DstEarthType, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_DstCoorType, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_DstUnit, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_DstPrjType, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_DstEarthType, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_DstZHDPrjType, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_DstEa, sizeof(double));
	Offset +=  sizeof(double);
	memcpy(DataBuf+Offset, &m_DstEf, sizeof(double));
	Offset +=  sizeof(double);
	memcpy(DataBuf+Offset, &m_DstPrjPars, sizeof(ZHDProjPars));
	Offset +=  sizeof(ZHDProjPars);

	// 七参数、四参数信息
	memcpy(DataBuf+Offset, &m_FourType, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_SevenType, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(DataBuf+Offset, &m_FourPar, sizeof(ZHDFourPar));
	Offset +=  sizeof(ZHDFourPar);
	memcpy(DataBuf+Offset, &m_SevenPar, sizeof(ZHDSevenPar));
	Offset +=  sizeof(ZHDSevenPar);
	return Offset;
}

int	CHdPJTranslator::LoadData(char* DataBuf, int DataLen)
{
	int Offset = 0;
	int Version = 0;
	if (DataBuf == NULL || DataLen < 580)
		return FALSE;

	// 源参考坐标系
	memcpy(&Version, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_SrcEarthType, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_SrcCoorType, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_SrcUnit, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_SrcPrjType, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_SrcEarthType, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_SrcZHDPrjType, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_SrcEa, DataBuf+Offset, sizeof(double));
	Offset +=  sizeof(double);
	memcpy(&m_SrcEf, DataBuf+Offset, sizeof(double));
	Offset +=  sizeof(double);
	memcpy(&m_SrcPrjPars, DataBuf+Offset, sizeof(ZHDProjPars));
	Offset +=  sizeof(ZHDProjPars);

	// 目的参考坐标系
	memcpy(&m_DstEarthType, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_DstCoorType, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_DstUnit, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_DstPrjType, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_DstEarthType, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_DstZHDPrjType, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_DstEa, DataBuf+Offset, sizeof(double));
	Offset +=  sizeof(double);
	memcpy(&m_DstEf, DataBuf+Offset, sizeof(double));
	Offset +=  sizeof(double);
	memcpy(&m_DstPrjPars, DataBuf+Offset, sizeof(ZHDProjPars));
	Offset +=  sizeof(ZHDProjPars);

	// 七参数、四参数信息
	memcpy(&m_FourType, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_SevenType, DataBuf+Offset, sizeof(int));
	Offset +=  sizeof(int);
	memcpy(&m_FourPar, DataBuf+Offset, sizeof(ZHDFourPar));
	Offset +=  sizeof(ZHDFourPar);
	memcpy(&m_SevenPar, DataBuf+Offset, sizeof(ZHDSevenPar));
	Offset +=  sizeof(ZHDSevenPar);
	return Offset;
}

int	CHdPJTranslator::StandardUnit(int CoorType, int  Unit, double &x, double &y)
{
	// 换算坐标单位
	if (CoorType == E_COOR_SYSTEM_TYPE_GEO)   // 地理坐标
	{
		// 统一将单位转化为弧度
		if (Unit == E_COOR_UNIT_TYPE_DEGREE)
		{
			x = x*PI64/180;
			y = y*PI64/180;
		}
		else if (Unit == E_COOR_UNIT_TYPE_RADIAN)
		{
			x = x;
			y = y;
		}
		else
			return FALSE;
	}
	else if (CoorType = E_COOR_SYSTEM_TYPE_PRJ)   // 投影坐标
	{
		// 将单位统一幻化为米
		if (Unit == E_COOR_UNIT_TYPE_METER)
		{
			x = x;
			y = y;
		}
		else if (Unit == E_COOR_UNIT_TYPE_CM)
		{
			x = x/100;
			y = y/100;
		}
		else if (Unit == E_COOR_UNIT_TYPE_MM)
		{
			x = x/1000;
			y = y/1000;
		}
		else
			return FALSE;
	}
	else
		return FALSE;

	return TRUE;
}

int	CHdPJTranslator::UnStandardUnit(int CoorType, int  Unit, double &x, double &y)
{
	// 换算坐标单位
	if (CoorType == E_COOR_SYSTEM_TYPE_GEO)   // 地理坐标
	{
		// 弧度单位进行反转
		if (Unit == E_COOR_UNIT_TYPE_DEGREE)
		{
			x = x*180/PI64;
			y = y*180/PI64;
		}
		else if ( Unit == E_COOR_UNIT_TYPE_RADIAN)
		{
			x = x;
			y = y;
		}
		else
			return FALSE;
	}
	else if (CoorType = E_COOR_SYSTEM_TYPE_PRJ)   // 投影坐标
	{
		// 米单位进行反转
		if (Unit == E_COOR_UNIT_TYPE_METER)
		{
			x = x;
			y = y;
		}
		else if ( Unit == E_COOR_UNIT_TYPE_CM)
		{
			x = x*100;
			y = y*100;
		}
		else if ( Unit == E_COOR_UNIT_TYPE_MM)
		{
			x = x*1000;
			y = y*1000;
		}
		else
			return FALSE;
	}
	else
		return FALSE;

	return TRUE;
}

int CHdPJTranslator::TranslatorByParam( double in_x, double in_y, double in_z, double *out_x, double *out_y, double *out_z )
{
	double _outx, _outy;
	double _inh = 0, _outh = 0;
	double _inx,_iny,_inz;

	// 不成功情况下，直接返回传入值
	*out_x = in_x;
	*out_y = in_y;
	*out_z = in_z;
	_outx = in_x;
	_outy = in_y;
	_outh = in_z;

	_outx = in_y;  _outy = in_x; _outh = in_z;
	if( StandardUnit(m_SrcCoorType, m_SrcUnit, _outx, _outy) == FALSE)
		return FALSE;

	// 此处七参数直接进行转换可能存在问题，需从转换原理考虑验证;
	if ( m_SevenType == E_CONVERT_TYPE_7) // 七参数
	{
		// 传入经纬度，先转换为空间直角坐标系，完成七参数转换，再转换回大地坐标系;
		_inx = _outx; _iny = _outy; _inh = _outh;
		BtoX(m_SrcEa, m_SrcEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);

		// 7参数
		XtoX_Bursa_Simple__(m_SevenPar, &_outx, &_outy, &_outh);

		_inx = _outx;  _iny = _outy;  _inh = _outh;
		XtoB(m_DstEa, m_DstEf, _inx, _iny, _inh, &_outx, &_outy, &_outh);

		//// 7参数;
		//XtoX_Bursa_Simple__(m_SevenPar, &_outx, &_outy, &_outh);

		// 测量坐标系与数字坐标系XY轴是相反的，古此处需要转换;
		*out_x = _outx;
		*out_y = _outy;
		*out_z = _outh;
		return TRUE;
	}

	// 四参数直接进行转换;
	if (m_FourType == E_CONVERT_TYPE_4)
	{
		// 四参数转换;
		xtox(m_FourPar, &_outx, &_outy);

		// 测量坐标系与数字坐标系XY轴是相反的，古此处需要转换;
		*out_x = _outx;
		*out_y = _outy;
		*out_z = _outh;
		return TRUE;
	}

	return TRUE;
}

int CHdPJTranslator::SetHeightFitParam( int model,E_PJHeightFixPar_T* par )
{
	m_HeightFitType = model;
	if (par == NULL || m_HeightFitType == ZHD_HFixEnum_None)	
		return TRUE;

	memcpy(&m_HeightFixPar, par, sizeof(ZHDHFixPar));
	return TRUE;
}

void CHdPJTranslator::HFixCalus( double x, double y, double *dResult )
{
	HFixCalus2(m_HeightFitType,m_HeightFixPar,x,y,dResult);
}

//using namespace hd::geoproj;
//void CHdPJTranslator::BLtoxy_geo( int nModel,int EarthType, ZHDProjPars par, double dB, double dL, double &H, double &dx, double &dy )
//{
//	//1.【将L处理到合理范围-2011-10.9】
//	if (dL>0)
//	{
//		//规算到0到360度
//		int n = (int)(dL / (2.0*PI64));
//		dL = dL - n * 2.0 * PI64;
//	}else
//	{
//		//规算到0到-360度
//		int n = (int)(dL / (-2.0 * PI64));
//		dL = dL + n * 2.0 * PI64;
//	}
//	//2.
//	if (fabs(dL - par.Lo)>PI64)
//	{
//		if (dL>0)
//		{
//			dL = dL - PI64 * 2.0;
//
//		}else
//		{
//			dL = dL + PI64 * 2.0;
//
//		}
//	}
//
//	// 弧度转度后直接调用hdProjectedCoordinateSystem完成坐标转换-2018.10.17;
//	double angle_dL = dL * 180.0 / PI64;
//	double angle_dB = dB * 180.0 / PI64;
//
//	WGS84ToLacal param;
//
//	// 椭球参数设置;
//	param.m_SrcEllipsoid.m_a = m_SrcEa;
//	param.m_SrcEllipsoid.m_f = m_SrcEf;
//	param.m_DstEllipsoid.m_a = m_DstEa;
//	param.m_DstEllipsoid.m_f = m_DstEf;
//
//	param.m_bWGS84ToLocal = false;
//
//	// 设置其他中央经线，偏移量等信息;
//	param.m_ProjParam.m_lfCentralMeridian = m_DstPrjPars.Lo * 180.0 / PI64;
//	param.m_ProjParam.m_lfEastAdd = m_DstPrjPars.FE;
//	param.m_ProjParam.m_lfNorthAdd = m_DstPrjPars.FN;
//
//	param.m_ProjParam.m_lfBm = m_DstPrjPars.Bc;
//	param.m_ProjParam.m_bAddZoneNo = false;
//	param.m_ProjParam.m_lfProjHeight = m_DstPrjPars.PH;
//
//	//投影类型;
//	switch (m_DstZHDPrjType)
//	{
//	case ZHD_ProjectionEnum_Guas3://高斯3
//		param.m_ProjParam.m_eProjType = GAUSS3;
//		param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
//		break;
//	case ZHD_ProjectionEnum_Guass6://高斯6
//		param.m_ProjParam.m_eProjType = GAUSS6;
//		param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
//		break;
//	case ZHD_ProjectionEnum_Mecator:  //墨卡托;
//		param.m_ProjParam.m_eProjType = Mercator;
//		param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
//		break;
//	case ZHD_ProjectionEnum_UTM:  //UTM;
//		param.m_ProjParam.m_eProjType = UTM;
//		param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
//		break;
//	}
//
//	// 坐标转换;
//	Point3d neu;
//	Point3d neu_blh;
//	Point3d neu_new;
//	CHDCoord::TransCoord(Point3d(angle_dB,angle_dL,  H), neu, param);
//
//	dy = neu.E;
//	dx = neu.N;
//	H = neu.U;
//}

//void CHdPJTranslator::xytoBL_geo( int nModel, int EarthType, ZHDProjPars par, double dx, double dy, double dh, double &dB, double &dL, double &dH )
//{
//	WGS84ToLacal param;
//
//	// 椭球参数设置;
//	param.m_SrcEllipsoid.m_a = m_SrcEa;
//	param.m_SrcEllipsoid.m_f = m_SrcEf;
//	param.m_DstEllipsoid.m_a = m_DstEa;
//	param.m_DstEllipsoid.m_f = m_DstEf;
//
//	param.m_bWGS84ToLocal = false;
//
//	// 设置其他中央经线，偏移量等信息;
//	param.m_ProjParam.m_lfCentralMeridian = m_DstPrjPars.Lo * 180.0 / PI64;
//	param.m_ProjParam.m_lfEastAdd = m_DstPrjPars.FE;
//	param.m_ProjParam.m_lfNorthAdd = m_DstPrjPars.FN;
//
//	param.m_ProjParam.m_lfBm = m_DstPrjPars.Bc;
//	param.m_ProjParam.m_bAddZoneNo = false;
//	param.m_ProjParam.m_lfProjHeight = m_DstPrjPars.PH;
//
//	//投影类型;
//	switch (m_DstZHDPrjType)
//	{
//	case ZHD_ProjectionEnum_Guas3://高斯3
//		param.m_ProjParam.m_eProjType = GAUSS3;
//		param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
//		break;
//	case ZHD_ProjectionEnum_Guass6://高斯6
//		param.m_ProjParam.m_eProjType = GAUSS6;
//		param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
//		break;
//	case ZHD_ProjectionEnum_Mecator:  //墨卡托;
//		param.m_ProjParam.m_eProjType = Mercator;
//		param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
//		break;
//	case ZHD_ProjectionEnum_UTM:  //UTM;
//		param.m_ProjParam.m_eProjType = UTM;
//		param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
//		break;
//	}
//
//	// 坐标转换;
//	Point3d neu;
//	Point3d neu_blh;
//	Point3d neu_new;
//	neu.E = dy;
//	neu.N = dx;
//	neu.U = dh;
//
//	CHDCoord::TransCoordReverse_BL(neu,neu_blh,param);
//
//	dL = neu_blh.E;
//	dB = neu_blh.N;
//	dH = neu_blh.U;
//
//
//	//// 转换对象设置;
//	//hdGeographicCoordinateSystem geographic_coord_system;
//	//hdSpatialReferenceH source_coord_system = hdSpatialReferenceCreate(E_SR_PROJ);
//	//hdProjectedCoordinateSystem* ptr_projected_coord_system = static_cast<hdProjectedCoordinateSystem*>(source_coord_system);
//	////hdProjectedCoordinateSystem projected_coord_system;
//	////projected_coord_system.m_proj_name = "gauss";
//	////projected_coord_system.setProjectMethod(GAUSS_KRUGER);
//
//	////hdProjectedCoordinateSystem projected_coord_system;
//
//	//// 大地基准设置;
//	//switch(EarthType){
//	//case E_EARTH_TYPE_Beijing54:
//	//	{
//	//		geographic_coord_system.m_datum_type = BEIJING_1954;
//	//		ptr_projected_coord_system->setGeogCoordSystem(geographic_coord_system);
//	//		break;
//	//	}
//	//case E_EARTH_TYPE_Xian80:
//	//	{
//	//		geographic_coord_system.m_datum_type = XIAN_1980;
//	//		ptr_projected_coord_system->setGeogCoordSystem(geographic_coord_system);
//	//		break;
//	//	}
//	//case E_EARTH_TYPE_China2000:
//	//case E_EARTH_TYPE_WGS84:
//	//	//ptr_geographic_coord_system->m_datum_type = WGS_1984;
//	//	break;
//	//default:
//	//	break;
//	//}
//	//
//	//// 设置投影类型;
//	//switch (nModel)
//	//{
//	//case ZHD_ProjectionEnum_Guas3://高斯3
//	//case ZHD_ProjectionEnum_Guass6://高斯6
//	//case ZHD_ProjectionEnum_Guass_Userdefine://自定义高斯
//	//	ptr_projected_coord_system->setProjectMethod(GAUSS_KRUGER);
//	//	break;
//	//case ZHD_ProjectionEnum_Mecator://莫卡托投影
//	//	ptr_projected_coord_system->setProjectMethod(TRANSVERSE_MERCATOR);
//	//	break;
//	//}
//
//	//// 设置其他中央经线，偏移量等信息;
//	//ptr_projected_coord_system->setCentralMeridian(par.Lo * 180.0 / PI64);
//	//ptr_projected_coord_system->setFalseEasting(par.FE); // 东向偏移量
//	//ptr_projected_coord_system->setFalseNorthing(par.FN); // 北向偏移量
//	//ptr_projected_coord_system->m_mean_latitude = par.Bc; // 平均纬度
//	//ptr_projected_coord_system->m_proj_height = par.PH;  // 投影面高程
//
//	//// 将投影坐标转换经纬度;
//	//double angle_dL = dy;
//	//double angle_dB = dx;
//	////ptr_projected_coord_system->GaussProject_BL2NE(angle_dB,angle_dL);
//	//double degree_latitude,degree_longtitude;
//	//degree_latitude = degree_longtitude = 0.0;
//	//ptr_projected_coord_system->GaussProject_NE2BL(1,&angle_dB,&angle_dL,&degree_latitude,&degree_longtitude);
//
//	//dH = dh;
//	//dB = degree_latitude * PI64 / 180.0;
//	//dL = degree_longtitude * PI64 / 180.0;
//
//	//// 资源释放;
//	//hdSpatialReferenceRelease(source_coord_system);
//	//source_coord_system = NULL;
//}

// 考虑七参数等的经纬度转换投影坐标-接口废弃;
int CHdPJTranslator::TransLators_BLToNE( int ncount,double* degree_latitude, double* degree_longtitude,double* north,double* east )
{
	////::CoInitialize(NULL);

	//WGS84ToLacal param;

	//// 椭球参数设置;
	//param.m_SrcEllipsoid.m_a = m_SrcEa;
	//param.m_SrcEllipsoid.m_f = m_SrcEf;
	//param.m_DstEllipsoid.m_a = m_DstEa;
	//param.m_DstEllipsoid.m_f = m_DstEf;

	//param.m_bWGS84ToLocal = false;

	//// 设置其他中央经线，偏移量等信息;
	//param.m_ProjParam.m_lfCentralMeridian = m_DstPrjPars.Lo * 180.0 / PI64;
	//param.m_ProjParam.m_lfEastAdd = m_DstPrjPars.FE;
	//param.m_ProjParam.m_lfNorthAdd = m_DstPrjPars.FN;

	//param.m_ProjParam.m_lfBm = m_DstPrjPars.Bc;
	//param.m_ProjParam.m_bAddZoneNo = false;
	//param.m_ProjParam.m_lfProjHeight = m_DstPrjPars.PH;

	////投影类型;
	//switch (m_DstZHDPrjType)
	//{
	//case ZHD_ProjectionEnum_Guas3://高斯3
	//	param.m_ProjParam.m_eProjType = GAUSS3;
	//	param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
	//	break;
	//case ZHD_ProjectionEnum_Guass6://高斯6
	//	param.m_ProjParam.m_eProjType = GAUSS6;
	//	param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
	//	break;
	//case ZHD_ProjectionEnum_Mecator:  //墨卡托;
	//	param.m_ProjParam.m_eProjType = Mercator;
	//	param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
	//	break;
	//case ZHD_ProjectionEnum_UTM:  //UTM;
	//	param.m_ProjParam.m_eProjType = UTM;
	//	param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
	//	break;
	//}

	//// 坐标转换;
	//Point3d neu;
	//Point3d neu_blh;
	//for (int n = 0;n < ncount;n++)
	//{
	//	neu_blh.N = degree_latitude[n];
	//	neu_blh.E = degree_longtitude[n];
	//	neu_blh.U = 0.0;

	//	CHDCoord::TransCoord(neu_blh, neu, param);

	//	north[n] = neu.N;
	//	east[n] = neu.E;
	//}

	////::CoUninitialize();

	return 1;

	//dy = neu.E;
	//dx = neu.N;
	//H = neu.U;

	//// 转换对象设置,hdProjectedCoordinateSystem设置setGeogCoordSystem后存在内存泄露;
	//hdGeographicCoordinateSystem geographic_system;
	//hdProjectedCoordinateSystem projected_coord_system;
	//projected_coord_system.m_proj_name = "gauss";

	//// 大地基准设置;
	//switch(m_DstEarthType)
	//{
	//case E_EARTH_TYPE_Beijing54:
	//	{
	//		// ;
	//		geographic_system.m_datum_type = BEIJING_1954;
	//		projected_coord_system.setGeogCoordSystem(geographic_system);
	//		break;
	//	}
	//case E_EARTH_TYPE_Xian80:
	//	{
	//		geographic_system.m_datum_type = XIAN_1980;
	//		projected_coord_system.setGeogCoordSystem(geographic_system);
	//		break;
	//	}
	//case E_EARTH_TYPE_China2000:
	//case E_EARTH_TYPE_WGS84:
	//	{
	//		break;
	//	}
	//default:
	//	{
	//		break;
	//	}
	//}

	//// 设置投影类型;
	//switch (m_DstZHDPrjType)
	//{
	//case ZHD_ProjectionEnum_Guas3://高斯3
	//case ZHD_ProjectionEnum_Guass6://高斯6
	//case ZHD_ProjectionEnum_Guass_Userdefine://自定义高斯
	//	projected_coord_system.setProjectMethod(GAUSS_KRUGER);
	//	break;
	//case ZHD_ProjectionEnum_Mecator://莫卡托投影
	//	projected_coord_system.setProjectMethod(TRANSVERSE_MERCATOR);
	//	break;
	//}

	//// 设置其他中央经线，偏移量等信息;
	//projected_coord_system.setCentralMeridian(m_DstPrjPars.Lo * 180.0 / PI64);
	//projected_coord_system.setFalseEasting(m_DstPrjPars.FE); // 东向偏移量
	//projected_coord_system.setFalseNorthing(m_DstPrjPars.FN); // 北向偏移量
	//projected_coord_system.m_mean_latitude = m_DstPrjPars.Bc; // 平均纬度
	//projected_coord_system.m_proj_height = m_DstPrjPars.PH;  // 投影面高程

	//projected_coord_system.GaussProject_BL2NE(ncount,degree_latitude,degree_longtitude,north,east);

	//return 1;
}

// 考虑七参数等的投影坐标转换经纬度-接口废弃;
int CHdPJTranslator::TransLators_NEToBL( int ncount,double* north, double* east ,double* degree_latitude,double* degree_longtitude )
{
	//WGS84ToLacal param;

	//param.m_SrcEllipsoid.m_a = m_SrcEa;
	//param.m_SrcEllipsoid.m_f = m_SrcEf;
	//param.m_DstEllipsoid.m_a = m_DstEa;
	//param.m_DstEllipsoid.m_f = m_DstEf;

	//if ( m_SevenType == E_CONVERT_TYPE_7)
	//{
	//	param.m_ConvertType = E_CONVERT_TYPE_7PARAM;
	//	param.m_bWGS84ToLocal = true;

	//	// 设置七参数;
	//	TransCoord7Param param_7;
	//	param_7.m_lfDX = m_SevenPar.DX;
	//	param_7.m_lfDY = m_SevenPar.DY;
	//	param_7.m_lfDZ = m_SevenPar.DZ;
	//	param_7.m_lfRX = m_SevenPar.WX * (180.0 * 3600.0) / PI64;
	//	param_7.m_lfRY = m_SevenPar.WY * (180.0 * 3600.0) / PI64;
	//	param_7.m_lfRZ = m_SevenPar.WZ * (180.0 * 3600.0) / PI64;
	//	param_7.m_lfK = m_SevenPar.K;

	//	param.m_TransCoord7Param = param_7;
	//}
	//else if (m_FourType == E_CONVERT_TYPE_4)  // 四参数;
	//{
	//	param.m_ConvertType = E_CONVERT_TYPE_PLANE_HEIGHT;
	//	param.m_bWGS84ToLocal = true;

	//	TransParams4 param_4;
	//	param_4.m_lfE = m_FourPar.Dy;
	//	param_4.m_lfN = m_FourPar.Dx;
	//	param_4.m_lfScale = m_FourPar.K;
	//	param_4.m_lfRotation = m_FourPar.T;

	//	param.m_PlaneTrans4Para = param_4;

	//	TransHeightParams param_height;
	//	param_height.m_lfA = m_HeightFixPar.A;
	//	param_height.m_lfB = m_HeightFixPar.B;
	//	param_height.m_lfC = m_HeightFixPar.C;
	//	param_height.m_lfD = m_HeightFixPar.D;
	//	param_height.m_lfE = m_HeightFixPar.E;
	//	param_height.m_lfF = m_HeightFixPar.F;
	//	param_height.m_lfN0 = m_HeightFixPar.X0;
	//	param_height.m_lfE0 = m_HeightFixPar.Y0;

	//	switch (m_HeightFitType)
	//	{
	//	case 1:
	//		param_height.m_model = CONSTANT; // 固定差改正
	//		break;
	//	case 2:
	//		param_height.m_model = PLANEFIT; // 平面拟合
	//		break;
	//	case 3:
	//		param_height.m_model = QUADRATIC_SURFACE; // 曲面拟合
	//		break;
	//	}

	//	param.m_HeightFit = param_height;
	//}
	//else
	//{
	//	param.m_bWGS84ToLocal = false;
	//}

	//// 设置投影类型;
	//switch (m_DstZHDPrjType)
	//{
	//case ZHD_ProjectionEnum_Guas3://高斯3
	//case ZHD_ProjectionEnum_Guass_Userdefine://自定义高斯
	//	param.m_ProjParam.m_eProjType = GAUSS3;
	//	param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
	//	break;
	//case ZHD_ProjectionEnum_Guass6://高斯6
	//	param.m_ProjParam.m_eProjType = GAUSS6;
	//	param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
	//	break;
	//case ZHD_ProjectionEnum_Mecator://莫卡托投影
	//	param.m_ProjParam.m_eProjType = Mercator;
	//	param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
	//	break;
	//case ZHD_ProjectionEnum_UTM://UTM投影
	//	param.m_ProjParam.m_eProjType = UTM;
	//	param.m_ProjParam.m_lfScale = m_DstPrjPars.Ko;
	//	break;
	//}

	//// 设置其他中央经线，偏移量等信息;
	//param.m_ProjParam.m_lfCentralMeridian = m_DstPrjPars.Lo * 180.0 / PI64;
	//param.m_ProjParam.m_lfEastAdd = m_DstPrjPars.FE;
	//param.m_ProjParam.m_lfNorthAdd = m_DstPrjPars.FN;

	//param.m_ProjParam.m_lfBm = m_DstPrjPars.Bc;
	//param.m_ProjParam.m_bAddZoneNo = false;
	//param.m_ProjParam.m_lfProjHeight = m_DstPrjPars.PH;

	//// 坐标转换;
	////Point3d neu;
	////Point3d neu_blh;
	////Point3d neu_new;
	////CHDCoord::TransCoord(Point3d(in_b,in_l,  in_h), neu, param);
	//// 坐标转换;
	//Point3d neu;
	//Point3d neu_blh;
	//for (int n = 0;n < ncount;n++)
	//{
	//	neu.N = north[n];
	//	neu.E = east[n];
	//	neu.U = 0.0;

	//	CHDCoord::TransCoordReverse_BL(neu, neu_blh, param);

	//	degree_latitude[n] = neu_blh.N;
	//	degree_longtitude[n] = neu_blh.E;
	//}

	////*out_e = neu.E;
	////*out_n = neu.N;
	////*out_h = neu.U;

	return 1;

	//WGS84ToLacal param;

	//// 椭球参数设置;
	//param.m_SrcEllipsoid.m_a = m_SrcEa;
	//param.m_SrcEllipsoid.m_f = m_SrcEf;
	//param.m_DstEllipsoid.m_a = m_DstEa;
	//param.m_DstEllipsoid.m_f = m_DstEf;

	//param.m_bWGS84ToLocal = false;

	//// 设置其他中央经线，偏移量等信息;
	//param.m_ProjParam.m_lfCentralMeridian = m_DstPrjPars.Lo * 180.0 / PI64;
	//param.m_ProjParam.m_lfEastAdd = m_DstPrjPars.FE;
	//param.m_ProjParam.m_lfNorthAdd = m_DstPrjPars.FN;

	//param.m_ProjParam.m_lfBm = m_DstPrjPars.Bc;
	//param.m_ProjParam.m_bAddZoneNo = false;
	//param.m_ProjParam.m_lfProjHeight = m_DstPrjPars.PH;

	//// 坐标转换;
	//Point3d neu;
	//Point3d neu_blh;
	//for (int n = 0;n < ncount;n++)
	//{
	//	neu.N = north[n];
	//	neu.E = east[n];
	//	neu.U = 0.0;

	//	CHDCoord::TransCoordReverse_BL(neu, neu_blh, param);

	//	degree_latitude[n] = neu_blh.N;
	//	degree_longtitude[n] = neu_blh.E;
	//}

	//return 1;


	//// 转换对象设置,hdProjectedCoordinateSystem设置setGeogCoordSystem后存在内存泄露;
	//hdGeographicCoordinateSystem geographic_system;
	//hdProjectedCoordinateSystem projected_coord_system;
	//projected_coord_system.m_proj_name = "gauss";

	//// 大地基准设置;
	//switch(m_DstEarthType)
	//{
	//case E_EARTH_TYPE_Beijing54:
	//	{
	//		// ;
	//		geographic_system.m_datum_type = BEIJING_1954;
	//		projected_coord_system.setGeogCoordSystem(geographic_system);
	//		break;
	//	}
	//case E_EARTH_TYPE_Xian80:
	//	{
	//		geographic_system.m_datum_type = XIAN_1980;
	//		projected_coord_system.setGeogCoordSystem(geographic_system);
	//		break;
	//	}
	//case E_EARTH_TYPE_China2000:
	//case E_EARTH_TYPE_WGS84:
	//	{
	//		break;
	//	}
	//default:
	//	{
	//		break;
	//	}
	//}

	//// 设置投影类型;
	//switch (m_DstZHDPrjType)
	//{
	//case ZHD_ProjectionEnum_Guas3://高斯3
	//case ZHD_ProjectionEnum_Guass6://高斯6
	//case ZHD_ProjectionEnum_Guass_Userdefine://自定义高斯
	//	projected_coord_system.setProjectMethod(GAUSS_KRUGER);
	//	break;
	//case ZHD_ProjectionEnum_Mecator://莫卡托投影
	//	projected_coord_system.setProjectMethod(TRANSVERSE_MERCATOR);
	//	break;
	//}

	//// 设置其他中央经线，偏移量等信息;
	//projected_coord_system.setCentralMeridian(m_DstPrjPars.Lo * 180.0 / PI64);
	//projected_coord_system.setFalseEasting(m_DstPrjPars.FE); // 东向偏移量
	//projected_coord_system.setFalseNorthing(m_DstPrjPars.FN); // 北向偏移量
	//projected_coord_system.m_mean_latitude = m_DstPrjPars.Bc; // 平均纬度
	//projected_coord_system.m_proj_height = m_DstPrjPars.PH;  // 投影面高程

	//projected_coord_system.GaussProject_NE2BL(ncount,north,east,degree_latitude,degree_longtitude);

	//return 1;
}

#define PI_TO_180_DEGREE 57.295779513082320876798154814105
#define D180_TO_PI_RADIAN 0.01745329251994329576923690768489
int CHdPJTranslator::TranslatorBLToNeH( double in_l,double in_b,double in_h,double* out_e,double* out_n,double* out_h,bool bOnly )
{
	if (bOnly)
	{
		// 经纬度转换目标投影坐标;
		BLtoxy(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, in_b * D180_TO_PI_RADIAN, in_l * D180_TO_PI_RADIAN, in_h, 
			*out_n, *out_e, TRUE, TRUE);

		*out_h = in_h;
	}
	//else
	//{
	//	// 可能带七参数等转换信息;
	//	Point3d neu;
	//	m_phdCoord->TransCoord(Point3d(in_b,in_l,  in_h), neu, *m_param);

	//	*out_e = neu.E;
	//	*out_n = neu.N;
	//	*out_h = neu.U;
	//}

	//Point3d neu;
	//m_phdCoord->TransCoord(Point3d(in_b,in_l,  in_h), neu, *m_param);

	//if (abs(neu.E - *out_e) > 0.001 || abs(neu.N - *out_n) > 0.001  || abs(neu.U - *out_h) > 0.001 )
	//{
	//	int test = 0;
	//}

	//*out_e = neu.E;
	//*out_n = neu.N;
	//*out_h = neu.U;

	return 1;
}

// 经纬度转换空间直角坐标系;
int CHdPJTranslator::TransLators_BLToXYZ( double degree_latitude, double degree_longtitude,double phereHeight,double* out_X,double* out_Y,double* out_Z )
{
	// 大地坐标转为空间直角坐标(BLH->XYZ);
	//CHDCoord::BtoX(m_SrcEa, m_SrcEf, degree_latitude * PI64 / 180.0, degree_longtitude * PI64 / 180.0, phereHeight, out_X, out_Y,out_Z);
	BtoX(m_SrcEa, m_SrcEf, degree_latitude * PI64 / 180.0, degree_longtitude * PI64 / 180.0, phereHeight, out_X, out_Y,out_Z);
	return 1;
}

// 空间直角转换经纬度;
int CHdPJTranslator::TransLators_XYZToBL( double srcX,double srcY,double srcZ,double* out_latitude, double* out_longtitude,double* out_phereHeight )
{
	// 空间直角坐标(XYZ-BLH)转为目标经纬度;
	XtoB(m_DstEa, m_DstEf, srcX, srcY, srcZ, out_latitude, out_longtitude,out_phereHeight);
	//CHDCoord::XtoB(m_DstEa, m_DstEf, srcX, srcY, srcZ, out_latitude, out_longtitude,out_phereHeight);
	return 1;
}

int CHdPJTranslator::TranslatorXYZByParam( double in_x, double in_y, double in_z, double *out_x, double *out_y, double *out_z )
{
	double _outx,_outy,_outh;
	_outx = _outy = _outh = 0.0;
	_outx = in_x;
	_outy = in_y;
	_outh = in_z;
	double out_latitude;
	double out_longtitude;
	double out_phereHeight;
	if ( m_SevenType == E_CONVERT_TYPE_7) // 七参数;
	{
		// 先将空间直角坐标系进行7参数转换;
		XtoX_Bursa_Simple__(m_SevenPar, &_outx, &_outy, &_outh);

		// 再转换至目标椭球的经纬度，返回为弧度;
		XtoB(m_DstEa, m_DstEf, _outx, _outy, _outh, &out_latitude, &out_longtitude,&out_phereHeight);

		// 参与计算为弧度,转换至目标椭球投影坐标系;
		BLtoxy(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, out_latitude, out_longtitude, out_phereHeight, 
			_outy, _outx, TRUE, TRUE);

		*out_x = _outx;
		*out_y = _outy;
		*out_z = out_phereHeight;
		return 1;
	}

	// 四参数直接进行转换;
	if (m_FourType == E_CONVERT_TYPE_4)
	{
		// 输入为源椭球空间直角坐标系,先转换至源椭球的经纬度;
		XtoB(m_SrcEa, m_SrcEf, _outx, _outy, _outh, &out_latitude, &out_longtitude,&out_phereHeight);

		// 经纬度转换投影坐标;
		BLtoxy(m_DstZHDPrjType, m_SrcEa, m_SrcEf, m_DstPrjPars, out_latitude, out_longtitude, out_phereHeight, 
			_outy, _outx, TRUE, TRUE);

		// 投影坐标进行四参数转换;
		xtox(m_FourPar, &_outy, &_outx);

		double adResult = 0.0;
		HFixCalus(_outy,_outx,&adResult);

		// 测量坐标系与数字坐标系XY轴是相反的，古此处需要转换;
		*out_x = _outx;
		*out_y = _outy;
		*out_z = out_phereHeight - adResult;
		return 1;
	}

	// 无四参数也无七参数转换;
	XtoB(m_DstEa, m_DstEf, _outx, _outy, _outh, &out_latitude, &out_longtitude,&out_phereHeight);

	// 参与计算为弧度,转换至目标椭球投影坐标系;
	BLtoxy(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, out_latitude, out_longtitude, out_phereHeight, 
		_outy, _outx, TRUE, TRUE);

	*out_x = _outx;
	*out_y = _outy;
	*out_z = out_phereHeight;
	return 1;
}

int CHdPJTranslator::TranslatorXYZByParamInverse(double in_x, double in_y, double in_z, double *out_x, double *out_y, double *out_z)
{
	double _outx, _outy, _outh;
	_outx = _outy = _outh = 0.0;
	_outx = in_x; // x y z 表示为 东 北 高;
	_outy = in_y;
	_outh = in_z;
	double out_latitude;
	double out_longtitude;
	double out_phereHeight;
	if (m_SevenType == E_CONVERT_TYPE_7) // 七参数;
	{
		// 投影坐标转换经纬度;
		xytoBL(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _outy, _outx, _outh, out_latitude, out_longtitude, out_phereHeight, TRUE, TRUE);

		// 经纬度反向转换空间直角坐标;
		BtoX(m_DstEa, m_DstEf, out_latitude, out_longtitude, out_phereHeight, &_outx, &_outy, &_outh);

		// 七参数反向计算;
		XtoX_Bursa_Simple_False__(m_SevenPar, &_outx, &_outy, &_outh);

		*out_x = _outx;
		*out_y = _outy;
		*out_z = _outh;

		return 1;
	}

	// 四参数直接进行转换;
	if (m_FourType == E_CONVERT_TYPE_4)
	{
		_outx = in_x;
		_outy = in_y;

		// 高程拟合值;
		double adResult = 0.0;
		HFixCalus(_outy, _outx, &adResult);
		_outh = _outh + adResult;

		// 投影坐标反向四参数转换，传入应为北东;
		xtox_false(m_FourPar, &_outy, &_outx);

		// 投影坐标转换经纬度;
		xytoBL(m_DstZHDPrjType, m_SrcEa, m_SrcEf, m_DstPrjPars,_outy,_outx,_outh,out_latitude,out_longtitude,out_phereHeight, TRUE, TRUE);

		// 源椭球的经纬度转换至源椭球的空间直角;
		BtoX(m_SrcEa, m_SrcEf, out_latitude, out_longtitude, out_phereHeight,&_outx,&_outy,&_outh);

		*out_x = _outx;
		*out_y = _outy;
		*out_z = _outh;
		return 1;
	}

	// 投影坐标先转换目标椭球的经纬度;
	xytoBL(m_DstZHDPrjType, m_DstEa, m_DstEf, m_DstPrjPars, _outy, _outx, _outh, out_latitude, out_longtitude, out_phereHeight,TRUE, TRUE);

	// 无四参数也无七参数转换，经纬度直接转换空间直角坐标;
	BtoX(m_DstEa, m_DstEf, out_latitude, out_longtitude, out_phereHeight, &_outx, &_outy, &_outh);

	*out_x = _outx;
	*out_y = _outy;
	*out_z = _outh;
	return 1;
}

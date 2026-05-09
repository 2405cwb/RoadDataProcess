/*! hdGeoPositionAPI.cpp
********************************************************************************
<PRE>
模块名       : HDARXCommand
文件名       : hdGeoPositionAPI.cpp
相关文件     : hdGeoPositionAPI.h
文件实现功能 : 对坐标转换相关操作的接口以及参数传递的接口封装
作者         : 谢卓
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2016/03/31   1.0      谢卓               新增    
</PRE>
*******************************************************************************/
#include "StdAfx.h"
#include "./GeoProjection/inc/IHdPJTranslator.h"
#include "hdGeoPositionAPI.h"
#include "hdLonLatParm.h"

#define  PI 3.14159265359

// 坐标转换:WGS84 高斯3度带经纬度转大地坐标
HDGEOPOSITION_APIS void TranslateDegree2Gauss(double longitude, double latitude, double& dX_OUT, double& dY_OUT)
{
	double           m_dCenterB = 0;			    // 中央子午经线
	Spatial_Ref_t    m_srcParam;				// 原始数据转换参数
	Spatial_Ref_t    m_dstParam;				// 目标数据转换参数

	IHdPJTranslator* pHdTranslator = NULL;	// 数据转换对象
	CreateIHdPJTranslator(&pHdTranslator);

	// 定义两个临时变量
	double dZ_IN = 0;
	double dZ_OUT = 0;

	// 投影所在中央经线
	int dCenterB = (int)(longitude / 3 + 0.5);
	m_dCenterB = dCenterB * 3;

	// 经纬度转高斯投影
	// 设置原始数据属性
	memset(&m_srcParam, 0, sizeof(m_srcParam));
	m_srcParam.coorSystem = E_COOR_SYSTEM_TYPE_GEO;
	m_srcParam.coorUnit = E_COOR_UNIT_TYPE_DEGREE;
	m_srcParam.earthType = E_EARTH_TYPE_WGS84;
	pHdTranslator->SetSrcSpatialRef(&m_srcParam);

	// 设置目标数据属性
	memset(&m_dstParam, 0, sizeof(m_dstParam));
	m_dstParam.coorSystem = E_COOR_SYSTEM_TYPE_PRJ;
	m_dstParam.coorUnit = E_COOR_UNIT_TYPE_METER;
	m_dstParam.earthType = E_EARTH_TYPE_WGS84;
	m_dstParam.prjType = E_PROJECT_TYPE_Gauss_Kruger;
	m_dstParam.Lo = m_dCenterB * PI / 180;
	m_dstParam.Ko = 1.0;
	m_dstParam.FE = 500000;
	m_dstParam.W= 3;
	pHdTranslator->SetDstSpatialRef(&m_dstParam);

	// 坐标转换
	pHdTranslator->Translator(longitude, latitude, dZ_IN, &dX_OUT, &dY_OUT, &dZ_OUT);
	DestroyIHdPJTranslator(pHdTranslator);
}

// 经纬度转高斯WGS84投影坐标
HDGEOPOSITION_APIS void GaussToBLToGaussExt(double longitude, double latitude,double dCenter,double& x,double& y,double& z)
{
	double           m_dCenterB = 0;			    // 中央子午经线
	Spatial_Ref_t    m_srcParam;				// 原始数据转换参数
	Spatial_Ref_t    m_dstParam;				// 目标数据转换参数

	// 中央经线换算
	int nL = (int)(dCenter/3 + 0.5);
	nL *= 3;

	int nL2 = (int)( longitude / 3 + 0.5);
	nL2 *= 3;

	// 特定中央经线114
	if (dCenter == 114)
	{
		nL = nL2;
	}

	IHdPJTranslator* pHdTranslator = NULL;	// 数据转换对象
	CreateIHdPJTranslator(&pHdTranslator);

	// 定义两个临时变量
	double dZ_IN = 0;
	double dZ_OUT = 0;

	// 设置原始数据属性
	memset(&m_srcParam, 0, sizeof(m_srcParam));
	m_srcParam.coorSystem = E_COOR_SYSTEM_TYPE_GEO;
	m_srcParam.coorUnit = E_COOR_UNIT_TYPE_DEGREE;
	m_srcParam.earthType = E_EARTH_TYPE_WGS84;
	pHdTranslator->SetSrcSpatialRef(&m_srcParam);

	// 设置目标数据属性
	memset(&m_dstParam, 0, sizeof(m_dstParam));
	m_dstParam.coorSystem = E_COOR_SYSTEM_TYPE_PRJ;
	m_dstParam.coorUnit = E_COOR_UNIT_TYPE_METER;
	m_dstParam.earthType = E_EARTH_TYPE_WGS84;
	m_dstParam.prjType = E_PROJECT_TYPE_Gauss_Kruger;
	m_dstParam.Lo = nL * PI / 180;
	m_dstParam.Ko = 1.0;
	m_dstParam.FE = 500000;
	pHdTranslator->SetDstSpatialRef(&m_dstParam);

	// 坐标转换
	pHdTranslator->Translator(longitude,latitude, dZ_IN, &x, &y, &z);
	DestroyIHdPJTranslator(pHdTranslator);
}

// WGS84高斯投影坐标转经纬度
HDGEOPOSITION_APIS void GaussToBLToGauss(double longitude, double latitude,double dCenter,double& x,double& y)
{
	double           m_dCenterB = 0;			// 中央子午经线
	Spatial_Ref_t    m_srcParam;				// 原始数据转换参数
	Spatial_Ref_t    m_dstParam;				// 目标数据转换参数

	// 中央经线计算
	int nL = (int)(dCenter/3 + 0.5);
	nL *= 3;

	int nL2 = (int)(longitude / 3 + 0.5);
	nL2 *= 3;

	if (dCenter == 114)
	{
		nL = nL2;
	}

	IHdPJTranslator* pHdTranslator = NULL;	// 数据转换对象
	CreateIHdPJTranslator(&pHdTranslator);

	// 定义两个临时变量
	double dZ_IN = 0;
	double dZ_OUT = 0;

	// 设置原始数据属性
	memset(&m_srcParam, 0, sizeof(m_srcParam));
	m_srcParam.coorSystem = E_COOR_SYSTEM_TYPE_GEO;
	m_srcParam.coorUnit = E_COOR_UNIT_TYPE_DEGREE;
	m_srcParam.earthType = E_EARTH_TYPE_WGS84;
	pHdTranslator->SetSrcSpatialRef(&m_srcParam);

	// 设置目标数据属性
	memset(&m_dstParam, 0, sizeof(m_dstParam));
	m_dstParam.coorSystem = E_COOR_SYSTEM_TYPE_PRJ;
	m_dstParam.coorUnit = E_COOR_UNIT_TYPE_METER;
	m_dstParam.earthType = E_EARTH_TYPE_WGS84;
	m_dstParam.prjType = E_PROJECT_TYPE_Gauss_Kruger;
	m_dstParam.Lo=nL*PI/180;
	m_dstParam.Ko = 1.0;
	m_dstParam.FE = 500000;
	//// 设置为3度带 2015/06/04 luowenmin
	m_dstParam.W=3;
	pHdTranslator->SetDstSpatialRef(&m_dstParam);

	double z = 0.0;
	// 坐标转换
	pHdTranslator->Translator(longitude,latitude, dZ_IN, &x, &y, &z);
	DestroyIHdPJTranslator(pHdTranslator);
}

// WGS84高斯投影坐标转经纬度
HDGEOPOSITION_APIS void BLToGaussToGauss( double x,double y,double z,double dCenter,double& longitude, double& latitude )
{
	double           m_dCenterB = 0;			// 中央子午经线
	Spatial_Ref_t    m_srcParam;				// 原始数据转换参数
	Spatial_Ref_t    m_dstParam;				// 目标数据转换参数

	// 中央经线
	int nL = dCenter;

	IHdPJTranslator* pHdTranslator = NULL;	// 数据转换对象
	CreateIHdPJTranslator(&pHdTranslator);

	// 定义两个临时变量
	double dZ_IN = 0;
	double dZ_OUT = 0;

	// 设置原始数据属性
	memset(&m_srcParam, 0, sizeof(m_srcParam));
	m_srcParam.coorSystem = E_COOR_SYSTEM_TYPE_PRJ;
	m_srcParam.coorUnit = E_COOR_UNIT_TYPE_METER;
	m_srcParam.earthType = E_EARTH_TYPE_WGS84;
	m_srcParam.prjType = E_PROJECT_TYPE_Gauss_Kruger;
	m_srcParam.Lo = nL * PI / 180;
	m_srcParam.Ko = 1.0;
	m_srcParam.FE = 500000;
	pHdTranslator->SetSrcSpatialRef(&m_srcParam);

	// 设置目标数据属性
	memset(&m_dstParam, 0, sizeof(m_dstParam));
	m_dstParam.coorSystem = E_COOR_SYSTEM_TYPE_GEO;
	m_dstParam.coorUnit = E_COOR_UNIT_TYPE_DEGREE;
	m_dstParam.earthType = E_EARTH_TYPE_WGS84;
	pHdTranslator->SetDstSpatialRef(&m_dstParam);

	// 坐标转换
	pHdTranslator->Translator(x, y, dZ_IN, &longitude, &latitude, &dZ_OUT);
	DestroyIHdPJTranslator(pHdTranslator);
}

// 坐标转换:WGS84 大地坐标转高斯3度带经纬度
HDGEOPOSITION_APIS void TranslateGauss2Degree(double dX_IN, double dY_IN, double dCenter, double& longitude, double& latitude)
{
	double           m_dCenterB = 0;			// 中央子午经线
	Spatial_Ref_t    m_srcParam;				// 原始数据转换参数
	Spatial_Ref_t    m_dstParam;				// 目标数据转换参数

	IHdPJTranslator* pHdTranslator = NULL;	// 数据转换对象
	CreateIHdPJTranslator(&pHdTranslator);

	// 定义两个临时变量
	double dZ_IN = 0;
	double dZ_OUT = 0;
	// 计算中央经线	dCenter = (int)(dCenter/3 +0.5);
	int dCenterB = (int)(dCenter / 3 + 0.5);
	m_dCenterB = dCenterB * 3;
	// 高斯投影转经纬度
	// 设置原始数据属性
	memset(&m_srcParam, 0, sizeof(m_srcParam));
	m_srcParam.coorSystem = E_COOR_SYSTEM_TYPE_PRJ;
	m_srcParam.coorUnit = E_COOR_UNIT_TYPE_METER;
	m_srcParam.earthType = E_EARTH_TYPE_WGS84;
	m_srcParam.prjType = E_PROJECT_TYPE_Gauss_Kruger;
	m_srcParam.Lo = m_dCenterB * PI / 180;
	m_srcParam.Ko = 1.0;
	m_srcParam.W = 3;
	m_srcParam.FE = 500000;
	pHdTranslator->SetSrcSpatialRef(&m_srcParam);

	// 设置目标数据属性
	memset(&m_dstParam, 0, sizeof(m_dstParam));
	m_dstParam.coorSystem = E_COOR_SYSTEM_TYPE_GEO;
	m_dstParam.coorUnit = E_COOR_UNIT_TYPE_DEGREE;
	m_dstParam.earthType = E_EARTH_TYPE_WGS84;
	pHdTranslator->SetDstSpatialRef(&m_dstParam);	

	// 坐标转换
	pHdTranslator->Translator(dX_IN, dY_IN, dZ_IN, &longitude, &latitude, &dZ_OUT);
	DestroyIHdPJTranslator(pHdTranslator);
}

// 经纬度纠偏	--xz
HDGEOPOSITION_APIS void MapCorrect(double dL, double dB, double& out_dlParm, double& out_dbParm)
{
	hd::CHdLonLatParm pram;
	pram.ReadLonLarParm(); 
	double dlParm = 0;
	double dbParm = 0;
	pram.Query(dL, dB, dlParm, dbParm);
	out_dlParm = dlParm;
	out_dbParm = dbParm;
}
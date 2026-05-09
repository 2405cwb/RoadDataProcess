/*! PositionTranfrom.cpp
********************************************************************************
<PRE>
模块名       : hdPositionTran
文件名       : PositionTranfrom.cpp
相关文件     : PositionTranfrom.h
文件实现功能 : 地理坐标转换，计算
作者         : 研发部 熊勇钢
版本         : 1.0
版权		 : CopyRight @ 2013 海达数云
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容  
2015/10/10   1.0      熊勇钢                新建
2015/11/4    1.1      熊勇钢                新增根据hdi返回解算投影带接口
</PRE>
*******************************************************************************/

#include "StdAfx.h"
#include "PositionTranfrom.h"
#include "./GeoProjection/inc/IHdPJTranslator.h"

#define  PI 3.14159265359

CGeoPositionTran::CGeoPositionTran(void)
{
	m_dCenterB = 0.0;
}

CGeoPositionTran::~CGeoPositionTran(void)
{

}

// 经纬度转高斯WGS84投影坐标
void  CGeoPositionTran::GaussToBLToGauss(double longitude, double latitude,double dCenter,double& x,double& y,double& z)
{
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
void  CGeoPositionTran::GaussToBLToGauss(double longitude, double latitude,double dCenter,double& x,double& y)
{
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
void  CGeoPositionTran::BLToGaussToGauss( double x,double y,double z,double dCenter,double& longitude, double& latitude )
{
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

// 坐标转换:WGS84 高斯3度带经纬度转大地坐标
void CGeoPositionTran::TranslateDegree2Gauss(double longitude, double latitude, double& dX_OUT, double& dY_OUT)
{
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

// 坐标转换:WGS84 大地坐标转高斯3度带经纬度
void CGeoPositionTran::TranslateGauss2Degree(double dX_IN, double dY_IN, double dCenter, double& longitude, double& latitude)
{
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

// 通过对应的经纬度和投影坐标得到高斯三度带投影代号
int CGeoPositionTran::GetGauss3ProjectionZoneNO(double longitude,	double latitude,double x,double y) 
{
	// 投影带号
	int nZoneNO = 0;

	// 计算当前经线对应的中央经线	dCenter = (int)(dCenter/3 +0.5);
	int dCenterB = (int)(longitude / 3.0 + 0.5);
	m_dCenterB = dCenterB * 3;

	// hdi中记录xyz坐标与其记录经纬度坐标采用各自内插系统获得，存在差异，此处仅为计算投影带号，不需相关检验。--add by zhubo 2016.09.07
	nZoneNO = (int)(m_dCenterB / 3);
	return nZoneNO;

	//FILE* pFile = fopen("D:\\TEST.txt","a+t");
	//fprintf_s(pFile,"%d,%lf\n",dCenterB,m_dCenterB);

    // 数据转换对象，为内部new出来的，创建后应析构-add by zhubo 20160114
	IHdPJTranslator* pHdTranslator = NULL;	
	CreateIHdPJTranslator(&pHdTranslator);

	// 定义两个临时变量
	double dZ_IN = 0;
	double dX_OUT = 0;
	double dY_OUT = 0;
	double dZ_OUT = 0;

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

	// 进行经纬度坐标转大地坐标
	pHdTranslator->Translator(longitude, latitude, dZ_IN, &dX_OUT, &dY_OUT, &dZ_OUT);

	//nZoneNO = (int)(m_dCenterB / 3);
	//fprintf_s(pFile,"%d,%lf\n",nZoneNO,x - dX_OUT);
	//fclose(pFile);

	// 对比对应大地坐标与转化坐标是否统一，暂时将0.1修改为1.0 -add by zhubo 20160114
	if (abs(x - dX_OUT) < 1.0)
	{
		// 析构转化对象
		DestroyIHdPJTranslator(pHdTranslator);

		nZoneNO = (int)(m_dCenterB / 3);
		return nZoneNO;
	}
	else
	{
		m_dCenterB -= 3;
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

		// 进行经纬度坐标转大地坐标
		pHdTranslator->Translator(longitude, latitude, dZ_IN, &dX_OUT, &dY_OUT, &dZ_OUT);

		if (abs(x - dX_OUT) < 1.0)
		{
			// 析构转化对象
			DestroyIHdPJTranslator(pHdTranslator);

			nZoneNO = (int)(m_dCenterB / 3);
			return nZoneNO;
		}
		else
		{
			m_dCenterB += 6;
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

			// 进行经纬度坐标转大地坐标
			pHdTranslator->Translator(longitude, latitude, dZ_IN, &dX_OUT, &dY_OUT, &dZ_OUT);

			if (abs(x - dX_OUT) < 1.0)
			{
				// 析构转化对象
				DestroyIHdPJTranslator(pHdTranslator);

				nZoneNO = (int)(m_dCenterB / 3);
				return nZoneNO;
			}
			else
			{
				// 析构转化对象
				DestroyIHdPJTranslator(pHdTranslator);
				return 0;
			}
		}
	}
    
	// 析构转化对象
	DestroyIHdPJTranslator(pHdTranslator);
}
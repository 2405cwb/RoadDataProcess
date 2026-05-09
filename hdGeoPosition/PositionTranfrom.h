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
#pragma once
#include <vector>
#include "hdPositionTrandef.h"
#include "GeoProjection/inc/IHdPJTranslator.h"

using namespace std;

class HDGEOPOSITION_APIS CGeoPositionTran
{
public:
	// 构造函数
	CGeoPositionTran(void);
	// 析构函数
	~CGeoPositionTran(void);
public:
	// WGS84高斯投影转经纬度
	void TranslateDegree2Gauss(
		double longitude,           // 经度
		double latitude,            // 纬度
		double& dX_OUT,             // 投影坐标X
		double& dY_OUT);            // 投影坐标Y

	// WGS84经纬度转高斯投影
	void TranslateGauss2Degree(
		double dX_IN,               // 投影坐标X
		double dY_IN,               // 投影坐标Y
		double dCenter,             // 对应中央经线
		double& longitude,          // 经度
		double& latitude);          // 纬度

	// 经纬度转高斯WGS84投影坐标
	void GaussToBLToGauss(
		double longitude,           // 经度
		double latitude,            // 纬度
		double dCenter,             // 对应中央经线
		double& x,                  // 投影坐标X
		double& y,                  // 投影坐标Y
		double& z);                 // 投影坐标Z

	// 经纬度转高斯WGS84投影坐标
	void GaussToBLToGauss(
		double longitude,           // 经度
		double latitude,            // 纬度
		double dCenter,             // 对应中央经线
		double& x,                  // 投影坐标X
		double& y);                 // 投影坐标Y

	// WGS84高斯投影坐标转经纬度
	void BLToGaussToGauss(
		double x,                   // 投影坐标X
		double y,                   // 投影坐标Y
		double z,                   // 投影坐标Z
		double dCenter,             // 对应中央经线
		double& longitude,          // 经度
		double& latitude);          // 纬度

	// 通过对应的经纬度和投影坐标得到高斯三度带投影代号
	int GetGauss3ProjectionZoneNO(
		double longitude,          // 经度
		double latitude,           // 纬度
		double x,                  // 投影坐标X
		double y);                 // 投影坐标Y

public:
	double           m_dCenterB;			    // 中央子午经线
	Spatial_Ref_t    m_srcParam;				// 原始数据转换参数
	Spatial_Ref_t    m_dstParam;				// 目标数据转换参数
};


/*! hdGeoPositionAPI.h
********************************************************************************
<PRE>
模块名       : HDARXCommand
文件名       : hdGeoPositionAPI.h
相关文件     : hdGeoPositionAPI.cpp
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
#include "hdPositionTrandef.h"
#include "GeoProjection/inc/IHdPJTranslator.h"

extern "C"  
{
	// WGS84高斯投影转经纬度
	HDGEOPOSITION_APIS void TranslateDegree2Gauss(
		double longitude,           // 经度
		double latitude,            // 纬度
		double& dX_OUT,             // 投影坐标X
		double& dY_OUT);            // 投影坐标Y

	// WGS84经纬度转高斯投影
	HDGEOPOSITION_APIS void TranslateGauss2Degree(
		double dX_IN,               // 投影坐标X
		double dY_IN,               // 投影坐标Y
		double dCenter,             // 对应中央经线
		double& longitude,          // 经度
		double& latitude);          // 纬度

	// 经纬度转高斯WGS84投影坐标
	HDGEOPOSITION_APIS void GaussToBLToGaussExt(
		double longitude,           // 经度
		double latitude,            // 纬度
		double dCenter,             // 对应中央经线
		double& x,                  // 投影坐标X
		double& y,                  // 投影坐标Y
		double& z);                 // 投影坐标Z

	// 经纬度转高斯WGS84投影坐标
	HDGEOPOSITION_APIS void GaussToBLToGauss(
		double longitude,           // 经度
		double latitude,            // 纬度
		double dCenter,             // 对应中央经线
		double& x,                  // 投影坐标X
		double& y);                 // 投影坐标Y

	// WGS84高斯投影坐标转经纬度
	HDGEOPOSITION_APIS void BLToGaussToGauss(
		double x,                   // 投影坐标X
		double y,                   // 投影坐标Y
		double z,                   // 投影坐标Z
		double dCenter,             // 对应中央经线
		double& longitude,          // 经度
		double& latitude);          // 纬度

	HDGEOPOSITION_APIS void MapCorrect(
		double dL,					// 精度
		double dB, 					// 纬度
		double& dlParm,				// 精度偏差
		double& dbParm);				// 纬度偏差
}
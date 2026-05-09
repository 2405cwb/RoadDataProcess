#pragma once
#include "hnqtcommon_global.h"
typedef HNQTCOMMON_EXPORT struct
{
	// 1 设置中央经线,单位度点度;
	  double dCenterL;

	// 2 设置椭球体，0表示北京54,1表示西安80,2表示WGS84,3表示CGCS2000;
	 int nSphereType;

	//3 投影方法设置，0表示高斯三度带投影，1表示高斯6度带，2表示墨卡托投影，3表示横轴墨卡托投影;
	 int nProjectType;

	//4 设置投影面高程;
	 double dProjectHeight;

	// 5 东向加常数;
	 double dEastAdd;

	//6 平均纬度;
	 double dAverageLat;

	//7 尺度因子;
	 double dProjectScale;

	//8 是否使用七参数或四参数转换，1表示使用四参数，2表示使用七参数，0表示不使用;
	 int nUseConvertModel;

	// 四参数设置,不考虑高程;
	   double dFourX;
	   double dFourY;
	   double dFourR;
	   double dFourK;

	//9 七参数值设置;
	   double dOffsetX;
	   double dOffsetY;
	   double dOffsetZ;
	   double dRotateX;
	   double dRotateY;
	   double dRotateZ;
	   double dK;
}POS_CONVERT_INFO;
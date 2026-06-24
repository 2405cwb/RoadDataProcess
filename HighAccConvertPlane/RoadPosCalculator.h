#pragma once

#define   EARTH_WGS84_EA	6378137
#define   EARTH_WGS84_EF	298.2572236
#define PI_M      3.141592653589793238462643383279
#include "hnhighAccConvertPlane_global.h"
class HIGHACCCONVERTPLANE_EXPORT RoadPosCalculator
{
public:
	RoadPosCalculator() =default;
	~RoadPosCalculator() = default;

	// dCurLon当前路面拍照时刻对应的经度，单位为度；
	// dCurLat当前路面拍照时刻对应的纬度，单位为度；
	// dLastLon前一张路面拍照时刻对应的经度(bInverse为false)，单位为度（当当前拍照时刻为第一张时，dLastLon可传入第二张的经纬度，同时标记bInverse 为true）；
	// dLastLat前一张路面拍照时刻对应的纬度，单位为度；
	// returnLon返回的当前路面中心位置的经度;
	// returnLat返回的当前路面中心位置的纬度;
	// 传入经纬度均为度点度值，dOffsetX为天线中心定义的车体坐标系（X轴指向前进方向车右方，应设置为0，
	//即硬件上天线中心与路面破损中心在前进方向一条直线上，减少计算量，Y轴指向前进方向车前方，Z轴指天）;
	   bool  calcLatToPicCenter(double dCurLon, double dCurLat, double dCurHeight, double dLastLon, double dLastLat, double dLastHeight,
		double dOffsetX, double dOffsetY, double dOffsetZ, double& returnLon,  double& returnLat,  double& returnHeight, bool bInverse =false);


	   //// 传入当前路面图片中心点的经纬高;
	   //// 前一个（下一个）图片中心点的经纬高;
	   //// 图片目标点像素x（x对应宽） y（y对应高）;
	   //// 图片像素宽高;
	   //// bInverse标记是否反向;
	   //// 图片拍照实际宽高，宽为2米，高默认3.75米; 
	    bool  calcLatToPicPos(bool showGps, double dCurPicLon, double dCurPicLat, double dCurPicH,
		   double dLastPicLon, double dLastPicLat, double dLastPicH,
		   int picX, int picY, int picWidth, int picHeight,
		   double& returnLon, double& returnLat, double& returnHeight
			,int equip = 0,  bool bInverse=false, double dWidth=2.0, double dHeight=3.75);


private:
	void Gauss_Btox(double a, double f, double h, double Bm, double B, double L, double L0, double& m_x, double& m_y);

	void Gauss_xtoB(double a, double f, double h, double Bm, double x, double y, double L0, double& m_B, double& m_L);


};


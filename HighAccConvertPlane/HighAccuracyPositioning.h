#pragma once
#include <QString> 
#include "../hnProject/hnProject.h"
#include "hnhighAccConvertPlane_global.h"
#include "..\hnQtCommon\MapGPSMile.h"
#include "..\hnQtCommon\ExcelGPS.h"
class HIGHACCCONVERTPLANE_EXPORT  HighAccuracyPositioning
{
public:
	HighAccuracyPositioning(hnPro::hnProject* project);
	~HighAccuracyPositioning();
	//根据高精度定位信息，计算每张图片中心位置gps写入文件中
	bool writeHighAccPicture();
	 
	// showGps 是否以经纬度格式输出 
	bool getHighAccPosition(bool showGps, int equipType, double nowMile, int curPosX, int curPosY, 
		 double& dDiseaseLon, double& dDiseaseLat,  double& dDiseaseH);

	int findMileIndexSorted(const QVector<_EXCELGPS_>&vecotr, double _mile, double epsilon = 1e-6);

	bool mileEqual(double a, double b, double epsilon = 1e-6);
private:
	hnPro::hnProject* m_project;

	QVector<_EXCELGPS_> gpsInfos;
};


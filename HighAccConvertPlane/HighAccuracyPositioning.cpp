#include "HighAccuracyPositioning.h"  
#include <QFile>
#include <QSettings>
#include <algorithm>
#include <cmath>
#include "..\hnProject\hn2DProject.h"
#include "..\hnProject\hnProject.h"

#include "..\hnQtCommon\MyCommonMethods.h"
#include "RoadPosCalculator.h"

namespace
{
	bool isValidLonLat(double lon, double lat)
	{
		return std::abs(lon) > 1.0 && std::abs(lat) > 1.0;
	}

	bool isValidGps(const GPSInfo& gps)
	{
		return isValidLonLat(gps._longitude, gps._latitude);
	}

	bool isValidGps(const _EXCELGPS_& gps)
	{
		return isValidLonLat(gps._longitude, gps._latitude);
	}

	bool isSameGpsPosition(const GPSInfo& a, const GPSInfo& b)
	{
		const double epsilon = 1e-9;
		return std::abs(a._longitude - b._longitude) < epsilon
			&& std::abs(a._latitude - b._latitude) < epsilon
			&& std::abs(a._elevation - b._elevation) < epsilon;
	}

	bool isSameGpsPosition(const _EXCELGPS_& a, const _EXCELGPS_& b)
	{
		const double epsilon = 1e-9;
		return std::abs(a._longitude - b._longitude) < epsilon
			&& std::abs(a._latitude - b._latitude) < epsilon
			&& std::abs(a._elevation - b._elevation) < epsilon;
	}

	int findNearestDifferentGpsIndex(const QVector<MapGPSMile>& gpsinfos, int curIdx)
	{
		if (curIdx < 0 || curIdx >= gpsinfos.size())
		{
			return -1;
		}

		const GPSInfo& currentGps = gpsinfos[curIdx]._gpsinfo;
		for (int offset = 1; offset < gpsinfos.size(); ++offset)
		{
			int prevIdx = curIdx - offset;
			if (prevIdx >= 0
				&& isValidGps(gpsinfos[prevIdx]._gpsinfo)
				&& !isSameGpsPosition(currentGps, gpsinfos[prevIdx]._gpsinfo))
			{
				return prevIdx;
			}

			int nextIdx = curIdx + offset;
			if (nextIdx < gpsinfos.size()
				&& isValidGps(gpsinfos[nextIdx]._gpsinfo)
				&& !isSameGpsPosition(currentGps, gpsinfos[nextIdx]._gpsinfo))
			{
				return nextIdx;
			}
		}

		return -1;
	}

	int findNearestValidGpsIndex(const QVector<_EXCELGPS_>& gpsInfos, int startIndex)
	{
		if (startIndex < 0 || startIndex >= gpsInfos.size())
		{
			return -1;
		}
		if (isValidGps(gpsInfos[startIndex]))
		{
			return startIndex;
		}

		for (int offset = 1; offset < gpsInfos.size(); ++offset)
		{
			int prevIdx = startIndex - offset;
			if (prevIdx >= 0 && isValidGps(gpsInfos[prevIdx]))
			{
				return prevIdx;
			}

			int nextIdx = startIndex + offset;
			if (nextIdx < gpsInfos.size() && isValidGps(gpsInfos[nextIdx]))
			{
				return nextIdx;
			}
		}

		return -1;
	}

	int findNearestValidReferenceIndex(const QVector<_EXCELGPS_>& gpsInfos, int curIdx)
	{
		if (curIdx < 0 || curIdx >= gpsInfos.size() || !isValidGps(gpsInfos[curIdx]))
		{
			return -1;
		}

		for (int offset = 1; offset < gpsInfos.size(); ++offset)
		{
			int prevIdx = curIdx - offset;
			if (prevIdx >= 0
				&& isValidGps(gpsInfos[prevIdx])
				&& !isSameGpsPosition(gpsInfos[curIdx], gpsInfos[prevIdx]))
			{
				return prevIdx;
			}

			int nextIdx = curIdx + offset;
			if (nextIdx < gpsInfos.size()
				&& isValidGps(gpsInfos[nextIdx])
				&& !isSameGpsPosition(gpsInfos[curIdx], gpsInfos[nextIdx]))
			{
				return nextIdx;
			}
		}

		return -1;
	}
}

HighAccuracyPositioning::HighAccuracyPositioning(hnPro::hnProject* project)
{
	m_project = project;
	QString projectPath = m_project->get2DProject()->getBasePath();
	QString outHighGpstxtPath = projectPath + "/HighGps2Mile.txt";
	if (!QFile::exists(outHighGpstxtPath))
	{
		return;
	}
	 gpsInfos = m_project->get2DProject()->getGpsInfoList();
	//根据桩号找到对应中心位置
}

HighAccuracyPositioning::~HighAccuracyPositioning()
{
} 

 
bool HighAccuracyPositioning::writeHighAccPicture()
{

	QVector< QString> allHighAccInfo;
	QVector<MapGPSMile> gpsinfos;//获得定位信息
	//获取工程图片和桩号对应关系
	QVector<hnMile> miles = m_project->getCurrentMileVector();
	QString projectPath = m_project->get2DProject()->getBasePath();
	QString mileFile = projectPath + "/GPS2Mile.txt";
	QString outHighGpstxtPath = projectPath + "/HighGps2Mile.txt";

	 if (QFile::exists(mileFile))
	 {
		 QStringList gps2Mile = MyCommonMethods::ReadAllLines(mileFile);
		 int imgCount = qMin (miles.size(), gps2Mile.size());
		 for (int i =  0 ; i<imgCount; ++i)
		 {
			 QString curStr = gps2Mile[i];
			 MapGPSMile curGpsInfo(curStr); 
			 gpsinfos.push_back(curGpsInfo);
		 }
		 //获取参数
		 QString iniPath = projectPath + "/GPSModel/Config.ini";
		 double dYOffsetLength = 2.61;
		 double dZOffsetLength = 1.8;
		 double dXOffsetLength = 0;
		 QSettings settings(iniPath, QSettings::IniFormat);
		 if (settings.contains("YOffset"))
		 {
			 dYOffsetLength = settings.value("YOffset", dYOffsetLength).toDouble();
		 }
		 if (settings.contains("XOffset"))
		 {
			 dXOffsetLength = settings.value("XOffset", dXOffsetLength).toDouble();
		 }
		 if (settings.contains("ZOffset"))
		 {
			 dZOffsetLength = settings.value("ZOffset", dZOffsetLength).toDouble();
		 }
		 for (int i = 0; i < imgCount; ++i)
		 {
			 int curIdx = i;
			 int closestIndex = 0;
			 int closestIndex1 = 0;
			 double dCenterLon = 0, dCenterLat = 0, dCenterH = 0; //当前图像的经纬高
			 MapGPSMile gpsPre;
			 MapGPSMile gpsNow;
			 bool bInverse = false;
			if (curIdx == 0 )//第一张图像
			{
				//根据图片桩号获得 对应经纬度 
				//第一张图像的经纬度 
				closestIndex = 0;
				closestIndex1 = 1;  
				bInverse = true;
			}
			else
			{
				closestIndex = curIdx;
				closestIndex1 = curIdx-1; 
				bInverse = false;
			}
			gpsNow = gpsinfos[closestIndex];
			gpsPre = gpsinfos[closestIndex1];
			
			QString line; 
			QString nowMile = QString::number(gpsNow._mile);
			QString nowDmi = QString::number(gpsNow._dmi);
			RoadPosCalculator calculator;
			bool calcOk = false;
			if (isValidGps(gpsNow._gpsinfo) && isValidGps(gpsPre._gpsinfo))
			{
				calcOk = calculator.calcLatToPicCenter(gpsNow._gpsinfo._longitude, gpsNow._gpsinfo._latitude, gpsNow._gpsinfo._elevation,
					gpsPre._gpsinfo._longitude, gpsPre._gpsinfo._latitude, gpsPre._gpsinfo._elevation, dXOffsetLength, dYOffsetLength, dZOffsetLength,
					dCenterLon, dCenterLat, dCenterH, bInverse);
			}
			if (!calcOk)
			{
				int refIndex = findNearestDifferentGpsIndex(gpsinfos, curIdx);
				if (refIndex >= 0)
				{
					gpsPre = gpsinfos[refIndex];
					bInverse = refIndex > curIdx;
					calcOk = calculator.calcLatToPicCenter(gpsNow._gpsinfo._longitude, gpsNow._gpsinfo._latitude, gpsNow._gpsinfo._elevation,
						gpsPre._gpsinfo._longitude, gpsPre._gpsinfo._latitude, gpsPre._gpsinfo._elevation, dXOffsetLength, dYOffsetLength, dZOffsetLength,
						dCenterLon, dCenterLat, dCenterH, bInverse);
				}
			}
			if (!calcOk)
			{
				dCenterLon = gpsNow._gpsinfo._longitude;
				dCenterLat = gpsNow._gpsinfo._latitude;
				dCenterH = gpsNow._gpsinfo._elevation;
			}
		QString time = 	gpsNow._gpsinfo._utctime.time().toString("HHmmsszzz");
			line = QString("%1 %2 %3 %4 %5 %6")
				.arg(time)
				.arg(dCenterLon,0,'f',7).arg(dCenterLat,0,'f',7).arg(dCenterH,0,'f',2)
				.arg(nowDmi)
				.arg(nowMile);
			allHighAccInfo.push_back(line);
		 }
		 if (allHighAccInfo.size()>0)
		 {
			 //写入
			 MyCommonMethods::writeAllLines(outHighGpstxtPath, allHighAccInfo);
		 }
	 }
	 else
	 {
		 return false;

	 }
	 
	 return true;

}

bool HighAccuracyPositioning::getHighAccPosition(bool showGps, int equipType, double nowMile,
	int curPosX, int curPosY,  double& dDiseaseLon, double& dDiseaseLat, double& dDiseaseH)
{
	dDiseaseLon = 0;
	dDiseaseLat = 0;
	dDiseaseH = 0;

	if (m_project == nullptr || m_project->get2DProject() == nullptr)
	{
		return false;
	}

	int pictureW = m_project->get2DProject()->getRoadPictureWidth();
	int pictureH = m_project->get2DProject()->getRoadPictureHeight();
	double	roadWidth = m_project->getCurProSetInfo().dRoadWidth;
	double	roadLength = m_project->getCurProSetInfo().dRoadLength;
	//获得高精度定位信息
	QString projectPath = m_project->get2DProject()->getBasePath(); 
	if (gpsInfos.size() <= 0)
	{
		QString outHighGpstxtPath = projectPath + "/HighGps2Mile.txt";
		if (!QFile::exists(outHighGpstxtPath))
		{
			return false;
		}
		gpsInfos = m_project->get2DProject()->getGpsInfoList();
	}
	if (gpsInfos.size() < 2)
	{
		return false;
	}
	
	//根据桩号找到对应中心位置
	if (equipType == 1)
	{
		curPosX = pictureW - curPosX;
	}

	bool bInverse = false;

	int closestIndex = findMileIndexSorted(gpsInfos, nowMile);
	if (closestIndex < 0 || closestIndex >= gpsInfos.size())
	{
		return false;
	}

	closestIndex = findNearestValidGpsIndex(gpsInfos, closestIndex);
	if (closestIndex < 0)
	{
		return false;
	}

	int closestIndex1 = findNearestValidReferenceIndex(gpsInfos, closestIndex);
	if (closestIndex1 < 0)
	{
		return false;
	}
	bInverse = closestIndex1 > closestIndex;

	double dCenterLon = gpsInfos[closestIndex]._longitude;
	double dCenterLat = gpsInfos[closestIndex]._latitude;
	double dCenterH = gpsInfos[closestIndex]._elevation;

	double lastCenterlon = gpsInfos[closestIndex1]._longitude;
	double lastCenterlat = gpsInfos[closestIndex1]._latitude;
	double lastCenterH = gpsInfos[closestIndex1]._elevation;

	RoadPosCalculator calculator;
	bool calcOk = calculator.calcLatToPicPos(showGps,
		dCenterLon, dCenterLat, dCenterH,
		lastCenterlon, lastCenterlat, lastCenterH,
		curPosX, curPosY, pictureW, pictureH, dDiseaseLon, dDiseaseLat, dDiseaseH, equipType, bInverse, roadWidth, roadLength);
	if (!calcOk)
	{
		return false;
	}

	dDiseaseLon = QString("%1").arg(dDiseaseLon,0, 'f', 7).toDouble();
	dDiseaseLat = QString("%1").arg(dDiseaseLat, 0,'f', 7).toDouble();
	dDiseaseH = QString("%1").arg(dDiseaseH, 0,'f', 2).toDouble();

	return true;
}

int HighAccuracyPositioning::findMileIndexSorted(const QVector<_EXCELGPS_>&vecotr, double _mile, double epsilon /*= 1e-6*/)
{
	if (vecotr.isEmpty())
	{
		return -1;
	}
	if (vecotr.size() == 1)
	{
		return 0;
	}

	const bool ascending = vecotr.first()._mile <= vecotr.last()._mile;
	QVector<_EXCELGPS_>::const_iterator it;
	if (ascending)
	{
		it = std::lower_bound(vecotr.begin(), vecotr.end(), _mile, [](const _EXCELGPS_& obj, double value)
		{
			return obj._mile < value;
		});
	}
	else
	{
		it = std::lower_bound(vecotr.begin(), vecotr.end(), _mile, [](const _EXCELGPS_& obj, double value)
		{
			return obj._mile > value;
		});
	}

	if (it == vecotr.begin())
	{
		return 0;
	}
	if (it == vecotr.end())
	{
		return vecotr.size() - 1;
	}

	int rightIndex = static_cast<int>(std::distance(vecotr.begin(), it));
	int leftIndex = rightIndex - 1;
	if (std::abs(vecotr[rightIndex]._mile - _mile) < std::abs(vecotr[leftIndex]._mile - _mile))
	{
		return rightIndex;
	}
	return leftIndex;
}

bool HighAccuracyPositioning::mileEqual(double a, double b, double epsilon /*= 1e-6*/)
{
	return std::abs(a - b) < epsilon;
}

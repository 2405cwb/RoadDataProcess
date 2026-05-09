#include "HighAccuracyPositioning.h"  
#include <QFile>
#include <QSettings>
#include "..\hnProject\hn2DProject.h"
#include "..\hnProject\hnProject.h"

#include "..\hnQtCommon\MyCommonMethods.h"
#include "RoadPosCalculator.h";

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
			calculator.calcLatToPicCenter(gpsNow._gpsinfo._longitude, gpsNow._gpsinfo._latitude, gpsNow._gpsinfo._elevation,
				gpsPre._gpsinfo._longitude, gpsPre._gpsinfo._latitude, gpsPre._gpsinfo._elevation, dXOffsetLength, dYOffsetLength, dZOffsetLength,
				dCenterLon, dCenterLat, dCenterH,bInverse);
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

void HighAccuracyPositioning::getHighAccPosition(bool showGps, int equipType, double nowMile,
	int curPosX, int curPosY,  double& dDiseaseLon, double& dDiseaseLat, double& dDiseaseH)
{
	int pictureW = m_project->get2DProject()->getRoadPictureWidth();
	int pictureH = m_project->get2DProject()->getRoadPictureHeight();
	double	roadWidth = m_project->getCurProSetInfo().dRoadWidth;
	double	roadLength = m_project->getCurProSetInfo().dRoadLength;
	//获得高精度定位信息
	QString projectPath = m_project->get2DProject()->getBasePath(); 
	if (gpsInfos.size()<=0)
	{
		QString outHighGpstxtPath = projectPath + "/HighGps2Mile.txt";
		if (!QFile::exists(outHighGpstxtPath))
		{
			return;
		}
		  gpsInfos = m_project->get2DProject()->getGpsInfoList();
	}
	
	//根据桩号找到对应中心位置
	if (equipType == 0)
	{

	}
	else if (equipType == 1)
	{
		curPosX = pictureW - curPosX;
	}


	bool bInverse = false;

	int closestIndex = 0;
	int closestIndex1 = 0;
	double dCenterLon = 0, dCenterLat = 0, dCenterH = 0; //当前图像的经纬高
	double lastCenterlon = 0, lastCenterlat = 0, lastCenterH = 0;//后一张图像中心的经维高 

	closestIndex = findMileIndexSorted(gpsInfos, nowMile);
	if (closestIndex ==-1)
	{
		return;
	}
	if (closestIndex == 0)
	{
		closestIndex1 = 1;
		bInverse = true;
	}
	else
	{
		closestIndex1 = closestIndex - 1;
	}

	dCenterLon = gpsInfos[closestIndex]._longitude;
	dCenterLat = gpsInfos[closestIndex]._latitude;
	dCenterH = gpsInfos[closestIndex]._elevation;

	lastCenterlon = gpsInfos[closestIndex1]. _longitude;
	lastCenterlat = gpsInfos[closestIndex1]._latitude;
	lastCenterH = gpsInfos[closestIndex1]._elevation;

	RoadPosCalculator calculator;
	calculator.calcLatToPicPos(showGps, 
		dCenterLon,dCenterLat,dCenterH,
		lastCenterlon,lastCenterlat,lastCenterH ,
		curPosX,curPosY,pictureW,pictureH,dDiseaseLon,dDiseaseLat,dDiseaseH,equipType, bInverse,roadWidth,roadLength);

	dDiseaseLon = QString("%1").arg(dDiseaseLon,0, 'f', 7).toDouble();
	dDiseaseLat = QString("%1").arg(dDiseaseLat, 0,'f', 7).toDouble();
	dDiseaseH = QString("%1").arg(dDiseaseH, 0,'f', 2).toDouble();


}

int HighAccuracyPositioning::findMileIndexSorted(const QVector<_EXCELGPS_>&vecotr, double _mile, double epsilon /*= 1e-6*/)
{
	if (vecotr.isEmpty())
	{
		return -1;
	}
	auto it = std::lower_bound(vecotr.begin(), vecotr.end(), _mile, [epsilon](const _EXCELGPS_&obj, double value)
	{
		return obj._mile < value - epsilon; //严格小于value - epsilon
	}
	);
	if (it != vecotr.end() && mileEqual(it->_mile, _mile,epsilon))
	{
		return std::distance(vecotr.begin(), it);
	}
	if (it!=vecotr.begin())
	{
		--it;
		if (mileEqual(it->_mile, _mile, epsilon))
		{
			return std::distance(vecotr.begin(), it);
		}
	}
	if (it!= vecotr.end() && it!=vecotr.begin())
	{
		//获得距离最近的
		it = std::lower_bound(vecotr.begin(), vecotr.end(), _mile, [epsilon](const _EXCELGPS_&obj, double value)
		{
			return obj._mile < value ; 
		});
		return std::distance(vecotr.begin(), it);
	}
	return -1;
}

bool HighAccuracyPositioning::mileEqual(double a, double b, double epsilon /*= 1e-6*/)
{
	return std::abs(a - b) < epsilon;
}

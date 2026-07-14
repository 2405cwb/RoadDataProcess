#include "stdafx.h"
#include "hnCalRoadGeometry.h"
#include <io.h>
#include "..\hnPavementCreate3d\hnPavementCamReader.h"
#include "..\hnPavementCreate3d\hnPavementImageInfo.h"
#include "..\hnApplication\hnDataManager.h"
#include "..\hnProject\hnProject.h"
#include "..\hnProject\hn3DProject.h"
#include <QDir>
#include<cmath>
using namespace hnApp;
using namespace hnPro;

hnCalRoadGeometry::hnCalRoadGeometry()
{
	memset(m_strPos, 0, 1024);
}


hnCalRoadGeometry::~hnCalRoadGeometry()
{
}

// 设置标定参数 dBaseRoll-横向标定参数(单位：度)；dBasePitch-俯仰标定参数(单位：度）,strPos-pos文件的路径
void hnCalRoadGeometry::setBaseParam(double dBaseRoll, double dBasePitch, const char* strPos)
{
	// 清空已有隧道pos信息
	if (m_vecPosInfo.size() > 0)
	{
		m_vecPosInfo.clear();
	}

	m_dBasePitch = dBasePitch;
	m_dBaseRoll = dBaseRoll;

	strcpy(m_strPos, strPos);
}

//// 根据输入的时间计算出相应位置横坡、纵坡以及曲率半径
//bool hnCalRoadGeometry::calRoadGeometery(vector<hnRoadGeoParam>& vecGeoParam, double dDist, bool(*pProgress)(float fVal, const char* qstrName, bool bCancle)/* = NULL*/)
//{
//	if (pProgress)
//	{
//		pProgress(0.0, "加载POS", false);
//	}
//
//	// 加载POS
//	loadPosData(m_strPos,pProgress);
//
//	if (m_vecPosInfo.size() <= 3)
//	{
//		return false;
//	}
//
//	
//	if (pProgress)
//	{
//		pProgress(0.9, "解算道路几何状态", false);
//	}
//
//	// 回调进度
//	float fProgress = 0.0;
//
//	for (int i = 0; i < vecGeoParam.size(); i++)
//	{
//		if (pProgress)
//		{
//			if (i % 10 == 0)
//			{
//				fProgress = 0.6 + (i + 1) / vecGeoParam.size();
//
//				if (fProgress >= 0.9)
//				{
//					fProgress = 0.9;
//				}
//
//				pProgress(fProgress, "解算道路几何状态", false);
//			}
//		}
//
//        // 根据GPS 查找POS信息
//		dichotomyFindNearestLoc(m_vecPosInfo, vecGeoParam[i].dGpsTimer, vecGeoParam[i]);
//
//		// 计算横坡
//		vecGeoParam[i].dHAngle = tan(((vecGeoParam[i].dRoll - m_dBaseRoll) / 180.0 * 3.1415929));
//
//		// 计算纵坡
//		vecGeoParam[i].dVAngle = tan(((vecGeoParam[i].dPitch - m_dBasePitch) / 180.0 * 3.1415929));
//	}
//
//	// 计算曲率半径
//	for (int i = 1; i < vecGeoParam.size(); i++)
//	{
//		if (pProgress)
//		{
//			if (i % 10 == 0)
//			{
//				fProgress = 0.9 + (i + 1) / vecGeoParam.size();
//
//				if (fProgress >= 1.0)
//				{
//					fProgress = 0.99;
//				}
//
//				pProgress(fProgress, "解算道路几何状态", false);
//			}
//		}
//
//		// 计算曲率
//		vecGeoParam[i].dC = 1.0 / (3.1415926*(vecGeoParam[i].dYaw - vecGeoParam[i - 1].dYaw) / (180.0 * dDist));
//	}
//
//	vecGeoParam[0].dC = vecGeoParam[1].dC;
//
//	if (pProgress)
//	{
//		pProgress(1.0, "解算道路几何状态", false);
//	}
//}

// 根据输入的时间计算出相应位置横坡、纵坡以及曲率半径
bool hnCalRoadGeometry::calRoadGeometeryNew(vector<hnRoadGeoParam>& vecGeoParam, double dDist, bool(*pProgress)(float fVal, const char* qstrName, bool bCancle)/* = NULL*/)
{
	if (pProgress)
	{
		pProgress(0.0, "加载POS", false);
	}

	auto* curProject = hnDataManager::getDataManager()->getCurrentProject();
	hnCalRoadGeometry calRoadGeo;
	double dBaseRoll = 0.0;
	double dBasePitch = 0.0;
	QString basePath = curProject->get3DProPath();
	
    //	加载POS
	loadPosData(m_strPos, pProgress);

	if (m_vecPosInfo.size() <= 3)
	{
		return false;
	}

	hnPavementCamReader *  camReader = curProject->get3DProject()->getPavementCamReader();
	hn::hnProjectSetting setting;
	setting.setProjectDir(m_strPos);
	m_creatImage.setSystemSetting(&setting);

	std::vector<POINT_STRUCT_XYZIT_INFO> pionts;
	int return_pt_count;

	int sumFrame = 5000;
	int mainFrame = sumFrame / 40;
	int subRrame = sumFrame - (mainFrame * 40);

	// 临时三维点云
	vector<hnPoint3d> vecPt;

	// 单个断面的中心点
	hnPoint3d ptCenter;

	double dMinValue = 10000.0;
	double dTempDist = 0.0;
	// dangqian
	POS_STRUCT_INFO curPosInfo;
	int nearestIndex = 0;
	bool bSucc = false;

	// 遍历所有传入信息
	for (int i = 0; i < vecGeoParam.size(); i++)
	{
		sumFrame = vecGeoParam[i].dMileage / 0.002;
		mainFrame = sumFrame / 40;
		subRrame = sumFrame - (mainFrame * 40);

		//
		dMinValue = 10000.0;
		camReader->getSubFramePoints(mainFrame, subRrame, 0, pionts, return_pt_count);

		vecPt.resize(return_pt_count);

		//获取有效点数据及中心点
		for (int j = 0; j<return_pt_count;j++)
		{
			// 转换三维点云
			vecPt[j].x = pionts[j].x;
			vecPt[j].y = pionts[j].y;
			vecPt[j].z = pionts[j].z;

			bSucc = m_creatImage.linearInsertPos(pionts[j].timeSecond,  m_vecPosInfo, curPosInfo, nearestIndex);
			m_creatImage.updatePosMatrix(curPosInfo);
			m_creatImage.calcuCoord(curPosInfo, pionts[j], vecPt[j].x, vecPt[j].y, vecPt[j].z);

			// 获取航向角
			if (j == 0)
			{
				vecGeoParam[i].dYaw = curPosInfo.dHeading;
			}

			// 获取中心点三维坐标
			dTempDist = sqrt((pionts[j].x * pionts[j].x + pionts[j].z*pionts[j].z));
			if (dTempDist < dMinValue)
			{
				dMinValue = dTempDist;
				ptCenter = vecPt[j];
			}
			
			vecPt[j].y = 0.0;
		}

		// 计算横坡 --滤波+拟合获取横坡
		int tempDDD = 0;
	}


	if (pProgress)
	{
		pProgress(0.9, "解算道路几何状态", false);
	}

	// 回调进度
	float fProgress = 0.0;

	//
	for (int i = 0; i < vecGeoParam.size(); i++)
	{
		if (pProgress)
		{
			if (i % 10 == 0)
			{
				fProgress = 0.6 + (i + 1) / vecGeoParam.size();

				if (fProgress >= 0.9)
				{
					fProgress = 0.9;
				}

				pProgress(fProgress, "解算道路几何状态", false);
			}
		}

		// 计算纵坡
		if (i == vecGeoParam.size() - 1)
		{
			vecGeoParam[i].dVAngle = vecGeoParam[i - 1].dVAngle;
		}
		else
		{
			vecGeoParam[i].dVAngle = (vecGeoParam[i + 1].pt.z - vecGeoParam[i].pt.z) / (vecGeoParam[i + 1].pt.y - vecGeoParam[i].pt.y);
		}
		
	}

	// 计算曲率半径
	for (int i = 1; i < vecGeoParam.size(); i++)
	{
		if (pProgress)
		{
			if (i % 10 == 0)
			{
				fProgress = 0.9 + (i + 1) / vecGeoParam.size();

				if (fProgress >= 1.0)
				{
					fProgress = 0.99;
				}

				pProgress(fProgress, "解算道路几何状态", false);
			}
		}

		// 计算曲率
		vecGeoParam[i].dC = 1.0 / (3.1415926*(vecGeoParam[i].dYaw - vecGeoParam[i - 1].dYaw) / (180.0 * dDist));
	}

	vecGeoParam[0].dC = vecGeoParam[1].dC;

	if (pProgress)
	{
		pProgress(1.0, "解算道路几何状态", false);
	}
}

bool sortbyZ(hnPoint3d begPt, hnPoint3d endPt)
{
	return begPt.z < endPt.z;
}

// 根据输入的时间计算出相应位置横坡、纵坡以及曲率半径
bool hnCalRoadGeometry::calRoadGeometeryNew1(vector<hnRoadGeoParam>& vecGeoParam, double dDist, bool(*pProgress)(float fVal, const char* qstrName, bool bCancle)/* = NULL*/)
{
	if (pProgress)
	{
		pProgress(0.0, "加载POS", false);
	}

	auto* curProject = hnDataManager::getDataManager()->getCurrentProject();
	hnCalRoadGeometry calRoadGeo;
	double dBaseRoll = 0.0;
	double dBasePitch = 0.0;
	QString basePath = curProject->get3DProPath();

	////	加载POS
	//loadPosData(m_strPos, pProgress);

	//if (m_vecPosInfo.size() <= 3)
	//{
	//	return false;
	//}

	//1.读POS得到经纬度// 加载POS数据至内存
	m_coordinate.loadPosData(m_strPos, m_vecPosInfo, pProgress);

	QString basePath1 = curProject->get3DProPath() + "/" + curProject->get3DProName() ;
	//QDir dir(basePath1);
//	dir.cdUp();
//	QString parentPath = dir.absolutePath();
	//QString sep = QDir::separator();
	QString qstrIscanFile = basePath1  +  "/Mms-Para.db";

	auto tempPath = qstrIscanFile.toLocal8Bit();
	auto tempPath2 = tempPath.toStdString();

	//2.读config获得延迟时间18s 并将延迟传出去  用于帧和POS的时间差   设置扫描的年月日信息,读取ISACN参数  构造按iscan旋转角度方式Z-X-Y构建旋转矩阵; 
	//读db得到点云到惯导的内部旋转矩阵
	m_coordinate.setiScanParaPath(0, tempPath2.c_str(), m_PtsToPos);

	hnPavementCamReader *  camReader = curProject->get3DProject()->getPavementCamReader();
	hn::hnProjectSetting setting;
	setting.setProjectDir(m_strPos);
	m_creatImage.setSystemSetting(&setting);
	int scanCount = camReader->GetScanLines();
	double scanLength = scanCount * 40 * 0.002;
	std::vector<POINT_STRUCT_XYZIT_INFO> pionts;
	int return_pt_count;

	int sumFrame = 5000;
	int mainFrame = sumFrame / 40;
	int subRrame = sumFrame - (mainFrame * 40);

	// 临时三维点云
	vector<hnPoint3d> vecPt;

	// 单个断面的中心点
	hnPoint3d ptCenter;

	double dMinValue = 10000.0;
	double dTempDist = 0.0;
	// dangqian
	POS_STRUCT_INFO curPosInfo;
	int nearestIndex = 0;
	bool bSucc = false;

	QVector<double > tempsZP;
	QVector<double > tempsTime;
	// 遍历所有传入信息

	if (pProgress)
	{
		pProgress(0.4, "解算道路几何状态", false);
	}
	for (int i = 0; i < vecGeoParam.size(); i++)
	{
		if (i * 10  >= scanLength)
		{
			continue;
		}

		if (pProgress)
		{
			float startOffset = 0.4f;
			float weight = 1.0f;
			float fProgress = startOffset + (static_cast<float>(i) / vecGeoParam.size()) *weight;

			if (fProgress >= 0.9)
			{
				fProgress = 0.9;
			}
			pProgress(fProgress, "转换三维点云，计算纵坡，横坡", false);
		}

		// 纵坡点云数据0.1米取一帧数据
		int nSlopecnt = dDist / 0.1;
		QVector<hnPoint3d> vecSlopePts;
		hnPoint3d ptSlope;
		for (int j = 0; j < nSlopecnt; j++)
		{
			sumFrame = (vecGeoParam[i].dMileage + j *0.1) / 0.002;
			mainFrame = sumFrame / 40;
			subRrame = sumFrame - (mainFrame * 40);

			// 获取点云
			camReader->getSubFramePoints(mainFrame, subRrame, 0, pionts, return_pt_count);

			vecPt.resize(return_pt_count);

			int nValitCnt = 0;

			for (int k = 0; k < return_pt_count; k++)
			{
				if (pionts[k].x > 0.01 || pionts[k].x < -0.01)
				{
					continue;
				}

				// 转换三维点云
				vecPt[nValitCnt].x = pionts[k].x;
				vecPt[nValitCnt].y = pionts[k].y;
				vecPt[nValitCnt].z = pionts[k].z;
				pt2D23D(vecPt[nValitCnt], pionts[k].timeSecond, 0.0, 0.0, curPosInfo);

				nValitCnt++;
			}

			if (nValitCnt == 0)
			{
				vecGeoParam[i].dVAngle = 0.0;
				continue;
			}

			vecPt.resize(nValitCnt);

			// 排序
			sort(vecPt.begin(), vecPt.end(), sortbyZ);

			nValitCnt = 0;
			ptSlope.z = 0.0;
			for (int k = vecPt.size()*0.05; k < vecPt.size() * 0.95; k++)
			{
				ptSlope.z += vecPt[k].z;
				nValitCnt++;
			}

			ptSlope.z = ptSlope.z / nValitCnt;
			
			ptSlope.x = j * 0.1;

			vecSlopePts.push_back(ptSlope);
		}

		double	k = 0.0;
		double b = 0.0;
		double crossSlopePercent = 0.0;

		// 计算纵坡
		if (vecSlopePts.size() > 0)
		{
			//滤波
			QVector<hnPoint3d> newPoints11 = filterCloudByScore(vecSlopePts);
			//拟合直线
			calculateLaneCrossSlop(newPoints11, 0.02, 500, k, b, crossSlopePercent);

			vecGeoParam[i].dVAngle = k;
		}


        // 计算横坡
		sumFrame = vecGeoParam[i].dMileage / 0.002;
		mainFrame = sumFrame / 40;
		subRrame = sumFrame - (mainFrame * 40);

		//
		dMinValue = 10000.0;
		
		camReader->getSubFramePoints(mainFrame, subRrame, 0, pionts, return_pt_count);

		vecPt.resize(return_pt_count); 
		int midIndex = 0; 
		//获取有效点数据及中心点
		for (int j = 0; j < return_pt_count; j++)
		{
			// 转换三维点云
			vecPt[j].x = pionts[j].x;
			vecPt[j].y = pionts[j].y;
			vecPt[j].z = pionts[j].z;

			/*	bSucc = m_creatImage.linearInsertPos(pionts[j].timeSecond, m_vecPosInfo, curPosInfo, nearestIndex);
				m_creatImage.updatePosMatrix(curPosInfo);
				m_creatImage.calcuCoord(curPosInfo, pionts[j], vecPt[j].x, vecPt[j].y, vecPt[j].z);*/

			pt2D23D(vecPt[j], pionts[j].timeSecond, 0.0, 0.0, curPosInfo);

			// 获取航向角
			if (j == 0)
			{
				vecGeoParam[i].dYaw = curPosInfo.dHeading;
				tempsZP.push_back(curPosInfo.dHeading);
				tempsTime.push_back(pionts[j].timeSecond);
			}
	 

			// 获取中心点三维坐标
			dTempDist = sqrt(pionts[j].x * pionts[j].x);
			if (dTempDist < dMinValue)
			{
				dMinValue = dTempDist;
				ptCenter = vecPt[j];
				midIndex = j;
			}

			//vecPt[j].y = 0.0;
		}

		//计算高程 中点附近 左右取50个点  取中位数
		int startVAngleIndex = midIndex -50;
		if (midIndex - 50 <0)
		{
			startVAngleIndex = 0;
		}
		int endVangeleIndex = midIndex + 50;
		if (midIndex +50 >= return_pt_count)
		{
			endVangeleIndex = return_pt_count-1;
		}
		QVector<double> zValues; 
		for (int startIdx = startVAngleIndex ; startIdx < endVangeleIndex ; ++startIdx)
		{
			zValues.append(vecPt[startIdx].z);
		}
		std::sort(zValues.begin(), zValues.end());
		double stableZ = zValues[zValues.size() / 2];
		QVector<double> realZValues;
		for (int tempZIdx = 10; tempZIdx < zValues.size() -10 ; ++ tempZIdx)
		{
			realZValues.append( zValues[tempZIdx]);
		} 
		vecGeoParam[i].pt = ptCenter;

		vecGeoParam[i].pt.z = std::accumulate(realZValues.begin(), realZValues.end(), 0.0) / realZValues.size(); 

		int pointSize = vecPt.size();
		
		k = 0.0;
		b = 0.0;
		crossSlopePercent = 0.0;

		// 计算横坡 --滤波+拟合获取横坡 
		QVector<hnPoint3d> newPoints0 = filterCloudByScore(QVector<hnPoint3d>::fromStdVector(vecPt));
		for (size_t i = 0; i < newPoints0.size(); i++)
		{
			if (i==0 )
			{
				newPoints0[i].x = 0;
			}
			else
			{
				double value = (vecPt[i - 1].x - vecPt[i].x) * (vecPt[i - 1].x - vecPt[i].x) + (vecPt[i - 1].y - vecPt[i].y) * (vecPt[i - 1].y - vecPt[i].y);
				newPoints0[i].x = std::sqrt(value)+newPoints0[i-1].x;
			}
			newPoints0[i].y = 0;
		}

		//滤波
		QVector<hnPoint3d> newPoints = 	filterCloudByScore(newPoints0);
		//拟合直线
		
		calculateLaneCrossSlop(newPoints, 0.02, 500,k,b,crossSlopePercent);
		//MidianAverageFileter(ArrayHeight, 0, pointSize-1, pointSize, ArrayHeight);
		//float* correctH = new float[pointSize]();
		//LSLineFit_New(ArrayHeight, ArrayHeightX,0, pointSize-1, m_k, m_b);

		vecGeoParam[i].dHAngle = k;
	}



	if (pProgress)
	{
		pProgress(0.9, "解算道路几何状态", false);
	}

	// 回调进度
	float fProgress = 0.0;
	for (int i = 0; i < vecGeoParam.size(); i++)
	{
		if (pProgress)
		{
			if (i % 10 == 0)
			{
				fProgress = 0.95 + (i + 1) / vecGeoParam.size();

				if (fProgress >= 0.95)
				{
					fProgress = 0.95;
				}

				pProgress(fProgress, "计算纵坡", false);
			}
		}
		////取五十个点的平均值
		//// 计算纵坡
		//if (i == vecGeoParam.size() - 1)
		//{
		//	vecGeoParam[i].dVAngle = vecGeoParam[i - 1].dVAngle;
		//}
		//else
		//{
		//	double dx = vecGeoParam[i + 1].pt.x - vecGeoParam[i].pt.x;
		//	double dy = vecGeoParam[i + 1].pt.y - vecGeoParam[i].pt.y;
		//	double dist = std::sqrt(dx*dx + dy*dy);
		//	//double dist = dy;
		//	double slope = (vecGeoParam[i + 1].pt.z - vecGeoParam[i].pt.z) / dist;
		//	vecGeoParam[i].dVAngle = slope;
		//	//vecGeoParam[i].dVAngle = (vecGeoParam[i + 1].pt.z - vecGeoParam[i].pt.z) / (vecGeoParam[i + 1].pt.y - vecGeoParam[i].pt.y);
		//}

	}

	// 计算曲率半径
	for (int i = 1; i < vecGeoParam.size(); i++)
	{
		if (pProgress)
		{
			if (i % 10 == 0)
			{
				fProgress = 0.95 + (i + 1) / vecGeoParam.size();

				if (fProgress >= 1.0)
				{
					fProgress = 0.99;
				}

				pProgress(fProgress, "计算曲率", false);
			}
		}
	 

		// 计算曲率
	//vecGeoParam[i].dC = /*1.0 /*/ (3.1415926*(vecGeoParam[i].dYaw - vecGeoParam[i - 1].dYaw) / (180.0 * dDist));
	
	
	double deltaYaw = vecGeoParam[i].dYaw - vecGeoParam[i-1].dYaw;

	while (deltaYaw>180.0)
	{
		deltaYaw -= 360.0;
	}
	while (deltaYaw <-180.0)
	{
		deltaYaw += 360.0;

	}

		//计算曲率
		double kappa = (deltaYaw * M_PI /180.0) / dDist;

		vecGeoParam[i].dC = kappa; 
	}

	vecGeoParam[0].dC = vecGeoParam[1].dC;

	if (pProgress)
	{
		pProgress(1.0, "解算道路几何状态", false);
	}
	return true;
}

bool hnCalRoadGeometry::calSlope(hnRoadGeoParam& vecGeoParam, double& dSlope)
{
	// 
	return true;
}

//1.读POS得到经纬度// 加载POS数据至内存
bool hnCalRoadGeometry::loadPosData(const char* strPosPath, bool(*pProgress)(float fVal, const char* qstrName, bool bCancle))
{
	// 检查文件是否存在
	if (_access(strPosPath, 0) != 0)
	{
		return 0;
	}

	// 中间文件用于读取数据
	char strData[1024];
	memset(strData, 0, 1024);

	// 读取文件
	int file_line_count = 0;
	bool bFindData = false;
	FILE* ptrFile = fopen(strPosPath, "rt");
	while (!feof(ptrFile))
	{
		fgets(strData, 1024, ptrFile);
		file_line_count++;
	}
	fclose(ptrFile);
	ptrFile = fopen(strPosPath, "rt");

	// 读取第一行数据
	fgets(strData, 1024, ptrFile);
	string strLine = strData;
	int nPos = strLine.find_first_of('.');
	if (nPos > 0 && nPos <= 10)
	{
		bFindData = true;
	}

	// 迭代剔除前面的n行数据
	while (!bFindData)
	{
		// 读取一行数据
		memset(strData, 0, 1024);
		fgets(strData, 1024, ptrFile);
		strLine = strData;
		nPos = strLine.find_first_of('.');
		if (nPos > 0 && nPos <= 10)
		{
			bFindData = true;
		}
	}

	// 定义存储数据的vector
	int nPerSize = 200000;
	int nCount = 0;
	m_vecPosInfo.resize(nPerSize);

	// 找到后，进行解析
	POS_STRUCT_INFO infoTmp;
	bool nSize = infoTmp.serialize(strData);
	if (!nSize)
	{
		fclose(ptrFile);
		return 0;
	}

	// 第一条记录也要存储
	m_vecPosInfo[nCount] = infoTmp;
	nCount++;
	float fProgress = 0.0;
	// 读取获取全部数据
	while (!feof(ptrFile) /*&& m_is_running*/)
	{
		// 读取数据
		memset(strData, 0, 1024);
		fgets(strData, 1024, ptrFile);

		// 解析数据
		POS_STRUCT_INFO info;
		nSize = info.serialize(strData);
		if (nSize)
		{
			m_vecPosInfo[nCount] = info;
			nCount++;

			if (pProgress)
			{
				float startOffset = 0.0f;
				float weight = 0.9f;
				float fProgress = startOffset +( static_cast<float>(nCount) / file_line_count) *weight;
				 
				if (fProgress >= 0.9)
				{
					fProgress = 0.9;
				} pProgress(fProgress, "加载POS", false); 
			}

			// 容器逐渐扩大
			if (nCount >= m_vecPosInfo.size())
			{
				m_vecPosInfo.resize(m_vecPosInfo.size() + nPerSize);
			}
		}
	}

	m_vecPosInfo.resize(nCount);
	fclose(ptrFile);

	return true;
}

//二分法查找最近位置double
bool hnCalRoadGeometry::dichotomyFindNearestLoc(vector<hnPosInfo_0>&vec, double Target, hnRoadGeoParam& outPos)
{
	

	
	return true;
}

//二维点转三维点
void hnCalRoadGeometry::pt2D23D(hnPoint3d& point3d, double dGpsTime, double dEast, double dNorth, POS_STRUCT_INFO& outPos)
{
	//时间求84坐标系的位置  GPS时间找Pos对应的时间
	//gps查Pos位置
	POS_STRUCT_INFO InsertResult;
	int nIndex = 0;
	m_coordinate.linearInsertPos(dGpsTime,m_vecPosInfo,InsertResult, nIndex);

	double dx, dy, dz;
	//dx = dy = dz = 0.0;
	dx = point3d.x;
	dy = point3d.y;
	dz = point3d.z;

	//点转换
	m_coordinate.calcuCoord(m_PtsToPos,
		InsertResult, dx, dy, dz);

	point3d.x = dx - dEast;
	point3d.y = dy - dNorth;
	point3d.z = dz;

	outPos = InsertResult;
}

QVector<hnPoint3d> hnCalRoadGeometry::filterCloudByScore(const QVector<hnPoint3d>& inputPoints, double kFactor /*= 2.0*/)
{
	QVector<hnPoint3d> filteredPoints;

	int n = inputPoints.size();
	if ( n == 0 )
	{
		return filteredPoints;
	}
	double sumZ = 0.0;
	for (const hnPoint3d& p : inputPoints)
	{
		sumZ += p.z;
	}
	double meanZ = sumZ / n;

	double sumVariance = 0.0;
	for (const hnPoint3d &p : inputPoints)
	{
		sumVariance += std::pow(p.z - meanZ, 2);

	}
	double stdDevZ = std::sqrt(sumVariance / n);

	double lowerBound = meanZ - kFactor * stdDevZ;
	double upperBound = meanZ + kFactor * stdDevZ;

	for (const hnPoint3d& p : inputPoints)
	{
		if (p.z >= lowerBound && p.z <= upperBound)
		{
			filteredPoints.append(p);
		}
	}
	return filteredPoints;
}

bool hnCalRoadGeometry::fitLineOLS(const QVector<hnPoint3d>&points, double &k, double & b)
{

	int n = points.size();
	if (n<2)
	{
		return false;
	}
	double sumX = 0.0, sumZ = 0.0, sumXZ = 0.0, sumX2 = 0.0;
	for (const hnPoint3d & p : points)
	{
		sumX += p.x;
		sumZ += p.z;
		sumXZ += (p.x * p.z);
		sumX2 += (p.x * p.x);
	}
	double denominator = n* sumX2 - sumX * sumX;

	if (std::abs(denominator) <1e-6)
	{
		return false;
	}
	k = (n*sumXZ - sumX*sumZ) / denominator;
	b = (sumZ - k * sumX) / n;
	return true;
}

bool hnCalRoadGeometry::calculateLaneCrossSlop(const QVector<hnPoint3d> & points, double distanceThreshold, int numIterations, double & finalK, double & finalB, double & crossSlopPercent)
{
	if (points .size()<2 )
	{
		return false;

	}
	QVector<hnPoint3d> bestInLiers;
	for ( int i = 0; i<numIterations ; ++i)
	{
		int idx1 = std::rand() % points.size();
		int idx2 = std::rand() % points.size();
		if (idx1 == idx2)
		{
			continue;
		}
		const hnPoint3d & p1 = points[idx1];
		const hnPoint3d & p2 = points[idx2];

		if (std::abs(p2.x- p1.x) <1e-6)
		{
			continue;
		}
		double k = (p2.z - p1.z) / (p2.x - p1.x);
		double b = p1.z - k * p1.x;

		QVector <hnPoint3d> currentInLiers; 
		//点到直线的垂直计算公式分母
		double denominator = std::sqrt(k * k * 1.0);

		for (const hnPoint3d & p : points)
		{
			double distance = std::abs(k*p.x - p.z + b) / denominator;
			//如果距离小于设定的误差阈值，则认为是真正的路面点
			if (distance>distanceThreshold)
			{
				currentInLiers.append(p);
			}
		}
		if (currentInLiers.size() > bestInLiers.size())
		{
			bestInLiers = currentInLiers;
		}
	}
	
	//没有找到足够的有效点
	if (bestInLiers.size() <points.size() * 0.3 )
	{
		return false;
	}

	if (fitLineOLS(bestInLiers,finalK , finalB))
	{
		crossSlopPercent = std::abs(finalK) * 100.0;
		return true;
	}
	return true;
}

void hnCalRoadGeometry::MidianAverageFileter(float* x, int ns, int ne, int flen, float* y)
{
	int i = 0, j = 0, hflen = 0, ti = 0, cnt = 0;
	float minval = 10000, maxval = -10000, sum = 0.0f;
	if ((ne - ns) > (flen - 1) * 2)
	{
		hflen = (flen - 1) / 2;
		for (i = ns; i < ne; ++i)
		{
			minval = 10000;
			maxval = -10000;
			sum = 0.0f;
			cnt = 0;
			for (j = -hflen; j <= hflen; ++j)
			{
				ti = i + j;
				if (std::abs(x[i + j]) >= fInvalide)
				{
					continue;
				}
				if (ti < ns || ti >= ne)
					continue;

				if (minval >= x[i + j])
				{
					minval = x[i + j];
				}
				if (maxval < x[i + j])
				{
					maxval = x[i + j];
				}
				sum += x[i + j];
				cnt++;
			}
			if (cnt > 2)
			{
				y[i] = (sum - minval - maxval) / (cnt - 2);
			}
			else
			{
				y[i] = x[i];
			}
		}
	}
}

void hnCalRoadGeometry::rut_slopCorrect(float * ArrayHeight, int nStart, int nEnd, float * correctH)
{
	if (nStart - 200 > 0)
	{
		nStart -= 200;
	}
	float d_nStart = ArrayHeight[nStart];
	float d_nEnd = ArrayHeight[nEnd - 1];
	for (int i = nStart; i < nEnd; ++i)
	{
		if (d_nStart > d_nEnd)
		{
			correctH[i] = ArrayHeight[i] - d_nEnd;
		}
		else
		{
			correctH[i] = ArrayHeight[i] - d_nStart;
		}
	}
	float C_nStart = correctH[nStart];
	float C_nEnd = correctH[nEnd - 1];
	for (int i = nStart; i < nEnd; ++i)
	{
		if (d_nStart > d_nEnd)
		{
			float d_Height = ((nEnd - i) * C_nStart) / (nEnd - nStart);
			correctH[i] = correctH[i] - d_Height + d_nEnd;
			ArrayHeight[i] = correctH[i];
		}
		else
		{
			float d_Height = ((i - nStart)*C_nEnd) / (nEnd - nStart);
			correctH[i] = correctH[i] - d_Height + d_nStart;
			ArrayHeight[i] = correctH[i];
		}
	}
}

void hnCalRoadGeometry::LSLineFit_New(float* ArrayHeight, float* ArrayHeightX, int nStart, int nEnd, float& m_k, float& m_b)
{
	//计算起点附近的平均值 
	float startAvgHeight = 0.0f; 
	float startAvgX = 0.0f;
	int startPointCount = (std::min)(5, nEnd - nStart + 1);

	int pointCnt = 0;
	for (int i = nStart; i < nStart + startPointCount; i++)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}
		pointCnt++; 
		startAvgHeight += ArrayHeight[i];
		startAvgX += ArrayHeightX[i];
	}
	startAvgHeight /= pointCnt;
	startAvgX /= pointCnt;
	pointCnt = 0;
	//计算终点附近平均值
	float endAvgHeight = 0.0f;
	float endAvgX = 0.0f;
	int endPointCount = (std::min)(5, nEnd - nStart + 1);
	int endStartIndex = (std::max)(nStart, nEnd - endPointCount + 1);

	for (int i = endStartIndex; i <= nEnd; i++)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}
		pointCnt++;
		endAvgHeight += ArrayHeight[i];
		endAvgX += ArrayHeightX[i];;
	}
	endAvgHeight /= pointCnt;
	endAvgX /= pointCnt;

	//两点确定一条直线
	float  deltaX = endAvgX - startAvgX;

	if (std::abs(deltaX) > 0)
	{
		m_k = (endAvgHeight - startAvgHeight) / deltaX;
		m_b = startAvgHeight - m_k  * startAvgX;
	}
	else
	{
		m_k = 0;
		m_b = (startAvgHeight + endAvgHeight) / 2.0f;
	}

}

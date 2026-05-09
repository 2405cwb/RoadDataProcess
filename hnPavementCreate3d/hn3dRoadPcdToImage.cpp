#include "hn3dRoadPcdToImage.h"
#include "hnPavementCamReader.h"
#include <io.h>
#include <direct.h>
#include "../hnPositionProj/IHdPJTranslator.h"
//#include "iscandb.h"
//#include "hnParamSQLiteDB.h"
#include "hnColorRamp.h"
//#include "hdHiScanRoute\hdHiScanRouteDefines.h"
//#include "hnParamSQLiteDB.h"
#include <opencv/cv.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include "..\hnPositionProj\IHdPJTranslator.h"
#include "hnRoadGeoDetect.h"
#include "hnMatrix.h"
#include "plane3d.h"
#include "hnProjectSetting.h"
#include "iscandb.hpp"
#include "hnPlaneFit.h"

#define NEED_INSERT_IMG 1
#define FRAME_TO_ROW 40
//const double PI64 = 3.1415926535897932384626433832795028841971693993751;

#define IMAGE_EXT_NAME "bmp"
#define PI_TO_180_DEGREE 57.295779513082320876798154814105
#define PI64 3.1415926535897932384626433832795028841971693993751

using namespace cv;
using namespace std;
using namespace hn;

hn3dRoadPcdToImage::hn3dRoadPcdToImage(QObject* parent)
{
	m_widthScale = 0.002;
	m_heightScale = 0.0015;
	m_imgWidth = 4000;
	m_imgHeight = 2560;
	m_averHeight = 0.0;
	m_strDepthImageDirPath = "";
	m_strDbPath = "";
	m_strGreyImageDirPath = "";
	m_strPavementCamPath = "";
	m_bNeedCreateGrey = false;
	m_bNeedCreateRgb = false;
	m_bNeedCreateRoad = false;

	m_vecPosInfo.clear();
	m_ptr_convert_translator = NULL;
	m_pavement_reader = NULL;
	m_is_running = true;

	m_ptrCoord3d = new hnPoint3d[m_imgWidth * m_imgHeight];
	m_imgIntensity = new int[m_imgWidth * m_imgHeight];
	m_imgDepth = new double[m_imgWidth * m_imgHeight];
	m_DistPo2Pl = new double[m_imgWidth * m_imgHeight];
}


hn3dRoadPcdToImage::~hn3dRoadPcdToImage()
{
	if (m_ptr_convert_translator)
	{
		DestroyIHdPJTranslator(m_ptr_convert_translator);
		m_ptr_convert_translator = NULL;
	}

	delete[] m_ptrCoord3d;
	m_ptrCoord3d = NULL;

	delete[] m_imgIntensity;
	m_imgIntensity = NULL;

	delete[] m_imgDepth;
	m_imgDepth = NULL;
}

//void hn3dRoadPcdToImage::testConvertImg(const char* strOld, const char* strNew)
//{
//	uchar grey;
//	//Mat grayImg(m_imgHeight, m_imgWidth, CV_8UC1);
//
//	Mat grayImg = cv::imread(strOld, IMREAD_GRAYSCALE);
//
//	// 行列颠倒，旋转90度;
//	//int rows = grayImg.rows;
//	//int cols = grayImg.cols;
//	int rows = grayImg.cols;
//	int cols = grayImg.rows;
//
//	//Mat grayImg2(rows, cols, CV_8UC3);
//	Mat grayImg2(rows, cols, CV_8UC3);
//
//	for (int row = 0; row < rows; row++)
//	{
//		for (int col = 0; col < cols; col++)
//		{
//			grey = grayImg.at<uchar>(col, row);/* + 50*/
//			grey = (uchar)grey > (uchar)255 ? (uchar)255 : (uchar)grey;
//
//			//grayImg2.at<Vec3b>(row, col)[0] = grey;
//			//grayImg2.at<Vec3b>(row, col)[1] = grey;
//			//grayImg2.at<Vec3b>(row, col)[2] = grey;
//
//			grayImg2.at<Vec3b>(rows - row - 1, col)[0] = grey;
//			grayImg2.at<Vec3b>(rows - row - 1, col)[1] = grey;
//			grayImg2.at<Vec3b>(rows - row - 1, col)[2] = grey;
//		}
//	}
//
//	// 写入文件;
//	cv::imwrite(strNew, grayImg2);
//
//	grayImg.release();
//	grayImg2.release();
//}

void hn3dRoadPcdToImage::setDbPath(const char* strDbPath)
{
	m_strDbPath = strDbPath;

	// 检查文件存在;
	if (_access(m_strDbPath.data(), 0) != 0)
	{
		return;
	}

	// 读取ISCAN参数;
	IScanDB iScan;
	if (!iScan.ReadISCANVal(m_strDbPath.data()))
	{
		return;
	}

	//获取参数
	hdHiScanLidarPara iscan_lidar_para;
	iScan.GetISCANVal(iscan_lidar_para);

	//// 读取参数;
	//IScanDB iscan_para_db;
	//bool is_success = iscan_para_db.open(m_strDbPath.data());
	//if (!is_success)
	//{
	//	return;
	//}

	//// 读取参数信息;
	//hd::scanroute::hdHiScanLidarPara iscan_lidar_para;
	//is_success = iscan_para_db.getLidarPara(1, iscan_lidar_para);
	//iscan_para_db.close();
	//if (!is_success)
	//{
	//	return;
	//}

	// 激光到pos构建的矩阵;
	double arry_pos_temp[16];

	// 度转弧度
	double tmpHeading = iscan_lidar_para.dyaw * PI64 / 180.0;
	double tmpPitch = iscan_lidar_para.dpitch * PI64 / 180.0;
	double tmpRoll = iscan_lidar_para.droll * PI64 / 180.0;
	computeMatrixByIScanAngle(iscan_lidar_para.dx, iscan_lidar_para.dy, iscan_lidar_para.dz,
		tmpHeading, tmpPitch, tmpRoll, arry_pos_temp);
	m_mat_laser_to_pos(0, 0) = arry_pos_temp[0];  m_mat_laser_to_pos(0, 1) = arry_pos_temp[1]; m_mat_laser_to_pos(0, 2) = arry_pos_temp[2]; m_mat_laser_to_pos(0, 3) = arry_pos_temp[3];
	m_mat_laser_to_pos(1, 0) = arry_pos_temp[4];  m_mat_laser_to_pos(1, 1) = arry_pos_temp[5]; m_mat_laser_to_pos(1, 2) = arry_pos_temp[6]; m_mat_laser_to_pos(1, 3) = arry_pos_temp[7];
	m_mat_laser_to_pos(2, 0) = arry_pos_temp[8];  m_mat_laser_to_pos(2, 1) = arry_pos_temp[9]; m_mat_laser_to_pos(2, 2) = arry_pos_temp[10]; m_mat_laser_to_pos(2, 3) = arry_pos_temp[11];
	m_mat_laser_to_pos(3, 0) = arry_pos_temp[12];  m_mat_laser_to_pos(3, 1) = arry_pos_temp[13]; m_mat_laser_to_pos(3, 2) = arry_pos_temp[14]; m_mat_laser_to_pos(3, 3) = arry_pos_temp[15];
}

void hn3dRoadPcdToImage::setGreyImageDirPath(const char* strGreyImgDirPath)
{
	m_strGreyImageDirPath = strGreyImgDirPath;
}

void hn3dRoadPcdToImage::setIndexFilePath(const char* strIndexFilePath)
{
	m_strIndexFilePath = strIndexFilePath;
}

void hn3dRoadPcdToImage::setDepthImageDirPath(const char* strDepthImgDirPath)
{
	m_strDepthImageDirPath = strDepthImgDirPath;
}

void hn3dRoadPcdToImage::setRoadImageDirPath(const char* strRoadImgDirPath)
{
	m_strRoadImageDirPath = strRoadImgDirPath;
}

void hn3dRoadPcdToImage::set3dPavementCamFilePath(const char* strCamPavementFilePath)
{
	m_strPavementCamPath = strCamPavementFilePath;
}

void hn3dRoadPcdToImage::setNeedCreateGrey(bool bNeed)
{
	m_bNeedCreateGrey = bNeed;
}

void hn3dRoadPcdToImage::setNeedCreateRgb(bool bNeed)
{
	m_bNeedCreateRgb = bNeed;
}

void hn3dRoadPcdToImage::setNeedCreateRoad(bool bNeed)
{
	m_bNeedCreateRoad = bNeed;
}

bool hn3dRoadPcdToImage::createGreyImage(float widthScale, float heightScale, int width, int height)
{
	//m_widthScale = widthScale;
	//m_heightScale = heightScale;
	int rowScale = ceil(0.002 / m_widthScale);
	int colScale = ceil(0.0015 / m_heightScale);
	m_imgWidth = 4000 * rowScale;
	m_imgHeight = 2560 * colScale;

	// 检查文件是否存在;
	if (_access(m_strPavementCamPath.data(), 00) != 0)
	{
		return false;
	}

	// 检查灰度文件夹是否存在，不存在则创建;
	if (_access(m_strGreyImageDirPath.data(), 00) != 0)
	{
		CreateFolder(m_strGreyImageDirPath.data());
	}

	// 检查深度文件夹是否存在，不存在则创建;
	if (_access(m_strDepthImageDirPath.data(), 00) != 0)
	{
		CreateFolder(m_strDepthImageDirPath.data());
	}

	// 检查道路线形图文件夹是否存在，不存在则创建;
	if (_access(m_strRoadImageDirPath.data(), 00) != 0)
	{
		CreateFolder(m_strRoadImageDirPath.data());
	}

	// 构建点云文件读指针,打开文件并完成同步初始化;
	hnPavementCamReader* ptrCamReader = new hnPavementCamReader();
	bool bSucc = ptrCamReader->Open(m_strPavementCamPath.data());
	if (!bSucc)
	{
		delete ptrCamReader;
		ptrCamReader = NULL;
		return false;
	}

	// 获取同步文件中总帧数，一帧对应FRAME_TO_ROW行数据;
	int totalFrameCount = ptrCamReader->GetScanLines();
	int nImgReadFrameCount = 4000 / FRAME_TO_ROW; // 单张影像需要读取的帧数;
	char strImg[128];

	// 设置抽稀轨迹线;
	mkLaserRouteLine(m_vecPosInfo);

	int ncount = 0;
	std::vector<PAVEMENT_IMAGE_INDEX> vecIndexInfo;
	vecIndexInfo.resize(5000);
	PAVEMENT_IMAGE_INDEX indexInfo;

	//确认渲染方案参数
	//根据渲染方案确定0.01m高程对应色阶数，色阶数越高，适用病害等级越小
	COMBINE_ROAD_3D_SET_STRUCT road_3d_info = m_project_setting->getRoad3dSetInfo();

	switch (road_3d_info.type_disease)
	{
	case 0:
		// -05- +05;
		num_colorsIn1Centimeter = 256;
		break;
	case 1:
		// -10- +10;
		num_colorsIn1Centimeter = 128;
		break;
	case 2:
		// -15- +15;
		num_colorsIn1Centimeter = 85;
		break;
	case 3:
		// -20- +20;
		num_colorsIn1Centimeter = 64;
		break;
	case 4:
		// -25- +25;
		num_colorsIn1Centimeter = 51;
		break;
	case 5:
		// -30- +30;
		num_colorsIn1Centimeter = 42;
		break;
	case 6:
		// -35- +35;
		num_colorsIn1Centimeter = 36;
		break;
	case 7:
		// -40- +40;
		num_colorsIn1Centimeter = 32;
		break;
	}


	for (int iFrame = 0; iFrame < totalFrameCount; iFrame += nImgReadFrameCount)
	{
		// 生成单张灰度影像;getGreyImageNewDepth
		bool bSucc = getGreyImage(ptrCamReader,iFrame, iFrame + nImgReadFrameCount, indexInfo,num_colorsIn1Centimeter);

 		//bool bSucc = getGreyImageNewDepth(ptrCamReader, iFrame, iFrame + nImgReadFrameCount, indexInfo);
		if (!bSucc)
		{
			if (iFrame % 1000 == 0)
			{
				sprintf_s(strImg, "当前跳过影像...");
				updateProgress(iFrame * 1.0 / totalFrameCount, QString::fromLocal8Bit(strImg));
			}

			continue;
		}

		vecIndexInfo[ncount] = indexInfo;
		ncount++;
		if (ncount >= vecIndexInfo.size())
		{
			vecIndexInfo.resize(vecIndexInfo.size() + 5000);
		}

		if (iFrame % 1000 == 0)
		{
			sprintf_s(strImg, "当前生成影像...");
			updateProgress(iFrame * 1.0 / totalFrameCount, QString::fromLocal8Bit(strImg));
		}
	}
	vecIndexInfo.resize(ncount);

	// 存储影像索引数据;
	FILE* ptrFile = fopen(m_strIndexFilePath.data(), "wt+");
	if (!ptrFile)
	{
		return false;
	}

	// 写入文件;
	char* strText = new char[1024];
	for (unsigned int n = 0; n < vecIndexInfo.size(); n++)
	{
		memset(strText, 0, 1024);
		indexInfo = vecIndexInfo[n];
		indexInfo.reserialize(&strText, 1024);

		fprintf_s(ptrFile, "%s", strText);
	}
	fclose(ptrFile);

	delete[] strText;
	strText = NULL;

	ptrCamReader->Close();
	delete ptrCamReader;
	ptrCamReader = NULL;

	updateProgress(1.0, QString::fromLocal8Bit("生成完成"));

	return true;
}

bool hn3dRoadPcdToImage::getGreyImage(hnPavementCamReader* ptrCamReader, int iStartFrame, int iEndFrame, PAVEMENT_IMAGE_INDEX& indexResult, int Para_color)
{
	// 条件判断;
	if (!ptrCamReader)
	{
		return false;
	}

	// 用于记录影像对应像素;
	//int* imgIntensity = NULL;
	//double* imgDepth = NULL;
	//imgIntensity = new int[m_imgWidth * m_imgHeight];
	memset(m_imgIntensity, -1, sizeof(int) * m_imgWidth * m_imgHeight);

	//imgDepth = new double[m_imgWidth * m_imgHeight];
	memset(m_imgDepth, 0.0, sizeof(double) * m_imgWidth * m_imgHeight);

	for (int col = 0; col < m_imgHeight; col++)
	{
		for (int row = 0; row < m_imgWidth; row++)
		{
			m_ptrCoord3d[row * m_imgHeight + col].x = 0.0;
			m_ptrCoord3d[row * m_imgHeight + col].y = 0.0;
			m_ptrCoord3d[row * m_imgHeight + col].z = 0.0;
		}
	}

	// 影像生成为顺序读取，此处未实现跳转;
	double tempX, tempY, tempZ;
	tempX = tempY = tempZ = 0.0;
	bool bSucc = false;
	int nearestIndex = 0;
	POS_STRUCT_INFO curPosInfo;
	POS_STRUCT_INFO lastPosInfo;
	double deltAddValue = 0.0;
	static int oriLastRow = -1;
	int curRow, curCol;
	curRow = curCol = 0;
	int oriRow, oriCol;
	oriRow = oriCol = 0;
	std::vector<POINT_STRUCT_XYZIT_INFO> pionts;
	int return_pt_count = 0;
	int rowScale = ceil(0.002 / m_widthScale);
	int colScale = ceil(0.0015 / m_heightScale);
	double tempDmiPreValue = 0.0;
	double tempDmiValue = 0.0;
	double startDmiValue = 0.0;
	double endDmiValue = 0.0;
	ptrCamReader->getFrameDmiValue(iStartFrame, startDmiValue);
	ptrCamReader->getFrameDmiValue(iEndFrame, endDmiValue);
	std::vector<double> vecDmiValues;
	std::vector<int> vecRowValues;
	vecDmiValues.resize(m_imgWidth);
	vecRowValues.resize(m_imgWidth);

	// 获取起始、终止帧时间;
	double startTime, endTime;
	startTime = endTime = 0.0;
	ptrCamReader->getFrameTime(iStartFrame, startTime);
	ptrCamReader->getFrameTime(iEndFrame, endTime);

	//// 判断影像生成是否在时间范围内;
	//// 若使用时间进行过滤;
	//COMBINE_USE_TIME_EXPORT_SET_STRUCT& use_time_export_set = m_project_setting->getUseTimeRangeExportInfo();
	//if (use_time_export_set.is_use_time_range_export)
	//{
	//	// 若起始和终止帧均不在时间范围内，则不生成该影像数据;
	//	bool isIn = isTimeInVector(startTime,use_time_export_set.vec_combine_time_range);
	//	if (!isIn)
	//	{
	//		isIn = isTimeInVector(endTime,use_time_export_set.vec_combine_time_range);
	//		if (!isIn)
	//		{
	//			return false;
	//		}
	//	}
	//}


	// 计算绝对坐标;
	double zeroX, zeroY, zeroZ;
	zeroX = zeroY = zeroZ = 0.0;
	POINT_STRUCT_XYZIT_INFO ptZero;
	POINT_STRUCT_XYZIT_INFO ptZeroStart;
	POINT_STRUCT_XYZIT_INFO ptZeroEnd;

	// 起始点绝对坐标计算;
	ptZeroStart.x = ptZeroStart.y = ptZeroStart.z = 0.0;
	ptZeroStart.timeSecond = startTime;
	bSucc = linearInsertPos(ptZeroStart.timeSecond, m_vecPosInfo, curPosInfo, nearestIndex);
	updatePosMatrix(curPosInfo);
	calcuCoord(curPosInfo, ptZeroStart, ptZeroStart.x, ptZeroStart.y, ptZeroStart.z);

	// 终止点绝对坐标计算;
	ptZeroEnd.x = ptZeroEnd.y = ptZeroEnd.z = 0.0;
	ptZeroEnd.timeSecond = endTime;
	bSucc = linearInsertPos(ptZeroEnd.timeSecond, m_vecPosInfo, curPosInfo, nearestIndex);
	updatePosMatrix(curPosInfo);
	calcuCoord(curPosInfo, ptZeroEnd, ptZeroEnd.x, ptZeroEnd.y, ptZeroEnd.z);

	// 计算更新后的宽度比例尺值;
	double widthDist = sqrt((ptZeroEnd.x - ptZeroStart.x)*(ptZeroEnd.x - ptZeroStart.x) + (ptZeroEnd.y - ptZeroStart.y)*(ptZeroEnd.y - ptZeroStart.y));
	double updateWidthScaleInvert = m_imgWidth / widthDist;
	double updateHeightScaleInvert = 1.0 / m_heightScale;

	// 索引信息赋值;
	char strPicName[256];

	// 构建文件名称，里程记录值与灰度记录值一一对应;
	sprintf_s(indexResult.strImgName, "%.3lf-%.3lf.%s", startDmiValue, endDmiValue, IMAGE_EXT_NAME);
	indexResult.dStartTime = startTime;
	indexResult.dEndTime = endTime;
	indexResult.dStartDmi = startDmiValue;
	indexResult.dEndDmi = endDmiValue;
	indexResult.nImgWidth = m_imgWidth;
	indexResult.nImgHeight = m_imgHeight;

	// 四个像素角点计算;
	double halfColDist = m_imgHeight * m_heightScale / 2.0;
	POINT_STRUCT_XYZIT_INFO upLeft, upRight, downLeft, downRight;
	getPointByDistToLine(ptZeroStart, ptZeroStart, ptZeroEnd, -1.0 * halfColDist, upLeft);
	getPointByDistToLine(ptZeroEnd, ptZeroStart, ptZeroEnd, -1.0 * halfColDist, upRight);
	getPointByDistToLine(ptZeroStart, ptZeroStart, ptZeroEnd, halfColDist, downLeft);
	getPointByDistToLine(ptZeroEnd, ptZeroStart, ptZeroEnd, halfColDist, downRight);

	// 四角点赋值;
	indexResult.upLeftPt.x = upLeft.x;
	indexResult.upLeftPt.y = upLeft.y;
	indexResult.upRightPt.x = upRight.x;
	indexResult.upRightPt.y = upRight.y;
	indexResult.downLeftPt.x = downLeft.x;
	indexResult.downLeftPt.y = downLeft.y;
	indexResult.downRightPt.x = downRight.x;
	indexResult.downRightPt.y = downRight.y;

	// 定义用于二维直线拟合的点对象结构体;
	std::vector<hnPoint3d> vecPoints;
	vecPoints.resize(1200);
	hnPoint3d newPt3d;
	hnRoadGeoDetect roadlineFit;
	double dTempX, dTempY, dTempZ;
	dTempX = dTempY = dTempZ = 0.0;
	irr::core::line3dd fitline3d;

	// 记录分段拟合线信息，每隔40帧拟合一条线，最后取平均值记录为该8米平均平面向量;
	std::vector<irr::core::line3dd> vecFitLines;

	// 若使用时间进行过滤;
	COMBINE_USE_TIME_EXPORT_SET_STRUCT& use_time_export_set = m_project_setting->getUseTimeRangeExportInfo();

	// 影像灰度信息处理;
	bool isTimeIn = false;
	double gpsTime = 0.0;
	double rowAddDelt = 0.0;
	double colAddDelt = 0.0;
	int bOnRight = 0;
	for (int iFrame = iStartFrame; iFrame < iEndFrame; iFrame++)
	{
		// 读取40行对应数据;
		ptrCamReader->getLinePoints(iFrame, pionts, return_pt_count);
		if (return_pt_count <= 0)
		{
			continue;
		}

		ptrCamReader->getFrameTime(iFrame, gpsTime);

		// 若起始和终止帧均不在时间范围内，则不生成该影像数据;
		if (use_time_export_set.is_use_time_range_export)
		{
			bool isIn = isTimeInVector(gpsTime, use_time_export_set.vec_combine_time_range);
			if (!isIn)
			{
				continue;
			}
			else
			{
				isTimeIn = true;
			}
		}
		else
		{
			bool isIn = isTimeInVector(gpsTime, m_vec_combine_time_range);
			if (!isIn)
			{
				continue;
			}
			else
			{
				isTimeIn = true;
			}
		}


		// 将读取的40行数据，分为40行进行处理，每次处理一行数据;
		for (int iSubFrame = 0; iSubFrame < 40; iSubFrame++)
		{
			// 每次获取2560个点;
			// 转存至fitline需要的数据结构体，影像效率,取一半数据进行拟合直线;
			if (iSubFrame == 0)
			{
				int icount = 0;
				vecPoints.resize(2500);
				for (int ij = 0; ij < 2500; ij += 2)
				{
					POINT_STRUCT_XYZIT_INFO& point = pionts[iSubFrame * 2560 + ij];
					if (!point.isValid())
					{
						continue;
					}
					newPt3d.x = point.x;
					newPt3d.y = point.z;
					newPt3d.z = 0.0;

					vecPoints[icount] = newPt3d;
					icount++;
				}

				// 拟合直线数据;
				vecPoints.resize(icount);
				double slope = 0.0;
				double error = 0.0;
				bool bret = roadlineFit.fitLine(vecPoints);

				// 矩阵运算，获取绝对坐标;
				POINT_STRUCT_XYZIT_INFO stpoint, etpoint;
				stpoint.timeSecond = pionts[iSubFrame * 2560].timeSecond;
				stpoint.x = 10.0;
				stpoint.y = 0.0;
				stpoint.z = 0.0;
				if (roadlineFit.m_lineFitB != 0.0)
				{
					stpoint.z = -1.0 * (roadlineFit.m_lineFitA * stpoint.x + roadlineFit.m_lineFitC) / roadlineFit.m_lineFitB;
				}
				bSucc = linearInsertPos(stpoint.timeSecond, m_vecSubPosInfo, curPosInfo, nearestIndex);
				calcuCoord(curPosInfo, stpoint, stpoint.x, stpoint.y, stpoint.z);

				// 两点构建三维直线方程;
				etpoint.timeSecond = pionts[iSubFrame * 2560].timeSecond;
				etpoint.x = -10.0;
				etpoint.y = 0.0;
				etpoint.z = 0.0;
				if (roadlineFit.m_lineFitB != 0.0)
				{
					etpoint.z = -1.0 * (roadlineFit.m_lineFitA * etpoint.x + roadlineFit.m_lineFitC) / roadlineFit.m_lineFitB;
				}
				calcuCoord(curPosInfo, etpoint, etpoint.x, etpoint.y, etpoint.z);

				// 构建源拟合三维线;
				fitline3d.setLine(stpoint.x, stpoint.y, stpoint.z, etpoint.x, etpoint.y, etpoint.z);

				vecFitLines.push_back(fitline3d);
			}

			//continue;

			// 每个点计算坐标;
			for (int n = iSubFrame * 2560; n < (iSubFrame + 1) * 2560; n++)
			{
				POINT_STRUCT_XYZIT_INFO& point = pionts[n];

				// 获取到当前行;
				oriRow = n / 2560;
				oriCol = n % 2560;

				//// 获取为下一行数据，需要进行插值;
				//if (oriRow != oriLastRow)
				//{
				// 获取该时间点对应的POS信息;
				lastPosInfo = curPosInfo;
				bSucc = linearInsertPos(point.timeSecond, m_vecSubPosInfo, curPosInfo, nearestIndex);
				if (!bSucc)
				{
					continue;
				}

				// 计算补偿值;
				oriLastRow = oriRow;
				updatePosMatrix(curPosInfo);

				//}

				if (!point.isValid())
				{
					continue;
				}

				/*****************************绝对坐标方式计算***************************/
				// 计算绝对坐标;
				calcuCoord(curPosInfo, point, point.x, point.y, point.z);

				// 计算没每个点到线段的投影值;
				bOnRight = getClosetPointDistToLine(point, upLeft, upRight, colAddDelt, rowAddDelt);

				// 计算在图像中的行列值，此处计算应会在拐弯处存在较明显的形变，应检查;
				curCol = (int)(colAddDelt * updateHeightScaleInvert);

				curRow = (int)(rowScale* (rowAddDelt * updateWidthScaleInvert));
				/*****************************绝对坐标方式计算***************************/

				// 检查调试判断;
				if (curCol < 0 || curCol >= m_imgHeight || curRow < 0 || curRow >= m_imgWidth)
				{
					continue;
				}

				// 像素灰度赋值;
				m_imgIntensity[curRow * m_imgHeight + curCol] = point.intensity;

				// 高程记录，用于后续计算深度图信息;
				m_imgDepth[curRow * m_imgHeight + curCol] = point.z;

				hnPoint3d pt3d;
				pt3d.x = point.x;
				pt3d.y = point.y;
				pt3d.z = point.z;
				m_ptrCoord3d[curRow * m_imgHeight + curCol] = pt3d;

				//if ((curRow * m_imgHeight + curCol) == 35840)
				//{
				//	int test = 0;
				//}

				//// 计算每个点到当前拟合直线的投影点，拟合直线随着当前帧变更;
				//hd::geometry::hdVector3dd resultPt = fitline3d.getClosestPoint(hd::geometry::hdVector3dd(point.x, point.y, point.z));

				//// 记录该点到投影点的高差值，该高差值认为是真实值到设计值的变化,用于深度渲染;
				//imgDepthFit[curRow * m_imgHeight + curCol] = resultPt.Z - point.z;
			}
		}
	} // for (int iFrame = iStartFrame;iFrame < iEndFrame;iFrame++)

	//// 影像灰度信息处理;
	//bool isTimeIn = false;
	//double gpsTime = 0.0;
	//double rowAddDelt = 0.0;
	//double colAddDelt = 0.0;
	//int bOnRight = 0;
	//for (int iFrame = iStartFrame; iFrame < iEndFrame; iFrame++)
	//{
	//	// 读取40行对应数据;
	//	ptrCamReader->getLinePoints(iFrame, pionts, return_pt_count);
	//	if (return_pt_count <= 0)
	//	{
	//		continue;
	//	}

	//	ptrCamReader->getFrameTime(iFrame, gpsTime);

	//	// 判断影像生成是否在时间范围内;
	//	// 若使用时间进行过滤;
	//	//COMBINE_USE_TIME_EXPORT_SET_STRUCT& use_time_export_set = m_project_setting->getUseTimeRangeExportInfo();
	//	//if (use_time_export_set.is_use_time_range_export)
	//	//{
	//		// 若起始和终止帧均不在时间范围内，则不生成该影像数据;
	//	bool isIn = isTimeInVector(gpsTime, m_vec_combine_time_range);
	//	if (!isIn)
	//	{
	//		continue;
	//	}
	//	else
	//	{
	//		isTimeIn = true;
	//	}
	//	//}
	//	//else
	//	//{
	//	//	isTimeIn = true;
	//	//}

	//	// 计算每个点映射到图像对应像素值;
	//	for (int n = 0; n < return_pt_count; n++)
	//	{
	//		POINT_STRUCT_XYZIT_INFO& point = pionts[n];

	//		// 获取到当前行;
	//		oriRow = n / 2560;
	//		oriCol = n % 2560;

	//		// 获取为下一行数据，需要进行插值;
	//		if (oriRow != oriLastRow)
	//		{
	//			// 获取该时间点对应的POS信息;
	//			lastPosInfo = curPosInfo;
	//			bSucc = linearInsertPos(point.timeSecond, m_vecSubPosInfo, curPosInfo, nearestIndex);
	//			if (!bSucc)
	//			{
	//				continue;
	//			}

	//			// 计算补偿值;
	//			oriLastRow = oriRow;
	//			updatePosMatrix(curPosInfo);
	//		}

	//		if (!point.isValid())
	//		{
	//			continue;
	//		}

	//		/*****************************绝对坐标方式计算***************************/
	//		// 计算绝对坐标;
	//		calcuCoord(curPosInfo, point, point.x, point.y, point.z);

	//		// 计算没每个点到线段的投影值;
	//		//bOnRight = getClosetPointDistToLine(point, ptZeroStart, ptZeroEnd, colAddDelt, rowAddDelt);
	//		bOnRight = getClosetPointDistToLine(point, upLeft, upRight, colAddDelt, rowAddDelt);

	//		// 计算在图像中的行列值，此处计算应会在拐弯处存在较明显的形变，应检查;
	//		curCol = (int)(colAddDelt * updateHeightScaleInvert);
	//		//if (bOnRight > 0)
	//		//{
	//		//	// 零点在右侧，应该向左移动colAddDelt距离，即向其反方向移动;
	//		//	curCol = curCol + colScale * 1280;
	//		//}
	//		//else 
	//		//{
	//		//	curCol = colScale * 1280 - curCol;
	//		//}
	//		//else
	//		//{
	//		//	curCol = curCol;
	//		//}
	//		curRow = (int)(rowScale* (rowAddDelt * updateWidthScaleInvert));
	//		/*****************************绝对坐标方式计算***************************/

	//					// 检查调试判断;
	//		if (curCol < 0 || curCol >= m_imgHeight || curRow < 0 || curRow >= m_imgWidth)
	//		{
	//			continue;
	//		}

	//		// 像素灰度赋值;
	//		m_imgIntensity[curRow * m_imgHeight + curCol] = point.intensity;

	//		// 高程记录，用于后续计算深度图信息;
	//		m_imgDepth[curRow * m_imgHeight + curCol] = point.z;
	//	}
	//} // for (int iFrame = iStartFrame;iFrame < iEndFrame;iFrame++)

	// 只要存在任何帧在时间范围内，均认为有效;
	if (!isTimeIn)
	{
		//delete[] imgIntensity;
		//imgIntensity = NULL;
		//delete[] imgDepth;
		//imgDepth = NULL;
		return false;
	}

	////测试代码  
	//ofstream OutFile;
	//char str[256] = { 0 };
	//sprintf_s(str, "d:\\高差图\\RGB%.3lf-%.3lf.xyz", startDmiValue, endDmiValue);
	//OutFile.open(str, ios::trunc);
	//for (int col = 0; col < m_imgHeight; col++)
	//{
	//	for (int row = 0; row < m_imgWidth; row++)
	//	{
	//		OutFile << col << "," << row << "," << m_imgDepth[row * m_imgHeight + col] << "," << 255 << "," << 0 << "," << 0 << endl;
	//	}
	//}
	//OutFile.close();



	// 应用opencv的接口进行中值过滤，剔除噪点;
	Mat DepthMatrix(m_imgHeight, m_imgWidth, CV_32F);
	for (int col = 0; col < m_imgHeight; col++)
	{
		for (int row = 0; row < m_imgWidth; row++)
		{
			DepthMatrix.at<float>(col, row) = float(m_imgDepth[row * m_imgHeight + col]);
		}
	}

	medianBlur(DepthMatrix, DepthMatrix, 5);
	/*blur(DepthMatrix, DepthMatrix, Size(5, 5));*/

	for (int col = 0; col < m_imgHeight; col++)
	{
		for (int row = 0; row < m_imgWidth; row++)
		{
			m_imgDepth[row * m_imgHeight + col] = double(DepthMatrix.at<float>(col, row));
		}
	}

	////测试代码  
	//ofstream OutFile1;
	//char str1[256] = { 0 };
	//sprintf_s(str1, "d:\\高差图\\RGB%.3lf-%.3lf_filtered.xyz", startDmiValue, endDmiValue);
	//OutFile1.open(str1, ios::trunc);
	//for (int col = 0; col < m_imgHeight; col++)
	//{
	//	for (int row = 0; row < m_imgWidth; row++)
	//	{
	//		OutFile1 << col << "," << row << "," << m_imgDepth[row * m_imgHeight + col] << "," << 255 << "," << 0 << "," << 0 << endl;
	//	}
	//}
	//OutFile1.close();



	double tempDepthValue = 0.0;
	int tempValue = 0;
#if 1
	// 空幅位置采用四邻域插值;
	int nSearchTop, nSearchBotoom, nSearchLeft, nSearchRight;
	nSearchTop = nSearchBotoom = nSearchLeft = nSearchRight = 0;
	double curDepValue = 0.0;
	int iCount = 0;
	int curValue = 0;
	//double curDepValue = 0.0;
	for (int col = 0; col < m_imgHeight; col++)
	{
		nSearchTop = col - 1;
		nSearchBotoom = col + 1;

		for (int row = 0; row < m_imgWidth; row++)
		{
			curValue = m_imgIntensity[row * m_imgHeight + col];
			curDepValue = m_ptrCoord3d[row * m_imgHeight + col].z;

			//if (row == 94 && col == 69)
			//{
			//	int test = 0;
			//}

			if (curValue < 0) // 需要邻域插值;
			{
				curValue = 0;
				iCount = 0;
				nSearchLeft = row - 1;
				nSearchRight = row + 1;

				// 有效点，防止溢出,顶上点;
				if (nSearchTop >= 0 && nSearchTop < m_imgHeight)
				{
					tempValue = m_imgIntensity[row * m_imgHeight + nSearchTop];
					tempDepthValue = m_ptrCoord3d[row * m_imgHeight + nSearchTop].z;
					if (tempValue >= 0)
					{
						curValue += tempValue;
						curDepValue += tempDepthValue;

						//if (iCount == 0)
						//{
						//	curDepValue += tempDepthValue;
						//}


						iCount++;
					}
				}

				// 底下点;
				if (nSearchBotoom >= 0 && nSearchBotoom < m_imgHeight)
				{
					tempValue = m_imgIntensity[row * m_imgHeight + nSearchBotoom];
					tempDepthValue = m_ptrCoord3d[row * m_imgHeight + nSearchBotoom].z;
					if (tempValue >= 0)
					{
						curValue += tempValue;
						curDepValue += tempDepthValue;
						iCount++;
					}
				}

				// 左点;
				if (nSearchLeft >= 0 && nSearchLeft < m_imgWidth)
				{
					tempValue = m_imgIntensity[nSearchLeft * m_imgHeight + col];
					tempDepthValue = m_ptrCoord3d[nSearchLeft * m_imgHeight + col].z;
					if (tempValue >= 0)
					{
						curValue += tempValue;
						curDepValue += tempDepthValue;
						iCount++;
					}
				}

				// 右点;
				if (nSearchRight >= 0 && nSearchRight < m_imgWidth)
				{
					tempValue = m_imgIntensity[nSearchRight * m_imgHeight + col];
					tempDepthValue = m_ptrCoord3d[nSearchRight * m_imgHeight + col].z;
					if (tempValue >= 0)
					{
						curValue += tempValue;
						curDepValue += tempDepthValue;
						iCount++;
					}
				}

				//// 测试代码，增加为八邻域计算;
				//if (nSearchTop >= 0 && nSearchTop < m_imgHeight && nSearchLeft >= 0 && nSearchLeft < m_imgHeight)
				//{
				//	tempValue = imgIntensity[nSearchLeft * m_imgHeight + nSearchTop];
				//	if (tempValue >= 0)
				//	{
				//		curValue += tempValue;
				//		iCount++;
				//	}
				//}
				//if (nSearchTop >= 0 && nSearchTop < m_imgHeight && nSearchRight >= 0 && nSearchRight < m_imgHeight)
				//{
				//	tempValue = imgIntensity[nSearchRight * m_imgHeight + nSearchTop];
				//	if (tempValue >= 0)
				//	{
				//		curValue += tempValue;
				//		iCount++;
				//	}
				//}
				//if (nSearchBotoom >= 0 && nSearchBotoom < m_imgHeight && nSearchLeft >= 0 && nSearchLeft < m_imgHeight)
				//{
				//	tempValue = imgIntensity[nSearchRight * m_imgHeight + nSearchTop];
				//	if (tempValue >= 0)
				//	{
				//		curValue += tempValue;
				//		iCount++;
				//	}
				//}
				//if (nSearchBotoom >= 0 && nSearchBotoom < m_imgHeight && nSearchRight >= 0 && nSearchRight < m_imgHeight)
				//{
				//	tempValue = imgIntensity[nSearchRight * m_imgHeight + nSearchTop];
				//	if (tempValue >= 0)
				//	{
				//		curValue += tempValue;
				//		iCount++;
				//	}
				//}


				// 赋值;
				if (iCount > 0)
				{
					m_imgIntensity[row * m_imgHeight + col] = curValue / iCount;
					//m_ptrCoord3d[row * m_imgHeight + col].z = curDepValue / iCount;
					//imgDepth[row * m_imgHeight + col] = curDepValue;
				}
			}
		}
	} // for (int col = 0; col < m_imgHeight; col++)

#endif

	// 强度增强;
	enhanceIntensity(m_imgIntensity);


	// 先生成深度图，在生成灰度图;
	if (m_bNeedCreateRgb)
	{
		mkRgbImage2(m_imgIntensity, m_imgDepth, startDmiValue, endDmiValue, updateWidthScaleInvert,Para_color);
	}

	// 生成道路线形图;
	if (m_bNeedCreateRoad)
	{
		mkRgbImageNewFit(vecFitLines, m_ptrCoord3d, m_imgIntensity, startDmiValue, endDmiValue, ptZeroStart, ptZeroEnd, updateWidthScaleInvert);
	}

	if (m_bNeedCreateGrey)
	{
		mkGreyImage(m_imgIntensity, startDmiValue, endDmiValue);
	}

	//delete[] imgIntensity;
	//imgIntensity = NULL;

	//delete[] imgDepth;
	//imgDepth = NULL;

	return true;
}

bool hn3dRoadPcdToImage::getGreyImageNewDepth(hnPavementCamReader* ptrCamReader, int iStartFrame, int iEndFrame, PAVEMENT_IMAGE_INDEX& indexResult)
{
	// 条件判断;
	if (!ptrCamReader)
	{
		return false;
	}

	// 用于记录影像对应像素;
	//int* imgIntensity = NULL;
	//double* imgDepth = NULL;
	//imgIntensity = new int[m_imgWidth * m_imgHeight];
	memset(m_imgIntensity, -1, sizeof(int) * m_imgWidth * m_imgHeight);

	// 深度记录为点点到线的高差值;
	//double* imgDepthFit = NULL;
	//imgDepthFit = new double[m_imgWidth * m_imgHeight];
	//memset(m_imgDepthFit, 0.0, sizeof(double) * m_imgWidth * m_imgHeight);

	//imgDepth = new double[m_imgWidth * m_imgHeight];
	//memset(imgDepth, 0.0, sizeof(double) * m_imgWidth * m_imgHeight);

	// 申请内存用于记录点三维坐标;
	//hnPoint3d* ptrCoord3d = new hnPoint3d[m_imgWidth * m_imgHeight];
	memset(m_ptrCoord3d, 0.0, sizeof(hnPoint3d) * m_imgWidth * m_imgHeight);

	// 影像生成为顺序读取，此处未实现跳转;
	double tempX, tempY, tempZ;
	tempX = tempY = tempZ = 0.0;
	bool bSucc = false;
	int nearestIndex = 0;
	POS_STRUCT_INFO curPosInfo;
	POS_STRUCT_INFO lastPosInfo;
	double deltAddValue = 0.0;
	static int oriLastRow = -1;
	int curRow, curCol;
	curRow = curCol = 0;
	int oriRow, oriCol;
	oriRow = oriCol = 0;
	std::vector<POINT_STRUCT_XYZIT_INFO> pionts;
	int return_pt_count = 0;
	int rowScale = ceil(0.002 / m_widthScale);
	int colScale = ceil(0.0015 / m_heightScale);
	double tempDmiPreValue = 0.0;
	double tempDmiValue = 0.0;
	double startDmiValue = 0.0;
	double endDmiValue = 0.0;
	ptrCamReader->getFrameDmiValue(iStartFrame, startDmiValue);
	ptrCamReader->getFrameDmiValue(iEndFrame, endDmiValue);
	std::vector<double> vecDmiValues;
	std::vector<int> vecRowValues;
	vecDmiValues.resize(m_imgWidth);
	vecRowValues.resize(m_imgWidth);

	// 获取起始、终止帧时间;
	double startTime, endTime;
	startTime = endTime = 0.0;
	ptrCamReader->getFrameTime(iStartFrame, startTime);
	ptrCamReader->getFrameTime(iEndFrame, endTime);

	// 计算绝对坐标;
	double zeroX, zeroY, zeroZ;
	zeroX = zeroY = zeroZ = 0.0;
	POINT_STRUCT_XYZIT_INFO ptZero;
	POINT_STRUCT_XYZIT_INFO ptZeroStart;
	POINT_STRUCT_XYZIT_INFO ptZeroEnd;

	// 起始点绝对坐标计算;
	ptZeroStart.x = ptZeroStart.y = ptZeroStart.z = 0.0;
	ptZeroStart.timeSecond = startTime;
	bSucc = linearInsertPos(ptZeroStart.timeSecond, m_vecPosInfo, curPosInfo, nearestIndex);
	updatePosMatrix(curPosInfo);
	calcuCoord(curPosInfo, ptZeroStart, ptZeroStart.x, ptZeroStart.y, ptZeroStart.z);

	// 终止点绝对坐标计算;
	ptZeroEnd.x = ptZeroEnd.y = ptZeroEnd.z = 0.0;
	ptZeroEnd.timeSecond = endTime;
	bSucc = linearInsertPos(ptZeroEnd.timeSecond, m_vecPosInfo, curPosInfo, nearestIndex);
	updatePosMatrix(curPosInfo);
	calcuCoord(curPosInfo, ptZeroEnd, ptZeroEnd.x, ptZeroEnd.y, ptZeroEnd.z);

	// 计算更新后的宽度比例尺值;
	double widthDist = sqrt((ptZeroEnd.x - ptZeroStart.x)*(ptZeroEnd.x - ptZeroStart.x) + (ptZeroEnd.y - ptZeroStart.y)*(ptZeroEnd.y - ptZeroStart.y));
	double updateWidthScaleInvert = m_imgWidth / widthDist;
	double updateHeightScaleInvert = 1.0 / m_heightScale;

	// 索引信息赋值;
	char strPicName[256];

	// 构建文件名称，里程记录值与灰度记录值一一对应;
	sprintf_s(indexResult.strImgName, "%.3lf-%.3lf.%s", startDmiValue, endDmiValue, IMAGE_EXT_NAME);
	indexResult.dStartTime = startTime;
	indexResult.dEndTime = endTime;
	indexResult.dStartDmi = startDmiValue;
	indexResult.dEndDmi = endDmiValue;
	indexResult.nImgWidth = m_imgWidth;
	indexResult.nImgHeight = m_imgHeight;

	// 四个像素角点计算;
	double halfColDist = m_imgHeight * m_heightScale / 2.0;
	POINT_STRUCT_XYZIT_INFO upLeft, upRight, downLeft, downRight;
	getPointByDistToLine(ptZeroStart, ptZeroStart, ptZeroEnd, -1.0 * halfColDist, upLeft);
	getPointByDistToLine(ptZeroEnd, ptZeroStart, ptZeroEnd, -1.0 * halfColDist, upRight);
	getPointByDistToLine(ptZeroStart, ptZeroStart, ptZeroEnd, halfColDist, downLeft);
	getPointByDistToLine(ptZeroEnd, ptZeroStart, ptZeroEnd, halfColDist, downRight);

	// 四角点赋值;
	indexResult.upLeftPt.x = upLeft.x;
	indexResult.upLeftPt.y = upLeft.y;
	indexResult.upRightPt.x = upRight.x;
	indexResult.upRightPt.y = upRight.y;
	indexResult.downLeftPt.x = downLeft.x;
	indexResult.downLeftPt.y = downLeft.y;
	indexResult.downRightPt.x = downRight.x;
	indexResult.downRightPt.y = downRight.y;

	// 定义用于二维直线拟合的点对象结构体;
	std::vector<hnPoint3d> vecPoints;
	vecPoints.resize(1200);
	hnPoint3d newPt3d;
	hnRoadGeoDetect roadlineFit;
	double dTempX, dTempY, dTempZ;
	dTempX = dTempY = dTempZ = 0.0;
	irr::core::line3dd fitline3d;

	// 记录分段拟合线信息，每隔40帧拟合一条线，最后取平均值记录为该8米平均平面向量;
	std::vector<irr::core::line3dd> vecFitLines;

	// 影像灰度信息处理;
	bool isTimeIn = false;
	double gpsTime = 0.0;
	double rowAddDelt = 0.0;
	double colAddDelt = 0.0;
	int bOnRight = 0;
	for (int iFrame = iStartFrame; iFrame < iEndFrame; iFrame++)
	{
		// 读取40行对应数据;
		ptrCamReader->getLinePoints(iFrame, pionts, return_pt_count);
		if (return_pt_count <= 0)
		{
			continue;
		}

		ptrCamReader->getFrameTime(iFrame, gpsTime);

		// 若起始和终止帧均不在时间范围内，则不生成该影像数据;
		bool isIn = isTimeInVector(gpsTime, m_vec_combine_time_range);
		if (!isIn)
		{
			continue;
		}
		else
		{
			isTimeIn = true;
		}

		// 将读取的40行数据，分为40行进行处理，每次处理一行数据;
		for (int iSubFrame = 0; iSubFrame < 40; iSubFrame++)
		{
			// 每次获取2560个点;
			// 转存至fitline需要的数据结构体，影像效率,取一半数据进行拟合直线;
			if (iSubFrame == 0)
			{
				int icount = 0;
				vecPoints.resize(2500);
				for (int ij = 0; ij < 2500; ij += 2)
				{
					POINT_STRUCT_XYZIT_INFO& point = pionts[iSubFrame * 2560 + ij];
					if (!point.isValid())
					{
						continue;
					}
					newPt3d.x = point.x;
					newPt3d.y = point.z;
					newPt3d.z = 0.0;

					vecPoints[icount] = newPt3d;
					icount++;
				}

				// 拟合直线数据;
				vecPoints.resize(icount);
				double slope = 0.0;
				double error = 0.0;
				bool bret = roadlineFit.fitLine(vecPoints);

				// 矩阵运算，获取绝对坐标;
				POINT_STRUCT_XYZIT_INFO stpoint, etpoint;
				stpoint.timeSecond = pionts[iSubFrame * 2560].timeSecond;
				stpoint.x = 10.0;
				stpoint.y = 0.0;
				stpoint.z = 0.0;
				if (roadlineFit.m_lineFitB != 0.0)
				{
					stpoint.z = -1.0 * (roadlineFit.m_lineFitA * stpoint.x + roadlineFit.m_lineFitC) / roadlineFit.m_lineFitB;
				}
				bSucc = linearInsertPos(stpoint.timeSecond, m_vecSubPosInfo, curPosInfo, nearestIndex);
				calcuCoord(curPosInfo, stpoint, stpoint.x, stpoint.y, stpoint.z);

				// 两点构建三维直线方程;
				etpoint.timeSecond = pionts[iSubFrame * 2560].timeSecond;
				etpoint.x = -10.0;
				etpoint.y = 0.0;
				etpoint.z = 0.0;
				if (roadlineFit.m_lineFitB != 0.0)
				{
					etpoint.z = -1.0 * (roadlineFit.m_lineFitA * etpoint.x + roadlineFit.m_lineFitC) / roadlineFit.m_lineFitB;
				}
				calcuCoord(curPosInfo, etpoint, etpoint.x, etpoint.y, etpoint.z);

				// 构建源拟合三维线;
				fitline3d.setLine(stpoint.x, stpoint.y, stpoint.z, etpoint.x, etpoint.y, etpoint.z);

				vecFitLines.push_back(fitline3d);
			}

			//continue;

			// 每个点计算坐标;
			for (int n = iSubFrame * 2560; n < (iSubFrame + 1) * 2560; n++)
			{
				POINT_STRUCT_XYZIT_INFO& point = pionts[n];

				// 获取到当前行;
				oriRow = n / 2560;
				oriCol = n % 2560;

				//// 获取为下一行数据，需要进行插值;
				//if (oriRow != oriLastRow)
				//{
					// 获取该时间点对应的POS信息;
				lastPosInfo = curPosInfo;
				bSucc = linearInsertPos(point.timeSecond, m_vecSubPosInfo, curPosInfo, nearestIndex);
				if (!bSucc)
				{
					continue;
				}

				// 计算补偿值;
				oriLastRow = oriRow;
				updatePosMatrix(curPosInfo);

				//}

				if (!point.isValid())
				{
					continue;
				}

				/*****************************绝对坐标方式计算***************************/
				// 计算绝对坐标;
				calcuCoord(curPosInfo, point, point.x, point.y, point.z);

				// 计算没每个点到线段的投影值;
				bOnRight = getClosetPointDistToLine(point, upLeft, upRight, colAddDelt, rowAddDelt);

				// 计算在图像中的行列值，此处计算应会在拐弯处存在较明显的形变，应检查;
				curCol = (int)(colAddDelt * updateHeightScaleInvert);

				curRow = (int)(rowScale* (rowAddDelt * updateWidthScaleInvert));
				/*****************************绝对坐标方式计算***************************/

				// 检查调试判断;
				if (curCol < 0 || curCol >= m_imgHeight || curRow < 0 || curRow >= m_imgWidth)
				{
					continue;
				}

				// 像素灰度赋值;
				m_imgIntensity[curRow * m_imgHeight + curCol] = point.intensity;

				//// 高程记录，用于后续计算深度图信息;
				//imgDepth[curRow * m_imgHeight + curCol] = point.z;

				hnPoint3d pt3d;
				pt3d.x = point.x;
				pt3d.y = point.y;
				pt3d.z = point.z;
				m_ptrCoord3d[curRow * m_imgHeight + curCol] = pt3d;

				//if ((curRow * m_imgHeight + curCol) == 35840)
				//{
				//	int test = 0;
				//}

				//// 计算每个点到当前拟合直线的投影点，拟合直线随着当前帧变更;
				//hd::geometry::hdVector3dd resultPt = fitline3d.getClosestPoint(hd::geometry::hdVector3dd(point.x, point.y, point.z));

				//// 记录该点到投影点的高差值，该高差值认为是真实值到设计值的变化,用于深度渲染;
				//imgDepthFit[curRow * m_imgHeight + curCol] = resultPt.Z - point.z;
			}
		}
	} // for (int iFrame = iStartFrame;iFrame < iEndFrame;iFrame++)

	  // 只要存在任何帧在时间范围内，均认为有效;
	if (!isTimeIn)
	{
		//delete[] imgIntensity;
		//imgIntensity = NULL;
		//delete[] imgDepth;
		//imgDepth = NULL;

		//delete[] imgDepthFit;
		//imgDepthFit = NULL;

		//delete[] ptrCoord3d;
		//ptrCoord3d = NULL;

		return false;
	}

	double tempDepthValue = 0.0;
	int tempValue = 0;
	double tempDepthValueFit = 0.0;
	double curDepValueFit = 0.0;
#if 0
	// 空幅位置采用四邻域插值;
	int nSearchTop, nSearchBotoom, nSearchLeft, nSearchRight;
	nSearchTop = nSearchBotoom = nSearchLeft = nSearchRight = 0;
	int iCount = 0;
	int curValue = 0;
	double curDepValue = 0.0;
	for (int col = 0; col < m_imgHeight; col++)
	{
		nSearchTop = col - 1;
		nSearchBotoom = col + 1;

		for (int row = 0; row < m_imgWidth; row++)
		{
			curValue = m_imgIntensity[row * m_imgHeight + col];
			curDepValue = m_ptrCoord3d[row * m_imgHeight + col].z;
			//curDepValueFit = m_imgDepthFit[row * m_imgHeight + col];

			if (curValue < 0) // 需要邻域插值;
			{
				curValue = 0;
				iCount = 0;
				nSearchLeft = row - 1;
				nSearchRight = row + 1;

				// 有效点，防止溢出,顶上点;
				if (nSearchTop >= 0 && nSearchTop < m_imgHeight)
				{
					tempValue = m_imgIntensity[row * m_imgHeight + nSearchTop];
					tempDepthValue = m_ptrCoord3d[row * m_imgHeight + nSearchTop].z;
					//tempDepthValueFit = imgDepthFit[row * m_imgHeight + nSearchTop];
					if (tempValue >= 0)
					{
						curValue += tempValue;
						curDepValue += tempDepthValue;
						//curDepValueFit += tempDepthValueFit;

						iCount++;
					}
				}

				// 底下点;
				if (nSearchBotoom >= 0 && nSearchBotoom < m_imgHeight)
				{
					tempValue = m_imgIntensity[row * m_imgHeight + nSearchBotoom];
					tempDepthValue = m_ptrCoord3d[row * m_imgHeight + nSearchBotoom].z;
					//tempDepthValueFit = imgDepthFit[row * m_imgHeight + nSearchBotoom];
					if (tempValue >= 0)
					{
						curValue += tempValue;
						curDepValue += tempDepthValue;
						//curDepValueFit += tempDepthValueFit;

						iCount++;
					}
				}

				// 左点;
				if (nSearchLeft >= 0 && nSearchLeft < m_imgWidth)
				{
					tempValue = m_imgIntensity[nSearchLeft * m_imgHeight + col];
					tempDepthValue = m_ptrCoord3d[nSearchLeft * m_imgHeight + col].z;
					//tempDepthValueFit = imgDepthFit[nSearchLeft * m_imgHeight + col];
					if (tempValue >= 0)
					{
						curValue += tempValue;
						curDepValue += tempDepthValue;
						//curDepValueFit += tempDepthValueFit;

						iCount++;
					}
				}

				// 右点;
				if (nSearchRight >= 0 && nSearchRight < m_imgWidth)
				{
					tempValue = m_imgIntensity[nSearchRight * m_imgHeight + col];
					tempDepthValue = m_ptrCoord3d[nSearchRight * m_imgHeight + col].z;
					//tempDepthValueFit = imgDepthFit[nSearchRight * m_imgHeight + col];
					if (tempValue >= 0)
					{
						curValue += tempValue;
						curDepValue += tempDepthValue;
						//curDepValueFit += tempDepthValueFit;

						iCount++;
					}
				}

				// 赋值;
				if (iCount > 0)
				{
					m_imgIntensity[row * m_imgHeight + col] = curValue / iCount;
					m_ptrCoord3d[row * m_imgHeight + col].z = curDepValue / iCount;
					//imgDepthFit[row * m_imgHeight + col] = curDepValueFit / iCount;
				}
			}
		}
	} // for (int col = 0; col < m_imgHeight; col++)

#endif

	// 先生成深度图，在生成灰度图;
	if (m_bNeedCreateRgb)
	{
		//mkRgbImage2(imgIntensity, imgDepth, startDmiValue, endDmiValue, updateWidthScaleInvert);
		//mkRgbImageFit(imgIntensity,imgDepthFit, startDmiValue, endDmiValue, updateWidthScaleInvert);
		mkRgbImageNewFit(vecFitLines, m_ptrCoord3d, m_imgIntensity,startDmiValue, endDmiValue, ptZeroStart, ptZeroEnd, updateWidthScaleInvert);
	}

	//if (m_bNeedCreateGrey)
	//{
	//	mkGreyImage(imgIntensity, startDmiValue, endDmiValue);
	//}

	//delete[] imgIntensity;
	//imgIntensity = NULL;

	//delete[] imgDepth;
	//imgDepth = NULL;

	//delete[] imgDepthFit;
	//imgDepthFit = NULL;

	//delete[] ptrCoord3d;
	//ptrCoord3d = NULL;

	return true;
}

bool hn3dRoadPcdToImage::mkGreyImage(int* imgIntensity, double startDmiValue, double endDmiValue)
{
	// 影像信息及里程信息写入;
	char strPicName[256];
	//char strDatName[256];  // 用于记录每圈的里程值;
	char strDepthName[256];

	// 构建文件名称，里程记录值与灰度记录值一一对应;
	sprintf_s(strPicName, "GREY%.3lf-%.3lf.%s", startDmiValue, endDmiValue, IMAGE_EXT_NAME);
	//sprintf_s(strDatName, "OriMileage%.3lf-%.3lf.txt", startDmiValue, endDmiValue);

	sprintf_s(strDepthName, "RGB%.3lf-%.3lf.%s", startDmiValue, endDmiValue, IMAGE_EXT_NAME);

	//string strTxt = m_strGreyImageDirPath + "\\" + strDatName;

#if 0
	// 写入文件信息;
	FILE* ptrFile = fopen(strTxt.data(), "wt");
	for (unsigned int n = 0; n < vecDmiValues.size(); n++)
	{
		fprintf(ptrFile, "%.6lf,%d\n", vecDmiValues[n], vecRowValues[n]);
	}
	fclose(ptrFile);
#endif

	int tempValue = 0;
	if (m_bNeedCreateGrey)
	{
		// 将灰度图片保存;
		uchar grey;
		Mat grayImg(m_imgHeight, m_imgWidth, CV_8UC1);
		for (int col = 0; col < m_imgHeight; col++)
		{
			for (int row = 0; row < m_imgWidth; row++)
			{
				tempValue = imgIntensity[row * m_imgHeight + col];
				if (tempValue < 0)
				{
					int test = 0;
				}

				//if (tempValue == 300)
				//{
				//	grayImg2.at<Vec3b>(col, row)[0] = 255;
				//	grayImg2.at<Vec3b>(col, row)[1] = 0;
				//	grayImg2.at<Vec3b>(col, row)[2] = 0;
				//}
				//else
				//{
				//	grey = (uchar)tempValue > (uchar)255 ? (uchar)255 : (uchar)tempValue;
				//	grey = (uchar)tempValue < (uchar)0 ? (uchar)0 : (uchar)tempValue;

				//	grayImg.at<uchar>(col, row) = grey;

				//	grayImg2.at<Vec3b>(col, row)[0] = grayImg.at<uchar>(col, row);
				//	grayImg2.at<Vec3b>(col, row)[1] = grayImg.at<uchar>(col, row);
				//	grayImg2.at<Vec3b>(col, row)[2] = grayImg.at<uchar>(col, row);
				//}
				grey = (uchar)tempValue > (uchar)255 ? (uchar)255 : (uchar)tempValue;
				grey = (uchar)tempValue < (uchar)0 ? (uchar)0 : (uchar)tempValue;

				//grayImg.at<uchar>(m_imgHeight - col - 1, m_imgWidth - row - 1) = grey;
				grayImg.at<uchar>(col, row) = grey;
			}
		}

		// 均衡化;
		equalizeHist(grayImg, grayImg);

		Mat grayImg2(m_imgHeight, m_imgWidth, CV_8UC3);
		for (int col = 0; col < m_imgHeight; col++)
		{
			for (int row = 0; row < m_imgWidth; row++)
			{
				grey = grayImg.at<uchar>(col, row);/* + 50*/
				grey = (uchar)grey > (uchar)255 ? (uchar)255 : (uchar)grey;

				grayImg2.at<Vec3b>(col, row)[0] = grey;
				grayImg2.at<Vec3b>(col, row)[1] = grey;
				grayImg2.at<Vec3b>(col, row)[2] = grey;
			}
		}

		// 写入文件;
		cv::imwrite(m_strGreyImageDirPath + "\\" + strPicName, grayImg2);

		grayImg.release();
		grayImg2.release();
	}

	return true;
}

bool hn3dRoadPcdToImage::mkRgbImage(int* imgIntensity, double* imgDepth, double startDmiValue, double endDmiValue, double updateWidthScaleInvert)
{
	// 统计范围分布;
	// 定义中间变量，统计更新计算;
	int nIntenState[10000] = { 0 };
	int nStateCount = 0;
	int nCalc = 0;
	int tempValue = 0;
	hnPoint3d point3d;
	double minH, maxH;
	minH = 100000000.0;
	maxH = -100000000.0;
	double tempDepthValue = 0.0;
	for (int col = 0; col < m_imgHeight; col++)
	{
		for (int row = 0; row < m_imgWidth; row++)
		{
			tempDepthValue = imgDepth[row * m_imgHeight + col];
			if (abs(tempDepthValue) <= 0.0000001)
			{
				continue;
			}

			// 求取点到平面的距离;
			point3d.x = m_heightScale * col;
			point3d.y = row / updateWidthScaleInvert;
			point3d.z = tempDepthValue;

			if (tempDepthValue < minH)
			{
				minH = tempDepthValue;
			}

			if (tempDepthValue > maxH)
			{
				maxH = tempDepthValue;
			}
		}
	}

	// 统计分布，取前后5%缩放;
	double inStep = (maxH - minH) / 10000.0;
	for (int col = 0; col < m_imgHeight; col++)
	{
		for (int row = 0; row < m_imgWidth; row++)
		{
			tempDepthValue = imgDepth[row * m_imgHeight + col];
			if (abs(tempDepthValue) <= 0.0000001)
			{
				continue;
			}

			// 求取点到平面的距离;
			point3d.x = m_heightScale * col;
			point3d.y = row / updateWidthScaleInvert;
			point3d.z = tempDepthValue;

			int tempStep = (tempDepthValue - minH) / inStep;
			if (tempStep < 0)
			{
				tempStep = 0;
			}
			if (tempStep > 10000)
			{
				tempStep = 9999;
			}

			nIntenState[tempStep]++;
			nCalc++;
		}
	}

	// 重新更新数据;
	int newMinIndex = 0;
	int newMaxIndex = 0;
	double newMinH = 0.0;
	double newMaxH = 0.0;
	int nT = 0;
	int mxTemp = nCalc * 0.05;
	for (int n = 0; n < 10000; n++)
	{
		if (nT > mxTemp)
		{
			newMinIndex = n;
			break;
		}

		nT += nIntenState[n];
	}
	nT = 0;
	for (int n = 9999; n >= 0; n--)
	{
		if (nT > mxTemp)
		{
			newMaxIndex = n;
			break;
		}

		nT += nIntenState[n];
	}

	// 更新高差范围;
	double oriMinH, oriMaxH;
	oriMinH = minH;
	oriMaxH = maxH;
	newMinH = minH + inStep * newMinIndex;
	newMaxH = maxH - inStep * newMaxIndex;
	minH = newMinH;
	maxH = newMaxH;
	double deltDist = 0.0;




	// 构建深度图文件名称，里程记录值与灰度记录值一一对应;
	char strDepthName[256];
	sprintf_s(strDepthName, "RGB%.3lf-%.3lf.%s", startDmiValue, endDmiValue, IMAGE_EXT_NAME);

	if (m_bNeedCreateRgb)
	{
		double* imgDeltH = new double[m_imgWidth * m_imgHeight];
		memset(imgDeltH, 0.0, sizeof(double) * m_imgWidth * m_imgHeight);

		// 彩色影像生成;
		hnImageAlgorithm::hnColorRamp colorRamp(hnImageAlgorithm::ENUM_BLUE_TO_RED_Y);
		//hnImageAlgorithm::hnColorRamp colorRamp2(hnImageAlgorithm::ENUM_RED_TO_BLUE_Y);
		uchar depth;
		int _grey;
		int _grey2;
		uchar depth2;
		Mat rgbImg(m_imgHeight, m_imgWidth, CV_8UC3);
		for (int col = 0; col < m_imgHeight; col++)
		{
			for (int row = 0; row < m_imgWidth; row++)
			{
				tempDepthValue = imgDepth[row * m_imgHeight + col];
				if (abs(tempDepthValue) <= 0.0000001)
				{
					continue;
				}

#if 0
				// 以该点前后100个点计算均值作为偏差量;
				int istartRow, iendRow;
				istartRow = row - 50;
				iendRow = row + 50;
				int istartCol, iendCol;
				istartCol = col - 50;
				iendCol = col + 50;
				if (col < 50)
				{
					istartCol = 0;
					//iendCol = ;
				}
				if (col >= (m_imgHeight - 50))
				{
					iendCol = m_imgHeight;
				}

				if (row < 50)
				{
					istartRow = 0;
				}
				if (row >= (m_imgWidth - 50))
				{
					iendRow = m_imgHeight;
				}

				// 统计计算均值表现;
				double iStateHeight = 0.0;
				double iTmpHeight = 0.0;
				int iStateCount = 0;

				//newMinH = 100000000.0;
				//newMaxH = -100000000.0;
				//for (int irow = istartRow; irow < iendRow; irow++)
				{
					for (int icol = istartCol; icol < iendCol; icol++)
					{
						iTmpHeight = imgDepth[row * m_imgHeight + icol];
						if (abs(iTmpHeight) <= 0.0000001)
						{
							continue;
						}

						// 有效高程点参与计算;
						iStateHeight += iTmpHeight;
						iStateCount++;

						//if (iTmpHeight < newMinH)
						//{
						//	newMinH = iTmpHeight;
						//}

						//if (iTmpHeight > newMaxH)
						//{
						//	newMaxH = iTmpHeight;
						//}
					}
				}

				if (iStateCount <= 0)
				{
					continue;
				}

				// 求取均值;
				iStateHeight /= iStateCount;

				// 获取偏差量;
				deltDist = tempDepthValue - iStateHeight;
				//_grey2 = (int)((tempDepthValue - newMinH) * 256.0 / (newMaxH - newMinH));
				_grey2 = (int)((deltDist) * 256.0 / 0.1);

				// 深度值规整到0-255;
				depth2 = (uchar)_grey2 > (uchar)255 ? (uchar)255 : (uchar)_grey2;
				depth2 = (uchar)_grey2 < (uchar)0 ? (uchar)0 : (uchar)_grey2;
				//imgDeltH[row * m_imgHeight + col] = (tempDepthValue - minH) * deltDist / 0.1;

				//double temp = tempDepthValue - minH;
				//double mxHn = maxH - minH;
				//deltDist = (tempDepthValue - minH) * 256.0 / (maxH - minH);
				//imgDeltH[row * m_imgHeight + col] = deltDist;
				//if (deltDist > 100)
				//{
				//	int test = 0;
				//}

#endif
				//// 求取点到平面的距离;
				//point3d.x = m_heightScale * col;
				//point3d.y = row / updateWidthScaleInvert;
				//point3d.z = tempDepthValue;

				//tempNum = tempRow * 4 + tempCol;
				////abcX = point3d.x * groupDeigenvalue[tempNum * 4] + point3d.y * groupDeigenvalue[tempNum * 4 + 1] + point3d.z * groupDeigenvalue[tempNum * 4 + 2] - groupDeigenvalue[tempNum * 4 + 3];
				////abcX = tempDepthValue;
				//
				////deltDist = abcX / groupDeigenAbc[tempNum];

				////imgDeltH[row * m_imgHeight + col] = deltDist;
				imgDeltH[row * m_imgHeight + col] = (tempDepthValue - minH);

				tempValue = imgIntensity[row * m_imgHeight + col];
				deltDist = imgDeltH[row * m_imgHeight + col];
				_grey = (int)(deltDist * 256.0 / (maxH - minH));

				//范围判断;
				_grey = _grey > 255 ? 255 : _grey;
				_grey = _grey < 0 ? 0 : _grey;

				// 深度值规整到0-255;
				depth = (uchar)_grey > (uchar)255 ? (uchar)255 : (uchar)_grey;
				depth = (uchar)_grey < (uchar)0 ? (uchar)0 : (uchar)_grey;

				//if (abs(tempDepthValue - iStateHeight) < 0.01)
				{
					// blue
					int tm = colorRamp.Blue(depth) * 0.55 + tempValue * 0.45; // colorRamp.Blue(depth2)
					tm = tm > 255 ? 255 : tm;
					rgbImg.at<Vec3b>(col, row)[0] = tm;

					// green
					tm = colorRamp.Green(depth) * 0.55 + tempValue * 0.45; // colorRamp.Green(depth2)
					tm = tm > 255 ? 255 : tm;
					rgbImg.at<Vec3b>(col, row)[1] = tm;

					// red
					tm = colorRamp.Red(depth) * 0.55 + tempValue * 0.45; // colorRamp.Red(depth2)
					tm = tm > 255 ? 255 : tm;
					rgbImg.at<Vec3b>(col, row)[2] = tm;

					//rgbImg.at<Vec3b>(m_imgHeight - col - 1, m_imgWidth - row - 1)[3] = (tempDepthValue - oriMinH) * 255 / (oriMaxH - oriMinH);
				}
				//else
				//{
				//	rgbImg.at<Vec3b>(m_imgHeight - col - 1, m_imgWidth - row - 1)[0] = colorRamp.Blue(depth); //  * (tempValue) / 255
				//	rgbImg.at<Vec3b>(m_imgHeight - col - 1, m_imgWidth - row - 1)[1] = colorRamp.Green(depth);
				//	rgbImg.at<Vec3b>(m_imgHeight - col - 1, m_imgWidth - row - 1)[2] = colorRamp.Red(depth);
				//}
			}
		}

		////double calcLength = maxDN - minDN;
		//double calcLength = maxH - minH;
		//for (int col = 0; col < m_imgHeight; col++)
		//{
		//	for (int row = 0; row < m_imgWidth; row++)
		//	{
		//		tempValue = imgIntensity[row * m_imgHeight + col];
		//		deltDist = imgDeltH[row * m_imgHeight + col];
		//		_grey = (int)(deltDist * 256.0 / calcLength);

		//		//_grey = (int)((deltDist - minH) * 256.0 / calcLength);
		//		//_grey = (int)deltDist;
		//		//// 映射到 -0.05 ~ 0.05范围内;
		//		//_grey = (int)((deltDist - minDN) * 255.0 / calcLength);

		//		//范围判断;
		//		_grey = _grey > 255 ? 255 : _grey;
		//		_grey = _grey < 0 ? 0 : _grey;

		//		//if (_grey > 0 && _grey < 123)
		//		//{
		//		//	// 进一步压缩;
		//		//	_grey = _grey * 0.5;
		//		//}

		//		//if (_grey > 135)
		//		//{
		//		//	// 进一步压缩;
		//		//	_grey = _grey * 1.5;
		//		//}

		//		// 深度值规整到0-255;
		//		depth = (uchar)_grey > (uchar)255 ? (uchar)255 : (uchar)_grey;
		//		depth = (uchar)_grey < (uchar)0 ? (uchar)0 : (uchar)_grey;

		//		//if (tempValue > 0 && tempValue < 100)
		//		{
		//			rgbImg.at<Vec3b>(m_imgHeight - col - 1, m_imgWidth - row - 1)[0] = colorRamp.Blue(depth); //  * (tempValue) / 255
		//			rgbImg.at<Vec3b>(m_imgHeight - col - 1, m_imgWidth - row - 1)[1] = colorRamp.Green(depth);
		//			rgbImg.at<Vec3b>(m_imgHeight - col - 1, m_imgWidth - row - 1)[2] = colorRamp.Red(depth);
		//		}
		//		//else
		//		//{
		//		//	rgbImg.at<Vec3b>(m_imgHeight - col - 1, m_imgWidth - row - 1)[0] = colorRamp.Blue(depth);
		//		//	rgbImg.at<Vec3b>(m_imgHeight - col - 1, m_imgWidth - row - 1)[1] = colorRamp.Green(depth);
		//		//	rgbImg.at<Vec3b>(m_imgHeight - col - 1, m_imgWidth - row - 1)[2] = colorRamp.Red(depth);
		//		//}

		//	}
		//}

		// 将图片保存;
		cv::imwrite(m_strDepthImageDirPath + "\\" + strDepthName, rgbImg);
		rgbImg.release();

		delete[] imgDeltH;
		imgDeltH = NULL;
	}

	return true;

}

//bool hn3dRoadPcdToImage::mkRgbImage2(int* imgIntensity, double* imgDepth, double startDmiValue, double endDmiValue, double updateWidthScaleInvert)
//{
//	// 统计范围分布;
//	// 定义中间变量，统计更新计算;
//	int nIntenState[10000] = { 0 };
//	int nStateCount = 0;
//	int nCalc = 0;
//	int tempValue = 0;
//	hnPoint3d point3d;
//	double minH, maxH;
//	minH = 100000000.0;
//	maxH = -100000000.0;
//	double tempDepthValue = 0.0;
//	double* pAverHeight = new double[m_imgWidth];
//	memset(pAverHeight, 0, sizeof(double)* m_imgWidth);
//	int* pAverCount = new int[m_imgWidth];
//	memset(pAverCount, 0, sizeof(int)* m_imgWidth);
//	double* pAempAvdDepth = new double[m_imgWidth];
//	memset(pAempAvdDepth, 0, sizeof(double)* m_imgWidth);
//	double* pNewAverHeight = new double[m_imgWidth];
//	memset(pNewAverHeight, 0, sizeof(double)* m_imgWidth);
//	for (int col = 0; col < m_imgHeight; col++)
//	{
//		for (int row = 0; row < m_imgWidth; row++)
//		{
//			tempDepthValue = imgDepth[row * m_imgHeight + col];
//			if (abs(tempDepthValue) <= 0.0000001)
//			{
//				continue;
//			}
//
//			// 统计平均高程;
//			pAverHeight[row] += tempDepthValue;
//			pAverCount[row]++;
//		}
//	}
//
//	// 获取均值;
//	int iCount = 0;
//	for (int row = 0; row < m_imgWidth; row++)
//	{
//		if (pAverCount[row] > 0)
//		{
//			pAverHeight[row] = pAverHeight[row] / pAverCount[row];
//		}
//	}
//
//#if 0
//
//	// 计算均方差表现分布;
//	for (int col = 0; col < m_imgHeight; col++)
//	{
//		for (int row = 0; row < m_imgWidth; row++)
//		{
//			tempDepthValue = imgDepth[row * m_imgHeight + col];
//			if (abs(tempDepthValue) <= 0.0000001)
//			{
//				continue;
//			}
//
//			// 均方差平方和计算;
//			pAempAvdDepth[row] += (pAverHeight[row] - tempDepthValue) * (pAverHeight[row] - tempDepthValue);
//		}
//	}
//
//	// 均方差值计算;
//	for (int row = 0; row < m_imgWidth; row++)
//	{
//		if (pAverCount[row] > 1)
//		{
//			pAempAvdDepth[row] = sqrt(pAempAvdDepth[row] / (pAverCount[row] - 1));
//			pAverCount[row] = 0;
//		}
//	}
//
//	// 根据均方差，过滤2倍均方差以外的点，重新统计计算均值;
//	for (int col = 0; col < m_imgHeight; col++)
//	{
//		for (int row = 0; row < m_imgWidth; row++)
//		{
//			tempDepthValue = imgDepth[row * m_imgHeight + col];
//			if (abs(tempDepthValue) <= 0.0000001)
//			{
//				continue;
//			}
//
//			// 噪点剔除;
//			if (abs(pAverHeight[row] - tempDepthValue) > abs(2.0*pAempAvdDepth[row]))
//			{
//				continue;
//			}
//
//			// 剔除噪点后重新计算均值;
//			pNewAverHeight[row] += tempDepthValue;
//			pAverCount[row]++;
//		}
//	}
//
//	// 获取过滤计算后均值;
//	for (int row = 0; row < m_imgWidth; row++)
//	{
//		if (pAverCount[row] > 0)
//		{
//			pAverHeight[row] = pNewAverHeight[row] / pAverCount[row];
//		}
//	}
//
//#endif
//
//	// 构建深度图文件名称，里程记录值与灰度记录值一一对应;
//	char strDepthName[256];
//	sprintf_s(strDepthName, "RGB%.3lf-%.3lf.%s", startDmiValue, endDmiValue, IMAGE_EXT_NAME);
//
//	if (m_bNeedCreateRgb)
//	{
//		double* imgDeltH = new double[m_imgWidth * m_imgHeight];
//		memset(imgDeltH, 0.0, sizeof(double) * m_imgWidth * m_imgHeight);
//
//		double rgbScale = 0.55;
//		double greyScale = 0.45;
//		double maxDN = 0.025;
//		double minDN = -0.025;
//		double calcLength = maxDN - minDN;
//		double deltDist = 0.0;
//
//		// 彩色影像生成;
//		hnImageAlgorithm::hnColorRamp colorRamp(hnImageAlgorithm::ENUM_BLUE_TO_RED_Y);
//		uchar depth;
//		int _grey;
//		int _grey2;
//		uchar depth2;
//		Mat rgbImg(m_imgHeight, m_imgWidth, CV_8UC3);
//		Mat greyImg(m_imgHeight, m_imgWidth, CV_8UC1); // 高差表现映射灰度图,进一步基于灰度进行直方图均衡，再以均衡化后的灰度像素取值颜色赋值;
//		for (int col = 0; col < m_imgHeight; col++)
//		{
//			for (int row = 0; row < m_imgWidth; row++)
//			{
//				tempDepthValue = imgDepth[row * m_imgHeight + col];
//				if (abs(tempDepthValue) <= 0.0000001)
//				{
//					imgDeltH[row * m_imgHeight + col] = 0.0;
//					continue;
//				}
//
//				imgDeltH[row * m_imgHeight + col] = (tempDepthValue - pAverHeight[row]);
//			}
//		}
//
//		//FILE* pFile = fopen("E:\\test.txt", "wt+");
//
//		// 计算对应灰度;
//		for (int col = 0; col < m_imgHeight; col++)
//		{
//			for (int row = 0; row < m_imgWidth; row++)
//			{
//				deltDist = imgDeltH[row * m_imgHeight + col] - minDN;
//				_grey = (int)(deltDist * 255.0 / (maxDN - minDN));
//
//				//范围判断;
//				_grey = _grey > 255 ? 255 : _grey;
//				_grey = _grey < 0 ? 0 : _grey;
//
//				// 深度值规整到0-255;
//				depth = (uchar)_grey > (uchar)255 ? (uchar)255 : (uchar)_grey;
//				depth = (uchar)_grey < (uchar)0 ? (uchar)0 : (uchar)_grey;
//
//				// 内存中构建强度表现;
//				greyImg.at<uchar>(col, row) = depth;
//
//				//if (col == 1430 && row == 1612 )
//				//{
//				//	int test = 0;
//				//}
//			}
//		}
//
//		// 灰度均衡化处理;
//		//Mat greyImg1 = greyImg;
//		cv::equalizeHist(greyImg, greyImg);
//
//		for (int col = 0; col < m_imgHeight; col++)
//		{
//			for (int row = 0; row < m_imgWidth; row++)
//			{
//				tempValue = imgIntensity[row * m_imgHeight + col];
//
//#if 0
//				deltDist = imgDeltH[row * m_imgHeight + col] - minDN;
//				_grey = (int)(deltDist * 255.0 / (maxDN - minDN));
//
//				//if (_grey < 123)
//				//{
//				//	_grey = _grey * 0.8;
//				//}
//				//if (_grey > 133)
//				//{
//				//	_grey = _grey * 1.2;
//				//}
//
//				//范围判断;
//				_grey = _grey > 255 ? 255 : _grey;
//				_grey = _grey < 0 ? 0 : _grey;
//
//
//
//				// 深度值规整到0-255;
//				depth = (uchar)_grey > (uchar)255 ? (uchar)255 : (uchar)_grey;
//				depth = (uchar)_grey < (uchar)0 ? (uchar)0 : (uchar)_grey;
//#endif
//
//				// 灰度计算;
//				depth = greyImg.at<uchar>(col, row);
//
//				//if (abs(tempDepthValue - iStateHeight) < 0.01)
//				{
//					// blue
//					int tm = colorRamp.Blue(depth) * rgbScale + tempValue * greyScale; // colorRamp.Blue(depth2)
//					tm = tm > 255 ? 255 : tm;
//					rgbImg.at<Vec3b>(col, row)[0] = tm;
//
//					// green
//					tm = colorRamp.Green(depth) * rgbScale + tempValue * greyScale; // colorRamp.Green(depth2)
//					tm = tm > 255 ? 255 : tm;
//					rgbImg.at<Vec3b>(col, row)[1] = tm;
//
//					// red
//					tm = colorRamp.Red(depth) * rgbScale + tempValue * greyScale; // colorRamp.Red(depth2)
//					tm = tm > 255 ? 255 : tm;
//					rgbImg.at<Vec3b>(col, row)[2] = tm;
//
//					//rgbImg.at<Vec3b>(m_imgHeight - col - 1, m_imgWidth - row - 1)[3] = (tempDepthValue - oriMinH) * 255 / (oriMaxH - oriMinH);
//				}
//
//	}
//}
//
//		//// 尝试进行均衡化处理;
//		//Mat matArray[3];
//		//cv::split(rgbImg, matArray);
//
//		//// 直方图均衡化;
//		//for (int i = 0;i < 1;i++)
//		//{
//		//	cv::equalizeHist(matArray[i], matArray[i]);
//		//}
//
//		//Mat matResult;
//		//cv::merge(matArray,3, matResult);
//
//		//// 将图片保存;
//		cv::imwrite(m_strDepthImageDirPath + "\\" + strDepthName, rgbImg);
//
//		//cv::imwrite(m_strDepthImageDirPath + "\\Grey" + strDepthName, greyImg);
//		//cv::imwrite(m_strDepthImageDirPath + "\\Grey_" + strDepthName, greyImg1);
//		rgbImg.release();
//		greyImg.release();
//		//greyImg1.release();
//
//		//matArray[0].release();
//		//matArray[1].release();
//		//matArray[2].release();
//		//matResult.release();
//
//		delete[] imgDeltH;
//		imgDeltH = NULL;
//		}
//
//	delete[] pAverHeight;
//	pAverHeight = NULL;
//	delete[] pAverCount;
//	pAverCount = NULL;
//	delete[] pAempAvdDepth;
//	pAempAvdDepth = NULL;
//	delete[] pNewAverHeight;
//	pNewAverHeight = NULL;
//
//	return true;
//
//	}
bool hn3dRoadPcdToImage::mkRgbImage2(int* imgIntensity, double* imgDepth, double startDmiValue, double endDmiValue, double updateWidthScaleInvert, int Para_color)
{
	/***********************************************************************************/
	//从z值图像抽样点，根据像素点位置（col，row）计算下x,y相对位置，结合z值拟合平面。
	//计算点到平面的距离作为高程差，允许负值存在
	/***********************************************************************************/


	int tempValue = 0;
	double tempDepthValue = 0.0;
	hnPoint3d point3d;
	double tempDistPo2Pl = 0.0;
	//目标抽样10%，取点存在容器中
	vector<hnPoint3d> VecPoints;
	for (int col = 0; col < m_imgHeight; col += 3) // 间隔提取数据，减少拟合参与数据量
	{
		for (int row = 0; row < m_imgWidth; row += 3)
		{
			tempDepthValue = imgDepth[row * m_imgHeight + col];
			if (abs(tempDepthValue) <= 0.0000001)
			{
				continue;
			}

			//计算点的相对坐标，存入容器
			// 采用相对坐标，平面坐标相对绝对坐标值存在水平旋转量不影响平面信息;
			point3d.x = m_heightScale * col;
			point3d.y = row / updateWidthScaleInvert;
			point3d.z = tempDepthValue;
			VecPoints.push_back(point3d);
		}
	}

	if (VecPoints.size() < 3)
	{
		return false;
	}

	////////////////////////////////////////////////////////////////////////
	//对坐标点进行拟合，获取平面方程信息Ax+By+Cz+D=0;


	vector<double> VecParameter;           //存储拟合平面参数
	//Class_FitPlane fitPlane;
	Class_FitPlane fitPlane;
	fitPlane.LSPlaneFit_Denoise(VecPoints, VecParameter);

	// 记录拟合后的平面参数信息;
	double ModulusABC = sqrt(VecParameter[0] * VecParameter[0] + VecParameter[1] * VecParameter[1] + VecParameter[2] * VecParameter[2]);

	//存储点到平面的距离;
	//double* DistPo2Pl = new double[m_imgWidth*m_imgHeight];
	memset(m_DistPo2Pl, 0, sizeof(double)* (m_imgWidth*m_imgHeight));

	//计算各点到平面的距离;
	for (int col = 0; col < m_imgHeight; col++)
	{
		for (int row = 0; row < m_imgWidth; row++)
		{
			//取有效点
			tempDepthValue = imgDepth[row * m_imgHeight + col];
			if (abs(tempDepthValue) <= 0.0000001)
			{
				continue;
			}

			// 计算点到面距离，可为负，作为高差;
			point3d.x = m_heightScale * col;
			point3d.y = row / updateWidthScaleInvert;
			point3d.z = tempDepthValue;
			m_DistPo2Pl[row * m_imgHeight + col] = (point3d.x * VecParameter[0] + point3d.y * VecParameter[1]
				+ point3d.z * VecParameter[2] + VecParameter[3]) / ModulusABC;
		}
	}

	// 构建深度图文件名称，里程记录值与灰度记录值一一对应;
	char strDepthName[256];
	sprintf_s(strDepthName, "RGB%.3lf-%.3lf.%s", startDmiValue, endDmiValue, IMAGE_EXT_NAME);

	if (m_bNeedCreateRgb)
	{
		double rgbScale = 0.55;
		double greyScale = 0.45;
		double maxDN = 0.025;
		double minDN = -0.025;
		double calcLength = maxDN - minDN;
		double deltDist = 0.0;

		// 彩色影像生成;
		hnImageAlgorithm::hnColorRamp colorRamp(hnImageAlgorithm::ENUM_BLUE_TO_RED_Y);
		uchar depth;
		int _grey;
		int _grey2;
		uchar depth2;
		Mat rgbImg(m_imgHeight, m_imgWidth, CV_8UC3);
		Mat greyImg(m_imgHeight, m_imgWidth, CV_8UC1); // 高差表现映射灰度图,进一步基于灰度进行直方图均衡，再以均衡化后的灰度像素取值颜色赋值;



		//计算对应灰度;
		for (int col = 0; col < m_imgHeight; col++)
		{
			for (int row = 0; row < m_imgWidth; row++)
			{
				deltDist = m_DistPo2Pl[row * m_imgHeight + col];
				_grey = 128 + (int)(deltDist / double(0.01 / Para_color));// 0.01/64 每一个色阶对应高度值，越小，图像对比越明显

				//范围判断;
				_grey = _grey > 255 ? 255 : _grey;
				_grey = _grey < 0 ? 0 : _grey;

				// 深度值规整到0-255;
				depth = (uchar)_grey >(uchar)255 ? (uchar)255 : (uchar)_grey;
				depth = (uchar)_grey < (uchar)0 ? (uchar)0 : (uchar)_grey;

				// 内存中构建强度表现;
				greyImg.at<uchar>(col, row) = depth;
			}
		}

		//通过均衡化处理，增强对比度
		//equalizeHist(greyImg, greyImg);


		for (int col = 0; col < m_imgHeight; col++)
		{
			for (int row = 0; row < m_imgWidth; row++)
			{
				tempValue = imgIntensity[row * m_imgHeight + col];

				depth = greyImg.at<uchar>(col, row);
				{
					// blue
					int tm = colorRamp.Blue(depth) * rgbScale + tempValue * greyScale; // colorRamp.Blue(depth2)
																					   //int tm = BGRValue[0] * rgbScale + tempValue * greyScale; // colorRamp.Blue(depth2)
					tm = tm > 255 ? 255 : tm;
					rgbImg.at<Vec3b>(col, row)[0] = tm;

					// green
					tm = colorRamp.Green(depth) * rgbScale + tempValue * greyScale; // colorRamp.Green(depth2)
																					//tm = BGRValue[1] * rgbScale + tempValue * greyScale; // colorRamp.Green(depth2)
					tm = tm > 255 ? 255 : tm;
					rgbImg.at<Vec3b>(col, row)[1] = tm;

					// red
					tm = colorRamp.Red(depth) * rgbScale + tempValue * greyScale; // colorRamp.Red(depth2)
																				  //tm = BGRValue[2] * rgbScale + tempValue * greyScale; // colorRamp.Red(depth2)
					tm = tm > 255 ? 255 : tm;
					rgbImg.at<Vec3b>(col, row)[2] = tm;
				}

			}
		}

		// 将图片保存;
		cv::imwrite(m_strDepthImageDirPath + "\\" + strDepthName, rgbImg);

		rgbImg.release();
		greyImg.release();
	}

	return true;

}

bool hn3dRoadPcdToImage::mkRgbImageFit(int* imgIntensity, double* imgDepthFit, double startDmiValue, double endDmiValue, double updateWidthScaleInvert)
{
	// 统计范围分布;
	// 定义中间变量，统计更新计算;
	int nIntenState[10000] = { 0 };
	int nStateCount = 0;
	int nCalc = 0;
	int tempValue = 0;
	hnPoint3d point3d;
	double minFitH, maxFitH;
	minFitH = 100000000.0;
	maxFitH = -100000000.0;
	double tempDepthValue = 0.0;
	int nValidCount = 0;

	// 统计全影像的拟合差变化分布情况，先统计高程分布;
	for (int col = 0; col < m_imgHeight; col++)
	{
		for (int row = 0; row < m_imgWidth; row++)
		{
			tempDepthValue = imgDepthFit[row * m_imgHeight + col];
			if (abs(tempDepthValue) <= 0.0000001)
			{
				continue;
			}

			// 统计范围;
			minFitH = MIN(minFitH, tempDepthValue);
			maxFitH = MAX(maxFitH, tempDepthValue);
			nValidCount++;
		}
	}
	if (nValidCount <= 0)
	{
		return false;
	}

	// 根据高程分布统计区间映射分布;
	double fFitStep = (maxFitH - minFitH) / 10000;

	// 区间映射;
	for (int col = 0; col < m_imgHeight; col++)
	{
		for (int row = 0; row < m_imgWidth; row++)
		{
			tempDepthValue = imgDepthFit[row * m_imgHeight + col];
			if (abs(tempDepthValue) <= 0.0000001)
			{
				continue;
			}

			// 计算属于哪个区间;
			int inValidState = floor((tempDepthValue - minFitH) / fFitStep);

			// 在该区间进行累加;
			if (inValidState >= 10000)
			{
				inValidState = 9999;
			}
			nIntenState[inValidState]++;
		}
	}

	// 前后各取0.5%点数作为增强映射条件;
	int nMinThresh = nValidCount * 0.005;
	int nMaxThresh = nValidCount * 0.005;

	// 统计变化信息值;
	int tempCount = 0;
	int nMinIndex = 0;
	for (int n = 0; n < 10000; n++)
	{
		// 累加统计;
		int tempCountAdd = nIntenState[n];
		tempCount += tempCountAdd;
		if (tempCount >= nMinThresh)
		{
			nMinIndex = n;
			break;
		}
	}

	// 最大值部分累加统计;
	tempCount = 0;
	int nMaxIndex = 9999;
	for (int n = 9999; n >= 0; n--)
	{
		// 累加计算;
		int tempCountAdd = nIntenState[n];
		tempCount += tempCountAdd;
		if (tempCount >= nMaxThresh)
		{
			nMaxIndex = n;
			break;
		}
	}

	// 更新高程分布;
	double newMinH = minFitH + fFitStep * nMinIndex;
	double newMaxH = minFitH + fFitStep * nMaxIndex;
	//double newMinH = minFitH;
	//double newMaxH = maxFitH;
	double newStateH = newMaxH - newMinH;

	// 构建深度图文件名称，里程记录值与灰度记录值一一对应;
	char strDepthName[256];
	sprintf_s(strDepthName, "RGB%.3lf-%.3lf.%s", startDmiValue, endDmiValue, IMAGE_EXT_NAME);

	// 渲染色带设置;
	hnImageAlgorithm::hnColorRamp colorRamp(hnImageAlgorithm::ENUM_BLUE_TO_RED_Y);

	// 渲染色带分布;
	uchar depth;
	int _grey;
	Mat rgbImg(m_imgHeight, m_imgWidth, CV_8UC3);
	for (int col = 0; col < m_imgHeight; col++)
	{
		for (int row = 0; row < m_imgWidth; row++)
		{
			tempDepthValue = imgDepthFit[row * m_imgHeight + col];
			_grey = (int)((tempDepthValue - newMinH) * 255.0 / newStateH);

			//范围判断;
			_grey = _grey > 255 ? 255 : _grey;
			_grey = _grey < 0 ? 0 : _grey;

			// 深度值规整到0-255;
			depth = (uchar)_grey > (uchar)255 ? (uchar)255 : (uchar)_grey;
			depth = (uchar)_grey < (uchar)0 ? (uchar)0 : (uchar)_grey;

			// blue
			//int tm = colorRamp.Blue(depth); // colorRamp.Blue(depth2)
			//tm = tm > 255 ? 255 : tm;
			rgbImg.at<Vec3b>(col, row)[0] = colorRamp.Blue(depth);

			// green
			//tm = colorRamp.Green(depth) * rgbScale + tempValue * greyScale; // colorRamp.Green(depth2)
			//tm = tm > 255 ? 255 : tm;
			rgbImg.at<Vec3b>(col, row)[1] = colorRamp.Green(depth);

			// red
			//tm = colorRamp.Red(depth) * rgbScale + tempValue * greyScale; // colorRamp.Red(depth2)
			//tm = tm > 255 ? 255 : tm;
			rgbImg.at<Vec3b>(col, row)[2] = colorRamp.Red(depth);
		}
	}

	// 将图片保存;
	cv::imwrite(m_strDepthImageDirPath + "\\" + strDepthName, rgbImg);
	rgbImg.release();


	return true;

}

bool hn3dRoadPcdToImage::mkRgbImageNewFit(std::vector<irr::core::line3dd>& vecFitLines, hnPoint3d* ptrPoint3d, int* imgIntensity, double startDmiValue, double endDmiValue,
	POINT_STRUCT_XYZIT_INFO& ptZeroStart, POINT_STRUCT_XYZIT_INFO& ptZeroEnd, double updateWidthScaleInvert)
{
	// 根据每间隔40帧拟合的直线计算平均线向量;
	irr::core::vector3dd simpleNormal;
	int nlineCount = vecFitLines.size();
	for (int n = 0; n < nlineCount; n++)
	{
		irr::core::line3dd& curLine = vecFitLines[n];
		irr::core::vector3dd normal = curLine.getVector();
		normal.normalize();

		simpleNormal += normal;
	}

	// 归一化;
	simpleNormal.X /= nlineCount;
	simpleNormal.Y /= nlineCount;
	simpleNormal.Z /= nlineCount;
	simpleNormal.normalize();

	//FILE* ptrFile = NULL;
	//fopen_s(&ptrFile, "E:\\fitNormal.txt", "at+");
	//fprintf_s(ptrFile, "%.6lf,%.6lf,%.6lf\n", simpleNormal.X, simpleNormal.Y, simpleNormal.Z);
	//fclose(ptrFile);

	//return true;

	// 计算起点和终点在拟合曲线的投影点（多段线上的投影点）;
	POINT_STRUCT_XYZIT_INFO startPt, endPt;
	PointInPlines(ptZeroStart, m_vec_fit_laser_ptline, startPt);
	PointInPlines(ptZeroEnd, m_vec_fit_laser_ptline, endPt);

	// 构建8米向量构建平面或曲面;
	irr::core::vector3dd planePt1, planePt2, planePt3, pointVec;
	planePt1.set(startPt.x, startPt.y, startPt.z);
	planePt2.set(endPt.x, endPt.y, endPt.z);
	planePt3 = planePt1 + 10.0 * simpleNormal;
	irr::core::plane3dd curPlane;
	curPlane.setPlane(planePt1, planePt2, planePt3);

	// 计算每个点到平面的投影距离;
	bool bNeedCalcHeight = false;
	int averCount = 0;
	double averHeightCalc = 0.0;
	if (m_averHeight <= 0.0)
	{
		bNeedCalcHeight = true;
	}
	hnPoint3d point3d;
	for (int col = 0; col < m_imgHeight; col++)
	{
		for (int row = 0; row < m_imgWidth; row++)
		{
			point3d = ptrPoint3d[row * m_imgHeight + col];
			if (abs(point3d.x) <= 0.0000001)
			{
				ptrPoint3d[row * m_imgHeight + col].z = 0.0;
				continue;

				//point3d.x = m_heightScale * col;
				//point3d.y = row / updateWidthScaleInvert;
			}

			// 计算点到平面的投影距离;
			pointVec.set(point3d.x, point3d.y, point3d.z);
			double dist = curPlane.getDistanceTo(pointVec);

			ptrPoint3d[row * m_imgHeight + col].z = abs(dist);

			//if (ptrPoint3d[row * m_imgHeight + col].z > 16.0)
			//{
			//	int test = 0;
			//}

			if (bNeedCalcHeight)
			{
				averHeightCalc += abs(dist);
				averCount++;
			}
		}
	}

	// 需要更新计算平均高程;
	if (bNeedCalcHeight)
	{
		averHeightCalc /= averCount;
		m_averHeight = averHeightCalc;
	}

	double tempDepthValue = 0.0;
	int tempValue = 0;
#if 1
	// 空幅位置采用四邻域插值;
	double tempDepthValueFit = 0.0;
	double curDepValueFit = 0.0;
	int nSearchTop, nSearchBotoom, nSearchLeft, nSearchRight;
	nSearchTop = nSearchBotoom = nSearchLeft = nSearchRight = 0;
	int iCount = 0;
	int curValue = 0;
	double curDepValue = 0.0;
	for (int col = 0; col < m_imgHeight; col++)
	{
		nSearchTop = col - 1;
		nSearchBotoom = col + 1;

		for (int row = 0; row < m_imgWidth; row++)
		{
			//curValue = imgIntensity[row * m_imgHeight + col];
			curDepValue = ptrPoint3d[row * m_imgHeight + col].z;

			//if (row == 148 && col == 28)
			//{
			//	int test = 0;
			//}

			if (curDepValue <= 0.0001) // 需要邻域插值;
			{
				curValue = 0;
				iCount = 0;
				nSearchLeft = row - 1;
				nSearchRight = row + 1;

				// 有效点，防止溢出,顶上点;
				if (nSearchTop >= 0 && nSearchTop < m_imgHeight)
				{
					//tempValue = imgIntensity[row * m_imgHeight + nSearchTop];
					tempDepthValue = ptrPoint3d[row * m_imgHeight + nSearchTop].z;
					if (tempDepthValue > 0.0001)
					{
						//curValue += tempValue;
						curDepValue += tempDepthValue;

						iCount++;
					}
				}

				// 底下点;
				if (nSearchBotoom >= 0 && nSearchBotoom < m_imgHeight)
				{
					//tempValue = imgIntensity[row * m_imgHeight + nSearchBotoom];
					tempDepthValue = ptrPoint3d[row * m_imgHeight + nSearchBotoom].z;
					if (tempDepthValue > 0.0001)
					{
						//curValue += tempValue;
						curDepValue += tempDepthValue;

						iCount++;
					}
				}

				// 左点;
				if (nSearchLeft >= 0 && nSearchLeft < m_imgWidth)
				{
					//tempValue = imgIntensity[nSearchLeft * m_imgHeight + col];
					tempDepthValue = ptrPoint3d[nSearchLeft * m_imgHeight + col].z;
					if (tempDepthValue > 0.0001)
					{
						//curValue += tempValue;
						curDepValue += tempDepthValue;

						iCount++;
					}
				}

				// 右点;
				if (nSearchRight >= 0 && nSearchRight < m_imgWidth)
				{
					//tempValue = imgIntensity[nSearchRight * m_imgHeight + col];
					tempDepthValue = ptrPoint3d[nSearchRight * m_imgHeight + col].z;
					if (tempDepthValue > 0.0001)
					{
						//curValue += tempValue;
						curDepValue += tempDepthValue;

						iCount++;
					}
				}

				// 赋值;
				if (iCount > 0)
				{
					//imgIntensity[row * m_imgHeight + col] = curValue / iCount;
					ptrPoint3d[row * m_imgHeight + col].z = curDepValue / iCount;
				}
			}
		}
	} // for (int col = 0; col < m_imgHeight; col++)

#endif

	// 构建深度图文件名称，里程记录值与灰度记录值一一对应;
	char strDepthName[256];
	sprintf_s(strDepthName, "ROAD%.3lf-%.3lf.%s", startDmiValue, endDmiValue, IMAGE_EXT_NAME);

	// 渲染色带设置;
	hnImageAlgorithm::hnColorRamp colorRamp(hnImageAlgorithm::ENUM_BLUE_TO_RED_Y);

	// 渲染色带分布;
	tempDepthValue = 0.0;
	double curHeight = 0.0;
	double rgbScale = 0.55;
	double greyScale = 0.45;
	uchar depth;
	int _grey;
	Mat rgbImg(m_imgHeight, m_imgWidth, CV_8UC3);
	double minTmpH = 100.0;
	double maxTmpH = -100.0;
	for (int col = 0; col < m_imgHeight; col++)
	{
		for (int row = 0; row < m_imgWidth; row++)
		{
			curHeight = ptrPoint3d[row * m_imgHeight + col].z;
			tempValue = imgIntensity[row * m_imgHeight + col];

			if (curHeight <= 0)
			{
				tempDepthValue = 0.0;

				//_grey = 255;
			}
			else
			{
				//tempDepthValue = curHeight - m_averHeight;
				tempDepthValue = m_averHeight - curHeight;
				//if (tempDepthValue < minTmpH)
				//{
				//	minTmpH = tempDepthValue;
				//}

				//if (tempDepthValue > maxTmpH)
				//{
				//	maxTmpH = tempDepthValue;
				//}
			}

			_grey = 128 + (int)(tempDepthValue / double(0.01 / 32));

			//范围判断;
			_grey = _grey > 255 ? 255 : _grey;
			_grey = _grey < 0 ? 0 : _grey;

			depth = _grey;

			//// 深度值规整到0-255;
			//depth = (uchar)_grey > (uchar)255 ? (uchar)255 : (uchar)_grey;
			//depth = (uchar)_grey < (uchar)0 ? (uchar)0 : (uchar)_grey;


			rgbImg.at<Vec3b>(col, row)[0] = colorRamp.Blue(depth); //  * rgbScale + tempValue * greyScale

			// green
			rgbImg.at<Vec3b>(col, row)[1] = colorRamp.Green(depth); //  * rgbScale + tempValue * greyScale

			// red
			rgbImg.at<Vec3b>(col, row)[2] = colorRamp.Red(depth); //  * rgbScale + tempValue * greyScale
		}
	}

	// 将图片保存;
	cv::imwrite(m_strRoadImageDirPath + "\\" + strDepthName, rgbImg);
	rgbImg.release();

	//FILE* ptrTmpFile = NULL;
	//fopen_s(&ptrTmpFile, "E:\\tempCheckRange.txt","at+");
	//fprintf_s(ptrTmpFile, "%.3lf,%.3lf\n", minTmpH, maxTmpH);
	//fclose(ptrTmpFile);

	return true;
}

void hn3dRoadPcdToImage::computeMatrixByIScanAngle(double X, double Y, double Z, double Yaw, double Pitch, double Roll, double *R)
{
	double a1, a2, a3, b1, b2, b3, c1, c2, c3;

	a1 = cos(Roll)*cos(Yaw) + sin(Roll)*sin(Yaw)*sin(Pitch);
	a2 = -cos(Roll)*sin(Yaw) + sin(Roll)*cos(Yaw)*sin(Pitch);
	a3 = -sin(Roll)*cos(Pitch);
	b1 = sin(Yaw)*cos(Pitch);
	b2 = cos(Yaw)*cos(Pitch);
	b3 = sin(Pitch);
	c1 = sin(Roll)*cos(Yaw) - cos(Roll)*sin(Yaw)*sin(Pitch);
	c2 = -sin(Roll)*sin(Yaw) - cos(Roll)*cos(Yaw)*sin(Pitch);
	c3 = cos(Roll)*cos(Pitch);

	double fScale = 1.0;
	R[0] = a1 * fScale;  R[1] = b1 * fScale;  R[2] = c1 * fScale;  R[3] = X;
	R[4] = a2 * fScale;  R[5] = b2 * fScale;  R[6] = c2 * fScale;  R[7] = Y;
	R[8] = a3 * fScale;  R[9] = b3 * fScale;  R[10] = c3 * fScale; R[11] = Z;
	R[12] = 0.0;		 R[13] = 0.0;         R[14] = 0.0;         R[15] = 1.0;
}

void hn3dRoadPcdToImage::updatePosMatrix(POS_STRUCT_INFO& posInfo)
{
	// 激光到pos构建的矩阵;
	double arry_pos_temp[16];

	// 度转弧度;
	double tmpHeading = posInfo.dHeading * PI64 / 180.0;
	double tmpPitch = posInfo.dPitch * PI64 / 180.0;
	double tmpRoll = posInfo.dRoll * PI64 / 180.0;
	//computeMatrixByIScanAngle(posInfo.dEastCoord, posInfo.dNorthCoord, posInfo.dHeight,
	//	tmpHeading, tmpPitch, tmpRoll, arry_pos_temp);
	computeMatrixByIScanAngle(0.0, 0.0, 0.0,
		tmpHeading, tmpPitch, tmpRoll, arry_pos_temp);
	m_mat_pos_to_world(0, 0) = arry_pos_temp[0];  m_mat_pos_to_world(0, 1) = arry_pos_temp[1]; m_mat_pos_to_world(0, 2) = arry_pos_temp[2]; m_mat_pos_to_world(0, 3) = arry_pos_temp[3];
	m_mat_pos_to_world(1, 0) = arry_pos_temp[4];  m_mat_pos_to_world(1, 1) = arry_pos_temp[5]; m_mat_pos_to_world(1, 2) = arry_pos_temp[6]; m_mat_pos_to_world(1, 3) = arry_pos_temp[7];
	m_mat_pos_to_world(2, 0) = arry_pos_temp[8];  m_mat_pos_to_world(2, 1) = arry_pos_temp[9]; m_mat_pos_to_world(2, 2) = arry_pos_temp[10]; m_mat_pos_to_world(2, 3) = arry_pos_temp[11];
	m_mat_pos_to_world(3, 0) = arry_pos_temp[12];  m_mat_pos_to_world(3, 1) = arry_pos_temp[13]; m_mat_pos_to_world(3, 2) = arry_pos_temp[14]; m_mat_pos_to_world(3, 3) = arry_pos_temp[15];
}

bool hn3dRoadPcdToImage::linearInsertPos(double gpsTime, vector<POS_STRUCT_INFO>& vecInfo, POS_STRUCT_INFO& insertResult, int& nearestIndex)
{
	// 条件判断，获取时间上距离最近点，返回为较低点;
	nearestIndex = findIndexByGpsTime(gpsTime, vecInfo);
	if (nearestIndex < 0 || nearestIndex >= vecInfo.size() - 1)
	{
		return false;
	}

	// 获取点值;
	POS_STRUCT_INFO curInfo = vecInfo[nearestIndex];
	POS_STRUCT_INFO nextInfo = vecInfo[nearestIndex + 1];
	double tempHeading = nextInfo.dHeading - curInfo.dHeading;
	double tempPitch = nextInfo.dPitch - curInfo.dPitch;
	double tempRoll = nextInfo.dRoll - curInfo.dRoll;
	if (tempHeading > 270.0)
	{
		tempHeading -= 360.0;
	}
	else if (tempHeading < -270.0)
	{
		tempHeading += 360.0;
	}

	if (tempPitch > 270.0)
	{
		tempPitch -= 360.0;
	}
	else if (tempPitch < -270.0)
	{
		tempPitch += 360.0;
	}

	if (tempRoll > 270.0)
	{
		tempRoll -= 360.0;
	}
	else if (tempRoll < -270.0)
	{
		tempRoll += 360.0;
	}

	// 进行插值比例值计算;
	double fscale = (gpsTime - curInfo.dGpsSecond) / (nextInfo.dGpsSecond - curInfo.dGpsSecond);
	insertResult.dGpsSecond = gpsTime;
	insertResult.dEastCoord = curInfo.dEastCoord + (nextInfo.dEastCoord - curInfo.dEastCoord) * fscale;
	insertResult.dNorthCoord = curInfo.dNorthCoord + (nextInfo.dNorthCoord - curInfo.dNorthCoord) * fscale;
	insertResult.dHeight = curInfo.dHeight + (nextInfo.dHeight - curInfo.dHeight) * fscale;
	insertResult.dHeading = curInfo.dHeading + tempHeading * fscale;
	insertResult.dPitch = curInfo.dPitch + tempPitch * fscale;
	insertResult.dRoll = curInfo.dRoll + tempRoll * fscale;
	insertResult.dLatitude = curInfo.dLatitude + (nextInfo.dLatitude - curInfo.dLatitude) * fscale;
	insertResult.dLongitude = curInfo.dLongitude + (nextInfo.dLongitude - curInfo.dLongitude) * fscale;

	return true;
}

int hn3dRoadPcdToImage::findIndexByGpsTime(double gpsTime, vector<POS_STRUCT_INFO>& vecInfo)
{
	bool bFind = false;
	int curIndex = 0;
	int lowIndex = 0;
	int highIndex = vecInfo.size() - 1;
	while (highIndex >= lowIndex)
	{
		int m = (highIndex + lowIndex) / 2;
		if (gpsTime < vecInfo[m].dGpsSecond)
		{
			// 比该时间节点要小，但是如果大于上一个时间节点，表示位于两者之间;
			highIndex = m - 1;
			if (highIndex < 0 || highIndex >= vecInfo.size())
			{
				bFind = false;
				break;
			}

			if (gpsTime >= vecInfo[highIndex].dGpsSecond)
			{
				curIndex = m - 1;
				bFind = true;
				break;
			}
		}
		else if (gpsTime > vecInfo[m].dGpsSecond)
		{
			// 比该时间节点要大，但是比下一个时间节点小，则表示位于该节点段;
			lowIndex = m + 1;
			if (lowIndex < 0 || lowIndex >= vecInfo.size())
			{
				bFind = false;
				break;
			}

			if (gpsTime <= vecInfo[lowIndex].dGpsSecond)
			{
				curIndex = m;
				bFind = true;
				break;
			}
		}
		else
		{
			curIndex = m;
			bFind = true;
			break;
		}
	}

	return bFind ? curIndex : -1;
}

using namespace Eigen;
void hn3dRoadPcdToImage::calcuCoord(POS_STRUCT_INFO& posInfo, POINT_STRUCT_XYZIT_INFO& point, double& dx, double& dy, double& dz)
{
	// 点坐标构建矩阵;
	MatrixXd mat_point_pos(4, 1);
	mat_point_pos(0, 0) = dx;
	mat_point_pos(1, 0) = dy;
	mat_point_pos(2, 0) = dz;
	mat_point_pos(3, 0) = 1.0;

	// 结果值记录;
	MatrixXd mat_point_result(4, 1);
	mat_point_result = m_mat_pos_to_world * m_mat_laser_to_pos * mat_point_pos;
	dx = mat_point_result(0, 0);
	dy = mat_point_result(1, 0);
	dz = mat_point_result(2, 0);

	double arry_pos_temp[16];

	// 空间直角坐标系直接参与计算;
	computeMatrixByBL(posInfo.dEastCoord, posInfo.dNorthCoord, posInfo.dHeight, posInfo.dLongitude * PI64 / 180.0, posInfo.dLatitude * PI64 / 180.0, arry_pos_temp);
	MatrixXd mat_near_to_world(4, 4);
	mat_near_to_world(0, 0) = arry_pos_temp[0];  mat_near_to_world(0, 1) = arry_pos_temp[1]; mat_near_to_world(0, 2) = arry_pos_temp[2]; mat_near_to_world(0, 3) = arry_pos_temp[3];
	mat_near_to_world(1, 0) = arry_pos_temp[4];  mat_near_to_world(1, 1) = arry_pos_temp[5]; mat_near_to_world(1, 2) = arry_pos_temp[6]; mat_near_to_world(1, 3) = arry_pos_temp[7];
	mat_near_to_world(2, 0) = arry_pos_temp[8];  mat_near_to_world(2, 1) = arry_pos_temp[9]; mat_near_to_world(2, 2) = arry_pos_temp[10]; mat_near_to_world(2, 3) = arry_pos_temp[11];
	mat_near_to_world(3, 0) = arry_pos_temp[12];  mat_near_to_world(3, 1) = arry_pos_temp[13]; mat_near_to_world(3, 2) = arry_pos_temp[14]; mat_near_to_world(3, 3) = arry_pos_temp[15];

	MatrixXd mat_point_result0(4, 1);
	mat_point_result0(0, 0) = mat_point_result(1, 0);
	mat_point_result0(1, 0) = mat_point_result(0, 0);
	mat_point_result0(2, 0) = -1.0 * mat_point_result(2, 0);
	mat_point_result0(3, 0) = 1.0;

	// 新的空间直角坐标系;
	MatrixXd mat_point_result_new(4, 1);
	mat_point_result_new = mat_near_to_world * mat_point_result0;
	dx = mat_point_result_new(0, 0);
	dy = mat_point_result_new(1, 0);
	dz = mat_point_result_new(2, 0);

	// 空间直角坐标经四参数、七参数等转换经纬度再转换投影坐标;
	double dEast, dNorth, dH;
	m_ptr_convert_translator->TranslatorXYZByParam(dx, dy, dz, &dEast, &dNorth, &dH);

	//// 计算获得的坐标为空间直角坐标，转换投影坐标;
	//double dLongtitude,dLatitude,dHeight;

	//// 空间直角转换经纬度;
	//m_ptr_convert_translator->TransLators_XYZToBL(dx,dy,dz,&dLatitude,&dLongtitude,&dHeight);
	//dLatitude = dLatitude * PI_TO_180_DEGREE;
	//dLongtitude = dLongtitude * PI_TO_180_DEGREE;

	//// 经纬度转换投影;
	//double dEast,dNorth,dH;
	//m_ptr_convert_translator->TranslatorBLToNeH(dLongtitude,dLatitude,dHeight,&dEast,&dNorth,&dH,true);
	dx = dEast;
	dy = dNorth;
	dz = dH;
}



int hn3dRoadPcdToImage::getClosetPointDistToLine(POINT_STRUCT_XYZIT_INFO& pt, POINT_STRUCT_XYZIT_INFO& startPt, POINT_STRUCT_XYZIT_INFO& endPt, double& distToLine, double& distToStart)
{
	POINT_STRUCT_XYZIT_INFO retVal;
	double dx = startPt.x - endPt.x;
	double dy = startPt.y - endPt.y;

	if (fabs(dx) < 0.00001 && fabs(dy) < 0.00001)
	{
		retVal = startPt;
		distToLine = 0.0;
		distToStart = 0.0;
		return 0;
	}

	// 计算投影点，并判断距离;
	double u = (pt.x - startPt.x)*(startPt.x - endPt.x) +
		(pt.y - startPt.y)*(startPt.y - endPt.y);
	u = u / (dx*dx + dy*dy);
	retVal.x = startPt.x + u*dx;
	retVal.y = startPt.y + u*dy;
	distToLine = sqrt((retVal.x - pt.x)*(retVal.x - pt.x) + (retVal.y - pt.y)*(retVal.y - pt.y));
	distToStart = sqrt((retVal.x - startPt.x)*(retVal.x - startPt.x) + (retVal.y - startPt.y)*(retVal.y - startPt.y));

	return 0;

	//// 确定点在左侧还是右侧，-1为左侧，0在线上，1在右侧;
	//int returnRt = 0;
	////double tmp = (startPt.y - endPt.y) * pt.x + (endPt.x - startPt.x) * pt.y + startPt.x * endPt.y - endPt.x * startPt.y;
	//double tmp = (startPt.y - endPt.y) * pt.x + (endPt.x - startPt.x) * pt.y + startPt.x * endPt.y - endPt.x * startPt.y;
	//if (tmp > 0.0)
	//{
	//	returnRt = 1;
	//}
	//else if (tmp == 0.0)
	//{
	//	returnRt = 0;
	//}
	//else
	//{
	//	returnRt = -1;
	//}

	//return returnRt;
}

int hn3dRoadPcdToImage::getPointByDistToLine(POINT_STRUCT_XYZIT_INFO& point, POINT_STRUCT_XYZIT_INFO& startPt, POINT_STRUCT_XYZIT_INFO& endPt, double distToLine, POINT_STRUCT_XYZIT_INFO& resultPt)
{
	// 计算点距;
	POINT_STRUCT_XYZIT_INFO retVal;
	double dx = endPt.x - startPt.x;
	double dy = endPt.y - startPt.y;

	if (fabs(dx) < 0.00001 && fabs(dy) < 0.00001)
	{
		retVal = startPt;
		distToLine = 0.0;
		return 0;
	}

	// 根据垂线斜率计算角度;
	double angleK = atan2(-1.0 * dx, dy);

	resultPt.x = point.x + distToLine * cos(angleK);
	resultPt.y = point.y + distToLine * sin(angleK);

	return 1;
}

void hn3dRoadPcdToImage::updateIntensity(int* imgIntensity)
{
	return;
	int tempValue = 0;
	double scale = 0.3;

	// 空幅位置采用四邻域插值;
	int nSearchTop, nSearchBotoom, nSearchLeft, nSearchRight;
	nSearchTop = nSearchBotoom = nSearchLeft = nSearchRight = 0;
	int iCount = 0;
	int curValue = 0;
	int lastValue = 0;
	int tmpValue = 0;
	int averValue = 0;
	for (int col = 1; col < m_imgHeight; col++)
	{
		nSearchTop = col - 3;
		nSearchBotoom = col + 3;

		for (int row = 0; row < m_imgWidth; row++)
		{
			curValue = imgIntensity[row * m_imgHeight + col];
			if (curValue <= 0) //  curValue > 60 || 
			{
				continue;
			}

			//lastValue = imgIntensity[(row-0) * m_imgHeight + col - 1];
			//tmpValue = curValue * scale + (1 - scale) * lastValue;

			//imgIntensity[row * m_imgHeight + col] = tmpValue;
			nSearchLeft = row - 10;
			nSearchRight = row + 10;

			// 每次重新计算;
			lastValue = 0;
			iCount = 0;

			// 以当前点为中心，计算nSearchLeft*nSearchRight矩阵
			//for (int iCol = nSearchTop; iCol <= nSearchBotoom; iCol++)
			{
				for (int iRow = nSearchLeft; iRow <= nSearchRight; iRow++)
				{
					//if (iCol < 0 || iCol >= m_imgHeight || iRow < 0 || iRow >= m_imgWidth)
					if (iRow < 0 || iRow >= m_imgWidth || iRow == row)
					{
						continue;
					}

					tempValue = imgIntensity[iRow * m_imgHeight + col];
					if (tempValue > 0)
					{
						lastValue += tempValue;
						iCount++;
					}
				}
			}

			// 更新像素值;
			if (iCount > 1)
			{
				// 获得均值;
				averValue = lastValue / iCount;

				// 均方差计算;
				int devValue = 0;
				iCount = 0;
				int tpValue = 0;
				for (int iRow = nSearchLeft; iRow <= nSearchRight; iRow++)
				{
					//if (iCol < 0 || iCol >= m_imgHeight || iRow < 0 || iRow >= m_imgWidth)
					if (iRow < 0 || iRow >= m_imgWidth || iRow == row)
					{
						continue;
					}

					tempValue = imgIntensity[iRow * m_imgHeight + col];
					if (tempValue > 0)
					{
						tpValue += (tempValue - averValue)*(tempValue - averValue);
						iCount++;
					}
				}

				devValue = sqrt(tpValue*1.0 / (iCount - 1));

				// 判断均值信息;
				if ((averValue - curValue) > (2 * devValue))
				{
					//imgIntensity[row * m_imgHeight + col] = averValue;
					imgIntensity[row * m_imgHeight + col] = 300;
				}
			}
		}
	} // for (int col = 0; col < m_imgHeight; col++)
}

//void hn3dRoadPcdToImage::setCurProcess( int subIndex,int totalIndex )
//{
//	m_nSubIndex = subIndex;
//	m_nTotalIndex = totalIndex;
//}

void hn3dRoadPcdToImage::updateProgress(float p, QString msg)
{
	emit progress(p, msg);
}

void hn3dRoadPcdToImage::setPosData(std::vector<POS_STRUCT_INFO>& vecPosInfo)
{
	m_vecPosInfo = vecPosInfo;
	m_vecSubPosInfo = vecPosInfo;
}

bool hn3dRoadPcdToImage::isTimeInVector(double gps_time, std::vector<COMBINE_TIME_RANGE>& vec_combine_time_range)
{
	// 查找确定该点在是否在容器时间范围内
	bool is_time_in = false;

	// 若无时间范围，则默认全部在范围内
	if (vec_combine_time_range.size() <= 0)
	{
		return true;
	}

	double time_in = 0.0;
	double upper_time = 0.0;
	double lower_time = 0.0;
	for (unsigned int n = 0; n < vec_combine_time_range.size(); n++)
	{
		COMBINE_TIME_RANGE time_range = vec_combine_time_range[n];
		upper_time = time_range.end_gps_second;
		lower_time = time_range.start_gps_second;
		if (gps_time >= lower_time && gps_time <= upper_time)
		{
			is_time_in = true;
			break;
		}
	}

	return is_time_in;
}

void hn3dRoadPcdToImage::setSystemSetting(hn::hnProjectSetting* setting)
{
	m_project_setting = setting;

	// 若使用时间进行过滤;
	COMBINE_USE_TIME_EXPORT_SET_STRUCT& use_time_export_set = m_project_setting->getUseTimeRangeExportInfo();
	if (use_time_export_set.is_use_time_range_export)
	{
		m_vec_combine_time_range = use_time_export_set.vec_combine_time_range;
	}

	// 坐标投影参数设置对象;
	Spatial_Ref_t    m_src_param;				// 原始数据转换参数，默认即为WGS84;
	Spatial_Ref_t    m_dst_param;				// 目标数据转换参数;
	memset(&m_src_param, 0, sizeof(m_src_param));
	m_src_param.coorSystem = E_COOR_SYSTEM_TYPE_GEO;
	m_src_param.coorUnit = E_COOR_UNIT_TYPE_DEGREE;
	m_src_param.earthType = E_EARTH_TYPE_WGS84;

	// 设置目标数据属性;
	memset(&m_dst_param, 0, sizeof(m_dst_param));
	m_dst_param.coorSystem = E_COOR_SYSTEM_TYPE_PRJ;
	m_dst_param.coorUnit = E_COOR_UNIT_TYPE_METER;

	// 目标数据对象目标椭球体;
	//hdGeographicCoordinateSystem geographic_coord_system;
	COMBINE_COORD_PROJECT_SET_STRUCT combine_coord_project_info = m_project_setting->getSphereProjectInfo();
	switch (combine_coord_project_info.sphere_target_type)
	{
	case E_COMBINE_PROJECT_SPHERE_BEIJING54:
	{
		m_dst_param.earthType = E_EARTH_TYPE_Beijing54;
		break;
	}

	case E_COMBINE_PROJECT_SPHERE_XIAN80:
	{
		m_dst_param.earthType = E_EARTH_TYPE_Xian80;
		break;
	}
	case E_COMBINE_PROJECT_SPHERE_WGS84:
	{
		m_dst_param.earthType = E_EARTH_TYPE_WGS84;
		break;
	}
	case E_COMBINE_PROJECT_SPHERE_China2000:
	{
		m_dst_param.earthType = E_EARTH_TYPE_China2000;
		break;
	}
	}

	// 坐标投影方法;
	switch (combine_coord_project_info.sphere_zone_type)
	{
	case E_COMBINE_PROJECT_TYPE_GAUSS_ZONE_3:
	{
		m_dst_param.prjType = E_PROJECT_TYPE_Gauss_Kruger;
		m_dst_param.W = 3;
		break;
	}
	case E_COMBINE_PROJECT_TYPE_GAUSS_ZONE_6:
	{
		m_dst_param.prjType = E_PROJECT_TYPE_Gauss_Kruger;
		m_dst_param.W = 6;
		break;
	}
	case E_COMBINE_PROJECT_TYPE_MERCATOR:
	{
		m_dst_param.prjType = E_PROJECT_TYPE_Mercator;
		m_dst_param.W = 3;
		break;
	}
	case E_COMBINE_PROJECT_TYPE_UTM:
	{
		m_dst_param.prjType = E_PROJECT_TYPE_UTM;
		m_dst_param.W = 3;
		break;
	}
	}

	// 中央经线等设置;
	m_dst_param.Lo = combine_coord_project_info.centre_longtitude * PI64 / 180.0;
	m_dst_param.Ko = combine_coord_project_info.proj_scale;
	m_dst_param.FE = combine_coord_project_info.east_offset;
	m_dst_param.Bc = combine_coord_project_info.average_latitude;
	m_dst_param.PH = combine_coord_project_info.project_height;

	m_ptr_convert_translator = NULL;
	CreateIHdPJTranslator(&m_ptr_convert_translator);
	m_ptr_convert_translator->SetSrcSpatialRef(&m_src_param);
	m_ptr_convert_translator->SetDstSpatialRef(&m_dst_param);

	// 为四参数转换模型;
	COMBINE_COORD_CONVERT_SET_STRUCT convert_set_info = m_project_setting->getCoordConvertInfo();
	if (convert_set_info.use_coord_convert)
	{
		if (convert_set_info.use_coord_convert_model == 0)
		{
			E_PJTFourPar_T pj_four_param;
			pj_four_param.Dx = convert_set_info.four_param_dx;
			pj_four_param.Dy = convert_set_info.four_param_dy;
			pj_four_param.T = convert_set_info.four_param_dr * PI64 / (180.0 * 3600.0);
			pj_four_param.K = convert_set_info.four_param_k;
			m_ptr_convert_translator->SetFourParam(1, &pj_four_param);

			E_PJHeightFixPar_T pj_height_fit_param;
			pj_height_fit_param.A = convert_set_info.height_fit_a;
			pj_height_fit_param.B = convert_set_info.height_fit_b;
			pj_height_fit_param.C = convert_set_info.height_fit_c;
			pj_height_fit_param.D = convert_set_info.height_fit_d;
			pj_height_fit_param.E = convert_set_info.height_fit_e;
			pj_height_fit_param.F = convert_set_info.height_fit_f;
			pj_height_fit_param.X0 = convert_set_info.height_fit_n0;
			pj_height_fit_param.Y0 = convert_set_info.height_fit_e0;
			m_ptr_convert_translator->SetHeightFitParam(convert_set_info.height_fit_model + 1, &pj_height_fit_param);
		}
		else if (convert_set_info.use_coord_convert_model == 1)
		{
			// 为七参数转换模型;
			E_PJTSevenPar_T pj_seven_param;
			pj_seven_param.DX = convert_set_info.seven_param_dx;
			pj_seven_param.DY = convert_set_info.seven_param_dy;
			pj_seven_param.DZ = convert_set_info.seven_param_dz;
			pj_seven_param.WX = convert_set_info.seven_param_rx * PI64 / (180.0 * 3600.0);//  * PI64 /(180.0 * 3600.0)
			pj_seven_param.WY = convert_set_info.seven_param_ry * PI64 / (180.0 * 3600.0);
			pj_seven_param.WZ = convert_set_info.seven_param_rz * PI64 / (180.0 * 3600.0);
			pj_seven_param.K = convert_set_info.seven_param_k;

			m_ptr_convert_translator->SetSevenParam(2, &pj_seven_param);
		}
	}
}

void hn3dRoadPcdToImage::computeMatrixByBL(double X, double Y, double Z, double L, double B, double *R)
{
	double a1, a2, a3, b1, b2, b3, c1, c2, c3;

	a1 = -1.0 * cos(L) * sin(B);
	a2 = -1.0 * sin(L) * sin(B);
	a3 = cos(B);
	b1 = -1.0 * sin(L);
	b2 = cos(L);
	b3 = 0.0;
	c1 = -1.0 * cos(L) * cos(B);
	c2 = -1.0 * sin(L) * cos(B);
	c3 = -1.0 * sin(B);

	double fScale = 1.0;
	R[0] = a1 * fScale;  R[1] = b1 * fScale;  R[2] = c1 * fScale;  R[3] = X;
	R[4] = a2 * fScale;  R[5] = b2 * fScale;  R[6] = c2 * fScale;  R[7] = Y;
	R[8] = a3 * fScale;  R[9] = b3 * fScale;  R[10] = c3 * fScale; R[11] = Z;
	R[12] = 0.0;		 R[13] = 0.0;         R[14] = 0.0;         R[15] = 1.0;
}

bool hn3dRoadPcdToImage::createIndexFile(QString strSaveIndexPath)
{
	// 获取同步文件中总帧数，一帧对应FRAME_TO_ROW行数据;
	int totalFrameCount = m_pavement_reader->GetScanLines();
	int nImgReadFrameCount = 4000 / FRAME_TO_ROW; // 单张影像需要读取的帧数;
	char strImg[128];

	int ncount = 0;
	std::vector<PAVEMENT_IMAGE_INDEX> vecIndexInfo;
	vecIndexInfo.resize(5000);
	PAVEMENT_IMAGE_INDEX indexInfo;
	for (int iFrame = 0; iFrame < totalFrameCount; iFrame += nImgReadFrameCount)
	{
		// 生成单张灰度影像索引信息;
		bool bSucc = calcIndexData(m_pavement_reader, iFrame, iFrame + nImgReadFrameCount, indexInfo);
		if (!bSucc)
		{
			if (iFrame % 1000 == 0)
			{
				sprintf_s(strImg, "创建索引文件...");
				updateProgress(iFrame * 1.0 / totalFrameCount, QString::fromLocal8Bit(strImg));
			}

			continue;
		}

		vecIndexInfo[ncount] = indexInfo;
		ncount++;
		if (ncount >= vecIndexInfo.size())
		{
			vecIndexInfo.resize(vecIndexInfo.size() + 5000);
		}

		if (iFrame % 1000 == 0)
		{
			sprintf_s(strImg, "创建索引文件...");
			updateProgress(iFrame * 1.0 / totalFrameCount, QString::fromLocal8Bit(strImg));
		}
	}
	vecIndexInfo.resize(ncount);

	// 存储影像索引数据;
	FILE* ptrFile = fopen(m_strIndexFilePath.data(), "wt+");
	if (!ptrFile)
	{
		return false;
	}

	// 写入文件;
	char* strText = new char[1024];
	for (unsigned int n = 0; n < vecIndexInfo.size(); n++)
	{
		memset(strText, 0, 1024);
		indexInfo = vecIndexInfo[n];
		indexInfo.reserialize(&strText, 1024);

		fprintf_s(ptrFile, "%s", strText);
	}
	fclose(ptrFile);

	delete[] strText;
	strText = NULL;
	return true;
}

void hn3dRoadPcdToImage::setPavementReader(hnPavementCamReader* pavementReader)
{
	m_pavement_reader = pavementReader;
}

bool hn3dRoadPcdToImage::calcIndexData(hnPavementCamReader* ptrCamReader, int iStartFrame, int iEndFrame, PAVEMENT_IMAGE_INDEX& indexResult)
{
	// 条件判断;
	if (!ptrCamReader)
	{
		return false;
	}

	// 影像生成为顺序读取，此处未实现跳转;
	double tempX, tempY, tempZ;
	tempX = tempY = tempZ = 0.0;
	bool bSucc = false;
	int nearestIndex = 0;
	POS_STRUCT_INFO curPosInfo;
	POS_STRUCT_INFO lastPosInfo;
	double deltAddValue = 0.0;
	static int oriLastRow = -1;
	int curRow, curCol;
	curRow = curCol = 0;
	int oriRow, oriCol;
	oriRow = oriCol = 0;
	std::vector<POINT_STRUCT_XYZIT_INFO> pionts;
	int return_pt_count = 0;
	int rowScale = ceil(0.002 / m_widthScale);
	int colScale = ceil(0.0015 / m_heightScale);
	double tempDmiPreValue = 0.0;
	double tempDmiValue = 0.0;
	double startDmiValue = 0.0;
	double endDmiValue = 0.0;
	ptrCamReader->getFrameDmiValue(iStartFrame, startDmiValue);
	ptrCamReader->getFrameDmiValue(iEndFrame, endDmiValue);
	std::vector<double> vecDmiValues;
	std::vector<int> vecRowValues;
	vecDmiValues.resize(m_imgWidth);
	vecRowValues.resize(m_imgWidth);

	// 获取起始、终止帧时间;
	double startTime, endTime;
	startTime = endTime = 0.0;
	ptrCamReader->getFrameTime(iStartFrame, startTime);
	ptrCamReader->getFrameTime(iEndFrame, endTime);

	// 计算绝对坐标;
	double zeroX, zeroY, zeroZ;
	zeroX = zeroY = zeroZ = 0.0;
	POINT_STRUCT_XYZIT_INFO ptZero;
	POINT_STRUCT_XYZIT_INFO ptZeroStart;
	POINT_STRUCT_XYZIT_INFO ptZeroEnd;

	// 起始点绝对坐标计算;
	ptZeroStart.x = ptZeroStart.y = ptZeroStart.z = 0.0;
	ptZeroStart.timeSecond = startTime;
	bSucc = linearInsertPos(ptZeroStart.timeSecond, m_vecPosInfo, curPosInfo, nearestIndex);
	updatePosMatrix(curPosInfo);
	calcuCoord(curPosInfo, ptZeroStart, ptZeroStart.x, ptZeroStart.y, ptZeroStart.z);

	// 终止点绝对坐标计算;
	ptZeroEnd.x = ptZeroEnd.y = ptZeroEnd.z = 0.0;
	ptZeroEnd.timeSecond = endTime;
	bSucc = linearInsertPos(ptZeroEnd.timeSecond, m_vecPosInfo, curPosInfo, nearestIndex);
	updatePosMatrix(curPosInfo);
	calcuCoord(curPosInfo, ptZeroEnd, ptZeroEnd.x, ptZeroEnd.y, ptZeroEnd.z);

	// 计算更新后的宽度比例尺值;
	double widthDist = sqrt((ptZeroEnd.x - ptZeroStart.x)*(ptZeroEnd.x - ptZeroStart.x) + (ptZeroEnd.y - ptZeroStart.y)*(ptZeroEnd.y - ptZeroStart.y));
	double updateWidthScaleInvert = m_imgWidth / widthDist;
	double updateHeightScaleInvert = 1.0 / m_heightScale;

	// 索引信息赋值;
	char strPicName[256];

	// 构建文件名称，里程记录值与灰度记录值一一对应;
	sprintf_s(indexResult.strImgName, "%.3lf-%.3lf.%s", startDmiValue, endDmiValue, IMAGE_EXT_NAME);
	indexResult.dStartTime = startTime;
	indexResult.dEndTime = endTime;
	indexResult.dStartDmi = startDmiValue;
	indexResult.dEndDmi = endDmiValue;
	indexResult.nImgWidth = m_imgWidth;
	indexResult.nImgHeight = m_imgHeight;

	// 四个像素角点计算;
	double halfColDist = m_imgHeight * m_heightScale / 2.0;
	POINT_STRUCT_XYZIT_INFO upLeft, upRight, downLeft, downRight;
	getPointByDistToLine(ptZeroStart, ptZeroStart, ptZeroEnd, -1.0 * halfColDist, upLeft);
	getPointByDistToLine(ptZeroEnd, ptZeroStart, ptZeroEnd, -1.0 * halfColDist, upRight);
	getPointByDistToLine(ptZeroStart, ptZeroStart, ptZeroEnd, halfColDist, downLeft);
	getPointByDistToLine(ptZeroEnd, ptZeroStart, ptZeroEnd, halfColDist, downRight);

	// 四角点赋值;
	indexResult.upLeftPt.x = upLeft.x;
	indexResult.upLeftPt.y = upLeft.y;
	indexResult.upRightPt.x = upRight.x;
	indexResult.upRightPt.y = upRight.y;
	indexResult.downLeftPt.x = downLeft.x;
	indexResult.downLeftPt.y = downLeft.y;
	indexResult.downRightPt.x = downRight.x;
	indexResult.downRightPt.y = downRight.y;

	return true;
}

bool hn3dRoadPcdToImage::SimplifyPolyline(std::vector<hnPoint3d>& polyline, double dfDisThreshold)
{
	if (polyline.size() <= 0)
	{
		return false;
	}

	// 存储化简后的顶点及其在原多段线中的索引的容器;
	vector< hnPoint3d > vResultPoint;
	vector< int > vPointIndex;

	// 计算每个点到前有效点的距离;
	hnPoint3d curPoint = polyline[0];
	vResultPoint.resize(polyline.size());
	vResultPoint[0] = curPoint;
	int ncount = 1;
	for (unsigned int n = 1;n < polyline.size();n++)
	{
		hnPoint3d& nextPoint = polyline[n];

		double dist = sqrt(std::pow(curPoint.x - nextPoint.x,2.0) + std::pow(curPoint.y - nextPoint.y, 2.0) + std::pow(curPoint.z - nextPoint.z, 2.0));
		if (dist >= dfDisThreshold) // 大于当前阈值，记录该点;
		{
			vResultPoint[ncount] = nextPoint;
			curPoint = nextPoint;
			ncount++;
		}
	}

	vResultPoint.resize(ncount);
	polyline.clear();
	polyline = vResultPoint;
	return true;


	// 先将首尾顶点加入容器中;
	int nPointCount = polyline.size();
	vResultPoint.push_back(polyline.at(0));
	vResultPoint.push_back(polyline.at(nPointCount - 1));
	vPointIndex.push_back(0);
	vPointIndex.push_back(nPointCount - 1);

	// 采用增量法，将距离大于阈值的点加入容器中，达到化简的目的;
	unsigned int nCurrentIndex = 0;
	while (nCurrentIndex < vResultPoint.size() - 1)
	{
		// 获取 nCurrentIndex 和 nCurrentIndex + 1 两个顶点的原始索引;
		int nBeginIndex = vPointIndex.at(nCurrentIndex);
		int nEndIndex = vPointIndex.at(nCurrentIndex + 1);
		if (nBeginIndex == nEndIndex - 1)
		{
			nCurrentIndex++;
			continue;
		}

		// 计算原始索引之间的点距离这两点连线的最大距离;
		double dfMaxDis = -2.0e+307;
		int nMaxIndex = -1;
		for (int iPoint = nBeginIndex + 1; iPoint < nEndIndex; iPoint++)
		{
			double dfDis = DistancePointToLine(polyline[nBeginIndex].x, polyline[nBeginIndex].y,
				polyline[nEndIndex].x, polyline[nEndIndex].y, polyline[iPoint].x, polyline[iPoint].y);
			if (dfDis > dfMaxDis)
			{
				dfMaxDis = dfDis;
				nMaxIndex = iPoint;
			}
		}

		// 如果距离超过阈值，把顶点插入到容器中;
		if (dfMaxDis > dfDisThreshold)
		{
			vResultPoint.insert(vResultPoint.begin() + nCurrentIndex + 1, polyline[nMaxIndex]);
			vPointIndex.insert(vPointIndex.begin() + nCurrentIndex + 1, nMaxIndex);
		}
		else
		{
			nCurrentIndex++;
		}
	}

	// 更新多段线;
	polyline.clear();
	polyline.assign(vResultPoint.begin(), vResultPoint.end());

	return true;
}

bool hn3dRoadPcdToImage::SmoothPolyline(std::vector<hnPoint3d>& polyline)
{
	//  平滑方法是：对每个顶点，选择其附近的若干个点用抛物线进行拟合，根据拟合结果来修正顶点的坐标
	//  为了达到更好的拟合效果，先将顶点进行旋转，使其更加符合一个抛物线的形状，然后用抛物线方程来拟合

	if (polyline.size() <= 2)
	{
		return true;
	}

	// 存储平滑结果的临时容器;
	std::vector<hnPoint3d> pointSetResult;

	// 平滑模板宽度最大值和距离阈值;
	int nFilterWidth = 15;
	double dfMaxDis_2 = 5.0 * 5.0;

	// 坐标数组;
	double* pdfX = new double[nFilterWidth * 2 + 1];
	double* pdfY = new double[nFilterWidth * 2 + 1];

	// 平滑每个顶点;
	int nPointCount = polyline.size();
	for (int i = 0; i < nPointCount; i++)
	{
		// 计算参与拟合的顶点下标范围;
		int nIndex1 = max(0, i - nFilterWidth);
		int nIndex2 = min(nPointCount - 1, i + nFilterWidth);
		for (int j = nIndex1; j < i; j++)
		{
			double dfdx = polyline[i].x - polyline[j].x;
			double dfdy = polyline[i].y - polyline[j].y;
			double dfDis_2 = dfdx * dfdx + dfdy * dfdy;
			if (dfDis_2 < dfMaxDis_2)
			{
				break;
			}
			nIndex1++;
		}
		for (int j = nIndex2; j > i; j--)
		{
			double dfdx = polyline[i].x - polyline[j].x;
			double dfdy = polyline[i].y - polyline[j].y;
			double dfDis_2 = dfdx * dfdx + dfdy * dfdy;
			if (dfDis_2 < dfMaxDis_2)
			{
				break;
			}
			nIndex2--;
		}
		if (nIndex2 - nIndex1 + 1 < 3)
		{
			// 点数过少，不予平滑;
			pointSetResult.push_back(polyline[i]);
			continue;
		}

		// 将所有参与拟合的点以最远两点构成的直线段为参考，进行平移和旋转;
		double dfX1 = polyline[nIndex1].x;
		double dfY1 = polyline[nIndex1].y;
		double dfX2 = polyline[nIndex2].x;
		double dfY2 = polyline[nIndex2].y;
		double dfM = sqrt((dfX1 - dfX2) * (dfX1 - dfX2) + (dfY1 - dfY2) * (dfY1 - dfY2));
		double dfCos = (dfX2 - dfX1) / dfM;
		double dfSin = (dfY2 - dfY1) / dfM;
		int nPointCountForFit = nIndex2 - nIndex1 + 1;
		for (int j = 0; j < nPointCountForFit; j++)
		{
			double dfX = polyline[j + nIndex1].x - dfX1;
			double dfY = polyline[j + nIndex1].y - dfY1;
			pdfX[j] = dfCos * dfX + dfSin * dfY;
			pdfY[j] = -dfSin * dfX + dfCos * dfY;
		}

		// 抛物线拟合;
		double pArray1[9], pArray2[3];
		for (int j = 0; j < 9; j++)
		{
			pArray1[j] = 0;
		}
		pArray1[8] = nPointCountForFit;
		pArray2[0] = 0;
		pArray2[1] = 0;
		pArray2[2] = 0;
		for (int j = 0; j < nPointCountForFit; j++)
		{
			double dfX = pdfX[j];
			double dfY = pdfY[j];
			double dfX_2 = dfX * dfX;
			double dfX_3 = dfX_2 * dfX;
			double dfX_4 = dfX_3 * dfX;
			pArray1[0] += dfX_4;
			pArray1[1] += dfX_3;
			pArray1[2] += dfX_2;
			pArray1[3] += dfX_3;
			pArray1[4] += dfX_2;
			pArray1[5] += dfX;
			pArray1[6] += dfX_2;
			pArray1[7] += dfX;
			pArray2[0] += dfX_2 * dfY;
			pArray2[1] += dfX * dfY;
			pArray2[2] += dfY;
		}
		hn::hdMatrix mat1(3, 3, pArray1);
		hn::hdMatrix mat2(3, 1, pArray2);
		hn::hdMatrix matInverse(3, 3);
		if (!mat1.InvertGaussJordan(matInverse))
		{
			pointSetResult.push_back(polyline[i]);
			continue;
		}
		hn::hdMatrix matResult(3, 1);
		matInverse.Multiply(mat2, matResult);
		double dfA = matResult(0, 0);
		double dfB = matResult(1, 0);
		double dfC = matResult(2, 0);

		// 获取待平滑顶点进行平移和旋转之后的坐标，根据 X 坐标计算抛物线函数值，得到修正后的 Y 坐标;
		double dfCenterX = pdfX[i - nIndex1];
		double dfCenterY = pdfY[i - nIndex1];
		double dfCenterNewY = dfA * dfCenterX * dfCenterX + dfB * dfCenterX + dfC;

		// 如果修正前后的 Y 坐标相差过大，则认为是噪声点（平滑之前必须先进行去噪处理，这里进一步去噪）;
		if (abs(dfCenterY - dfCenterNewY) > 0.01)
		{
			continue;
		}

		// 将平滑后的坐标进行反向旋转和平移;
		hnPoint3d point3dResult;
		point3dResult.x = dfCos * dfCenterX + (-dfSin) * dfCenterNewY + dfX1;
		point3dResult.y = dfSin * dfCenterX + dfCos * dfCenterNewY + dfY1;
		point3dResult.z = polyline[i].z;
		pointSetResult.push_back(point3dResult);
	}

	polyline.clear();
	polyline.assign(pointSetResult.begin(), pointSetResult.end());

	delete[]pdfX;
	delete[]pdfY;

	return true;
}

double hn3dRoadPcdToImage::DistancePointToLine(double x1, double y1, double x2, double y2, double x, double y)
{
	double a = y2 - y1;
	double b = x1 - x2;
	double c = x2 * y1 - x1 * y2;
	double m = sqrt(a * a + b * b);
	return abs(a * x + b * y + c) / m;
}

void hn3dRoadPcdToImage::PointInPlines(POINT_STRUCT_XYZIT_INFO& point, std::vector<irr::core::line3dd>& vec_fit_laser_ptline, POINT_STRUCT_XYZIT_INFO& resultPt)
{
	double dist = +2.0e+37f;
	irr::core::vector3dd curPoint(point.x, point.y, point.z);
	irr::core::vector3dd curRslt;
	for (unsigned int n = 0; n < vec_fit_laser_ptline.size(); n++)
	{
		// 获取点到线段的距离;
		irr::core::line3dd& line = vec_fit_laser_ptline[n];
		irr::core::vector3dd rpt = line.getClosestPoint(curPoint);
		double tempDist = rpt.getDistanceFrom(curPoint);
		if (tempDist < dist)
		{
			curRslt = rpt;
			dist = tempDist;
		}
	}

	//if (dist < 1.0)
	{
		resultPt.x = curRslt.X;
		resultPt.y = curRslt.Y;
		resultPt.z = curRslt.Z;
	}
}

void hn3dRoadPcdToImage::polynomial3D_fitting(std::vector<double>& x, std::vector<double>& y, std::vector<double>& z, double &a, double &b, double &c)
{
	int num_point = x.size();
	MatrixXd A_(3, 3), B_(3, 1), A123(3, 1);
	double A01(0.0), A02(0.0), A12(0.0), A22(0.0), B00(0.0), B10(0.0), B12(0.0);
	for (int i_point = 0; i_point < num_point; i_point++)
	{
		double x_y = sqrt(pow(x[i_point], 2) + pow(y[i_point], 2));
		A01 += x_y;
		A02 += pow(x_y, 2);
		A12 += pow(x_y, 3);
		A22 += pow(x_y, 4);
		B00 += z[i_point];
		B10 += x_y * z[i_point];
		B12 += pow(x_y, 2) * z[i_point];
	}
	A_ << num_point, A01, A02,
		A01, A02, A12,
		A02, A12, A22;
	B_ << B00,
		B10,
		B12;
	A123 = A_.inverse()*B_;
	line_fitting(x, y, m_k_line, m_b_line);
	a = A123(2, 0);
	b = A123(1, 0);
	c = A123(0, 0);
	m_c_3d = c;
	m_b_3d = b;
	m_a_3d = a;
}

void hn3dRoadPcdToImage::line_fitting(std::vector<double>& x, std::vector<double>& y, double &k, double &b )
{
	MatrixXd A_(2, 2), B_(2, 1), A12(2, 1);//A_是个矩阵两行两列，其他同理
	int num_point = x.size();//num_point等于输入的x的总个数
	double A01(0.0), A02(0.0), B00(0.0), B10(0.0);//A01表示第0行第1列//A01(0.0), A02(0.0)表明A矩阵的第一行都初始化为0
	for (int i_point = 0; i_point < num_point; i_point++)
	{
		A01 += x[i_point] * x[i_point];
		A02 += x[i_point];
		B00 += x[i_point] * y[i_point];
		B10 += y[i_point];
	}
	A_ << A01, A02,
		A02, num_point;//把这四个数塞给A矩阵；A矩阵就是推导中的x转置乘x的逆
	B_ << B00,
		B10;//B矩阵就是X转置乘Y
	A12 = A_.inverse()*B_;//A_.inverse()是求A的逆矩阵；
	k = A12(0, 0);//A12是两行一列，那么k就是第0行第0列
	b = A12(1, 0);//b就是第1行第0列
}

void hn3dRoadPcdToImage::enhanceIntensity(int* ptrIntensity)
{
	// 遍历统计强度范围;
	int tempValue = 0;
	int maxInten = 0;
	int minInten = 10000;
	for (int col = 0; col < m_imgHeight; col++)
	{
		for (int row = 0; row < m_imgWidth; row++)
		{
			tempValue = ptrIntensity[row * m_imgHeight + col];
			if (tempValue < 0)
			{
				continue;
			}

			if (tempValue < minInten)
			{
				minInten = tempValue;
			}

			if (tempValue > maxInten)
			{
				maxInten = tempValue;
			}

		}// for (int row = 0; row < m_imgWidth; row++)
	} // for (int col = 0; col < m_imgHeight; col++)

	if (minInten < 1)
	{
		minInten = 1;
	}

	// 强度映射计算;
	float fMinRf = log(1.0 * minInten);
	float fMaxRf = log(1.0 * maxInten);
	float fEnRangeRf = fMaxRf - fMinRf;
	float fCurRf = 0.0;

	// 更新强度值;
	for (int col = 0; col < m_imgHeight; col++)
	{
		for (int row = 0; row < m_imgWidth; row++)
		{
			tempValue = ptrIntensity[row * m_imgHeight + col];
			if (tempValue < 0)
			{
				continue;
			}

			fCurRf = log(1.0 * tempValue);
			if (fCurRf < fMinRf)
			{
				fCurRf = fMinRf;
			}
			if (fCurRf > fMaxRf)
			{
				fCurRf = fMaxRf;
			}

			ptrIntensity[row * m_imgHeight + col] = (fCurRf - fMinRf) * 255.0 / (fMaxRf - fMinRf);
		}// for (int row = 0; row < m_imgWidth; row++)
	} // for (int col = 0; col < m_imgHeight; col++)
}

bool hn3dRoadPcdToImage::createSplitImgs(int startIndex, int endIndex)
{
	if (!m_pavement_reader)
	{
		return false;
	}

	// 获取同步文件中总帧数，一帧对应FRAME_TO_ROW行数据;
	int totalFrameCount = m_pavement_reader->GetScanLines();
	int nImgReadFrameCount = 4000 / FRAME_TO_ROW; // 单张影像需要读取的帧数;
	char strImg[128];

	// 设置分段信息;
	m_pavement_reader->setSplitDatIndex(startIndex, endIndex);
	m_pavement_reader->jumpToSplitStartIndex();

	// 获取分割段的时间范围，有前后限制;
	double split_start_time, split_end_time;
	split_start_time = split_end_time = 0.0;
	m_pavement_reader->getSplitDatTime(split_start_time, split_end_time);

	// 添加时间过滤条件;
	COMBINE_TIME_RANGE time_range;
	time_range.start_gps_second = split_start_time;
	time_range.end_gps_second = split_end_time;
	m_vec_combine_time_range.push_back(time_range);

	// 设置抽稀轨迹线;
	mkLaserRouteLine(m_vecPosInfo);

	// 更新POS容器，减少遍历次数;
	updatePosByTimeRange(m_vec_combine_time_range, m_vecPosInfo, m_vecSubPosInfo);

	// 起始索引帧和结束索引帧;
	int startFrameIndex = startIndex * 100;
	int endFrameIndex = endIndex * 100;

	int calcTotalIndex = endFrameIndex - startFrameIndex + 1;

	int ncount = 0;
	//std::vector<PAVEMENT_IMAGE_INDEX> vecIndexInfo;
	//vecIndexInfo.resize(5000);
	PAVEMENT_IMAGE_INDEX indexInfo;


	//确认渲染方案参数
	//根据渲染方案确定0.01m高程对应色阶数，色阶数越高，适用病害等级越小
	COMBINE_ROAD_3D_SET_STRUCT road_3d_info = m_project_setting->getRoad3dSetInfo();
	switch (road_3d_info.type_disease)
	{
	case 0:
		// -05- +05;
		num_colorsIn1Centimeter = 256;
		break;
	case 1:
		// -10- +10;
		num_colorsIn1Centimeter = 128;
		break;
	case 2:
		// -15- +15;
		num_colorsIn1Centimeter = 85;
		break;
	case 3:
		// -20- +20;
		num_colorsIn1Centimeter = 64;
		break;
	case 4:
		// -25- +25;
		num_colorsIn1Centimeter = 51;
		break;
	case 5:
		// -30- +30;
		num_colorsIn1Centimeter = 42;
		break;
	case 6:
		// -35- +35;
		num_colorsIn1Centimeter = 36;
		break;
	case 7:
		// -40- +40;
		num_colorsIn1Centimeter = 32;
		break;
	}

	for (int iFrame = startFrameIndex; iFrame < endFrameIndex; iFrame += nImgReadFrameCount)
	{
		// 外部强制中断;
		if (!m_is_running)
		{
			continue;
		}

		// 生成单张灰度影像和深度影像;
		bool bSucc = getGreyImage(m_pavement_reader, iFrame, iFrame + nImgReadFrameCount, indexInfo,num_colorsIn1Centimeter);
		//bool bSucc = getGreyImageNewDepth(m_pavement_reader, iFrame, iFrame + nImgReadFrameCount, indexInfo);
		if (!bSucc)
		{
			if (iFrame % 100 == 0)
			{
				sprintf_s(strImg, "当前跳过影像...");
				updateProgress(iFrame * 1.0 / calcTotalIndex, QString::fromLocal8Bit(strImg));
			}

			continue;
		}

		if (iFrame % 100 == 0)
		{
			sprintf_s(strImg, "当前%d - %d 段生成影像...", startIndex, endIndex);
			updateProgress(iFrame * 1.0 / calcTotalIndex, QString::fromLocal8Bit(strImg));
		}
	}

	sprintf_s(strImg, "当前 %d - %d 段处理完成",startIndex,endIndex);
	updateProgress(1.0, QString::fromLocal8Bit(strImg));

	return true;
}

void hn3dRoadPcdToImage::stopWork()
{
	m_is_running = false;
}

bool hn3dRoadPcdToImage::updatePosByTimeRange(std::vector<COMBINE_TIME_RANGE>& vec_combine_time_range, std::vector<POS_STRUCT_INFO>& vec_pos_info, std::vector<POS_STRUCT_INFO>& vecSubPosInfo)
{
	bool isTimeIn = false;
	vecSubPosInfo.clear();
	for (unsigned int n = 0; n < vec_pos_info.size(); n++)
	{
		POS_STRUCT_INFO& posInfo = vec_pos_info[n];
		isTimeIn = isTimeInVector(posInfo.dGpsSecond, vec_combine_time_range);
		if (isTimeIn)
		{
			vecSubPosInfo.push_back(posInfo);
		}
	}

	return true;
}

void hn3dRoadPcdToImage::mkPosFitLine(std::vector<POS_STRUCT_INFO>& vec_pos_info, std::vector<hnPoint3d>& vec_result_ptline)
{
	// 条件判断;
	if (vec_pos_info.size() < 100)
	{
		return;
	}

	// 获取第一个POS点的时间;
	double dStartGpsTime = vec_pos_info[0].dGpsSecond;

	// 定义中间临时变量;
	hnPoint3d point3d;
	int ncount = 0;
	POINT_STRUCT_XYZIT_INFO tempPt;

	// 参数结果值记录;
	vec_result_ptline.clear();
	vec_result_ptline.resize(vec_pos_info.size());

	// 计算轨迹线上的点;
	for (unsigned int n = 0; n < vec_pos_info.size(); n += 2)
	{
		POS_STRUCT_INFO& posInfo = vec_pos_info[n];

		// 计算相机零点位置;
		tempPt.timeSecond = posInfo.dGpsSecond;
		tempPt.x = tempPt.y = tempPt.z = 0.0;
		point3d.x = point3d.y = point3d.z = 0.0;

		// 更新POS构建的旋转矩阵;
		updatePosMatrix(posInfo);

		// 计算绝对坐标;
		calcuCoord(posInfo, tempPt, point3d.x, point3d.y, point3d.z);

		vec_result_ptline[ncount] = point3d;
		ncount++;
	}

	vec_result_ptline.resize(ncount);
}

void hn3dRoadPcdToImage::mkLaserRouteLine(std::vector<POS_STRUCT_INFO>& vec_pos_info)
{
	// 获取零点位置，每隔10ms计算一个点;
	std::vector<hnPoint3d> vec_result_ptline;
	mkPosFitLine(vec_pos_info, vec_result_ptline);

	//// 先简化轨迹数据;
	//SimplifyPolyline(vec_result_ptline, 5);

	//FILE* ptrFile = NULL;
	//fopen_s(&ptrFile, "H:\\test\\checkLine0.csv", "wt+");
	//for (unsigned int n = 0;n < vec_result_ptline.size();n++)
	//{
	//	fprintf_s(ptrFile, "%.3lf,%.3lf,%.3lf\n", vec_result_ptline[n].x, vec_result_ptline[n].y, vec_result_ptline[n].z);
	//}
	//fclose(ptrFile);

	//// 再进行抛物线拟合;
	//SmoothPolyline(vec_result_ptline);

	//// 进行空间曲线拟合点赋值;
	//std::vector<double> vecXs;
	//std::vector<double> vecYs;
	//std::vector<double> vecZs;
	//vecXs.resize(vec_result_ptline.size());
	//vecYs.resize(vec_result_ptline.size());
	//vecZs.resize(vec_result_ptline.size());
	//for (unsigned int n = 0;n < vec_result_ptline.size();n++)
	//{
	//	vecXs[n] = vec_result_ptline[n].x;
	//	vecYs[n] = vec_result_ptline[n].y;
	//	vecZs[n] = vec_result_ptline[n].z;
	//}

	//FILE* ptrFile = NULL;
	//fopen_s(&ptrFile, "H:\\test\\checkLine3.csv", "wt+");

	//int nsplitCount = (int)floor(vec_result_ptline.size() / 50 + 0.5);
	//for (int n = 0;n < nsplitCount;n++)
	//{
	//	vecXs.resize(50);
	//	vecYs.resize(50);
	//	vecZs.resize(50);
	//	int nPtCount = 0;
	//	for (int m = n * 50;m < (n+1)* 50;m++)
	//	{
	//		if (m >= vec_result_ptline.size())
	//		{
	//			continue;
	//		}

	//		vecXs[nPtCount] = vec_result_ptline[m].x;
	//		vecYs[nPtCount] = vec_result_ptline[m].y;
	//		vecZs[nPtCount] = vec_result_ptline[m].z;
	//		nPtCount++;
	//	}

	//	vecXs.resize(nPtCount);
	//	vecYs.resize(nPtCount);
	//	vecZs.resize(nPtCount);

	//	// 拟合空间曲线，曲线方程为z=a*(x^2+y^2)+b*sqrt(x^2+y^2)+c;
	//	double fitA, fitB, fitC;
	//	fitA = fitB = fitC = 0.0;
	//	polynomial3D_fitting(vecXs, vecYs, vecZs, fitA, fitB, fitC);

	//	for (int ij = 0;ij < nPtCount;ij++)
	//	{
	//		double x, y;
	//		x = vecXs[ij];
	//		y = vecYs[ij];

	//		double tempZ = fitA * (x * x + y * y) + fitB * sqrt(x * x + y * y) + fitC;
	//		fprintf_s(ptrFile, "%.3lf,%.3lf,%.3lf,%.3lf\n", vecXs[ij], vecYs[ij], vecZs[ij], tempZ);
	//	}
	//}

	//fclose(ptrFile);

	//FILE* ptrFile = NULL;
	//fopen_s(&ptrFile, "H:\\test\\1.xyz", "rt");
	//char strLine[1024];
	//vec_result_ptline.clear();
	//while (!feof(ptrFile))
	//{
	//	memset(strLine, 0, 1024);
	//	fgets(strLine, 1024,ptrFile);

	//	double dx, dy, dz;
	//	int nret = sscanf_s(strLine, "%lf,%lf,%lf\n",&dx,&dy,&dz);
	//	if (nret < 3)
	//	{
	//		continue;
	//	}
	//	hnPoint3d point;
	//	point.x = dx;
	//	point.y = dy;
	//	point.z = dz;
	//	vec_result_ptline.push_back(point);
	//}
	//fclose(ptrFile);

	// 简化轨迹数据;
	SimplifyPolyline(vec_result_ptline, 10);


	//构建多段线（多段线是不准确的，应考虑构建曲线）;
	m_vec_fit_laser_ptline.resize(vec_result_ptline.size() - 1);
	for (unsigned int n = 0; n < vec_result_ptline.size() - 1; n++)
	{
		irr::core::line3dd line(vec_result_ptline[n].x, vec_result_ptline[n].y, vec_result_ptline[n].z,
			vec_result_ptline[n + 1].x, vec_result_ptline[n + 1].y, vec_result_ptline[n + 1].z);
		m_vec_fit_laser_ptline[n] = line;
	}
}

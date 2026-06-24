#ifndef GETGLOBALCOORDINATE_H
#define GETGLOBALCOORDINATE_H

#include "getglobalcoordinate_global.h"
#include <vector>
#include <iostream>
#include "..\hnConvert\hnDataCombineStructInfo.h"
#include <io.h>
#include <QString>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include "../hdGeoPosition/GeoProjection/inc/IHdPJTranslator.h"
#include "..\3rd\Eigen\Dense"
#include "..\3rd\Eigen\src\Core\Matrix.h"
//#include "..\hnDataCommon\hnPoint.h"
//#include "..\hnDataCommon\hnDataStruct.h"

using namespace std;
//using namespace hnCommon;

//打开工程同步的时候
//1.读POS得到经纬度 ok

//2.经纬度转绝对坐标XYZ ok

//3.读DB获得7参数矩阵Rpt2pos，Tpt2pos  ok

//4.读config获得延迟时间18s ok

//点云拟合的时候
//5.获得拟合点云的帧数中xyz和对应的时间t

//6.点云的时间差值找POS时间 

//7.根据pos时间得到Rpos，Tpos

//8.绝对坐标转换Global=Rpos[Rpt2pos*PtLoc+Tpt2pos]+Tpos

class  _declspec(dllexport) hnPcdCoordinate
{
public:
	hnPcdCoordinate();
	~hnPcdCoordinate();
public:
	//1.读POS得到经纬度// 加载POS数据至内存
	bool loadPosData(const char* strPosPath, std::vector<POS_STRUCT_INFO>& vecInfo ,bool(*pProgress)(float fVal, const char* qstrName, bool bCancle) = NULL,bool cancel=false);

	//1.读POS得到经纬度// 加载POS数据至内存
	bool loadPosDataTest(const char* strPosPath, std::vector<POS_STRUCT_INFO>& vecInfo,double dFirGpTime);

	//2.经纬度转绝对坐标XYZ// 将vecInfo中的经纬度坐标转换投影坐标;
	void convertBlhToNeh(std::vector<POS_STRUCT_INFO>& vecInfo);

	//3.读config获得延迟时间18s 设置扫描的年月日信息,读取ISACN参数  构造按iscan旋转角度方式Z-X-Y构建旋转矩阵;
	bool setiScanParaPath(int scan_no, const char* str_iscan_para_path, Eigen::Matrix<double, 4, 4> &PtsToPos);

	//3-1.设置扫描的年月日信息,读取ISACN参数  构造按iscan旋转角度方式Z-X-Y构建旋转矩阵;
	void setScanPara(double laser_to_pos_x, double laser_to_pos_y, double laser_to_pos_z,
		double laser_to_pos_heading, double laser_to_pos_pitch, double laser_to_pos_roll, Eigen::Matrix<double, 4, 4> &PtsToPos);
	
	//3-2按iscan旋转角度方式Z-X-Y构建旋转矩阵;
	void computeMatrixByIScanAngle(double X, double Y, double Z, double Yaw, double Pitch, double Roll, double *R);

	//5通过插值计算获取点时间对应的POS点位置、姿态信息，采用线性插值计算;根据时间查询距离该点时间最近的记录值索引;
	bool linearInsertPos(double gpsTime, vector<POS_STRUCT_INFO>& vecInfo, POS_STRUCT_INFO& insertResult, int& nearestIndex);

	//5-1 根据时间查询距离该点时间最近的记录值索引;
	int findIndexByGpsTime(double gpsTime, vector<POS_STRUCT_INFO>& vecInfo);
	
	//6计算点的绝对三维坐标，通过激光到pos，pos到绝对坐标系进行矩阵运算，坐标值传入传出；
	void calcuCoord(Eigen::Matrix<double, 4, 4> &PtsToPos, POS_STRUCT_INFO& posInfo, double& dx, double& dy, double& dz);

	//2020.9.21实现3d转2d
	void dTo2d(Eigen::Matrix<double, 4, 4> &PtsToPos, POS_STRUCT_INFO& posInfo, double& dx, double& dy, double& dz);

	//Pos位置找帧
	/*void frameByPos(double dGpsTime, vector<hnFrameGpsInfo>&vecFrameGpsInfo,
		int& nOutFrame);*/

private:
	vector<double>m_vecGpsTime;
//	vector<PointXYZIRGB>m_vecPts;
	// 记录GPS与UTC的时间跳秒值;
	double m_utc_to_gps_second;


	// 记录激光中心到pos的位置姿态值，便于构建旋转矩阵;
	double m_laser_topos_x;
	double m_laser_topos_y;
	double m_laser_topos_z;
	double m_laser_topos_heading;
	double m_laser_topos_pitch;
	double m_laser_topos_roll;

	// 用于记录上一次个点所在的POS索引值
	int m_nPreIndex;


public:
	//二分法查找最近位置double
	bool DichotomyFindNearestLoc(vector<double>&vec, double Target, int &nNearestLoc);

	//4.获得拟合点云的帧数中xyz和对应的时间t
	//void GetPtsCoordinateAndGpsTime(vector<double>&vecGpsTime, vector<PointXYZIRGB>&vecPts);
};

#endif // GETGLOBALCOORDINATE_H

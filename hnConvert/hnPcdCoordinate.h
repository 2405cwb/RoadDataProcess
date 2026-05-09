#ifndef GETGLOBALCOORDINATE_H
#define GETGLOBALCOORDINATE_H

#include <vector>
#include <iostream>
#include "hnDataCombineStructInfo.h"
#include <io.h>
#include "../hdGeoPosition/GeoProjection/inc/IHdPJTranslator.h"
#include "..\3rd\Eigen\Dense"
#include "..\3rd\Eigen\src\Core\Matrix.h"
#include "hnDataCombineStructInfo.h"
#include "hnconvert_global.h"
//#include "../hnITSFileIO/hnCloudDefine.h"

using namespace std;

class IHdPJTranslator;

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

class HNCONVERT_EXPORT hnPcdCoordinate
{

public:
	hnPcdCoordinate();
	~hnPcdCoordinate();
public:

	//0时间转换
	bool UTCT2GPST(const hnSynInfo& stTime, int& nGpsWeek,
		double& dGpsSeconds, double dGPSSubUTC= 18.0);

	//1.读POS得到经纬度// 加载POS数据至内存
	bool loadPosData(const char* strPosPath, std::vector<hnPosInfo>& vecInfo);

	//1.读POS得到经纬度// 加载POS数据至内存
	bool loadPosDataTest(const char* strPosPath, std::vector<hnPosInfo>& vecInfo,double dFirGpTime);

	//2.经纬度转绝对坐标XYZ// 将vecInfo中的经纬度坐标转换投影坐标;
	void convertBlhToNeh(std::vector<hnPosInfo>& vecInfo, bool isProj = false);

	//3.读config获得延迟时间18s 设置扫描的年月日信息,读取ISACN参数  构造按iscan旋转角度方式Z-X-Y构建旋转矩阵;
	bool setiScanParaPath(int scan_no, const char* str_iscan_para_path, Eigen::Matrix<double, 4, 4> &PtsToPos);

	//3-1.设置扫描的年月日信息,读取ISACN参数  构造按iscan旋转角度方式Z-X-Y构建旋转矩阵;
	void setScanPara(double laser_to_pos_x, double laser_to_pos_y, double laser_to_pos_z,
		double laser_to_pos_heading, double laser_to_pos_pitch, double laser_to_pos_roll, Eigen::Matrix<double, 4, 4> &PtsToPos);
	
	//3-2按iscan旋转角度方式Z-X-Y构建旋转矩阵;
	void computeMatrixByIScanAngle(double X, double Y, double Z, double Yaw, double Pitch, double Roll, double *R);

	//5通过插值计算获取点时间对应的POS点位置、姿态信息，采用线性插值计算;根据时间查询距离该点时间最近的记录值索引;
	bool linearInsertPos(double gpsTime, vector<hnPosInfo>& vecInfo, hnPosInfo& insertResult, int& nearestIndex);

	//5-1 根据时间查询距离该点时间最近的记录值索引;
	int findIndexByGpsTime(double gpsTime, vector<hnPosInfo>& vecInfo);
	
	//6计算点的绝对三维坐标，通过激光到pos，pos到绝对坐标系进行矩阵运算，坐标值传入传出；
	void calcuCoord(Eigen::Matrix<double, 4, 4> &PtsToPos, hnPosInfo& posInfo, double& dx, double& dy, double& dz);

	//2020.9.21实现3d转2d
	void dTo2d(Eigen::Matrix<double, 4, 4> &PtsToPos, hnPosInfo& posInfo, double& dx, double& dy, double& dz);


	// 设置进度条回调函数;
	void(*loadCallback)(float, const char*);

	////Pos位置找帧
	//void frameByPos(double dGpsTime, vector<hnFrameGpsInfo>&vecFrameGpsInfo,
	//	int& nOutFrame);

private:
	// 封装接口，直接进行经纬度转换投影坐标;
	void convertBLtoProjXYDirect(IHdPJTranslator* converter, std::vector<POS_STRUCT_INFO>& vecInfo);

	// 计算点的绝对三维坐标，通过激光到pos，pos到绝对坐标系进行矩阵运算，坐标值传入传出；
	void calcuCoord2(Eigen::Matrix<double, 4, 4> &PtsToPos,POS_STRUCT_INFO& posInfo, double& dx, double& dy, double& dz);

	// POS当地水平参考坐标系到WGS84空间直角坐标系转换矩阵 3* 3；
	void computeMatrixByBL(double X, double Y, double Z, double longtitude, double latitude, double *R);

private:

	//三维点云转换关系
	Eigen::Matrix<double, 4, 4>m_PtsToPos;
	
	//pos信息
	std::vector<hnPosInfo>m_vecPosInfo;


	//vector<double>m_vecGpsTime;
	//vector<PointXYZIRGB>m_vecPts;
	// 记录GPS与UTC的时间跳秒值;
	double m_utc_to_gps_second;


	// 记录激光中心到pos的位置姿态值，便于构建旋转矩阵;
	double m_laser_topos_x;
	double m_laser_topos_y;
	double m_laser_topos_z;
	double m_laser_topos_heading;
	double m_laser_topos_pitch;
	double m_laser_topos_roll;

	IHdPJTranslator* m_ptr_convert_translator;

public:
	//二分法查找最近位置double
	bool DichotomyFindNearestLoc(vector<double>&vec, double Target, int &nNearestLoc);

	//4.获得拟合点云的帧数中xyz和对应的时间t
	//void GetPtsCoordinateAndGpsTime(vector<double>&vecGpsTime, vector<PointXYZIRGB>&vecPts);
};

#endif // GETGLOBALCOORDINATE_H

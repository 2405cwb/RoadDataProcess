#pragma once
#include "hnZfsReader.h"
#include "hnDataCombineStructInfo.h"
#include "..\hnPositionProj\IHdPJTranslator.h"
#include "..\Common\Eigen\Dense"
#include <vector>
#include <iostream>
#include <QObject>

using namespace std;

namespace hn
{
	class hnProjectSetting;
}

namespace hd
{
	namespace geoproj
	{
		class hdCoordinateTransformation;
	}
}

class hnZfsCombine : public QObject
{
	Q_OBJECT
public:
	hnZfsCombine(void);
	~hnZfsCombine(void);

signals:
	// 信号标记，主要用于进度处理;
	void progress(float p,QString msg);
public:
	// 线程任务接口
	void stopWork();

	// 用于验证POS控制点改正;
	int calcuPosCorrect(const char* str_old_pos_path,double start_time,double end_time,const char* str_save_pos_path,const char* str_cpt_path);
	void loadCptFile(const char* str_cpt_file_path,std::vector<POS_CPT_STRUCT_INFO>& vec_cpt_info);
	double getPosDist(std::vector<POS_STRUCT_INFO>& vecInfo,int start_index,int end_index);
	void writePosData(const char* strPosSavePath,std::vector<POS_STRUCT_INFO>& vecInfo);


	// 设置标记使用线程，需要自定义进度条信息;
	void setUseThread(bool use_thread);

	// 通过插值查找最近时间点，并获取对应的投影坐标及方位角信息，写入文件;
	void ctrlPointCal(const char* str_pos_path,const char* str_ctrl_file,const char* str_ctrl_result);

	// HLS转换HLZ格式;
	int convertHlsToHlz(const char* str_hls_path,const char* str_hlz_path);

	// 测试代码，确定LIN文件输出的位置姿态信息值;
	void testCalLinPos(const char* str_pos_path,const char* str_lin_path);

	void convertBlhToNehN(POS_STRUCT_INFO& posInfo);
private:
	// 将vecInfo中的经纬度坐标转换投影坐标;
	void convertBlhToNeh(std::vector<POS_STRUCT_INFO>& vecInfo);

	// 根据时间查询距离该点时间最近的记录值索引;
	int findIndexByGpsTime(double gpsTime,vector<POS_STRUCT_INFO>& vecInfo);

	// 通过插值计算获取点时间对应的POS点位置、姿态信息，采用线性插值计算;
	bool linearInsertPos(double gpsTime,vector<POS_STRUCT_INFO>& vecInfo,POS_STRUCT_INFO& insertResult,int& nearest_index);

	// 按iscan旋转角度方式Z-X-Y构建旋转矩阵;
	void computeMatrixByIScanAngle( double X,double Y,double Z,double Yaw,double Pitch,double Roll,double *R );

	// 计算点的绝对三维坐标，通过激光到pos，pos到绝对坐标系进行矩阵运算，坐标值传入传出；
	void calcuCoord(POS_STRUCT_INFO& posInfo,double& dx,double& dy,double& dz);

	// 根据控制点信息计算距离最近点POS，并记录该点位POS点的时刻值
	void calcuCtrlPosTime(std::vector<COMBINE_CTRL_POINT_STRUCT_INFO> vec_ctrl_points,double dist_tol,
		std::vector<POS_STRUCT_INFO>& vec_pos_info,std::vector<COMBINE_TIME_RANGE>& vec_combine_time_range );

	// 获取时间范围内的记录;
	bool getCtrlTimeInTime(double gps_time,std::vector<COMBINE_TIME_RANGE>& vec_time_in);

	// 判断点是否在控制点范围内:
	bool isPointInCtrlRadius(double point_x,double point_y,double point_z,std::vector<COMBINE_CTRL_POINT_STRUCT_INFO> vec_ctrl_points,
		double dist_tol);

	// utc转换gps周秒;
	bool UTCT2GPST(const DATE_TIME_INFO& stTime,int& nGpsWeek,double& dGpsSeconds,double dGPSSubUTC= 0.);

	// 融合解算，生成HLZ,不输出time文件;
	bool dataCombineHlz(const char* strZfsPath,const char* strPosPath,const char* strHlzPath,const char* strTimePath);

	// 融合解算，生成HLS；
	bool dataCombineHls(const char* strZfsPath,const char* strPosPath,const char* strHlsPath,const char* strTimePath);
public:
	// 设置融合解算相关参数;
	void setCombineSetting(hn::hnProjectSetting* project_setting);

	// 加载POS数据至内存
	bool loadPosData(const char* strPosPath,std::vector<POS_STRUCT_INFO>& vecInfo);

	// 设置传入控制点路径，若调用，表明使用控制点;
	bool setCtrlPointPath(const char* strCtrlPath);

	// 设置传入iScan-para.db路径，与setScanPara互通;
	bool setiScanParaPath(int scan_no,const char* str_iscan_para_path);

	// 处理转换zfs文件，坐标换算至绝对坐标系;
	bool dataCombine(const char* strZfsPath,const char* strPosPath,const char* strHlsPath,const char* strTimePath);

	// 导出Lin文件;
	bool exportLinFile(double start_time,double end_time,const char* strLinPath,std::vector<POS_STRUCT_INFO>& vecInfo);

	// 设置扫描的年月日信息;
	void setScanDate(int year,int month,int day);

	// 设置iScan的扫描标定角度值;
	void setScanPara(double laser_to_pos_x,double laser_to_pos_y,double laser_to_pos_z,
		double laser_to_pos_heading,double laser_to_pos_pitch,double laser_to_pos_roll);

	// 设置进度信息;
	void setProgress(float,const char*);

	// 设置进度条回调函数;
	void (*loadCallback)(float,const char*);
	//void (*loadCallback)(float,const char*);

	// 设置进度条回调函数;
	void (*wloadCallback)(float,const wchar_t*);
private:
	// 记录激光中心到pos的位置姿态值，便于构建旋转矩阵;
	double m_laser_topos_x;
	double m_laser_topos_y;
	double m_laser_topos_z;
	double m_laser_topos_heading;
	double m_laser_topos_pitch;
	double m_laser_topos_roll;
	DATE_TIME_INFO m_scan_date_info;

	// 记录GPS与UTC的时间跳秒值;
	double m_utc_to_gps_second;

	// 用于记录上一次个点所在的POS索引值
	int m_nPreIndex;

	// 记录标记是否使用控制点文件;
	bool m_use_ctrl_pt;
	std::vector<COMBINE_CTRL_POINT_STRUCT_INFO> m_vec_ctrl_points;

	// 记录传入设置的时间段融合信息;
	std::vector<COMBINE_TIME_RANGE> m_vec_combine_time_range;

	// iscan-para.db构建的旋转矩阵，直接用成员变量记录，减少矩阵构建计算量;
	//MatrixXd m_mat_pos_to_world(4,4);
	Eigen::Matrix<double,4,4> m_mat_laser_to_pos;

	// 融合设置相关参数;
	hn::hnProjectSetting* m_project_setting;

	// 坐标投影参数设置对象;
	Spatial_Ref_t    m_src_param;				// 原始数据转换参数，默认即为WGS84;
	Spatial_Ref_t    m_dst_param;				// 目标数据转换参数;
	IHdPJTranslator* m_ptr_convert_translator;
	bool m_use_thread;

	// 状态标记当前是否正在融合处理;
	bool m_is_running;

	// wgs84等相关经纬度坐标转换，由于emap坐标转换存在0.003m左右的差值，采用hdProjectedCoordinateSystem进行投影转换,m_ptr_convert_translator进行四参数、七参数转换;
	hd::geoproj::hdCoordinateTransformation* m_ptr_coord_trans_system;
};


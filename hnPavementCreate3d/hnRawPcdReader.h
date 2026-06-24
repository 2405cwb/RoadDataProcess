#ifndef HNRAWPCDREADER_H
#define HNRAWPCDREADER_H
#include "..\hnConvert\hnDataCombineStructInfo.h"
#include <vector>
#include "hnpavementcreate3d_global.h"

enum ENUM_RAW_SCAN_TYPE
{
	ENUM_RAW_SCAN_UNKNOW = 0,
	ENUM_RAW_SCAN_LEISHEN_32 = 1,
	ENUM_RAW_SCAN_VELODYNE_16 = 2,
	ENUM_RAW_SCAN_LEISHEN_16 = 3,
	ENUM_RAW_SCAN_ZFS = 4,
	ENUM_RAW_SCAN_BKTH_16 = 5,
	ENUM_RAW_SCAN_PAVEMENT_CAM = 6,
	ENUM_RAW_SCAN_NIMI_VUX = 7,
	ENUM_RAW_SCAN_BKTH_32 = 8,
	ENUM_RAW_SCAN_HNGD_01 = 10,
	ENUM_RAW_SCAN_LJHN_JZ = 11,
	ENUM_RAW_SCAN_LJHN_TW = 12,
	ENUM_RAW_SCAN_LJTM_16 = 13,
	ENUM_RAW_SCAN_ITS_HN = 14,
	ENUM_RAW_SCAN_LJHN_MULTI_TW = 15,
	ENUM_RAW_SCAN_ITE_HN = 16
};

class HNPAVEMENTCREATE3D_EXPORT hnRawPcdReader
{
public:

	hnRawPcdReader(void);
	virtual ~hnRawPcdReader(void);

	// 打开文件读取,重载实现;
	virtual bool Open(const char* path) = 0;

	// 关闭文件，重载实现;
	virtual bool Close() = 0;

	// 开始读取，重载实现;
	virtual void startRead() = 0;

	// 读取一帧数据，重载实现;
	virtual bool getLinePoints(int idx,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count) = 0;

	// 获取点云总帧数，重载实现;
	virtual int GetScanLines() = 0;

	// 通用接口实现，获取扫描持续时间,重载实现;
	virtual bool getScanTimeRange(double& start_gps_time,double& end_gps_time) = 0;

	// 根据设置传入的时间信息获取距离该时间前后一定范围的帧数据索引，重载实现;
	virtual void getLinesIndex(std::vector<int>& vec_result_index,void (*processCallback)(float,const char*) = NULL) = 0;

	// 传入帧编号获取当前帧GPS时间;
	virtual void getLineIndexTime(int curIndex,double& gpsTime);

	// 设置激光扫描数据类型;
	virtual void setScanType(int scanType);

	// 获取边线数据接口，重载实现，仅支持3d路面数据重载实现;
	virtual bool getRoadLinesPoints( int idx,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count );

public:

	// 通用接口实现，设置采集开始的年月日时间，以便后续计算GPS周秒，注意此处应传入为UTC时间系统;
	virtual void setScanTime(DATE_TIME_INFO& date_info);

	// 通用接口实现，设置utc转换gps秒的时间差值，目前（2019年）为18秒;
	void setUtcToGpsAbs(double second);

	// 通用接口实现，设置点云系统延时，单位为秒;
	void setOffsetTime(double offsetTime);

	// 通用接口实现，设置时间过滤段;
	void setTimeRange(std::vector<COMBINE_TIME_RANGE>& vec_combine_time_range);

	// utc转换gps周秒;
	bool UTCT2GPST(const DATE_TIME_INFO& stTime,int& nGpsWeek,double& dGpsSeconds,double dGPSSubUTC= 0.);

	// 设置同步时刻时间信息，用于校正未同步时间段信息;
	void setGpsSynInfo(SYN_UPDATE_STRUCT& synInfo);

	// 设置是否融合单线数据，及设置单线索引;
	void setExportSingleLine(int needSingle,int singleIndex);

	// 设置是否角度过滤，及角度过滤范围;
	void setUseAngleFilter(int useAngleFilter,double angleStart,double angleEnd);

protected:
	// 时间范围过滤设置;
	std::vector<COMBINE_TIME_RANGE> m_vec_combine_time_range;

	// 外部设置的日期信息;
	DATE_TIME_INFO m_scan_date_time;

	// 设置utc转gps时的跳秒值;
	double m_utc_to_gps_abs;

	// 外部设置的时间延时;
	double m_offset_time;

	// 记录当前存储数据格式;
	ENUM_RAW_SCAN_TYPE m_scan_type;

	// 记录同步时刻的时间信息;
	SYN_UPDATE_STRUCT m_syn_update_info;

	// 设置是否单线融合，融合哪一线;
	int m_need_export_single_line;
	int m_export_single_index;

	// 设置角度过滤;
	int m_use_angle_filter;
	double m_angle_filter_start;
	double m_angle_filter_end;
};

#endif // HNRAWPCDREADER_H
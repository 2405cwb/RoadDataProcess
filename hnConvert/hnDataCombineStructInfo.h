#pragma once
#include <stdio.h>
#include <memory>
#include <io.h>
#include <direct.h>
#include <string>
#include <vector>
#include "..\hnCommon\hnCommonDef.h"

typedef struct POS_STRUCT_INFO
{
	POS_STRUCT_INFO()
	{
		dGpsSecond = dEastCoord = dNorthCoord = dHeight = dHeading = dPitch = dRoll = dLatitude = dLongitude = 0.0;
		fVEast = fVNorth = fVUp = 0.0f;
		nQ = 0;
		fUnknow1 = 0.0f;
		fUnknow2 = 0.0f;
		fUnknow3 = 0.0f;
		fUnknow4 = 0.0f;
		fUnknow5 = 0.0f;
		fUnknow6 = 0.0f;
	}

	bool serialize(const char* strData)
	{
		//int nSize = sscanf_s(strData,"%lf	%lf	%lf	%lf	%lf	%lf	%lf	%lf	%lf	%f	%f	%f	%d\n",
		//	&dGpsSecond,&dEastCoord,&dNorthCoord,&dHeight,&dHeading,&dPitch,&dRoll,&dLatitude,&dLongitude,
		//	&fVEast,&fVNorth,&fVUp,&nQ);
		//return nSize >= 13? true:false;
		int nSize = sscanf_s(strData,"%lf	%lf	%lf	%lf	%lf	%lf	%lf	%f	%f	%f	%f	%f	%f	%f	%f	%f	%d\n",
			&dGpsSecond,&dLatitude,&dLongitude,&dHeight,&dHeading,&dPitch,&dRoll,
			&fVEast,&fVNorth,&fVUp,
			&fUnknow1,&fUnknow2,&fUnknow3,&fUnknow4,&fUnknow5,&fUnknow6,&nQ);
		if (nSize < 17)
		{
			//2021.9.7 是否为13个值
			int n = 0;
			double d1, d2;
			nSize = sscanf_s(strData, "%lf %lf %lf	%lf %lf %lf %lf %lf %lf %f %f %f %d\n",
				&dGpsSecond, &d1, &d2, &dHeight, &dHeading, &dPitch, &dRoll,
				&dLatitude, &dLongitude, &fUnknow3, &fUnknow4, &fUnknow5, &n);

			if (nSize != 13)
			{
				nSize = sscanf_s(strData, "%lf	%lf	%lf	%lf	%lf	%lf	%lf	%f	%f	%f\n",
					&dGpsSecond, &dLatitude, &dLongitude, &dHeight, &dHeading, &dPitch, &dRoll,
					&fVEast, &fVNorth, &fVUp);
			}
			
		}

		//dHeading += 4.5245076696;

		//return nSize >= 17? true : false;

		//int nSize = sscanf_s(strData,"%lf	%lf	%lf	%lf	%lf	%lf	%lf	%f	%f	%f\n",
		//	&dGpsSecond,&dLatitude,&dLongitude,&dHeight,&dHeading,&dPitch,&dRoll,
		//	&fVEast,&fVNorth,&fVUp);
		return nSize >= 10? true : false;

		//return nSize;
	}

	void reserialize(char** strOutput,int count)
	{
		//sprintf_s(*strOutput,count,"%.3lf	%.3lf	%.3lf	%.3lf	%.10lf	%.10lf	%.10lf	%.10lf	%.10lf	%.3f	%.3f	%.3f	%d\n",
		//	dGpsSecond,dEastCoord,dNorthCoord,dHeight,dHeading,dPitch,dRoll,dLatitude,dLongitude,fVEast,fVNorth,fVUp,nQ);

		int nSize = sprintf_s(*strOutput,count,"%.3lf	%.12lf	%.12lf	%.6lf	%.6lf	%.6lf	%.6lf	%.3f	%.3f	%.3f	%.3f	%.3f	%.3f	%.3f	%.3f	%.3f	%d\n",
			dGpsSecond,dLatitude,dLongitude,dHeight,dHeading,dPitch,dRoll,
			fVEast,fVNorth,fVUp,
			fUnknow1,fUnknow2,fUnknow3,fUnknow4,fUnknow5,fUnknow6,nQ);
	}

	double dGpsSecond; // gps time
	double dEastCoord; // 东向坐标,L
	double dNorthCoord;// 北向坐标,B
	double dHeight;    // 高程
	double dHeading;   // 航向角
	double dPitch;     // 俯仰角
	double dRoll;      // 横滚
	double dLatitude;  // 纬度
	double dLongitude; // 经度
	float fVEast;      // 东向速度
	float fVNorth;     // 北向速度
	float fVUp;        // 天顶速度
	float fUnknow1;
	float fUnknow2;
	float fUnknow3;
	float fUnknow4;
	float fUnknow5;
	float fUnknow6;

	int nQ;            // 质量
}hnPosInfo;


struct SYN_UPDATE_STRUCT
{
	SYN_UPDATE_STRUCT()
	{
		unsynTime.year = 0;
		unsynTime.month = 0;
		unsynTime.day = 0;
		unsynTime.hour = 0;
		unsynTime.minute = 0;
		unsynTime.second = 0;
		unsynTime.milliseconds = 0;

		synTime.year = 0;
		synTime.month = 0;
		synTime.day = 0;
		synTime.hour = 0;
		synTime.minute = 0;
		synTime.second = 0;
		synTime.milliseconds = 0;
		synTimeDist = 0.0;
		useSyn = false;

		unsynWeek = synWeek = 0;
		unsynSec = synSec = 0.0;
	}

	bool useSyn;        // 若存在同步文件，则需要进行判断;
	DATE_TIME_INFO unsynTime;   // 同步时刻的本地系统时间值;
	DATE_TIME_INFO synTime;     // 同步时刻的GPS系统时间值;
	double synTimeDist; // 同步时间差，秒内值;

	int unsynWeek;
	double unsynSec;
	int synWeek;
	double synSec;
};

// 三维点数据
typedef struct _3D_POINT_
{
	_3D_POINT_()
	{
		x = 0.0;
		y = 0.0;
		z = 0.0;
	}

	_3D_POINT_(double dx, double dy, double dz)
	{
		x = dx;
		y = dy;
		z = dz;
	}

	// 重载操作符
	bool operator==(const _3D_POINT_& pTargetPt)
	{
		if (this->x == pTargetPt.x && this->y == pTargetPt.y && this->z == pTargetPt.z)
		{
			return true;
		}

		return false;
	}

	void clear()
	{
		x = 0.0;
		y = 0.0;
		z = 0.0;

	}

	// 点坐标;
	double x;
	double y;
	double z;

} hnPoint3d;

struct POINT_STRUCT_XYZIT_INFO
{
	POINT_STRUCT_XYZIT_INFO()
	{
		x = y = z = 0.0;
		intensity = 0;
		timeSecond = 0.0;
	}

	bool isValid()
	{
		bool isSuc = false;
		if (abs(x) <= 0.0001 && abs(y) <= 0.0001 && abs(z) <= 0.0001)
		{
			isSuc = false;
		}
		else
		{
			isSuc = true;
		}

		return isSuc;
	}

	double x; // 该点坐标
	double y;
	double z;
	int intensity;// 该点反射强度
	double timeSecond;// 该点记录已转换为周秒
};

struct COMBINE_CTRL_POINT_STRUCT_INFO
{
	COMBINE_CTRL_POINT_STRUCT_INFO()
	{
		memset(ctrl_name,0,128);
		ctrl_coord_x = ctrl_coord_y = ctrl_coord_z = 0.0;
	}

	// 序列化一行信息;
	bool serialize(const char* str_line_data)
	{
		int size_count = sscanf_s(str_line_data,"%s %lf %lf %lf\n",
			ctrl_name,128,&ctrl_coord_x,&ctrl_coord_y,&ctrl_coord_z);
		if (size_count < 4)
		{
			return false;
		}

		return true;
	}

	// cpt格式定义;
	char ctrl_name[128];
	double ctrl_coord_x;
	double ctrl_coord_y;
	double ctrl_coord_z;

};

struct COMBINE_TIME_RANGE
{
	COMBINE_TIME_RANGE()
	{
		start_gps_second = end_gps_second = 0.0;
	}
	double start_gps_second;
	double end_gps_second;
};

struct POS_CPT_STRUCT_INFO
{
	POS_CPT_STRUCT_INFO()
	{
		index = 0;
		gps_time = latitude = longtitude = height = 0.0;
		lever_x = lever_y = lever_z =stddev_x = stddev_y = stddev_z = 0.0f;
		dx = dy = 0.0;
	}

	bool serialize(char* str_line)
	{
		char str[128];
		int nret = sscanf_s(str_line,"%s = %d %lf %lf %lf %lf %f %f %f %f %f %f\n",
			str,128,&index,&gps_time,&latitude,&longtitude,&height,
			&lever_x,&lever_y,&lever_z,&stddev_x,&stddev_y,&stddev_z);
		if (nret >= 12)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	
	int index;
	double gps_time;
	double dx;
	double dy;
	double latitude;
	double longtitude;
	double height;
	float lever_x;
	float lever_y;
	float lever_z;
	float stddev_x;
	float stddev_y;
	float stddev_z;
};

/*! @class hdHiScanLidarPara hdHiScanRouteDefines.h
* @ingroup hdHiScanRoute
* @brief 激光扫描仪检校参数 数据表结构
*/
struct hdHiScanLidarPara
{
	//! 构造函数，赋初值
	hdHiScanLidarPara()
	{
		//memset(hiscan_no, 0, DEVICE_NO_LEN);
		//memset(hiscan_lidar_no, 0, SENSE_NO_LEN);
		dx = dy = dz = 0.0f;
		dyaw = dpitch = droll = 0.0f;
		//memset(str_memo_info, 0, REMARK_LEN);
	}

	////! iScan系统编号
	//char hiscan_no[DEVICE_NO_LEN];

	////! 传感器编号
	//char hiscan_lidar_no[SENSE_NO_LEN];

	////! 参数检校时间
	//LOCAL_TIME para_time;

	//! 相对位置X
	double dx;

	//! 相对位置Y
	double dy;

	//! 相对位置Z
	double dz;

	//! X轴夹角
	double dyaw;

	//! Y轴夹角
	double dpitch;

	//! Z轴夹角
	double droll;

	////! 备注
	//char str_memo_info[REMARK_LEN];
};

// 时间系统
typedef struct HN_DATA_TIME
{
	HN_DATA_TIME()
	{
		year = 2000;
		month = 1;
		day = 1;
		hour = 0;
		minute = 0;
		second = 0;
		milliSecond = 0;
		wMilliSecond = 0;
	}

	// 时间定义 年月日时分秒毫秒微秒
	unsigned short year;
	unsigned short month;
	unsigned short day;
	unsigned short hour;
	unsigned short minute;
	unsigned short second;
	unsigned short milliSecond;
	unsigned short wMilliSecond;

	////! 将字符串转为时间
	//inline bool FromTimeString(const char* strTime)
	//{
	//	// 有效性检查
	//	if (strTime != NULL)
	//	{
	//		u32 wYear, wMonth, wDay, wHour, wMinute, wSecond, wMilliSecond;
	//		int count = sscanf_s(strTime, "%04u%02u%02u%02u%02u%02u%03u", &wYear, &wMonth, &wDay, &wHour, &wMinute, &wSecond, &wMilliSecond);
	//		year = wYear;
	//		month = wMonth;
	//		day = wDay;
	//		hour = wHour;
	//		minute = wMinute;
	//		second = wSecond;
	//		milliSecond = wMilliSecond;
	//		return count == 7;
	//	}
	//	return false;
	//}
}hnTime;

// 同步信息
typedef struct HN_SYN_INFO
{
	// 帧号
	unsigned long long nFrame;

	// 时间
	hnTime timeData;

	// 编码器值
	unsigned long long nEncl;
}hnSynInfo;

inline void SplitString(const std::string &s, std::vector<std::string> &v, const std::string &c)
{
	std::string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
	{
		v.push_back(s.substr(pos1));
	}
}

inline void CreateFolder(std::string folderpath)
{
	std::vector<std::string> vecSegTag;
	SplitString(folderpath, vecSegTag, "/");

	std::string PartDir = "";
	//std::vector<std::string>::iterator it;
	//for(it=vecSegTag.begin(); it!=vecSegTag.end(); ++it)
	for (int nn = 0; nn < vecSegTag.size(); nn++)
	{
		std::string str_temp = vecSegTag[nn];
		if (nn != 0)
		{
			PartDir += "/";
		}

		PartDir += str_temp;

		//PartDir = PartDir+"/"+*it;
		if (_access(PartDir.c_str(), 0) == -1)
		{
			if (mkdir(PartDir.c_str()) == 0)
			{
				//                cout<<"folder "<<PartDir<<" creat successed!"<<endl;
			}
			else
			{
				//char Message[128];
				//sprintf(Message, "folder %s creat failed!", PartDir.c_str());
				//WriteLog(Message);
				//printf("%s\n", Message);
			}
			//system(("echo 'whuwhu'|sudo -S chmod 777 "+PartDir).c_str());
		}
	}
}




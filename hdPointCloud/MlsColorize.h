/*! @file
********************************************************************************
<PRE>
模块名       : HD_3LS_SCENE
文件名       : MlsColorize.h
相关文件     : MlsColorize.cpp
文件实现功能 : 车载点云着色
作者         : 危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/04/07   1.0      危迟				   创建
</PRE>
*******************************************************************************/
#pragma once
#include "hdPointCloud.h"
#include "..\..\hdCommon\point_types.h"
#include "point_cloud.h"
#include <vector>

using namespace std;

namespace hd
{
	typedef struct CommonTime
	{
		short	hour;
		short	minute;
		short	second;
		short	milliSecond;

		CommonTime()
		{
			hour = minute = second = milliSecond = 0;
		}

		CommonTime& operator=(const CommonTime& other)
		{
			hour = other.hour;
			minute = other.minute;
			second = other.second;
			milliSecond = other.milliSecond;
			return *this;
		}
	}CommonTime;

	typedef struct POS
	{
		double x;
		double y;
		double z;
		double yaw;
		double pitch;
		double roll;

		POS()
		{
			x = y = z = yaw = pitch = roll = 0;
		}

		POS& operator=(const POS& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			yaw = other.yaw;
			pitch = other.pitch;
			roll = other.roll;
			return *this;
		}
	}POS;

	class HDPOINTCLOUD_API MlsColorize
	{
	public:
		MlsColorize(void);
		~MlsColorize(void);

	public:
		// 点云分割函数
		void SegPointCloudByPanoPos(void (*processCallback)(float,const char*));
		// 分块赋色
		void ColorizePointCloud(void (*processCallback)(float,const char*));
	};
}



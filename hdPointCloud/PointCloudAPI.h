#pragma once
#include "point_cloud.h"
using namespace hd;
extern "C"  
{
	// 根据hls文件路径创建点云指针
	HDPOINTCLOUD_API void* CreatePointCloudPtrByHls(const char* strFilePath);

	// 释放内存
	HDPOINTCLOUD_API void DestroyPointCloudPtr(void* pPcd);

	// 获取HLS点云扫描站的位置
	HDPOINTCLOUD_API void GetHlsHeader(const char* strFilePath,OUT float& CenterX,OUT float& CenterY,OUT float& CenterZ);

	// 获取HLZ点云扫描站的位置
	HDPOINTCLOUD_API void GetHlzHeader(const char* strFilePath,OUT double& CenterX,OUT double& CenterY,OUT double& CenterZ);
}


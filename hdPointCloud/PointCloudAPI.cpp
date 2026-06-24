#include "StdAfx.h"
#include "PointCloudAPI.h"
#include "..\hdHLSlib\HLSReadOpener.h"

void* CreatePointCloudPtrByHls( const char* strFilePath )
{
	PointCloud* pcd = new PointCloud;
	if (pcd->loadHlsFile(strFilePath))
	{
		return pcd;
	}
	
	return NULL;
}



void DestroyPointCloudPtr( void* pPcd )
{
	PointCloud* ptr = (PointCloud*)pPcd;
	if (ptr)
	{
		delete ptr;
		ptr = NULL;
	}
}

 // 获取HLS点云扫描站的位置
void GetHlsHeader(const char* strFilePath,OUT float& CenterX,OUT float& CenterY,OUT float& CenterZ)
{
	CHLSReadOpener hlsOpener;
	IHLSReader* hlsReader = hlsOpener.Open(strFilePath);
	HLSheader hlsHeader = hlsReader->GetHeader();
	CenterX = hlsHeader.offsetX;
	CenterY	= hlsHeader.offsetY;
	CenterZ = hlsHeader.offsetZ;
}

// 获取HLZ点云扫描站的位置
void GetHlzHeader(const char* strFilePath,OUT double& CenterX,OUT double& CenterY,OUT double& CenterZ)
{
	CHLSReadOpener hlsOpener;
	IHLSReader* hlsReader = hlsOpener.Open(strFilePath);
	HLZheader hlzReader = hlsReader->GetHlzHeader();
	CenterX = hlzReader.offsetX;
	CenterY	= hlzReader.offsetY;
	CenterZ = hlzReader.offsetZ;
}

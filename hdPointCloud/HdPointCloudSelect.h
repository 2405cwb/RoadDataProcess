#pragma once
#include "hdPointCloud.h"
#include "..\hdHlslib\IHLSReader.h"
#include "point_cloud.h"
#include <string>
#include "ogr_geometry.h"
#include "ogrsf_frmts.h"

namespace hd
{

	class HDPOINTCLOUD_API PointCloudSelected
	{
	public:
		PointCloudSelected(void);
		~PointCloudSelected(void);

	public:		
		//选中矢量所有要素缓冲区并集范围内的点云
		void SelectPtByUnionVector(std::string vectorPath, PointCloud* pcd, double dfDist);

		//选中指定要素缓冲区范围内的点云
		void SelectPtByVector(OGRFeature* pFeature, PointCloud* pcd, double dfDist);

	private:
		//选中缓冲区范围内的点云
		void SelectPoint(OGRGeometry* bufferGeo, PointCloud* pcd);

		//获取缓冲区并集
		OGRGeometry* UnionBuffer(std::string vectorPath, double dfDist);	
	};

}
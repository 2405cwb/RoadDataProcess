#pragma once
#include "hdPointCloud.h"
#include "..\hdCommon\point_types.h"
#include <vector>
#include "point_cloud.h"

using namespace std;
namespace hd
{
	class HDPOINTCLOUD_API CFitPlane
	{
	public:
		CFitPlane(void);
		~CFitPlane(void);

	public:
		bool getPlyPoints(vector<PointXYZ>& vecPts);
		vector<float> FitPlaneInit(vector<PointXYZ>& pts);
		vector<PointXYZ> FitPlane(vector<PointXYZ>& pts,vector<PointXYZ>& result,float eps1,int jt1);
	private:
		
		
	private:
//		vector<PointXYZ> m_deltas;              //均值坐标差
		float m_sigma;                          //去噪阈值标准偏差

	public:
		vector<float> m_ds;                     //存放d[i]数组
		float *m_deigenvalue;                   //平面坐标参数a、b、c、d ,ax + by + cz = d               
	    vector<PointXYZ> m_plyPts;              //去噪后点坐标

	};
}
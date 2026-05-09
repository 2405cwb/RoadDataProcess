#pragma once
#include "hdPointCloud.h"
#include "point_cloud.h"
#include "..\hdCommon\point_types.h"
#include <vector>

using namespace std;
namespace hd
{
	class HDPOINTCLOUD_API CFitCylinder
	{
	public:
		CFitCylinder(void);
		~CFitCylinder(void);
	public:
/*! @cylinderFit
********************************************************************************
<PRE>
函数名   : cylinderFit
功能     : 由点云拟合柱面（由于点云法向量方向原因，计算圆心时只用坐标点进行计算）
算法思路 ：由点云计算得到法向量，再聚类分析得到大圆数据，随机取法向量点获得最佳轴
		   线向量数据，用此法向量数据进行平面拟合去噪，得到最终轴线法向量，将坐标
		   点投影到过原点的平面上，坐标转换至二维，在此平面拟合计算最佳圆心和半径
		   再转换回原三维坐标系下。
参数     : 
           PointCloud:不含法向量的点云数据
		   loadCallback: 处理进度回调函数,第一个参数是百分比，第二消息
           
返回值   : 无
作者	 ：朱旭波
日期	 : 2012-12-25
*******************************************************************************/
		bool cylinderFit(PointCloud& ptCloud,void (*loadCallback)(float,const char*) = NULL);
	protected:
		vector<float> fitPlaneInit(vector<NormalPointXYZ>& inPts);
		vector<NormalPointXYZ> fitPlane(vector<NormalPointXYZ>& inPts,vector<NormalPointXYZ>& result,float eps1,int jt1);
	private:
		//float inline absSum(float a,float b){return abs(a - b);}
	public:
		float *m_posXYZRH;                      //圆柱面定位参数：定位点圆心（x,y,z）圆半径r，圆柱高h
		
	private:
		float *m_deigenvalue;                   //平面法向量
		float m_sigma;                          //去噪阈值标准偏差
		vector<float> m_ds;                     //存放d[i]数组
	};
}
/*! HdSearchSphere.h
********************************************************************************
<PRE>
模块名       : hdPointCloud
文件名       : HdSearchSphere.h
相关文件     : 
文件实现功能 : 自动查找地面测站点云中的靶球
作者         : 朱立雄
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人           修改内容   
2016/08/8    1.0      朱立雄            创建
</PRE>
*******************************************************************************/
#ifndef HDPOINTCLOUD_HDSEARCHSPHERE_H
#define HDPOINTCLOUD_HDSEARCHSPHERE_H

#include "hdPointCloud.h"
#include "..\hdCore\hdDefs.h"
#include "..\hdCore\hdVector3d.h"
#include "..\hdCore\hdVector2d.h"
#include "..\hdCommon\point_types.h"
#include <vector>

using namespace std;

namespace hd
{
	class PointCloud;

	// 靶球自动查找类
	class HDPOINTCLOUD_API CHdSearchSphere
	{
	public:
		CHdSearchSphere();
		virtual~ CHdSearchSphere();
	private:
		CHdSearchSphere(const CHdSearchSphere&);
		CHdSearchSphere& operator= (const CHdSearchSphere&);
		
	public:

		// 设置球的半径
		void SetSphereRadius(double dfRadius);

		// 指定点云，以及一个点的圈索引和在圈中的索引，搜索出该点附近的一个球
		//bool SearchSingleSphere(PointCloud* pPointCloud, int nLoopIndex, int nPointIndex, PointSphere& sphere);

		// 搜索出点云中的所有球
		bool SearchAllSpheres(PointCloud* pPointCloud, vector< PointSphere >& vSphere, ProcessCallbackFunc pProgressFunc = NULL);

	private:

		// 采用稳健的球面拟合方法，根据一个种子点在点集中找出一个固定半径的球
		bool SearchSphere(vector< CHdVector3df >& vPoint3d, CHdVector3df pointSeed, double dfRadius, PointSphere& sphere);

		// 根据输入的点集中的所有点来拟合出一个固定半径的球，且需传入球心的初始值
		bool FitFixedSphere(vector< CHdVector3df >& vPoint3d, double dfRadius, CHdVector3dd& sphereCenter);

		// 根据输入的点集中的所有点来拟合一个球
		bool FitSphere(vector< CHdVector3df >& vPoint3d, CHdVector3dd& sphereCenter, double& dfRadius);

		// 根据输入的点集中的所有点来拟合一个圆，同时计算点到圆的最大距离
		bool FitCircle(vector< CHdVector2df >& vPoint2d, double& dfCenterX, double& dfCenterY,
			double& dfRadius, double& dfMaxDis);

	private:

		double m_dfRadius;                   // 靶球半径

		double m_dfUpHeight;                 // 靶球相对于测站中心的最大高度
		double m_dfUpDistance;               // 靶球离测站中心的最大距离
		unsigned short m_nDownIntensity;     // 靶球上的点的最小强度（暂时不用）
	};
}

#endif
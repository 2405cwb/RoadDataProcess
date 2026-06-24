/*! computeNormal.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : computeNormal.h
相关文件     : 
文件实现功能 : 点云法向量计算
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/11/01   1.0      龚书林    
</PRE>
*******************************************************************************/
#ifndef _COMPUTE_NORMAL_H_
#define _COMPUTE_NORMAL_H_

#include "hdPointCloud.h"
#include "..\hdCommon\eigen.h"
#include "..\hdCommon\point_types.h"
#include <vector>
using namespace std;
using namespace hd;
/*! @computeNormal
********************************************************************************
<PRE>
函数名   : computePointNormal
功能     : 拟合平面
参数     : 
           pts :		点集合
           nx:	拟合平面的法向量
		   ny:	
           nz:
		   curvature: 拟合平面的曲率
返回值   : 无
作者	 ：龚书林
日期	 : 2012-11-11
*******************************************************************************/
HDPOINTCLOUD_API void computePointNormal ( const std::vector<Eigen::Vector3f> &pts,float &nx, float &ny, float &nz, float &curvature);

/*! @computeNormal
********************************************************************************
<PRE>
函数名   : computePointNormal
功能     : 根据视点改正法向量
参数     : 
           point :	目标点
		   vp_x:	视点
		   vp_y:
		   vp_z:
           nx:		需要改正的法向量
		   ny:	
           nz:
返回值   : 无
作者	 ：龚书林
日期	 : 2012-11-11
*******************************************************************************/
HDPOINTCLOUD_API void flipNormalTowardsViewpoint (const Eigen::Vector3f &point, float vp_x, float vp_y, float vp_z,
	float &nx, float &ny, float &nz);


/*! @computeNormal
********************************************************************************
<PRE>
函数名   : computePointNormal
功能     : 拟合平面
参数     : 
           pts :		点集合
           nx:	拟合平面的法向量
		   ny:	
           nz:
		   curvature: 拟合平面的曲率
返回值   : 无
作者	 ：龚书林
日期	 : 2013-10-11
*******************************************************************************/
HDPOINTCLOUD_API void computePointNormal ( const std::vector<PointXYZIPRGBA>& pts,float &nx, float &ny, float &nz, float &curvature);

/*! @computeNormal
********************************************************************************
<PRE>
函数名   : computePointNormal
功能     : 拟合平面
参数     : 
           pts :		点集合
           nx:	拟合平面的法向量
		   ny:	
           nz:
		   curvature: 拟合平面的曲率
返回值   : 无
作者	 ：龚书林
日期	 : 2013-10-11
*******************************************************************************/
HDPOINTCLOUD_API void computePointNormal ( const std::vector<PointXYZIPRGBA*>& pts,float &nx, float &ny, float &nz, float &curvature);

#endif
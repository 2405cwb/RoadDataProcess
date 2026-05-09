/*! Filters.h
********************************************************************************
<PRE>
模块名       : Filters
文件名       : Filters.h
相关文件     : 
文件实现功能 : 扫描过滤
作者         : 危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/07/27   1.0      危迟					创建
2013/9/14    1.01	  龚书林			  添加根据扫描头轨迹点高度过滤点云	
2013/11/23   1.1      危迟				  添加注释
</PRE>
*******************************************************************************/
#pragma once
#include "point_cloud.h"
#include "hdPointCloud.h"
#include "..\hdCommon\hdHdiStruct.h"

using namespace hd;

#define SECONDSPRELOOP	4.438	// Faro X330噪声每一圈计算大致花费的时间(ms)

namespace hd
{
	class HDPOINTCLOUD_API CFilters
	{
	public:
		CFilters(void);
		~CFilters(void);
	
	public: 
		//!离群过滤
		void OutlierFilter(
			PointCloud& ptCloud,							// 需要处理的点云
			bool select,									// 是否处理选中的点
			int index,										// 测站索引
			int neighbour,									// 邻域大小
			float disThreshold,								// 距离阈值
			float assignThreshold,							// 比例阈值
			void (*processCallback)(float,const char*));	// 进度回调
	
		//!离群过滤-Faro
		int OutlierFilterFaro(								// 需要处理的点云
			PointCloud& ptCloud,							// 是否处理选中的点
			bool select,									// 测站索引
			int neighbour,									// 邻域大小
			float disThreshold,								// 距离阈值
			float assignThreshold,							// 比例阈值
			void (*processCallback)(float,const char*));	// 进度回调

		//! 根据距离和离群过滤Faro-X330 [zhangfei 2014/5/27]
		int OutlierAndDistFilterFaro(
			PointCloud& ptCloud,							// 点云
			string& linPath,								// Lin文件路径
			double dPosDistIngore,							// 距离POS中心一定距离的点不进行过滤
			int nFieldSize,									// 离群过滤临域大小
			double dOutlierDistThre,						// 离群过滤距离阈值
			int nOutlierRatio,								// 利群过滤比例
			int nProcPcdIndex,								// 处理第几个点云
			void (*processCallback)(float,const char*)		// 进度
			);
	
		//!离群过滤-PCL 基于统计 是否服从正态分布
		int OutlierFilterStatistic( 
			PointCloud& ptCloud,							// 需要处理的点云
			bool select,									// 是否处理选中的点
			int neighbour,									// 邻域大小
			double Threshold,								// 阈值
			void (*processCallback)(float,const char*));	// 进度回调
															
		//! iScan点云统计过滤，采用分段的方式去处理					 
		int OutlierFilterStatisticIScan(
			const char* hlsPath,							// hls文件路径
			int neighbour,									// 邻域大小
			double Threshold,								// 阈值
			void (*processCallback)(float,const char*));	// 进度回调
	
		//!距离过滤
		int DistanceFilter(
			PointCloud& ptCloud,							// 需要处理的点云
			bool select, 									// 是否处理选中的点
			float minDis,									// 最小距离阈值
			float maxDis,									// 最大距离阈值
			void (*aoprocessCallback)(float,const char*));	// 进度回调
	
		//!半径过滤-PCL 查询点的半径邻域中的点数是否满足条件
		int RadiusFilter(
			PointCloud& ptCloud,							// 需要处理的点云
			bool select, 									// 是否处理选中的点
			float radius,									// 邻域半径大小
			int NumThreshold,								// 数量阈值
			void (*processCallback)(float,const char*));	// 进度回调
	
		//!平滑过滤
		int SmoothFiler(
			PointCloud& ptCloud,							// 需要处理的点云
			bool select, 									// 是否处理选中的点
			int neighbour,									// 邻域大小
			float disThreshold,								// 距离阈值
			void (*processCallback)(float,const char*));	// 进度回调
	
		//! 角度过滤
		int AngleFilter(
			PointCloud& ptCloud,							// 需要处理的点云
			bool select,									// 是否处理选中的点
			string& linPath,								// Lin文件路径
			float angleThreld,								// 角度阈值
			bool bDelete, 									// 过滤完成是否直接删除
			void (*processCallback)(float,const char*));	// 进度回调
	
		int AngleFilter(
			PointCloud& ptCloud,							// 需要处理的点云
			float angleThreld,								// 角度阈值
			bool bDelete,									// 过滤完成是否直接删除
			void (*processCallback)(float,const char*));	// 进度回调
															
		int AngleFilter(									
			PointCloud& ptCloud,							// 需要处理的点云
			float angleThreld,								// 角度阈值
			void (*processCallback)(float,const char*));	// 进度回调
															
		int AngleFilter(
			PointCloud& ptCloud,							// 需要处理的点云
			string& linPath,								// lin文件路径
			float angleThreld,								// 角度阈值
			void (*processCallback)(float,const char*));	// 进度回调
	
		//! 根据扫描头轨迹点高度过滤点云,过滤后选中.rxs-2016/12/7
		int ScanPosFilter(
			PointCloud& ptCloud,							// 需要处理的点云,不能是分段加载的点云
			const string& linPath,							// 扫描头轨迹点文件
			double height,									// 离扫描头高度,正表示上方,负表示下方
			bool   bHigh,									// 是否过滤高于,true过滤高于轨迹点,false过滤低于
			bool   bAddFilter,								// 是否叠加过滤
			void (*processCallback)(float,const char*));	// 进度回调

		//! 根据扫描头轨迹点高度过滤点云,过滤后选中.gsl-2013/9/14
		int ScanPosFilter(
			PointCloud& ptCloud,							// 需要处理的点云,不能是分段加载的点云
			const string& linPath,							// 扫描头轨迹点文件
			double height,									// 离扫描头高度,正表示上方,负表示下方
			bool   bHigh,									// 是否过滤高于,true过滤高于轨迹点,false过滤低于
			void (*processCallback)(float,const char*));	// 进度回调

		int FilterPtCloudByHight(
			PointCloud& ptCloud,							// 需要处理的点云,不能是分段加载的点云
			double height,									// 高度
			bool   bHigh,									// 是否过滤高于,true过滤高于height,false过滤低于height
			void (*processCallback)(float,const char*));	// 进度回调

		// 介于高度进行过滤
		int FilterPtCloudByHight(
			PointCloud& ptCloud,							// 需要处理的点云,不能是分段加载的点云
			double minheight,								// 高度下限
			double maxheight,								// 高度上限
			void (*processCallback)(float,const char*));	// 进度回调


		//! 根据扫描头轨迹点高度过滤点云,过滤后选中.gsl-2013/9/14
		int ScanPosFilter(
			PointCloud& ptCloud,							// 需要处理的点云,不能是分段加载的点云
			const string& linPath,							// 扫描头轨迹点文件
			double minHeight,								// 离扫描头底下
			double maxHeight,								// 离扫描头上方
			void (*processCallback)(float,const char*));	// 进度回调

		//! 根据扫描头轨迹点高度过滤点云,过滤后选中.rxs-2016/12/7
		int ScanPosFilter(
			PointCloud& ptCloud,							// 需要处理的点云,不能是分段加载的点云
			const string& linPath,							// 扫描头轨迹点文件
			double minHeight,								// 离扫描头底下
			double maxHeight,								// 离扫描头上方
			bool   bAddFilter,								// 是否叠加过滤
			void (*processCallback)(float,const char*));	// 进度回调


		//! 根据扫描头轨迹点距离过滤点云,过滤后选中.张飞-2014/6/17
		int ScanPosDistFilter(
			PointCloud& ptCloud,							// 需要处理的点云,不能是分段加载的点云
			const string& linPath,							// 扫描头轨迹点文件
			double distance,								// 离扫描头中心距离
			bool   bLess,									// 是否过滤近于，true过滤近于distance的点云，false过滤远于distance的点云
			void (*processCallback)(float,const char*));

		//! 根据扫描头轨迹点距离过滤点云,过滤后选中.rxs-2016/12/7
		int ScanPosDistFilter(
			PointCloud& ptCloud,							// 需要处理的点云,不能是分段加载的点云
			const string& linPath,							// 扫描头轨迹点文件
			double distance,								// 离扫描头中心距离
			bool   bLess,									// 是否过滤近于，true过滤近于distance的点云，false过滤远于distance的点云
			bool   bAddFilter,								// 是否叠加过滤
			void (*processCallback)(float,const char*));

		private:
			bool		LoadInLinFile(string& linPath);
			double		GetDistToPos(PointCloud& ptCloud,			// 点云
						PointXYZIPRGBA& pt,							// 需要计算距离的点
						int nRow);									// 改点所在的圈号，用于获得POS位置
			
			double		GetAngleToPos(PointCloud& ptCloud,
						PointXYZIPRGBA& pt,
						int nRow);

			// 分析部分圈点云中噪点 
			int X330NoiseAnalysis(PointCloud& ptCloud,		// 用于计算到POS的距离
				u32 nLoopSt,								// 部分圈起始的圈序号
				hdBlkArray<PointXYZIPRGBA>& ptBuf,			// 部分圈的点集
				double dPosDistIngore,						// 参数：距离POS中心一定距离的点不进行过滤
				int nFieldSize,								// 参数：离群过滤临域大小
				double dOutlierDistThre,					// 参数：离群过滤距离阈值
				int nOutlierRatio							// 参数：利群过滤比例
				);			

			// 根据X330圆盖形噪点的距离特点对噪点进行标记
			bool IsX330CricleNoise(PointXYZIPRGBA& pt,		// 当前点
				PointCloud& ptCloud,						// 点云
				u32 nLoop);									// 当前点所在的圈

		private:
			vector<HD_SCANHDIINFO> m_vecIScanPos;
	};

}
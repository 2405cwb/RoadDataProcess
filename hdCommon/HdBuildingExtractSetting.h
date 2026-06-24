/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：HdBulidingExtractSetting.h
文件实现功能：实现建筑物轮廓线提取参数配置
作者		：蔡红云
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2014/05/30	1.0			蔡红云		创建
</PRE>
******************************************************************************************************/
#pragma once
#pragma  warning(disable:4251)
#include "hdCommon.h"
#include <string>
#include <vector>
using namespace std;
namespace hd
{

	class HDCOMMON_API CHdBuildingExtractSetting
	{
	public:
		CHdBuildingExtractSetting(void):m_iAnalystLoop(16),m_fSearchRange(1.6f),m_dPlanar(0.7f),m_dnz(0.08f)
			,m_ilineNum(200),m_dsit_threshold(0.15f),m_dfitTol(0.5f),m_dsimpleTol(2.0f),m_dobjDisTol(2.5f),
			m_itolMinPtCount(10),m_fobjDx(3.0f),m_fobjDy(3.0f),m_fobjDz(3.0f)
		{

			// 默认配置文件路径
			m_strConfigPath =string(getCurrentDir())  + "HdBuildingExtractSetting.xml";
		}

		~CHdBuildingExtractSetting(void)
		{

		}

		// 读取配置文件HdBuildingExtractSetting.xml中的插件信息
		bool ReadHdBuildingExtractSetting();

		// 写入插件路径信息至配置文件HdBuildingExtractSetting.xml
		void WriteHdBuildingExtractSetting();


		// 获取建筑物点云提取参数
		// AnalystLoop 分析圈数  SearchRange 搜索范围 dPlanar 平面性阈值 nz 平面法向量阈值
		inline void  GetBuildingPcdFactor(int& AnalystLoop, float& SearchRange, double& dPlanar,double& nz)
		{
			AnalystLoop = m_iAnalystLoop;
			SearchRange = m_fSearchRange;
			dPlanar = m_dPlanar;
			nz = m_dnz;
		}

		// 设置建筑物点云提取参数
		// AnalystLoop 分析圈数  SearchRange 搜索范围 dPlanar 平面性阈值 nz 平面法向量阈值
		inline void SetBuildingPcdFactor(int AnalystLoop, float SearchRange, double dPlanar,double nz )
		{
			m_iAnalystLoop = AnalystLoop;
			m_fSearchRange = SearchRange;
			m_dPlanar = dPlanar;
			m_dnz = nz;
		}


		// 获取建筑物点云轮廓线模型提取参数
		// ilineNum线内点个数，dsit_threshold点到直线的距离
		inline void GetBuildingLineFactor(int& ilineNum, double& dsit_threshold)
		{
			ilineNum = m_ilineNum;
			dsit_threshold = m_dsit_threshold;
		}

		// 设置建筑物点云轮廓线模型提取参数
		// ilineNum线内点个数，dsit_threshold点到直线的距离
		inline void  SetBuildingLineFactor(int ilineNum, double dsit_threshold)
		{
			m_ilineNum = ilineNum;
			m_dsit_threshold = dsit_threshold ;
		}

		// 获取建筑物面片对象提取参数
		// dfitTol  建筑物边线拟合节点阈值  dsimpleTol  建筑物简化对象阈值  dobjDisTol 面片对象间隔阈值, objdist 面片尺寸数组
		inline void GetBuildingObjFactor( double& dfitTol,  double& dsimpleTol, double& dobjDisTol,   int& itolMinPtCount, float objdist[3])
		{
			dfitTol = m_dfitTol;
			dsimpleTol = m_dsimpleTol;
			dobjDisTol = m_dobjDisTol;
			itolMinPtCount = m_itolMinPtCount;
			objdist[0] = m_fobjDx;
			objdist[1] = m_fobjDy;
			objdist[2] = m_fobjDz;
		}

		// 设置建筑物面片对象提取参数
		// dfitTol  建筑物边线拟合节点阈值  dsimpleTol  建筑物简化对象阈值  dobjDisTol 面片对象间隔阈值, objdist 面片尺寸数组
		inline void SetBuildingObjFactor( double dfitTol,  double dsimpleTol, double dobjDisTol,   int itolMinPtCount, float objdist[3])
		{
			m_dfitTol = dfitTol;
			m_dsimpleTol = dsimpleTol;
			m_dobjDisTol = dobjDisTol;
			m_itolMinPtCount = itolMinPtCount;
			m_fobjDx  = objdist[0];
			m_fobjDy = objdist[1];
			m_fobjDz = objdist[2];
		}


		// 获取配置文件路径
		inline string GetConfigPath() 
		{

			return m_strConfigPath;
		}

		// 设置配置文件路径
		inline void SetConfigPath( string path)
		{
			m_strConfigPath = path;
		}

	private:

		// 分析圈数
		int m_iAnalystLoop;

		// 临近点搜索范围
		float m_fSearchRange;

		// 平面性阈值
		double m_dPlanar;

		// 平面Z方向法向量阈值
		double m_dnz;

		// 直线内点个数
		int m_ilineNum;

		// 点到直线距离阈值
		double m_dsit_threshold;

		// 建筑物边线拟合节点阈值
		double m_dfitTol;

		// 建筑物轮廓线简化阈值
		double m_dsimpleTol;

		// 面片对象间隔阈值
		double m_dobjDisTol;

		// 面片对象包含的最小点数
		int m_itolMinPtCount;

		//面片尺寸x阈值 
		float m_fobjDx;

		// 面片尺寸y阈值
		float m_fobjDy;

		// 面片尺寸z阈值
		float m_fobjDz;

		// 参数配置文件路径
		string m_strConfigPath;
	};
}


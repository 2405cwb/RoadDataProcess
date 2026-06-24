#pragma once
#pragma  warning(disable:4251)
#include "hdPointCloud.h"
#include "point_cloud.h"
#include "..\hdCore\hdDefs.h"
#include "..\hdCore\hdBox3d.h"
#include "..\hdCommon\point_types.h"
#include "..\hdHLSlib\HL2Reader.h"
#include "..\hdCore\hdArray.h"
#include "..\hdCommon\BursaWolfModel.h"
#include <vector>
#include <map>
#include "Eigen/Core"
#include "Eigen/Geometry"

using namespace std;

namespace hd
{

class HDPOINTCLOUD_API CHdPtCloud:public PointCloud
{
	friend class hd::fm::CHdPointSelect;
	friend class hd::fm::CHdPickCentriod;
	friend class hd::scene::CHdPlanarView;
public:
	//const ScreenTrans &m_screenTrans;
	// 点列数组
	//std::vector<PointXYZI*> m_mapData;
	// 读取对象
	CHL2Reader m_hlsReader;
private:
	// 细部加载限制
	u32 m_detailSimple;
	// 已经加载点云数量
	u64 m_count;
	//! 查询结果扫描列序号
	std::vector<u32> m_queryResult;
	//! 每圈的全局下标,第0圈下标是0,第1圈下标是0圈个数,第n圈是前n-1圈点数总和
	std::vector<u64> m_vecLoopSuf;
	//! 读取游标
	u32  m_curRoop;
	//! 当前圈
	PointXYZIPRGBA* m_pCurLoopBuf;
private:
	//! 禁止拷贝
	CHdPtCloud(const CHdPtCloud &pcd)
	{
	}
	//! 禁止复制
	const CHdPtCloud& operator=(const CHdPtCloud& other)
	{
		return *this;
	}
	//! 根据点序号获取所在圈数据
	inline void GetLoopByIndex(u64 index) 
	{
		if (!(index >= (*(m_vecLoopSuf._Myfirst + m_curRoop)) &&
			index < (*(m_vecLoopSuf._Myfirst + m_curRoop + 1))))		
		{
			// 先判断是否落入下一个扫描圈,如果外部顺序读取,则肯定在下一个扫描圈
			if (m_curRoop < m_vecLoopSuf.size() - 2 && 
				index >= (*(m_vecLoopSuf._Myfirst + m_curRoop + 1)) &&
				index < (*(m_vecLoopSuf._Myfirst + m_curRoop + 2)))
			{
				m_curRoop++;
			}
			else
			{
				// 二分法查找落入哪个扫描圈
				u32 loopCount = (m_vecLoopSuf._Mylast - m_vecLoopSuf._Myfirst);
				u32 low = 0,high = loopCount -1,middle;
				while(low <= high)
				{
					middle = (low + high)/2;
					if (*(m_vecLoopSuf._Myfirst  + middle) > index)
					{
						high = middle -1;
					}
					else if(*(m_vecLoopSuf._Myfirst  + middle) < index)
					{
						low = middle + 1;
					}
					else
					{
						// 正好相等情况
						low = middle + 1;
						break;
					}
				}

				m_curRoop = low -1;	
			}
			// 设置当前圈buffer为空,后续需要再读取
			m_pCurLoopBuf = NULL;
		}

		if(m_pCurLoopBuf == NULL)
		{
			u32 count = 0;
			u32 loopIndex = *(m_queryResult._Myfirst + m_curRoop);	 
			m_hlsReader.ReadLoop(m_pCurLoopBuf,count,loopIndex,false);
		}
	}
public:
	CHdPtCloud(void);
	~CHdPtCloud(void);

	inline PointXYZIPRGBA& operator[](u64 index)
	{ 
		GetLoopByIndex(index);
		return *(m_pCurLoopBuf + index - (*(m_vecLoopSuf._Myfirst + m_curRoop)));
	}
	
	/*inline const PointXYZI& operator[](u64 index)
	{ 
		GetLoopByIndex(index);
		return *(m_pCurLoopBuf + index - (*(m_vecLoopSuf._Myfirst + m_curRoop)));
	}*/

	inline virtual u64 count() const {return m_count;}

	inline BOOL IsSimpleLoad(){return m_simpleLevel != 1;}
	inline u32  GetSimpleLevel(){return m_simpleLevel;}

	//! 打开点云文件
	BOOL Open(const char* hlsFile);
	//! 将hls点云一次性抽稀记录到内存
	BOOL LoadHlsFile(void (*loadCallback)(float,const char*) = NULL);
	//! 根据视口查询加载数据
	inline BOOL LoadByViewPort(
		BOOL (*ViewTrans)(const f32&,const f32&,const f32&,s32&,s32&),		// 三维坐标转屏幕坐标回调函数
		s32 srcWidth,									// 视口宽度
		s32 srcHeight,									// 视口高度
		const CHdBox3df& box,							// 视锥体外边框
		const bool isOrthogonal,						// 是否为正射投影
		HRGN srcRgn = NULL);							// 屏幕选择范围,用于用户拉框查询											
	//! 加载圈数据
	//inline BOOL LoadLoop(int iLoop);

	//! 查询有多少个扫描列或者块在范围内,返回列数或块数
	inline u32 QueryByExtent(f32 xmin,f32 ymin,f32 zmin,f32 xmax,f32 ymax,f32 zmax);
	//! 查询所有列或者块,返回列数或块数
	inline u32 QueryAll();
	//! 重置查询游标到起始位置
	inline void ResetRead();
	//! 读取下一个扫描列,返回的点数组需要做二次判断是否在范围内.返回是否成功
	inline BOOL ReadNext(
		PointXYZIPRGBA*& pPtBuf,			// 返回的点数组指针,外部定义空的PointXYZI指针传入即可
		u32& ptCount);	 			// 返回点个数
	//! 结束读取
	inline void EndRead();
	//! 得到当前圈范围
	inline void GetExtent(f32& xmin,f32& ymin,f32& zmin,f32& xmax,f32& ymax,f32& zmax);
	//inline HLS2_LOOPINDEX* GetLoopIndex();
	inline void SelectPoint(HRGN hRgn,
							s32 srcWidth,									// 视口宽度
							s32 srcHeight,									// 视口高度
							BOOL (*ViewTrans)(const f32&,const f32&,const f32&,s32&,s32&));		// 三维坐标转屏幕坐标回调函数);
private:
	// 判断当前列是否在视窗内
	inline BOOL IsLoopInView(
		PointXYZIPRGBA* ptBuf,								// 点数组
		u32		   ptCount,								// 总点数
		BOOL (*ViewTrans)(const f32&,const f32&,const f32&,s32&,s32&),		// 三维坐标转屏幕坐标回调函数
		s32 srcWidth,									// 视口宽度
		s32 srcHeight,									// 视口高度
		u32 uStart,										// 起始点号
		u32 uStep,										// 采样判断步长
		HRGN srcRgn = NULL);										
};


}
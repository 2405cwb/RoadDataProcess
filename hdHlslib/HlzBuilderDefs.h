/*HlzBuilderDefs.h
********************************************************************************

文件实现功能 : 定义创建HLZ文件时用到的结构

*******************************************************************************/
#pragma once
#include <vector>

 namespace hd
 {
#define MEMORYBUF_SIZE 32000
#define SIZE_OF_BUFFER 20
#define MAX_THREAD_COUNT 4

// 块集子单元允许的最大点数
// 将点数过多的块集划分成小单元进行读取，防止内存不足
// 2500W 刘仙雄 2015/07/22
#define MAX_POINT_NUM 40000000

	 // 分块内存点云
	 struct PointXYZIPRGBA;
	 struct PointXYZIPRGBA_Short;
	 struct SplitMemoryBuf
	 {
		 int						  m_count;
		 std::vector<PointXYZIPRGBA*> m_vecBuf;
		 SplitMemoryBuf()
			 :m_count(0)
		 {
			 m_vecBuf.resize(MEMORYBUF_SIZE,0);
		 }
		 // 清空内存
		 void clear()
		 {
			 m_vecBuf.clear();
			 m_vecBuf.swap(std::vector<PointXYZIPRGBA*>());
		 }

	 };

	 template <class T>
	 struct DivideMemoryBuf
	 {
		 int						        m_count;
		 std::vector<T*>           m_vecBuf;
		 DivideMemoryBuf<T>()
			 :m_count(0)
		 {
			 m_vecBuf.resize(MEMORYBUF_SIZE);
		 }
		 // 清空内存
		 void clear()
		 {
			 m_vecBuf.clear();
			 m_vecBuf.swap(std::vector<T*>());
		 }
	 };

	 struct EntityIndex
	 {
		 I32 xNo; // 块集x方向全局编号
		 I32 yNo; // 块集y方向全局编号
		 I32 zNo; // 块集z方向全局编号 

		 EntityIndex():xNo(0),yNo(0),zNo(0)
		 {}

		 EntityIndex(I32 xNo, I32 yNo, I32 zNo):xNo(xNo),yNo(yNo),zNo(zNo)
		 {}

		 bool operator < (const EntityIndex& other) const
		 {
			 if(zNo != other.zNo)
			 {
				 return zNo < other.zNo;
			 }

			 if(yNo != other.yNo)
			 {
				 return yNo < other.yNo;
			 }

			 return xNo < other.xNo;
		 }
	 };

	 // 坐标强度统计结构体
	 struct CoordStatInfo
	 {
		 U32 xStepStat[1000];
		 U32 yStepStat[1000];
		 U32 zStepStat[1000];
		 U32 intStepStat[1000];
		 F32 xRcp;
		 F32 yRcp;
		 F32 zRcp;
		 F32 iRcp;

		 CoordStatInfo()
			 :xRcp(0.0f),yRcp(0.0f),zRcp(0.0f),iRcp(0.0f)
		 {	
			 Clear();
		 }

		 void Clear()
		 {
			 memset(xStepStat,0,sizeof(U32)*1000);
			 memset(yStepStat,0,sizeof(U32)*1000);
			 memset(zStepStat,0,sizeof(U32)*1000);
			 memset(intStepStat,0,sizeof(U32)*1000);
		 }
	 };
 }
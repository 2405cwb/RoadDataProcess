/*! @HlzCloudBuilder
********************************************************************************
<PRE>
模块名       : HdHlsLib
文件名       : HlzCloudBuilder.h
相关文件     : 
文件实现功能 : HlZ进行大数存储 
作者         : 熊勇钢
版本         : VER 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2015/10/15   1.0      熊勇钢	             创建
2015/11/01   1.0      熊勇钢	             分带存储
</PRE>
*******************************************************************************/


#pragma once
#include <vector>
#include <string>
#include <list>
#include <map>
#include <set>
#include "HlzDefs.h"
//#include "..\hdCore\hdString.h"
#include "..\hdCore\hdBox3d.h"
#include "HdLevel.h"
#include "..\hdCommon\point_types.h"
#include "HLZWriter.h"
#include "HlzBuilderDefs.h"

using namespace std;
using namespace hd;

namespace hd
{
	// 前置声明
	class IHLSReader;
	class CHdCoreData;
	class CHLZWriter;


class HLS_API CHlzCloudBuilder
{
	//! 私有变量
private:
	// hlz写入对象CHLZWriter
	CHLZWriter* m_pHlzWrite;
	// 区域外点云写入
    CHLZWriter* m_pNextHlzWrite;
	// 块集临时文件数组
	std::map<U32, BlockSetFileInfo> m_BlocksetFiles; 
	// 对应下一带区块集临时文件数组
	std::map<U32, BlockSetFileInfo> m_BlocksetFilesNext; 
	//块集数据临时存储容器
	std::vector<PointXYZIPRGBA> m_blsDataBuf;

	// 块数据存储数组，每个键值对表示一个块
	// 键代表块的索引，值为存储块里的点指针的vector容器
	std::map<string , vector<PointXYZIPRGBA *> > m_pBlockData;

	// 包临时数据存储数组
	// 键表示包的索引，值为指向块集数据区的点指针，不进行内存分配
	std:: map<string , vector<PointXYZIPRGBA *> > m_ParcelData;

	// 第0层对象
	//HdLevel m_level0;

	//点云文件列表
	std::vector<hd::stringc> m_hlsFileList;	

	// 分层集合
	//std::set<CHdLevel*> m_LevelSet;

	// 当前HLS文件读取对象
	//IHLSReader* m_pHlsReader;

	// 分层分块管理对象
	CHdCoreData* m_pCoreData;

	// 划分3度带工程后对下一区分层分块管理对象
	CHdCoreData* m_pCoreDataNext;

	// 点云整体范围
	CHdBox3dd    m_fullExtent;

	// 分区点云包围盒
	CHdBox3dd m_fullExtentNext;
	// 切片区域范围
	CHdBox3dd    m_zoneSliceExtent;
	// 切割块集起始位置坐标
	F64 m_blockSetStartX;
	F64 m_blockSetStartY;

	// 工程切割出来的非本带块集起始位置坐标
	F64 m_blockSetStartXNext;
	F64 m_blockSetStartYNext;
	//索引文件路径
	hd::stringc	m_name;			

	// 所有临时文件保存目录
	hd::stringc	m_savePath;	

	//网格大小,X步长
	F32					m_stepX;	

	//网格大小,Y步长
	F32					m_stepY;			
	
	//! 坐标统计变量
	CoordStatInfo m_coordStats;

	//! 坐标分区统计变量
	CoordStatInfo m_coordStatsNext;

	// hlz头文件的点数，重新统计
	U64 m_ptNum;
	// hlz头文件的点数，重新统计-对应划分出去的新区
	U64 m_ptNumNext;

	// 投影带起点坐标X
	F64                m_zoneStartX;
	// 投影带起点坐标Y
	F64                m_zoneStartY;
	// 投影带终点坐标X
	F64                m_zoneEndX;
	// 投影带终点坐标X
	F64                m_zoneEndY;

	// 当前工程所处区域经线
	F64                m_dCenter;

	//! 标记转换是否对0级格网均匀抽稀，若抽稀，精度设置为0层精度m，对应子层精度m*2
	bool m_bResampleZero;

	//! 对格网均匀抽稀精度设置，若0层重采样抽稀，则该精度值为0层精度，否则，为1层精度
	double m_dGridPrecision;

	//! 私有成员函数
private:
		
	// 切割输入文件
	bool SplitInputFiles(const char* savePath);

	// 切割输入的跨区文件
	bool SplitInputMultiZoneFiles(const char* savePath);

	// 按标准格网切割Hls成为块集
	bool SplitBlockSetFiles(const char* savePath, hd::stringc& strHlsFile);

	// 按标准格网切割Hls成为块集
	bool SplitMultiZoneBlockSetFiles(const char* savePath, hd::stringc& strHlsFile);

	// 按标准格网切割Hls成为块集
	bool SplitBlockSetFiles_LN(const char* savePath, hd::stringc& strHlsFile,U8 iLevel);

	// 切割Las到块集文件
	bool SplitBlockSetLas(const char* savePath,hd::stringc& strLasFile);

	// 对块集进行切割成块,并且将切分结果保存在内存中
	//输入:待切分的块集
	bool SplitBlockset2Block_Buffer(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks);//

	// 对块集进行切割成块,并且将切分结果保存在内存中
	//输入:待切分的块集（相邻区域块集）
	bool SplitBlockset2Block_BufferNext(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks);//
	// 对内存中的块切割成包
	bool SplitBlock2Parcel_Buffer(
		const CHdBox3df&	blockBox,								// 输入块的空间范围
		const char* strBlockID,							    // 输入待切分的块文件的索引
		const char* strPacelID,								// 输入切分后的包保存的索引
		std::vector<ParcelFileInfo>& arrayParcels			// 输出切分后的包信息
		);	
	
	// 切割包数据,包成每个包的点云数量不超过64000
	// 对于包内点数大于64000，进行递归条用对包文件进行切割
	bool SplitParcel2Parcel_Buffer(
		const CHdBox3df&	blockBox,								// 输入块的空间范围
		const char* srcParcelID,							    // 输入待切分的包文件的索引
		const string destPacelID,								// 输入切分后的包保存的索引
		std::vector<ParcelFileInfo>& arrayParcels			// 输出切分后的包信息
		);	

	//! 从临时文件中读取一定数量的点到内存
	//U32 readPtsBuffer(FILE*& pFile, std::vector<PointXYZIPRGBA>& vecBuffer);

	//! 根据块集列表
	CHdBox3df GetBlockListBox(std::vector<BlockSetFileInfo>& bsFileList);
	//! 将第0层数据写入到文件
	void writeLevel0Data(CHdLevel* pLevel0);
	//! 将第0层数据写入到文件(相邻块集)
	void writeLevel0DataNext(CHdLevel* pLevel0);
	//! 根据上一层数据，将其下一层数据写入到文件,返回当前层的点数
	U64  writeNextLevelData(CHdLevel* pPreLevel, U8 nLevelNo, CHdLevel* curLevel);
	//! 根据上一层数据，将其下一层数据写入到文件,返回当前层的点数
	U64  writeNextLevelDataNextZone(CHdLevel* pPreLevel, U8 nLevelNo, CHdLevel* curLevel);
	//! 写入下一层数据
	U64  writeNextLevel(U8 nLevelNo, CHdLevel* curLevel);

	//! 根据当前层的块集编号确定上一层的块集
	void getPreLevelBlockset(
		U16 xNo,					// 输入当前块集的位置X
		U16 yNo,					// 输入当前块集的位置Y
		CHdLevel* pPreLevel,			// 输出上一层
		std::vector<BlockSetFileInfo>& preBlocksetFiles);	// 当前块集对应的上一层块集

	//! 根据当前层的块集编号确定上一层的块集(对应相临区域)
	void getPreLevelBlocksetNext(
		U16 xNo,					// 输入当前块集的位置X
		U16 yNo,					// 输入当前块集的位置Y
		CHdLevel* pPreLevel,			// 输出上一层
		std::vector<BlockSetFileInfo>& preBlocksetFiles);	// 当前块集对应的上一层块集
	//! 统计点云强度范围
	void StatIntensity(
		IHLSReader* pHlsReader,			// 输入的hls读取对象
		U16& min,						// 输出的最小强度
		U16& max);						// 输出的最大强度

	//! 根据点的坐标确定其所在的块位置
	// 将块集切分为八块，即XYZ方向各切一刀;编码从左到右,从下到上
	//    6--7
	//  4 --5|
	//  | |  |
	//	| 2--3
	//	|/  /
	//  0--1
	inline void getPtBlockIndex(
		PointXYZIPRGBA& pt,		// 输入的点，真实坐标
		//CHdVector3df& min,		// 输入块集的包围盒左下角
		const CHdVector3df& mid,		// 输入块集的包围盒中心点
		U8& index);				// 输出该点的块位置

	//! 获取块（或者包）的空间范围
	void getBlockBox(
		//const char* blockPath,			// 输入块的路径，从中获取块的序号
		const CHdVector3df& mid,				// 输入所属块集的范围中心
		const CHdVector3df& halfSize,			// 输出该块的空间范围
		CHdBox3df& box,					//输出该块的空间范围
		U16 index);

	//! 获取块集文件夹下的所有块文件
	//U32 getBlockFiles(const char* strBSPath, std::vector<hd::stringc>& vecBlockFiles);

	// 清空临时块集文件的内存空间
	void clearBlocksetData();

	// 清除包文件数据
	void clearParcelData();

	// 清空vector<SplitMemoryBuf>的内存
	void clearSplitMemBuf(vector<SplitMemoryBuf> & vecMemBuf);

	//计算HLS文件中的数据的最大值、最小值
	void calHlsExtnt(hd::IHLSReader* reader, double& x_min, double& x_max,double& y_min,double& y_max,double& z_min, double& z_max);

	//! 根据点云统计块集划分方法

	//! 根据总体

// 	// 一次性读取并划分整个块集
// 	void readBlkByAll();
// 
// 	// 分批次以子块集的形式划分整个块集
// 	void readBlksetBySub();

	// 根据块集划分块文件
	bool SplitBlockFiles(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks);
	// 根据块集划分块文件(相邻区域块集)
	bool SplitBlockFilesNext(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks);
	// 将块文件分割成子块
	bool SplitBlock2Parts(const CHdBox3df& blockBox, const string strBlockFile, std::vector<subBlockFileInfo>& subBlk);
	// 将块文件分割成子块(相邻区域子块)
	bool SplitBlock2PartsNext(const CHdBox3df& blockBox, const string strBlockFile, std::vector<subBlockFileInfo>& subBlk);
	// 子块文件分包
	bool SplitSbkFile2Parcel_Buffer(const CHdBox3df& blockBox, const string strSbkFile, std::vector<ParcelFileInfo>& vecParcelInfo);
	// 子块文件分包（相邻区域文件包）
	bool SplitSbkFile2Parcel_BufferNext(const CHdBox3df& blockBox, const string strSbkFile, std::vector<ParcelFileInfo>& vecParcelInfo);
public:
	CHlzCloudBuilder(void);
	~CHlzCloudBuilder(void);

	// 强度最小值
	U16				    m_iMin;
	
	//! 强度最大值
	U16					m_iMax;

	//! 关闭
	void Close();
	//! 进度统计对象
	void (*processCallback)(float,const char*);

	//! 打开hls1.0/hls2.0
	//IHLSReader* OpenHls(const char* strPath);

	//! 关闭当前hls1.0/hls2.0读取对象
	//void CloseHls();

	//! 添加一个文件hls或者xyz文件
	void addFile(const hd::stringc& hlsFile)
	{
		m_hlsFileList.push_back(hlsFile);
	}

	//! 添加一系列hls或者xyz文件
	void addFiles(const std::vector<hd::stringc>& hlsFiles)
	{
		for (unsigned int i = 0;i < hlsFiles.size();i++)
		{
			m_hlsFileList.push_back(hlsFiles[i]);
		}
	}

	//! 清空所有hls文件
	void clearFiles()
	{
		m_hlsFileList.clear();
	}

	//! 关闭所有文件  2013/11/20 蔡红云 
	void CloseTempFiles();

	//! 删除所有临时文件 2013/11/19 蔡红云 
	BOOL RemoveTempFiles(const char* path);

	//! 删除块集文件
	BOOL RemoveBSFiles(std::map<U32, BlockSetFileInfo>& bsFiles);

	//! 开始生成hlz文件
	bool buildHlz(const char* savePath, BOOL isCompress = FALSE);

	//! 开始生成hlz文件
	bool buildHlz(const char* savePath,const char* savePathNext, BOOL isCompress = FALSE, int nCenterLongitude = 0);

	//! 外部接口设置，是否抽稀零层及抽稀精度，若为fou，该抽稀精度为1层的抽稀精度1/2
	void SetResampleZero(bool bResample,double dGridPrecision);

	// 数据复制
	inline void VecCopy3fv(double v[3], const double a[3])
	{
		v[0] = a[0];
		v[1] = a[1];
		v[2] = a[2];
	}

	// 比较最大值最小值
	inline void VecUpdateMinMax3dv(double min[3], double max[3], const double v[3])
	{
		if (v[0]<min[0]) min[0]=v[0]; else if (v[0]>max[0]) max[0]=v[0];
		if (v[1]<min[1]) min[1]=v[1]; else if (v[1]>max[1]) max[1]=v[1];
		if (v[2]<min[2]) min[2]=v[2]; else if (v[2]>max[2]) max[2]=v[2];
	}

};

}

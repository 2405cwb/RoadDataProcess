/*! HLZWriter.h
********************************************************************************
<PRE>
模块名       : HLSLib
文件名       : HLZWriter.h
相关文件     : 
文件实现功能 : 压缩点云文件hlz写入
作者         : 张飞
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2015/02/04   1.0      张飞			  创建
</PRE>
*******************************************************************************/

#pragma once
#include "..\..\hdCommon\point_types2.h"
#include "HlzDefs.h"
#include ".\inc\mydefs.hpp"
#include "..\hdCore\hdString.h"

namespace hd
{
	class CHdParcelBase;
	enum EntityType    //空间实体类型(层/块集/块/包)  袁亮  20160805
	{
		ET_UNKNOWN = -1,
		ET_LEVEL,
		ET_BLOCKSET,
		ET_BLOCK,
		ET_PARCEL
	};

	struct SplitMsg  //空间实体分割请求  袁亮  20160805
	{
		U32             id;         //请求id
		CHdParcelBase*  ptEntity;   //空间实体管理对象指针
		EntityType      type;       //实体类型
		U64             offset;     //实体在缓冲区中起始位置
		U64             count;      //实体包含的点数目
		CHdBox3df       box;        //实体包围盒
	
		SplitMsg()
			:id(0), ptEntity(NULL), type(ET_UNKNOWN), offset(0), count(0)
		{
			box.MinEdge = CHdVector3df(F32_MAX, F32_MAX, F32_MAX);
			box.MaxEdge = CHdVector3df(F32_MIN, F32_MIN, F32_MIN);
		}
	};

	struct CacheInfo //实体缓存文件信息  袁亮  20160805
	{
		CHdParcelBase* ptEntity;  //空间实体管理对象指针
		EntityType     type;      //实体类型
		std::string    path;      //缓存文件路径
		U64            numPoint;  //实体包含的点数目
		CHdBox3df      box;       //实体包围盒
		CacheInfo()
			:ptEntity(NULL), type(ET_UNKNOWN), path(""), numPoint(0)
		{
			box.MinEdge = CHdVector3df(F32_MAX, F32_MAX, F32_MAX);
			box.MaxEdge = CHdVector3df(F32_MIN, F32_MIN, F32_MIN);
		}
	};

	// 分割的包文件具有的信息
	struct ParcelFileInfo
	{
		hd::stringc path;		// 文件路径
		U16 index;				// 文件序号
		CHdBox3df box;			// 包的空间范围
		U64 numPoint;			// 包内点数
		ParcelFileInfo()
			:numPoint(0),index(0xffff)
		{
			box.MinEdge = CHdVector3df(F32_MAX, F32_MAX, F32_MAX);
			box.MaxEdge = CHdVector3df(F32_MIN, F32_MIN, F32_MIN);
		}
	};

	// 分割的子块文件具有的信息
	struct subBlockFileInfo
	{
		hd::stringc path;		// 文件路径
		U16 index;				// 文件序号
		CHdBox3df box;			// 块的空间范围
		U64 numPoint;			// 块内点数
		subBlockFileInfo()
			:numPoint(0),index(0xffff)
		{
			box.MinEdge = CHdVector3df(F32_MAX, F32_MAX, F32_MAX);
			box.MaxEdge = CHdVector3df(F32_MIN, F32_MIN, F32_MIN);
		}
	};

	// 分割的块文件具有的信息
	struct BlockFileInfo
	{
		hd::stringc path;		// 文件路径
		U16 index;				// 文件序号
		CHdBox3df box;			// 块的空间范围
		U64 numPoint;			// 块内点数
		//vector<ParcelFileInfo> vecParcel; // 记录子包
		//bool bHasParcel;        // 标记是否有子包
		BlockFileInfo()
			:numPoint(0),index(0xffff)
		{
			box.MinEdge = CHdVector3df(F32_MAX, F32_MAX, F32_MAX);
			box.MaxEdge = CHdVector3df(F32_MIN, F32_MIN, F32_MIN);
			//bHasParcel = false;
		}
	};

	// 分割的块集文件具有的信息
	struct BlockSetFileInfo
	{
		std::string path;		// 文件路径
		CHdBox3df box;			// 块集包围盒
		U64 numPoint;           // 块集内的点数  新增by刘仙雄  2015/07/22
		//vector<BlockFileInfo> vecBlock; // 子block集
		//bool bHasBlock;         // 标记包含子block
		BlockSetFileInfo()
			:numPoint(0)
		{
			box.MinEdge = CHdVector3df(F32_MAX, F32_MAX, F32_MAX);
			box.MaxEdge = CHdVector3df(F32_MIN, F32_MIN, F32_MIN);
			//bHasBlock = false;
		}

	};

	// 定义结构体
	struct PosKey 
	{
		U64 gridIndex;
		int nIndex;
		double dDist;
		PosKey()
		{
			gridIndex = 0;
			nIndex = 0;
			dDist = 0.0;
		}
	};

	bool SortByGrid(PosKey& pt0,PosKey& pt1);

	class CHdLevel;

class HLS_API CHLZWriter
{
private:
	char  m_dir[256];		// 数据所在路径
	char  m_name[256];		// 数据文件名称,不包含扩展名

	FILE*  m_pIndexFile;		// 索引文件指针
	HANDLE m_pDataFile;		   // 当前数据文件指针,大于2GB文件操作需要用windows读写函数
	HANDLE m_pColorFile;		// 当前颜色文件指针
	HANDLE m_pPropFile;		// 当前分类文件指针

	FILE* m_pLogFile;		// log文件指针

	// 增加写入只一圈点云而不对头文件做更新判断
	bool m_bOnlyUndateData;

	U16 m_curFileIndex;		// 当前数据文件序号

	// 文件标识,0NTFS,1FAT32,2FAT
	U8  m_fileFlag;	
	
	// 当前数据位置
	U64 m_curDataAddr;

	// 设置0层重采样精度（空间格网精度）
	double m_dGridPrecision;

	HANDLE m_fileMap;    //文件映射句柄
    U32  m_nWrittenLen;  //当前hld文件已经写入的长度
	char* m_pWriteAddr;  //映射指针指向hld尚未写入数据首部
	char* m_pMapBegAddr; 
	char* m_pMapEndAddr; //当前文件映射指针结尾位置

	bool  m_bNewHlz; //文件是否为hlz 3.2格式（3.1格式被污染，视为3.0格式）

private:
	//写hlz3.0格式索引信息
	BOOL WriteOldIndex(map<U16,CHdLevel*>& mLevel);
	//写hlz3.1格式索引信息
	BOOL WriteNewIndex(map<U16,CHdLevel*>& mLevel);

public:
	// 文件头信息
	HLZheader m_header;

	// 强度的最大最小值，暂时保存在这
	//U16 m_maxInt;
	//U16 m_minInt;

public:
	CHLZWriter(void);
	~CHLZWriter(void);

	//! 打开准备写入
	BOOL Open(const char* file_name,		// 指定保存路径
		bool bOnlyUpdata = false,			// 打开只更新头文件
		U32 io_buffer_size=65536,			// IO缓存大小
		bool bAppend = false);				// 是否追加数据写入				

	//! 关闭文件
	BOOL Close();

	//! 在索引文件中写入层索引信息
	BOOL WriteLevelInfo(HdLevel& level) ;

	//! 写入索引信息
	BOOL WriteIndex(map<U16,CHdLevel*>& mLevel);

	//! 写入索引信息,进行块集、块、包偏移后写入
	BOOL WriteIndex(map<U16,CHdLevel*>& mLevel,F32 offsetX,F32 offsetY);

	//! 写入块集索引和数据，针对块集中没有分块的情况
	//支持同步写入颜色数据信息   袁亮  20160625
	BOOL WriteBlockSetData(HdBlockset& blockSet, const HdPointXYZ* ptBuf, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count);

	//! 写入块集索引和数据，针对块集中没有分块的情况，其中xyz的坐标是压缩后的数据流
	//支持同步写入颜色数据信息   袁亮  20160625
	BOOL WriteBlockSetData(HdBlockset& blockSet, const U8* ptXYZBuf, const U32 ptXYZBufSize, const U8* intenBuf, const HdPtColor* ptColorBuf, U32 count);

	//! 写入块索引和数据, 针对该块中没有分包的情况
	//支持同步写入颜色数据信息   袁亮  20160625
	BOOL WriteBlockData(HdBlock& block, const HdPointXYZ* ptBuf, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count);
	
	//! 写入块索引和数据, 针对该块中没有分包的情况, 针对压缩的情况
	//支持同步写入颜色数据信息   袁亮  20160625
	BOOL WriteBlockData(HdBlock& block, const U8* ptXYZBuf, const U32 ptXYZBufSize, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count);

	//! 写入包索引和数据
	//支持同步写入颜色数据信息   袁亮  20160625
	BOOL WriteParcelData(HdParcel& parcel, const HdPointXYZ* ptBuf, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count);

	//! 写入包索引和数据，针对压缩的情况
	//支持同步写入颜色数据信息   袁亮  20160625
	BOOL WriteParcelData(HdParcel& parcel, const U8* ptXYZBuf, const U32 ptXYZBufSize, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count);

	//! 写入块集、块、包数据
	BOOL WriteData(
		const HdPointXYZ* ptBuf,	// 待写入的坐标数据
		const U8* intenBuf,			// 待写入的强度数据
		const HdPtColor* ptColorBuf, //待写入的颜色数据   袁亮  20160625
		U64 count,					// 点数
		U64& addrCoord,				// 返回的坐标地址
		U64& addrIntensity,         // 返回的强度地址
		U64& addrColor);		    // 返回的颜色地址     袁亮  20160625

	//效率不高，已经弃用
	//坐标、强度、颜色一次性写入，减少IO开销     袁亮   20160805
	/*BOOL WriteDataAtOnce(
		const HdPointXYZ* ptBuf,	// 待写入的坐标数据
		const U8* intenBuf,			// 待写入的强度数据
		const HdPtColor* ptColorBuf, //待写入的颜色数据   
		U64 count,					// 点数
		U64& addrCoord,				// 返回的坐标地址
		U64& addrIntensity,         // 返回的强度地址
		U64& addrColor);		    // 返回的颜色地址*/     

	//! 写入压缩后的块集、块、包数据
	BOOL WriteCmpData(
		const U8* ptXYZBuf,	        // 待写入的坐标数据(压缩后的数据）
		const U32 ptXYZBufSize,     // 待写入数据的大小
		const U8* intenBuf,			// 待写入的强度数据
		const HdPtColor* ptColorBuf, //待写入的颜色数据   袁亮  20160625
		U64 count,					// 点数
		U64& addrCoord,				// 返回的坐标地址
		U64& addrIntensity,		    // 返回的强度地址
		U64& addrColor);		    // 返回的颜色地址     袁亮  20160625

	//效率不高，已经弃用
	//坐标、强度、颜色一次性写入，减少IO开销     袁亮   20160805
	/*BOOL WriteCmpDataAtOnce(
		const U8* ptXYZBuf,	        // 待写入的坐标数据(压缩后的数据）
		const U32 ptXYZBufSize,     // 待写入数据的大小
		const U8* intenBuf,			// 待写入的强度数据
		const HdPtColor* ptColorBuf, //待写入的颜色数据   
		U64 count,					// 点数
		U64& addrCoord,				// 返回的坐标地址
		U64& addrIntensity,		    // 返回的强度地址
		U64& addrColor);		    // 返回的颜色地址*/     

	U64 WriteNextLevelBlocksetByMid(
		const char* savePath,
		std::vector<BlockSetFileInfo>& preBlocksetFiles,							
		HdBlockset& curBlockset,
		U8 curLevel);

	//! 根据上一层的四个块集确定当前层的一个块集，并相对块集原点偏移后写入-zxb
	U64 WriteNextLevelBlocksetByMidCloud(
		const char* savePath,
		std::vector<BlockSetFileInfo>& preBlocksetFiles,							
		HdBlockset& curBlockset,
		CHdLevel* curLevel,
		U32 curBlockSetNo,
		U8 nCurLevel);

	//U64 WriteNextLevelBlocksetByMid2(
	//	const char* savePath,
	//	std::vector<BlockSetFileInfo>& preBlocksetFiles,							
	//	HdBlockset& curBlockset,
	//	U8 curLevel);

	// 写入数据文件，返回为本次均匀抽稀后写入的总点数
	U64 WriteLevelBlocksetFileByMeanBox(
		double dStepPrecision,   // 为本次层级均匀抽稀格网精度
		const char* strFilePath, // 待读取的临时文件，为划分后的块集/块/包，可一次读取完成
		CHdBox3df extentBox,     // 临时文件对应的包围盒，减少二次统计时间
		HANDLE pBlckstDataFile,
		LARGE_INTEGER& li,
		LARGE_INTEGER& li1); // 待写入的下一层级块集临时文件句柄，内部直接写入

	//! 根据上一层的四个块集确定当前层的一个块集，并写入
	U64 WriteNextLevelBlockset(
		const char* savePath,
		std::vector<BlockSetFileInfo>& preBlocksetFiles,							
		HdBlockset& curBlockset);	

	//! 根据上一层的四个块集确定当前层的一个块集，并相对块集原点偏移后写入-xyg
	U64 WriteNextLevelBlockset(
		const char* savePath,
		std::vector<BlockSetFileInfo>& preBlocksetFiles,							
		HdBlockset& curBlockset,
		CHdLevel* curLevel,
		U32 curBlockSetNo);	
	//! 同步写文本日志文件
	void WriteLogFile(const char* logStr);

	//! 从临时文件中读取一定数量的点到内存
	U32 ReadPtsBuffer(HANDLE pFile, std::vector<PointXYZIPRGBA>& vecBuffer);

	//! 从临时文件中读取一定数量的点到内存，外部传入传出读取多少点数，vec由外部一次申请较大内存
	U32 ReadPtsBuffer(HANDLE pFile, std::vector<PointXYZIPRGBA>& vecBuffer,U64& readCount);

	//! 从临时文件中读取所有数据到内存中
	// 文件小于3G，可以用FILE来读写
	U32 ReadAllPts(FILE* &pFile, std::vector<PointXYZIPRGBA>& vecBuffer);

	//! 从临时文件中读取一定数量的点
	U32 ReadPtsBySize(HANDLE pFile, std::vector<PointXYZIPRGBA>& vecBuffer, U64 bufferSize);

	//! 更新写入文件头
	void WriteHeader();

	// 写入头文件
	void WriteHeader(const HLZheader& hlzHeader);

	// 设置均匀抽稀格网精度，为0层精度d，子层级n精度为d*pow(2,n),用于从上一层级抽稀到下一层级时的格网精度
	void SetGridPrecision(double dGridPrecision);
protected:
	//! 关闭文件之前检查文件大小是否为dwAllocationGranularity的整数倍
	void CloseDataFile(FILE* pFile);
};
}


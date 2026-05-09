//#pragma once
//#include "..\..\hdCommon\point_types2.h"
//#include "HlzDefs31.h"
//#include ".\inc\mydefs.hpp"
//#include "..\hdCore\hdString.h"
//#include "HLZWriter.h"
//
//namespace hd
//{
//	class CHdLevel31;
//	class CHdParcelBase;
//
//		// 分割的包文件具有的信息
//	struct ParcelFileInfo31
//	{
//		hd::stringc path;		// 文件路径
//		U16 index;				// 文件序号
//		CHdBox3di box;
//		U64 numPoint;			// 包内点数
//		ParcelFileInfo31()
//			:numPoint(0),index(0xffff)
//		{
//			box.MinEdge = CHdVector3di(I32_MAX, I32_MAX, I32_MAX);
//			box.MaxEdge = CHdVector3di(I32_MIN, I32_MIN, I32_MIN);
//		}
//	};
//
//	// 分割的子块文件具有的信息
//	struct subBlockFileInfo31
//	{
//		hd::stringc path;		// 文件路径
//		U16 index;				// 文件序号
//		CHdBox3di box;
//		U64 numPoint;			// 块内点数
//		subBlockFileInfo31()
//			:numPoint(0),index(0xffff)
//		{
//			box.MinEdge = CHdVector3di(I32_MAX, I32_MAX, I32_MAX);
//			box.MaxEdge = CHdVector3di(I32_MIN, I32_MIN, I32_MIN);
//		}
//	};
//
//	// 分割的块文件具有的信息
//	struct BlockFileInfo31
//	{
//		hd::stringc path;		// 文件路径
//		U16 index;				// 文件序号
//		CHdBox3di box;
//		U64 numPoint;			// 块内点数
//		BlockFileInfo31()
//			:numPoint(0),index(0xffff)
//		{
//			box.MinEdge = CHdVector3di(I32_MAX, I32_MAX, I32_MAX);
//			box.MaxEdge = CHdVector3di(I32_MIN, I32_MIN, I32_MIN);
//		}
//	};
//
//	// 分割的块集文件具有的信息
//	struct BlockSetFileInfo31
//	{
//		std::string path;		// 文件路径
//		CHdBox3di  box;
//		U64 numPoint;           // 块集内的点数  新增by刘仙雄  2015/07/22
//		BlockSetFileInfo31()
//			:numPoint(0)
//		{
//			// 所有块集的包围盒均为(0,0,0)~(65536, 65536, 65536)(便于计算所以取值为65536，实际应该为65535)
//			box.MinEdge = CHdVector3di(0, 0, 0);
//			box.MaxEdge = CHdVector3di(65536, 65536, 65536);
//		}
//
//	};
//
//	class HLS_API CHLZ31Writer 
//	{
//	public:
//		CHLZ31Writer(void);
//		~CHLZ31Writer(void);
//
//		//! 打开准备写入
//		BOOL Open(const char* file_name,		// 指定保存路径
//			bool bOnlyUpdata = false,			// 打开只更新头文件
//			U32 io_buffer_size=65536,			// IO缓存大小
//			bool bAppend = false);				// 是否追加数据写入				
//
//		//! 关闭文件
//		BOOL Close();
//
//		//! 更新写入文件头
//		void WriteHeader();
//
//		// 写入头文件
//		void WriteHeader(const HLZheader& hlzHeader);
//
//		//! 写入索引信息
//		BOOL WriteIndex(map<U16,CHdLevel31*>& mLevel);
//
//		//! 写入块集索引和数据，针对块集中没有分块的情况
//		//支持同步写入颜色数据信息   袁亮  20160625
//		BOOL WriteBlockSetData(HdBlockset31& blockSet,  const U8* ptBaseAttri,  U32 baseAttriLen, const HdPtColor* ptColorBuf, U64 count, U32 cmpLen = 0);
//
//		//! 写入块索引和数据, 针对该块中没有分包的情况
//		//支持同步写入颜色数据信息   袁亮  20160625
//		BOOL WriteBlockData(HdBlock31& block, const U8* ptBaseAttri,  U32 baseAttriLen, const HdPtColor* ptColorBuf, U64 count, U32 cmpLen = 0);
//
//		//! 写入包索引和数据
//		//支持同步写入颜色数据信息   袁亮  20160625
//		BOOL WriteParcelData(HdParcel31& parcel, const U8* ptBaseAttri, U32 baseAttriLen, const HdPtColor* ptColorBuf, U64 count, U32 cmpLen = 0);
//
//	private:
//		//! 写入块集、块、包数据
//		BOOL WriteData(
//			const U8* ptBaseAttri,          //待写入的分类数据
//			U32 baseAttriLen,
//			const HdPtColor* ptColorBuf, //待写入的颜色数据   袁亮  20160625
//			U64 count,					// 点数
//			U64& addrBaseAttri,			// 返回的必含属性数据地址（按照坐标、强度、颜色顺序存放）
//			U64& addrColor);       // 返回的颜色地址（可选属性）
//
//	private:
//		char  m_dir[256];		// 数据所在路径
//		char  m_name[256];		// 数据文件名称,不包含扩展名
//
//		FILE*  m_pIndexFile;		// 索引文件指针
//		HANDLE m_pDataFile;		   // 当前数据文件指针,大于2GB文件操作需要用windows读写函数
//		HANDLE m_pColorFile;		// 当前颜色文件指针
//		HANDLE m_pPropFile;		// 当前分类文件指针
//
//
//		// 增加写入只一圈点云而不对头文件做更新判断
//		bool m_bOnlyUndateData;
//
//		U16 m_curFileIndex;		// 当前数据文件序号
//
//		// 文件标识,0NTFS,1FAT32,2FAT
//		U8  m_fileFlag;	
//
//		// 当前数据位置
//		U64 m_curDataAddr;
//
//		HANDLE  m_fileMap;    //文件映射句柄
//		U32     m_nWrittenLen;  //当前hld文件已经写入的长度
//		char*   m_pWriteAddr;  //映射指针指向hld尚未写入数据首部
//		char*   m_pMapBegAddr; 
//		char*   m_pMapEndAddr; //当前文件映射指针结尾位置
//		DWORD m_dwAllocGran; //内存分配粒度
//
//	public:
//		// 文件头信息
//		HLZheader m_header;
//	};
//}
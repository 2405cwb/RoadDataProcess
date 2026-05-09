/*! HLSReadOpener.h
********************************************************************************
<PRE>
模块名       : hdHLSlib
文件名       : HLS2Reader.h
相关文件     : 
文件实现功能 : 海达数云点文件hls2.0读取
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/06/5   1.0      龚书林    
</PRE>
*******************************************************************************/

#pragma once
#include "..\hdHlslib\inc\hl2Definition.h"
#include "..\hdHlslib\inc\hlsDefinition.h"
#include "..\hdCommon\point_types.h"
#include "IHLSReader.h"
#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <istream>
#include <fstream>
using namespace std;
using namespace hd;

namespace hd
{
	class HLS_API CHLS2Reader
		:public IHLSReader
	{
	private:

		// 数据所在路径
		char  m_dir[256];
		// 数据文件名称,不包含扩展名
		char  m_name[256];
		// windows文件句柄
		HANDLE m_pDataFile;	
		// 内存映射句柄
		HANDLE m_fileMap;
		// 内存映射数据区起始位置
		char*  m_mapAddress;
		// 代码锁
		CRITICAL_SECTION m_cs;
		// 当前圈序号
		I32 m_loopIndex;
		// 当前数据文件序号
		U16 m_curFileIndex;	
		// 文件头7参数对应的坐标转换矩阵
		//double m_matrix[16];
		// 映射区段起始位置
		U64 m_mrStart;
		// 映射区段终止位置
		U64 m_mrEnd;
		// 上次读取的列
		hdVector<PointXYZIPRGBA> m_pPtBuf;
		
	public:
		CHLS2Reader(void);
		virtual ~CHLS2Reader(void);
		// 打开文件读取
		virtual BOOL Open(const char* path);
		// 获取HLS2_LOOPINDEX
		inline virtual CLoopIndex* GetLoopIndex(void (*loadCallback)(float,const char*) = NULL);
		// 读取一个点
		inline virtual BOOL Read_Point(PointXYZIPRGBA& pt,U64 index);
		// 读取一圈,将数据拷贝到目标数组
		inline virtual BOOL ReadLoop(
			hdVector<PointXYZIPRGBA>& ptBuf,	// 外部传入的数组,外部管理指针
			I32 loop,						// 圈号
			U32 simple = 1);			// 抽稀加载间隔		
		// 读取一圈,获取数据指针
		//inline virtual BOOL ReadLoop(
		//	PointXYZIPRGBA*& ptBuf,	// 外部传入的数组,外部管理指针	
		//	U32& count,				// 读取到的点数
		//	I32 loop);				// 圈号
		// 读取一圈,将数据拷贝到目标数组,包含无效点
		inline virtual BOOL ReadLoopFull(
			hdVector<PointXYZIPRGBA>& ptBuf,		// 外部传入的数组,外部管理指针
			I32 loop,							// 圈号
			U32 simple = 1);				// 抽稀加载间隔		

		// 更新一圈数据,到数据文件
		inline virtual BOOL UpdateLoop(
			const hdVector<PointXYZIPRGBA>& ptBuf,	// 需要更新的列
			I32 loop);									// 列序号

		// 关闭
		//BOOL virtual Close();
		// 关闭文件映射
		inline virtual void CloseFile();

		// 是否使用内存映射 用于调试 [2014/06/19 危迟]
		bool m_bUseMemMap;
	protected:
		//! 关闭内存映射
		inline void CloseMapdata();
		// 读取一圈的数据
		inline BOOL ReadBuffer(void*& pBuf,const HLS2_LOOPINDEX& loopIndex,bool bCopy = true);
		// 写入一圈数据 [2015/04/19 危迟]
		inline BOOL WriteBuffer(void*& pBuf,const HLS2_LOOPINDEX& loopIndex,bool bCopy = true);

		// 打开文件映射
		inline BOOL OpenFile(const char* filePath);
		
	};
}

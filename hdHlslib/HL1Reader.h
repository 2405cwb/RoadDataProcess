/*! HLSReadOpener.h
********************************************************************************
<PRE>
模块名       : hdHLSlib
文件名       : HL1Reader.h
相关文件     : 
文件实现功能 : 海达数云点文件hls1.0读取
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
#include "ihlsreader.h"

#include <vector>
using namespace std;

namespace hd
{
class HLS_API CHL1Reader :
	public IHLSReader
{
private:
	// 代码锁
	CRITICAL_SECTION m_cs;
	// windows文件句柄
	HANDLE m_hFile;
	// 一次读取的缓存
	char*  m_pPtsBuf;
	// 一次读取点个数
	U32    m_bufPtCount;
	// 当前读取点序号
	U32    m_ptIndex;
	// 文件大小
	U64    m_fileSize;
	
public:
	CHL1Reader(void);

	virtual ~CHL1Reader(void);
	// 打开文件读取
	virtual BOOL Open(const char* path);
	// 读取一个点
	virtual BOOL Read_Point( PointXYZIPRGBA& pt,U64 index );
	// 读取一圈,将数据拷贝到目标数组
	virtual BOOL ReadLoop(
		hdVector<PointXYZIPRGBA>& ptBuf,		// 外部传入的数组,外部管理指针
		I32 loop,								// 圈号
		U32 simple = 1);					// 抽稀加载间隔		

	// 读取一圈,将数据拷贝到目标数组,包含无效点
	virtual BOOL ReadLoopFull(
		hdVector<PointXYZIPRGBA>& ptBuf,		// 外部传入的数组,外部管理指针	
		I32 loop,								// 圈号
		U32 simple = 1);					// 抽稀加载间隔		

	// 更新一圈数据,到数据文件
	inline virtual BOOL UpdateLoop(
		const hdVector<PointXYZIPRGBA>& ptBuf,	// 需要更新的列
		I32 loop);									// 列序号

	// 关闭
	//virtual BOOL Close();
	// 获取HLS2_LOOPINDEX
	inline virtual CLoopIndex* GetLoopIndex(void (*loadCallback)(float,const char*) = NULL);
	// 获取列数
	inline virtual U32 GetLoopCount();
	//! 获取每一圈包含点数
	virtual U32 GetCountInLoop(){return m_bufPtCount;}
protected:
	virtual void CloseFile();
private:
	// 读取一块数据,返回点个数
	U32 ReadBlock(U64 index);
	// 扫描数据,计算索引
	//void CalculateIndex();
};

}
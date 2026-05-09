/*! CRsaWriter.h
********************************************************************************
<PRE>
模块名       : hdHLSLib
文件名       : CRsaWriter.h
相关文件     : 
文件实现功能 : 海达数云扫描点云写入
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/03/29   1.0      龚书林    

*******************************************************************************/

#pragma once
#include "RsaDefs.h"

class HLS_API CRsaWriter
{
private:
	// 文件写入指针
	FILE* m_pFile;
	// 已经写入点数
	U64   m_nCount;
public:
	CRsaWriter(void);
	virtual ~CRsaWriter(void);
	//! 打开准备写入
	BOOL Open(
		const char* path,			// 文件路径
		U32 io_buffer_size=65536);	// IO缓存大小,默认65536bytes
	//! 写入设备接收到的点缓存
	BOOL WriteBuf(
		const MDL_BUFHEADER& bufHeader,			//	点缓存头
		const MDL_SCANPOINT* pScanBuf);			//  点数组
						
	//! 关闭写入
	BOOL Close();
};


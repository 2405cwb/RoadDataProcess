#include "RsaWriter.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

CRsaWriter::CRsaWriter(void)
	:m_pFile(NULL),m_nCount(0)
{
}


CRsaWriter::~CRsaWriter(void)
{
	Close();
}

//! 打开准备写入
BOOL CRsaWriter::Open(
	const char* path,			// 文件路径
	U32 io_buffer_size)			// IO缓存大小,默认65536bytes
{
	// 将之前打开的关闭
	Close();
	m_pFile = fopen(path,"w+b");
	if (m_pFile == NULL)
	{
		return FALSE;
	}
	if (setvbuf(m_pFile, NULL, _IOFBF, io_buffer_size) != 0)
	{
		fprintf(stderr, "WARNING: setvbuf() failed with buffer size %u\n", io_buffer_size);
	}
	return TRUE;
}

//! 写入设备接收到的点缓存
BOOL CRsaWriter::WriteBuf(
	const MDL_BUFHEADER& bufHeader,			//	点缓存头
	const MDL_SCANPOINT* pScanBuf)			//  点数组
{
	if(m_pFile == NULL || bufHeader.ptCount == 0)
		return FALSE;
	fwrite(&bufHeader,sizeof(MDL_BUFHEADER),1,m_pFile);
	fwrite(pScanBuf,sizeof(MDL_SCANPOINT),bufHeader.ptCount,m_pFile);
	m_nCount += bufHeader.ptCount;
	return TRUE;
}

//! 关闭写入
BOOL CRsaWriter::Close()
{
	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
	m_nCount = 0;
	return TRUE;
}
#include "RsaReader.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

CRsaReader::CRsaReader(void)
	:m_pFile(NULL),m_nCount(0)
{
}


CRsaReader::~CRsaReader(void)
{
	Close();
}

//! 打开准备写入
BOOL CRsaReader::Open(
	const char* path,			// 文件路径
	U32 io_buffer_size)			// IO缓存大小,默认65536bytes
{
	Close();
	m_pFile = fopen(path,"rb");
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
//! 读取下一个缓存
BOOL CRsaReader::ReadBuf(
	MDL_BUFHEADER& bufHeader,			//	点缓存头信息
	MDL_SCANPOINT* pScanBuf)			//  点数组
{
	if(m_pFile == NULL || feof(m_pFile))
		return FALSE;
	size_t size = fread(&bufHeader,sizeof(MDL_BUFHEADER),1,m_pFile);
	if(size != 1)
		return FALSE;
	size = fread(pScanBuf,sizeof(MDL_SCANPOINT),bufHeader.ptCount,m_pFile);
	if(size != bufHeader.ptCount)
		return FALSE;
	m_nCount += bufHeader.ptCount;
	return TRUE;
}
//! 关闭写入
BOOL CRsaReader::Close()
{
	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
	m_nCount = 0;
	return TRUE;
}
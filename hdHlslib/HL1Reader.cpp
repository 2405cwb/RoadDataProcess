#include "HL1Reader.h"
#include <io.h>
#include "..\hdCommon\hdSceneStr.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

#define MAPDATA_SIZE (2<<24)	//32M
namespace hd
{

CHL1Reader::CHL1Reader(void)
	:IHLSReader(),m_hFile(NULL),m_pPtsBuf(NULL),m_bufPtCount(0),m_fileSize(0)
{
	::InitializeCriticalSectionAndSpinCount( &m_cs, 0x80000401 );
}


CHL1Reader::~CHL1Reader(void)
{
	//Close();
	
	m_header.clean();
	if (m_pPtsBuf)
	{
		free(m_pPtsBuf);
		m_pPtsBuf = NULL;
	}
	m_bufPtCount = 0;
	m_fileSize = 0;

	DeleteCriticalSection(&m_cs);
}

BOOL CHL1Reader::Open( const char* path )
{
	CloseFile();

	m_header.clean();
	if (m_pPtsBuf)
	{
		free(m_pPtsBuf);
		m_pPtsBuf = NULL;
	}
	m_bufPtCount = 0;
	m_fileSize = 0;

	if (path == 0 || _access(path,04) != 0)
	{
		return FALSE;
	}
	EnterCriticalSection(&m_cs);
	// 打开文件
	m_hFile = CreateFile(path, 
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, 
		NULL,
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		LeaveCriticalSection(&m_cs);
		return FALSE;
	}

	DWORD sizeHigh = 0;
	m_fileSize = ::GetFileSize(m_hFile,&sizeHigh);
	if (sizeHigh != 0)
	{
		m_fileSize = ((U64)sizeHigh << 32) | m_fileSize;
	}

	// 读取解析文件头
	char  tmp[256] = {0};
	DWORD numRead;
	if(!ReadFile(m_hFile,tmp,256,&numRead,NULL))
	{
		LeaveCriticalSection(&m_cs);
		return FALSE;
	}

	m_header.clean();
	m_header.prase(tmp);
	
	// 解决旧的数据没有7参数
	if (m_header.scale == 0.0)
	{
		m_header.scale = 1.0;
	}
	// 设置缓存点数,并申请缓存空间
	if(m_header.number_of_col * m_header.number_of_row == m_header.number_of_point_records
		&& m_header.number_of_col != 1 && m_header.number_of_row != 1)
	{
		m_bufPtCount = m_header.number_of_row;
	}
	else
	{
		m_bufPtCount = 10000;
	}
	m_pPtsBuf = (char*)malloc(m_bufPtCount * m_header.point_data_record_length);
	m_ptIndex = 0;
	if(!ReadBlock(m_ptIndex))
	{
		LeaveCriticalSection(&m_cs);
		return FALSE;
	}
		
	// 加载倾角仪数据
	char strAnglesFile[MAX_PATH] = {0};
	sprintf(strAnglesFile,"%s.angle",path);
	m_incFit.FitBySimpleData(strAnglesFile);
	// 计算转换坐标矩阵
	ComputeMatrix();
	
	LeaveCriticalSection(&m_cs);

	strcpy(m_filepath,path);
	return TRUE;
}

CLoopIndex* CHL1Reader::GetLoopIndex(void (*loadCallback)(float,const char*))
{
	// 计算索引,hls1.0需要程序计算获取
	if (m_pLoopIndex == NULL)
	{
		U32 blockCount = m_header.number_of_point_records / m_bufPtCount;
		m_pLoopIndex = IHLSReader::AllocLoopIndex(blockCount);// new CLoopIndex[blockCount];
		PointXYZIPRGBA pt;
		for (U32 i = 0;i<blockCount;i++)
		{
			if (loadCallback && (i % 100) == 0)
			{
				loadCallback(i / (float)blockCount, HDSCENE_IDS_CREATE_INDEX);
			}

			U32 count = ReadBlock(i * m_bufPtCount);
			CLoopIndex& loopIndex = m_pLoopIndex[i];
			loopIndex.m_loopIdx.offset = i * m_bufPtCount * m_header.point_data_record_length + m_header.header_size;
			loopIndex.m_loopIdx.fileNo = 0;
			loopIndex.m_loopIdx.rowCount = count;
			loopIndex.m_loopIdx.count = count;
			loopIndex.m_loopIdx.segCount = 1;
			for (U32 iPt = 0;iPt<count;iPt++)
			{
				memcpy(&pt,m_pPtsBuf + m_header.point_data_record_length * iPt, MIN(m_header.point_data_record_length,sizeof(PointXYZIPRGBA)));
				if (m_header.point_data_format == HLS_POINTFORMAT_RHVI)
				{
					if (pt.intensity > 0 && pt.x != 0.0f)
					{
						ComputeCoordate(pt);
					}
					else
					{
						pt.x = 0.0f;
						pt.y = 0.0f;
						pt.z = 0.0f;
						pt.intensity = 0;
					}
				}
				if (pt.isValid())
				{
					loopIndex.m_loopIdx.xmin = MIN(loopIndex.m_loopIdx.xmin,pt.x);
					loopIndex.m_loopIdx.ymin = MIN(loopIndex.m_loopIdx.ymin,pt.y);
					loopIndex.m_loopIdx.zmin = MIN(loopIndex.m_loopIdx.zmin,pt.z);

					loopIndex.m_loopIdx.xmax = MAX(loopIndex.m_loopIdx.xmax,pt.x);
					loopIndex.m_loopIdx.ymax = MAX(loopIndex.m_loopIdx.ymax,pt.y);
					loopIndex.m_loopIdx.zmax = MAX(loopIndex.m_loopIdx.zmax,pt.z);

					loopIndex.m_loopIdx.minIntensity = MIN(loopIndex.m_loopIdx.minIntensity,pt.intensity);
					loopIndex.m_loopIdx.maxIntensity = MAX(loopIndex.m_loopIdx.maxIntensity,pt.intensity);
				}
			}//for (U32 iPt = 0;iPt<count;iPt++)

			m_header.min_x = MIN(m_header.min_x,loopIndex.m_loopIdx.xmin);
			m_header.min_y = MIN(m_header.min_y,loopIndex.m_loopIdx.ymin);
			m_header.min_z = MIN(m_header.min_z,loopIndex.m_loopIdx.zmin);

			m_header.max_x = MAX(m_header.max_x,loopIndex.m_loopIdx.xmax);
			m_header.max_y = MAX(m_header.max_y,loopIndex.m_loopIdx.ymax);
			m_header.max_z = MAX(m_header.max_z,loopIndex.m_loopIdx.zmax);
		}//for (U32 i = 0;i<blockCount;i++)
	}
	// 返回索引
	return IHLSReader::GetLoopIndex();
}

// 获取列数
U32 CHL1Reader::GetLoopCount()
{
	U32 count = m_header.number_of_point_records / m_bufPtCount;
	return m_header.number_of_point_records % m_bufPtCount == 0?
		count : count + 1;
}

// 读取一块数据
U32 CHL1Reader::ReadBlock(U64 index)
{
	if(index >= m_header.number_of_point_records)
		return 0;
	m_ptIndex = (index / m_bufPtCount) * m_bufPtCount;
	U64 pos = m_ptIndex * m_header.point_data_record_length + m_header.header_size;
	DWORD numRead = 0;
	DWORD numSize;
	if (pos + m_bufPtCount * m_header.point_data_record_length <= m_fileSize)
	{
		numSize = m_bufPtCount * m_header.point_data_record_length;
	}
	else
	{
		numSize = m_fileSize - (pos);
	}
	SetFilePointer(m_hFile,pos,0,FILE_BEGIN);
	if(!ReadFile(m_hFile,m_pPtsBuf,numSize,&numRead,NULL) || numRead != numSize)
		return 0;
	return numSize / m_header.point_data_record_length;
}

BOOL CHL1Reader::Read_Point( PointXYZIPRGBA& pt,U64 index )
{
	EnterCriticalSection(&m_cs);
	if (index >= m_header.number_of_point_records)
	{
		LeaveCriticalSection(&m_cs);
		return FALSE;
	}
	// 判断读取的点是否在缓存块内
	if(index < m_ptIndex || index >= m_ptIndex + m_bufPtCount)
	{
		m_ptIndex = (index / m_bufPtCount) * m_bufPtCount;
		if(!ReadBlock(m_ptIndex))
		{
			LeaveCriticalSection(&m_cs);
			return FALSE;
		}
	}
	memcpy(&pt,m_pPtsBuf + m_header.point_data_record_length * (index - m_ptIndex),14);
	if (m_header.point_data_format == HLS_POINTFORMAT_XYZIRGB ||
		m_header.point_data_format == HLS_POINTFORMAT_XYZIRGBP)
	{
		memcpy(&pt.r,m_pPtsBuf + m_header.point_data_record_length * (index - m_ptIndex) + 16,1);
		memcpy(&pt.g,m_pPtsBuf + m_header.point_data_record_length * (index - m_ptIndex) + 18,1);
		memcpy(&pt.b,m_pPtsBuf + m_header.point_data_record_length * (index - m_ptIndex) + 20,1);
	}
	if (m_header.point_data_format == HLS_POINTFORMAT_RHVI)
	{
		ComputeCoordate(pt);
	}
	LeaveCriticalSection(&m_cs);
	return TRUE;
}

BOOL CHL1Reader::ReadLoop(hdVector<PointXYZIPRGBA>& ptBuf, I32 loop,U32 simple)
{
	EnterCriticalSection(&m_cs);
	if(loop < 0 || loop >= m_header.number_of_point_records/m_bufPtCount)
	{
		LeaveCriticalSection(&m_cs);
		return FALSE;
	}

	U32 count = ReadBlock(loop * m_bufPtCount);
	if(count <= 0)
	{
		ptBuf.clear();
		LeaveCriticalSection(&m_cs);
		return FALSE;
	}

	BOOL bRet = FALSE;
	try
	{	
		ptBuf.resize(count / simple);
		U32 ptIndex = 0;
		for (int i = 0;i < count;i+=simple)
		{		
			PointXYZIPRGBA& pt = *(ptBuf._Myfirst + ptIndex);
			memcpy(&pt,m_pPtsBuf + m_header.point_data_record_length * i,14);
			if (m_header.point_data_format == HLS_POINTFORMAT_XYZIRGB ||
				m_header.point_data_format == HLS_POINTFORMAT_XYZIRGBP)
			{
				memcpy(&pt.r,m_pPtsBuf + m_header.point_data_record_length * i + 16,1);
				memcpy(&pt.g,m_pPtsBuf + m_header.point_data_record_length * i + 18,1);
				memcpy(&pt.b,m_pPtsBuf + m_header.point_data_record_length * i + 20,1);
			}
			if (m_header.point_data_format == HLS_POINTFORMAT_RHVI)
			{
				// 存储的极坐标,需要计算直角坐标
				ComputeCoordate(pt);
			}
			if (pt.isValid())
			{
				ptIndex++;
			}
		}
		ptBuf.resize(ptIndex);
		bRet = TRUE;
	}
	catch (...)
	{
	}
	LeaveCriticalSection(&m_cs);
	return bRet;
}

BOOL CHL1Reader::ReadLoopFull( hdVector<PointXYZIPRGBA>& ptBuf, I32 loop,U32 simple )
{
	EnterCriticalSection(&m_cs);
	if (loop < 0 || loop >= m_header.number_of_point_records / m_bufPtCount)
	{
		LeaveCriticalSection(&m_cs);
		return FALSE;
	}
	
	U32 count = ReadBlock(loop * m_bufPtCount);
	if(count <= 0)
	{
		ptBuf.clear();
		LeaveCriticalSection(&m_cs);
		return FALSE;
	}
	BOOL bRet = FALSE;
	try
	{	
		ptBuf.resize(count/simple);
		int index = 0;
		for (int i = 0;i < count && index < ptBuf.size();i+=simple)
		{		
			PointXYZIPRGBA& pt = *(ptBuf._Myfirst + (index++));
			memcpy(&pt,m_pPtsBuf + m_header.point_data_record_length * i,14);
			if (m_header.point_data_format == HLS_POINTFORMAT_XYZIRGB ||
				m_header.point_data_format == HLS_POINTFORMAT_XYZIRGBP)
			{
				memcpy(&pt.r,m_pPtsBuf + m_header.point_data_record_length * i + 16,2);
				memcpy(&pt.g,m_pPtsBuf + m_header.point_data_record_length * i + 18,2);
				memcpy(&pt.b,m_pPtsBuf + m_header.point_data_record_length * i + 20,2);
			}
		}

		if (m_header.point_data_format == HLS_POINTFORMAT_RHVI)
		{
			ComputeCoordate(ptBuf);
		}
		bRet = TRUE;
	}
	catch (...)
	{
	}
	LeaveCriticalSection(&m_cs);
	return bRet;
}

BOOL CHL1Reader::UpdateLoop( 
	const hdVector<PointXYZIPRGBA>& ptBuf, /* 需要更新的列数据 */ 
	I32 loop )
{
	EnterCriticalSection(&m_cs);
	if (loop < 0 || loop >= m_header.number_of_point_records / m_bufPtCount 
		|| ptBuf.size() > m_header.number_of_point_records / m_bufPtCount  
		|| m_header.point_data_format == HLS_POINTFORMAT_RHVI
		|| m_header.point_data_format == HLS2_POINTFORMAT_RHVI)
	{
		LeaveCriticalSection(&m_cs);
		return FALSE;
	}
	U64 pos = m_ptIndex * m_header.point_data_record_length + m_header.header_size;
	DWORD numSize = m_header.point_data_record_length;
	DWORD numWrite = 0;
	for (int i = 0;i < ptBuf.size();i++)
	{
		PointXYZIPRGBA& pt = *(ptBuf._Myfirst + i);
		WriteFile(m_hFile,&pt,numSize,&numWrite,NULL);
	}
	LeaveCriticalSection(&m_cs);
	return TRUE;
}

//BOOL CHL1Reader::Close()
//{
//	
//
//	//IHLSReader::Close();
//	return TRUE;
//}

void CHL1Reader::CloseFile()
{
	EnterCriticalSection(&m_cs);
	
	if (m_hFile != NULL)
	{
		CloseHandle(m_hFile);
		m_hFile = NULL;
	}
	LeaveCriticalSection(&m_cs);
}

}
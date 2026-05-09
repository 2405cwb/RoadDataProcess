#include "HL2Reader.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{

	CHL2Reader::CHL2Reader(void)
		:m_pDataFile(NULL),m_fileMap(NULL),m_loopIndex(-1),
		m_pLoopIndex(NULL),m_mapAddress(NULL),m_curFileIndex(0xffff)
	{
		memset(m_dir,0,256);
		memset(m_name,0,256);

		::InitializeCriticalSectionAndSpinCount( &m_cs, 0x80000401 );
	}

	CHL2Reader::~CHL2Reader(void)
	{
		Close();

		DeleteCriticalSection(&m_cs);
	}

	void CHL2Reader::CloseMapdata()
	{
		if (m_fileMap)
		{
			if (m_mapAddress)
			{
				UnmapViewOfFile (m_mapAddress);
				m_mapAddress = 0;
			}
			CloseHandle (m_fileMap);
			m_fileMap = 0;
		}
	}

	BOOL CHL2Reader::Open( const char* path)
	{
		//// 对该接口加密
		//string strSoftName = "hdVector";
		//char strMsg[256] = {0};
		//if (!CheckLicense(strSoftName.c_str(), strMsg))
		//{
		//	::MessageBox(NULL,"请向武汉汉宁轨道交通技术有限公司申请加密狗！", "提示",MB_OK);
		//	return false;
		//}

		Close();
		// 解析路径得到m_dir,m_name
		char drive[256] = {0};
		char dir[256] = {0};
		char filename[256] = {0};
		char ext[256] = {0};

		_splitpath(path,drive,dir,filename,ext);
		_makepath(m_dir,drive,dir,NULL,NULL);
		strcpy(m_name,filename);

		// 打开索引文件
		FILE* pIndexFile = fopen(path,"rb");
		if (pIndexFile == NULL)
		{
			return FALSE;
		}
		// 读取文件头
		char  tmp[256] = {0};		
		fread(tmp,sizeof(m_header),1,pIndexFile);
		m_header.prase(tmp);

		if (m_header.file_signature[0] != 'H' ||
			m_header.file_signature[1] != 'L' ||
			m_header.file_signature[2] != 'S' ||
			m_header.file_signature[3] != '2')
		{
			fclose(pIndexFile);
			pIndexFile = NULL;
			return FALSE;
		}

		// 进行内存分配失败异常捕捉 [蔡红云2014/3/12]
		try
		{
    		if(m_header.number_of_row > 0)
			{
				m_pPtBuf.resize(m_header.number_of_row);
			}

			if (m_header.version_major == 2)
			{
				fseek(pIndexFile,256,SEEK_SET);
				// 读取索引
				m_pLoopIndex = new CLoopIndex[m_header.number_of_col];
				for (U32 i = 0;i<m_header.number_of_col;i++)
				{
					CLoopIndex& loopIdx = m_pLoopIndex[i];
					fread(&loopIdx.m_loopIdx,sizeof(HLS2_LOOPINDEX),1,pIndexFile);
					if (loopIdx.m_loopIdx.segCount > 0)
					{
						loopIdx.AllocLoopSeg();
						fread(loopIdx.m_pLoopSeg,sizeof(HLS2_LOOPSEG),loopIdx.m_loopIdx.segCount,pIndexFile);
					}
				}

				fclose(pIndexFile);
				pIndexFile = NULL;
			}
		}

		catch (...)
		{
			return FALSE;
		}

		// 计算转换坐标矩阵
		computeMatrix();

		return TRUE;
	}

	BOOL CHL2Reader::OpenFile(const char* filePath)
	{
		CloseFile();

		EnterCriticalSection(&m_cs);
		m_pDataFile = CreateFile(filePath, 
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ|FILE_SHARE_WRITE, 
			NULL,
			OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, 
			NULL);

		if (m_pDataFile == INVALID_HANDLE_VALUE)
		{
			LeaveCriticalSection(&m_cs);
			fprintf(stderr, "ERROR: cannot open file '%s'\n", filePath);
			return FALSE;
		}

		DWORD sizeHigh = 0;
		U64 fileSize = ::GetFileSize(m_pDataFile,&sizeHigh);
		if (sizeHigh != 0)
		{
			fileSize = ((U64)sizeHigh << 32) | fileSize;
		}

		m_fileMap = ::CreateFileMapping(m_pDataFile, NULL, PAGE_READWRITE, 0, 0, NULL);//PAGE_READONLY
		if (m_fileMap == NULL)
		{
			LeaveCriticalSection(&m_cs);
			fprintf(stderr, "ERROR: cannot create file mapping\n");
			return FALSE;
		}

		m_mapAddress = (char*)(::MapViewOfFile (m_fileMap,FILE_MAP_ALL_ACCESS, 0,0,0));// FILE_MAP_WRITE | FILE_MAP_READ

		LeaveCriticalSection(&m_cs);
		return m_mapAddress != NULL;		
	}

	BOOL CHL2Reader::Read_Point( PointXYZIPRGBA& pt,U64 index )
	{
		if (m_header.number_of_col != 0 &&
			m_header.number_of_row != 0 &&
			m_header.number_of_col * m_header.number_of_row >= m_header.number_of_point_records)
		{
			I32 loop = index / m_header.number_of_row;
			if (loop != m_loopIndex)
			{
				m_loopIndex = loop;
			}
			const CLoopIndex& loopIndex = m_pLoopIndex[loop];
			U32 count = 0;
			if(ReadLoopFull(m_pPtBuf._Myfirst(),count,m_loopIndex))
			{
				I32 row = index % m_header.number_of_row;
				pt = m_pPtBuf[row];
				return TRUE;
			}
		}
		return FALSE;
	}

	inline BOOL CHL2Reader::ReadLoop( 
		PointXYZIPRGBA*& ptBuf,		// 外部传入的数组,外部管理指针	
		U32& count,				// 读取到的点数
		I32 loop,				// 圈号
		bool bCopyData)			// 是否拷贝数据到ptBuf,true拷贝数据,false复制指针
	{
		if (/*m_pDataFile == NULL ||*/ (bCopyData && ptBuf == NULL) || m_header.get_pointformat() != HLS2_POINTFORMAT_XYZIRGBP
			|| loop < 0 || loop >= m_header.number_of_col)
		{
			return FALSE;
		}

		const CLoopIndex& loopIndex = m_pLoopIndex[loop];
		count = loopIndex.m_loopIdx.count;//size / sizeof(PointXYZI);
		m_loopIndex = loop;
		return ReadBuffer((void*&)ptBuf,loopIndex.m_loopIdx,bCopyData);
	}

	BOOL CHL2Reader::ReadLoopFull( 
		PointXYZIPRGBA*& ptBuf, /* 外部传入的数组,外部管理指针 */ 
		U32& count,				/* 读取到的点数 */ 
		I32 loop )				/* 圈号 */ 
	{
		if (m_pDataFile == NULL || ptBuf == NULL || m_header.get_pointformat() != HLS2_POINTFORMAT_XYZIRGBP
			|| loop < 0 || loop >= m_header.number_of_col)
		{
			return FALSE;
		}

		BOOL bRet = FALSE;
		const CLoopIndex& loopIndex = m_pLoopIndex[loop];
		count = loopIndex.m_loopIdx.rowCount;
		m_loopIndex = loop;
		if (loopIndex.m_loopIdx.segCount == 0)
		{
			bRet = ReadBuffer((void*&)ptBuf,loopIndex.m_loopIdx,true);
		}
		else
		{
			PointXYZIPRGBA* pValidBuf = NULL;
			bRet = ReadBuffer((void*&)pValidBuf,loopIndex.m_loopIdx,false);
			if (bRet)
			{
				memset(ptBuf,0,sizeof(PointXYZIPRGBA) * loopIndex.m_loopIdx.rowCount);
				U32 index = 0;
				for (U16 i = 0;i < loopIndex.m_loopIdx.segCount;i++)
				{
					const HLS2_LOOPSEG& loopSeg = loopIndex.m_pLoopSeg[i];
					U32 start = loopSeg.getStart();
					U32 end = loopSeg.getStart() + loopSeg.getCount();
					for (U32 j = start;j < end;j++)
					{
						PointXYZIPRGBA& pt = ptBuf[j];
						pt = pValidBuf[index];
						index++;
					}
				}
			}
		}

		return bRet;
	}
	// 读取一圈,将数据拷贝到目标数组
	BOOL CHL2Reader::ReadLoopTest(
		PointXYZIPRGBA*& ptBuf,	// 外部传入的数组,外部管理指针	
		U32& count,				// 读取到的点数
		I32 loop)				// 圈号
	{
		if (/*m_pDataFile == NULL ||*/ (ptBuf == NULL) || m_header.get_pointformat() != HLS2_POINTFORMAT_XYZIRGBP
			|| loop < 0 || loop >= m_header.number_of_col)
		{
			return FALSE;
		}

		m_loopIndex = loop;
		const CLoopIndex& loopIndex = m_pLoopIndex[loop];
		count = loopIndex.m_loopIdx.count;//size / sizeof(PointXYZI);
		if (loopIndex.m_loopIdx.fileNo != m_curFileIndex || m_mapAddress == NULL)
		{
			char strDataFile[256] = {0};
			sprintf(strDataFile,"%s\\%s-%04d.hls",m_dir,m_name,loopIndex.m_loopIdx.fileNo);
			if(!OpenFile(strDataFile))
				return FALSE;

			m_curFileIndex = loopIndex.m_loopIdx.fileNo;
		}
		DWORD  dwBytesRead = 0;
		return ReadFile(m_pDataFile,ptBuf,count * sizeof(PointXYZIPRGBA),&dwBytesRead,NULL);
	}
	// 读取一圈的数据
	BOOL CHL2Reader::ReadBuffer(void*& pBuf,const HLS2_LOOPINDEX& loopIndex,bool bCopyData)
	{
		if (loopIndex.fileNo != m_curFileIndex || m_mapAddress == NULL)
		{
			char strDataFile[256] = {0};
			sprintf(strDataFile,"%s\\%s-%04d.hls",m_dir,m_name,loopIndex.fileNo);
			if(!OpenFile(strDataFile))
				return FALSE;

			m_curFileIndex = loopIndex.fileNo;
		}

		BOOL bRet = FALSE;
		if (m_mapAddress)
		{
			if (bCopyData)
			{
				errno_t err = memcpy_s(pBuf,loopIndex.count * m_header.get_recordLength(),
					m_mapAddress + loopIndex.offset,loopIndex.count * m_header.get_recordLength());
				bRet = (err == 0);
			}
			else
			{
				pBuf = m_mapAddress + loopIndex.offset;
				bRet = TRUE;
			}
		}
		return bRet;
	}

	void CHL2Reader::CloseFile()
	{
		EnterCriticalSection(&m_cs);
		CloseMapdata();
		if (m_pDataFile != NULL)
		{
			CloseHandle(m_pDataFile);
			m_pDataFile = NULL;
		}
		m_curFileIndex = 0xffff;
		LeaveCriticalSection(&m_cs);
	}

	BOOL CHL2Reader::Close()
	{
		CloseFile();

		if (m_pLoopIndex)
		{
			delete[] m_pLoopIndex;
			m_pLoopIndex = NULL;
		}
		m_header.clean();

		m_loopIndex = 0;
		m_mapAddress = NULL;
		m_curFileIndex = 0;
		return TRUE;
	}

	void CHL2Reader::GetCoordinate( double& x,double& y,double& z )
	{
		// 将本地坐标转换为全局坐标
		double tmpX = x;
		double tmpY = y;
		double tmpZ = z;
		x = m_matrix[0]*tmpX + m_matrix[1]*tmpY + m_matrix[2]*tmpZ + m_matrix[3];
		y = m_matrix[4]*tmpX + m_matrix[5]*tmpY + m_matrix[6]*tmpZ + m_matrix[7];
		z = m_matrix[8]*tmpX + m_matrix[9]*tmpY + m_matrix[10]*tmpZ + m_matrix[11];
		double w = m_matrix[12]*tmpX + m_matrix[13]*tmpY + m_matrix[14]*tmpZ + m_matrix[15];

		double f = 1.0/w;
		x = static_cast<double>(x*f);
		y = static_cast<double>(y*f);
		z = static_cast<double>(z*f);
	}

	void CHL2Reader::computeMatrix()
	{
		m_header.computeMatrix(m_matrix);
	}

}

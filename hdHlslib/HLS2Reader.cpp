#include "HLS2Reader.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{

	CHLS2Reader::CHLS2Reader(void)
		:IHLSReader(),m_pDataFile(NULL),m_fileMap(NULL),m_mapAddress(NULL),m_loopIndex(-1),
		m_curFileIndex(0xffff),m_mrStart(0),m_mrEnd(0)
	{
		memset(m_dir,0,256);
		memset(m_name,0,256);

		::InitializeCriticalSectionAndSpinCount( &m_cs, 0x80000401 );

		m_bUseMemMap = false;
	}

	CHLS2Reader::~CHLS2Reader(void)
	{
		CloseFile();

		m_header.clean();

		m_loopIndex = -1;
		m_curFileIndex = 0xffff;
		memset(m_dir,0,256);
		memset(m_name,0,256);

		DeleteCriticalSection(&m_cs);
	}

	BOOL CHLS2Reader::Open( const char* path)
	{
		CloseFile();

		m_header.clean();

		m_loopIndex = -1;
		m_curFileIndex = 0xffff;
		memset(m_dir,0,256);
		memset(m_name,0,256);


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

		if(m_header.number_of_row > 0)
		{
			m_pPtBuf.resize(m_header.number_of_row);
		}

		fclose(pIndexFile);
		pIndexFile = NULL;

		// 加载倾角仪数据
		char strAnglesFile[MAX_PATH] = {0};
		sprintf(strAnglesFile,"%s.angle",path);
		m_incFit.FitBySimpleData(strAnglesFile);
		// 计算转换坐标矩阵
		ComputeMatrix();

		strcpy(m_filepath,path);
		return TRUE;
	}

	CLoopIndex* CHLS2Reader::GetLoopIndex( void (*loadCallback)(float,const char*) /*= NULL*/ )
	{		
		if (m_header.version_major == 2 && m_pLoopIndex == NULL)
		{
			char path[256];
			sprintf(path,"%s\\%s.hls",m_dir,m_name);
			FILE* pIndexFile = fopen(path,"rb");
			if (pIndexFile != NULL)
			{
				fseek(pIndexFile,256,SEEK_SET);
				// 读取索引
				m_pLoopIndex = IHLSReader::AllocLoopIndex(m_header.number_of_col);//new CLoopIndex[m_header.number_of_col];
				for (int i = 0;i<m_header.number_of_col;i++)
				{
					CLoopIndex& loopIdx = m_pLoopIndex[i];
					HLS2_LOOPINDEX* pIds = &loopIdx.m_loopIdx;
					if (!pIds)
					{
						return 0;
					}
					fread(&loopIdx.m_loopIdx,sizeof(HLS2_LOOPINDEX),1,pIndexFile);

					if (loopIdx.m_loopIdx.segCount > 0)
					{
						loopIdx.AllocLoopSeg();
						fread(loopIdx.m_pLoopSeg,sizeof(HLS2_LOOPSEG),loopIdx.m_loopIdx.segCount,pIndexFile);
					}

					m_header.min_x = MIN(m_header.min_x,loopIdx.m_loopIdx.xmin);
					m_header.min_y = MIN(m_header.min_y,loopIdx.m_loopIdx.ymin);
					m_header.min_z = MIN(m_header.min_z,loopIdx.m_loopIdx.zmin);
														
					m_header.max_x = MAX(m_header.max_x,loopIdx.m_loopIdx.xmax);
					m_header.max_y = MAX(m_header.max_y,loopIdx.m_loopIdx.ymax);
					m_header.max_z = MAX(m_header.max_z,loopIdx.m_loopIdx.zmax);
				}

				fclose(pIndexFile);
				pIndexFile = NULL;
			}
		}

		return IHLSReader::GetLoopIndex(loadCallback);
	}

	BOOL CHLS2Reader::OpenFile(const char* filePath)
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

		if (!m_bUseMemMap)
		{
			LeaveCriticalSection(&m_cs);
			return TRUE;
		}

		m_fileMap = ::CreateFileMapping(m_pDataFile, NULL, PAGE_READWRITE, 0, 0, NULL);//PAGE_READONLY
		if (m_fileMap == NULL)
		{
			LeaveCriticalSection(&m_cs);
			fprintf(stderr, "ERROR: cannot create file mapping\n");
			return FALSE;
		}

		// 得到系统分配力度
		SYSTEM_INFO SysInfo;
		GetSystemInfo(&SysInfo);

		DWORD dwGran = SysInfo.dwAllocationGranularity;

		DWORD sizeHigh = 0;
		U64 fileSize = ::GetFileSize(m_pDataFile,&sizeHigh);
		if (sizeHigh != 0)
		{
			fileSize = ((U64)sizeHigh << 32) | fileSize;
		}

		// 偏移地址
		U64 qwFileOffset = 0;
		U64 T_newmap = 900*dwGran;

		// 映射块大小
		DWORD dwBlockBytes = 1000*dwGran; // 文件数据分段大小

		if (fileSize - qwFileOffset < dwBlockBytes)
		{
			dwBlockBytes = (DWORD)fileSize;
		}

		m_mapAddress = (char*)(::MapViewOfFile (m_fileMap,FILE_MAP_ALL_ACCESS, (DWORD)(qwFileOffset >> 32),(DWORD)(qwFileOffset & 0xFFFFFFFF),dwBlockBytes));// FILE_MAP_WRITE | FILE_MAP_READ
		m_mrStart = 0;
		m_mrEnd = m_mrStart + dwBlockBytes;

		DWORD err = ::GetLastError();

		LeaveCriticalSection(&m_cs);

		//return m_mapAddress != NULL;
		return TRUE;
	}

	BOOL CHLS2Reader::Read_Point( PointXYZIPRGBA& pt,U64 index )
	{
		EnterCriticalSection(&m_cs);
		if (m_header.number_of_col != 0 &&
			m_header.number_of_row != 0 &&
			m_header.number_of_col * m_header.number_of_row >= m_header.number_of_point_records &&
			index < m_header.number_of_point_records)
		{
			GetLoopIndex();
			I32 loop = index / m_header.number_of_row;
			if (loop != m_loopIndex)
			{
				m_loopIndex = loop;
				m_pPtBuf.clear();
				if(!ReadLoopFull(m_pPtBuf,m_loopIndex))
				{
					LeaveCriticalSection(&m_cs);
					return FALSE;
				}
			}
			
			I32 row = index % m_header.number_of_row;
			if (m_pPtBuf.size() <= row)
			{
				LeaveCriticalSection(&m_cs);
				return FALSE;
			}

			pt = *(m_pPtBuf._Myfirst + row);
			LeaveCriticalSection(&m_cs);
			return TRUE;			
		}
		LeaveCriticalSection(&m_cs);
		return FALSE;
	}

	inline BOOL CHLS2Reader::ReadLoop( 
		hdVector<PointXYZIPRGBA>& ptBuf,		// 外部传入的数组,外部管理指针	
		I32 loop,U32 simple)				// 圈号
	{
		EnterCriticalSection(&m_cs);
		if ((m_header.point_data_format != HLS2_POINTFORMAT_XYZIRGBP && m_header.point_data_format == HLS2_POINTFORMAT_RHVI)
			|| loop < 0 || loop >= m_header.number_of_col)
		{
			LeaveCriticalSection(&m_cs);
			return FALSE;
		}
		
		const CLoopIndex& loopIndex = m_pLoopIndex[loop];
		m_loopIndex = loop;
		BOOL bRet = FALSE;
		try
		{		
			if (simple == 1)
			{
				ptBuf.resize(loopIndex.m_loopIdx.count);
				bRet = ReadBuffer((void*&)ptBuf._Myfirst,loopIndex.m_loopIdx);
			}
			else if(simple > 1)
			{
				//PointXYZIPRGBA* pPts = NULL;
				hdVector<PointXYZIPRGBA> pPts;
				pPts.resize(loopIndex.m_loopIdx.count);
				bRet = ReadBuffer((void*&)pPts._Myfirst,loopIndex.m_loopIdx);
				if (bRet)
				{
					ptBuf.resize(loopIndex.m_loopIdx.count / simple);
				
					int index = 0;
					//for (int i = 0;i < loopIndex.m_loopIdx.count;i+=simple)
					//{		
					//	PointXYZIPRGBA& pt = *(ptBuf._Myfirst + index);
					//	memcpy(&pt,pPts._Myfirst + i,m_header.point_data_record_length);
					//	index++;
					//}

					// 修改数组越界问题（朱立雄 2017-4-20）
					for (int i = simple - 1; i < loopIndex.m_loopIdx.count; i += simple)
					{
						ptBuf[index ++] = pPts[i];
					}
				}
			}

			if (bRet && m_header.point_data_format == HLS2_POINTFORMAT_RHVI)
			{
				ComputeCoordate(ptBuf);
			}
		}
		catch (...)
		{
			LeaveCriticalSection(&m_cs);
			return false;
		}
		LeaveCriticalSection(&m_cs);
		return bRet;
	}

	BOOL CHLS2Reader::ReadLoopFull( 
		hdVector<PointXYZIPRGBA>& ptBuf,  /* 外部传入的数组,外部管理指针 */ 
		I32 loop,U32 simple )							 /* 圈号 */ 
	{
		EnterCriticalSection(&m_cs);
		if (m_header.point_data_format != HLS2_POINTFORMAT_XYZIRGBP
			|| loop < 0 || loop >= m_header.number_of_col)
		{
			LeaveCriticalSection(&m_cs);
			return FALSE;
		}

		const CLoopIndex& loopIndex = m_pLoopIndex[loop];
		
		m_loopIndex = loop;
		BOOL bRet = FALSE;
		//ptBuf.clear();
		try
		{			
			if (loopIndex.m_loopIdx.segCount == 1)
			{
				ptBuf.resize(loopIndex.m_loopIdx.rowCount);
				PointXYZIPRGBA* pTmp = ptBuf._Myfirst + loopIndex.m_pLoopSeg[0].getStart();
				bRet = ReadBuffer((void*&)(pTmp),loopIndex.m_loopIdx);
			}
			else
			{
				hdVector<PointXYZIPRGBA> validPts;
				validPts.resize(loopIndex.m_loopIdx.count);
				bRet = ReadBuffer((void*&)validPts._Myfirst,loopIndex.m_loopIdx);
				if (bRet)
				{
					ptBuf.resize(loopIndex.m_loopIdx.rowCount);
					U32 index = 0;
					for (U16 i = 0;i < loopIndex.m_loopIdx.segCount;i++)
					{
						const HLS2_LOOPSEG& loopSeg = loopIndex.m_pLoopSeg[i];
						U32 start = loopSeg.getStart();
						U32 end = start + loopSeg.getCount();
						for (U32 j = start;j<end;j++)
						{
							PointXYZIPRGBA& pt = ptBuf[j];
							pt = *(validPts._Myfirst + index);
							index++;
						}
					}
				}
			}

			if (simple > 1 && ptBuf.size() > 0)
			{
				int index = 0;
				for (int i = 0;i < loopIndex.m_loopIdx.rowCount;i+=simple)
				{		
					*(ptBuf._Myfirst + index) = *(ptBuf._Myfirst + i);
					index++;
				}
				ptBuf.resize(index);
			}

			if (m_header.point_data_format == HLS2_POINTFORMAT_RHVI)
			{
				ComputeCoordate(ptBuf);
			}
		}
		catch (...)
		{
		}
		LeaveCriticalSection(&m_cs);
		return bRet;
	}
	
	// 读取一圈的数据
	BOOL CHLS2Reader::ReadBuffer(void*& pBuf,const HLS2_LOOPINDEX& loopIndex,bool bCopy)
	{
		if (loopIndex.fileNo != m_curFileIndex )
		{
			char strDataFile[256] = {0};
			sprintf(strDataFile,"%s\\%s-%04d.hls",m_dir,m_name,loopIndex.fileNo);
			if(!OpenFile(strDataFile))
				return FALSE;

			m_curFileIndex = loopIndex.fileNo;
		}

		BOOL bRet = FALSE;

		// 不使用内存映射
		if (!m_bUseMemMap)
		{
			if (bCopy)
			{
							
				SetFilePointer(m_pDataFile,(LONG)loopIndex.offset,0,FILE_BEGIN);
				DWORD numRead = 0;
				DWORD numSize = loopIndex.count * m_header.get_recordLength();
				bRet = ReadFile(m_pDataFile,pBuf,numSize,&numRead,NULL);
				bRet = (numRead == numSize);
			}
			else
			{
				pBuf = m_mapAddress + loopIndex.offset;
				bRet = TRUE;
			}
		
			return bRet;
		}

		if (m_mapAddress != NULL)
		{
			if (bCopy)
			{
				// 判断此时需要读取的位置是否大于映射区域
				// 读取的区段在映射范围内
				if (loopIndex.offset >= m_mrStart && loopIndex.offset + loopIndex.count*m_header.get_recordLength() < m_mrEnd)
				{
					errno_t err = memcpy_s(pBuf,loopIndex.count * m_header.get_recordLength(),
						m_mapAddress + loopIndex.offset - m_mrStart,loopIndex.count * m_header.get_recordLength());
					bRet = (err == 0);
				}
				// 读取的区段在映射范围前段
				else if (loopIndex.offset < m_mrStart)
				{
					// 重新映射
					UnmapViewOfFile(m_mapAddress);  // 释放当前映射

					m_mapAddress = 0;

					// 在读取区段的开始处开始映射		
					SYSTEM_INFO SysInfo;
					GetSystemInfo(&SysInfo);

					DWORD dwGran = SysInfo.dwAllocationGranularity;

					double times = (double)loopIndex.offset/(double)dwGran;
					m_mrStart = (int)times*dwGran;

					// 映射块大小
					DWORD dwBlockBytes = 1000*dwGran; // 文件数据分段大小

					m_mapAddress = (char*)(::MapViewOfFile(m_fileMap,FILE_MAP_READ,(DWORD)(m_mrStart >> 32),(DWORD)(m_mrStart & 0xFFFFFFFF),dwBlockBytes));

					DWORD err = ::GetLastError(); 

					if (m_mapAddress)
					{
						// 读取数据
						errno_t err = memcpy_s(pBuf,loopIndex.count * m_header.get_recordLength(),
							m_mapAddress + loopIndex.offset - m_mrStart,loopIndex.count * m_header.get_recordLength());

						bRet = (err == 0);

						m_mrEnd = m_mrStart + dwBlockBytes;

					}

					return bRet;

				}
				// 读取的区段在映射范围后段
				else if (loopIndex.offset > m_mrEnd)
				{
					// 重新映射
					UnmapViewOfFile(m_mapAddress);  // 释放当前映射

					m_mapAddress = 0;

					// 在读取区段的开始离64K整数倍最近处开始映射
					SYSTEM_INFO SysInfo;
					GetSystemInfo(&SysInfo);

					DWORD dwGran = SysInfo.dwAllocationGranularity;
				
					double times = (double)loopIndex.offset/(double)dwGran;
					m_mrStart = (int)times*dwGran;

					// 映射块大小
					DWORD dwBlockBytes = 1000*dwGran; // 文件数据分段大小

					// 得到文件大小
					DWORD sizeHigh = 0;
					U64 fileSize = ::GetFileSize(m_pDataFile,&sizeHigh);
					if (sizeHigh != 0)
					{
						fileSize = ((U64)sizeHigh << 32) | fileSize;
					}

					if (m_mrStart + dwBlockBytes > fileSize)
					{
						dwBlockBytes = fileSize - m_mrStart;
					}

					m_mapAddress = (char*)(::MapViewOfFile(m_fileMap,FILE_MAP_READ,(DWORD)(m_mrStart >> 32),(DWORD)(m_mrStart & 0xFFFFFFFF),dwBlockBytes));

					DWORD err = ::GetLastError(); 

					if (m_mapAddress)
					{
						// 读取数据
						errno_t err = memcpy_s(pBuf,loopIndex.count * m_header.get_recordLength(),
							m_mapAddress + loopIndex.offset - m_mrStart,loopIndex.count * m_header.get_recordLength());

						bRet = (err == 0);

						m_mrEnd = m_mrStart + dwBlockBytes;

					}

					return bRet;
				}
				

				// 写出时文件大小控制为2G，文件的偏移小于long型，暂时该函数满足使用 fengjing
				/*SetFilePointer(m_pDataFile,(LONG)loopIndex.offset,0,FILE_BEGIN);
				DWORD numRead = 0;
				DWORD numSize = loopIndex.count * m_header.get_recordLength();
				bRet = ReadFile(m_pDataFile,pBuf,numSize,&numRead,NULL);
				bRet = (numRead == numSize);*/
				
			}
			else
			{
				pBuf = m_mapAddress + loopIndex.offset;
				bRet = TRUE;
			}
		}
		return bRet;
	}

	BOOL CHLS2Reader::UpdateLoop( 
		const hdVector<PointXYZIPRGBA>& ptBuf, // 需要更新的列数据 
		I32 loop )								  // 列序号
	{
		if (m_header.point_data_format != HLS2_POINTFORMAT_XYZIRGBP
			|| loop < 0 || loop >= m_header.number_of_col)
		{
			return FALSE;
		}
		const CLoopIndex& loopIndex = m_pLoopIndex[loop];
		//如果点数大于当前圈的点数则失败
		if (ptBuf.size() > loopIndex.m_loopIdx.rowCount)
		{
			return FALSE;
		}
		if (loopIndex.m_loopIdx.fileNo != m_curFileIndex )
		{
			char strDataFile[256] = {0};
			sprintf(strDataFile,"%s\\%s-%04d.hls",m_dir,m_name,loopIndex.m_loopIdx.fileNo);
			if(!OpenFile(strDataFile))
				return FALSE;

			m_curFileIndex = loopIndex.m_loopIdx.fileNo;
		}
		if(m_pDataFile == NULL /*|| m_mapAddress == NULL*/)
			return FALSE;
		BOOL bRet = FALSE;
		try
		{
			PointXYZIPRGBA* pTmp = ptBuf._Myfirst;
			bRet = WriteBuffer((void*&)(pTmp),loopIndex.m_loopIdx);
		}
		catch (...)
		{
			
		}
		return bRet;
		/*errno_t err = memcpy_s(m_mapAddress + loopIndex.m_loopIdx.offset,
			m_header.point_data_record_length * ptBuf.size(),
			ptBuf._Myfirst,m_header.point_data_record_length * ptBuf.size());
		return err == 0;*/
	}

	void CHLS2Reader::CloseFile()
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

	void CHLS2Reader::CloseMapdata()
	{
		BOOL bRet = FALSE;
		if (m_fileMap)
		{
			if (m_mapAddress)
			{
				bRet = ::UnmapViewOfFile (m_mapAddress);
				m_mapAddress = 0;
			}
			bRet = CloseHandle (m_fileMap);
			m_fileMap = 0;
		}
	}

	BOOL CHLS2Reader::WriteBuffer( void*& pBuf,const HLS2_LOOPINDEX& loopIndex,bool bCopy /*= true*/ )
	{
		if (loopIndex.fileNo != m_curFileIndex )
		{
			char strDataFile[256] = {0};
			sprintf(strDataFile,"%s\\%s-%04d.hls",m_dir,m_name,loopIndex.fileNo);
			if(!OpenFile(strDataFile))
				return FALSE;

			m_curFileIndex = loopIndex.fileNo;
		}

		BOOL bRet = FALSE;

		// 不使用内存映射
		if (!m_bUseMemMap)
		{
			if (bCopy)
			{		
				SetFilePointer(m_pDataFile,(LONG)loopIndex.offset,0,FILE_BEGIN);
				DWORD numWrite = 0;
				DWORD numSize = loopIndex.count * m_header.get_recordLength();
				//bRet = ReadFile(m_pDataFile,pBuf,numSize,&numRead,NULL);
				bRet = WriteFile(m_pDataFile,pBuf,numSize,&numWrite,NULL);
				bRet = (numWrite == numSize);
			}
			else
			{
				pBuf = m_mapAddress + loopIndex.offset;
				bRet = TRUE;
			}
		
			return bRet;
		}

		return bRet;
	}

	/*BOOL CHLS2Reader::Close()
	{

	return TRUE;
	}*/

}

//#include "HLZ31Writer.h"
//#include "HdCoreData.h"
//#include "../hdCommon/HdRandomGenerator.h"
//#include "HdLevel31.h"
//#include "HdBlockset31.h"
//#include "HdBlock31.h"
//#include "HdParcel31.h"
//#include <time.h>
//
//#ifdef _DEBUG
//#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
//#endif
//
//namespace hd
//{
//	CHLZ31Writer::CHLZ31Writer(void)
//		:m_curFileIndex(0),m_pIndexFile(NULL),m_pDataFile(NULL),m_pColorFile(NULL),m_pPropFile(NULL),
//		m_nWrittenLen(0), m_pWriteAddr(NULL), m_pMapEndAddr(NULL), m_pMapBegAddr(NULL), m_fileMap(NULL)
//	{
//		m_bOnlyUndateData = false;
//		m_curDataAddr = 0;
//
//		SYSTEM_INFO sinf;
//		::GetSystemInfo(&sinf);
//		m_dwAllocGran = sinf.dwAllocationGranularity;
//	}
//
//
//	CHLZ31Writer::~CHLZ31Writer(void)
//	{
//		Close();
//	}
//
//	BOOL CHLZ31Writer::Open( const char* file_name, bool bOnlyUpdata, U32 io_buffer_size,bool bAppend)
//	{
//		Close();
//
//		// 获取文件路径和文件名
//		char drive[256] = {0};
//		char dir[256] = {0};
//		char filename[256] = {0};
//		char ext[256] = {0};
//
//		char dirAbs[256] = {0};
//		char name[256] = {0};
//		_splitpath(file_name,drive,dir,filename,ext);
//		_makepath(dirAbs,drive,dir,NULL,NULL);
//
//		strcpy(m_dir,dirAbs);
//		strcpy(m_name,filename);
//
//		char VolName[256]={0};// 磁盘驱动器卷标名称
//
//		DWORD SerailNumber;// 磁盘驱动器卷标序列号
//		DWORD MaxCLenth;// 系统允许的最大文件名长度
//		DWORD FileSysFlag;// 文件系统标识
//		char FileSysName[256]={0};// 文件操作系统名称
//
//		GetVolumeInformation(drive,VolName,255,&SerailNumber,&MaxCLenth,&FileSysFlag,FileSysName,255);
//
//		if (strcmp("FAT32",FileSysName) == 0)
//		{
//			m_fileFlag = 1;
//		}
//		else if (strcmp("NTFS",FileSysName) == 0)
//		{
//			m_fileFlag = 0;
//		}
//
//		if (!bOnlyUpdata)
//		{
//			// 打开数据文件dir+name-dxxxx.hls
//			char strDataFile[256] = {0};
//			sprintf(strDataFile,"%s\\%s-%04d.hld",m_dir,m_name,m_curFileIndex);
//			m_pDataFile = CreateFile(strDataFile, 
//				GENERIC_READ | GENERIC_WRITE,
//				FILE_SHARE_READ|FILE_SHARE_WRITE, 
//				NULL,
//				bAppend ? OPEN_ALWAYS : CREATE_ALWAYS, 
//				FILE_ATTRIBUTE_NORMAL, 
//				NULL);
//
//			if (m_pDataFile == NULL)
//			{
//				return FALSE;
//			}
//
//			LARGE_INTEGER li;
//			li.HighPart = 0;
//			li.LowPart = 0;
//
//			LARGE_INTEGER li1;
//			li1.HighPart = 0;
//			li1.LowPart = 0;
//
//			SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
//
//			// 打开颜色文件dir+name-cxxxx.hls
//			// 打开分类文件dir+name-pxxxx.hls
//
//			// 打开索引文件dir+name.hls
//			char strIndexFile[256] = {0};
//			sprintf(strIndexFile,"%s\\%s.hlz",m_dir,m_name);
//			m_pIndexFile = fopen(strIndexFile/*file_name*/,"w+b");
//			if (m_pIndexFile == NULL)
//			{
//				return FALSE;
//			}
//
//			m_header.number_of_col = 0;
//			m_header.number_of_point_records = 0;
//
//			WriteHeader();	// 空的头文件写入
//
//			if(!bAppend)
//			{
//				m_fileMap = ::CreateFileMapping(m_pDataFile, NULL, PAGE_READWRITE, 0, 0x80000000, NULL);
//				if(NULL == m_fileMap)
//				{
//					return FALSE;
//				}
//
//				m_nWrittenLen = 0;
//				m_pMapBegAddr = (char*)::MapViewOfFile(m_fileMap, FILE_MAP_WRITE, 0, 0, 0x06400000);
//				m_pWriteAddr = m_pMapBegAddr;
//				m_pMapEndAddr = m_pWriteAddr + 0x06400000;
//			}
//		}
//		else
//		{
//			// 打开索引文件dir+name.hls
//			char strIndexFile[256] = {0};
//			sprintf(strIndexFile,"%s\\%s.hlz",m_dir,m_name);
//			m_pIndexFile = fopen(strIndexFile/*file_name*/,"r+b");
//			if (m_pIndexFile == NULL)
//			{
//				return FALSE;
//			}
//		}
//
//		return TRUE;
//	}
//
//	void CHLZ31Writer::WriteHeader()
//	{
//		if (m_pIndexFile == NULL)
//		{
//			return;
//		}
//
//		fseek(m_pIndexFile,0,SEEK_SET);
//
//		// 写入文件头
//		size_t headerSize = sizeof(HLZheader);
//		char hdBuf[256] = {0};
//		m_header.serialize(hdBuf);
//
//		fwrite(hdBuf,sizeof(char),256,m_pIndexFile);
//	}
//
//	// 写入头文件
//	void CHLZ31Writer::WriteHeader(const HLZheader& hlzHeader)
//	{
//		if (m_pIndexFile == NULL)
//		{
//			return;
//		}
//
//		fseek(m_pIndexFile,0,SEEK_SET);
//
//		char hdBuf[256] = {0};
//		hlzHeader.serialize(hdBuf);
//
//		fwrite(hdBuf,sizeof(char),256,m_pIndexFile);
//
//		fclose(m_pIndexFile);
//		m_pIndexFile = NULL;
//	}
//
//	//! 写入索引信息
//	BOOL CHLZ31Writer::WriteIndex(map<U16,CHdLevel31*>& mLevel)
//	{
//		if (m_pIndexFile == NULL)
//		{
//			return FALSE;
//		}
//		// 先抹去原来内容
//		fseek(m_pIndexFile, 0, SEEK_END);
//		long pos = ftell(m_pIndexFile);
//		long size = pos - 256;
//		char* cTmp = new char[size];
//		memset(cTmp,0,size);
//		fseek(m_pIndexFile, 256, SEEK_SET);
//		fwrite(cTmp,1,size,m_pIndexFile);
//		delete[] cTmp;
//		cTmp = NULL;
//
//		// 定位到索引起始位置
//		fseek(m_pIndexFile, 256, SEEK_SET);
//		int lvlCount = m_header.number_of_level;
//		CHdLevel31* pLevel = NULL;
//		U16 i = 0;
//		for (i = 0;i < lvlCount;i++)
//		{
//			pLevel = mLevel[i];
//			fwrite(&(pLevel->m_level),sizeof(HdLevel31),1,m_pIndexFile);
//
//			CHdBlockset31* pBlockSet = NULL;
//			for (auto itLevel = pLevel->m_pListBlockset.begin();itLevel != pLevel->m_pListBlockset.end();itLevel++)
//			{
//				pBlockSet = itLevel->second;
//				//blockNum = 0说明块集没有分块，这时块集索引中含有数据地址信息
//				if(pBlockSet->m_blockSet.getBlockNum() == 0)
//				{
//					///1.先写块集索引中定长部分
//					U16 fixLen = sizeof(HdBlockset31) - 2 * sizeof(HdAddr);
//					fwrite(&pBlockSet->m_blockSet, fixLen, 1, m_pIndexFile);
//
//					//2.判断是否包含颜色、时间数据，决定写入块集索引的长度
//					//包含颜色时间数据
//					if(pBlockSet->m_blockSet.hasColor() && pBlockSet->m_blockSet.hasTime())
//					{
//						fwrite(&pBlockSet->m_blockSet.addrColor, 2 * sizeof(HdAddr), 1, m_pIndexFile);
//					}
//					//仅包含颜色数据
//					else if(pBlockSet->m_blockSet.hasColor())
//					{
//						fwrite(&pBlockSet->m_blockSet.addrColor, sizeof(HdAddr), 1, m_pIndexFile);
//					}
//					//仅包含时间数据
//					else if(pBlockSet->m_blockSet.hasTime())
//					{
//						fwrite(&pBlockSet->m_blockSet.addrTime, sizeof(HdAddr), 1, m_pIndexFile);
//					}
//					//不包含时间、颜色数据，已经写入完毕
//					else
//					{
//					}
//
//					//当前块集不包含块，直接处理下一个块集
//					continue;
//				}
//				else
//				{
//					//blockNum > 0说明块集有分块，这时块集索引中不包含数据地址信息
//					fwrite(&pBlockSet->m_blockSet, sizeof(HdBlockset31) - sizeof(HdAddr)*3 - sizeof(U16), 1, m_pIndexFile);
//				}
//
//				CHdBlock31* pBlock = NULL;
//				for (auto itBK = pBlockSet->m_pListBlock.begin();itBK != pBlockSet->m_pListBlock.end();itBK++)
//				{
//					pBlock = itBK->second;
//
//					//parcelNum = 0说明块没有分包，这时块索引中含有数据地址信息
//					if(pBlock->m_block.numParcel == 0)
//					{
//						//1.先写入定长索引部分
//						fwrite(&pBlock->m_block, sizeof(HdBlock31) - 2 * sizeof(HdAddr), 1, m_pIndexFile);
//
//						//2.判断是否包含颜色、时间数据，决定写入块索引的长度
//						//包含颜色时间数据
//						if(pBlock->m_block.hasColor() && pBlock->m_block.hasTime())
//						{
//							fwrite(&pBlock->m_block.addrColor,  2* sizeof(HdAddr), 1, m_pIndexFile);
//						}
//						//仅包含颜色数据
//						else if(pBlock->m_block.hasColor())
//						{
//							fwrite(&pBlock->m_block.addrColor, sizeof(HdAddr), 1, m_pIndexFile);
//						}
//						//仅包含时间数据
//						else if(pBlock->m_block.hasTime())
//						{
//							fwrite(&pBlock->m_block.addrTime, sizeof(HdAddr), 1, m_pIndexFile);
//						}
//						//不包含时间、颜色数据，已经写入完毕
//						else
//						{
//						}
//
//						//当前块不包含包，直接处理下一个块
//						continue;
//					}
//					else
//					{
//						//parcelNum > 0说明块有分包，这时块索引中不包含数据地址信息
//						fwrite(&pBlock->m_block, sizeof(HdBlock31) - sizeof(HdAddr)*3 - sizeof(U16), 1, m_pIndexFile);
//					}
//
//					CHdParcel31* pParcel = NULL;
//					for (auto itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
//					{
//						pParcel = itPcl->second;
//
//						//1.先写包索引中定长部分
//						U16 fixLen = sizeof(HdParcel31) - 2 * sizeof(HdAddr);
//						fwrite(&pParcel->m_parcel, fixLen, 1, m_pIndexFile);
//
//						//2.判断是否包含颜色、时间数据，决定写入包索引的长度
//						//包含颜色时间数据
//						if(pParcel->m_parcel.hasColor() && pParcel->m_parcel.hasTime())
//						{
//							fwrite(&pParcel->m_parcel.addrColor, 2 * sizeof(HdAddr), 1, m_pIndexFile);
//						}
//						//仅包含颜色或时间数据
//						else if(pParcel->m_parcel.hasColor())
//						{
//							fwrite(&pParcel->m_parcel.addrColor, sizeof(HdAddr), 1, m_pIndexFile);
//						}
//						//仅包含时间数据
//						else if(pParcel->m_parcel.hasTime())
//						{
//							fwrite(&pParcel->m_parcel.addrTime, sizeof(HdAddr), 1, m_pIndexFile);
//						}
//						//不包含时间、颜色数据，已经写入完毕
//						else
//						{
//						}
//					}				
//				}//for (iBk = 0;iBk < pBlockSet->m_blockSet.m_numBlock;iBk++)
//			}//for (iBs = 0;iBs < bsCount;iBs++)
//		}//for (i = 0;i < lvlCount;i++)
//
//		return TRUE;
//	}
//
//	//支持同步写入颜色数据信息   袁亮  20160625
//	BOOL CHLZ31Writer::WriteBlockSetData(HdBlockset31& blockSet, const U8* ptBaseAttri, U32 baseAttriLen, const HdPtColor* ptColorBuf, U64 count, U32 cmpLen)
//	{
//		if (m_pDataFile == INVALID_HANDLE_VALUE || m_pIndexFile == NULL)
//		{
//			return FALSE;
//		}
//
//		U64 addrBaseAttri = 0;
//		U64 addrColor = 0;
//		// 写入包数据
//		//同步写入颜色数据     袁亮   20160625
//		if(!WriteData(ptBaseAttri, baseAttriLen, ptColorBuf, count, addrBaseAttri, addrColor))
//			return FALSE;
//
//		blockSet.addrBaseAttri.Prase(addrBaseAttri);
//		blockSet.addrColor.Prase(addrColor);
//		blockSet.setCmpSize(cmpLen);
//
//		return TRUE;
//	}
//
//	BOOL CHLZ31Writer::WriteBlockData(HdBlock31& block, const U8* ptBaseAttri, U32 baseAttriLen, const HdPtColor* ptColorBuf, U64 count, U32 cmpLen)
//	{
//		if (m_pDataFile == NULL || m_pIndexFile == NULL)
//		{
//			return FALSE;
//		}
//
//		U64 addrBaseAttri = 0;
//		U64 addrColor = 0;
//		// 写入包数据
//		//同步写入颜色数据     袁亮   20160625
//		if(!WriteData(ptBaseAttri, baseAttriLen, ptColorBuf,count,addrBaseAttri,addrColor))
//			return FALSE;
//
//		block.addrBaseAttri.Prase(addrBaseAttri);
//		block.addrColor.Prase(addrColor);
//		block.setCmpSize(cmpLen);
//
//		return TRUE;
//	}
//
//	BOOL CHLZ31Writer::WriteParcelData(HdParcel31& parcel, const U8* ptBaseAttri, U32 baseAttriLen, const HdPtColor* ptColorBuf, U64 count, U32 cmpLen)
//	{
//		if (m_pDataFile == INVALID_HANDLE_VALUE || m_pIndexFile == NULL)
//		{
//			return FALSE;
//		}
//
//
//		U64 addrBaseAttri = 0;
//		U64 addrColor = 0;
//		// 写入包数据
//		//同步写入颜色数据     袁亮   20160625
//		if(!WriteData(ptBaseAttri, baseAttriLen, ptColorBuf, count, addrBaseAttri, addrColor))
//			return FALSE;
//
//		parcel.addrBaseAttri.Prase(addrBaseAttri);
//		parcel.addrColor.Prase(addrColor);
//		parcel.setCmpSize(cmpLen);
//
//		return TRUE;
//	}
//
//	BOOL CHLZ31Writer::WriteData(const U8* ptBaseAttri,  U32 baseAttriLen, const HdPtColor* ptColorBuf, U64 count,U64& addrBaseAttri, U64& addrColor)
//	{
//		if (m_pDataFile == INVALID_HANDLE_VALUE || count <= 0)
//		{
//			return FALSE;
//		}
//
//		/*LARGE_INTEGER li;
//		li.HighPart = 0;
//		li.LowPart = 0;
//
//		LARGE_INTEGER li1;
//		li1.HighPart = 0;
//		li1.LowPart = 0;
//
//		SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);*/
//
//		//改用内存映射文件方式写hld文件，提高文件IO效率  袁亮    20160909
//		U64 curFileAddr = (U64)m_curFileIndex * (U64)0x80000000;
//		U32 nLeftSpace = 0x80000000 - m_nWrittenLen;
//		U32 nWriteLen =  baseAttriLen + sizeof(HdPtColor) * count;
//		//颜色可选
//		if(ptColorBuf == NULL)
//		{
//			nWriteLen = baseAttriLen;
//		}
//
//		addrBaseAttri  = addrColor = curFileAddr + m_nWrittenLen;
//		//addrCoord = curFileAddr + (U64)li1.QuadPart;
//
//		//不论文件系统，hlz数据文件都按照2GB大小进行划分    袁亮  20160818
//		/*if(m_fileFlag == 1)
//		{*/
//		U64 curDataAddr = addrBaseAttri + nWriteLen;
//		if(curDataAddr > curFileAddr + 0x80000000)//0x80000000 2GB
//		{
//			// 关闭上次文件
//			if (m_pDataFile!=INVALID_HANDLE_VALUE)
//			{
//				// 关闭数据文件
//				U32 nWriteLen = m_pWriteAddr - m_pMapBegAddr;
//				::FlushViewOfFile(m_pMapBegAddr, nWriteLen);
//				::UnmapViewOfFile(m_pMapBegAddr);
//				CloseHandle(m_fileMap);
//				SetFilePointer(m_pDataFile, m_nWrittenLen, NULL, FILE_BEGIN);
//				SetEndOfFile(m_pDataFile);
//				CloseHandle(m_pDataFile);
//				m_pDataFile = NULL;
//			}
//
//			// 打开数据文件dir+name-dxxxx.hls			
//			m_curFileIndex++;
//			curFileAddr = (U64)m_curFileIndex * (U64)0x80000000;
//
//			char strDataFile[256] = {0};
//			sprintf(strDataFile,"%s\\%s-%04d.hld",m_dir,m_name,m_curFileIndex);
//			m_pDataFile = CreateFile(strDataFile, 
//				GENERIC_READ | GENERIC_WRITE,
//				FILE_SHARE_READ|FILE_SHARE_WRITE, 
//				NULL,
//				CREATE_ALWAYS, 
//				FILE_ATTRIBUTE_NORMAL, 
//				NULL);
//
//			if (m_pDataFile == NULL)
//			{
//				return FALSE;
//			}
//
//			m_fileMap = ::CreateFileMapping(m_pDataFile, NULL, PAGE_READWRITE, 0, 0x80000000, NULL);
//			if(NULL == m_fileMap)
//			{
//				return FALSE;
//			}
//
//			m_nWrittenLen = 0;
//			m_pMapBegAddr = (char*)::MapViewOfFile(m_fileMap, FILE_MAP_WRITE, 0, 0, 0x06400000);
//			m_pWriteAddr = m_pMapBegAddr;
//			m_pMapEndAddr = m_pWriteAddr + 0x06400000;
//
//			addrBaseAttri = addrColor = curFileAddr;
//
//			/*SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
//			addrCoord = curFileAddr + (U64)li1.QuadPart;*/
//		}
//		//}
//
//		if(m_pWriteAddr + nWriteLen > m_pMapEndAddr)
//		{
//			U32 nWriteNum = m_pWriteAddr - m_pMapBegAddr;
//			::FlushViewOfFile(m_pMapBegAddr, nWriteNum);
//			::UnmapViewOfFile(m_pMapBegAddr);
//
//			U32 newMapAddr = m_nWrittenLen / m_dwAllocGran * m_dwAllocGran;
//			U32 offset = m_nWrittenLen - newMapAddr;
//			U32 dataLen = MIN(nLeftSpace, 0x06400000);
//			m_pMapBegAddr = (char*)::MapViewOfFile(m_fileMap, FILE_MAP_WRITE, 0, newMapAddr, dataLen+offset);
//			m_pWriteAddr = m_pMapBegAddr + offset;
//			m_pMapEndAddr = m_pWriteAddr + dataLen;
//		}
//
//
//		if(ptBaseAttri != NULL)
//		{
//			memcpy(m_pWriteAddr, ptBaseAttri, baseAttriLen);
//			m_pWriteAddr += baseAttriLen;
//			m_nWrittenLen += baseAttriLen;
//			addrColor = addrBaseAttri + baseAttriLen;
//		}
//
//		//颜色可选，不包含颜色数据时，颜色地址置为无效值
//		if(ptColorBuf != NULL)
//		{
//			memcpy(m_pWriteAddr, ptColorBuf, (DWORD)count * sizeof(HdPtColor));
//			m_pWriteAddr += count * sizeof(HdPtColor);
//			m_nWrittenLen += count * sizeof(HdPtColor);
//		}
//		else
//		{
//			addrColor = 0xFFFFFFFFFFFF;
//		}
//
//		return TRUE;
//	}
//
//	BOOL CHLZ31Writer::Close()
//	{
//		if(m_pMapBegAddr != NULL)
//		{
//			U32 nWriteLen = m_pWriteAddr - m_pMapBegAddr;
//			::FlushViewOfFile(m_pMapBegAddr, nWriteLen);
//			::UnmapViewOfFile(m_pMapBegAddr);
//			m_pMapBegAddr = NULL;
//
//			if(m_fileMap != NULL)
//			{
//				CloseHandle(m_fileMap);
//				m_fileMap = NULL;
//			}
//
//			SetFilePointer(m_pDataFile, m_nWrittenLen, NULL, FILE_BEGIN);
//			SetEndOfFile(m_pDataFile);
//		}
//
//		if(m_fileMap != NULL)
//		{
//			CloseHandle(m_fileMap);
//			m_fileMap = NULL;
//		}
//
//		if (m_pDataFile!=INVALID_HANDLE_VALUE)
//		{
//			// 关闭数据文件
//			CloseHandle(m_pDataFile);
//			m_pDataFile = NULL;
//		}
//
//		if (m_pColorFile)
//		{
//			// 关闭颜色数据文件
//		}
//
//		if (m_pPropFile)
//		{
//			// 关闭分类数据文件
//		}
//
//		if (m_pIndexFile)
//		{	
//			// 关闭索引文件
//			WriteHeader();
//			fclose(m_pIndexFile);
//			m_pIndexFile = NULL;
//		}
//
//		m_pDataFile = (NULL);
//		m_pColorFile = (NULL);
//		m_pPropFile = (NULL);
//		m_pIndexFile = (NULL);
//
//		return TRUE;
//	}
//}
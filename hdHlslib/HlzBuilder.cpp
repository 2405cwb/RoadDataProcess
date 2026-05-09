#include "HlzBuilder.h"

#include "HdLevel.h"
#include "HdBlockset.h"
#include "HdBlock.h"
#include "HdParcel.h"
#include "HdCoreData.h"
#include "..\hdHlslib\inc\lasreader.hpp"
#include "..\hdHlslib\IHLSReader.h"
#include "..\hdHlslib\HLSReadOpener.h"
#include "..\hdHlslib\HLSReader.h"
#include "..\hdHlslib\IHLSReader.h"
#include "..\hdCommon\hdSceneStr.h"
#include "..\hdCore\hdMath.h"
#include "..\hdCore\hdArray.h"
#include "..\hdHlslib\PtEncoder.h"

#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <algorithm>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

#define BLOCK_HOLD 64000
#define DEBUG_LOG 0
#define BUFFER_SIZE 100000
namespace hd
{

	CHlzBuilder::CHlzBuilder(void)
	{
		m_iMin = 0xffff;
		m_iMax = 0;
		m_ptNum = 0;
		m_pHlzWrite = NULL;
		processCallback = NULL;
		m_stepX = 64;		// 第0层格网大小为64m
		m_stepY = 64;
		m_xNoFrom = m_xNoTo = m_yNoFrom = m_yNoTo = 0;
		m_pCoreData = new CHdCoreData;

		// 初始化标记不重采样0层
		m_bResampleZero = false;
		m_dGridPrecision = 1.0/256.0;
	}

	CHlzBuilder::~CHlzBuilder(void)
	{	
		Close();
		// 删除所有临时文件
		//RemoveTempFiles(m_savePath.c_str());
	}

	void CHlzBuilder::Close()
	{
		//CloseHls();

		if (m_pHlzWrite)
		{
			delete m_pHlzWrite;
			m_pHlzWrite = NULL;
		}

		if (m_pCoreData)
		{
			delete m_pCoreData;
			m_pCoreData = NULL;
		}

		// 关闭所有文件
		//CloseTempFiles();
	}

	// 划分包文件
	bool CHlzBuilder::SplitParcelFiles(const CHdBox3df& blockBox, const char* strBlockFile, const char* savePath, std::vector<ParcelFileInfo>& arrayParcels)
	{
		// 打开分块文件
		HANDLE pDataFile = CreateFile(strBlockFile,
			GENERIC_READ,
			FILE_SHARE_READ, 
			NULL,
			OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, 
			NULL);

		if (pDataFile == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}

		// 对块文件进行切分
		std::map<U8, ParcelFileInfo> mapSplitParcels;	// 一个块分割出来的包文件

		// 移到文件头
		//fseek(pBlockFile, 0, SEEK_SET);		

		//std::vector<PointXYZIPRGBA> bufferPts;

		CHdVector3df center = blockBox.getCenter();
		CHdVector3df halfSize = blockBox.getExtent() / 2.0f;

		//重复利用点数据缓冲区，减少内存new/delete开销			
		PointXYZIPRGBA* pBuffer = new PointXYZIPRGBA[BUFFER_SIZE];

		//使用内存映射文件读取块中的点数据   袁亮   20160909
		SYSTEM_INFO sinf;
		::GetSystemInfo(&sinf);
		DWORD dwAllocGran = sinf.dwAllocationGranularity;
		//每次映射加载500W个点(5000000*18/1024/1024 = 85.8M)
		DWORD dwDataLen = 5000000 * sizeof(PointXYZIPRGBA);

		DWORD dwFileSizeHigh;
		U64 qwFileSize = ::GetFileSize(pDataFile, &dwFileSizeHigh);
		qwFileSize += (((U64)dwFileSizeHigh) << 32);

		HANDLE hFileMap = ::CreateFileMapping(pDataFile, NULL, PAGE_READONLY, 0, 0, NULL);

		U32 bufferSize = 128000; //按照12.8W个点进行块分拨和写入工作
		U32 dwOffset = 0, dwMapLen = 0;
		U64 qwFileOffset = 0, qwMapAddr = 0;
		while(qwFileSize > 0)
		{
			if(qwFileSize < dwDataLen)
			{
				dwDataLen = (DWORD)qwFileSize;
			}

			qwMapAddr = qwFileOffset / dwAllocGran * dwAllocGran;
			dwOffset = qwFileOffset - qwMapAddr;
			dwMapLen = dwDataLen + dwOffset;

			char* pData = (char*)::MapViewOfFile(hFileMap, FILE_MAP_READ, (DWORD)(qwMapAddr >> 32), (DWORD)(qwMapAddr & 0xFFFFFFFF), dwMapLen);
			PointXYZIPRGBA* pPtData = (PointXYZIPRGBA*)(pData + dwOffset);
			U32 nPtNum = dwDataLen / sizeof(PointXYZIPRGBA);
			while(nPtNum > 0)
			{
				vector<SplitMemoryBuf> parcelPoints;
				parcelPoints.resize(8);

				U32 num = nPtNum > bufferSize ? bufferSize : nPtNum;
				for (U32 i=0; i < num; i++)
				{
					// 该点所在的包序号
					U8 parcelIdx = 0;
					getPtBlockIndex(*pPtData, center, parcelIdx);

					SplitMemoryBuf& memPoints = *(parcelPoints._Myfirst() + parcelIdx);
					if (memPoints.m_count >= memPoints.m_vecBuf.size())
					{
						memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
					}

					PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
					memPoints.m_count++;
					pPtMem = pPtData;
					++pPtData;
				}

				char parcelFilePath[512] = {0};	
				// 1.将内存中的点一次写入到包文件中; 2.统计包文件信息;
				for (I32 i = 0;i < parcelPoints.size();i++)
				{			
					SplitMemoryBuf& pacPoints = *(parcelPoints._Myfirst() + i);
					int nCount = pacPoints.m_count;
					if (nCount <= 0)
					{
						continue;
					}

					sprintf_s(parcelFilePath, "%s\\parcel_%d.tmp", savePath, i);

					HANDLE pParcelDataFile = CreateFile(parcelFilePath, 
						GENERIC_READ | GENERIC_WRITE,
						FILE_SHARE_READ|FILE_SHARE_WRITE, 
						NULL,
						OPEN_ALWAYS, 
						FILE_ATTRIBUTE_NORMAL, 
						NULL);

					if (pParcelDataFile == INVALID_HANDLE_VALUE)
					{
						continue;
					}

					LARGE_INTEGER li;
					li.HighPart = 0;
					li.LowPart = 0;

					LARGE_INTEGER li1;
					li1.HighPart = 0;
					li1.LowPart = 0;

					SetFilePointerEx(pParcelDataFile,li,&li1,FILE_END);

					int buffcnt = 0;
					int k = 0;

					for (I32 j= 0; j < nCount;j++)
					{
						PointXYZIPRGBA* pPt = *(pacPoints.m_vecBuf._Myfirst() + j);
						DWORD reslt;

						*(pBuffer + k) = *pPt;
						k++;
						if (k >= BUFFER_SIZE)
						{
							WriteFile (pParcelDataFile, pBuffer, sizeof(PointXYZIPRGBA)*BUFFER_SIZE, &reslt, NULL);
							buffcnt++;
							k = 0;
						}		
					}

					if ( k > 0 )
					{
						DWORD dwResult;
						WriteFile (pParcelDataFile, pBuffer, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);
					}

					// 关闭块文件
					if (pParcelDataFile!=INVALID_HANDLE_VALUE)
					{
						CloseHandle(pParcelDataFile);
						pParcelDataFile = NULL;
					}

					// 收集包文件信息
					if (!mapSplitParcels.count(i))
					{
						ParcelFileInfo info;
						getBlockBox(center,halfSize,info.box,(U16)i);
						info.path = parcelFilePath;
						info.numPoint = nCount;
						mapSplitParcels.insert(make_pair(i, info));
					}
					else
					{
						ParcelFileInfo& parcelInfo = mapSplitParcels[i];
						parcelInfo.numPoint += nCount;
					}
				}//for (I32 i = 0;i < parcelPoints.size();i++)

				nPtNum -= num;
			}

			::UnmapViewOfFile(pData);
			qwFileOffset += dwDataLen;
			qwFileSize -= dwDataLen;
		}

		if(pBuffer != NULL)
		{
			delete[] pBuffer;
			pBuffer = NULL;
		}

		::CloseHandle(hFileMap);
		::CloseHandle(pDataFile);

		//while (m_pHlzWrite->ReadPtsBuffer(pDataFile, bufferPts) > 0)
		//{
		//	// 切分生成的包文件的点
		//	vector<SplitMemoryBuf> parcelPoints;
		//	parcelPoints.resize(8);
		//	for (I32 i=0; i < bufferPts.size(); i++)
		//	{
		//		PointXYZIPRGBA* ptTmp = NULL;				
		//		ptTmp = (bufferPts._Myfirst() + i);
		//		
		//		// 该点所在的包序号
		//		U8 parcelIdx = 0;
		//		getPtBlockIndex(*ptTmp, center, parcelIdx);
		//		
		//		SplitMemoryBuf& memPoints = *(parcelPoints._Myfirst() + parcelIdx);
		//		if (memPoints.m_count >= memPoints.m_vecBuf.size())
		//		{
		//			memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
		//		}

		//		PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
		//		memPoints.m_count++;
		//		pPtMem = bufferPts._Myfirst() + i;
		//		
		//	}//for (I32 i=0; i < bufferPts.size(); i++)

		//	char parcelFilePath[512] = {0};	
		//	// 1.将内存中的点一次写入到包文件中; 2.统计包文件信息;
		//	for (I32 i = 0;i < parcelPoints.size();i++)
		//	{			
		//		SplitMemoryBuf& pacPoints = *(parcelPoints._Myfirst() + i);
		//		int nCount = pacPoints.m_count;
		//		if (nCount <= 0)
		//		{
		//			continue;
		//		}

		//		sprintf_s(parcelFilePath, "%s\\parcel_%d.tmp", savePath, i);
		//		

		//		HANDLE pParcelDataFile = CreateFile(parcelFilePath, 
		//			GENERIC_READ | GENERIC_WRITE,
		//			FILE_SHARE_READ|FILE_SHARE_WRITE, 
		//			NULL,
		//			OPEN_ALWAYS, 
		//			FILE_ATTRIBUTE_NORMAL, 
		//			NULL);

		//		if (pParcelDataFile == INVALID_HANDLE_VALUE)
		//		{
		//			continue;
		//		}

		//		LARGE_INTEGER li;
		//		li.HighPart = 0;
		//		li.LowPart = 0;

		//		LARGE_INTEGER li1;
		//		li1.HighPart = 0;
		//		li1.LowPart = 0;

		//		SetFilePointerEx(pParcelDataFile,li,&li1,FILE_END);

		//		int buffcnt = 0;
		//		int k = 0;

		//		/*// 临时buffer用于写文件				
		//		PointXYZIPRGBA* pBuffer = new PointXYZIPRGBA[BUFFER_SIZE];*/

		//		for (I32 j= 0; j < nCount;j++)
		//		{
		//			PointXYZIPRGBA* pPt = *(pacPoints.m_vecBuf._Myfirst() + j);
		//			DWORD reslt;
		//			
		//			*(pBuffer + k) = *pPt;
		//			k++;
		//			if (k >= BUFFER_SIZE)
		//			{
		//				WriteFile (pParcelDataFile, pBuffer, sizeof(PointXYZIPRGBA)*BUFFER_SIZE, &reslt, NULL);
		//				buffcnt++;
		//				k = 0;
		//				
		//				/*delete [] pBuffer;
		//				pBuffer = new PointXYZIPRGBA[BUFFER_SIZE];*/
		//			}		
		//		}

		//		if ( k > 0 )
		//		{
		//			DWORD dwResult;
		//			WriteFile (pParcelDataFile, pBuffer, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);
		//		}

		//		//delete []pBuffer;

		//		// 关闭块文件
		//		if (pParcelDataFile!=INVALID_HANDLE_VALUE)
		//		{
		//			CloseHandle(pParcelDataFile);
		//			pParcelDataFile = NULL;
		//		}

		//		// 收集包文件信息
		//		if (!mapSplitParcels.count(i))
		//		{
		//			ParcelFileInfo info;
		//			getBlockBox(center,halfSize,info.box,(U16)i);
		//			info.path = parcelFilePath;
		//			info.numPoint = nCount;
		//			mapSplitParcels.insert(make_pair(i, info));
		//		}
		//		else
		//		{
		//			ParcelFileInfo& parcelInfo = mapSplitParcels[i];
		//			parcelInfo.numPoint += nCount;
		//		}
		//	}//for (I32 i = 0;i < parcelPoints.size();i++)

		//	bufferPts.clear();
		//}

		//if(pBuffer != NULL)
		//{
		//	delete[] pBuffer;
		//	pBuffer = NULL;
		//}

		//// 关闭块文件
		//if (pDataFile!=INVALID_HANDLE_VALUE)
		//{
		//	CloseHandle(pDataFile);
		//	pDataFile = NULL;
		//}

		// 包文件写完之后，干掉块文件节省磁盘空间
		int err = remove(strBlockFile);
		if (err == -1)
		{
	//		perror("无法删除BlockFile--SplitParcelFiles");
		}

		// 对包文件进行递归切分
		for (std::map<U8, ParcelFileInfo>::iterator it = mapSplitParcels.begin();
			it != mapSplitParcels.end(); it++)
		{
			if ((it->second).numPoint <= BLOCK_HOLD)
			{
				// 包文件不需要再往下切分
				arrayParcels.push_back(it->second);
			}
			else
			{
				std::string dir = (it->second).path.c_str();
				dir = dir.substr(0, dir.find_last_of('.'));
				I32 ret = _mkdir(dir.c_str());

				char splitPath[MAX_PATH] = {0};
				sprintf_s(splitPath,"%s", dir.c_str());

				// 继续往下切分
				SplitParcelFiles((it->second).box, (it->second).path.c_str(), splitPath, arrayParcels);
			}
		}

		return true;
	}

	// 根据块集划分块文件
	bool CHlzBuilder::SplitBlockFiles(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks)//
	{
		HANDLE pDataFile = CreateFile(procBlockSet.path.c_str(), 
			GENERIC_READ,
			FILE_SHARE_READ, 
			NULL,
			OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, 
			NULL);

		if (pDataFile == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}
		//fseek(pFile, 0, SEEK_SET);// 移到文件头

		// 创建存储目录
		std::string dir = procBlockSet.path;
		dir = dir.substr(0, dir.find_last_of('.'));
		I32 ret = _mkdir(dir.c_str());

		// 一次读取6.4w点到内存后写入到文件
		//std::vector<PointXYZIPRGBA> bufferPts;

		//PointXYZIPRGBA* ptTmp = NULL;
		// 当前点所在的块序号

		CHdVector3df center = procBlockSet.box.getCenter();
		CHdVector3df halfSize = procBlockSet.box.getExtent() /2.0f;
		arrayBlocks.resize(8);

		//内存缓冲区重复利用，减少new/delete开销。   袁亮  20160909
		PointXYZIPRGBA* pBuffer = new PointXYZIPRGBA[BUFFER_SIZE];

		//使用内存映射文件读取块集中的点数据   袁亮   20160909
		SYSTEM_INFO sinf;
		::GetSystemInfo(&sinf);
        DWORD dwAllocGran = sinf.dwAllocationGranularity;
		//每次映射加载500W个点(5000000*18/1024/1024 = 85.8M)
		DWORD dwDataLen = 5000000 * sizeof(PointXYZIPRGBA);

		DWORD dwFileSizeHigh;
		U64 qwFileSize = ::GetFileSize(pDataFile, &dwFileSizeHigh);
		qwFileSize += (((U64)dwFileSizeHigh) << 32);

		HANDLE hFileMap = ::CreateFileMapping(pDataFile, NULL, PAGE_READONLY, 0, 0, NULL);
	
		U32 bufferSize = 128000; //按照12.8W个点进行块分拨和写入工作
		U32 dwOffset = 0, dwMapLen = 0;
		U64 qwFileOffset = 0, qwMapAddr = 0;
		while(qwFileSize > 0)
		{
			if(qwFileSize < dwDataLen)
			{
				dwDataLen = (DWORD)qwFileSize;
			}

			qwMapAddr = qwFileOffset / dwAllocGran * dwAllocGran;
			dwOffset = qwFileOffset - qwMapAddr;
			dwMapLen = dwDataLen + dwOffset;

			char* pData = (char*)::MapViewOfFile(hFileMap, FILE_MAP_READ, (DWORD)(qwMapAddr >> 32), (DWORD)(qwMapAddr & 0xFFFFFFFF), dwMapLen);
			PointXYZIPRGBA* pPtData = (PointXYZIPRGBA*)(pData + dwOffset);
			U32 nPtNum = dwDataLen / sizeof(PointXYZIPRGBA);
			while(nPtNum > 0)
			{
				vector<SplitMemoryBuf> blockPoints;
				blockPoints.resize(8);

				U32 num = nPtNum > bufferSize ? bufferSize : nPtNum;
				for (U32 i=0; i < num; i++)
				{
					U8 blockIdx = 0;			
					getPtBlockIndex(*pPtData, center, blockIdx);

					SplitMemoryBuf& memPoints = *(blockPoints._Myfirst() + blockIdx);

					if (memPoints.m_count >= memPoints.m_vecBuf.size())
					{
						memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
					}

					PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
					memPoints.m_count++;
					pPtMem = pPtData;
					++pPtData;
				}

				// 将内存中的点一次写入到块文件中
				for (I32 i = 0;i < 8;i++)
				{
					SplitMemoryBuf& blkPoints = *(blockPoints._Myfirst() + i);
					int nCount = blkPoints.m_count;
					if(nCount <= 0)
						continue;
					char blockFilePath[512] = {0};
					sprintf_s(blockFilePath, "%s\\block_%d.tmp", dir.c_str(), i);

					HANDLE pBlckDataFile = CreateFile(blockFilePath, 
						GENERIC_READ | GENERIC_WRITE,
						FILE_SHARE_READ|FILE_SHARE_WRITE, 
						NULL,
						OPEN_ALWAYS, 
						FILE_ATTRIBUTE_NORMAL, 
						NULL);

					if (pBlckDataFile == INVALID_HANDLE_VALUE)
					{
						continue;
					}

					LARGE_INTEGER li;
					li.HighPart = 0;
					li.LowPart = 0;

					LARGE_INTEGER li1;
					li1.HighPart = 0;
					li1.LowPart = 0;

					SetFilePointerEx(pBlckDataFile,li,&li1,FILE_END);

					BlockFileInfo& blkInfo = *(arrayBlocks._Myfirst() + i);
					blkInfo.path = blockFilePath;
					blkInfo.numPoint += nCount;
					blkInfo.index = i;
					getBlockBox(center,halfSize,blkInfo.box,(U16)i);

					int buffcnt = 0;
					int k = 0;

					for (I32 j = 0; j < nCount; j++)
					{
						PointXYZIPRGBA* pPt = *(blkPoints.m_vecBuf._Myfirst() +j );
						DWORD dwResult;
						*(pBuffer + k) = *pPt;
						k++;
						if (k >= BUFFER_SIZE )
						{
							WriteFile (pBlckDataFile, pBuffer, sizeof(PointXYZIPRGBA)*BUFFER_SIZE, &dwResult, NULL);
							buffcnt++;
							k = 0;
						}			
					}

					if (k > 0)
					{
						DWORD dwResult;					
						WriteFile (pBlckDataFile, pBuffer, sizeof(PointXYZIPRGBA)*k, &dwResult, NULL);					
					}

					if (pBlckDataFile != INVALID_HANDLE_VALUE)
					{
						CloseHandle(pBlckDataFile);
						pBlckDataFile = NULL;
					}

				}

				nPtNum -= num;
			}

			::UnmapViewOfFile(pData);
			qwFileOffset += dwDataLen;
			qwFileSize -= dwDataLen;
		}

		if(pBuffer != NULL)
		{
			delete[] pBuffer;
			pBuffer = NULL;
		}

		::CloseHandle(hFileMap);
		::CloseHandle(pDataFile);

		return true;

		///*while (m_pHlzWrite->ReadPtsBuffer(pDataFile, bufferPts) > 0)
		//{

		//	// 切分生成的块文件对应的点
		//	vector<SplitMemoryBuf> blockPoints;
		//	blockPoints.resize(8);
		//	for (I32 i=0; i < bufferPts.size(); i++)
		//	{
		//		ptTmp = &(*(bufferPts._Myfirst() + i));
		//		
		//		U8 blockIdx = 0;			
		//		getPtBlockIndex(*ptTmp, center, blockIdx);
		//		
		//		SplitMemoryBuf& memPoints = *(blockPoints._Myfirst() + blockIdx);

		//		if (memPoints.m_count >= memPoints.m_vecBuf.size())
		//		{
		//			memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
		//		}

		//		PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
		//		memPoints.m_count++;
		//		pPtMem = bufferPts._Myfirst() + i;
		//		
		//	}//for (I32 i=0; i < bufferPts.size(); i++)

		//	// 将内存中的点一次写入到块文件中
		//	for (I32 i = 0;i < 8;i++)
		//	{
		//		SplitMemoryBuf& blkPoints = *(blockPoints._Myfirst() + i);
		//		int nCount = blkPoints.m_count;
		//		if(nCount <= 0)
		//			continue;
		//		char blockFilePath[512] = {0};
		//		sprintf_s(blockFilePath, "%s\\block_%d.tmp", dir.c_str(), i);
		//		
		//		HANDLE pBlckDataFile = CreateFile(blockFilePath, 
		//			GENERIC_READ | GENERIC_WRITE,
		//			FILE_SHARE_READ|FILE_SHARE_WRITE, 
		//			NULL,
		//			OPEN_ALWAYS, 
		//			FILE_ATTRIBUTE_NORMAL, 
		//			NULL);

		//		if (pBlckDataFile == INVALID_HANDLE_VALUE)
		//		{
		//			continue;
		//		}
		//		
		//		LARGE_INTEGER li;
		//		li.HighPart = 0;
		//		li.LowPart = 0;

		//		LARGE_INTEGER li1;
		//		li1.HighPart = 0;
		//		li1.LowPart = 0;

		//		SetFilePointerEx(pBlckDataFile,li,&li1,FILE_END);

		//		BlockFileInfo& blkInfo = *(arrayBlocks._Myfirst() + i);
		//		blkInfo.path = blockFilePath;
		//		blkInfo.numPoint += nCount;
		//		blkInfo.index = i;
		//		getBlockBox(center,halfSize,blkInfo.box,(U16)i);

		//		// 临时buffer用于写文件
		//		PointXYZIPRGBA* pBuffer = new PointXYZIPRGBA[BUFFER_SIZE];

		//		int buffcnt = 0;
		//		int k = 0;

		//		for (I32 j = 0; j < nCount; j++)
		//		{
		//			PointXYZIPRGBA* pPt = *(blkPoints.m_vecBuf._Myfirst() +j );
		//			DWORD dwResult;
		//			*(pBuffer + k) = *pPt;
		//			k++;
		//			if (k >= BUFFER_SIZE )
		//			{
		//				WriteFile (pBlckDataFile, pBuffer, sizeof(PointXYZIPRGBA)*BUFFER_SIZE, &dwResult, NULL);
		//				buffcnt++;
		//				k = 0;
		//				/*delete [] pBuffer;						
		//				pBuffer = new PointXYZIPRGBA[BUFFER_SIZE];*/
		//			}			
		//		}

		//		//int leftnum = nCount - buffcnt*BUFFER_SIZE;
		//		if (k > 0)
		//		{
		//			DWORD dwResult;					
		//			WriteFile (pBlckDataFile, pBuffer, sizeof(PointXYZIPRGBA)*k, &dwResult, NULL);					
		//			/*delete []pBuffer;
		//			pBuffer = NULL;*/
		//		}
		//		
		//		if (pBlckDataFile != INVALID_HANDLE_VALUE)
		//		{
		//			CloseHandle(pBlckDataFile);
		//			pBlckDataFile = NULL;
		//		}

		//	}//for (i = 0;i < 8;i++)将内存中的点一次写入到块文件中

		//	bufferPts.clear();
		//}//while (m_pHlzWrite->ReadPtsBuffer(pFile, bufferPts) > 0)

		//if(pBuffer != NULL)
		//{
		//	delete[] pBuffer;
		//	pBuffer = NULL;
		//}

		//if (pDataFile != INVALID_HANDLE_VALUE)
		//{
		//	CloseHandle(pDataFile);
		//	pDataFile = NULL;
		//}

		//return true;
	}

	// 划分块集文件
	bool CHlzBuilder::SplitBlockSetFiles(const char* savePath, hd::stringc& strHlsFile)
	{
		// 计算一次重采用精度的反转
		double dAntiStep = 1.0 / m_dGridPrecision;

		// 使用vector记录
		std::vector<PosKey> vecPosKeys;
		std::vector<int> vecTargetIndex;
		if (m_bResampleZero)
		{
			vecPosKeys.resize(2000000);
			vecTargetIndex.resize(vecPosKeys.size());
		}

		CHLSReadOpener hlsOpen;
		IHLSReader* pHlsReader = hlsOpen.Open(strHlsFile.c_str());
		if (pHlsReader == NULL)
		{
			return false;
		}

		hd::stringc strInfo = HDSCENE_IDS_PROCESS_CONVERTING1;
		int nPos = strHlsFile.findLast('\\');
		strInfo += strHlsFile.subString(nPos + 1, strHlsFile.size() - nPos - 1);

		// 进度条开始
		if (processCallback)
		{
			processCallback(0.0f, strInfo.c_str());
		}

		CHdLevel* pHdLevel = m_pCoreData->GetLevelRec(0);
		if(pHdLevel == NULL)
		{
			pHdLevel = new CHdLevel;
			pHdLevel->m_levelNo = 0;
			m_pCoreData->AddLevelRec(pHdLevel);
		}
		// 构造全局坐标转换矩阵
		double m[16];
		pHlsReader->m_header.computeMatrix(m);

		// 获取圈数
		int loopCount = pHlsReader->GetLoopCount();

		// 获取圈索引
		const CLoopIndex* pLoopIdx = pHlsReader->GetLoopIndex(); 

		// 创建第0层临时文件目录
		char dir[MAX_PATH] = {0};
		sprintf_s(dir, "%s\\level0", savePath);
		I32 ret = _mkdir(dir);

		char filePath[MAX_PATH] = {0};

		// 读取1000圈点云后一次写入到块集文件
		U32 nLoopSize = 200;
		U32 nCurLp = 0;
		//U32 selNum = 0;

		while((loopCount - nCurLp) > 0)
		{
			// 内存中的1000圈点云
			hdBlkArray<PointXYZIPRGBA> loopPtsArray;
			if ((loopCount - nCurLp) < nLoopSize)
			{
				nLoopSize = (loopCount - nCurLp);
			}
			loopPtsArray.setBlockCount(nLoopSize);

			// 从文件中读满loopPtsArray
			for (U32 n = 0; n < nLoopSize; n++)
			{
				hdVector<PointXYZIPRGBA>& vecPts = loopPtsArray.getBlock(n);
				if(!pHlsReader->ReadLoop(vecPts, n+nCurLp))
					continue;

			}

			// 更新当前圈
			nCurLp += nLoopSize;

			std::map<U32, SplitMemoryBuf> BsPointsArray;

			for (I32 i = 0; i < nLoopSize; i++)
			{
				hdVector<PointXYZIPRGBA>& vecPts = loopPtsArray.getBlock(i);

				for (I32 k = 0;k < vecPts.size();k++)
				{
					PointXYZIPRGBA& pt = vecPts[k];
					if(!pt.isValid())
						continue;

					// 转为全局坐标
					double x = pt.x;
					double y = pt.y;
					double z = pt.z;
					hdHomogeneousTransformPoint(m,x,y,z);

					// 坐标偏移
					pt.x = (hd::f32)(x - m_fullExtent.MinEdge.X);
					pt.y = (hd::f32)(y - m_fullExtent.MinEdge.Y);
					pt.z = (hd::f32)(z - m_fullExtent.MinEdge.Z);

					// 计算块集编号
					/*U32 xNo = (U32)((pt.x) / m_stepX);
					U32 yNo = (U32)((pt.y) / m_stepY);*/
					//根据当前点在全局网格中编号，结合当前空间范围的网格划分范围，计算当前块集的编号  袁亮  20160622
					I32 xNo = (I32)floor(x / m_stepX);
					I32 yNo = (I32)floor(y / m_stepY);
					xNo = xNo - m_xNoFrom;
					yNo = yNo - m_yNoFrom;
					U32 fileNo = pHdLevel->m_level.numBlocksetX*yNo + xNo;

					SplitMemoryBuf& memPoints = BsPointsArray[fileNo];

					if (memPoints.m_count >= memPoints.m_vecBuf.size())
					{
						memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
					}

					PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
					memPoints.m_count++;
					pPtMem = &pt;					
					
					// 统计x分布
					U32 selNum = (U32)((x - m_fullExtent.MinEdge.X) * m_coordStats.xRcp);
					selNum = clamp(selNum,(U32)0,(U32)999);
					m_coordStats.xStepStat[selNum]++;

					// 统计y分布
					selNum = (U32)((y - m_fullExtent.MinEdge.Y) * m_coordStats.yRcp);
					selNum = clamp(selNum,(U32)0,(U32)999);
					m_coordStats.yStepStat[selNum]++;

					// 统计z分布
					selNum = (U32)((z - m_fullExtent.MinEdge.Z) * m_coordStats.zRcp);
					selNum = clamp(selNum,(U32)0,(U32)999);
					m_coordStats.zStepStat[selNum]++;

					// 统计强度分布
					selNum = (U32)((pt.getIntensity() - m_iMin) * m_coordStats.iRcp);
					selNum = clamp(selNum,(U32)0,(U32)999);
					m_coordStats.intStepStat[selNum]++;
					m_ptNum++;
				}//for (I32 k = 0;k < vecPts.size();k++)
			}//for (I32 i=0; i < nLoopSize; i++)

			// 将内存中的点写入到块集文件中
			for (std::map<U32, SplitMemoryBuf>::iterator it = BsPointsArray.begin(); 
				it != BsPointsArray.end(); it++)
			{
				SplitMemoryBuf& bsPoints = (it->second);
				U32 nCount = bsPoints.m_count;
				if (nCount <= 0)
				{
					continue;
				}

				// 更新包围盒的Z值
				CHdBox3df& blocksetBox = m_BlocksetFiles[it->first].box;

				sprintf(filePath,"%s\\level0\\blockset_%04d.tmp",savePath,(it->first));
				
				HANDLE pBlstDataFile = CreateFile(filePath, 
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE, 
					NULL,
					OPEN_ALWAYS, 
					FILE_ATTRIBUTE_NORMAL, 
					NULL);

				if (pBlstDataFile == INVALID_HANDLE_VALUE)
				{
					continue;
				}

				// 定义重采样结果值记录
				I32 simpleSize = 0;

				// 若需要重采样0层，在此处进行采样（由于采用部分数据划分格网方式，采样并不完全均匀）
				if (m_bResampleZero)
				{
#pragma region 零层重采样
					// 可能初始resize大小不够，在此处进行一次判定处理
					if (nCount > vecPosKeys.size())
					{
						vecPosKeys.resize(nCount);
						vecTargetIndex.resize(nCount);
					}

					// 定义单次读取范围box
					double dMinX,dMinY,dMinZ,dMaxX,dMaxY,dMaxZ;
					dMinX = dMinY = dMinZ = F32_MAX;
					dMaxX = dMaxY = dMaxZ = F32_MIN;

					// 定义中间变量
					double dCenterX = 0.0;
					double dCenterY = 0.0;
					double dCenterZ = 0.0;

					// 确定格网范围
					for (unsigned int n = 0;n < nCount;n++)
					{
						//PointXYZIPRGBA& pts = *(bufferPts._Myfirst() + n);
						PointXYZIPRGBA* pts = *(bsPoints.m_vecBuf._Myfirst() + n);
						dMinX = MIN(dMinX,pts->x);
						dMinY = MIN(dMinY,pts->y);
						dMinZ = MIN(dMinZ,pts->z);
						dMaxX = MAX(dMaxX,pts->x);
						dMaxY = MAX(dMaxY,pts->y);
						dMaxZ = MAX(dMaxZ,pts->z);
					}

					// 根据范围及精度确定格网个数
					int xRows = ceil((dMaxX - dMinX) * dAntiStep);
					int yRows = ceil((dMaxY - dMinY) * dAntiStep);
					int zRows = ceil((dMaxZ - dMinZ) * dAntiStep);

					// 定义中间变量
					int xStep = 0;
					int yStep = 0;
					int zStep = 0;
					int nTmp = 0;
					simpleSize = 0;

					// 格网划分处理
					for (unsigned int n = 0;n < nCount;n++)
					{
						PointXYZIPRGBA* pts = *(bsPoints.m_vecBuf._Myfirst() + n);
						//PointXYZIPRGBA& pts = *(bufferPts._Myfirst() + n);

						// 计算对应格网索引值
						xStep = floor((pts->x - dMinX) * dAntiStep);
						yStep = floor((pts->y - dMinY) * dAntiStep);
						zStep = floor((pts->z - dMinZ) * dAntiStep);

						double dTmpX,dTmpY,dTmpZ;
						dTmpX = dMinX + (xStep +  0.5) * m_dGridPrecision - pts->x;
						dTmpY = dMinY + (yStep +  0.5) * m_dGridPrecision - pts->y;
						dTmpZ = dMinZ + (zStep +  0.5) * m_dGridPrecision - pts->z;

						// 对应键为
						U64 mapPin = zStep * (xRows * yRows) + yStep * xRows + xStep;

						// 插入值
						PosKey posKey;
						posKey.nIndex = n;

						// 计算获取距离box中心最近点
						posKey.dDist = pow(dTmpX,2.0) + pow(dTmpY,2.0) + pow(dTmpZ,2.0);
						posKey.gridIndex = mapPin;
						*(vecPosKeys._Myfirst() + n) = posKey;
					}

					// 针对vec进行处理排序
					std::sort(vecPosKeys.begin(),vecPosKeys.begin() + nCount,SortByGrid);

					// 定义中间变量
					PosKey& pos = *(vecPosKeys._Myfirst() + 0);
					U64 nCurGridPos = pos.gridIndex;
					*(vecTargetIndex._Myfirst() + 0) = pos.nIndex;
					int nCurIndex = 1;

					// 根据之前记录的距离值，grid相同的比较距离，从小到大排序
					for (unsigned int nn = 1;nn < nCount;nn++)
					{
						PosKey& pos = *(vecPosKeys._Myfirst() + nn);
						if (pos.gridIndex == nCurGridPos)
						{
							// 之前已按距离排序，同一格网距离中心最小为最前索引值
							continue;
						}
						else
						{
							// 处理到下一个格网时，记录
							nCurGridPos = pos.gridIndex;
							*(vecTargetIndex._Myfirst() + nCurIndex) = pos.nIndex;
							nCurIndex++;
						}
					}

					// 抽取的点数赋值
					simpleSize = nCurIndex;
#pragma endregion
				}

				LARGE_INTEGER li;
				li.HighPart = 0;
				li.LowPart = 0;

				LARGE_INTEGER li1;
				li1.HighPart = 0;
				li1.LowPart = 0;

				// 文件指针移动
				SetFilePointerEx(pBlstDataFile,li,&li1,FILE_END);

				// 申请buf内存
				PointXYZIPRGBA* pBuffer = new PointXYZIPRGBA [BUFFER_SIZE];

				int buffcnt = 0;
				int k = 0;

				if (m_bResampleZero)
				{
					// 条件写入
					for (U32 m = 0; m < simpleSize; m++)
					{
						int nTmpIndex = *(vecTargetIndex._Myfirst() + m);
						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + nTmpIndex);
						DWORD dwResult;
						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

						*(pBuffer + k) = *pPt;
						k++;

						if (k >= BUFFER_SIZE)
						{
							WriteFile (pBlstDataFile, pBuffer, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);

							buffcnt++;
							k = 0;
							//delete [] pBuffer;
							//pBuffer = new PointXYZIPRGBA [BUFFER_SIZE];
						}
					}
				}
				else
				{
					// 条件写入
					for (U32 m = 0; m < nCount; m++)
					{
						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + m);
						DWORD dwResult;
						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

						*(pBuffer + k) = *pPt;
						k++;

						if (k >= BUFFER_SIZE)
						{
							WriteFile (pBlstDataFile, pBuffer, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);

							buffcnt++;
							k = 0;
							//delete [] pBuffer;
							//pBuffer = new PointXYZIPRGBA [BUFFER_SIZE];
						}
					}
				}

				//// 条件写入
				//for (U32 m=0; m < nCount; m++)
				//{
				//	PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + m);
				//	DWORD dwResult;
				//	blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
				//	blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

				//	*(pBuffer + k) = *pPt;
				//	k++;
				//	
				//	if (k >= BUFFER_SIZE)
				//	{
				//		WriteFile (pBlstDataFile, pBuffer, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);

				//		buffcnt++;
				//		k = 0;
				//		//delete [] pBuffer;
				//		//pBuffer = new PointXYZIPRGBA [BUFFER_SIZE];
				//	}
				//}

				if (k > 0)
				{
					DWORD dwResult;
					WriteFile (pBlstDataFile, pBuffer, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);
				}

				// 更新文件名
				m_BlocksetFiles[it->first].path = filePath;
				//更新块集中点数目  袁亮   20160909
				m_BlocksetFiles[it->first].numPoint += m_bResampleZero ? simpleSize : nCount;

				if (pBuffer)
				{
					delete [] pBuffer;
					pBuffer = NULL;
				}

				if (pBlstDataFile!=INVALID_HANDLE_VALUE)
				{
					CloseHandle(pBlstDataFile);
					pBlstDataFile = NULL;
				}
			}//for (std::map<U32, SplitMemoryBuf>::iterator it = BsPointsArray.begin(); 

			if (processCallback)
			{			
				processCallback((float)nCurLp / loopCount, HDSCENE_IDS_CUTTING);
			}
		}


		// 计算块集的包围盒XY值和参考点
		for (map<U32, BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
			it != m_BlocksetFiles.end(); it++)
		{
			U32 BsIndex = it->first;

			// 计算块集编号
			I32 yNo = BsIndex / pHdLevel->m_level.numBlocksetX;
			I32 xNo = BsIndex - (yNo * pHdLevel->m_level.numBlocksetX);

			BlockSetFileInfo& BsInfo = it->second;
			/*BsInfo.box.MinEdge.X = xNo * m_stepX;
			BsInfo.box.MinEdge.Y = yNo * m_stepY;

			BsInfo.box.MaxEdge.X = (xNo+1) * m_stepX;
			BsInfo.box.MaxEdge.Y = (yNo+1) * m_stepY;*/
			//全局网格坐标转换为相对左下角的偏移坐标   袁亮   20160622
			BsInfo.box.MinEdge.X = (xNo+m_xNoFrom)*m_stepX - m_fullExtent.MinEdge.X;
			BsInfo.box.MinEdge.Y = (yNo+m_yNoFrom)*m_stepY - m_fullExtent.MinEdge.Y;
			BsInfo.box.MaxEdge.X = BsInfo.box.MinEdge.X + m_stepX;
			BsInfo.box.MaxEdge.Y = BsInfo.box.MinEdge.Y + m_stepY;
			

			if(DEBUG_LOG)
			{
				char logmsg[512] = {0};
				sprintf_s(logmsg,"level:%d,%d,%d,%d\n",pHdLevel->m_levelNo,BsIndex,xNo,yNo);
				m_pHlzWrite->WriteLogFile(logmsg);			
			}
		}

		if (pHlsReader != NULL)
		{
			delete pHlsReader;
			pHlsReader = NULL;
		}

		///----- 
		//printf("总点数为:-----%llu\n",m_ptNum);

		//getchar();

		return true;
	}

	//! 切割Las到块集文件
	bool CHlzBuilder::SplitBlockSetLas(const char* savePath,hd::stringc& strLasFile)
	{
		// 计算一次重采用精度的反转
		double dAntiStep = 1.0 / m_dGridPrecision;

		// 使用vector记录
		std::vector<PosKey> vecPosKeys;
		std::vector<int> vecTargetIndex;
		if (m_bResampleZero)
		{
			vecPosKeys.resize(2000000);
			vecTargetIndex.resize(vecPosKeys.size());
		}

		// las文件打开对象
		LASreadOpener lasreadopener;

		// 设置不合并
		lasreadopener.set_merged(FALSE);
		lasreadopener.set_populate_header(FALSE);

		// 设置文件名
		lasreadopener.set_file_name(strLasFile.c_str());
		if (!lasreadopener.active())
		{
			return false;
		}
		// 打开las文件
		LASreader* lasRead = lasreadopener.open();
		// 打开失败、返回
		if (lasRead == NULL)
		{
			return false;
		}

		I64 npoint = lasRead->npoints;
		hd::stringc strInfo = HDSCENE_IDS_PROCESS_CONVERTING1;
		int nPos = strLasFile.findLast('\\');
		strInfo += strLasFile.subString(nPos + 1, strLasFile.size() - nPos - 1);

		// 进度条开始
		if (processCallback)
		{
			processCallback(0.0f, strInfo.c_str());
		}

		CHdLevel* pHdLevel = m_pCoreData->GetLevelRec(0);
		if(pHdLevel == NULL)
		{
			pHdLevel = new CHdLevel;
			pHdLevel->m_levelNo = 0;
			m_pCoreData->AddLevelRec(pHdLevel);
		}

		// 创建第0层临时文件目录
		char dir[MAX_PATH] = {0};
		sprintf_s(dir, "%s\\level0", savePath);
		I32 ret = _mkdir(dir);

		char filePath[MAX_PATH] = {0};

		// 读取500000点云后一次写入到块集文件
		U32 nBufferSize = 5000000;
		I64 nCurLp = 0;
		U32 selNum = 0;

		BOOL bRet = FALSE;
		BOOL bReadEnd = FALSE;

		while((npoint - nCurLp) >= 0)
		{
			hdVector<PointXYZIPRGBA> bufArray;
			bufArray.resize(nBufferSize);
			I32 i = 0;
			for(i = 0;i < nBufferSize;i++)
			{
				bRet = lasRead->read_point();
				if(!bRet)
				{
					bReadEnd = TRUE;
					break;
				}
				PointXYZIPRGBA& pt = bufArray[i];
				pt.x = lasRead->get_x() - m_fullExtent.MinEdge.X;
				pt.y = lasRead->get_y() - m_fullExtent.MinEdge.Y;
				pt.z = lasRead->get_z() - m_fullExtent.MinEdge.Z;
				pt.intensity = U16((lasRead->point.intensity / 65535.0f)* 4095);
				nCurLp++;
				if(nCurLp >= npoint)
				{
					break;
				}
				// rgb颜色信息
				if (lasRead->point.have_rgb)
				{
					if (lasRead->point.rgb[0] > 255 || 
						lasRead->point.rgb[1] > 255 ||
						lasRead->point.rgb[2] > 255 )
					{
						pt.r = int((lasRead->point.rgb[0] / 65535.0f)* 255);
						pt.g = int((lasRead->point.rgb[1] / 65535.0f)* 255);
						pt.b = int((lasRead->point.rgb[2] / 65535.0f)* 255);
					}
					else
					{
						pt.r = (u8)lasRead->point.rgb[0];
						pt.g = (u8)lasRead->point.rgb[1];
						pt.b = (u8)lasRead->point.rgb[2];
					}
				}
			}

			std::map<U32, SplitMemoryBuf> BsPointsArray;

			//#pragma omp parallel for
			for (I32 k=0; k < i; k++)
			{			
				PointXYZIPRGBA& pt = bufArray[k];
				// 计算块集编号
				/*U32 xNo = (U32)((pt.x) / m_stepX);
				U32 yNo = (U32)((pt.y) / m_stepY);*/
				//根据当前点在全局网格中编号，结合当前空间范围的网格划分范围，计算当前块集的编号  袁亮  20160622
				I32 xNo = (I32)floor((pt.x + m_fullExtent.MinEdge.X)/ m_stepX);
				I32 yNo = (I32)floor((pt.y + m_fullExtent.MinEdge.Y)/ m_stepY);
				xNo = xNo - m_xNoFrom;
				yNo = yNo - m_yNoFrom;
				U32 fileNo = pHdLevel->m_level.numBlocksetX*yNo + xNo;

				//#pragma omp critical
				{
					SplitMemoryBuf& memPoints = BsPointsArray[fileNo];

					if (memPoints.m_count >= memPoints.m_vecBuf.size())
					{
						memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
					}

					PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
					memPoints.m_count++;
					pPtMem = &pt;					
				}
				// 统计x分布
				selNum = (U32)(pt.x * m_coordStats.xRcp);
				selNum = clamp(selNum,(U32)0,(U32)999);
				m_coordStats.xStepStat[selNum]++;

				// 统计y分布
				selNum = (U32)(pt.y * m_coordStats.yRcp);
				selNum = clamp(selNum,(U32)0,(U32)999);
				m_coordStats.yStepStat[selNum]++;

				// 统计z分布
				selNum = (U32)(pt.z * m_coordStats.zRcp);
				selNum = clamp(selNum,(U32)0,(U32)999);
				m_coordStats.zStepStat[selNum]++;

				// 统计强度分布
				selNum = (U32)((pt.getIntensity() - m_iMin) * m_coordStats.iRcp);
				selNum = clamp(selNum,(U32)0,(U32)999);
				m_coordStats.intStepStat[selNum]++;
				m_ptNum++;
			}//for (I32 k=0; k < i; k++)

			// 将内存中的点写入到块集文件中
			for (std::map<U32, SplitMemoryBuf>::iterator it = BsPointsArray.begin(); 
				it != BsPointsArray.end(); it++)
			{
				SplitMemoryBuf& bsPoints = (it->second);
				U32 nCount = bsPoints.m_count;
				if (nCount <= 0)
				{
					continue;
				}

				// 更新包围盒的Z值
				CHdBox3df& blocksetBox = m_BlocksetFiles[it->first].box;

				sprintf(filePath,"%s\\level0\\blockset_%04d.tmp",savePath,(it->first));

				HANDLE pBlstDataFile = CreateFile(filePath, 
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE, 
					NULL,
					OPEN_ALWAYS, 
					FILE_ATTRIBUTE_NORMAL, 
					NULL);

				if (pBlstDataFile == INVALID_HANDLE_VALUE)
				{
					continue;
				}

				// 定义重采样结果值记录
				U32 simpleSize = 0;

				// 若需要重采样0层，在此处进行采样（由于采用部分数据划分格网方式，采样并不完全均匀）
				if (m_bResampleZero)
				{
#pragma region 零层重采样
					// 可能初始resize大小不够，在此处进行一次判定处理
					if (nCount > vecPosKeys.size())
					{
						vecPosKeys.resize(nCount);
						vecTargetIndex.resize(nCount);
					}

					// 定义单次读取范围box
					double dMinX,dMinY,dMinZ,dMaxX,dMaxY,dMaxZ;
					dMinX = dMinY = dMinZ = F32_MAX;
					dMaxX = dMaxY = dMaxZ = F32_MIN;

					// 定义中间变量
					double dCenterX = 0.0;
					double dCenterY = 0.0;
					double dCenterZ = 0.0;

					// 确定格网范围
					for (unsigned int n = 0;n < nCount;n++)
					{
						//PointXYZIPRGBA& pts = *(bufferPts._Myfirst() + n);
						PointXYZIPRGBA* pts = *(bsPoints.m_vecBuf._Myfirst() + n);
						dMinX = MIN(dMinX,pts->x);
						dMinY = MIN(dMinY,pts->y);
						dMinZ = MIN(dMinZ,pts->z);
						dMaxX = MAX(dMaxX,pts->x);
						dMaxY = MAX(dMaxY,pts->y);
						dMaxZ = MAX(dMaxZ,pts->z);
					}

					// 根据范围及精度确定格网个数
					int xRows = ceil((dMaxX - dMinX) * dAntiStep);
					int yRows = ceil((dMaxY - dMinY) * dAntiStep);
					int zRows = ceil((dMaxZ - dMinZ) * dAntiStep);

					// 定义中间变量
					int xStep = 0;
					int yStep = 0;
					int zStep = 0;
					int nTmp = 0;
					simpleSize = 0;

					// 格网划分处理
					for (unsigned int n = 0;n < nCount;n++)
					{
						PointXYZIPRGBA* pts = *(bsPoints.m_vecBuf._Myfirst() + n);
						//PointXYZIPRGBA& pts = *(bufferPts._Myfirst() + n);

						// 计算对应格网索引值
						xStep = floor((pts->x - dMinX) * dAntiStep);
						yStep = floor((pts->y - dMinY) * dAntiStep);
						zStep = floor((pts->z - dMinZ) * dAntiStep);

						double dTmpX,dTmpY,dTmpZ;
						dTmpX = dMinX + (xStep +  0.5) * m_dGridPrecision - pts->x;
						dTmpY = dMinY + (yStep +  0.5) * m_dGridPrecision - pts->y;
						dTmpZ = dMinZ + (zStep +  0.5) * m_dGridPrecision - pts->z;

						// 对应键为
						U64 mapPin = zStep * (xRows * yRows) + yStep * xRows + xStep;

						// 插入值
						PosKey posKey;
						posKey.nIndex = n;

						// 计算获取距离box中心最近点
						posKey.dDist = pow(dTmpX,2.0) + pow(dTmpY,2.0) + pow(dTmpZ,2.0);
						posKey.gridIndex = mapPin;
						*(vecPosKeys._Myfirst() + n) = posKey;
					}

					// 针对vec进行处理排序
					std::sort(vecPosKeys.begin(),vecPosKeys.begin() + nCount,SortByGrid);

					// 定义中间变量
					PosKey& pos = *(vecPosKeys._Myfirst() + 0);
					U64 nCurGridPos = pos.gridIndex;
					*(vecTargetIndex._Myfirst() + 0) = pos.nIndex;
					int nCurIndex = 1;

					// 根据之前记录的距离值，grid相同的比较距离，从小到大排序
					for (unsigned int nn = 1;nn < nCount;nn++)
					{
						PosKey& pos = *(vecPosKeys._Myfirst() + nn);
						if (pos.gridIndex == nCurGridPos)
						{
							// 之前已按距离排序，同一格网距离中心最小为最前索引值
							continue;
						}
						else
						{
							// 处理到下一个格网时，记录
							nCurGridPos = pos.gridIndex;
							*(vecTargetIndex._Myfirst() + nCurIndex) = pos.nIndex;
							nCurIndex++;
						}
					}

					// 抽取的点数赋值
					simpleSize = nCurIndex;
#pragma endregion
				}
										
				LARGE_INTEGER li;
				li.HighPart = 0;
				li.LowPart = 0;

				LARGE_INTEGER li1;
				li1.HighPart = 0;
				li1.LowPart = 0;

				SetFilePointerEx(pBlstDataFile,li,&li1,FILE_END);

				PointXYZIPRGBA* pBuffer = new PointXYZIPRGBA[BUFFER_SIZE];

				int buffcnt = 0;
				int k = 0;

				if (m_bResampleZero)
				{
					// 条件写入
					for (U32 m = 0; m < simpleSize; m++)
					{
						int nTmpIndex = *(vecTargetIndex._Myfirst() + m);
						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + nTmpIndex);
						DWORD dwResult;
						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

						*(pBuffer + k) = *pPt;
						k++;

						if (k >= BUFFER_SIZE)
						{
							WriteFile (pBlstDataFile, pBuffer, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);

							buffcnt++;
							k = 0;
							//delete [] pBuffer;
							//pBuffer = new PointXYZIPRGBA [BUFFER_SIZE];
						}
					}
				}
				else
				{
					// 条件写入
					for (U32 m = 0; m < nCount; m++)
					{
						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + m);
						DWORD dwResult;
						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

						*(pBuffer + k) = *pPt;
						k++;

						if (k >= BUFFER_SIZE)
						{
							WriteFile (pBlstDataFile, pBuffer, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);

							buffcnt++;
							k = 0;
							//delete [] pBuffer;
							//pBuffer = new PointXYZIPRGBA [BUFFER_SIZE];
						}
					}
				}
				//for (U32 m=0; m < nCount;m++)
				//{
				//	PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + m);
				//	
				//	blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
				//	blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

				//	*(pBuffer + k) = *pPt;
				//	k++;
				//	DWORD dwResult;			

				//	if (k >= BUFFER_SIZE)
				//	{
				//		WriteFile (pBlstDataFile, pBuffer, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);
				//		buffcnt++;
				//		k = 0;

				//		delete [] pBuffer;
				//		pBuffer = new PointXYZIPRGBA [BUFFER_SIZE];
				//	}			
				//}

				int leftnum = nCount - buffcnt*BUFFER_SIZE;

				if ( k > 0)
				{					
					DWORD dwResult;
					WriteFile (pBlstDataFile, pBuffer, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);
					delete [] pBuffer;
					pBuffer = NULL;
				}

				// 更新文件名
				m_BlocksetFiles[it->first].path = filePath;
				//更新块集中点数目  袁亮   20160909
				m_BlocksetFiles[it->first].numPoint += m_bResampleZero ? simpleSize : nCount;

				if (pBlstDataFile != INVALID_HANDLE_VALUE)
				{
					CloseHandle(pBlstDataFile);
					pBlstDataFile = NULL;
				}
			}

			if (processCallback)
			{			
				processCallback((float)nCurLp / npoint, HDSCENE_IDS_CUTTING);
			}

			if(bReadEnd)
			{
				break;
			}
		}

		if(lasRead != NULL)
		{
			delete lasRead;
			lasRead = NULL;
		}

		// 计算块集的包围盒XY值和参考点
		for (map<U32, BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
			it != m_BlocksetFiles.end(); it++)
		{
			U32 BsIndex = it->first;

			// 计算块集编号
			//无符号整型与有符号整形运算，有符号整形将解释为无符号整形，会将负数变为很大的整数，
			//所以需要确保运算符号一致    袁亮  20160824
			I32 yNo = BsIndex / pHdLevel->m_level.numBlocksetX;
			I32 xNo = BsIndex - (yNo * pHdLevel->m_level.numBlocksetX);

			BlockSetFileInfo& BsInfo = it->second;
			/*BsInfo.box.MinEdge.X = xNo*m_stepX;
			BsInfo.box.MinEdge.Y = yNo*m_stepY;

			BsInfo.box.MaxEdge.X = (xNo+1)*m_stepX;
			BsInfo.box.MaxEdge.Y = (yNo+1)*m_stepY;*/
			//全局网格坐标转换为相对左下角的偏移坐标   袁亮   20160622
			BsInfo.box.MinEdge.X = (xNo+m_xNoFrom)*m_stepX - m_fullExtent.MinEdge.X;
			BsInfo.box.MinEdge.Y = (yNo+m_yNoFrom)*m_stepY - m_fullExtent.MinEdge.Y;
			BsInfo.box.MaxEdge.X = BsInfo.box.MinEdge.X + m_stepX;
			BsInfo.box.MaxEdge.Y = BsInfo.box.MinEdge.Y + m_stepY;
		}

		return true;
	}

	bool CHlzBuilder::SplitInputFiles(const char* savePath)
	{
		// 进度条开始
		if (processCallback)
		{
			processCallback(0.0f, HDSCENE_IDS_BUILDER_DIVIDE_GRID);
		}

		// 定义统计坐标渲染增强变量
		U64 count = m_pCoreData->m_hlzHeader.number_of_point_records;

		int selMax = 0;
		int selMin = 0;

		m_coordStats.Clear();

		F64 xRange = m_fullExtent.getExtent().X;
		F64 yRange = m_fullExtent.getExtent().Y;
		F64 zRange = m_fullExtent.getExtent().Z;

		m_coordStats.xRcp = (F32)(1000.0f / (xRange));
		m_coordStats.yRcp = (F32)(1000.0f / (yRange));
		m_coordStats.zRcp = (F32)(1000.0f / (zRange));
		m_coordStats.iRcp = (F32)(1000.0f / (m_iMax - m_iMin));

		for (U32 i = 0;i< m_hlsFileList.size();i++)
		{
			hd::stringc strExt = m_hlsFileList[i].subString(m_hlsFileList[i].size()-3, 3);
			strExt.make_lower();

			//如果是hls文件
			if (strExt == "hls")
			{
				if(!SplitBlockSetFiles(savePath, m_hlsFileList[i]))
					return false;
			}
			else if(strExt == "xyz")
			{

			}
			else if(strExt == "las")
			{
				SplitBlockSetLas(savePath,m_hlsFileList[i]);
			}
		}

		//块集包围盒退化为外包矩形（z维度丢失）时,手动将z坐标向外扩展0.001m，避免包围盒退化。     袁亮   20161013
		for (map<U32, BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
			it != m_BlocksetFiles.end(); it++)
		{
			BlockSetFileInfo& BsInfo = it->second;
			if(BsInfo.box.MaxEdge.Z - BsInfo.box.MinEdge.Z < 0.001f)
			{
				BsInfo.box.MinEdge.Z = BsInfo.box.MinEdge.Z - 0.0005f;
				BsInfo.box.MaxEdge.Z = BsInfo.box.MaxEdge.Z + 0.0005f;
			}
		}

		m_pCoreData->m_hlzHeader.number_of_point_records = m_ptNum;

		//调整X坐标分布，将点数较少部分剔除不参与统计，获得新的最大最小X值
		U64 selNum = 0;
		CHdVector3dd szBox = m_fullExtent.getExtent();
		for (int j = 999;j >= 0;j--)
		{
			selNum += m_coordStats.xStepStat[j];
			if (selNum >= (int)(m_pCoreData->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMax = j;
				break;
			}
		}

		selNum = 0;
		for (int j = 0;j <= 999;j++)
		{
			selNum += m_coordStats.xStepStat[j];
			if (selNum >= (int)(m_pCoreData->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMin = j;
				break;
			}
		}

		m_pCoreData->m_hlzHeader.renderMinX = (F32)(selMin * (szBox.X) / 1000.0f);
		m_pCoreData->m_hlzHeader.renderMaxX = (F32)(selMax * (szBox.X) / 1000.0f);

		// 调整y坐标分布
		selNum = 0;
		for (int j = 999;j >= 0;j--)
		{
			selNum += m_coordStats.yStepStat[j];
			if (selNum >= (int)(m_pCoreData->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMax = j;
				break;
			}
		}

		selNum = 0;
		for (int j = 0;j <= 999;j++)
		{
			selNum += m_coordStats.yStepStat[j];
			if (selNum >= (int)(m_pCoreData->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMin = j;
				break;
			}
		}
		m_pCoreData->m_hlzHeader.renderMinY = (F32)(selMin * (szBox.Y) / 1000.0f);
		m_pCoreData->m_hlzHeader.renderMaxY = (F32)(selMax * (szBox.Y) / 1000.0f);

		// 调整z坐标分布
		selNum = 0;
		for (int j = 999;j >= 0;j--)
		{
			selNum += m_coordStats.zStepStat[j];
			if (selNum >= (int)(m_pCoreData->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMax = j;
				break;
			}
		}

		selNum = 0;
		for (int j = 0;j <= 999;j++)
		{
			selNum += m_coordStats.zStepStat[j];
			if (selNum >= (int)(m_pCoreData->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMin = j;
				break;
			}
		}
		m_pCoreData->m_hlzHeader.renderMinZ = (F32)(selMin * (szBox.Z) / 1000.0f);
		m_pCoreData->m_hlzHeader.renderMaxZ = (F32)(selMax * (szBox.Z) / 1000.0f);

		// 调整强度分布
		selNum = 0;
		for (int j = 999;j >= 0;j--)
		{
			selNum += m_coordStats.intStepStat[j];
			if (selNum >= (int)(m_pCoreData->m_hlzHeader.number_of_point_records * 0.15))
			{
				selMax = j;
				break;
			}
		}

		selNum = 0;
		for (int j = 0;j <= 999;j++)
		{
			selNum += m_coordStats.intStepStat[j];
			if (selNum >= (int)(m_pCoreData->m_hlzHeader.number_of_point_records * 0.05))
			{
				selMin = j;
				break;
			}
		}
		U32 iRange = m_iMax - m_iMin;
		U32 iMin,iMax;
		iMin = (U32)(selMin * (iRange) / 1000.0f);
		iMax = (U32)(selMax * (iRange) / 1000.0f);
		m_pCoreData->m_hlzHeader.intensityMin = (U8)(iMin * 255 / (F32)iRange);
		m_pCoreData->m_hlzHeader.intensityMax = (U8)(iMax * 255 / (F32)iRange);
		return true;
	}

	bool CHlzBuilder::buildHlz( const char* savePath, BOOL isComress )
	{
		//// 对该接口加密
		//string strSoftName = "hdVector";
		//char strMsg[256] = {0};
		//if (!CheckLicense(strSoftName.c_str(), strMsg))
		//{
		//	::MessageBox(NULL,"请向武汉汉宁轨道交通技术有限公司申请加密狗！", "提示",MB_OK);
		//	return false;
		//}

		m_pCoreData->Close();
		// 解析保存文件路径
		char drive[256] = {0};// 磁盘
		char dir[256] = {0};// 文件夹
		char filename[256] = {0};// 文件名
		char ext[256] = {0};// 文件格式
		char path[256] = {0};// 文件路径

		_splitpath(savePath, drive, dir, filename, ext);

		if (strcmp(ext,".hlz") != 0)
		{
			if (processCallback)
			{
				processCallback(0.0, HDSCENE_IDS_BUILDER_DATA_WORING);
			}
			return false;
		}
		_makepath(path,drive,dir,NULL,NULL);
		m_name = filename;

		// 所有分切的临时文件都在该目录下
		m_savePath = path;
		m_savePath += "\\hls2hlz";
		I32 ret = _mkdir(m_savePath.c_str());

		if (m_hlsFileList.size() == 0)
		{
			return false;
		}

		if (m_BlocksetFiles.size() > 0 )
		{
			m_BlocksetFiles.clear();
		}

		if (m_pHlzWrite)
		{
			delete m_pHlzWrite;
			m_pHlzWrite = NULL;
		}
		m_pHlzWrite = new CHLZWriter();
		m_pHlzWrite->m_header.isCompress = isComress;

		// 设置格网均匀抽稀层级精度
		m_pHlzWrite->SetGridPrecision(m_dGridPrecision);

		// 统计强度范围，坐标范围
		U16 minInt = 0xffff;
		U16 maxInt = 0;
		double dMinX = F64_MAX,dMinY = F64_MAX,dMinZ = F64_MAX;
		double dMaxX = F64_MIN,dMaxY = F64_MIN,dMaxZ = F64_MIN;
		double minx,miny,minz,maxx,maxy,maxz;
		//m_pHlzWrite->m_header.number_of_point_records = 0;
		for (auto it = m_hlsFileList.begin(); it != m_hlsFileList.end();)
		{
			const hd::stringc& strPath = *it;

			hd::stringc strExt = strPath.subString(strPath.size()-3, 3);
			strExt.make_lower();

			//如果是hls文件
			if (strExt == "hls")
			{
				CHLSReadOpener hlsOpen;
				IHLSReader* pHlsReader = hlsOpen.Open(strPath.c_str());
				if (pHlsReader == NULL)
				{
					if(processCallback != NULL)
					{
						char tips[512] = {0};
						sprintf(tips, "打开文件%s失败，跳过转换...", strPath.c_str());
						processCallback(0.0f, tips);
					}

					it = m_hlsFileList.erase(it);
					continue;
				}

				// 统计坐标
				pHlsReader->m_header.getGlobalExtent(minx,miny,minz,maxx,maxy,maxz);
				dMinX = MIN(dMinX,minx);
				dMinY = MIN(dMinY,miny);
				dMinZ = MIN(dMinZ,minz);

				dMaxX = MAX(dMaxX,maxx);
				dMaxY = MAX(dMaxY,maxy);
				dMaxZ = MAX(dMaxZ,maxz);

				// 统计强度
				StatIntensity(pHlsReader,minInt,maxInt);
				m_iMin = MIN(m_iMin,minInt);
				m_iMax = MAX(m_iMax,maxInt);


				// 数据存在异常时，重新统计范围
				if (dMinX == dMaxX && dMinY == dMaxY && dMinZ == dMaxZ )
				{
					calHlsExtnt(pHlsReader, dMinX, dMaxX, dMinY, dMaxY, dMinZ, dMaxZ);
				}

				// 统计点数
				//m_pCoreData->m_hlzHeader.number_of_point_records += pHlsReader->m_header.number_of_point_records;

				delete pHlsReader;
				pHlsReader = NULL;
			}
			else if(strExt == "xyz")
			{

			}
			else if(strExt == "las")
			{
				// las文件打开对象
				LASreadOpener lasreadopener;

				// 设置不合并
				lasreadopener.set_merged(FALSE);
				lasreadopener.set_populate_header(FALSE);

				// 设置文件名
				lasreadopener.set_file_name(strPath.c_str());
				if (!lasreadopener.active())
				{
					if(processCallback != NULL)
					{
						char tips[512] = {0};
						sprintf(tips, "打开文件%s失败，跳过转换...", strPath.c_str());
						processCallback(0.0f, tips);
					}

					it = m_hlsFileList.erase(it);
					continue;
				}
				// 打开las文件
				LASreader* lasRead = lasreadopener.open();
				if(NULL == lasRead)
				{
					if(processCallback != NULL)
					{
						char tips[512] = {0};
						sprintf(tips, "打开文件%s失败，跳过转换...", strPath.c_str());
						processCallback(0.0f, tips);
					}

					it = m_hlsFileList.erase(it);
					continue;
				}

				// 统计坐标
				minx = lasRead->get_min_x();
				maxx = lasRead->get_max_x();
				miny = lasRead->get_min_y();
				maxy = lasRead->get_max_y();
				minz = lasRead->get_min_z();
				maxz = lasRead->get_max_z();
				dMinX = MIN(dMinX,minx);
				dMinY = MIN(dMinY,miny);
				dMinZ = MIN(dMinZ,minz);

				dMaxX = MAX(dMaxX,maxx);
				dMaxY = MAX(dMaxY,maxy);
				dMaxZ = MAX(dMaxZ,maxz);

				// 统计强度
				m_iMin = MIN(m_iMin,0);
				m_iMax = MAX(m_iMax,4095);

				// 统计点数
				//m_pCoreData->m_hlzHeader.number_of_point_records += lasRead->npoints;

				delete lasRead;
				lasRead = NULL;
			}

			++it;
		}

		if(m_hlsFileList.empty())
		{
			if(processCallback != NULL)
			{
				char tips[MAX_PATH] = {0};
				sprintf(tips, "%s.hlz：可用转换列表为空，转换失败！", m_name.c_str());
				processCallback(0.0f, tips);
			}

			return false;
		}

		m_fullExtent.MinEdge.X = dMinX;
		m_fullExtent.MinEdge.Y = dMinY;
		m_fullExtent.MinEdge.Z = dMinZ;
		m_fullExtent.MaxEdge.X = dMaxX;
		m_fullExtent.MaxEdge.Y = dMaxY;
		m_fullExtent.MaxEdge.Z = dMaxZ;


		// 文件头信息
		m_pHlzWrite->m_header.offsetX = m_fullExtent.MinEdge.X;
		m_pHlzWrite->m_header.offsetY = m_fullExtent.MinEdge.Y;
		m_pHlzWrite->m_header.offsetZ = m_fullExtent.MinEdge.Z;
		m_pHlzWrite->m_header.min_x = (F32)(dMinX - m_pHlzWrite->m_header.offsetX);
		m_pHlzWrite->m_header.min_y = (F32)(dMinY - m_pHlzWrite->m_header.offsetY);
		m_pHlzWrite->m_header.min_z = (F32)(dMinZ - m_pHlzWrite->m_header.offsetZ);
		m_pHlzWrite->m_header.max_x = (F32)(dMaxX - m_pHlzWrite->m_header.offsetX);
		m_pHlzWrite->m_header.max_y = (F32)(dMaxY - m_pHlzWrite->m_header.offsetY);
		m_pHlzWrite->m_header.max_z = (F32)(dMaxZ- m_pHlzWrite->m_header.offsetZ);

		//计算当前空间范围对应的全局网格划分范围  袁亮  20160622
		m_xNoFrom = (I32)floor(m_fullExtent.MinEdge.X / m_stepX);
		m_xNoTo = (I32)floor(m_fullExtent.MaxEdge.X / m_stepX);
		m_yNoFrom = (I32)floor(m_fullExtent.MinEdge.Y/ m_stepY);
		m_yNoTo = (I32)floor(m_fullExtent.MaxEdge.Y/ m_stepY);

		//m_pHlzWrite->m_header.grid_sx = floor(m_fullExtent.MinEdge.X);
		//m_pHlzWrite->m_header.grid_sy = floor(m_fullExtent.MinEdge.Y);
		//m_pHlzWrite->m_header.grid_sz = floor(m_fullExtent.MinEdge.Z);

		// 记录更新时间
		time_t timer;
		time(&timer);
		tm* t_tm = localtime(&timer);
		m_pHlzWrite->m_header.file_creation_day = (U16)t_tm->tm_yday + 1;
		m_pHlzWrite->m_header.file_creation_year = (U16)t_tm->tm_year + 1900;

		// 打开文件
		if(!m_pHlzWrite->Open(savePath))
		{
			if (processCallback)
			{
				processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
			}

			return false;
		}

		// 根据点云的范围确定XY方向划分的块数,必须是2的N次方
		//U32 baseNum = (U32)ceil(log(m_fullExtent.getExtent().X / m_stepX) / log(2.f));
		CHdLevel* pLevel = new CHdLevel;
		pLevel->m_levelNo = 0;
		m_pCoreData->AddLevelRec(pLevel);
		//CHdVector3dd extent = m_fullExtent.getExtent();
		//pLevel->m_level.numBlocksetX = (U16)ceil(extent.X / m_stepX);//(U32)pow(2.f,baseNum);
		//baseNum = (U32)ceil(log(m_fullExtent.getExtent().Y / m_stepY) / log(2.f));
		//pLevel->m_level.numBlocksetY = (U16)ceil(extent.Y / m_stepY);//(U32)pow(2.f,baseNum);
		//level0.m_pLevel->numBlocksetZ = (U16)ceil(log(m_fullExtent.getExtent().Z) / log(2));
		//根据全局网格划分范围，确定当前空间范围对应的网格尺寸   袁亮  20160622
		pLevel->m_level.numBlocksetX = m_xNoTo - m_xNoFrom + 1;
		pLevel->m_level.numBlocksetY = m_yNoTo - m_yNoFrom + 1;

		// 更新步长
		//m_stepX = m_fullExtent.getExtent().X / pLevel->m_level.numBlocksetX;
		//m_stepY = m_fullExtent.getExtent().Y / pLevel->m_level.numBlocksetY;
		//m_stepX = ceil(m_stepX);	// 向上取整
		//m_stepY = ceil(m_stepY);
		pLevel->m_level.sizeX = m_stepX;
		pLevel->m_level.sizeY = m_stepY;

		double start_time = GetTickCount();

		// 根据规范规则,进行块集切割
		if (SplitInputFiles(m_savePath.c_str()) == false)
		{
			if (processCallback)
			{
				processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
			}

			RemoveTempFiles(m_savePath.c_str());
			return false;
		}
		double stop_time = GetTickCount();
		printf("划分块集耗时：%f 秒\n", (stop_time - start_time)/((float)1000));
		// 将所有块集文件关闭
		//	CloseTempFiles();

		// 第0层日志信息
		if(DEBUG_LOG)
		{
			char strLv[512];
			sprintf_s(strLv,"level%d:\n",0);
			m_pHlzWrite->WriteLogFile(strLv);
			m_pHlzWrite->WriteLogFile("类型,点数,范围\n");
		}
		// 写入第0层
		start_time = GetTickCount();
		writeLevel0Data(pLevel);
		stop_time = GetTickCount();
		printf("写入0层数据耗时：%f 秒\n", (stop_time - start_time)/((float)1000));
		// 总点数为第0层的点数
		m_pHlzWrite->m_header.number_of_point_records = pLevel->m_level.pointNum;		

		//当前层的点数小于100w时不再分下一层
		U8 nLevelNo = 1;
		CHdLevel* levelPre = pLevel;
		//HdLevel levelNext;
		start_time = GetTickCount();
		while (levelPre->m_level.pointNum > 1000000)
		{
			// 层的日志信息
			if(DEBUG_LOG)
			{	
				char strLv[512] = {0};
				sprintf_s(strLv,"level%d:\n",nLevelNo);
				m_pHlzWrite->WriteLogFile(strLv);
				m_pHlzWrite->WriteLogFile("类型,点数,子块数,范围\n");
			}
			// 下一层
			CHdLevel* pNextLevel = new CHdLevel;
			pNextLevel->m_levelNo = nLevelNo;
			m_pCoreData->AddLevelRec(pNextLevel);
			writeNextLevelData(levelPre, nLevelNo, pNextLevel);

			nLevelNo++;
			levelPre = pNextLevel;
		}
		stop_time = GetTickCount();
		printf("写入其它层数据耗时：%f 秒\n", (stop_time - start_time)/((float)1000));

		m_pHlzWrite->m_header.renderMinX = m_pCoreData->m_hlzHeader.renderMinX;
		m_pHlzWrite->m_header.renderMinY = m_pCoreData->m_hlzHeader.renderMinY;
		m_pHlzWrite->m_header.renderMinZ = m_pCoreData->m_hlzHeader.renderMinZ;
		m_pHlzWrite->m_header.renderMaxX = m_pCoreData->m_hlzHeader.renderMaxX;
		m_pHlzWrite->m_header.renderMaxY = m_pCoreData->m_hlzHeader.renderMaxY;
		m_pHlzWrite->m_header.renderMaxZ = m_pCoreData->m_hlzHeader.renderMaxZ;
		m_pHlzWrite->m_header.intensityMin = m_pCoreData->m_hlzHeader.intensityMin;
		m_pHlzWrite->m_header.intensityMax = m_pCoreData->m_hlzHeader.intensityMax;
		m_pHlzWrite->m_header.number_of_level = nLevelNo;
		m_pHlzWrite->WriteHeader();
		m_pCoreData->m_hlzHeader = m_pHlzWrite->m_header;
		m_pCoreData->UpdateScale();
		m_pHlzWrite->WriteIndex(m_pCoreData->GetListLevel());
		// 写入索引文件并关闭数据文件
		m_pHlzWrite->Close();

		// 删除临时文件
		RemoveTempFiles(m_savePath.c_str());

		if (processCallback)
		{
			processCallback(0.0, HDSCENE_IDS_CONVERT_FINISH);
		}
		return true;
	}

	CHdBox3df CHlzBuilder::GetBlockListBox(std::vector<BlockSetFileInfo>& bsFileList)
	{
		std::vector<BlockSetFileInfo>::iterator it = bsFileList.begin();
		CHdBox3df box = it->box;
		it++;
		for(;it != bsFileList.end();it++)
		{
			box.addInternalBox(it->box);
		}
		return box;
	}

	U64 CHlzBuilder::writeNextLevelData(CHdLevel* pPreLevel, U8 nLevelNo, CHdLevel* pCurLevel)
	{
		/*pCurLevel->m_level.numBlocksetX = (U16)ceil((pPreLevel->m_level.numBlocksetX) / 2.0);
		pCurLevel->m_level.numBlocksetY = (U16)ceil((pPreLevel->m_level.numBlocksetY) / 2.0);*/
		//pCurLevel->m_level.numBlockset = pCurLevel->m_level.numBlocksetX * pCurLevel->m_level.numBlocksetY;
		pCurLevel->m_level.sizeX = pPreLevel->m_level.sizeX * 2.f;
		pCurLevel->m_level.sizeY = pPreLevel->m_level.sizeY * 2.f;
		//根据新的网格尺寸（原始网格尺寸的4倍），对整体空间范围进行重新划分  袁亮  20160623
		m_xNoFrom = (I32)floor(m_fullExtent.MinEdge.X / pCurLevel->m_level.sizeX);
		m_xNoTo = (I32)floor(m_fullExtent.MaxEdge.X / pCurLevel->m_level.sizeX);
		m_yNoFrom = (I32)floor(m_fullExtent.MinEdge.Y / pCurLevel->m_level.sizeY);
		m_yNoTo = (I32)floor(m_fullExtent.MaxEdge.Y / pCurLevel->m_level.sizeY);
		pCurLevel->m_level.numBlocksetX = m_xNoTo - m_xNoFrom + 1;
		pCurLevel->m_level.numBlocksetY = m_yNoTo - m_yNoFrom + 1;
		pCurLevel->m_level.pointNum = (U32)hd_round32(pPreLevel->m_level.pointNum/4.f);		// 大概的点数

		// 写入当前层索引
		m_pHlzWrite->WriteLevelInfo(pCurLevel->m_level);
		// 置0后精确统计
		pCurLevel->m_level.pointNum = 0;	

		// 当前层切分的块集字典表, 当前层写完后替换m_BlocksetFiles
		std::map<U32, BlockSetFileInfo> curBsFiles;

		int iPreBSCount = m_BlocksetFiles.size();
		int iAgrCount = 0;
		int currentCount = 0;
		//#pragma omp parallel for
		for (int iY=0; iY < pCurLevel->m_level.numBlocksetY; iY++)
		{
			//#pragma omp parallel for
			for (int jX=0; jX < pCurLevel->m_level.numBlocksetX; jX++)
			{			
				int err;
				// 获得上一层的四个块集
				std::vector<BlockSetFileInfo> preBlocksetFiles;
				getPreLevelBlockset(jX, iY, pPreLevel, preBlocksetFiles);

				// 当前块集的信息
				if (preBlocksetFiles.size() <= 0)
				{
					// 写入空的块集信息
					continue;
				}
				iAgrCount += preBlocksetFiles.size();


				// 块集的编号
				U32 nBsIndex = iY * pCurLevel->m_level.numBlocksetX + jX;

				if (processCallback)
				{			
					char log[MAX_PATH] = {0};
					sprintf_s(log, "%s%d%s", HDSCENE_IDS_WRITING,nLevelNo,HDSCENE_IDS_LEVEL);
					processCallback((float)nBsIndex / (pCurLevel->m_level.numBlocksetX * pCurLevel->m_level.numBlocksetY), log);
				}

				// 块集的文件路径
				char dir[MAX_PATH];
				sprintf_s(dir, "%s\\level%d", m_savePath.c_str(), nLevelNo);
				I32 ret = _mkdir(dir);

				char BsFileName[MAX_PATH];
				sprintf_s(BsFileName, "%s\\blockset-%04d_%04d.tmp", dir, jX, iY);

				// 新的块集对象
				CHdBlockset* curBlockset = new CHdBlockset;
				curBlockset->m_levelNo = pCurLevel->m_levelNo;
				curBlockset->m_nBlockSetNo = nBsIndex;

				// 根据上一层块集的点获取当前块集的点
				//m_pHlzWrite->WriteNextLevelBlockset(BsFileName, preBlocksetFiles, curBlockset->m_blockSet);
				//m_pHlzWrite->WriteNextLevelBlocksetByMid2(BsFileName, preBlocksetFiles, curBlockset->m_blockSet,nLevelNo);
				m_pHlzWrite->WriteNextLevelBlocksetByMid(BsFileName, preBlocksetFiles, curBlockset->m_blockSet,nLevelNo);
				//WriteNextLevelBlockset中计算的当前块集外包盒不正确，当前块集的包围盒尺寸应该和当前层的网格尺寸相同  袁亮   20160623
				curBlockset->m_blockSet.box.MinEdge.X = (jX + m_xNoFrom) * pCurLevel->m_level.sizeX - m_fullExtent.MinEdge.X;
				curBlockset->m_blockSet.box.MaxEdge.X = curBlockset->m_blockSet.box.MinEdge.X + pCurLevel->m_level.sizeX;
				curBlockset->m_blockSet.box.MinEdge.Y = (iY + m_yNoFrom) * pCurLevel->m_level.sizeY - m_fullExtent.MinEdge.Y;
				curBlockset->m_blockSet.box.MaxEdge.Y = curBlockset->m_blockSet.box.MinEdge.Y + pCurLevel->m_level.sizeY;
				if (curBlockset->m_blockSet.numPoint <= 0)
				{
					delete curBlockset;
					curBlockset = NULL;
					continue;
				}

				pCurLevel->AddBlockSetRec(curBlockset);

				currentCount++;

				// 保存当前块集
				BlockSetFileInfo BsInfo;
				//#pragma omp critical
				{
					if (!curBsFiles.count(nBsIndex))
					{
						BsInfo.box = curBlockset->m_blockSet.box;
						BsInfo.path = BsFileName;
						curBsFiles.insert(make_pair(nBsIndex, BsInfo));
					}
				}

				CHdVector3df centerBS = curBlockset->m_blockSet.box.getCenter();//BsInfo.box.getCenter();
				CHdVector3df halfSzBS = curBlockset->m_blockSet.box.getExtent() / 2.0f;//BsInfo.box.getExtent() / 2.0f;
				HdRefPoint refPtBS;
				refPtBS.FromBox(curBlockset->m_blockSet.box);

				// 存储点信息
				std::vector<PointXYZIPRGBA> vecBuf;

				if(DEBUG_LOG)
				{
					char logmsg[512] = {0};

					sprintf_s(logmsg,"%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
						curBlockset->m_blockSet.box.MinEdge.X,curBlockset->m_blockSet.box.MinEdge.Y,
						curBlockset->m_blockSet.box.MinEdge.Z,curBlockset->m_blockSet.box.MaxEdge.X,
						curBlockset->m_blockSet.box.MaxEdge.Y,curBlockset->m_blockSet.box.MaxEdge.Z);
					m_pHlzWrite->WriteLogFile(logmsg);
					sprintf_s(logmsg,"---------------------------------------------\n");
					m_pHlzWrite->WriteLogFile(logmsg);
				}

				if (curBlockset->m_blockSet.numPoint <= BLOCK_HOLD)
				{
					// 不需要向下分块
					curBlockset->m_blockSet.hasSubBlock = 0;

					//使用win32 API文件操作代替c文件操作，提高io效率。  袁亮   20160909
					HANDLE pBsFile = CreateFile(BsFileName, 
						GENERIC_READ,
						FILE_SHARE_READ, 
						NULL,
						OPEN_ALWAYS, 
						FILE_ATTRIBUTE_NORMAL, 
						NULL);

					if(pBsFile == INVALID_HANDLE_VALUE)
					{
						continue;
					}

					hd::u64 cnt = curBlockset->m_blockSet.numPoint;
					vecBuf.resize(cnt);

					DWORD numRead;
					ReadFile(pBsFile, vecBuf._Myfirst(), sizeof(PointXYZIPRGBA) * (DWORD)cnt, &numRead,NULL);

					::CloseHandle(pBsFile);
					pBsFile = NULL;
					/*FILE* pBsFile = fopen(BsFileName,"rb");			
					if (pBsFile == NULL)
					{
						continue;
					}

					// 读取到内存
					vecBuf.resize(curBlockset->m_blockSet.numPoint);
					fseek(pBsFile, 0, SEEK_SET);
					fread(vecBuf._Myfirst, sizeof(PointXYZIPRGBA), curBlockset->m_blockSet.numPoint, pBsFile);
					// 关闭块集文件
					if (pBsFile)
					{
						fclose(pBsFile);
						pBsFile = NULL;
					}*/

					// 内存点云拆分为坐标和强度
					HdPointXYZ* pPtXYZ = NULL;
					U8* pPtInten = NULL;
					HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
					if (!m_pHlzWrite->m_header.isCompress)
					{
						// 压缩
						//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
						U64 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), curBlockset->m_blockSet.numPoint, &refPtBS, &pPtXYZ, &pPtInten, &pPtColor, m_iMin, m_iMax);
						curBlockset->m_blockSet.numPoint = cntWrite;
						// 写入坐标和强度
						m_pHlzWrite->WriteBlockSetData(curBlockset->m_blockSet, pPtXYZ, pPtInten, pPtColor, curBlockset->m_blockSet.numPoint);
					}
					else
					{
						// 不压缩
						// 重排序
						std::sort(vecBuf.begin(), vecBuf.end(), lessByXYZ);
						// 拆分坐标和强度
						//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
						U64 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), curBlockset->m_blockSet.numPoint, &refPtBS, &pPtXYZ, &pPtInten, &pPtColor, m_iMin, m_iMax);
						curBlockset->m_blockSet.numPoint = cntWrite;
						// 压缩
						CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
						ptXYZEncoder.encodePoints();
						// 写入坐标和强度
						m_pHlzWrite->WriteBlockSetData(curBlockset->m_blockSet, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, curBlockset->m_blockSet.numPoint);
						ptXYZEncoder.clear();
					}

					// 统计总块数
					m_pHlzWrite->m_header.number_of_col++;

					if (pPtXYZ)
					{
						delete []pPtXYZ;
						pPtXYZ = NULL;
					}
					if (pPtInten)
					{
						delete []pPtInten;
						pPtInten = NULL;
					}
					if (pPtColor != NULL)
					{
						delete[] pPtColor;
						pPtColor = NULL;
					}

				}//if (curBlockset->m_blockSet.m_numPoint <= BLOCK_HOLD)
				else
				{// else分块写入
					// 该块集对应的块文件夹
					std::string blocksPath = BsFileName;
					blocksPath = blocksPath.substr(0, blocksPath.find_last_of('.'));

					// 采用二分的思想,对块集点云继续分割为块文件	
					std::vector<BlockFileInfo> arrayBlock;
					SplitBlockFiles(BsInfo,arrayBlock);

					curBlockset->m_blockSet.hasSubBlock = 1;
					curBlockset->m_blockSet.numPoint = 0; // 重新置0,因为内部会自动统计

					// 只写索引文件
					m_pHlzWrite->WriteBlockSetData(curBlockset->m_blockSet, NULL, NULL, NULL, 0);

					CHdBlock* curBlock = NULL;

					for (U32 n=0; n < arrayBlock.size(); n++)
					{
						BlockFileInfo& blockFileInfo = *(arrayBlock._Myfirst() + n);
					
						//直接从BlockFileInfo中读取块中点数目，不需要再计算。   袁亮  20160909
						U64 count = blockFileInfo.numPoint;
						if(count <= 0)
						{
							continue;
						}

						/*HANDLE pBlockDataFile = CreateFile(blockFileInfo.path.c_str(), 
							GENERIC_READ,
							FILE_SHARE_READ, 
							NULL,
							OPEN_EXISTING, 
							FILE_ATTRIBUTE_NORMAL, 
							NULL);

						if (pBlockDataFile == INVALID_HANDLE_VALUE)
						{
							continue;
						}

						U64 count = 0;
												
						LARGE_INTEGER li;
						li.HighPart = 0;
						li.LowPart = 0;
						LARGE_INTEGER li1;
						li1.HighPart = 0;
						li1.LowPart = 0;

						SetFilePointerEx(pBlockDataFile,li,&li1,FILE_END);
						U64 fileSize = ((U64)li1.HighPart<<32)|((U64)li1.LowPart);

						count = fileSize / (sizeof(PointXYZIPRGBA));*/
						
						// 当前块对象
						curBlock = new CHdBlock;
						curBlock->m_bsNo = curBlockset->m_nBlockSetNo;
						curBlock->m_nBlockNo = blockFileInfo.index;
						curBlockset->AddBlockRec(curBlock);

						// 当前块对象的空间范围
						getBlockBox(centerBS,halfSzBS,curBlock->m_block.box,curBlock->m_nBlockNo);
						HdRefPoint bkRefPt;
						bkRefPt.FromBox(curBlock->m_block.box);
						CHdVector3df centerBK = curBlock->m_block.box.getCenter();
						CHdVector3df halfSzBK = curBlock->m_block.box.getExtent() / 2.0f;

						if (count <= BLOCK_HOLD)
						{// 写入块
							curBlock->m_block.hasSubParcel = 0;
							curBlock->m_block.numPoint = count;

							HANDLE pBlockDataFile = CreateFile(blockFileInfo.path.c_str(), 
								GENERIC_READ,
								FILE_SHARE_READ, 
								NULL,
								OPEN_EXISTING, 
								FILE_ATTRIBUTE_NORMAL, 
								NULL);

							// 当前块文件不需要再切分
							vecBuf.resize(count);
							
							/*LARGE_INTEGER li;
							li.HighPart = 0;
							li.LowPart = 0;

							LARGE_INTEGER li1;
							li1.HighPart = 0;
							li1.LowPart = 0;

							SetFilePointerEx(pBlockDataFile,li,&li1,FILE_BEGIN);*/

							DWORD numRead;
							ReadFile(pBlockDataFile,vecBuf._Myfirst(),sizeof(PointXYZIPRGBA) * (DWORD)count,&numRead,NULL);

							::CloseHandle(pBlockDataFile);
							pBlockDataFile = NULL;

							HdPointXYZ* pPtXYZ = NULL;
							U8* pPtInten = NULL;
							HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
							if (!m_pHlzWrite->m_header.isCompress)
							{
								// 不压缩
								// 内存点云拆分为坐标和强度,写入块内点云
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &bkRefPt, &pPtXYZ, &pPtInten, &pPtColor, m_iMin, m_iMax);
								curBlockset->m_blockSet.numPoint = cntWrite;
								// 写入坐标和强度
								m_pHlzWrite->WriteBlockData(curBlock->m_block, pPtXYZ, pPtInten, pPtColor, curBlockset->m_blockSet.numPoint);
							}
							else
							{
								// 压缩
								// 重排序
								std::sort(vecBuf.begin(), vecBuf.end(), lessByXYZ);
								// 内存点云拆分为坐标和强度,写入块内点云
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &bkRefPt, &pPtXYZ, &pPtInten, &pPtColor, m_iMin, m_iMax);
								curBlockset->m_blockSet.numPoint = cntWrite;
								// 压缩
								CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
								ptXYZEncoder.encodePoints();
								// 写入坐标和强度
								m_pHlzWrite->WriteBlockData(curBlock->m_block, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, curBlockset->m_blockSet.numPoint);
								ptXYZEncoder.clear();
							}

							if (pPtXYZ)
							{
								delete []pPtXYZ;
								pPtXYZ = NULL;
							}
							if (pPtInten)
							{
								delete []pPtInten;
								pPtInten = NULL;
							}
							if (pPtColor != NULL)
							{
								delete[] pPtColor;
								pPtColor = NULL;
							}
						}
						else
						{// 分包写入
							// 切分后包的路径
							std::string dir = blockFileInfo.path.c_str(); //sBlockPath.c_str();
							dir = dir.substr(0, dir.find_last_of('.'));
							ret = _mkdir(dir.c_str());

							char parcelsPath[MAX_PATH] = {0};
							sprintf_s(parcelsPath,"%s", dir.c_str());

							// 将块切分为包
							std::vector<ParcelFileInfo> splitParcels;
							SplitParcelFiles(curBlock->m_block.box,blockFileInfo.path.c_str(),parcelsPath,splitParcels);

							// 只写块索引信息
							curBlock->m_block.hasSubParcel = 1;
							curBlock->m_block.numPoint = 0;		// 重新置0,内部添加包时自动统计
							m_pHlzWrite->WriteBlockData(curBlock->m_block, NULL, NULL, NULL, 0);

							// 将切分后包文件的点云写入
							U16 iPcl = 0;
							for (std::vector<ParcelFileInfo>::iterator it = splitParcels.begin();
								it != splitParcels.end(); it++)
							{
								// 创建包对象
								CHdParcel* parcel = new CHdParcel;
								parcel->m_bkNo = curBlock->m_nBlockNo;
								parcel->m_nParcelNo = iPcl;
								iPcl++;

								parcel->m_parcel.box = (*it).box;
								parcel->m_parcel.numPoint = (*it).numPoint;	
								HdRefPoint pclRefPt;
								pclRefPt.FromBox(parcel->m_parcel.box);

								// 数据点
								//win32 API文件操作代替c文件操作，提高io读取速度。   袁亮   20160909
								hd::u64  cnt = (*it).numPoint;
								vecBuf.resize(cnt);

								const hd::stringc& sParcelPath = (*it).path;
								HANDLE pParcelFile = CreateFile(sParcelPath.c_str(), 
									GENERIC_READ ,
									FILE_SHARE_READ, 
									NULL,
									OPEN_EXISTING, 
									FILE_ATTRIBUTE_NORMAL, 
									NULL);

								if(pParcelFile == INVALID_HANDLE_VALUE)
								{
									delete parcel;
									parcel = NULL;
									continue;
								}

								DWORD numRead;
								ReadFile(pParcelFile, vecBuf._Myfirst(),sizeof(PointXYZIPRGBA)*(DWORD)cnt, &numRead,NULL);

								::CloseHandle(pParcelFile);
								pParcelFile = NULL;
								/*FILE* pParcelFile = fopen(sParcelPath.c_str(), "rb");
								if (pParcelFile == NULL)
								{
									delete parcel;
									parcel = NULL;
									continue;
								}
								vecBuf.resize(parcel->m_parcel.numPoint);
								fseek(pParcelFile, 0, SEEK_SET);
								fread(vecBuf._Myfirst, sizeof(PointXYZIPRGBA), parcel->m_parcel.numPoint, pParcelFile);

								// 关闭文件
								if (pParcelFile)
								{
									fclose(pParcelFile);
									pParcelFile = NULL;
								}*/

								// 内存点云拆分为坐标和强度,写入包内点云
								HdPointXYZ* pPtXYZ = NULL;
								U8* pPtInten = NULL;
								HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
								if (!m_pHlzWrite->m_header.isCompress)
								{
									// 不压缩
									//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
									U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), parcel->m_parcel.numPoint, &pclRefPt, &pPtXYZ, &pPtInten, &pPtColor, m_iMin, m_iMax);
									parcel->m_parcel.numPoint = cntWrite;
									// 写入坐标和强度
									m_pHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, pPtColor, parcel->m_parcel.numPoint);
								}
								else
								{
									// 压缩
									// 重排序
									std::sort(vecBuf.begin(), vecBuf.end(), lessByXYZ);
									// 拆分成坐标和强度
									//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
									U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), parcel->m_parcel.numPoint, &pclRefPt, &pPtXYZ, &pPtInten, &pPtColor, m_iMin, m_iMax);
									parcel->m_parcel.numPoint = cntWrite;
									// 压缩
									CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
									ptXYZEncoder.encodePoints();
									// 写入坐标和强度
									m_pHlzWrite->WriteParcelData(parcel->m_parcel, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, parcel->m_parcel.numPoint);
									ptXYZEncoder.clear();
								}
								
								curBlock->AddParcel(parcel);

								if (pPtXYZ)
								{
									delete []pPtXYZ;
									pPtXYZ = NULL;
								}
								if (pPtInten)
								{
									delete []pPtInten;
									pPtInten = NULL;
								}
								if (pPtColor != NULL)
								{
									delete[] pPtColor;
									pPtColor = NULL;
								}

								// 将包文件删除，节省磁盘空间
								err = remove(sParcelPath.c_str());
								if (err == -1)
								{
				//					perror("无法删除Parcel文件writeNextLevelData");
								}

							}//for (std::vector<ParcelFileInfo>::iterator it = splitParcels.begin();	
							curBlock->Update();
						}// else分包写入

						/*if (pBlockDataFile != INVALID_HANDLE_VALUE)
						{
							CloseHandle(pBlockDataFile);
							pBlockDataFile = NULL;
						}*/

						// 将块文件删除，节省磁盘空间
						err = remove(blockFileInfo.path.c_str());
						if (err == -1)
						{
							perror("无法删除文件writeNextLevelData");
						}
					}
					curBlockset->Update();
				}// else分块写入
			}// for j
		}// for i
		pCurLevel->Update();
		pCurLevel->UpdateScale();

		// 写入日志文件
		if(DEBUG_LOG)
		{
			char logmsg[512] = {0};
			sprintf_s(logmsg,"pre:%d,agrC:%d,current:%d\n", iPreBSCount,iAgrCount,currentCount);		
			m_pHlzWrite->WriteLogFile(logmsg);
		}

		// 删除上一层块集文件数据
		RemoveBSFiles(m_BlocksetFiles);
		// 用当前层替换前一层的块集文件记录
		m_BlocksetFiles = curBsFiles;

		return pCurLevel->m_level.pointNum;
	}

	//! 写入下一层数据
	//! 按标准格网切割Hls成为块集
	bool CHlzBuilder::SplitBlockSetFiles_LN(const char* savePath, hd::stringc& strHlsFile,U8 iLevel)
	{
		CHLSReadOpener hlsOpen;
		IHLSReader* pHlsReader = hlsOpen.Open(strHlsFile.c_str());
		if (pHlsReader == NULL)
		{
			return false;
		}

		hd::stringc strInfo = HDSCENE_IDS_PROCESSING;
		int nPos = strHlsFile.findLast('\\');
		strInfo += strHlsFile.subString(nPos + 1, strHlsFile.size() - nPos - 1);

		// 进度条开始
		if (processCallback)
		{
			processCallback(0.0f, strInfo.c_str());
		}

		CHdLevel* pHdLevel = m_pCoreData->GetLevelRec(iLevel);
		if(pHdLevel == NULL)
		{
			pHdLevel = new CHdLevel;
			pHdLevel->m_levelNo = 0;
			m_pCoreData->AddLevelRec(pHdLevel);
		}
		// 构造全局坐标转换矩阵
		double m[16];
		pHlsReader->m_header.computeMatrix(m);

		// 获取圈数
		int loopCount = pHlsReader->GetLoopCount();

		// 获取圈索引
		const CLoopIndex* pLoopIdx = pHlsReader->GetLoopIndex(); 

		// 创建第0层临时文件目录
		char dir[MAX_PATH] = {0};
		sprintf_s(dir, "%s\\level0", savePath);
		I32 ret = _mkdir(dir);

		char filePath[MAX_PATH] = {0};

		// 读取1000圈点云后一次写入到块集文件
		U32 nLoopSize = 1000;
		U32 nCurLp = 0;
		
		while((loopCount - nCurLp) > 0)
		{
			// 内存中的1000圈点云
			hdBlkArray<PointXYZIPRGBA> loopPtsArray;
			if ((loopCount - nCurLp) < nLoopSize)
			{
				nLoopSize = (loopCount - nCurLp);
			}
			loopPtsArray.setBlockCount(nLoopSize);

			// 从文件中读满loopPtsArray
			for (U32 n = 0; n < nLoopSize; n++)
			{
				hdVector<PointXYZIPRGBA>& vecPts = loopPtsArray.getBlock(n);
				if(!pHlsReader->ReadLoop(vecPts, n+nCurLp))
					continue;
			}

			// 更新当前圈
			nCurLp += nLoopSize;

			std::map<U32, SplitMemoryBuf> BsPointsArray;

			for (I32 i=0; i < nLoopSize; i++)
			{
				hdVector<PointXYZIPRGBA>& vecPts = loopPtsArray.getBlock(i);

				for (I32 k = 0;k < vecPts.size();k++)
				{
					PointXYZIPRGBA& pt = vecPts[k];
					if(!pt.isValid())
						continue;

					// 转为全局坐标
					double x = pt.x;
					double y = pt.y;
					double z = pt.z;
					hdHomogeneousTransformPoint(m,x,y,z);

					// 坐标偏移
					pt.x = (hd::f32)(x - m_fullExtent.MinEdge.X);
					pt.y = (hd::f32)(y - m_fullExtent.MinEdge.Y);
					pt.z = (hd::f32)(z - m_fullExtent.MinEdge.Z);

					// 计算块集编号
					U32 xNo = (U32)((pt.x) / m_stepX);// - m_pHlzWrite->m_header.grid_sx
					U32 yNo = (U32)((pt.y) / m_stepY);// - m_pHlzWrite->m_header.grid_sy
					//U32 fileNo = pHdLevel->m_level.numBlocksetY*xNo + yNo;
					U32 fileNo = pHdLevel->m_level.numBlocksetX*yNo + xNo;
										
					SplitMemoryBuf& memPoints = BsPointsArray[fileNo];

					if (memPoints.m_count >= memPoints.m_vecBuf.size())
					{
						memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
					}

					PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
					memPoints.m_count++;
					pPtMem = &pt;					
					
					// 统计x分布
					U32 selNum = (U32)((x - m_fullExtent.MinEdge.X) * m_coordStats.xRcp);
					selNum = clamp(selNum,(U32)0,(U32)999);
					m_coordStats.xStepStat[selNum]++;

					// 统计y分布
					selNum = (U32)((y - m_fullExtent.MinEdge.Y) * m_coordStats.yRcp);
					selNum = clamp(selNum,(U32)0,(U32)999);
					m_coordStats.yStepStat[selNum]++;

					// 统计z分布
					selNum = (U32)((z - m_fullExtent.MinEdge.Z) * m_coordStats.zRcp);
					selNum = clamp(selNum,(U32)0,(U32)999);
					m_coordStats.zStepStat[selNum]++;

					// 统计强度分布
					selNum = (U32)((pt.getIntensity() - m_iMin) * m_coordStats.iRcp);
					selNum = clamp(selNum,(U32)0,(U32)999);
					m_coordStats.intStepStat[selNum]++;
				}//for (I32 k = 0;k < vecPts.size();k++)
			}//for (I32 i=0; i < nLoopSize; i++)

			// 将内存中的点写入到块集文件中
			for (std::map<U32, SplitMemoryBuf>::iterator it = BsPointsArray.begin(); 
				it != BsPointsArray.end(); it++)
			{
				SplitMemoryBuf& bsPoints = (it->second);
				U32 nCount = bsPoints.m_count;
				if (nCount <= 0)
				{
					continue;
				}

				// 更新包围盒的Z值
				CHdBox3df& blocksetBox = m_BlocksetFiles[it->first].box;

				//FILE*& pFile = m_BlocksetFiles[it->first].pFile;
				sprintf(filePath,"%s\\level0\\blockset_%04d.tmp",savePath,(it->first));
				FILE* pFile = fopen(filePath,"ab+");
				if (pFile != NULL)
				{				
					for (U32 m=0; m < nCount; m++)
					{
						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + m);
						fwrite(pPt, sizeof(PointXYZIPRGBA), 1, pFile);

						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);
					}

					// 更新文件名
					m_BlocksetFiles[it->first].path = filePath;
				}

				if (pFile)
				{
					fclose(pFile);
					pFile = NULL;
				}
			}		
		}

		// 计算块集的包围盒XY值和参考点
		for (map<U32, BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
			it != m_BlocksetFiles.end(); it++)
		{
			U32 BsIndex = it->first;

			// 计算块集编号
			U32 yNo = BsIndex / pHdLevel->m_level.numBlocksetX;
			U32 xNo = BsIndex - (yNo * pHdLevel->m_level.numBlocksetX);

			BlockSetFileInfo& BsInfo = it->second;
			BsInfo.box.MinEdge.X = xNo*m_stepX;
			BsInfo.box.MinEdge.Y = yNo*m_stepY;

			BsInfo.box.MaxEdge.X = (xNo+1)*m_stepX;
			BsInfo.box.MaxEdge.Y = (yNo+1)*m_stepY;
		}

		if (pHlsReader != NULL)
		{
			delete pHlsReader;
			pHlsReader = NULL;
		}

		return true;
	}

	void CHlzBuilder::getPreLevelBlockset(U16 xNo, U16 yNo, CHdLevel* pPreLevel, std::vector<BlockSetFileInfo>& preBlocksetFiles)
	{
		//计算当前网格相对左下角偏移坐标范围G，遍历上层块集划分结果，若上层块集的box完全包含于G，则加入到当前网格块集中  袁亮  20160623
		F32 xFrom = ((I32)xNo+m_xNoFrom)*pPreLevel->m_level.sizeX*2 - m_fullExtent.MinEdge.X;
		F32 xTo = xFrom + pPreLevel->m_level.sizeX*2;
		F32 yFrom = ((I32)yNo+m_yNoFrom)*pPreLevel->m_level.sizeY*2 - m_fullExtent.MinEdge.Y;
		F32 yTo = yFrom + pPreLevel->m_level.sizeY*2;

		for(auto it = m_BlocksetFiles.begin(); it != m_BlocksetFiles.end(); it++)
		{
			CHdBox3df& box = it->second.box;
			//if(xFrom <= box.MinEdge.X && box.MaxEdge.X <= xTo && yFrom <= box.MinEdge.Y && box.MaxEdge.Y <= yTo)
			//注意浮点型数据的"相等"需要指定容差范围   袁亮   20160927
			F32 fTolerace = 0.001;
			if((xFrom < box.MinEdge.X || fabs(xFrom -  box.MinEdge.X) <= fTolerace) &&
			   (box.MaxEdge.X < xTo || fabs(xTo - box.MaxEdge.X) <= fTolerace) && 
			   (yFrom < box.MinEdge.Y || fabs(yFrom - box.MinEdge.Y) <= fTolerace) && 
			   (box.MaxEdge.Y < yTo || fabs(yTo - box.MaxEdge.Y) <= fTolerace))
			{
				preBlocksetFiles.push_back(it->second);
			}

			if(DEBUG_LOG)
			{
				/*char logmsg[512] = {0};
				sprintf_s(logmsg,"level-%d-%d:%d,%d,%d,%d\n",pPreLevel->m_level.numBlocksetX,pPreLevel->m_level.numBlocksetY,pPreLevel->m_levelNo,it->first, xPre+jX,yPre+iY);
				m_pHlzWrite->WriteLogFile(logmsg);*/
			}					
		}
		// 当前层格网(xNo, yNo)对应的上一层的格网编号为(xNo*2, yNo*2);
		/*U16 xPre = xNo*2;
		U16 yPre = yNo*2;

		// 前一层四个块集确定当前层一个块集 
		for (U16 iY=0; iY < 2; iY++)
		{
			for (U16 jX=0; jX < 2; jX++)
			{
				if(yPre+iY >= pPreLevel->m_level.numBlocksetY ||
					xPre+jX >= pPreLevel->m_level.numBlocksetX)
					continue;
				// 上一层的序号
				U32 index = (yPre+iY)*pPreLevel->m_level.numBlocksetX + (xPre+jX);

				// 对应的上一层块集
				if (m_BlocksetFiles.count(index) == 0)
					continue;

				preBlocksetFiles.push_back(m_BlocksetFiles[index]);

				if(DEBUG_LOG)
				{
					char logmsg[512] = {0};
					sprintf_s(logmsg,"level-%d-%d:%d,%d,%d,%d\n",pPreLevel->m_level.numBlocksetX,pPreLevel->m_level.numBlocksetY,pPreLevel->m_levelNo,index,xPre+jX,yPre+iY);
					m_pHlzWrite->WriteLogFile(logmsg);
				}					
			}
		}*/
	}

	void CHlzBuilder::writeLevel0Data(CHdLevel* pLevel0)
	{
		// 写入第0层的索引信息
		//pLevel0->m_level.m_numBlockset = m_BlocksetFiles.size();			// 有效块集个数
		m_pHlzWrite->WriteLevelInfo(pLevel0->m_level);

		// 每个临时文件的点对应一个块集的点
		U16 iBS = 0;
		HdPointXYZ* pPtXYZ = NULL;
		U8* pPtInten = NULL;
		HdPtColor* pPtColor = NULL;  //保存点的颜色信息   袁亮  20160625
		U32 err;
		for (map<U32, BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
			it != m_BlocksetFiles.end(); it++)
		{
			/*HANDLE pBlstDataFile = CreateFile((it->second).path.c_str(), 
				GENERIC_READ ,
				FILE_SHARE_READ, 
				NULL,
				OPEN_ALWAYS, 
				FILE_ATTRIBUTE_NORMAL, 
				NULL);

			if (pBlstDataFile == INVALID_HANDLE_VALUE)
			{
				continue;
			}*/

			if (processCallback)
			{			
				processCallback((float)iBS / m_BlocksetFiles.size(), HDSCENE_IDS_WRITING_ZERO_LEVEL);
			}

			U32 fileNo = it->first;
			HdRefPoint refPtBS;
			refPtBS.FromBox(it->second.box);
			CHdVector3df centerBS = it->second.box.getCenter();
			CHdVector3df halfSzBS = it->second.box.getExtent() / 2.0f;

			// 块集对象
			CHdBlockset* pBlockSet = new CHdBlockset;
			pBlockSet->m_nBlockSetNo = fileNo;
			pBlockSet->m_blockSet.box = (it->second).box;
			iBS++;
			pBlockSet->m_levelNo = pLevel0->m_levelNo;
			
			// 存储点信息
			std::vector<PointXYZIPRGBA> vecBuf;
			//直接从BlockSetFileInfo中读取块集点数，不需要打开文件移动指针计算  袁亮  20160909
			hd::u64 count = it->second.numPoint;
						
			/*LARGE_INTEGER li;
			li.HighPart = 0;
			li.LowPart = 0;

			LARGE_INTEGER li1;
			li1.HighPart = 0;
			li1.LowPart = 0;

			
			SetFilePointerEx(pBlstDataFile,li,&li1,FILE_END);
			U64 fileSize = ((U64)li1.HighPart<<32)|li1.LowPart;

			count = fileSize / (sizeof(PointXYZIPRGBA));*/
			if (count == 0)
			{
				/*CloseHandle(pBlstDataFile);
				pBlstDataFile = NULL;*/
				continue;
			}

			pLevel0->AddBlockSetRec(pBlockSet);

			HdRefPoint refPtBK;
			if (count <= BLOCK_HOLD)
			{
				// 读取到内存
				vecBuf.resize(count);

				//文件内存映射读取文件   袁亮  20160909
				HANDLE pBlstDataFile = CreateFile((it->second).path.c_str(), 
					GENERIC_READ ,
					FILE_SHARE_READ, 
					NULL,
					OPEN_ALWAYS, 
					FILE_ATTRIBUTE_NORMAL, 
					NULL);

				/*LARGE_INTEGER li;
				li.HighPart = 0;
				li.LowPart = 0;

				LARGE_INTEGER li1;
				li1.HighPart = 0;
				li1.LowPart = 0;

				SetFilePointerEx(pBlstDataFile,li,&li1,FILE_BEGIN);*/

				DWORD numRead;
				ReadFile(pBlstDataFile,vecBuf._Myfirst(),sizeof(PointXYZIPRGBA)*(DWORD)count,&numRead,NULL);
	
				// 块集内有数据情况下,需要单独更新点数
				pBlockSet->m_blockSet.hasSubBlock = 0;
				pBlockSet->m_blockSet.numPoint = count;

				CloseHandle(pBlstDataFile);
				pBlstDataFile = NULL;

				if (!m_pHlzWrite->m_header.isCompress)
				{
					// 不压缩，直接写入
					// 写入块集内部点,内存点云拆分为坐标和强度
					//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
					U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBS, &pPtXYZ, &pPtInten, &pPtColor, m_iMin, m_iMax);
					pBlockSet->m_blockSet.numPoint = cntWrite;
					// 写入坐标和强度
					// 写入颜色数据   袁亮   20160625
					m_pHlzWrite->WriteBlockSetData(pBlockSet->m_blockSet, pPtXYZ, pPtInten, pPtColor, pBlockSet->m_blockSet.numPoint);
				}
				else
				{
					// 压缩写入
					// 重排序
					std::sort(vecBuf.begin(), vecBuf.end(), lessByXYZ);
					// 写入块集内部点,内存点云拆分为坐标和强度
					//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
					U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBS, &pPtXYZ, &pPtInten, &pPtColor, m_iMin, m_iMax);
					pBlockSet->m_blockSet.numPoint = cntWrite;
					// 压缩
					CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
					ptXYZEncoder.encodePoints();
					// 写入数据
					// 写入颜色数据   袁亮   20160625
					m_pHlzWrite->WriteBlockSetData(pBlockSet->m_blockSet, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, pBlockSet->m_blockSet.numPoint);
					ptXYZEncoder.clear();
				}
								
				// 统计总块数
				m_pHlzWrite->m_header.number_of_col++;

				if (pPtXYZ)
				{
					delete []pPtXYZ;
					pPtXYZ = NULL;
				}
				if (pPtInten)
				{
					delete []pPtInten;
					pPtInten = NULL;
				}
				if (pPtColor != NULL)
				{
					delete[] pPtColor;
					pPtColor = NULL;
				}
			}
			else
			{// else分块写入
				
				/*if (pBlstDataFile!=INVALID_HANDLE_VALUE)
				{
					CloseHandle(pBlstDataFile);
					pBlstDataFile = NULL;
				}*/

				// 该块集对应的块文件夹
				std::string blocksPath = (it->second).path;
				blocksPath = blocksPath.substr(0, blocksPath.find_last_of('.'));

				// 采用二分的思想,对块集点云继续分割为块文件
				std::vector<BlockFileInfo> arrayBlock;
				SplitBlockFiles(it->second,arrayBlock);

				pBlockSet->m_blockSet.hasSubBlock = 1;			

				// 只写块集索引
				m_pHlzWrite->WriteBlockSetData(pBlockSet->m_blockSet, NULL, NULL, NULL, 0);

				// 统计总块集数
				m_pHlzWrite->m_header.number_of_col += arrayBlock.size();

				for (U32 i=0; i < arrayBlock.size(); i++)
				{
					BlockFileInfo& blockFile = *(arrayBlock._Myfirst() + i);
					if(blockFile.numPoint == 0)
						continue;
					
					/*HANDLE pBlockDataFile = CreateFile(blockFile.path.c_str(), 
						GENERIC_READ ,
						FILE_SHARE_READ, 
						NULL,
						OPEN_EXISTING, 
						FILE_ATTRIBUTE_NORMAL, 
						NULL);

					if (pBlockDataFile == INVALID_HANDLE_VALUE)
					{
						continue;
					}
	
					LARGE_INTEGER li;
					li.HighPart = 0;
					li.LowPart = 0;

					LARGE_INTEGER li1;
					li1.HighPart = 0;
					li1.LowPart = 0;

					SetFilePointerEx(pBlockDataFile,li,&li1,FILE_END);

					U64 fileSize = ((U64)li1.HighPart<<32)|li1.LowPart;

					count = fileSize / (sizeof(PointXYZIPRGBA));
											
					if (count == 0)
					{
						CloseHandle(pBlockDataFile);
						pBlockDataFile = NULL;
						continue;
					}*/


					// 当前块对象
					//直接使用BlockFileInfo中记录的点数目，不需要重新计算。   袁亮  20160909
					count = blockFile.numPoint;

					CHdBlock* pCurBlock = new CHdBlock;
					pCurBlock->m_bsNo = pBlockSet->m_nBlockSetNo;
					pCurBlock->m_nBlockNo = blockFile.index;
					pBlockSet->AddBlockRec(pCurBlock);

					// 当前块对象的空间范围
					getBlockBox(centerBS,halfSzBS,pCurBlock->m_block.box,blockFile.index);
					refPtBK.FromBox(pCurBlock->m_block.box);
					CHdVector3df centerBK = pCurBlock->m_block.box.getCenter();
					CHdVector3df halfSzBK = pCurBlock->m_block.box.getExtent() / 2.0f;

					if (count <= BLOCK_HOLD)
					{
						pCurBlock->m_block.hasSubParcel = 0;
						pCurBlock->m_block.numPoint = count;

						// 当前块文件不需要再切分
						vecBuf.resize(count);

						HANDLE pBlockDataFile = CreateFile(blockFile.path.c_str(), 
							GENERIC_READ ,
							FILE_SHARE_READ, 
							NULL,
							OPEN_EXISTING, 
							FILE_ATTRIBUTE_NORMAL, 
							NULL);
						
						/*LARGE_INTEGER li;
						li.HighPart = 0;
						li.LowPart = 0;

						LARGE_INTEGER li1;
						li1.HighPart = 0;
						li1.LowPart = 0;

						SetFilePointerEx(pBlockDataFile,li,&li1,FILE_BEGIN);*/

						DWORD numRead;
						ReadFile(pBlockDataFile,vecBuf._Myfirst(),sizeof(PointXYZIPRGBA)*(DWORD)count,&numRead,NULL);

						CloseHandle(pBlockDataFile);
						pBlockDataFile = NULL;
						
						if (!m_pHlzWrite->m_header.isCompress)
						{
							// 不压缩
							// 内存点云拆分为坐标和强度,写入块内部点
							//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
							U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, m_iMin, m_iMax);
							pCurBlock->m_block.numPoint = cntWrite;

							// 写入坐标和强度
							m_pHlzWrite->WriteBlockData(pCurBlock->m_block, pPtXYZ, pPtInten, pPtColor, pCurBlock->m_block.numPoint);
						}
						else
						{
							// 压缩后输出
							// 重排序
							std::sort(vecBuf.begin(), vecBuf.end(), lessByXYZ);
							// 内存点云拆分为坐标和强度,写入块内部点
							//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
							U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, m_iMin, m_iMax);
							pCurBlock->m_block.numPoint = cntWrite;
							// 压缩，写入压缩后的数据
							CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
							ptXYZEncoder.encodePoints();
							// 写入坐标和强度
							m_pHlzWrite->WriteBlockData(pCurBlock->m_block, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, pCurBlock->m_block.numPoint);
							// 回收内存
							ptXYZEncoder.clear();
						}
						
						if (pPtXYZ)
						{
							delete []pPtXYZ;
							pPtXYZ = NULL;
						}
						if (pPtInten)
						{
							delete []pPtInten;
							pPtInten = NULL;
						}
						if (pPtColor != NULL)
						{
							delete[] pPtColor;
							pPtColor = NULL;
						}
					}
					else
					{
						// 分包写入
						// 切分后包的路径
						std::string dir = blockFile.path.c_str();//sBlockPath.c_str();
						dir = dir.substr(0, dir.find_last_of('.'));
						_mkdir(dir.c_str());

						char parcelsPath[MAX_PATH] = {0};
						sprintf_s(parcelsPath,"%s", dir.c_str());

						// 将块切分为包
						std::vector<ParcelFileInfo> splitParcels;

						SplitParcelFiles(pCurBlock->m_block.box,blockFile.path.c_str(),parcelsPath,splitParcels);

						// 只写块索引信息
						pCurBlock->m_block.hasSubParcel = 1;					
						m_pHlzWrite->WriteBlockData(pCurBlock->m_block, NULL, NULL, NULL, 0);

						// 将切分后包文件的点云写入
						U16 iPcl = 0;
						HdRefPoint refPtPcl;
						for (std::vector<ParcelFileInfo>::iterator it = splitParcels.begin();
							it != splitParcels.end(); it++)
						{
							if ((*it).numPoint == 0 )
							{
								continue;
							}
							CHdParcel* parcel = new CHdParcel;
							parcel->m_nParcelNo = iPcl;
							parcel->m_bkNo = pCurBlock->m_nBlockNo;
							iPcl++;
							parcel->m_parcel.box = (*it).box;
							parcel->m_parcel.numPoint = (*it).numPoint;
							refPtPcl.FromBox(parcel->m_parcel.box);

							// 数据点
							//win32 API文件操作代替c文件操作，提高io读取速度。   袁亮   20160909
							hd::u64  cnt = (*it).numPoint;
							vecBuf.resize(cnt);

							const hd::stringc& sParcelPath = (*it).path;
							HANDLE pParcelFile = CreateFile(sParcelPath.c_str(), 
								GENERIC_READ ,
								FILE_SHARE_READ, 
								NULL,
								OPEN_EXISTING, 
								FILE_ATTRIBUTE_NORMAL, 
								NULL);

							if(pParcelFile == INVALID_HANDLE_VALUE)
							{
								delete parcel;
								parcel = NULL;
								continue;
							}

							DWORD numRead;
							ReadFile(pParcelFile, vecBuf._Myfirst(),sizeof(PointXYZIPRGBA)*(DWORD)cnt, &numRead,NULL);

							::CloseHandle(pParcelFile);
							pParcelFile = NULL;

							/*const hd::stringc& sParcelPath = (*it).path;
							FILE* pParcelFile = fopen(sParcelPath.c_str(), "rb");
							if (pParcelFile == NULL)
							{
								delete parcel;
								parcel = NULL;
								continue;
							}

							vecBuf.resize(parcel->m_parcel.numPoint);
							fseek(pParcelFile, 0, SEEK_SET);
							fread(vecBuf._Myfirst, sizeof(PointXYZIPRGBA), parcel->m_parcel.numPoint, pParcelFile);

							// 关闭Parcel文件
							if (pParcelFile)
							{
								fclose(pParcelFile);
								pParcelFile = NULL;
							}*/

							if (!m_pHlzWrite->m_header.isCompress)
							{
								// 不压缩
								// 内存点云拆分为坐标和强度
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten, &pPtColor, m_iMin, m_iMax);
								parcel->m_parcel.numPoint = cntWrite;

								// 写入坐标和强度
								m_pHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, pPtColor, parcel->m_parcel.numPoint);
							}
							else
							{
								// 压缩
								// 重排序
								std::sort(vecBuf.begin(), vecBuf.end(), lessByXYZ);
								// 内存点云拆分为坐标和强度
								U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten, &pPtColor, m_iMin, m_iMax);
								parcel->m_parcel.numPoint = cntWrite;
								// 压缩
								CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
								ptXYZEncoder.encodePoints();
								// 写入坐标和强度
								m_pHlzWrite->WriteParcelData(parcel->m_parcel, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, parcel->m_parcel.numPoint);
							}
							// 添加包索引记录						
							pCurBlock->AddParcel(parcel);
														
							if (pPtXYZ)
							{
								delete []pPtXYZ;
								pPtXYZ = NULL;
							}
							if (pPtInten)
							{
								delete []pPtInten;
								pPtInten = NULL;
							}
							if (pPtColor != NULL)
							{
								delete[] pPtColor;
								pPtColor = NULL;
							}

							// 将包文件删除，节省磁盘空间
							err = remove(sParcelPath.c_str());
							if (err == -1)
							{
					//			perror("无法删除Parcel文件writeLevel0Data");
							}
						}//for (std::vector<ParcelFileInfo>::iterator it = splitParcels.begin();
						pCurBlock->Update();
					}// else分包写入

					/*if (pBlockDataFile!=INVALID_HANDLE_VALUE)
					{
						CloseHandle(pBlockDataFile);
						pBlockDataFile = NULL;
					}*/

					// 将块文件删除，节省磁盘空间
					err = remove(blockFile.path.c_str());

					if (err == -1)
					{
						perror("无法删除Block文件writeLevel0Data");
					}

				}//for (U32 i=0; i < arrayBlockFiles.size(); i++)

				pBlockSet->Update();
			}// else分块写入		
		}//for (map<U32, BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
		pLevel0->Update();
		pLevel0->UpdateScale();
	}

	BOOL CHlzBuilder::RemoveTempFiles(const char* path)
	{
		// 删除hls2hlz目录下所有临时文件		
		WIN32_FIND_DATAA finddata;
		HANDLE hFind;
		char * pdir;

		pdir=new char[strlen(path)+10];
		strcpy(pdir,path);
		if(path[strlen(path)-1]!='\\')
			strcat(pdir,"\\*.*");
		else
			strcat(pdir,"*.*");

		hFind = FindFirstFileA(pdir,&finddata);
		if(hFind == INVALID_HANDLE_VALUE)
			return FALSE;

		delete []pdir;
		pdir = NULL;
		do
		{
			pdir = new char[strlen(path)+strlen(finddata.cFileName)+2];
			sprintf(pdir,"%s\\%s",path,finddata.cFileName);
			if(strcmp(finddata.cFileName,".")==0
				||strcmp(finddata.cFileName,"..")==0)
			{
				RemoveDirectoryA(pdir);
				delete[] pdir;
				pdir = NULL;
				continue;
			}

			if((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
				DeleteFileA(pdir);
			else
				RemoveTempFiles(pdir);
			delete []pdir;
			pdir = NULL;
		}while(FindNextFileA(hFind,&finddata));

		FindClose(hFind);

		if(RemoveDirectoryA(path))
			return TRUE;
		else
			return FALSE;
	}

	//! 删除块集文件
	BOOL CHlzBuilder::RemoveBSFiles(std::map<U32, BlockSetFileInfo>& bsFiles)
	{
		for (map<U32,BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
			it != m_BlocksetFiles.end();it++)
		{	
			remove((it->second).path.c_str());
		}
		return TRUE;
	}

	void CHlzBuilder::CloseTempFiles()
	{
		return;
		for (map<U32,/*pair<FILE*,string>*/BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
			it != m_BlocksetFiles.end();it++)

		{	
			string strtmp = (it->second).path;
			//if ((it->second).pFile != NULL)
			//{
				// 关闭文件，同时把文件指针置空
				//fclose((it->second).pFile);
				//(it->second).pFile = NULL;
			//}
		}
	}

	//U32 CHlzBuilder::getBlockFiles(const char* strBSPath, std::vector<hd::stringc>& vecBlockFiles)
	//{
	//	if (_access(strBSPath, 00) != 0)
	//	{
	//		return 0;
	//	}
	//
	//	char szFind[MAX_PATH];
	//	//char szFile[MAX_PATH];
	//	WIN32_FIND_DATA FindFile;
	//	strcpy(szFind, strBSPath);
	//	strcat(szFind, "\\*.*");
	//	HANDLE hFind = ::FindFirstFile(szFind, &FindFile);
	//	if (INVALID_HANDLE_VALUE == hFind)
	//	{
	//		return 0;
	//	}
	//
	//	// 只查找当前文件夹下的文件，不查找子文件夹
	//	while (TRUE)
	//	{
	//		if ((FindFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FALSE)
	//		{
	//			char blockPath[MAX_PATH];
	//			sprintf_s(blockPath, "%s\\%s", strBSPath, FindFile.cFileName);
	//			vecBlockFiles.push_back(/*FindFile.cFileName*/blockPath);
	//		}
	//
	//		if (!FindNextFile(hFind, &FindFile))
	//		{
	//			break;
	//		}
	//	}
	//
	//	FindClose(hFind);
	//
	//	return vecBlockFiles.size();
	//}

	void CHlzBuilder::getBlockBox(
		const CHdVector3df& mid,				// 输入所属块集的范围中心
		const CHdVector3df& halfSize,			// 输入该块的大小1半
		CHdBox3df& box,					// 输出该块的空间范围
		U16 index)
	{
		switch (index)
		{
		case 0:
			{
				box.MinEdge = mid - halfSize;
				box.MaxEdge = mid;
			}
			break;
		case 1:
			{
				box.MinEdge.X = mid.X;
				box.MinEdge.Y = mid.Y - halfSize.Y;
				box.MinEdge.Z = mid.Z - halfSize.Z;

				box.MaxEdge = box.MinEdge + halfSize;		
			}
			break;
		case 2:
			{
				box.MinEdge.X = mid.X - halfSize.X;
				box.MinEdge.Y = mid.Y;
				box.MinEdge.Z = mid.Z - halfSize.Z;

				box.MaxEdge = box.MinEdge + halfSize;
			}
			break;
		case 3:
			{
				box.MinEdge.X = mid.X;
				box.MinEdge.Y = mid.Y;
				box.MinEdge.Z = mid.Z - halfSize.Z;

				box.MaxEdge = box.MinEdge + halfSize;
			}
			break;
		case 4:
			{
				box.MinEdge.X = mid.X - halfSize.X;
				box.MinEdge.Y = mid.Y - halfSize.Y;
				box.MinEdge.Z = mid.Z;

				box.MaxEdge = box.MinEdge + halfSize;
			}
			break;
		case 5:
			{
				box.MinEdge.X = mid.X;
				box.MinEdge.Y = mid.Y - halfSize.Y;
				box.MinEdge.Z = mid.Z;

				box.MaxEdge = box.MinEdge + halfSize;
			}
			break;
		case 6:
			{
				box.MinEdge.X = mid.X - halfSize.X;
				box.MinEdge.Y = mid.Y;
				box.MinEdge.Z = mid.Z;

				box.MaxEdge = box.MinEdge + halfSize;
			}
			break;
		case 7:
			{
				box.MinEdge = mid;

				box.MaxEdge = box.MinEdge + halfSize;			
			}
			break;
		default:
			break;

		}

	}

	void CHlzBuilder::getPtBlockIndex(PointXYZIPRGBA& pt, const CHdVector3df& center, U8& index)
	{	
		if (pt.x < center.X)
		{
			if (pt.y < center.Y)
			{
				if (pt.z < center.Z)
				{
					index = 0;		// 第0块
				}
				else
				{
					index = 4;		// 第4块
				}
			}
			else
			{
				if (pt.z < center.Z)
				{
					index = 2;		// 第2块
				}
				else
				{
					index = 6;		// 第6块
				}
			}
		}
		else if (pt.x >= center.X)
		{
			if (pt.y < center.Y)
			{
				if (pt.z < center.Z)
				{
					index = 1;		// 第1块
				}
				else
				{
					index = 5;		// 第5块
				}
			}
			else
			{
				if (pt.z < center.Z)
				{
					index = 3;		// 第3块
				}
				else
				{
					index = 7;		// 第7块
				}
			}
		}
	}

	//! 统计点云强度范围
	void CHlzBuilder::StatIntensity(
		IHLSReader* pHlsReader,			// 输入的hls读取对象
		U16& min,						// 输出的最小强度
		U16& max)						// 输出的最大强度
	{
		if (pHlsReader == NULL)
		{
			return;
		}

		min = 0xffff;
		max = 0;
		CLoopIndex* pLoopIndex = pHlsReader->GetLoopIndex();
		U32 loopCount = pHlsReader->GetLoopCount();
		U32 i = 0;
		for (;i < loopCount;i++)
		{
			min = MIN(pLoopIndex[i].m_loopIdx.minIntensity,min);
			max = MAX(pLoopIndex[i].m_loopIdx.maxIntensity,max);
		}

	}

	// 计算HLS文件中的数据的最大值、最小值 
	void CHlzBuilder::calHlsExtnt(hd::IHLSReader* reader, double& x_min, double& x_max,double& y_min,double& y_max,double& z_min, double& z_max)
	{
		// 打开文件

		if(reader == NULL)
			return ;


		// 首先利用文件头来取得最大值、最小值、
		double minX,maxX,minY,maxY,minZ,maxZ;
		reader->m_header.getGlobalExtent(minX, minY, minZ, maxX, maxY, maxZ);

		x_min = MIN(x_min, MIN(minX, maxX));
		y_min = MIN(y_min, MIN(minY, maxY));
		z_min = MIN(z_min, MIN(minZ, maxZ));

		x_max = MAX(x_max, MAX(maxX, minX));
		y_max = MAX(y_max, MAX(maxY, minY));
		z_max = MAX(z_max, MAX(maxZ, minZ));

		//如果头文件中最大值、最小值都为0
		// 那么对数据进行遍历分析，得到最大值、最小值
		if ( x_min ==  x_max && y_min ==  y_max &&
			x_min == 0 &&  y_max == 0 )
		{

			double m[16];
			reader->m_header.computeMatrix(m);
			hdVector<PointXYZIPRGBA> vecPts;
			int loopCount = reader->GetLoopCount();
			reader->GetLoopIndex();
			int n,k;
			double xyztmp[3];
			double xyzmin[3];  // 最小值
			double xyzmax[3];  // 最大值
			int flag = 0; // 用于标记第一次初始化xyz。 
			for (n = 0;n < loopCount;n++)
			{
				// 从文件读取坐标,n是文件中的点序号
				if(!reader->ReadLoop(vecPts,n))
					continue;

				for (k = 0;k < vecPts.size();k++)
				{
					PointXYZIPRGBA& pt = vecPts[k];
					if(!pt.isValid())
						continue;

					xyztmp[0]  =  pt.x;
					xyztmp[1]  =  pt.y;
					xyztmp[2]  =  pt.z;
					// 获得全局坐标
					hdHomogeneousTransformPoint(m,xyztmp[0],xyztmp[1],xyztmp[2]);
					flag++;
					// 第一次赋值
					if ( flag == 1)
					{
						VecCopy3fv(xyzmin, xyztmp);
						VecCopy3fv(xyzmin, xyztmp);

					}
					// 进行数据更新
					else
					{
						VecUpdateMinMax3dv(xyzmin, xyzmax, xyztmp);
					}

				}

			}

			// 重新更新m_boundBox最大值和最小值
			x_min = MIN(x_min, xyzmin[0]);
			y_min = MIN(y_min, xyzmin[1]);
			z_min = MIN(z_min, xyzmin[2]);
			x_max = MAX(x_max, xyzmax[0]);
			y_max = MAX(y_max, xyzmax[1]);
			z_max = MAX(z_max, xyzmax[2]);

		}
	}

	// 外部接口设置是否重采样0层，0层采样精度
	void CHlzBuilder::SetResampleZero( bool bResample,double dGridPrecision )
	{
		// 参数设置
		m_bResampleZero = bResample;
		m_dGridPrecision = dGridPrecision;
	}

}
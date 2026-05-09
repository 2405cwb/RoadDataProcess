#include "StdAfx.h"
#include "PointCloudCache.h"
#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <algorithm>
#include <cctype>
#include <string.h>
#include "..\hdHlslib\HLSReadOpener.h"
CPointCloudCache* CPointCloudCache::m_pCache = NULL;

CPointCloudCache::CPointCloudCache(void)
{
	m_nMaxPtSize		= 20000000;						//点云缓冲区最大点数默认设置为二千万个点
	m_nMaxPtSizePerFile = 5000000;						//单个文件最大加载点云数为五百万个点
	m_hMsgReceiver		= NULL;
	m_nMsgID			= WM_USER + 0x00000101;			//点云即将被删除的消息，WPARAM存放PointCloud指针
	m_editMode = 1;
	m_bAverLoad = true;
	m_load_simple_mode = E_LOAD_NUM_SIMPLE;
	m_sapce_filter_dist = 0.02f;
	static CHdCacheCleaner mCleaner;
}


CPointCloudCache::~CPointCloudCache(void)
{
	//清除所有点云
	unsigned int i;
	unsigned int nCount = m_pPcds.size();
	for (i = 0; i<nCount; i++)
	{
		if (m_pPcds[i])
		{
			delete m_pPcds[i];
			m_pPcds[i] = NULL;
		}
	}
	m_pPcds.clear();

	nCount = m_pDems.size();
	for (i = 0; i < nCount; i++)
	{
		if (m_pDems[i])
		{
			delete m_pDems[i];
			m_pDems[i] = NULL;
		}
	}
	m_pDems.clear();

	nCount = m_pTins.size();
	for (i = 0; i < nCount; i++)
	{
		if (m_pTins[i])
		{
			delete m_pTins[i];
			m_pTins[i] = NULL;
		}
	}
	m_pTins.clear();

	nCount = m_pSeaPcds.size();
	for (i=0; i<nCount; i++)
	{
		if (m_pSeaPcds[i])
		{
			delete m_pSeaPcds[i];
			m_pSeaPcds[i] = NULL;
		}
	}

	m_pSeaPcds.clear();

}
//! 设置点云编辑模式
void CPointCloudCache::setEditMode(int mode)
{
	if (mode == 0 || mode == 1)
	{
		if (m_editMode != mode)
		{
			// 浏览模式和编辑模式变换,需要卸载点云
			RemoveAllPointCloud();
		}
		m_editMode = mode;
	}
}

PointCloud* CPointCloudCache::Cache(const char* strPcdFile,bool bOnlyHeader,void (*loadCallback)(float,const char*))
{
	std::string strFile = strPcdFile;
	if (strFile == "")
	{
		return NULL;
	}

	// 部分中文用tolower转换为乱码，此处更改为toupper -zhubo
	//std::transform(strFile.begin(), strFile.end(), strFile.begin(),tolower);

	unsigned int i;
	unsigned int nCount = m_pPcds.size();
	for (i = 0; i<nCount; i++)
	{
		//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
		if (0 == stricmp(strFile.data(), m_pPcds[i]->strPcdFile.data()))
		{
			// 如果需要加载数据，而找到的点云只加载了文件头，则需要重新加载数据 
			PCD* pCurPcd = m_pPcds[i];
			if (pCurPcd->pPointCloud->count() == 0 && !bOnlyHeader)
			{
				if (m_load_simple_mode == E_LOAD_NUM_SIMPLE)
				{
					// 根据外部设置，非平均加载时，更新最大加载阈值m_nMaxPtSizePerFile
					if (!m_bAverLoad)
					{
						UpdateMaxPtSizePerFile(pCurPcd->pPointCloud->m_header.number_of_point_records);
					}

					// 计算实际load的点数
					U64 curLoadCount = 0;
					U32 simple = 0;

					if (pCurPcd->pPointCloud->isNormalPointCloud())
					{
						simple = (U32)sqrt(1.0 * pCurPcd->pPointCloud->m_header.number_of_point_records / m_nMaxPtSizePerFile) + 1;
						curLoadCount = pCurPcd->pPointCloud->m_header.number_of_point_records / (simple * simple);
					}
					else
					{
						simple = (U32)(1.0 * pCurPcd->pPointCloud->m_header.number_of_point_records / m_nMaxPtSizePerFile) + 1;
						curLoadCount = pCurPcd->pPointCloud->m_header.number_of_point_records / simple;
					}

					//CheckBuffer(MIN(m_nMaxPtSizePerFile,pCurPcd->pPointCloud->m_header.number_of_point_records))
					if(!CheckBuffer(curLoadCount))
						return NULL;
					//设置单个文件最大加载点个数
					pCurPcd->pPointCloud->setLoadSimple(m_nMaxPtSizePerFile);
					pCurPcd->pPointCloud->setLoadSimpleMode(E_LOAD_NUM_SIMPLE);
	
				}
				else if (m_load_simple_mode == E_LOAD_SPACE_SIMPLE)
				{
					pCurPcd->pPointCloud->setSpaceSimpleDist(m_sapce_filter_dist);
					pCurPcd->pPointCloud->setLoadSimpleMode(E_LOAD_SPACE_SIMPLE);
				}
				
				//设置编辑模式
				pCurPcd->pPointCloud->setEditMode(m_editMode);
				//加载hls点云数据
				pCurPcd->pPointCloud->loadHlsData(loadCallback);

			}
			//m_pPcds.erase(m_pPcds.begin() + i);
			//m_pPcds.insert(m_pPcds.begin(), pCurPcd);

			return pCurPcd->pPointCloud;	
		}
	}

	//如果没有找到，则加载该点云
	if (i == nCount)
	{
		BOOL bLoaded = FALSE;
		PointCloud* newPointCloud = new PointCloud();

		string strExt = strFile.substr(strFile.find_last_of('.') + 1);
		transform(strExt.begin(), strExt.end(), strExt.begin(), tolower);
		if (strExt == "hls")
		{
			//加载hls点云文件头
			bLoaded = newPointCloud->loadHlsFileHeader(strFile.data());
			if (bLoaded && !bOnlyHeader)
			{
				if (m_load_simple_mode == E_LOAD_NUM_SIMPLE)
				{
					// 根据外部设置，非平均加载时，更新最大加载阈值m_nMaxPtSizePerFile
					if (!m_bAverLoad)
					{
						UpdateMaxPtSizePerFile(newPointCloud->m_header.number_of_point_records);
					}

					// 计算实际load的点数
					U64 curLoadCount = 0;
					//U32 simple = newPointCloud->m_header.number_of_point_records / (m_nMaxPtSizePerFile * 2) + 1;
					U32 simple = 0;
					if (newPointCloud->isNormalPointCloud())
					{
						simple = (U32)sqrt(1.0 * newPointCloud->m_header.number_of_point_records / m_nMaxPtSizePerFile) + 1;
						curLoadCount = newPointCloud->m_header.number_of_point_records / (simple * simple);
					}
					else
					{
						simple = (U32)(1.0 * newPointCloud->m_header.number_of_point_records / m_nMaxPtSizePerFile) + 1;
						curLoadCount = newPointCloud->m_header.number_of_point_records / simple;
					}

					// CheckBuffer(MIN(m_nMaxPtSizePerFile,newPointCloud->m_header.number_of_point_records))
					if(!CheckBuffer(curLoadCount))
						return NULL;
	
					//设置单个文件最大加载点个数
					newPointCloud->setLoadSimple(m_nMaxPtSizePerFile);
					newPointCloud->setLoadSimpleMode(E_LOAD_NUM_SIMPLE);

					
				}
				else if (m_load_simple_mode == E_LOAD_SPACE_SIMPLE)
				{
					newPointCloud->setSpaceSimpleDist(m_sapce_filter_dist);
					newPointCloud->setLoadSimpleMode(E_LOAD_SPACE_SIMPLE);
				}

				//设置编辑模式
				newPointCloud->setEditMode(m_editMode);
				//加载hls点云数据
				bLoaded = newPointCloud->loadHlsData(loadCallback);
				
			}
		}
		/*else if (strExt == "las")
		{
		bLoaded = newPointCloud->loadLasFile(strFile.data(), NULL);
		}*/

		if (bLoaded)
		{
			PCD* newPCD = new PCD;
			newPCD->strPcdFile = strFile;
			newPCD->pPointCloud = newPointCloud;

			//将新缓存的点云插入到队列的最前面
			m_pPcds.insert(m_pPcds.begin(), newPCD);

			return newPointCloud;
		}
		else
		{
			delete newPointCloud;
			return NULL;
		}
	}

	return NULL;
}


PointCloud* CPointCloudCache::Cache(const char* strPcdFile,hd::f64 nStartCol,hd::f64 nEndCol,bool bOnlyHeader,void (*loadCallback)(float,const char*))
{
	std::string strFile = strPcdFile;
	if (strFile == "")
	{
		return NULL;
	}

	// 部分中文用tolower转换为乱码，此处更改为toupper -zhubo
	//std::transform(strFile.begin(), strFile.end(), strFile.begin(),tolower);

	unsigned int i;
	unsigned int nCount = m_pPcds.size();
	for (i = 0; i<nCount; i++)
	{
		//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
		if (0 == stricmp(strFile.data(), m_pPcds[i]->strPcdFile.data()))
		{
			// 如果需要加载数据，而找到的点云只加载了文件头，则需要重新加载数据 
			PCD* pCurPcd = m_pPcds[i];
			if (pCurPcd->pPointCloud->count() == 0 && !bOnlyHeader)
			{
				// 计算实际load的点数
				U64 curLoadCount = 0;
				U64 nTotalCount = (U64)(pCurPcd->pPointCloud->m_header.number_of_point_records * (nEndCol - nStartCol));
				U32 simple = (U32)sqrt(1.0 * nTotalCount / m_nMaxPtSizePerFile) + 1;

				if (pCurPcd->pPointCloud->isNormalPointCloud())
				{
					curLoadCount = nTotalCount / (simple * simple);
				}
				else
				{
					curLoadCount = nTotalCount / simple;
				}

				// MIN(m_nMaxPtSizePerFile,pCurPcd->pPointCloud->m_header.number_of_point_records)
				if(!CheckBuffer(curLoadCount))
					return NULL;

				//设置编辑模式
				pCurPcd->pPointCloud->setEditMode(m_editMode);
				//设置单个文件最大加载点个数
				pCurPcd->pPointCloud->setLoadSimple(m_nMaxPtSizePerFile);
				//double loopCount = pCurPcd->pPointCloud->m_header.number_of_col;
				//double startScale = (double)nStartCol/loopCount;
				//double eneScale = (double)nEndCol/loopCount;
				//加载hls点云数据
				pCurPcd->pPointCloud->loadHlsByScale(nStartCol,nEndCol,loadCallback);
			}
			//m_pPcds.erase(m_pPcds.begin() + i);
			//m_pPcds.insert(m_pPcds.begin(), pCurPcd);

			return pCurPcd->pPointCloud;	
		}
	}

	//如果没有找到，则加载该点云
	if (i == nCount)
	{
		BOOL bLoaded = FALSE;
		PointCloud* newPointCloud = new PointCloud();

		string strExt = strFile.substr(strFile.find_last_of('.') + 1);
		transform(strExt.begin(), strExt.end(), strExt.begin(), tolower);
		if (strExt == "hls")
		{
			//加载hls点云文件头
			bLoaded = newPointCloud->loadHlsFileHeader(strFile.data());
			if (bLoaded && !bOnlyHeader)
			{
				// 计算实际load的点数
				U64 curLoadCount = 0;
				U64 nTotalCount = (U64)(newPointCloud->m_header.number_of_point_records * (nEndCol - nStartCol));
				U32 simple = (U32)sqrt(1.0 * nTotalCount / m_nMaxPtSizePerFile) + 1;

				if (newPointCloud->isNormalPointCloud())
				{
					curLoadCount = nTotalCount / (simple * simple);
				}
				else
				{
					curLoadCount = nTotalCount / simple;
				}

				// MIN(m_nMaxPtSizePerFile,newPointCloud->m_header.number_of_point_records)
				if(!CheckBuffer(curLoadCount))
					return NULL;
				//设置编辑模式
				newPointCloud->setEditMode(m_editMode);
				//设置单个文件最大加载点个数
				newPointCloud->setLoadSimple(m_nMaxPtSizePerFile);
				//double loopCount = newPointCloud->m_header.number_of_col;
				double startScale = nStartCol;//(double)nStartCol/loopCount;
				double endScale = nEndCol;//(double)nEndCol/loopCount;

				//加载hls点云数据
				bLoaded = newPointCloud->loadHlsByScale(startScale < endScale ? startScale : endScale,
					startScale < endScale ? endScale : startScale,loadCallback);
			}
		}

		if (bLoaded)
		{
			PCD* newPCD = new PCD;
			newPCD->strPcdFile = strFile;
			newPCD->pPointCloud = newPointCloud;

			//将新缓存的点云插入到队列的最前面
			m_pPcds.insert(m_pPcds.begin(), newPCD);

			return newPointCloud;
		}
		else
		{
			delete newPointCloud;
			return NULL;
		}
	}

	return NULL;
}


U64 CPointCloudCache::GetSize()
{
	unsigned int i = 0;
	U64 nTotalSize = 0;
	unsigned int nCount = m_pPcds.size();
	for (i = 0; i<nCount; i++)
	{
		nTotalSize += m_pPcds[i]->pPointCloud->count();
	}

	return nTotalSize;
}

void CPointCloudCache::Remove(PointCloud* pcd)
{
	for(vector<PCD*>::iterator it = m_pPcds.begin();
		it != m_pPcds.end();it++)
	{
		if ((*it)->pPointCloud == pcd)
		{
			//向消息接收者发送消息，告知某个点云即将被删除
			if (m_hMsgReceiver)
			{
				::SendMessage(m_hMsgReceiver, m_nMsgID, (WPARAM)(*it), (LPARAM)0);
			}

			std::string strPath = pcd->GetPointCloudPath();
			pcd->clear();
			pcd->loadHlsFileHeader(strPath.c_str());
			//delete (*it);
			//m_pPcds.erase(it);
			break;
		}
	}
}

void CPointCloudCache::Remove(const char* strPcd)
{
	if (!strPcd)
	{
		return;
	}
	std::string strFile = strPcd;
	//std::transform(strFile.begin(), strFile.end(), strFile.begin(), tolower);

	for(vector<PCD*>::iterator it = m_pPcds.begin();
		it != m_pPcds.end();it++)
	{
		PointCloud* pcd = (*it)->pPointCloud;
		string strPcd = pcd->GetPointCloudPath();
		std::transform(strPcd.begin(), strPcd.end(), strPcd.begin(),tolower);
		if (0 == stricmp(strPcd.data(), strFile.data()))
		{
			//向消息接收者发送消息，告知某个点云即将被删除
			if (m_hMsgReceiver)
			{
				::SendMessage(m_hMsgReceiver, m_nMsgID, (WPARAM)(*it), (LPARAM)0);
			}
			std::string strPath = pcd->GetPointCloudPath();
			pcd->clear();
			pcd->loadHlsFileHeader(strPath.c_str());
			//delete (*it);
			//m_pPcds.erase(it);
			break;
		}
	}
}

void CPointCloudCache::RemoveAllPointCloud()
{
	for(vector<PCD*>::iterator it = m_pPcds.begin();
		it != m_pPcds.end();it++)
	{
		//向消息接收者发送消息，告知某个点云即将被删除
		if (m_hMsgReceiver)
		{
			::SendMessage(m_hMsgReceiver, m_nMsgID, (WPARAM)(*it), (LPARAM)0);
		}

		delete (*it);
	}

	m_pPcds.clear();
}

// 获取可用内存
U64 CPointCloudCache::GetAvailPhy()
{
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);
	GlobalMemoryStatusEx (&statex);

	// 判断当前操作系统为32位或64位
	BOOL is64Bit = IsWow64();

	// 判断当前进程可使用空间
	U64 availPhys = statex.ullAvailPhys;
	PROCESS_MEMORY_COUNTERS pmc;
	int* p = NULL;
	if (sizeof(p) == 4)// 通过检测指针大小判断是否为32位程序
	{
		// 32位系统计算已使用内存(实际win32可用内存最大只有3.25G)
		U64 loadPhys = statex.ullTotalPhys - statex.ullAvailPhys;

		U64 u325GB = (U64)(3.25 * 1024 * 1024 * 1024);

		// 判断，如果总内存大于u325GB值，表明是在win64下安装的win32软件，取较小值
		if (!is64Bit) // 32位系统
		{
			// 32位系统，最多可用内存3.25G，与实际内存比较，取小值
			if (u325GB <= statex.ullTotalPhys)
			{
				availPhys = u325GB - loadPhys;
			}
		}
		else  // 若为64位系统，则为实际可用内存
		{
			availPhys = statex.ullAvailPhys;
		}


		// 如果是32位程序,每个进程最多只能占用2GB
		HANDLE handle = GetCurrentProcess();
		// 获取当前进程占用内存pmc.WorkingSetSize
		GetProcessMemoryInfo(handle,&pmc,sizeof(pmc));
		U64 u2GB = ((U64)2) * 1024 * 1024 * 1024;
		//availPhys = MIN(statex.ullAvailPhys,u2GB - pmc.WorkingSetSize);
		availPhys = MIN(availPhys,u2GB - pmc.WorkingSetSize);
	}
	return availPhys;
}

BOOL CPointCloudCache::CheckBuffer(U64 toLoadCount)
{
	U64 nTotalSize = GetSize();
	//根据实际物理可用内存判断,gsl-2013/6/1
	U64 availPhys = GetAvailPhy();

	// 预留不需要太多
	U64 pcdSize = ((U64)toLoadCount) * sizeof(PointXYZIPRGBA) * 6 / 5;//sizeof(PointXYZIPRGBA)* 3 / 2

	return (availPhys > pcdSize);//直接返回，不自动卸载已加载点云

	unsigned int nCount = m_pPcds.size();

	// 如果内存已经小于点云文件所占内存,则删除最后一个点云
	while (availPhys < pcdSize && nCount > 0)
	{
		U64 nLastPcdSize = m_pPcds[nCount - 1]->pPointCloud->count();
		if (nLastPcdSize <= 0)
		{
			nCount--;
			continue;
		}
		//向消息接收者发送消息，告知某个点云即将被删除
		if (m_hMsgReceiver)
		{
			::SendMessage(m_hMsgReceiver, m_nMsgID, (WPARAM)(m_pPcds[nCount - 1]), (LPARAM)0);
		}

		//删除队列中最后一个点云
		delete m_pPcds[nCount - 1];
		m_pPcds.erase(m_pPcds.begin() + nCount - 1);

		nTotalSize -= nLastPcdSize;

		availPhys = GetAvailPhy();
		nCount = m_pPcds.size();
	}
}

bool CPointCloudCache::IsPointCloudLoaded(const char* strPcdFile,bool& bOnlyHeader)
{
	bool bLoaded = false;

	std::string strFile = strPcdFile;
	//std::transform(strFile.begin(), strFile.end(), strFile.begin(), tolower);

	unsigned int i;
	unsigned int nCount = m_pPcds.size();
	for (i = 0; i<nCount; i++)
	{
		//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
		if (0 == stricmp(strFile.data(), m_pPcds[i]->strPcdFile.data()))
		{
			bLoaded = true;

			if (m_pPcds[i]->pPointCloud->count() == 0)
			{
				bOnlyHeader = true;
			}
			else
			{
				bOnlyHeader = false;
			}
		}
	}
	return bLoaded;
}

PointCloud* CPointCloudCache::GetLoadedPointCloud( const char* strPcdFile )
{
	try
	{	
		/*std::string strFile = strPcdFile;*/
		//std::transform(strFile.begin(), strFile.end(), strFile.begin(), tolower);

		unsigned int i;
		unsigned int nCount = m_pPcds.size();
		for (i = 0; i<nCount; i++)
		{
			//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
			if (0 == stricmp(strPcdFile, m_pPcds[i]->strPcdFile.data()))
			{
				// 如果需要加载数据，而找到的点云只加载了文件头，则需要重新加载数据 
				PCD* pCurPcd = m_pPcds[i];

				m_pPcds.erase(m_pPcds.begin() + i);
				m_pPcds.insert(m_pPcds.begin(), pCurPcd);

				return pCurPcd->pPointCloud;
			}
		}
	}
	catch (...)
	{
		return NULL;
	}
	return NULL;
}

BOOL CPointCloudCache::CheckBufferForDem(U64 toLoadCount)
{
	unsigned int i = 0;
	U64 nTotalSize = 0;
	unsigned int nCount = m_pDems.size();
	for (i = 0; i < nCount; i++)
	{
		nTotalSize += m_pDems[i]->pDem->count();
	}

	U64 availPhys = GetAvailPhy();

	// 计算加载点云需要占用内存大小
	U64 pcdSize = ((U64)toLoadCount) * sizeof(S3DVertex2TCoords) * 3 / 2;//sizeof(PointXYZIPRGBA)

	return (availPhys > pcdSize);//直接返回，不自动卸载已加载点云

	// 如果内存已经小于点云文件所占内存,则删除最后一个点云
	while (availPhys < pcdSize && nCount > 0)
	{
		U64 nLastPcdSize = m_pDems[nCount - 1]->pDem->count();
		if (nLastPcdSize <= 0)
		{
			nCount--;
			continue;
		}
		//向消息接收者发送消息，告知某个点云即将被删除
		if (m_hMsgReceiver)
		{
			::SendMessage(m_hMsgReceiver, m_nMsgID, (WPARAM)(m_pDems[nCount - 1]), (LPARAM)0);
		}

		//删除队列中最后一个点云
		delete m_pDems[nCount - 1];
		m_pDems.erase(m_pDems.begin() + nCount - 1);

		nTotalSize -= nLastPcdSize;

		availPhys = GetAvailPhy();
		nCount = m_pDems.size();
	}
}

CHdModelPointCloud* CPointCloudCache::CacheDem(const char* strPcdFile, void (*loadCallback)(float,const char*))
{
	std::string strFile = strPcdFile;
	if (strFile == "")
	{
		return NULL;
	}

	// 部分中文用tolower转换为乱码，此处更改为toupper -zhubo
	//std::transform(strFile.begin(), strFile.end(), strFile.begin(),tolower);

	unsigned int i;
	unsigned int nCount = m_pDems.size();
	for (i = 0; i < nCount; i++)
	{
		//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
		if (0 == stricmp(strFile.data(), m_pDems[i]->strPcdFile.data()))
		{
			// 如果需要加载数据，而找到的点云只加载了文件头，则需要重新加载数据 
			DEM* pCurDem = m_pDems[i];
			if (pCurDem->pDem->count() == 0)
			{
				if(!CheckBufferForDem(MIN(m_nMaxPtSizePerFile, nCount)))
					return NULL;

				//加载hls点云数据
				pCurDem->pDem->loadFile(strPcdFile, loadCallback);
			}
			return pCurDem->pDem;	
		}
	}

	//如果没有找到，则加载该点云
	if (i == nCount)
	{
		BOOL bLoaded = FALSE;
		CHdModelPointCloud* newDEM = new CHdModelPointCloud();

		string strExt = strFile.substr(strFile.find_last_of('.') + 1);
		transform(strExt.begin(), strExt.end(), strExt.begin(), tolower);
		if (strExt == "hls" || strExt == "las" || strExt == "tif" || strExt == "asc")
		{
			if(!CheckBufferForDem(MIN(m_nMaxPtSizePerFile,nCount)))
				return NULL;

			//加载hls点云数据
			bLoaded = newDEM->loadFile(strPcdFile, loadCallback);	
		}

		if (bLoaded)
		{
			DEM* newdem = new DEM;
			newdem->strPcdFile = strFile;
			newdem->pDem = newDEM;

			//将新缓存的点云插入到队列的最前面
			m_pDems.insert(m_pDems.begin(), newdem);			
			return newDEM;
		}
		else
		{
			delete newDEM;
			return NULL;
		}
	}

	return NULL;
}

// 移除点云
void CPointCloudCache::Remove(CHdModelPointCloud* dem)
{
	for(vector<DEM*>::iterator it = m_pDems.begin();
		it != m_pDems.end();it++)
	{
		if ((*it)->pDem == dem)
		{
			//向消息接收者发送消息，告知某个点云即将被删除
			if (m_hMsgReceiver)
			{
				::SendMessage(m_hMsgReceiver, m_nDemMsgID, (WPARAM)(*it), (LPARAM)0);
			}

			dem->clear();

			// 将数据卸载后，需要在DEM列表中将该数据删除 [2014/09/15 危迟]
			delete (*it);
			m_pDems.erase(it);

			break;
		}
	}
}

// 移除所有点云	
void CPointCloudCache::RemoveAllDEMPcd()
{
	for(vector<DEM*>::iterator it = m_pDems.begin();
		it != m_pDems.end();it++)
	{
		//向消息接收者发送消息，告知某个点云即将被删除
		if (m_hMsgReceiver)
		{
			::SendMessage(m_hMsgReceiver, m_nDemMsgID, (WPARAM)(*it), (LPARAM)0);
		}

		delete (*it);
	}

	m_pDems.clear();
}

// 点云是否加载 及是否只加载文件头
bool CPointCloudCache::IsDEMPcdLoaded(const char* strPcdFile,bool& bOnlyHeader)
{
	bool bLoaded = false;

	std::string strFile = strPcdFile;
	//std::transform(strFile.begin(), strFile.end(), strFile.begin(), tolower);

	unsigned int i;
	unsigned int nCount = m_pDems.size();
	for (i = 0; i < nCount; i++)
	{
		//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
		if (0 == stricmp(strFile.data(), m_pDems[i]->strPcdFile.data()))
		{
			bLoaded = true;

			if (m_pDems[i]->pDem->count() == 0)
			{
				bOnlyHeader = true;
			}
			else
			{
				bOnlyHeader = false;
			}
		}
	}
	return bLoaded;
}

// 获取存在的点云
CHdModelPointCloud* CPointCloudCache::GetLoadedDemPcd(const char* strPcdFile)
{
	try
	{	
		std::string strFile = strPcdFile;
		//std::transform(strFile.begin(), strFile.end(), strFile.begin(), tolower);

		unsigned int i;
		unsigned int nCount = m_pDems.size();
		for (i = 0; i < nCount; i++)
		{
			//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
			if (0 == stricmp(strFile.data(), m_pDems[i]->strPcdFile.data()))
			{
				// 如果需要加载数据，而找到的点云只加载了文件头，则需要重新加载数据 
				DEM* pCurDem = m_pDems[i];

				if (pCurDem->pDem->count() == 0)
				{

				}
				m_pDems.erase(m_pDems.begin() + i);
				m_pDems.insert(m_pDems.begin(), pCurDem);

				return pCurDem->pDem;
			}
		}
	}
	catch (...)
	{
		return NULL;
	}
	return NULL;
}

BOOL CPointCloudCache::CheckBufferForTIN(U64 toLoadCount)
{
	U64 nTotalSize = GetSize();
	//根据实际物理可用内存判断,gsl-2013/6/1
	U64 availPhys = GetAvailPhy();

	// 计算加载点云需要占用内存大小
	U64 pcdSize = ((U64)toLoadCount) * sizeof(S3DVertex2TCoords) * 3 / 2;//sizeof(PointXYZIPRGBA)

	return (availPhys > pcdSize);			// 直接返回，不自动卸载已加载点云
	unsigned int nCount = m_pTins.size();	// TIN内存队列

	// 如果内存已经小于点云文件所占内存,则删除最后一个点云
	while (availPhys < pcdSize && nCount > 0)
	{
		U64 nLastPcdSize = m_pTins[nCount - 1]->pTIN->count();
		if (nLastPcdSize <= 0)
		{
			nCount--;
			continue;
		}
		//向消息接收者发送消息，告知某个点云即将被删除
		if (m_hMsgReceiver)
		{
			::SendMessage(m_hMsgReceiver, m_nTinMsgID, (WPARAM)(m_pTins[nCount - 1]), (LPARAM)0);
		}

		//删除队列中最后一个点云
		delete m_pTins[nCount - 1];
		m_pTins.erase(m_pTins.begin() + nCount - 1);

		nTotalSize -= nLastPcdSize;

		availPhys = GetAvailPhy();
		nCount = m_pTins.size();
	}
}
// 缓存一个DEM文件 fengjing
CHdTINPointCloud* CPointCloudCache::CacheTin(const char* strPcdFile, void (*loadCallback)(float,const char*))
{
	std::string strFile = strPcdFile;
	// 判空
	if (strFile == "")
	{
		return NULL;
	}

	// 部分中文用tolower转换为乱码，此处更改为toupper
	std::transform(strFile.begin(), strFile.end(), strFile.begin(),tolower);

	unsigned int i;							// 计数器
	unsigned int nCount = m_pTins.size();	// TIN队列
	// 遍历点云队列
	for (i = 0; i < nCount; i++)
	{
		//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
		if (0 == stricmp(strFile.data(), m_pTins[i]->strPcdFile.data()))
		{
			// 如果需要加载数据，而找到的点云只加载了文件头，则需要重新加载数据 
			TIN* pCurPcd = m_pTins[i];
			if (pCurPcd->pTIN->count() == 0)
			{
				if(!CheckBufferForTIN(MIN(m_nMaxPtSizePerFile, nCount)))
					return NULL;

				//加载hls点云数据
				pCurPcd->pTIN->loadFile(strPcdFile, loadCallback);
			}
			return pCurPcd->pTIN;	
		}
	}

	//如果没有找到，则加载该点云
	if (i == nCount)
	{
		BOOL bLoaded = FALSE;
		CHdTINPointCloud* newTIN= new CHdTINPointCloud();

		string strExt = strFile.substr(strFile.find_last_of('.') + 1);
		transform(strExt.begin(), strExt.end(), strExt.begin(), tolower);
		if (strExt == "hls" || strExt == "las" || strExt == "tif" || strExt == "obj"|| strExt == "vtk")
		{

			if(!CheckBufferForTIN(MIN(m_nMaxPtSizePerFile, nCount)))
				return NULL;

			//加载hls点云数据
			bLoaded = newTIN->loadFile(strPcdFile, loadCallback);		
		}

		if (bLoaded)
		{
			TIN* newPCD = new TIN;
			newPCD->strPcdFile = strFile;
			newPCD->pTIN = newTIN;

			//将新缓存的点云插入到队列的最前面
			m_pTins.insert(m_pTins.begin(), newPCD);

			return newTIN;
		}
		else
		{
			delete newTIN;
			return NULL;
		}
	}
	return NULL;
}

// 移除DEM fengjing
void CPointCloudCache::Remove(CHdTINPointCloud* tin)
{
	for(vector<TIN*>::iterator it = m_pTins.begin();
		it != m_pTins.end();it++)
	{
		if ((*it)->pTIN == tin)
		{
			//向消息接收者发送消息，告知某个点云即将被删除
			if (m_hMsgReceiver)
			{
				::SendMessage(m_hMsgReceiver, m_nTinMsgID, (WPARAM)(*it), (LPARAM)0);
			}

			// 清除点云
			tin->clear();
			break;
		}
	}
}

// 移除所有点云	fengjing
void CPointCloudCache::RemoveAllTinPcd()
{
	for(vector<TIN*>::iterator it = m_pTins.begin();
		it != m_pTins.end();it++)
	{
		//向消息接收者发送消息，告知某个点云即将被删除
		if (m_hMsgReceiver)
		{
			::SendMessage(m_hMsgReceiver, m_nTinMsgID, (WPARAM)(*it), (LPARAM)0);
		}

		delete (*it);
	}

	// 清除TIN队列
	m_pTins.clear();
}

// 点云是否加载 fengjing
bool CPointCloudCache::IsTinPcdLoaded(const char* strPcdFile,bool& bOnlyHeader)
{
	bool bLoaded = false;

	std::string strFile = strPcdFile;
	//std::transform(strFile.begin(), strFile.end(), strFile.begin(), tolower);

	unsigned int i;
	unsigned int nCount = m_pTins.size();
	for (i = 0; i < nCount; i++)
	{
		//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
		if (0 == stricmp(strFile.data(), m_pTins[i]->strPcdFile.data()))
		{
			bLoaded = true;

			if (m_pTins[i]->pTIN->count() == 0)
			{
				bOnlyHeader = true;
			}
			else
			{
				bOnlyHeader = false;
			}
		}
	}
	return bLoaded;
}

// 获取存在的点云 fengjing
CHdTINPointCloud* CPointCloudCache::GetLoadedTinPcd(const char* strPcdFile)
{
	try
	{	
		std::string strFile = strPcdFile;
		//std::transform(strFile.begin(), strFile.end(), strFile.begin(), tolower);

		unsigned int i;
		unsigned int nCount = m_pTins.size();
		for (i = 0; i < nCount; i++)
		{
			//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
			if (0 == stricmp(strFile.data(), m_pTins[i]->strPcdFile.data()))
			{
				// 如果需要加载数据，而找到的点云只加载了文件头，则需要重新加载数据 
				TIN* pCurTin = m_pTins[i];

				m_pTins.erase(m_pTins.begin() + i);
				m_pTins.insert(m_pTins.begin(), pCurTin);

				return pCurTin->pTIN;
			}
		}
	}
	catch (...)
	{
		return NULL;
	}
	return NULL;
}

// 缓存一个SeaPcd文件
CSeaPointCloud* CPointCloudCache::CacheSeaPcd(const char* strPcdFile, void (*loadCallback)(float,const char*) )
{
	std::string strFile = strPcdFile;

	// 判空
	if (strFile == "")
	{
		return NULL;
	}

	// 部分中文用tolower转换为乱码，此处更改为toupper
	//std::transform(strFile.begin(), strFile.end(), strFile.begin(),tolower);

	unsigned int i;							// 计数器
	unsigned int nCount = m_pSeaPcds.size();	// 海量点云队列

	// 遍历点云队列
	for (i = 0; i < nCount; i++)
	{
		//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
		if (0 == stricmp(strFile.data(), m_pSeaPcds[i]->strPcdFile.data()))
		{			
			SEAPCD* pCurPcd = m_pSeaPcds[i];
			return pCurPcd->pSEAPCD;
		}
	}

	//如果没有找到，则加载该点云
	if (i == nCount)
	{
		BOOL bOpened = FALSE;
		CSeaPointCloud* pNewSeaPcd= new CSeaPointCloud();

		string strExt = strFile.substr(strFile.find_last_of('.') + 1);
		transform(strExt.begin(), strExt.end(), strExt.begin(), tolower);
		if (strExt == "hlz" )
		{

			/*if(!CheckBufferForTIN(MIN(m_nMaxPtSizePerFile, nCount)))
			return NULL;*/

			//打开hlz点云数据
			bOpened = pNewSeaPcd->open(strPcdFile);
		}

		if (bOpened)
		{
			SEAPCD* pNewSP = new SEAPCD;
			pNewSP->strPcdFile = strFile;
			pNewSP->pSEAPCD = pNewSeaPcd;

			//将新缓存的点云插入到队列的最前面
			m_pSeaPcds.insert(m_pSeaPcds.begin(), pNewSP);

			return pNewSeaPcd;
		}
		else
		{
			delete pNewSeaPcd;
			return NULL;
		}
	}
	return NULL;

}

// 移除海量点云
void CPointCloudCache::Remove(CSeaPointCloud* pSeaPcd)
{

	for(vector<SEAPCD*>::iterator it = m_pSeaPcds.begin();
		it != m_pSeaPcds.end();it++)
	{
		if ((*it)->pSEAPCD == pSeaPcd)
		{

			if (m_hMsgReceiver)
			{
				::SendMessage(m_hMsgReceiver, m_nSeaPcdMsgID, (WPARAM)(*it), (LPARAM)0);
			}

			delete (*it);
			m_pSeaPcds.erase(it);
			break;
		}
	}

}

// 移除所有海量点云
void CPointCloudCache::RemoveAllSeaPcd()
{
	for(vector<SEAPCD*>::iterator it = m_pSeaPcds.begin();
		it != m_pSeaPcds.end();it++)
	{
		//向消息接收者发送消息，告知某个点云即将被删除
		if (m_hMsgReceiver)
		{
			::SendMessage(m_hMsgReceiver, m_nSeaPcdMsgID, (WPARAM)(*it), (LPARAM)0);
		}

		delete (*it);
	}

	// 清除海量点云队列
	m_pSeaPcds.clear();
}

// 海量点云是否加载
bool CPointCloudCache::IsSeaPcdLoaded(const char* strPcdFile,bool& bOnlyHeader)
{

	bool bLoaded = false;

	std::string strFile = strPcdFile;
	//std::transform(strFile.begin(), strFile.end(), strFile.begin(), tolower);

	unsigned int i;
	unsigned int nCount = m_pSeaPcds.size();
	for (i = 0; i < nCount; i++)
	{
		//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
		if (0 == stricmp(strFile.data(), m_pSeaPcds[i]->strPcdFile.data()))
		{
			bLoaded = true;
						
		}
	}
	return bLoaded;
}

// 获取存在的点云
CSeaPointCloud* CPointCloudCache::GetLoadedSeaPcd(const char* strPcdFile)
{
	std::string strFile = strPcdFile;
	//std::transform(strFile.begin(), strFile.end(), strFile.begin(), tolower);

	unsigned int i;
	unsigned int nCount = m_pSeaPcds.size();
	for (i = 0; i < nCount; i++)
	{
		//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
		if (0 == stricmp(strFile.data(), m_pSeaPcds[i]->strPcdFile.data()))
		{
			
			SEAPCD* pCurSeaPcd = m_pSeaPcds[i];

			return pCurSeaPcd->pSEAPCD;
		}
	}

	return NULL;

}

// 此处判断所打开的点云文件是否为索引索引文件
bool CPointCloudCache::IsPcdIndexFile(const char* filePath)
{
	CHLSReadOpener reader;
	IHLSReader* preader = reader.Open(filePath);
	bool ret = preader!=NULL;
	if (ret)
	{
		delete preader;
		preader =NULL;
	}
	return ret;
}

// 设置外部是否按平均加载
void CPointCloudCache::SetbAverLoad( bool bAverage )
{
	m_bAverLoad = bAverage;
}

// 外部获得设置
bool CPointCloudCache::isAverLoad()
{
	return m_bAverLoad;
}

// 根据当前可用内存更新文件加载阈值
void CPointCloudCache::UpdateMaxPtSizePerFile( U64 curLoadCount )
{
	// 首先获得当前系统可用内存
	U64 availPhys = GetAvailPhy();

	// 获得当前需要加载的点云点数
	U64 pcdSize = ((U64)curLoadCount) * sizeof(PointXYZIPRGBA) * 6 / 5;

	// 点云总点数小于系统可用内存数，直接全部加载
	if (pcdSize <= availPhys)
	{
		m_nMaxPtSizePerFile = curLoadCount;

		// 应设置阈值略大于总点数
		m_nMaxPtSizePerFile += 10000;
	}
	else // 点云需要加载总点数大于系统可用内存数，根据可用内存设置最大加载点数
	{
		U64 toLoad = (U64)(availPhys * 2 / (3 * sizeof(PointXYZIPRGBA)));
		m_nMaxPtSizePerFile = toLoad;
	}
}

PointCloud* CPointCloudCache::DraftCache( const char* strPcdFile,bool bOnlyHeader /*= false*/,void (*loadCallback)(float,const char*) /*= NULL*/ )
{
	std::string strFile = strPcdFile;
	if (strFile == "")
	{
		return NULL;
	}

	// 部分中文用tolower转换为乱码，此处更改为toupper -zhubo
	//std::transform(strFile.begin(), strFile.end(), strFile.begin(),tolower);

	unsigned int i;
	unsigned int nCount = m_pPcds.size();
	for (i = 0; i<nCount; i++)
	{
		//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
		if (0 == stricmp(strFile.data(), m_pPcds[i]->strPcdFile.data()))
		{
			// 如果需要加载数据，而找到的点云只加载了文件头，则需要重新加载数据 
			PCD* pCurPcd = m_pPcds[i];
			if (pCurPcd->pPointCloud->count() == 0 && !bOnlyHeader)
			{
				// 根据外部设置，非平均加载时，更新最大加载阈值m_nMaxPtSizePerFile
				if (!m_bAverLoad)
				{
					UpdateMaxPtSizePerFile(pCurPcd->pPointCloud->m_header.number_of_point_records);
				}

				// 计算实际load的点数
				U64 curLoadCount = 0;
				U32 simple = 0;

				if (pCurPcd->pPointCloud->isNormalPointCloud())
				{
					simple = (U32)sqrt(1.0 * pCurPcd->pPointCloud->m_header.number_of_point_records / m_nMaxPtSizePerFile) + 1;
					curLoadCount = pCurPcd->pPointCloud->m_header.number_of_point_records / (simple * simple);
				}
				else
				{
					simple = (U32)(1.0 * pCurPcd->pPointCloud->m_header.number_of_point_records / m_nMaxPtSizePerFile) + 1;
					curLoadCount = pCurPcd->pPointCloud->m_header.number_of_point_records / simple;
				}


				//CheckBuffer(MIN(m_nMaxPtSizePerFile,pCurPcd->pPointCloud->m_header.number_of_point_records))
				//if(!CheckBuffer(curLoadCount))
				//	return NULL;

				U64 nTotalSize = GetSize();
				//根据实际物理可用内存判断,gsl-2013/6/1
				U64 availPhys = GetAvailPhy();

				// 预留不需要太多
				U64 pcdSize = ((U64)curLoadCount) * sizeof(PointXYZIPRGBA) * 6 / 5;//sizeof(PointXYZIPRGBA)* 3 / 2

				unsigned int nCount = m_pPcds.size();

				// 如果内存已经小于点云文件所占内存,则删除最后一个点云
				while (availPhys < pcdSize && nCount > 0)
				{
					U64 nLastPcdSize = m_pPcds[nCount - 1]->pPointCloud->count();
					if (nLastPcdSize <= 0)
					{
						nCount--;
						continue;
					}
					//向消息接收者发送消息，告知某个点云即将被删除
					if (m_hMsgReceiver)
					{
						::SendMessage(m_hMsgReceiver, m_nMsgID, (WPARAM)(m_pPcds[nCount - 1]), (LPARAM)0);
					}

					//删除队列中最后一个点云
					delete m_pPcds[nCount - 1];
					m_pPcds.erase(m_pPcds.begin() + nCount - 1);

					nTotalSize -= nLastPcdSize;

					availPhys = GetAvailPhy();
					nCount = m_pPcds.size();
				}

				//设置编辑模式
				pCurPcd->pPointCloud->setEditMode(m_editMode);
				//设置单个文件最大加载点个数
				pCurPcd->pPointCloud->setLoadSimple(m_nMaxPtSizePerFile);
				//加载hls点云数据
				pCurPcd->pPointCloud->loadHlsData(loadCallback);
			}
			//m_pPcds.erase(m_pPcds.begin() + i);
			//m_pPcds.insert(m_pPcds.begin(), pCurPcd);

			return pCurPcd->pPointCloud;	
		}
	}

	//如果没有找到，则加载该点云
	if (i == nCount)
	{
		BOOL bLoaded = FALSE;
		PointCloud* newPointCloud = new PointCloud();

		string strExt = strFile.substr(strFile.find_last_of('.') + 1);
		transform(strExt.begin(), strExt.end(), strExt.begin(), tolower);
		if (strExt == "hls")
		{
			//加载hls点云文件头
			bLoaded = newPointCloud->loadHlsFileHeader(strFile.data());
			if (bLoaded && !bOnlyHeader)
			{
				// 根据外部设置，非平均加载时，更新最大加载阈值m_nMaxPtSizePerFile
				if (!m_bAverLoad)
				{
					UpdateMaxPtSizePerFile(newPointCloud->m_header.number_of_point_records);
				}

				// 计算实际load的点数
				U64 curLoadCount = 0;
				//U32 simple = newPointCloud->m_header.number_of_point_records / (m_nMaxPtSizePerFile * 2) + 1;
				U32 simple = 0;
				if (newPointCloud->isNormalPointCloud())
				{
					simple = (U32)sqrt(1.0 * newPointCloud->m_header.number_of_point_records / m_nMaxPtSizePerFile) + 1;
					curLoadCount = newPointCloud->m_header.number_of_point_records / (simple * simple);
				}
				else
				{
					simple = (U32)(1.0 * newPointCloud->m_header.number_of_point_records / m_nMaxPtSizePerFile) + 1;
					curLoadCount = newPointCloud->m_header.number_of_point_records / simple;
				}

				//// CheckBuffer(MIN(m_nMaxPtSizePerFile,newPointCloud->m_header.number_of_point_records))
				//if(!CheckBuffer(curLoadCount))
				//	return NULL;

				U64 nTotalSize = GetSize();
				//根据实际物理可用内存判断,gsl-2013/6/1
				U64 availPhys = GetAvailPhy();

				// 预留不需要太多
				U64 pcdSize = ((U64)curLoadCount) * sizeof(PointXYZIPRGBA) * 6 / 5;//sizeof(PointXYZIPRGBA)* 3 / 2

				unsigned int nCount = m_pPcds.size();

				// 如果内存已经小于点云文件所占内存,则删除最后一个点云
				while (availPhys < pcdSize && nCount > 0)
				{
					U64 nLastPcdSize = m_pPcds[nCount - 1]->pPointCloud->count();
					if (nLastPcdSize <= 0)
					{
						nCount--;
						continue;
					}
					//向消息接收者发送消息，告知某个点云即将被删除
					if (m_hMsgReceiver)
					{
						::SendMessage(m_hMsgReceiver, m_nMsgID, (WPARAM)(m_pPcds[nCount - 1]), (LPARAM)0);
					}

					//删除队列中最后一个点云
					delete m_pPcds[nCount - 1];
					m_pPcds.erase(m_pPcds.begin() + nCount - 1);

					nTotalSize -= nLastPcdSize;

					availPhys = GetAvailPhy();
					nCount = m_pPcds.size();
				}



				//设置编辑模式
				newPointCloud->setEditMode(m_editMode);
				//设置单个文件最大加载点个数
				newPointCloud->setLoadSimple(m_nMaxPtSizePerFile);

				//加载hls点云数据
				bLoaded = newPointCloud->loadHlsData(loadCallback);
			}
		}
		/*else if (strExt == "las")
		{
		bLoaded = newPointCloud->loadLasFile(strFile.data(), NULL);
		}*/

		if (bLoaded)
		{
			PCD* newPCD = new PCD;
			newPCD->strPcdFile = strFile;
			newPCD->pPointCloud = newPointCloud;

			//将新缓存的点云插入到队列的最前面
			m_pPcds.insert(m_pPcds.begin(), newPCD);

			return newPointCloud;
		}
		else
		{
			delete newPointCloud;
			return NULL;
		}
	}

	return NULL;
}

PointCloud* CPointCloudCache::DraftCache( const char* strPcdFile,hd::f64 nStartCol,hd::f64 nEndCol,bool bOnlyHeader /*= false*/,void (*loadCallback)(float,const char*) /*= NULL*/ )
{
	std::string strFile = strPcdFile;
	if (strFile == "")
	{
		return NULL;
	}

	// 部分中文用tolower转换为乱码，此处更改为toupper -zhubo
	//std::transform(strFile.begin(), strFile.end(), strFile.begin(),tolower);

	unsigned int i;
	unsigned int nCount = m_pPcds.size();
	for (i = 0; i<nCount; i++)
	{
		//如果在缓存池中找到该点云了，则把它的指针放到队列最前面，并返回
		if (0 == stricmp(strFile.data(), m_pPcds[i]->strPcdFile.data()))
		{
			// 如果需要加载数据，而找到的点云只加载了文件头，则需要重新加载数据 
			PCD* pCurPcd = m_pPcds[i];
			if (pCurPcd->pPointCloud->count() == 0 && !bOnlyHeader)
			{
				// 计算实际load的点数
				U64 curLoadCount = 0;
				U64 nTotalCount = (U64)(pCurPcd->pPointCloud->m_header.number_of_point_records * (nEndCol - nStartCol));
				U32 simple = (U32)sqrt(1.0 * nTotalCount / m_nMaxPtSizePerFile) + 1;

				if (pCurPcd->pPointCloud->isNormalPointCloud())
				{
					curLoadCount = nTotalCount / (simple * simple);
				}
				else
				{
					curLoadCount = nTotalCount / simple;
				}

				// MIN(m_nMaxPtSizePerFile,pCurPcd->pPointCloud->m_header.number_of_point_records)
				//if(!CheckBuffer(curLoadCount))
				//	return NULL;

				U64 nTotalSize = GetSize();
				//根据实际物理可用内存判断,gsl-2013/6/1
				U64 availPhys = GetAvailPhy();

				// 预留不需要太多
				U64 pcdSize = ((U64)curLoadCount) * sizeof(PointXYZIPRGBA) * 6 / 5;//sizeof(PointXYZIPRGBA)* 3 / 2

				unsigned int nCount = m_pPcds.size();

				// 如果内存已经小于点云文件所占内存,则删除最后一个点云
				while (availPhys < pcdSize && nCount > 0)
				{
					U64 nLastPcdSize = m_pPcds[nCount - 1]->pPointCloud->count();
					if (nLastPcdSize <= 0)
					{
						nCount--;
						continue;
					}
					//向消息接收者发送消息，告知某个点云即将被删除
					if (m_hMsgReceiver)
					{
						::SendMessage(m_hMsgReceiver, m_nMsgID, (WPARAM)(m_pPcds[nCount - 1]), (LPARAM)0);
					}

					//删除队列中最后一个点云
					delete m_pPcds[nCount - 1];
					m_pPcds.erase(m_pPcds.begin() + nCount - 1);

					nTotalSize -= nLastPcdSize;

					availPhys = GetAvailPhy();
					nCount = m_pPcds.size();
				}

				//设置编辑模式
				pCurPcd->pPointCloud->setEditMode(m_editMode);
				//设置单个文件最大加载点个数
				pCurPcd->pPointCloud->setLoadSimple(m_nMaxPtSizePerFile);
				//double loopCount = pCurPcd->pPointCloud->m_header.number_of_col;
				//double startScale = (double)nStartCol/loopCount;
				//double eneScale = (double)nEndCol/loopCount;
				//加载hls点云数据
				pCurPcd->pPointCloud->loadHlsByScale(nStartCol,nEndCol,loadCallback);
			}
			//m_pPcds.erase(m_pPcds.begin() + i);
			//m_pPcds.insert(m_pPcds.begin(), pCurPcd);

			return pCurPcd->pPointCloud;	
		}
	}

	//如果没有找到，则加载该点云
	if (i == nCount)
	{
		BOOL bLoaded = FALSE;
		PointCloud* newPointCloud = new PointCloud();

		string strExt = strFile.substr(strFile.find_last_of('.') + 1);
		transform(strExt.begin(), strExt.end(), strExt.begin(), tolower);
		if (strExt == "hls")
		{
			//加载hls点云文件头
			bLoaded = newPointCloud->loadHlsFileHeader(strFile.data());
			if (bLoaded && !bOnlyHeader)
			{
				// 计算实际load的点数
				U64 curLoadCount = 0;
				U64 nTotalCount = (U64)(newPointCloud->m_header.number_of_point_records * (nEndCol - nStartCol));
				U32 simple = (U32)sqrt(1.0 * nTotalCount / m_nMaxPtSizePerFile) + 1;

				if (newPointCloud->isNormalPointCloud())
				{
					curLoadCount = nTotalCount / (simple * simple);
				}
				else
				{
					curLoadCount = nTotalCount / simple;
				}

				// MIN(m_nMaxPtSizePerFile,newPointCloud->m_header.number_of_point_records)
				//if(!CheckBuffer(curLoadCount))
				//	return NULL;

				U64 nTotalSize = GetSize();
				//根据实际物理可用内存判断,gsl-2013/6/1
				U64 availPhys = GetAvailPhy();

				// 预留不需要太多
				U64 pcdSize = ((U64)curLoadCount) * sizeof(PointXYZIPRGBA) * 6 / 5;//sizeof(PointXYZIPRGBA)* 3 / 2

				unsigned int nCount = m_pPcds.size();

				// 如果内存已经小于点云文件所占内存,则删除最后一个点云
				while (availPhys < pcdSize && nCount > 0)
				{
					U64 nLastPcdSize = m_pPcds[nCount - 1]->pPointCloud->count();
					if (nLastPcdSize <= 0)
					{
						nCount--;
						continue;
					}
					//向消息接收者发送消息，告知某个点云即将被删除
					if (m_hMsgReceiver)
					{
						::SendMessage(m_hMsgReceiver, m_nMsgID, (WPARAM)(m_pPcds[nCount - 1]), (LPARAM)0);
					}

					//删除队列中最后一个点云
					delete m_pPcds[nCount - 1];
					m_pPcds.erase(m_pPcds.begin() + nCount - 1);

					nTotalSize -= nLastPcdSize;

					availPhys = GetAvailPhy();
					nCount = m_pPcds.size();
				}

				//设置编辑模式
				newPointCloud->setEditMode(m_editMode);
				//设置单个文件最大加载点个数
				newPointCloud->setLoadSimple(m_nMaxPtSizePerFile);
				//double loopCount = newPointCloud->m_header.number_of_col;
				double startScale = nStartCol;//(double)nStartCol/loopCount;
				double endScale = nEndCol;//(double)nEndCol/loopCount;

				//加载hls点云数据
				bLoaded = newPointCloud->loadHlsByScale(startScale < endScale ? startScale : endScale,
					startScale < endScale ? endScale : startScale,loadCallback);
			}
		}

		if (bLoaded)
		{
			PCD* newPCD = new PCD;
			newPCD->strPcdFile = strFile;
			newPCD->pPointCloud = newPointCloud;

			//将新缓存的点云插入到队列的最前面
			m_pPcds.insert(m_pPcds.begin(), newPCD);

			return newPointCloud;
		}
		else
		{
			delete newPointCloud;
			return NULL;
		}
	}

	return NULL;
}

BOOL CPointCloudCache::IsWow64()
{
	BOOL bIsWow64 = FALSE;

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")),"IsWow64Process");
	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
		{
		}
	}
	return bIsWow64;
}

void CPointCloudCache::setLoadSimpleMode(int mode) 
{
	switch(mode)
	{
	case 0:
		m_load_simple_mode = E_LOAD_NUM_SIMPLE;
		break;
	case 1:
		m_load_simple_mode = E_LOAD_SPACE_SIMPLE;
		break;
	default:
		break;
	}
	
}

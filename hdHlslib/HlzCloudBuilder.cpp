#include "HlzCloudBuilder.h"

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
#include "mydefs.hpp"
#include "..\hdHlslib\PtEncoder.h"
#include "..\hdGeoPosition\PositionTranfrom.h"

#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <algorithm>
#include <Windows.h>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

#define BLOCK_HOLD 10240
#define DEBUG_LOG 0

namespace hd
{

	CHlzCloudBuilder::CHlzCloudBuilder(void)
	{
		m_iMin = 0xffff;
		m_iMax = 0;
		m_ptNum = 0;
		m_ptNumNext = 0.0;
		m_pHlzWrite = NULL;
		m_pNextHlzWrite = NULL;
		processCallback = NULL;
		m_stepX = 32.0;		// 第0层格网大小为64m
		m_stepY = 32.0;
		m_zoneStartX = 0.0;
		m_zoneStartY = 0.0;
		m_zoneEndX = 0.0;
		m_zoneEndY = 0.0;
		m_blockSetStartX = 0.0;
		m_blockSetStartY = 0.0;
		m_blockSetStartXNext = 0.0;
		m_blockSetStartYNext = 0.0;
		m_pCoreData = new CHdCoreData;
		m_pCoreDataNext = new CHdCoreData;
		m_dCenter = 0.0;

		// 初始化标记不重采样0层
		m_bResampleZero = false;
		m_dGridPrecision = 1.0/256.0;
	}

	CHlzCloudBuilder::~CHlzCloudBuilder(void)
	{	
		Close();
		// 删除所有临时文件
		//RemoveTempFiles(m_savePath.c_str());
	}

	void CHlzCloudBuilder::Close()
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
		if (m_pCoreDataNext)
		{
			delete m_pCoreDataNext;
			m_pCoreDataNext = NULL;
		}
		// 关闭所有文件
		CloseTempFiles();
	}

	// 块切成包
	bool CHlzCloudBuilder::SplitBlock2Parcel_Buffer(const CHdBox3df& blockBox, const char* strBlockID, const char* strParcelID, std::vector<ParcelFileInfo>& arrayParcels)
	{

		//找到块集内存中对应的的块文件
		std:: map<string , vector<PointXYZIPRGBA*> >:: iterator iter_block = m_pBlockData.find(strBlockID);
		if (iter_block == m_pBlockData.end()) 
		{
			//该块中不存在数据
			return false;
		}
		// 对块文件进行切分
		std::map<U8, ParcelFileInfo> mapSplitParcels;	// 一个块分割出来的包文件

		CHdVector3df center = blockBox.getCenter();
		CHdVector3df halfSize = blockBox.getExtent() / 2.0f;
		CHdVector3df centerSlice(0,0,0);
		int nFileX = (int)center.X/m_stepX;
		int nFileY = (int)center.Y/m_stepX;
		centerSlice.X =center.X - nFileX * m_stepX;
		centerSlice.Y =center.Y - nFileY * m_stepY;
		centerSlice.Z = center.Z;
		// 切分生成的包文件的点
		vector<SplitMemoryBuf> parcelPoints;
		parcelPoints.resize(8);
		for (I32 i=0; i < iter_block->second.size(); i++)
		{
			PointXYZIPRGBA* ptTmp = NULL;
			ptTmp = *((iter_block->second._Myfirst() + i));

			// 该点所在的包序号
			U8 parcelIdx = 0;
			//getPtBlockIndex(*ptTmp, center, parcelIdx);
			// 按偏移后的块中心坐标偏移
            getPtBlockIndex(*ptTmp, centerSlice, parcelIdx);
			SplitMemoryBuf& memPoints = *(parcelPoints._Myfirst() + parcelIdx);
			if (memPoints.m_count >= memPoints.m_vecBuf.size())
			{
				try
				{
					memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
				}
				catch(...)
				{
					::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
					// 删除临时文件
					RemoveTempFiles(m_savePath.c_str());
					return false;
				}
			}
			PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
			memPoints.m_count++;
			pPtMem = *(iter_block->second._Myfirst() + i);
		}  //for (I32 i=0; i < iter_block->second.size(); i++)

		char parcelDataID[512] = {0};	
		// 1.将内存中的点一次写入到包文件中; 2.统计包文件信息;
		for (I32 i = 0;i < parcelPoints.size();i++)
		{			
			SplitMemoryBuf& pacPoints = *(parcelPoints._Myfirst() + i);
			int nCount = pacPoints.m_count;
			if (nCount <= 0)
			{
				continue;
			}

			sprintf_s(parcelDataID, "%s\\parcel_%d.tmp", strParcelID, i);
			hd::stringc strParcelDataID = parcelDataID;		
			//写入数据
			int preSize = m_ParcelData[strParcelDataID.c_str()].size();
			//重新分配内存空间
			try
			{
				m_ParcelData[strParcelDataID.c_str()].resize(preSize + nCount);
			}
			catch(...)
			{
				::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
				// 删除临时文件
				RemoveTempFiles(m_savePath.c_str());
				return false;
			}
			std:: map<string , vector<PointXYZIPRGBA *> > :: iterator iter_parcel = m_ParcelData.find(strParcelDataID.c_str());
// 			for (I32 j= 0; j < nCount; j++)
// 			{
// 				// 点数据指针压入数据中
// 				*(iter_parcel->second._Myfirst() + preSize + j) = *(pacPoints.m_vecBuf._Myfirst() + j);
// 			}
			memcpy(iter_parcel->second._Myfirst() + preSize, pacPoints.m_vecBuf._Myfirst(), nCount * sizeof(PointXYZIPRGBA*));


			// 收集包文件信息
			if (!mapSplitParcels.count(i))
			{
				ParcelFileInfo info;
				//getBlockBox(parcelFilePath, c, info.box,(U16)i);		// 包的范围
				getBlockBox(center,halfSize,info.box,(U16)i);
				info.path = parcelDataID;
				info.numPoint = nCount;
				mapSplitParcels.insert(make_pair(i, info));
			}
			else
			{
				ParcelFileInfo& parcelInfo = mapSplitParcels[i];
				parcelInfo.numPoint += nCount;
			}
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
				if(SplitParcel2Parcel_Buffer((it->second).box, (it->second).path.c_str(), splitPath, arrayParcels))
				{
					//切分完成后从内存中删除该包的数据
					m_ParcelData.erase(it->second.path.c_str());
				}
			}
		}

		return true;
	}

	// 包递归切分成包直到每个包里的点小于64000
	bool CHlzCloudBuilder::SplitParcel2Parcel_Buffer(const CHdBox3df& blockBox, const char* srcParcelID, const string destParcelID, std::vector<ParcelFileInfo>& arrayParcels)
	{

		//找到需要切分的包文件
		std:: map<string , vector<PointXYZIPRGBA *> >:: iterator iter_parcel = m_ParcelData.find(srcParcelID);
		if (iter_parcel == m_ParcelData.end()) 
		{
			//该块中不存在数据
			return false;
		}
		// 对块文件进行切分
		std::map<U8, ParcelFileInfo> mapSplitParcels;	// 一个块分割出来的包文件


		CHdVector3df center = blockBox.getCenter();
		CHdVector3df halfSize = blockBox.getExtent() / 2.0f;
		CHdVector3df centerSlice(0,0,0);
		int nFileX = (int)center.X/m_stepX;
		int nFileY = (int)center.Y/m_stepY;
		centerSlice.X = center.X - nFileX * m_stepX;
		centerSlice.Y = center.Y - nFileY * m_stepY;
		centerSlice.Z = center.Z;
		// 切分生成的包文件的点
		vector<SplitMemoryBuf> parcelPoints;
		parcelPoints.resize(8);

		for (I32 i=0; i < iter_parcel->second.size(); i++)
		{
			PointXYZIPRGBA* ptTmp = NULL;
			ptTmp = *(iter_parcel->second._Myfirst() + i);

			// 该点所在的包序号
			U8 parcelIdx = 0;
			//getPtBlockIndex(*ptTmp, center, parcelIdx);
			getPtBlockIndex(*ptTmp, centerSlice, parcelIdx);
			SplitMemoryBuf& memPoints = *(parcelPoints._Myfirst() + parcelIdx);
			if (memPoints.m_count >= memPoints.m_vecBuf.size())
			{
				try
				{
					memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
				}
				catch(...)
				{
					::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
					// 删除临时文件
					RemoveTempFiles(m_savePath.c_str());
					return false;
				}
			}

			PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
			memPoints.m_count++;
			pPtMem = *(iter_parcel->second._Myfirst() + i);

		}//for (I32 i=0; i < iter_block->second.size(); i++)

		char* parcelDataID;	
		// 1.将内存中的点一次写入到包文件中; 2.统计包文件信息;
		for (I32 i = 0;i < parcelPoints.size();i++)
		{			
			SplitMemoryBuf& pacPoints = *(parcelPoints._Myfirst() + i);
			int nCount = pacPoints.m_count;
			if (nCount <= 0)
			{
				continue;
			}

			parcelDataID = new char[destParcelID.length()+14];
			sprintf_s(parcelDataID, destParcelID.length()+14,"%s\\parcel_%d.tmp", destParcelID.c_str(), i);
			hd::stringc strParcelDataID = parcelDataID;				
			//写入数据
			int preSize = m_ParcelData[strParcelDataID.c_str()].size();
			//重新分配内存空间
			try
			{
				m_ParcelData[strParcelDataID.c_str()].resize(preSize + nCount);
			}
			catch(...)
			{
				::MessageBox(NULL, "内存分配出错，请提高内存分配或者使用“临时文件方式”", NULL, MB_OK);
				// 删除临时文件
				RemoveTempFiles(m_savePath.c_str());
				return false;
			}
			std :: map<string, vector<PointXYZIPRGBA*> > :: iterator iter_parcel = m_ParcelData.find(strParcelDataID.c_str());
// 			for (I32 j= 0; j < nCount; j++)
// 			{
// 				// 点数据指针压入数据中
// 				*(iter_parcel->second._Myfirst() + preSize + j) = *(pacPoints.m_vecBuf._Myfirst() + j);
// 			}
			memcpy(iter_parcel->second._Myfirst() + preSize, pacPoints.m_vecBuf._Myfirst(), nCount*sizeof(PointXYZIPRGBA*));

			// 收集包文件信息
			if (!mapSplitParcels.count(i))
			{
				ParcelFileInfo info;
				//getBlockBox(parcelFilePath, c, info.box,(U16)i);		// 包的范围
				getBlockBox(center,halfSize,info.box,(U16)i);
				info.path = parcelDataID;
				info.numPoint = nCount;
				mapSplitParcels.insert(make_pair(i, info));
			}
			else
			{
				ParcelFileInfo& parcelInfo = mapSplitParcels[i];
				parcelInfo.numPoint += nCount;
			}
		}

		// 对包文件进行递归切分
		for (std::map<U8, ParcelFileInfo>::iterator it = mapSplitParcels.begin();
			it != mapSplitParcels.end(); it++)
		{
			CHdVector3df extent = (it->second).box.getExtent();
			//细分到0.0001，将不再继续细分
			if (extent.X<0.0001 && extent.Y < 0.01 && extent.Z < 0.0001)
			{
				U64 num = (it->second).numPoint;
				(it->second).numPoint = num < BLOCK_HOLD ? num:BLOCK_HOLD;
			}
			// 点数小于阈值，不进行递归切分
			if ((it->second).numPoint <= BLOCK_HOLD)
			{
				// 包文件不需要再往下切分
				arrayParcels.push_back(it->second);
			}
			else
			{
				std::string splitPath = (it->second).path.c_str();
				splitPath = splitPath.substr(0, splitPath.find_last_of('.'));

				// 继续往下切分
				if(SplitParcel2Parcel_Buffer((it->second).box, (it->second).path.c_str(), splitPath.c_str(), arrayParcels))
				{
					//切分完成后从内存中删除该包的数据
					m_ParcelData.erase(it->second.path.c_str());
				}
			}
		}

		return true;
	}

	// 划分块集文件
	bool CHlzCloudBuilder::SplitBlockSetFiles(const char* savePath, hd::stringc& strHlsFile)
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

		hd::stringc strInfo = HDSCENE_IDS_PROCESSING;
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
		U32 nLoopSize = 100;
		U32 nCurLp = 0;
		//U32 selNum = 0;

		//每5W个点写一次
		PointXYZIPRGBA* pPtBuf4Write;
		int wSize = 50000;
		pPtBuf4Write = new PointXYZIPRGBA[wSize];
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

					//// 坐标偏移
					//pt.x = (hd::f32)(x - m_fullExtent.MinEdge.X);
					//pt.y = (hd::f32)(y - m_fullExtent.MinEdge.Y);
					//pt.z = (hd::f32)(z - m_fullExtent.MinEdge.Z);
					//pt.x = (hd::f32)(x - m_zoneStartX);
					//pt.y = (hd::f32)(y - m_zoneStartY);
					pt.x = (hd::f32)(x - m_blockSetStartX);
					pt.y = (hd::f32)(y - m_blockSetStartY);
					pt.z = (hd::f32)(z - m_fullExtent.MinEdge.Z);
					// 计算块集编号
					U32 xNo = (U32)((pt.x) / m_stepX);// - m_pHlzWrite->m_header.grid_sx
					U32 yNo = (U32)((pt.y) / m_stepY);// - m_pHlzWrite->m_header.grid_sy
					//U32 fileNo = pHdLevel->m_level.numBlocksetY*xNo + yNo;
					U32 fileNo = pHdLevel->m_level.numBlocksetX*yNo + xNo;

					// 坐标偏移,最终坐标相对于所属格网的基点坐标
					//pt.x = (hd::f32)(x - m_fullExtent.MinEdge.X - xNo * m_stepX);
					//pt.y = (hd::f32)(y - m_fullExtent.MinEdge.Y - yNo * m_stepY);
					//pt.x = (hd::f32)(x - m_zoneStartX - xNo * m_stepX);
					//pt.y = (hd::f32)(y - m_zoneStartY - yNo * m_stepY);
					pt.x = (hd::f32)(x - m_blockSetStartX - xNo * m_stepX);
					pt.y = (hd::f32)(y - m_blockSetStartY - yNo * m_stepY);
					pt.z = (hd::f32)(z - m_fullExtent.MinEdge.Z);

					SplitMemoryBuf& memPoints = BsPointsArray[fileNo];

					if (memPoints.m_count >= memPoints.m_vecBuf.size())
					{
						try
						{
							memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
						}
						catch(...)
						{
							::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
							// 删除临时文件
							RemoveTempFiles(m_savePath.c_str());
							return false;
						}
					}

					PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
					memPoints.m_count++;
					pPtMem = &pt;					

					// 统计x分布
					U32 selNum = (U32)((x - /*m_zoneStartX*/m_fullExtent.MinEdge.X) * m_coordStats.xRcp);
					selNum = clamp(selNum,(U32)0,(U32)999);
					m_coordStats.xStepStat[selNum]++;

					// 统计y分布
					selNum = (U32)((y - /*m_zoneStartY*/m_fullExtent.MinEdge.Y) * m_coordStats.yRcp);
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
//				FILE* pFile = fopen(filePath,"ab+");
				HANDLE pFile = CreateFile(filePath,
					GENERIC_READ|GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE, 
					NULL,
					OPEN_ALWAYS, 
					FILE_ATTRIBUTE_NORMAL, 
					NULL);
				if(pFile == INVALID_HANDLE_VALUE)
				{
					continue;
				}

				if (pFile != NULL)
				{

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

					LARGE_INTEGER li1,li2;
					li1.HighPart = 0;
					li1.LowPart = 0;
					li2.HighPart = 0;
					li2.LowPart = 0;
					SetFilePointerEx(pFile, li1, &li2, FILE_END);
					
					//50000个点为单位写入
					int ptCount = 0;
					int k = 0;
					DWORD numWrite;

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

							*(pPtBuf4Write + k) = *pPt;
							k++;

							if (k == wSize)
							{
								WriteFile(pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
								ptCount++;
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

							*(pPtBuf4Write + k) = *pPt;
							k++;

							if (k == wSize)
							{
								//WriteFile (pBlstDataFile, pBuffer, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);
								WriteFile(pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
								ptCount++;
								k = 0;
								//delete [] pBuffer;
								//pBuffer = new PointXYZIPRGBA [BUFFER_SIZE];
							}
						}
					}

					//for (U32 m=0; m < nCount; )
					//{
					//	PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + m++);
					//	*(pPtBuf4Write + k++) = *pPt;
					//
					//	blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
					//	blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

					//	if (k == wSize)
					//	{
					//		
					//		WriteFile(pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
					//		ptCount++;
					//		k = 0;
					//		//delete[] pPtBuf4Write;
					//		//pPtBuf4Write = new PointXYZIPRGBA[wSize];
					//	}
					//}

					// 剩余点写入
					if ( k > 0)
					{
						DWORD dwResult;
						WriteFile (pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);

						//delete[] pPtBuf4Write;
						//pPtBuf4Write = new PointXYZIPRGBA[wSize];
					}

					// 更新文件名
					m_BlocksetFiles[it->first].path = filePath;

					// 更新点数
					if (m_bResampleZero)
					{
						m_BlocksetFiles[it->first].numPoint += simpleSize;
					}
					else
					{
						m_BlocksetFiles[it->first].numPoint += nCount;
					}
					
				}

				if (pFile)
				{
					CloseHandle(pFile);
					pFile = NULL;
				}
			}

			if (processCallback)
			{			
				processCallback((float)nCurLp / loopCount, HDSCENE_IDS_CUTTING);
			}
		}

		if (pPtBuf4Write)
		{
			delete[] pPtBuf4Write;
			pPtBuf4Write = NULL;
		}

		// 计算块集的包围盒XY值和参考点
		for (map<U32, BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
			it != m_BlocksetFiles.end(); it++)
		{
			U32 BsIndex = it->first;

			// 计算块集编号
			U32 yNo = BsIndex / pHdLevel->m_level.numBlocksetX;
			U32 xNo = BsIndex - (yNo * pHdLevel->m_level.numBlocksetX);
			// 对块集包围和进行整体偏移，满足显示需要
			BlockSetFileInfo& BsInfo = it->second;
			BsInfo.box.MinEdge.X = xNo * m_stepX;
			BsInfo.box.MinEdge.Y = yNo * m_stepY;

			BsInfo.box.MaxEdge.X = (xNo+1) * m_stepX;
			BsInfo.box.MaxEdge.Y = (yNo+1) * m_stepY;

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

		return true;
	}
	bool CHlzCloudBuilder::SplitMultiZoneBlockSetFiles(const char* savePath, hd::stringc& strHlsFile)
	{
		// 打开HLS头文件
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
		// 获取当前层对象数据
		CHdLevel* pHdLevel = m_pCoreData->GetLevelRec(0);
		CHdLevel* pHdLevelNext = m_pCoreDataNext->GetLevelRec(0);
		if(pHdLevel == NULL)
		{
			pHdLevel = new CHdLevel;
			pHdLevel->m_levelNo = 0;
			m_pCoreData->AddLevelRec(pHdLevel);
		}
		if(pHdLevelNext == NULL)
		{
			pHdLevelNext = new CHdLevel;
			pHdLevelNext->m_levelNo = 0;
			m_pCoreDataNext->AddLevelRec(pHdLevelNext);
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
		U32 nLoopSize = 100;
		U32 nCurLp = 0;

		//每5W个点写一次，在此处将分属不同投影带区的点云进行切分到不同的块集记录
		PointXYZIPRGBA* pPtBuf4Write;
		int wSize = 50000;
		pPtBuf4Write = new PointXYZIPRGBA[wSize];

		PointXYZIPRGBA* pPtBuf4WriteNext;
		pPtBuf4WriteNext = new PointXYZIPRGBA[wSize];
		// 不同投影带坐标坐标转换
		CGeoPositionTran positionTran;
		double dLongitude = 0.0;         // 坐标点对应经度
		double dLatitude = 0.0;          // 坐标点对应维度
		double dPointNextX = 0.0;        // 转换后大地坐标X
		double dPointNextY = 0.0;        // 转换后大地坐标Y
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
			// 构建两个块集点数组分别存储当前块的点和下一块的点
			std::map<U32, SplitMemoryBuf> BsPointsArray;
			std::map<U32, SplitMemoryBuf> BsPointsArrayNext;

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

					// 工程点云跨越了本投影带和之前的投影带
					if (x < m_zoneStartX)
					{
						// 对于点进行投影带坐标转换
						positionTran.TranslateGauss2Degree(x,y,m_dCenter,dLongitude,dLatitude);
						positionTran.TranslateDegree2Gauss(dLongitude,dLatitude,dPointNextX,dPointNextY);
						// 得到该点在对于带区包围和内的相对坐标
						pt.x = (hd::f32)(dPointNextX - m_blockSetStartXNext/*m_fullExtentNext.MinEdge.X*/);
						pt.y = (hd::f32)(dPointNextY - m_blockSetStartYNext/*m_fullExtentNext.MinEdge.Y*/);
						pt.z = (hd::f32)(z - m_fullExtentNext.MinEdge.Z);

						// 计算块集编号
						U32 xNo = (U32)((pt.x) / m_stepX);
						U32 yNo = (U32)((pt.y) / m_stepY);
						U32 fileNoNext = pHdLevelNext->m_level.numBlocksetX*yNo + xNo;

						pt.x = (hd::f32)(dPointNextX - m_blockSetStartXNext - xNo * m_stepX);
						pt.y = (hd::f32)(dPointNextY - m_blockSetStartYNext - yNo * m_stepY);
						pt.z = (hd::f32)(z - m_fullExtentNext.MinEdge.Z);

						SplitMemoryBuf& memPoints = BsPointsArrayNext[fileNoNext];

						if (memPoints.m_count >= memPoints.m_vecBuf.size())
						{
							try
							{
								memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
							}
							catch(...)
							{
								::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
								// 删除临时文件
								RemoveTempFiles(m_savePath.c_str());
								return false;
							}
						}

						PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
						memPoints.m_count++;
						pPtMem = &pt;					

						// 统计x分布
						U32 selNum = (U32)((x - /*m_zoneStartX*/m_fullExtentNext.MinEdge.X) * m_coordStats.xRcp);
						selNum = clamp(selNum,(U32)0,(U32)999);
						m_coordStatsNext.xStepStat[selNum]++;

						// 统计y分布
						selNum = (U32)((y - /*m_zoneStartY*/m_fullExtentNext.MinEdge.Y) * m_coordStats.yRcp);
						selNum = clamp(selNum,(U32)0,(U32)999);
						m_coordStatsNext.yStepStat[selNum]++;

						// 统计z分布
						selNum = (U32)((z - m_fullExtentNext.MinEdge.Z) * m_coordStats.zRcp);
						selNum = clamp(selNum,(U32)0,(U32)999);
						m_coordStatsNext.zStepStat[selNum]++;

						// 统计强度分布
						selNum = (U32)((pt.getIntensity() - m_iMin) * m_coordStats.iRcp);
						selNum = clamp(selNum,(U32)0,(U32)999);
						m_coordStatsNext.intStepStat[selNum]++;
						m_ptNumNext++;
					}
					else if (x > m_zoneEndX)  // 位于下一投影带的点进行转换
					{
						// 对于点进行投影带坐标转换
						positionTran.TranslateGauss2Degree(x,y,m_dCenter,dLongitude,dLatitude);
						positionTran.TranslateDegree2Gauss(dLongitude,dLatitude,dPointNextX,dPointNextY);
						// 得到该点在对于带区包围和内的相对坐标
						pt.x = (hd::f32)(dPointNextX - m_blockSetStartXNext/*m_fullExtentNext.MinEdge.X*/);
						pt.y = (hd::f32)(dPointNextY - m_blockSetStartYNext/*m_fullExtentNext.MinEdge.Y*/);
						pt.z = (hd::f32)(z - m_fullExtentNext.MinEdge.Z);

						// 计算块集编号
						U32 xNo = (U32)((pt.x) / m_stepX);
						U32 yNo = (U32)((pt.y) / m_stepY);
						U32 fileNoNext = pHdLevelNext->m_level.numBlocksetX*yNo + xNo;

						pt.x = (hd::f32)(dPointNextX - m_blockSetStartXNext - xNo * m_stepX);
						pt.y = (hd::f32)(dPointNextY - m_blockSetStartYNext - yNo * m_stepY);
						pt.z = (hd::f32)(z - m_fullExtentNext.MinEdge.Z);

						SplitMemoryBuf& memPoints = BsPointsArrayNext[fileNoNext];

						if (memPoints.m_count >= memPoints.m_vecBuf.size())
						{
							try
							{
								memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
							}
							catch(...)
							{
								::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
								// 删除临时文件
								RemoveTempFiles(m_savePath.c_str());
								return false;
							}
						}

						PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
						memPoints.m_count++;
						pPtMem = &pt;					

						// 统计x分布
						U32 selNum = (U32)((x - /*m_zoneStartX*/m_fullExtentNext.MinEdge.X) * m_coordStats.xRcp);
						selNum = clamp(selNum,(U32)0,(U32)999);
						m_coordStatsNext.xStepStat[selNum]++;

						// 统计y分布
						selNum = (U32)((y - /*m_zoneStartY*/m_fullExtentNext.MinEdge.Y) * m_coordStats.yRcp);
						selNum = clamp(selNum,(U32)0,(U32)999);
						m_coordStatsNext.yStepStat[selNum]++;

						// 统计z分布
						selNum = (U32)((z - m_fullExtentNext.MinEdge.Z) * m_coordStats.zRcp);
						selNum = clamp(selNum,(U32)0,(U32)999);
						m_coordStatsNext.zStepStat[selNum]++;

						// 统计强度分布
						selNum = (U32)((pt.getIntensity() - m_iMin) * m_coordStats.iRcp);
						selNum = clamp(selNum,(U32)0,(U32)999);
						m_coordStatsNext.intStepStat[selNum]++;
						m_ptNumNext++;
					}
					else // 位于本投影内的点云分块
					{
						//// 坐标偏移
						//pt.x = (hd::f32)(x - m_fullExtent.MinEdge.X);
						//pt.y = (hd::f32)(y - m_fullExtent.MinEdge.Y);
						//pt.z = (hd::f32)(z - m_fullExtent.MinEdge.Z);
						//pt.x = (hd::f32)(x - m_zoneStartX);
						//pt.y = (hd::f32)(y - m_zoneStartY);
						pt.x = (hd::f32)(x - m_blockSetStartX);
						pt.y = (hd::f32)(y - m_blockSetStartY);
						pt.z = (hd::f32)(z - m_fullExtent.MinEdge.Z);
						// 计算块集编号
						U32 xNo = (U32)((pt.x) / m_stepX);
						U32 yNo = (U32)((pt.y) / m_stepY);

						U32 fileNo = pHdLevel->m_level.numBlocksetX*yNo + xNo;

						// 坐标偏移,最终坐标相对于所属格网的基点坐标
						//pt.x = (hd::f32)(x - m_fullExtent.MinEdge.X - xNo * m_stepX);
						//pt.y = (hd::f32)(y - m_fullExtent.MinEdge.Y - yNo * m_stepY);
						//pt.x = (hd::f32)(x - m_zoneStartX - xNo * m_stepX);
						//pt.y = (hd::f32)(y - m_zoneStartY - yNo * m_stepY);
						pt.x = (hd::f32)(x - m_blockSetStartX - xNo * m_stepX);
						pt.y = (hd::f32)(y - m_blockSetStartY - yNo * m_stepY);
						pt.z = (hd::f32)(z - m_fullExtent.MinEdge.Z);

						SplitMemoryBuf& memPoints = BsPointsArray[fileNo];

						if (memPoints.m_count >= memPoints.m_vecBuf.size())
						{
							try
							{
								memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
							}
							catch(...)
							{
								::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
								// 删除临时文件
								RemoveTempFiles(m_savePath.c_str());
								return false;
							}
						}

						PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
						memPoints.m_count++;
						pPtMem = &pt;					

						// 统计x分布
						U32 selNum = (U32)((x - /*m_zoneStartX*/m_fullExtent.MinEdge.X) * m_coordStats.xRcp);
						selNum = clamp(selNum,(U32)0,(U32)999);
						m_coordStats.xStepStat[selNum]++;

						// 统计y分布
						selNum = (U32)((y - /*m_zoneStartY*/m_fullExtent.MinEdge.Y) * m_coordStats.yRcp);
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
					}

				}
			}
			// 将内存中的点写入到相邻投影带块集文件中
			for (std::map<U32, SplitMemoryBuf>::iterator it = BsPointsArrayNext.begin(); 
				it != BsPointsArrayNext.end(); it++)
			{
				SplitMemoryBuf& bsPoints = (it->second);
				U32 nCount = bsPoints.m_count;
				if (nCount <= 0)
				{
					continue;
				}

				// 更新包围盒的Z值
				CHdBox3df& blocksetBox = m_BlocksetFilesNext[it->first].box;

				sprintf(filePath,"%s\\level0\\blocksetNext_%04d.tmp",savePath,(it->first));

				HANDLE pFile = CreateFile(filePath,
					GENERIC_READ|GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE, 
					NULL,
					OPEN_ALWAYS, 
					FILE_ATTRIBUTE_NORMAL, 
					NULL);
				if(pFile == INVALID_HANDLE_VALUE)
				{
					continue;
				}
				if (pFile != NULL)
				{
					LARGE_INTEGER li1,li2;
					li1.HighPart = 0;
					li1.LowPart = 0;
					li2.HighPart = 0;
					li2.LowPart = 0;
					SetFilePointerEx(pFile, li1, &li2, FILE_END);
					//50000个点为单位写入
					int ptCount = 0;
					int k = 0;
					for (U32 m=0; m < nCount; )
					{
						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + m++);
						*(pPtBuf4WriteNext + k++) = *pPt;

						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

						if (k == wSize)
						{
							DWORD numWrite;
							WriteFile(pFile, pPtBuf4WriteNext, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
							ptCount ++;
							k = 0;
							delete[] pPtBuf4WriteNext;
							pPtBuf4WriteNext = new PointXYZIPRGBA[wSize];
						}
					}

					int leftnum = nCount - ptCount*wSize;
					if ( leftnum>0)
					{
						DWORD dwResult;
						WriteFile (pFile, pPtBuf4WriteNext, sizeof(PointXYZIPRGBA)*leftnum, &dwResult, NULL);

						delete[] pPtBuf4WriteNext;
						pPtBuf4WriteNext = new PointXYZIPRGBA[wSize];
					}

					// 更新文件名
					m_BlocksetFilesNext[it->first].path = filePath;
					// 更新点数
					m_BlocksetFilesNext[it->first].numPoint += nCount;
				}

				if (pFile)
				{
					CloseHandle(pFile);
					pFile = NULL;
				}
				if (processCallback)
				{			
					processCallback((float)nCurLp / loopCount, HDSCENE_IDS_CUTTING);
				}
			}

			// 将内存中的点写入到块集文件中（当前投影带块集）
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
//				FILE* pFile = fopen(filePath,"ab+");
				HANDLE pFile = CreateFile(filePath,
					GENERIC_READ|GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE, 
					NULL,
					OPEN_ALWAYS, 
					FILE_ATTRIBUTE_NORMAL, 
					NULL);
				if(pFile == INVALID_HANDLE_VALUE)
				{
					continue;
				}
				if (pFile != NULL)
				{
					LARGE_INTEGER li1,li2;
					li1.HighPart = 0;
					li1.LowPart = 0;
					li2.HighPart = 0;
					li2.LowPart = 0;
					SetFilePointerEx(pFile, li1, &li2, FILE_END);
					//50000个点为单位写入
					int ptCount = 0;
					int k = 0;
					for (U32 m=0; m < nCount; )
					{
						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + m++);
						*(pPtBuf4Write + k++) = *pPt;
					
						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

						if (k == wSize)
						{
							DWORD numWrite;
							WriteFile(pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
							ptCount ++;
							k = 0;
							delete[] pPtBuf4Write;
							pPtBuf4Write = new PointXYZIPRGBA[wSize];
						}
					}

					int leftnum = nCount - ptCount*wSize;
					if ( leftnum>0)
					{
						DWORD dwResult;
						WriteFile (pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*leftnum, &dwResult, NULL);

						delete[] pPtBuf4Write;
						pPtBuf4Write = new PointXYZIPRGBA[wSize];
					}

					// 更新文件名
					m_BlocksetFiles[it->first].path = filePath;
					// 更新点数
					m_BlocksetFiles[it->first].numPoint += nCount;
				}

				if (pFile)
				{
					CloseHandle(pFile);
					pFile = NULL;
				}
			}

			if (processCallback)
			{			
				processCallback((float)nCurLp / loopCount, HDSCENE_IDS_CUTTING);
			}
		}
		// 清空点数组:当前区和前一区文件
		if (pPtBuf4WriteNext)
		{
			delete[] pPtBuf4WriteNext;
			pPtBuf4WriteNext = NULL;
		}
		if (pPtBuf4Write)
		{
			delete[] pPtBuf4Write;
			pPtBuf4Write = NULL;
		}

		// 计算块集的包围盒XY值和参考点
		for (map<U32, BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
			it != m_BlocksetFiles.end(); it++)
		{
			U32 BsIndex = it->first;

			// 计算块集编号
			U32 yNo = BsIndex / pHdLevel->m_level.numBlocksetX;
			U32 xNo = BsIndex - (yNo * pHdLevel->m_level.numBlocksetX);
			// 对块集包围和进行整体偏移，满足显示需要
			BlockSetFileInfo& BsInfo = it->second;
			BsInfo.box.MinEdge.X = xNo * m_stepX;
			BsInfo.box.MinEdge.Y = yNo * m_stepY;

			BsInfo.box.MaxEdge.X = (xNo+1) * m_stepX;
			BsInfo.box.MaxEdge.Y = (yNo+1) * m_stepY;

			if(DEBUG_LOG)
			{
				char logmsg[512] = {0};
				sprintf_s(logmsg,"level:%d,%d,%d,%d\n",pHdLevel->m_levelNo,BsIndex,xNo,yNo);
				m_pHlzWrite->WriteLogFile(logmsg);		
			}
		}

		// 计算下一带区块集的包围盒XY值和参考点
		for (map<U32, BlockSetFileInfo>::iterator it = m_BlocksetFilesNext.begin();
			it != m_BlocksetFilesNext.end(); it++)
		{
			U32 BsIndex = it->first;

			// 计算块集编号
			U32 yNo = BsIndex / pHdLevelNext->m_level.numBlocksetX;
			U32 xNo = BsIndex - (yNo * pHdLevelNext->m_level.numBlocksetX);
			// 对块集包围和进行整体偏移，满足显示需要
			BlockSetFileInfo& BsInfo = it->second;
			BsInfo.box.MinEdge.X = xNo * m_stepX;
			BsInfo.box.MinEdge.Y = yNo * m_stepY;

			BsInfo.box.MaxEdge.X = (xNo+1) * m_stepX;
			BsInfo.box.MaxEdge.Y = (yNo+1) * m_stepY;

			if(DEBUG_LOG)
			{
				char logmsg[512] = {0};
				sprintf_s(logmsg,"level:%d,%d,%d,%d\n",pHdLevelNext->m_levelNo,BsIndex,xNo,yNo);
				m_pNextHlzWrite->WriteLogFile(logmsg);		
			}
		}
		if (pHlsReader != NULL)
		{
			delete pHlsReader;
			pHlsReader = NULL;
		}

		return true;
	}
	// 根据块集划分块文件
	bool CHlzCloudBuilder::SplitBlockFiles(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks)//
	{
// 		FILE* pFile = fopen(procBlockSet.path.c_str(), "rb");
// 		if (pFile == NULL)
// 		{
// 			return false;
// 		}
// 
// 		fseek(pFile, 0, SEEK_SET);// 移到文件头
		HANDLE pFile = CreateFile(procBlockSet.path.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (pFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		// 创建存储目录
		std::string dir = procBlockSet.path;
		dir = dir.substr(0, dir.find_last_of('.'));
		I32 ret = _mkdir(dir.c_str());

		// 一次读取6.4w点到内存后写入到文件
		std::vector<PointXYZIPRGBA> bufferPts;

		PointXYZIPRGBA* ptTmp;
		// 当前点所在的块序号

		CHdVector3df center = procBlockSet.box.getCenter();
		CHdVector3df halfSize = procBlockSet.box.getExtent() /2.0f;

		CHdVector3df centerSlice(0,0,0);
		int nFileX = (int)center.X/m_stepX;
		int nFileY = (int)center.Y/m_stepX;
		centerSlice.X =center.X - nFileX * m_stepX;
		centerSlice.Y =center.Y - nFileY * m_stepY;
		centerSlice.Z = center.Z;

		arrayBlocks.resize(8);

		PointXYZIPRGBA* pPtBuf4Write;
		int wSize = 50000;
		pPtBuf4Write = new PointXYZIPRGBA[wSize];
		while (m_pHlzWrite->ReadPtsBySize(pFile, bufferPts,MAX_POINT_NUM) > 0)
		{
			// 切分生成的块文件对应的点
			vector<SplitMemoryBuf> blockPoints;
			blockPoints.resize(8);
			for (I32 i=0; i < bufferPts.size(); i++)
			{

				ptTmp = &(*(bufferPts._Myfirst() + i));
				
				U8 blockIdx = 0;			
				//getPtBlockIndex(*ptTmp, center, blockIdx);
			    getPtBlockIndex(*ptTmp, centerSlice, blockIdx);
				SplitMemoryBuf& memPoints = *(blockPoints._Myfirst() + blockIdx);
				if (memPoints.m_count >= memPoints.m_vecBuf.size())
				{
					memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
				}
				PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
				memPoints.m_count++;
				pPtMem = bufferPts._Myfirst() + i;	
			}//for (I32 i=0; i < bufferPts.size(); i++)

			// 将内存中的点一次写入到块文件中
			for (I32 i = 0;i < 8;i++)
			{
				SplitMemoryBuf& blkPoints = *(blockPoints._Myfirst() + i);
				int nCount = blkPoints.m_count;
				if(nCount <= 0)
					continue;
				//			char blockFilePath[512] = {0};
				//			sprintf_s(blockFilePath, "%s\\block_%d.tmp", dir.c_str(), i);
				char* blockFilePath = new char[dir.length()+13];
				sprintf_s(blockFilePath, dir.length()+13, "%s\\block_%d.tmp", dir.c_str(), i);
// 				FILE* pBlockFile = fopen(blockFilePath, "ab+");
// 				if (pBlockFile == NULL)
// 				{
// 					continue;
// 				}
				HANDLE pBlockFile =  CreateFile(blockFilePath,
					GENERIC_READ|GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE,
					NULL,
					OPEN_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);
				if (pBlockFile == INVALID_HANDLE_VALUE)
				{
					continue;
				}
				LARGE_INTEGER li1,li2;
				li1.HighPart = 0;
				li1.LowPart = 0;
				li2.HighPart = 0;
				li2.LowPart = 0;
				SetFilePointerEx(pBlockFile, li1, &li2, FILE_END);

				BlockFileInfo& blkInfo = *(arrayBlocks._Myfirst() + i);
				blkInfo.path = blockFilePath;
				blkInfo.numPoint += nCount;
				blkInfo.index = i;
				getBlockBox(center,halfSize,blkInfo.box,(U16)i);

				//50000个点为单位写入
				int ptCount = 0;
				int k = 0;
				for (I32 j = 0; j < nCount; j++)
				{
					PointXYZIPRGBA* pPt = *(blkPoints.m_vecBuf._Myfirst() + j);
// 					DWORD numWrite;
// 					WriteFile(pBlockFile, pPt, sizeof(PointXYZIPRGBA), &numWrite, NULL);
//					fwrite(pPt, sizeof(PointXYZIPRGBA), 1, pBlockFile);
					*(pPtBuf4Write + k++) = *pPt;
					if (k == wSize)
					{
						DWORD numWrite;
						WriteFile(pBlockFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
						ptCount ++;
						k = 0;
						delete[] pPtBuf4Write;
						pPtBuf4Write = new PointXYZIPRGBA[wSize];
					}
				}
				int leftnum = nCount - ptCount * wSize;
				if ( leftnum>0)
				{
					DWORD dwResult;
					WriteFile (pBlockFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*leftnum, &dwResult, NULL);
					delete[] pPtBuf4Write;
					pPtBuf4Write = new PointXYZIPRGBA[wSize];
				}

				if (blockFilePath)
				{
					delete[] blockFilePath;
					blockFilePath = NULL;
				}
				if (pBlockFile)
				{
					CloseHandle(pBlockFile);
					pBlockFile = NULL;
				}			
			}//for (i = 0;i < 8;i++)将内存中的点一次写入到块文件中

			bufferPts.clear();
			clearSplitMemBuf(blockPoints);
		}//while (m_pHlzWrite->ReadPtsBuffer(pFile, bufferPts) > 0)

		if (pPtBuf4Write)
		{
			delete[] pPtBuf4Write;
			pPtBuf4Write = NULL;
		}

		CloseHandle(pFile);
		pFile = NULL;

		return true;
	}
	// 根据块集划分块文件（相邻区域）
	bool CHlzCloudBuilder::SplitBlockFilesNext(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks)//
	{
// 		FILE* pFile = fopen(procBlockSet.path.c_str(), "rb");
// 		if (pFile == NULL)
// 		{
// 			return false;
// 		}
// 
// 		fseek(pFile, 0, SEEK_SET);// 移到文件头
		HANDLE pFile = CreateFile(procBlockSet.path.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (pFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		// 创建存储目录
		std::string dir = procBlockSet.path;
		dir = dir.substr(0, dir.find_last_of('.'));
		I32 ret = _mkdir(dir.c_str());

		// 一次读取6.4w点到内存后写入到文件
		std::vector<PointXYZIPRGBA> bufferPts;

		PointXYZIPRGBA* ptTmp;
		// 当前点所在的块序号

		CHdVector3df center = procBlockSet.box.getCenter();
		CHdVector3df halfSize = procBlockSet.box.getExtent() /2.0f;
		CHdVector3df centerSlice(0,0,0);
		int nFileX = (int)center.X/m_stepX;
		int nFileY = (int)center.Y/m_stepX;
		centerSlice.X =center.X - nFileX * m_stepX;
		centerSlice.Y =center.Y - nFileY * m_stepY;
		centerSlice.Z = center.Z;

		arrayBlocks.resize(8);

		PointXYZIPRGBA* pPtBuf4Write;
		int wSize = 50000;
		pPtBuf4Write = new PointXYZIPRGBA[wSize];
		while (m_pNextHlzWrite->ReadPtsBySize(pFile, bufferPts,MAX_POINT_NUM) > 0)
		{
			// 切分生成的块文件对应的点
			vector<SplitMemoryBuf> blockPoints;
			blockPoints.resize(8);
			for (I32 i=0; i < bufferPts.size(); i++)
			{

				ptTmp = &(*(bufferPts._Myfirst() + i));
				
				U8 blockIdx = 0;			
				//getPtBlockIndex(*ptTmp, center, blockIdx);
			    getPtBlockIndex(*ptTmp, centerSlice, blockIdx);
				SplitMemoryBuf& memPoints = *(blockPoints._Myfirst() + blockIdx);
				if (memPoints.m_count >= memPoints.m_vecBuf.size())
				{
					memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
				}
				PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
				memPoints.m_count++;
				pPtMem = bufferPts._Myfirst() + i;	
			}//for (I32 i=0; i < bufferPts.size(); i++)

			// 将内存中的点一次写入到块文件中
			for (I32 i = 0;i < 8;i++)
			{
				SplitMemoryBuf& blkPoints = *(blockPoints._Myfirst() + i);
				int nCount = blkPoints.m_count;
				if(nCount <= 0)
					continue;
				//			char blockFilePath[512] = {0};
				//			sprintf_s(blockFilePath, "%s\\block_%d.tmp", dir.c_str(), i);
				char* blockFilePath = new char[dir.length()+13];
				sprintf_s(blockFilePath, dir.length()+13, "%s\\block_%d.tmp", dir.c_str(), i);
// 				FILE* pBlockFile = fopen(blockFilePath, "ab+");
// 				if (pBlockFile == NULL)
// 				{
// 					continue;
// 				}
				HANDLE pBlockFile =  CreateFile(blockFilePath,
					GENERIC_READ|GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE,
					NULL,
					OPEN_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);
				if (pBlockFile == INVALID_HANDLE_VALUE)
				{
					continue;
				}
				LARGE_INTEGER li1,li2;
				li1.HighPart = 0;
				li1.LowPart = 0;
				li2.HighPart = 0;
				li2.LowPart = 0;
				SetFilePointerEx(pBlockFile, li1, &li2, FILE_END);

				BlockFileInfo& blkInfo = *(arrayBlocks._Myfirst() + i);
				blkInfo.path = blockFilePath;
				blkInfo.numPoint += nCount;
				blkInfo.index = i;
				getBlockBox(center,halfSize,blkInfo.box,(U16)i);

				//50000个点为单位写入
				int ptCount = 0;
				int k = 0;
				for (I32 j = 0; j < nCount; j++)
				{
					PointXYZIPRGBA* pPt = *(blkPoints.m_vecBuf._Myfirst() + j);
// 					DWORD numWrite;
// 					WriteFile(pBlockFile, pPt, sizeof(PointXYZIPRGBA), &numWrite, NULL);
//					fwrite(pPt, sizeof(PointXYZIPRGBA), 1, pBlockFile);
					*(pPtBuf4Write + k++) = *pPt;
					if (k == wSize)
					{
						DWORD numWrite;
						WriteFile(pBlockFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
						ptCount ++;
						k = 0;
						delete[] pPtBuf4Write;
						pPtBuf4Write = new PointXYZIPRGBA[wSize];
					}
				}
				int leftnum = nCount - ptCount * wSize;
				if ( leftnum>0)
				{
					DWORD dwResult;
					WriteFile (pBlockFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*leftnum, &dwResult, NULL);
					delete[] pPtBuf4Write;
					pPtBuf4Write = new PointXYZIPRGBA[wSize];
				}

				if (blockFilePath)
				{
					delete[] blockFilePath;
					blockFilePath = NULL;
				}
				if (pBlockFile)
				{
					CloseHandle(pBlockFile);
					pBlockFile = NULL;
				}			
			}//for (i = 0;i < 8;i++)将内存中的点一次写入到块文件中

			bufferPts.clear();
			clearSplitMemBuf(blockPoints);
		}//while (m_pHlzWrite->ReadPtsBuffer(pFile, bufferPts) > 0)

		if (pPtBuf4Write)
		{
			delete[] pPtBuf4Write;
			pPtBuf4Write = NULL;
		}

		CloseHandle(pFile);
		pFile = NULL;

		return true;
	}
	// 将块文件划分成子块
	bool CHlzCloudBuilder::SplitBlock2Parts(const CHdBox3df& blockBox, const string strBlockFile, std::vector<subBlockFileInfo>& subBlkInfo)
	{
		// 打开分块文件
// 		FILE* pBlockFile = fopen(strBlockFile.c_str(), "rb");
// 
// 		if (!pBlockFile) // 打开失败返回
// 		{
// 			return false;
// 		}

		HANDLE pBlockFile = CreateFile(strBlockFile.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (pBlockFile == INVALID_HANDLE_VALUE)
		{
			pBlockFile = NULL;
		}

		string savePath = strBlockFile.substr(0, strBlockFile.find_last_of('.'));
		_mkdir(savePath.c_str());

		// 对块文件进行切分
		std::map<U8, subBlockFileInfo> mapSplitBlk;	// 一个块分割出来的包文件

		std::vector<PointXYZIPRGBA> bufferPts;

		CHdVector3df center = blockBox.getCenter();
		CHdVector3df halfSize = blockBox.getExtent() / 2.0f;
		
		CHdVector3df centerSlice(0,0,0);
		int nFileX = (int)center.X/m_stepX;
		int nFileY = (int)center.Y/m_stepX;
		centerSlice.X =center.X - nFileX * m_stepX;
		centerSlice.Y =center.Y - nFileY * m_stepY;
		centerSlice.Z = center.Z;

		PointXYZIPRGBA* pPtBuf4Write;
		int wSize = 50000;
		pPtBuf4Write = new PointXYZIPRGBA[wSize];

		while (m_pHlzWrite->ReadPtsBySize(pBlockFile, bufferPts, MAX_POINT_NUM) > 0)
		{
			// 切分生成的包文件的点
			vector<SplitMemoryBuf> parcelPoints;
			parcelPoints.resize(8);
			for (I32 i=0; i < bufferPts.size(); i++)
			{
				PointXYZIPRGBA* ptTmp = NULL;
				{
					ptTmp = (bufferPts._Myfirst() + i);
				}

				// 该点所在的包序号
				U8 parcelIdx = 0;
				//getPtBlockIndex(*ptTmp, center, parcelIdx);
				getPtBlockIndex(*ptTmp, centerSlice, parcelIdx);
				SplitMemoryBuf& memPoints = *(parcelPoints._Myfirst() + parcelIdx);
				if (memPoints.m_count >= memPoints.m_vecBuf.size())
				{
					try
					{
						memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
					}
					catch(...)
					{
						string msg = "内存分配失败，请提高内存配置或者使用“临时文件方式”\n(" + strBlockFile +")";
						::MessageBox(NULL, msg.c_str(), NULL, MB_OK);
						continue;
					}
				}

				PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
				memPoints.m_count++;
				pPtMem = bufferPts._Myfirst() + i;
			}//for (I32 i=0; i < bufferPts.size(); i++)

			// 1.将内存中的点一次写入到包文件中; 2.统计包文件信息;
			for (I32 i = 0;i < parcelPoints.size();i++)
			{			
				SplitMemoryBuf& pacPoints = *(parcelPoints._Myfirst() + i);
				int nCount = pacPoints.m_count;
				if (nCount <= 0)
				{
					continue;
				}
				char* subBlkFilePath = new char[savePath.length() + 14];		
				sprintf_s(subBlkFilePath, savePath.length() + 14, "%s\\subBlk_%d.tmp", savePath.c_str(), i);
// 				FILE* pSubBlkFile = fopen(subBlkFilePath, "ab+");
// 				if (pSubBlkFile == NULL)
// 				{
// 					continue;
// 				}
				HANDLE pSubBlkFile = CreateFile(subBlkFilePath,
					GENERIC_READ|GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE,
					NULL, 
					OPEN_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);
				if (pSubBlkFile == INVALID_HANDLE_VALUE)
				{
					continue;
				}
				// 定位到文件尾部
				LARGE_INTEGER li1,li2;
				li1.HighPart = 0;
				li1.LowPart = 0;
				li2.HighPart = 0;
				li2.LowPart = 0;
				SetFilePointerEx(pSubBlkFile, li1, &li2, FILE_END);
				//50000个点为单位写入
				int ptCount = 0;
				int k = 0;
				for (I32 j= 0; j < nCount; j++)
				{
					PointXYZIPRGBA* pPt = *(pacPoints.m_vecBuf._Myfirst() + j);
//					fwrite(pPt, sizeof(PointXYZIPRGBA), 1, pSubBlkFile);
// 					DWORD numWrite;
// 					WriteFile(pSubBlkFile, pPt, sizeof(PointXYZIPRGBA), &numWrite, NULL);
					*(pPtBuf4Write + k++) = *pPt;
					if (k == wSize)
					{
						DWORD numWrite;
						WriteFile(pSubBlkFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
						ptCount ++;
						k = 0;
						delete[] pPtBuf4Write;
						pPtBuf4Write = new PointXYZIPRGBA[wSize];
					}
				}
				int leftnum = nCount - ptCount * wSize;
				if ( leftnum>0)
				{
					DWORD dwResult;
					WriteFile (pSubBlkFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*leftnum, &dwResult, NULL);
					delete[] pPtBuf4Write;
					pPtBuf4Write = new PointXYZIPRGBA[wSize];
				}


				CloseHandle(pSubBlkFile);
				pSubBlkFile = NULL;

				// 收集包文件信息
				if (!mapSplitBlk.count(i))
				{
					subBlockFileInfo info;
					//getBlockBox(parcelFilePath, c, info.box,(U16)i);		// 包的范围
					getBlockBox(center,halfSize,info.box,(U16)i);
					info.path = subBlkFilePath;
					info.numPoint = nCount;
					mapSplitBlk.insert(make_pair(i, info));
				}
				else
				{
					subBlockFileInfo& parcelInfo = mapSplitBlk[i];
					parcelInfo.numPoint += nCount;
				}

				if (subBlkFilePath)
				{
					delete[] subBlkFilePath;
					subBlkFilePath = NULL;
				}
			}//for (I32 i = 0;i < parcelPoints.size();i++)

			// 回收内存
			bufferPts.clear();
			bufferPts.swap(vector<PointXYZIPRGBA>());
			clearSplitMemBuf(parcelPoints);
		}

		// 回收内存
		if (pPtBuf4Write)
		{
			delete[] pPtBuf4Write;
			pPtBuf4Write = NULL;
		}

		// 关闭块文件
		if (pBlockFile)
		{
			CloseHandle(pBlockFile);
			pBlockFile = NULL;
		}

		// 删除块文件
		int err = remove(strBlockFile.c_str());
		if (err == -1)
		{
			string strError = "无法删除文件：" ;
			strError = strError + strBlockFile;
			perror(strError.c_str());
		}

		// 对包文件进行递归切分
		for (std::map<U8, subBlockFileInfo>::iterator it = mapSplitBlk.begin();
			it != mapSplitBlk.end(); it++)
		{
			if ((it->second).numPoint <= MAX_POINT_NUM)
			{
				// 包文件不需要再往下切分
				subBlkInfo.push_back(it->second);
			}
			else
			{
				// 继续往下切分
				SplitBlock2Parts((it->second).box, (it->second).path.c_str(), subBlkInfo);
			}
		}
		mapSplitBlk.clear();
		mapSplitBlk.swap(map<U8, subBlockFileInfo>());
		return true;
	}
	// 将块文件划分成子块
	bool CHlzCloudBuilder::SplitBlock2PartsNext(const CHdBox3df& blockBox, const string strBlockFile, std::vector<subBlockFileInfo>& subBlkInfo)
	{
		// 打开分块文件
// 		FILE* pBlockFile = fopen(strBlockFile.c_str(), "rb");
// 
// 		if (!pBlockFile) // 打开失败返回
// 		{
// 			return false;
// 		}

		HANDLE pBlockFile = CreateFile(strBlockFile.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (pBlockFile == INVALID_HANDLE_VALUE)
		{
			pBlockFile = NULL;
		}

		string savePath = strBlockFile.substr(0, strBlockFile.find_last_of('.'));
		_mkdir(savePath.c_str());

		// 对块文件进行切分
		std::map<U8, subBlockFileInfo> mapSplitBlk;	// 一个块分割出来的包文件

		std::vector<PointXYZIPRGBA> bufferPts;

		CHdVector3df center = blockBox.getCenter();
		CHdVector3df halfSize = blockBox.getExtent() / 2.0f;
		CHdVector3df centerSlice(0,0,0);
		int nFileX = (int)center.X/m_stepX;
		int nFileY = (int)center.Y/m_stepX;
		centerSlice.X =center.X - nFileX * m_stepX;
		centerSlice.Y =center.Y - nFileY * m_stepY;
		centerSlice.Z = center.Z;

		PointXYZIPRGBA* pPtBuf4Write;
		int wSize = 50000;
		pPtBuf4Write = new PointXYZIPRGBA[wSize];

		while (m_pHlzWrite->ReadPtsBySize(pBlockFile, bufferPts, MAX_POINT_NUM) > 0)
		{
			// 切分生成的包文件的点
			vector<SplitMemoryBuf> parcelPoints;
			parcelPoints.resize(8);
			for (I32 i=0; i < bufferPts.size(); i++)
			{
				PointXYZIPRGBA* ptTmp = NULL;
				{
					ptTmp = (bufferPts._Myfirst() + i);
				}

				// 该点所在的包序号
				U8 parcelIdx = 0;
				//getPtBlockIndex(*ptTmp, center, parcelIdx);
				getPtBlockIndex(*ptTmp, centerSlice, parcelIdx);
				SplitMemoryBuf& memPoints = *(parcelPoints._Myfirst() + parcelIdx);
				if (memPoints.m_count >= memPoints.m_vecBuf.size())
				{
					try
					{
						memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
					}
					catch(...)
					{
						string msg = "内存分配失败，请提高内存配置或者使用“临时文件方式”\n(" + strBlockFile +")";
						::MessageBox(NULL, msg.c_str(), NULL, MB_OK);
						continue;
					}
				}

				PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
				memPoints.m_count++;
				pPtMem = bufferPts._Myfirst() + i;
			}//for (I32 i=0; i < bufferPts.size(); i++)

			// 1.将内存中的点一次写入到包文件中; 2.统计包文件信息;
			for (I32 i = 0;i < parcelPoints.size();i++)
			{			
				SplitMemoryBuf& pacPoints = *(parcelPoints._Myfirst() + i);
				int nCount = pacPoints.m_count;
				if (nCount <= 0)
				{
					continue;
				}
				char* subBlkFilePath = new char[savePath.length() + 14];		
				sprintf_s(subBlkFilePath, savePath.length() + 14, "%s\\subBlk_%d.tmp", savePath.c_str(), i);
// 				FILE* pSubBlkFile = fopen(subBlkFilePath, "ab+");
// 				if (pSubBlkFile == NULL)
// 				{
// 					continue;
// 				}
				HANDLE pSubBlkFile = CreateFile(subBlkFilePath,
					GENERIC_READ|GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE,
					NULL, 
					OPEN_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);
				if (pSubBlkFile == INVALID_HANDLE_VALUE)
				{
					continue;
				}
				// 定位到文件尾部
				LARGE_INTEGER li1,li2;
				li1.HighPart = 0;
				li1.LowPart = 0;
				li2.HighPart = 0;
				li2.LowPart = 0;
				SetFilePointerEx(pSubBlkFile, li1, &li2, FILE_END);
				//50000个点为单位写入
				int ptCount = 0;
				int k = 0;
				for (I32 j= 0; j < nCount; j++)
				{
					PointXYZIPRGBA* pPt = *(pacPoints.m_vecBuf._Myfirst() + j);
//					fwrite(pPt, sizeof(PointXYZIPRGBA), 1, pSubBlkFile);
// 					DWORD numWrite;
// 					WriteFile(pSubBlkFile, pPt, sizeof(PointXYZIPRGBA), &numWrite, NULL);
					*(pPtBuf4Write + k++) = *pPt;
					if (k == wSize)
					{
						DWORD numWrite;
						WriteFile(pSubBlkFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
						ptCount ++;
						k = 0;
						delete[] pPtBuf4Write;
						pPtBuf4Write = new PointXYZIPRGBA[wSize];
					}
				}
				int leftnum = nCount - ptCount * wSize;
				if ( leftnum>0)
				{
					DWORD dwResult;
					WriteFile (pSubBlkFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*leftnum, &dwResult, NULL);
					delete[] pPtBuf4Write;
					pPtBuf4Write = new PointXYZIPRGBA[wSize];
				}


				CloseHandle(pSubBlkFile);
				pSubBlkFile = NULL;

				// 收集包文件信息
				if (!mapSplitBlk.count(i))
				{
					subBlockFileInfo info;
					//getBlockBox(parcelFilePath, c, info.box,(U16)i);		// 包的范围
					getBlockBox(center,halfSize,info.box,(U16)i);
					info.path = subBlkFilePath;
					info.numPoint = nCount;
					mapSplitBlk.insert(make_pair(i, info));
				}
				else
				{
					subBlockFileInfo& parcelInfo = mapSplitBlk[i];
					parcelInfo.numPoint += nCount;
				}

				if (subBlkFilePath)
				{
					delete[] subBlkFilePath;
					subBlkFilePath = NULL;
				}
			}//for (I32 i = 0;i < parcelPoints.size();i++)

			// 回收内存
			bufferPts.clear();
			bufferPts.swap(vector<PointXYZIPRGBA>());
			clearSplitMemBuf(parcelPoints);
		}

		// 回收内存
		if (pPtBuf4Write)
		{
			delete[] pPtBuf4Write;
			pPtBuf4Write = NULL;
		}

		// 关闭块文件
		if (pBlockFile)
		{
			CloseHandle(pBlockFile);
			pBlockFile = NULL;
		}

		// 删除块文件
		int err = remove(strBlockFile.c_str());
		if (err == -1)
		{
			string strError = "无法删除文件：" ;
			strError = strError + strBlockFile;
			perror(strError.c_str());
		}

		// 对包文件进行递归切分
		for (std::map<U8, subBlockFileInfo>::iterator it = mapSplitBlk.begin();
			it != mapSplitBlk.end(); it++)
		{
			if ((it->second).numPoint <= MAX_POINT_NUM)
			{
				// 包文件不需要再往下切分
				subBlkInfo.push_back(it->second);
			}
			else
			{
				// 继续往下切分
				SplitBlock2Parts((it->second).box, (it->second).path.c_str(), subBlkInfo);
			}
		}
		mapSplitBlk.clear();
		mapSplitBlk.swap(map<U8, subBlockFileInfo>());
		return true;
	}
	// 子块文件分包
	bool CHlzCloudBuilder::SplitSbkFile2Parcel_Buffer(const CHdBox3df& blockBox, const string strSbkFile, std::vector<ParcelFileInfo>& vecParcelInfo)
	{
		// 打开分块文件
		FILE* pBlockFile = fopen(strSbkFile.c_str(), "rb");

		if (!pBlockFile) // 打开失败返回
		{
			return false;
		}

// 		HANDLE pBlockFile = CreateFile(strSbkFile.c_str(),
// 			GENERIC_READ,
// 			FILE_SHARE_READ,
// 			NULL,
// 			OPEN_EXISTING,
// 			FILE_ATTRIBUTE_NORMAL,
// 			NULL);
// 		if (pBlockFile == INVALID_HANDLE_VALUE)
// 		{
// 			return false;
// 		}

		string savePath = strSbkFile.substr(0, strSbkFile.find_last_of('.'));
		//		_mkdir(savePath.c_str());

		// 读取子块中的所有点
		m_pHlzWrite->ReadAllPts(pBlockFile, m_blsDataBuf);
		if (pBlockFile)
		{
//			CloseHandle(pBlockFile);
			fclose(pBlockFile);
			pBlockFile = NULL;
		}

		// 初始化块数据指针
		m_pBlockData[savePath].resize(m_blsDataBuf.size());
		std::map<string , vector<PointXYZIPRGBA *> >::iterator iter_block = m_pBlockData.find(savePath);
		for (int i=0; i< iter_block->second.size(); i++)
		{
			PointXYZIPRGBA* &pPt = *(iter_block->second._Myfirst() + i);
			pPt = m_blsDataBuf._Myfirst() + i;
		}

		// 对块文件进行切分
		std::map<U8, ParcelFileInfo> mapSplitParcels;	// 一个块分割出来的包文件

		CHdVector3df center = blockBox.getCenter();
		CHdVector3df halfSize = blockBox.getExtent() / 2.0f;

		CHdVector3df centerSlice(0,0,0);
		int nFileX = (int)center.X/m_stepX;
		int nFileY = (int)center.Y/m_stepX;
		centerSlice.X =center.X - nFileX * m_stepX;
		centerSlice.Y =center.Y - nFileY * m_stepY;
		centerSlice.Z = center.Z;

		// 切分生成的包文件的点
		vector<SplitMemoryBuf> parcelPoints;
		parcelPoints.resize(8);
		for (I32 i=0; i < iter_block->second.size(); i++)
		{
			PointXYZIPRGBA* ptTmp = NULL;
			ptTmp = *(iter_block->second._Myfirst() + i);

			// 该点所在的包序号
			U8 parcelIdx = 0;
			//getPtBlockIndex(*ptTmp, center, parcelIdx);
            getPtBlockIndex(*ptTmp, centerSlice, parcelIdx);
			SplitMemoryBuf& memPoints = *(parcelPoints._Myfirst() + parcelIdx);
			if (memPoints.m_count >= memPoints.m_vecBuf.size())
			{
				try
				{
					memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
				}
				catch(...)
				{
					::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
					// 删除临时文件
					RemoveTempFiles(m_savePath.c_str());
					return false;
				}
			}
			PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
			memPoints.m_count++;
			pPtMem = *(iter_block->second._Myfirst() + i);
		}  //for (I32 i=0; i < iter_block->second.size(); i++)

		char parcelDataID[512] = {0};	
		// 1.将内存中的点一次写入到包文件中; 2.统计包文件信息;
		for (I32 i = 0;i < parcelPoints.size();i++)
		{			
			SplitMemoryBuf& pacPoints = *(parcelPoints._Myfirst() + i);
			int nCount = pacPoints.m_count;
			if (nCount <= 0)
			{
				continue;
			}

			sprintf_s(parcelDataID, "%s\\parcel_%d.tmp", savePath.c_str(), i);
			hd::stringc strParcelDataID = parcelDataID;		
			//写入数据
			int preSize = m_ParcelData[strParcelDataID.c_str()].size();
			//重新分配内存空间
			try
			{
				m_ParcelData[strParcelDataID.c_str()].resize(preSize + nCount);
			}
			catch(...)
			{
				::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
				// 删除临时文件
				RemoveTempFiles(m_savePath.c_str());
				return false;
			}
			std:: map<string , vector<PointXYZIPRGBA *> > :: iterator iter_parcel = m_ParcelData.find(strParcelDataID.c_str());
// 			for (I32 j= 0; j < nCount; j++)
// 			{
// 				// 点数据指针压入数据中
// 				*(iter_parcel->second._Myfirst() + preSize + j) = *(pacPoints.m_vecBuf._Myfirst() + j);
// 			}
			memcpy(iter_parcel->second._Myfirst() + preSize, pacPoints.m_vecBuf._Myfirst(), nCount* sizeof(PointXYZIPRGBA*));


			// 收集包文件信息
			if (!mapSplitParcels.count(i))
			{
				ParcelFileInfo info;
				//getBlockBox(parcelFilePath, c, info.box,(U16)i);		// 包的范围
				getBlockBox(center,halfSize,info.box,(U16)i);
				info.path = parcelDataID;
				info.numPoint = nCount;
				mapSplitParcels.insert(make_pair(i, info));
			}
			else
			{
				ParcelFileInfo& parcelInfo = mapSplitParcels[i];
				parcelInfo.numPoint += nCount;
			}
		}
		//清空分包的临时数据
		clearSplitMemBuf(parcelPoints);

		// 删除子块文件
		int err = remove(strSbkFile.c_str());
		if (err == -1)
		{
			string strError = "无法删除文件：" ;
			strError = strError + strSbkFile;
			perror(strError.c_str());
		}

		// 对包文件进行递归切分
		for (std::map<U8, ParcelFileInfo>::iterator it = mapSplitParcels.begin();
			it != mapSplitParcels.end(); it++)
		{
			if ((it->second).numPoint <= BLOCK_HOLD)
			{
				// 包文件不需要再往下切分
				vecParcelInfo.push_back(it->second);
			}
			else
			{
				std::string dir = (it->second).path.c_str();
				dir = dir.substr(0, dir.find_last_of('.'));
				I32 ret = _mkdir(dir.c_str());

				char splitPath[MAX_PATH] = {0};
				sprintf_s(splitPath,"%s", dir.c_str());

				// 继续往下切分
				if(SplitParcel2Parcel_Buffer((it->second).box, (it->second).path.c_str(), splitPath, vecParcelInfo))
				{
					//切分完成后从内存中删除该包的数据
					m_ParcelData.erase(it->second.path.c_str());
				}
			}
		}

		return true;
	}
	// 子块文件分包（相邻区域分包）
	bool CHlzCloudBuilder::SplitSbkFile2Parcel_BufferNext(const CHdBox3df& blockBox, const string strSbkFile, std::vector<ParcelFileInfo>& vecParcelInfo)
	{
		// 打开分块文件
		FILE* pBlockFile = fopen(strSbkFile.c_str(), "rb");

		if (!pBlockFile) // 打开失败返回
		{
			return false;
		}

// 		HANDLE pBlockFile = CreateFile(strSbkFile.c_str(),
// 			GENERIC_READ,
// 			FILE_SHARE_READ,
// 			NULL,
// 			OPEN_EXISTING,
// 			FILE_ATTRIBUTE_NORMAL,
// 			NULL);
// 		if (pBlockFile == INVALID_HANDLE_VALUE)
// 		{
// 			return false;
// 		}

		string savePath = strSbkFile.substr(0, strSbkFile.find_last_of('.'));
		//		_mkdir(savePath.c_str());

		// 读取子块中的所有点
		m_pNextHlzWrite->ReadAllPts(pBlockFile, m_blsDataBuf);
		if (pBlockFile)
		{
//			CloseHandle(pBlockFile);
			fclose(pBlockFile);
			pBlockFile = NULL;
		}

		// 初始化块数据指针
		m_pBlockData[savePath].resize(m_blsDataBuf.size());
		std::map<string , vector<PointXYZIPRGBA *> >::iterator iter_block = m_pBlockData.find(savePath);
		for (int i=0; i< iter_block->second.size(); i++)
		{
			PointXYZIPRGBA* &pPt = *(iter_block->second._Myfirst() + i);
			pPt = m_blsDataBuf._Myfirst() + i;
		}

		// 对块文件进行切分
		std::map<U8, ParcelFileInfo> mapSplitParcels;	// 一个块分割出来的包文件

		CHdVector3df center = blockBox.getCenter();
		CHdVector3df halfSize = blockBox.getExtent() / 2.0f;

		CHdVector3df centerSlice(0,0,0);
		int nFileX = (int)center.X/m_stepX;
		int nFileY = (int)center.Y/m_stepX;
		centerSlice.X =center.X - nFileX * m_stepX;
		centerSlice.Y =center.Y - nFileY * m_stepY;
		centerSlice.Z = center.Z;

		// 切分生成的包文件的点
		vector<SplitMemoryBuf> parcelPoints;
		parcelPoints.resize(8);
		for (I32 i=0; i < iter_block->second.size(); i++)
		{
			PointXYZIPRGBA* ptTmp = NULL;
			ptTmp = *(iter_block->second._Myfirst() + i);

			// 该点所在的包序号
			U8 parcelIdx = 0;
			//getPtBlockIndex(*ptTmp, center, parcelIdx);
			getPtBlockIndex(*ptTmp, centerSlice, parcelIdx);
			SplitMemoryBuf& memPoints = *(parcelPoints._Myfirst() + parcelIdx);
			if (memPoints.m_count >= memPoints.m_vecBuf.size())
			{
				try
				{
					memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
				}
				catch(...)
				{
					::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
					// 删除临时文件
					RemoveTempFiles(m_savePath.c_str());
					return false;
				}
			}
			PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
			memPoints.m_count++;
			pPtMem = *(iter_block->second._Myfirst() + i);
		}  //for (I32 i=0; i < iter_block->second.size(); i++)

		char parcelDataID[512] = {0};	
		// 1.将内存中的点一次写入到包文件中; 2.统计包文件信息;
		for (I32 i = 0;i < parcelPoints.size();i++)
		{			
			SplitMemoryBuf& pacPoints = *(parcelPoints._Myfirst() + i);
			int nCount = pacPoints.m_count;
			if (nCount <= 0)
			{
				continue;
			}

			sprintf_s(parcelDataID, "%s\\parcel_%d.tmp", savePath.c_str(), i);
			hd::stringc strParcelDataID = parcelDataID;		
			//写入数据
			int preSize = m_ParcelData[strParcelDataID.c_str()].size();
			//重新分配内存空间
			try
			{
				m_ParcelData[strParcelDataID.c_str()].resize(preSize + nCount);
			}
			catch(...)
			{
				::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
				// 删除临时文件
				RemoveTempFiles(m_savePath.c_str());
				return false;
			}
			std:: map<string , vector<PointXYZIPRGBA *> > :: iterator iter_parcel = m_ParcelData.find(strParcelDataID.c_str());
// 			for (I32 j= 0; j < nCount; j++)
// 			{
// 				// 点数据指针压入数据中
// 				*(iter_parcel->second._Myfirst() + preSize + j) = *(pacPoints.m_vecBuf._Myfirst() + j);
// 			}
			memcpy(iter_parcel->second._Myfirst() + preSize, pacPoints.m_vecBuf._Myfirst(), nCount* sizeof(PointXYZIPRGBA*));


			// 收集包文件信息
			if (!mapSplitParcels.count(i))
			{
				ParcelFileInfo info;
				//getBlockBox(parcelFilePath, c, info.box,(U16)i);		// 包的范围
				getBlockBox(center,halfSize,info.box,(U16)i);
				info.path = parcelDataID;
				info.numPoint = nCount;
				mapSplitParcels.insert(make_pair(i, info));
			}
			else
			{
				ParcelFileInfo& parcelInfo = mapSplitParcels[i];
				parcelInfo.numPoint += nCount;
			}
		}
		//清空分包的临时数据
		clearSplitMemBuf(parcelPoints);

		// 删除子块文件
		int err = remove(strSbkFile.c_str());
		if (err == -1)
		{
			string strError = "无法删除文件：" ;
			strError = strError + strSbkFile;
			perror(strError.c_str());
		}

		// 对包文件进行递归切分
		for (std::map<U8, ParcelFileInfo>::iterator it = mapSplitParcels.begin();
			it != mapSplitParcels.end(); it++)
		{
			if ((it->second).numPoint <= BLOCK_HOLD)
			{
				// 包文件不需要再往下切分
				vecParcelInfo.push_back(it->second);
			}
			else
			{
				std::string dir = (it->second).path.c_str();
				dir = dir.substr(0, dir.find_last_of('.'));
				I32 ret = _mkdir(dir.c_str());

				char splitPath[MAX_PATH] = {0};
				sprintf_s(splitPath,"%s", dir.c_str());

				// 继续往下切分
				if(SplitParcel2Parcel_Buffer((it->second).box, (it->second).path.c_str(), splitPath, vecParcelInfo))
				{
					//切分完成后从内存中删除该包的数据
					m_ParcelData.erase(it->second.path.c_str());
				}
			}
		}

		return true;
	}
	//! 切割Las到块集文件
	bool CHlzCloudBuilder::SplitBlockSetLas(const char* savePath,hd::stringc& strLasFile)
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

		PointXYZIPRGBA* pPtBuf4Write;
		int wSize = 50000;
		pPtBuf4Write = new PointXYZIPRGBA[wSize];
		while((npoint - nCurLp) >= 0)
		{
			hdVector<PointXYZIPRGBA> bufArray;
			try
			{
				bufArray.resize(nBufferSize);
			}
			catch(...)
			{
				::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
				// 删除临时文件
				RemoveTempFiles(m_savePath.c_str());
				return false;
			}
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

			for (I32 k=0; k < i; k++)
			{			
				PointXYZIPRGBA& pt = bufArray[k];
				// 计算块集编号
				U32 xNo = (U32)((pt.x) / m_stepX);
				U32 yNo = (U32)((pt.y) / m_stepY);			
				U32 fileNo = pHdLevel->m_level.numBlocksetX*yNo + xNo;

				SplitMemoryBuf& memPoints = BsPointsArray[fileNo];

				if (memPoints.m_count >= memPoints.m_vecBuf.size())
				{
					try
					{
						memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
					}
					catch(...)
					{
						::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
						// 删除临时文件
						RemoveTempFiles(m_savePath.c_str());
						return false;
					}
				}
				PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
				memPoints.m_count++;
				pPtMem = &pt;					

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
//				FILE* pFile = fopen(filePath,"ab+");
				HANDLE pFile = CreateFile(filePath,
					GENERIC_READ|GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE,
					NULL,
					OPEN_ALWAYS, 
					FILE_ATTRIBUTE_NORMAL, 
					NULL);
				if (pFile != INVALID_HANDLE_VALUE)
				{			
					LARGE_INTEGER li1,li2;
					li1.HighPart = 0;
					li1.LowPart = 0;
					li2.HighPart = 0;
					li2.LowPart = 0;
					SetFilePointerEx(pFile, li1, &li2, FILE_END);

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

					//50000个点为单位写入
					int ptCount = 0;
					int k = 0;
					DWORD numWrite;

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

							*(pPtBuf4Write + k) = *pPt;
							k++;

							if (k == wSize)
							{
								WriteFile(pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
								ptCount++;
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

							*(pPtBuf4Write + k) = *pPt;
							k++;

							if (k == wSize)
							{
								//WriteFile (pBlstDataFile, pBuffer, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);
								WriteFile(pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
								ptCount++;
								k = 0;
								//delete [] pBuffer;
								//pBuffer = new PointXYZIPRGBA [BUFFER_SIZE];
							}
						}
					}

					//for (U32 m=0; m < nCount; )
					//{
					//	PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + m++);
					//	*(pPtBuf4Write + k++) = *pPt;
					//
					//	blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
					//	blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

					//	if (k == wSize)
					//	{
					//		
					//		WriteFile(pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
					//		ptCount++;
					//		k = 0;
					//		//delete[] pPtBuf4Write;
					//		//pPtBuf4Write = new PointXYZIPRGBA[wSize];
					//	}
					//}

					// 剩余点写入
					if ( k > 0)
					{
						DWORD dwResult;
						WriteFile (pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);

						//delete[] pPtBuf4Write;
						//pPtBuf4Write = new PointXYZIPRGBA[wSize];
					}

//					for (U32 m=0; m < nCount; m++)
//					{
//						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + m);
////						fwrite(pPt, sizeof(PointXYZIPRGBA), 1, pFile);
//// 						DWORD numWrite;
//// 						WriteFile(pFile, pPt,sizeof(PointXYZIPRGBA), &numWrite, NULL);
//						*(pPtBuf4Write + k++) = *pPt;
//
//						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
//						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);
//
//						if (k == wSize)
//						{
//							DWORD numWrite;
//							WriteFile(pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
//							ptCount ++;
//							k = 0;
//							delete[] pPtBuf4Write;
//							pPtBuf4Write = new PointXYZIPRGBA[wSize];
//						}
//					}
//					int leftnum = nCount - ptCount*wSize;
//					if ( leftnum>0)
//					{
//						DWORD dwResult;
//						WriteFile (pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*leftnum, &dwResult, NULL);
//						delete[] pPtBuf4Write;
//						pPtBuf4Write = new PointXYZIPRGBA[wSize];
//					}

					// 更新文件名
					m_BlocksetFiles[it->first].path = filePath;
				}

				if (pFile)
				{
					CloseHandle(pFile);
					pFile = NULL;
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

		if (pPtBuf4Write)
		{
			delete[] pPtBuf4Write;
			pPtBuf4Write = NULL;
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
			U32 yNo = BsIndex / pHdLevel->m_level.numBlocksetX;
			U32 xNo = BsIndex - (yNo * pHdLevel->m_level.numBlocksetX);

			BlockSetFileInfo& BsInfo = it->second;
			BsInfo.box.MinEdge.X = xNo*m_stepX;
			BsInfo.box.MinEdge.Y = yNo*m_stepY;

			BsInfo.box.MaxEdge.X = (xNo+1)*m_stepX;
			BsInfo.box.MaxEdge.Y = (yNo+1)*m_stepY;
		}

		return true;
	}

	bool CHlzCloudBuilder::SplitInputFiles(const char* savePath)
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

		//F64 xRange = m_fullExtent.MaxEdge.X - m_zoneStartX;
		//F64 yRange = m_fullExtent.MaxEdge.Y - m_zoneStartY;
		F64 xRange = m_fullExtent.MaxEdge.X - m_fullExtent.MinEdge.X;
		F64 yRange = m_fullExtent.MaxEdge.Y - m_fullExtent.MinEdge.Y;
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
		CHdVector3dd szBox = /*m_zoneSliceExtent.getExtent()*/m_fullExtent.getExtent();

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
			if (selNum >= (int)(m_pCoreData->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMax = j;
				break;
			}
		}

		selNum = 0;
		for (int j = 0;j <= 999;j++)
		{
			selNum += m_coordStats.intStepStat[j];
			if (selNum >= (int)(m_pCoreData->m_hlzHeader.number_of_point_records * 0.03))
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
	bool CHlzCloudBuilder::SplitInputMultiZoneFiles(const char* savePath)
	{
		// 进度条开始
		if (processCallback)
		{
			processCallback(0.0f, HDSCENE_IDS_BUILDER_DIVIDE_GRID);
		}

		// 定义统计坐标渲染增强变量
		U64 count = m_pCoreData->m_hlzHeader.number_of_point_records;
		U64 countNext = m_pCoreDataNext->m_hlzHeader.number_of_point_records;

		int selMax = 0;
		int selMin = 0;
		int selMaxNext = 0;
		int selMinNext = 0;

		m_coordStats.Clear();
		m_coordStatsNext.Clear();

		// 当前分区的统计参数
		F64 xRange = m_fullExtent.MaxEdge.X - m_fullExtent.MinEdge.X;
		F64 yRange = m_fullExtent.MaxEdge.Y - m_fullExtent.MinEdge.Y;
		F64 zRange = m_fullExtent.getExtent().Z;

		m_coordStats.xRcp = (F32)(1000.0f / (xRange));
		m_coordStats.yRcp = (F32)(1000.0f / (yRange));
		m_coordStats.zRcp = (F32)(1000.0f / (zRange));
		m_coordStats.iRcp = (F32)(1000.0f / (m_iMax - m_iMin));

		// 对应分区的统计参数，强度沿用文件统计值
		F64 xRangeNext = m_fullExtentNext.MaxEdge.X - m_fullExtentNext.MinEdge.X;
		F64 yRangeNext = m_fullExtentNext.MaxEdge.Y - m_fullExtentNext.MinEdge.Y;
		F64 zRangeNext = m_fullExtentNext.getExtent().Z;

		m_coordStatsNext.xRcp = (F32)(1000.0f / (xRangeNext));
		m_coordStatsNext.yRcp = (F32)(1000.0f / (yRangeNext));
		m_coordStatsNext.zRcp = (F32)(1000.0f / (zRangeNext));
		m_coordStatsNext.iRcp = (F32)(1000.0f / (m_iMax - m_iMin));


		for (U32 i = 0;i< m_hlsFileList.size();i++)
		{
			hd::stringc strExt = m_hlsFileList[i].subString(m_hlsFileList[i].size()-3, 3);
			strExt.make_lower();

			//如果是hls文件
			if (strExt == "hls")
			{
				if(!SplitMultiZoneBlockSetFiles(savePath, m_hlsFileList[i]))
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

		for (map<U32, BlockSetFileInfo>::iterator it = m_BlocksetFilesNext.begin();
			it != m_BlocksetFilesNext.end(); it++)
		{
			BlockSetFileInfo& BsInfo = it->second;
			if(BsInfo.box.MaxEdge.Z - BsInfo.box.MinEdge.Z < 0.001f)
			{
				BsInfo.box.MinEdge.Z = BsInfo.box.MinEdge.Z - 0.0005f;
				BsInfo.box.MaxEdge.Z = BsInfo.box.MaxEdge.Z + 0.0005f;
			}
		}

		// 统计得到当前投影带内点云数
		m_pCoreData->m_hlzHeader.number_of_point_records = m_ptNum;

		//调整本投影带X坐标分布，将点数较少部分剔除不参与统计，获得新的最大最小X值
		U64 selNum = 0;
		CHdVector3dd szBox = /*m_zoneSliceExtent.getExtent()*/m_fullExtent.getExtent();

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
			if (selNum >= (int)(m_pCoreData->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMax = j;
				break;
			}
		}

		selNum = 0;
		for (int j = 0;j <= 999;j++)
		{
			selNum += m_coordStats.intStepStat[j];
			if (selNum >= (int)(m_pCoreData->m_hlzHeader.number_of_point_records * 0.03))
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

		// 统计得到相邻投影带内点云数
		m_pCoreDataNext->m_hlzHeader.number_of_point_records = m_ptNumNext;

		//调整本投影带X坐标分布，将点数较少部分剔除不参与统计，获得新的最大最小X值
		U64 selNumNext = 0;
		CHdVector3dd szBoxNext = /*m_zoneSliceExtent.getExtent()*/m_fullExtentNext.getExtent();

		for (int j = 999;j >= 0;j--)
		{
			selNumNext += m_coordStatsNext.xStepStat[j];
			if (selNumNext >= (int)(m_pCoreDataNext->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMaxNext = j;
				break;
			}
		}

		selNumNext = 0;
		for (int j = 0;j <= 999;j++)
		{
			selNumNext += m_coordStatsNext.xStepStat[j];
			if (selNumNext >= (int)(m_pCoreDataNext->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMinNext = j;
				break;
			}
		}

		m_pCoreDataNext->m_hlzHeader.renderMinX = (F32)(selMinNext * (szBoxNext.X) / 1000.0f);
		m_pCoreDataNext->m_hlzHeader.renderMaxX = (F32)(selMaxNext * (szBoxNext.X) / 1000.0f);

		// 调整y坐标分布
		selNumNext = 0;
		for (int j = 999;j >= 0;j--)
		{
			selNumNext += m_coordStatsNext.yStepStat[j];
			if (selNumNext >= (int)(m_pCoreDataNext->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMaxNext = j;
				break;
			}
		}

		selNumNext = 0;
		for (int j = 0;j <= 999;j++)
		{
			selNumNext += m_coordStatsNext.yStepStat[j];
			if (selNumNext >= (int)(m_pCoreDataNext->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMinNext = j;
				break;
			}
		}
		m_pCoreDataNext->m_hlzHeader.renderMinY = (F32)(selMinNext * (szBoxNext.Y) / 1000.0f);
		m_pCoreDataNext->m_hlzHeader.renderMaxY = (F32)(selMaxNext * (szBoxNext.Y) / 1000.0f);

		// 调整z坐标分布
		selNumNext = 0;
		for (int j = 999;j >= 0;j--)
		{
			selNumNext += m_coordStatsNext.zStepStat[j];
			if (selNumNext >= (int)(m_pCoreDataNext->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMaxNext = j;
				break;
			}
		}

		selNumNext = 0;
		for (int j = 0;j <= 999;j++)
		{
			selNumNext += m_coordStatsNext.zStepStat[j];
			if (selNumNext >= (int)(m_pCoreDataNext->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMinNext = j;
				break;
			}
		}
		m_pCoreDataNext->m_hlzHeader.renderMinZ = (F32)(selMinNext * (szBoxNext.Z) / 1000.0f);
		m_pCoreDataNext->m_hlzHeader.renderMaxZ = (F32)(selMaxNext * (szBoxNext.Z) / 1000.0f);

		// 调整强度分布
		selNumNext = 0;
		for (int j = 999;j >= 0;j--)
		{
			selNumNext += m_coordStatsNext.intStepStat[j];
			if (selNumNext >= (int)(m_pCoreDataNext->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMaxNext = j;
				break;
			}
		}

		selNumNext = 0;
		for (int j = 0;j <= 999;j++)
		{
			selNumNext += m_coordStatsNext.intStepStat[j];
			if (selNumNext >= (int)(m_pCoreDataNext->m_hlzHeader.number_of_point_records * 0.03))
			{
				selMinNext = j;
				break;
			}
		}
		U32 iRangeNext = m_iMax - m_iMin;
		U32 iMinNext = 0;
		U32 iMaxNext = 0;
		iMinNext = (U32)(selMinNext * (iRangeNext) / 1000.0f);
		iMaxNext = (U32)(selMaxNext * (iRangeNext) / 1000.0f);
		m_pCoreDataNext->m_hlzHeader.intensityMin = (U8)(iMinNext * 255 / (F32)iRangeNext);
		m_pCoreDataNext->m_hlzHeader.intensityMax = (U8)(iMaxNext * 255 / (F32)iRangeNext);

		return true;
	}

	// hls转hlz点云接口，涉及到分带切割点云需要传人两个保存路径，非分带的只需要保存一个路径
	bool CHlzCloudBuilder::buildHlz( const char* savePath,const char* savePathNext,  BOOL isCompress,int nCenterLongitude)
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
		m_pCoreDataNext->Close();
		// 解析保存文件路径
		char drive[256] = {0};// 磁盘
		char dir[256] = {0};// 文件夹
		char filename[256] = {0};// 文件名
		char ext[256] = {0};// 文件格式
		char path[256] = {0};// 文件路径
		
		// 切分到下一带或上一带的文件路径
		//savePathNext = "F:\\shanghai\\hlstohlz\\next_hlz\\iScan-Pcd-1.hlz";
		 
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
		m_savePath += "\\__temp_to_hlz";

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
		m_pHlzWrite->m_header.isCompress = isCompress;

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
		// 验证点云坐标转换，示例点云数据中央经线为114
		// 该对应Gauss3度带投影的区块划分起点为(112.5,0);
		// 设置对应工程融合解算中央经线，外部传入
		m_dCenter = nCenterLongitude;
		// 计算对应工程投影带的起止点坐标
		double dCenterLongitude = m_dCenter - 1.5;
		CGeoPositionTran positionTran;
		double dLongitude = dCenterLongitude;
		double dLatitude = 0.0;
		double dMinXStart = 0.0;
		double dMinYStart = 0.0;
		positionTran.TranslateDegree2Gauss(dLongitude,dLatitude,dMinXStart,dMinYStart);
		
		// 下一个带范围起点
		double dLongitudeNext = dCenterLongitude + 3.0 - 0.00000000001;
		double dLatitudeNext = 0.0;
		double dMinXStartNext = 0.0;
		double dMinYStartNext = 0.0;
		positionTran.TranslateDegree2Gauss(dLongitudeNext,dLatitudeNext,dMinXStartNext,dMinYStartNext);
		
		// gauss转经纬度验证，转换后坐标精度损失3mm左右
		double dCenter = 114.2854718189;
		positionTran.TranslateGauss2Degree(dMinXStart,dMinYStart,dCenter,dLongitude,dLatitude);

		// 得到所属3度带区域起点XY；
		m_zoneStartX = dMinXStart;
		m_zoneStartY = dMinYStart;	
		m_zoneEndX = dMinXStartNext;
		m_zoneEndY = dMinYStartNext;
		
		//// 根据点云包围盒的位置确定工程跨带情况
		// 单工程点云没有跨越分界子午线所在轴，不需要进行点云切割
		if (dMinX > m_zoneStartX  && dMaxX < m_zoneEndX)
		{
			// 投影代号记录，分辨是否是云存储
			m_pHlzWrite->m_header.iZoneID = (U8)(m_dCenter / 3 + 0.5);
			// 点云外包围盒为点云工程真实包围盒
			m_fullExtent.MinEdge.X = dMinX;
			m_fullExtent.MinEdge.Y = dMinY;
			m_fullExtent.MinEdge.Z = dMinZ;
			m_fullExtent.MaxEdge.X = dMaxX;
			m_fullExtent.MaxEdge.Y = dMaxY;
			m_fullExtent.MaxEdge.Z = dMaxZ;
			// 计算点云外包围和的最小minEdge落在哪个块集，找到对应块集原点
			U32 nBlockSetNoX = (U32)((m_fullExtent.MinEdge.X -m_zoneStartX) / m_stepX);
			U32 nBlockSetNoY = (U32)((m_fullExtent.MinEdge.Y -m_zoneStartY) / m_stepY);
			m_blockSetStartX = nBlockSetNoX * m_stepX + m_zoneStartX;
			m_blockSetStartY = nBlockSetNoY * m_stepY + m_zoneStartY;

			// 分带格网范围，最小点XY落在该带起点，最大点为点云包围盒Max点；
			m_zoneSliceExtent.MinEdge.X = m_zoneStartX;
			m_zoneSliceExtent.MinEdge.Y = m_zoneStartY;
			m_zoneSliceExtent.MinEdge.Z = m_fullExtent.MinEdge.Z;
			m_zoneSliceExtent.MaxEdge.X = m_fullExtent.MaxEdge.X;
			m_zoneSliceExtent.MaxEdge.Y = m_fullExtent.MaxEdge.Y;
			m_zoneSliceExtent.MaxEdge.Z = m_fullExtent.MaxEdge.Z;

			// 文件头信息，为保证点云显示逻辑一致，将偏移量设置为点云外包围和最小点相对于网格起点的偏移量
			m_pHlzWrite->m_header.offsetX = m_blockSetStartX/*m_fullExtent.MinEdge.X*//*dMinXStart*/;
			m_pHlzWrite->m_header.offsetY = m_blockSetStartY/*m_fullExtent.MinEdge.Y*//*dMinYStart*/;
			m_pHlzWrite->m_header.offsetZ = m_fullExtent.MinEdge.Z;
			//m_pHlzWrite->m_header.min_x = (F32)(dMinX - m_pHlzWrite->m_header.offsetX);
			//m_pHlzWrite->m_header.min_y = (F32)(dMinY - m_pHlzWrite->m_header.offsetY);
			// 点云相对坐标范围
			m_pHlzWrite->m_header.min_x = (F32)(0.0);
			m_pHlzWrite->m_header.min_y = (F32)(0.0);
			m_pHlzWrite->m_header.min_z = (F32)(dMinZ - m_pHlzWrite->m_header.offsetZ);
			m_pHlzWrite->m_header.max_x = (F32)(dMaxX - m_pHlzWrite->m_header.offsetX);
			m_pHlzWrite->m_header.max_y = (F32)(dMaxY - m_pHlzWrite->m_header.offsetY);
			m_pHlzWrite->m_header.max_z = (F32)(dMaxZ - m_pHlzWrite->m_header.offsetZ);

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
			CHdLevel* pLevel = new CHdLevel;
			pLevel->m_levelNo = 0;
			m_pCoreData->AddLevelRec(pLevel);

			// 跟据工程点云的实际情况确定对应块的块集编号
			pLevel->m_level.numBlocksetX = (U16)ceil((m_fullExtent.MaxEdge.X - m_blockSetStartX) / m_stepX);//(U32)pow(2.f,baseNum);
			pLevel->m_level.numBlocksetY = (U16)ceil((m_fullExtent.MaxEdge.Y - m_blockSetStartY) / m_stepY);//(U32)pow(2.f,baseNum);
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

			// 将所有块集文件关闭
			CloseTempFiles();

			double stop_time = GetTickCount();

			printf("划分块集耗时：%f 秒\n", (stop_time - start_time)/((float)1000));

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

			// 统计并输出信息
			// 		string strSavePath = savePath; 
			// 		string strInfo = strSavePath.substr(0,strSavePath.length()-4);
			// 		strInfo += "_Info.txt";
			// 		FILE* pInfo = fopen(strInfo.c_str(),"wt");
			// 		U32 preSize;
			// 		if (pInfo != NULL)
			// 		{
			// 			U32 ptNum = pLevel->m_level.pointNum;
			// 			U16 lNo = pLevel->m_levelNo;
			// 			U32 lvsize = ptNum * (sizeof(HdPointXYZ) + sizeof(U8));
			// 			preSize = lvsize;
			// 			fprintf(pInfo, "第%u层信息：\n\t点数：%lu\n\t大小：%lu\n", lNo, ptNum, lvsize);
			// 		}

			//当前层的点数小于100w时不再分下一层
			U8 nLevelNo = 1;
			CHdLevel* levelPre = pLevel;
			//HdLevel levelNext;
			start_time = GetTickCount();
			while (levelPre->m_level.pointNum > 1000000)
			{
				// 层的日志信息
				if(DEBUG_LOG)
				{	char strLv[512] = {0};
				sprintf_s(strLv,"level%d:\n",nLevelNo);
				m_pHlzWrite->WriteLogFile(strLv);
				m_pHlzWrite->WriteLogFile("类型,点数,子块数,范围\n");
				}
				// 下一层
				CHdLevel* pNextLevel = new CHdLevel;
				pNextLevel->m_levelNo = nLevelNo;
				m_pCoreData->AddLevelRec(pNextLevel);
				writeNextLevelData(levelPre, nLevelNo, pNextLevel);
				// 			if (pInfo != NULL)
				// 			{
				// 				U32 ptNum = pNextLevel->m_level.pointNum;
				// 				U16 lNo = pNextLevel->m_levelNo;
				// 				U32 lvsize = ptNum * (sizeof(HdPointXYZ) + sizeof(U8));
				// 				float sacle = (float)((float)preSize/(float)lvsize);
				// 				fprintf(pInfo, "第%u层信息：\n\t点数：%lu\n\t大小：%lu\n\t抽稀比例：%f \n", lNo, ptNum, lvsize, sacle);
				// 				preSize = lvsize;
				// 			}

				nLevelNo++;
				levelPre = pNextLevel;

			}
			stop_time = GetTickCount();
			printf("写入其它层数据耗时：%f 秒\n", (stop_time - start_time)/((float)1000));

			/*		fclose(pInfo);*/
			// 写本区域头文件
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

			// 根据点云重新更新最后一层的层级包围盒(按空间抽稀后层级过多时包围盒扩大)


			if (processCallback)
			{
				processCallback(0.0, HDSCENE_IDS_CONVERT_FINISH);
			}

		}
		else if (dMinX < m_zoneStartX)// 当前工程点云与上一3度带跨带
		{
			// 构建新的hlz写文件对象
			if (m_pNextHlzWrite)
			{
				delete m_pNextHlzWrite;
				m_pNextHlzWrite = NULL;
			}
			m_pNextHlzWrite = new CHLZWriter();
			m_pNextHlzWrite->m_header.isCompress = isCompress;
			// 点云外包围盒为点云工程真实包围盒
			m_fullExtent.MinEdge.X = dMinX;
			m_fullExtent.MinEdge.Y = dMinY;
			m_fullExtent.MinEdge.Z = dMinZ;
			m_fullExtent.MaxEdge.X = dMaxX;
			m_fullExtent.MaxEdge.Y = dMaxY;
			m_fullExtent.MaxEdge.Z = dMaxZ;

			// 对点云包围盒的最小角MinEdge重新计算对应3度带的坐标值
			double dCenter = m_dCenter;
			double dLongitudePre = 0.0;
			double dLatitudePre = 0.0;
			double dMinXPre = 0.0;
			double dMinYPre = 0.0;
			positionTran.TranslateGauss2Degree(m_fullExtent.MinEdge.X,m_fullExtent.MinEdge.Y,dCenter,dLongitudePre,dLatitudePre);
			positionTran.TranslateDegree2Gauss(dLongitudePre,dLatitudePre,dMinXPre,dMinYPre);

			// 投影代号记录，分辨是否是云存储
			m_pHlzWrite->m_header.iZoneID = (U8)(dCenter / 3 + 0.5);
			m_pNextHlzWrite->m_header.iZoneID = m_pHlzWrite->m_header.iZoneID - 1;


			// 当前带点云外包围盒为点云工程真实包围盒与分界线交点
			m_fullExtent.MinEdge.X = m_zoneStartX;
			m_fullExtent.MinEdge.Y = dMinY;
			m_fullExtent.MinEdge.Z = dMinZ;
			m_fullExtent.MaxEdge.X = dMaxX;
			m_fullExtent.MaxEdge.Y = dMaxY;
			m_fullExtent.MaxEdge.Z = dMaxZ;

			// 前一3度带点云外包围盒为点云工程真实包围盒与分界线交点
			m_fullExtentNext.MinEdge.X = dMinXPre;
			m_fullExtentNext.MinEdge.Y = dMinYPre;
			m_fullExtentNext.MinEdge.Z = dMinZ;
			m_fullExtentNext.MaxEdge.X = m_zoneEndX;
			m_fullExtentNext.MaxEdge.Y = dMaxY;
			m_fullExtentNext.MaxEdge.Z = dMaxZ;

			// 计算当前带点云外包围和的最小minEdge落在哪个块集，找到对应块集原点
			U32 nBlockSetNoX = (U32)floor((m_fullExtent.MinEdge.X -m_zoneStartX) / m_stepX);
			U32 nBlockSetNoY = (U32)floor((m_fullExtent.MinEdge.Y -m_zoneStartY) / m_stepY);
			m_blockSetStartX = nBlockSetNoX * m_stepX + m_zoneStartX;
			m_blockSetStartY = nBlockSetNoY * m_stepY + m_zoneStartY;

			// 文件头信息，为保证点云显示逻辑一致，将偏移量设置为点云外包围和最小点相对于网格起点的偏移量
			m_pHlzWrite->m_header.offsetX = m_blockSetStartX;
			m_pHlzWrite->m_header.offsetY = m_blockSetStartY;
			m_pHlzWrite->m_header.offsetZ = m_fullExtent.MinEdge.Z;

			// 根据划分到前一3度带的点云数据计算点云外包围盒的最小minEdge落在哪个块集，找到对应块集原点
			U32 nBlockSetNoXPre = (U32)floor((dMinXPre -m_zoneStartX) / m_stepX);
			U32 nBlockSetNoYPre = (U32)floor((dMinYPre -m_zoneStartY) / m_stepY);
			m_blockSetStartXNext = nBlockSetNoXPre * m_stepX + m_zoneStartX;
			m_blockSetStartYNext = nBlockSetNoYPre * m_stepY + m_zoneStartY;

			// 划分到前一3度带的hlz文件偏移量
			m_pNextHlzWrite->m_header.offsetX = m_blockSetStartXNext;
			m_pNextHlzWrite->m_header.offsetY = m_blockSetStartYNext;
			m_pNextHlzWrite->m_header.offsetZ = m_fullExtentNext.MinEdge.Z;

			// 当前区域点云相对坐标范围-范围起点默认为(0,0,dminZ)
			m_pHlzWrite->m_header.min_x = (F32)(0.0);
			m_pHlzWrite->m_header.min_y = (F32)(0.0);
			m_pHlzWrite->m_header.min_z = (F32)(dMinZ - m_pHlzWrite->m_header.offsetZ);
			m_pHlzWrite->m_header.max_x = (F32)(dMaxX - m_pHlzWrite->m_header.offsetX);
			m_pHlzWrite->m_header.max_y = (F32)(dMaxY - m_pHlzWrite->m_header.offsetY);
			m_pHlzWrite->m_header.max_z = (F32)(dMaxZ - m_pHlzWrite->m_header.offsetZ);

			// 前一3度带区域点云相对坐标范围
			m_pNextHlzWrite->m_header.min_x = (F32)(0.0);
			m_pNextHlzWrite->m_header.min_y = (F32)(0.0);
			m_pNextHlzWrite->m_header.min_z = (F32)(dMinZ - m_pNextHlzWrite->m_header.offsetZ);
			m_pNextHlzWrite->m_header.max_x = (F32)(m_zoneEndX - m_pNextHlzWrite->m_header.offsetX);
			m_pNextHlzWrite->m_header.max_y = (F32)(dMaxY - m_pNextHlzWrite->m_header.offsetY);
			m_pNextHlzWrite->m_header.max_z = (F32)(dMaxZ - m_pNextHlzWrite->m_header.offsetZ);

			// 记录更新时间
			time_t timer;
			time(&timer);
			tm* t_tm = localtime(&timer);
			m_pHlzWrite->m_header.file_creation_day = (U16)t_tm->tm_yday + 1;
			m_pHlzWrite->m_header.file_creation_year = (U16)t_tm->tm_year + 1900;
			m_pNextHlzWrite->m_header.file_creation_day = (U16)t_tm->tm_yday + 1;
			m_pNextHlzWrite->m_header.file_creation_year = (U16)t_tm->tm_year + 1900;

			// 打开文件
			if(!m_pHlzWrite->Open(savePath))
			{
				if (processCallback)
				{
					processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
				}

				return false;
			}
			// 打开文件
			if(!m_pNextHlzWrite->Open(savePathNext))
			{
				if (processCallback)
				{
					processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
				}

				return false;
			}
			// 根据点云的范围确定XY方向划分的块数,必须是2的N次方
			CHdLevel* pLevel = new CHdLevel;
			pLevel->m_levelNo = 0;
			m_pCoreData->AddLevelRec(pLevel);

			// 跟据工程点云的实际情况确定对应块的块集编号
			pLevel->m_level.numBlocksetX = (U16)ceil((m_fullExtent.MaxEdge.X - m_fullExtent.MinEdge.X) / m_stepX);
			pLevel->m_level.numBlocksetY = (U16)ceil((m_fullExtent.MaxEdge.Y - m_fullExtent.MinEdge.Y) / m_stepY);
			pLevel->m_level.sizeX = m_stepX;
			pLevel->m_level.sizeY = m_stepY;

			// 根据点云的范围确定XY方向划分的块数,必须是2的N次方
			CHdLevel* pLevelNext = new CHdLevel;
			pLevelNext->m_levelNo = 0;
			m_pCoreDataNext->AddLevelRec(pLevelNext);

			// 跟据工程点云的实际情况确定对应块的块集编号
			pLevelNext->m_level.numBlocksetX = (U16)ceil((m_fullExtentNext.MaxEdge.X - m_blockSetStartXNext) / m_stepX);
			pLevelNext->m_level.numBlocksetY = (U16)ceil((m_fullExtentNext.MaxEdge.Y - m_blockSetStartYNext) / m_stepY);
			pLevelNext->m_level.sizeX = m_stepX;
			pLevelNext->m_level.sizeY = m_stepY;

			double start_time = GetTickCount();

			// 根据规范规则,进行块集切割

			if (SplitInputMultiZoneFiles(m_savePath.c_str()) == false)
			{
				if (processCallback)
				{
					processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
				}

				RemoveTempFiles(m_savePath.c_str());
				return false;
			}

			// 将所有块集文件关闭
			CloseTempFiles();

			double stop_time = GetTickCount();

			printf("划分块集耗时：%f 秒\n", (stop_time - start_time)/((float)1000));

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
			writeLevel0DataNext(pLevelNext);
			stop_time = GetTickCount();
			printf("写入0层数据耗时：%f 秒\n", (stop_time - start_time)/((float)1000));
			// 总点数为第0层的点数
			m_pHlzWrite->m_header.number_of_point_records = pLevel->m_level.pointNum;	
			// 总点数为相邻区块第0层的点数
			m_pNextHlzWrite->m_header.number_of_point_records = pLevelNext->m_level.pointNum;
			
			//当前层的点数小于100w时不再分下一层
			U8 nLevelNo = 1;
			CHdLevel* levelPre = pLevel;
			// 开始计时
			start_time = GetTickCount();
			while (levelPre->m_level.pointNum > 1000000)
			{
				// 层的日志信息
				if(DEBUG_LOG)
				{	char strLv[512] = {0};
				sprintf_s(strLv,"level%d:\n",nLevelNo);
				m_pHlzWrite->WriteLogFile(strLv);
				m_pHlzWrite->WriteLogFile("类型,点数,子块数,范围\n");
				}
				// 下一层
				CHdLevel* pNextLevel = new CHdLevel;
				pNextLevel->m_levelNo = nLevelNo;
				m_pCoreData->AddLevelRec(pNextLevel);
				// 写入下一层数据
				writeNextLevelData(levelPre, nLevelNo, pNextLevel);

				nLevelNo++;
				levelPre = pNextLevel;
			}
			//当前层的点数小于100w时不再分下一层（非本带区）
			U8 nLevelNoNextZone = 1;
			CHdLevel* levelPreNextZone = pLevelNext;
			while (levelPreNextZone->m_level.pointNum > 1000000)
			{
				// 层的日志信息
				if(DEBUG_LOG)
				{	char strLv[512] = {0};
				sprintf_s(strLv,"level%d:\n",nLevelNoNextZone);
				m_pHlzWrite->WriteLogFile(strLv);
				m_pHlzWrite->WriteLogFile("类型,点数,子块数,范围\n");
				}
				// 下一层
				CHdLevel* pNextLevel = new CHdLevel;
				pNextLevel->m_levelNo = nLevelNoNextZone;
				m_pCoreDataNext->AddLevelRec(pNextLevel);
				// 写入下一层数据
				writeNextLevelDataNextZone(levelPreNextZone, nLevelNoNextZone, pNextLevel);

				nLevelNoNextZone++;
				levelPreNextZone = pNextLevel;
			}
			stop_time = GetTickCount();
			printf("写入其它层数据耗时：%f 秒\n", (stop_time - start_time)/((float)1000));

			// 更新头文件
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

			// 写相临区域头文件
			m_pNextHlzWrite->m_header.renderMinX = m_pCoreDataNext->m_hlzHeader.renderMinX;
			m_pNextHlzWrite->m_header.renderMinY = m_pCoreDataNext->m_hlzHeader.renderMinY;
			m_pNextHlzWrite->m_header.renderMinZ = m_pCoreDataNext->m_hlzHeader.renderMinZ;
			m_pNextHlzWrite->m_header.renderMaxX = m_pCoreDataNext->m_hlzHeader.renderMaxX;
			m_pNextHlzWrite->m_header.renderMaxY = m_pCoreDataNext->m_hlzHeader.renderMaxY;
			m_pNextHlzWrite->m_header.renderMaxZ = m_pCoreDataNext->m_hlzHeader.renderMaxZ;
			m_pNextHlzWrite->m_header.intensityMin = m_pCoreDataNext->m_hlzHeader.intensityMin;
			m_pNextHlzWrite->m_header.intensityMax = m_pCoreDataNext->m_hlzHeader.intensityMax;
			m_pNextHlzWrite->m_header.number_of_level = nLevelNoNextZone;
			m_pNextHlzWrite->WriteHeader();
			m_pCoreDataNext->m_hlzHeader = m_pNextHlzWrite->m_header;
			m_pCoreDataNext->UpdateScale();
			m_pNextHlzWrite->WriteIndex(m_pCoreDataNext->GetListLevel());
			// 写入索引文件并关闭数据文件
			m_pNextHlzWrite->Close();

			// 删除临时文件
			RemoveTempFiles(m_savePath.c_str());

			if (processCallback)
			{
				processCallback(0.0, HDSCENE_IDS_CONVERT_FINISH);
			}

		}
		else if (dMinX > m_zoneStartX  && dMaxX > m_zoneEndX)// 当前工程点云与下一3度带跨带
		{
			// 构建新的hlz写文件对象
			if (m_pNextHlzWrite)
			{
				delete m_pNextHlzWrite;
				m_pNextHlzWrite = NULL;
			}
			m_pNextHlzWrite = new CHLZWriter();
			m_pNextHlzWrite->m_header.isCompress = isCompress;
			// 点云外包围盒为点云工程真实包围盒
			m_fullExtent.MinEdge.X = dMinX;
			m_fullExtent.MinEdge.Y = dMinY;
			m_fullExtent.MinEdge.Z = dMinZ;
			m_fullExtent.MaxEdge.X = dMaxX;
			m_fullExtent.MaxEdge.Y = dMaxY;
			m_fullExtent.MaxEdge.Z = dMaxZ;

			// 对点云包围盒的最大角MinEdge重新计算对应3度带的坐标值
			double dCenter = m_dCenter;
			double dLongitudePre = 0.0;
			double dLatitudePre = 0.0;
			double dMaxXNext = 0.0;
			double dMaxYNext = 0.0;
			positionTran.TranslateGauss2Degree(m_fullExtent.MaxEdge.X,m_fullExtent.MaxEdge.Y,dCenter,dLongitudePre,dLatitudePre);
			positionTran.TranslateDegree2Gauss(dLongitudePre,dLatitudePre,dMaxXNext,dMaxYNext);

			// 投影代号记录，分辨是否是云存储
			m_pHlzWrite->m_header.iZoneID = (U8)(dCenter / 3 + 0.5);
			m_pNextHlzWrite->m_header.iZoneID = m_pHlzWrite->m_header.iZoneID + 1;

			// 当前带点云外包围盒为点云工程真实包围盒与分界线交点
			m_fullExtent.MinEdge.X = dMinX;
			m_fullExtent.MinEdge.Y = dMinY;
			m_fullExtent.MinEdge.Z = dMinZ;
			m_fullExtent.MaxEdge.X = m_zoneEndX;
			m_fullExtent.MaxEdge.Y = dMaxY;
			m_fullExtent.MaxEdge.Z = dMaxZ;

			// 下一3度带点云外包围盒为点云工程真实包围盒与分界线交点
			m_fullExtentNext.MinEdge.X = m_zoneStartX;
			m_fullExtentNext.MinEdge.Y = dMinY;
			m_fullExtentNext.MinEdge.Z = dMinZ;
			m_fullExtentNext.MaxEdge.X = dMaxXNext;
			m_fullExtentNext.MaxEdge.Y = dMaxYNext;
			m_fullExtentNext.MaxEdge.Z = dMaxZ;

			// 计算当前带点云外包围和的最小minEdge落在哪个块集，找到对应块集原点
			U32 nBlockSetNoX = (U32)floor((m_fullExtent.MinEdge.X -m_zoneStartX) / m_stepX);
			U32 nBlockSetNoY = (U32)floor((m_fullExtent.MinEdge.Y -m_zoneStartY) / m_stepY);
			m_blockSetStartX = nBlockSetNoX * m_stepX + m_zoneStartX;
			m_blockSetStartY = nBlockSetNoY * m_stepY + m_zoneStartY;

			// 文件头信息，为保证点云显示逻辑一致，将偏移量设置为点云外包围和最小点相对于网格起点的偏移量
			m_pHlzWrite->m_header.offsetX = m_blockSetStartX;
			m_pHlzWrite->m_header.offsetY = m_blockSetStartY;
			m_pHlzWrite->m_header.offsetZ = m_fullExtent.MinEdge.Z;

			// 根据划分到下一3度带的点云数据计算点云外包围盒的最小minEdge落在哪个块集，找到对应块集原点
			U32 nBlockSetNoXPre = (U32)floor((m_fullExtentNext.MinEdge.X -m_zoneStartX) / m_stepX);
			U32 nBlockSetNoYPre = (U32)floor((m_fullExtentNext.MinEdge.Y -m_zoneStartY) / m_stepY);
			m_blockSetStartXNext = nBlockSetNoXPre * m_stepX + m_zoneStartX;
			m_blockSetStartYNext = nBlockSetNoYPre * m_stepY + m_zoneStartY;

			// 划分到下一3度带的hlz文件偏移量
			m_pNextHlzWrite->m_header.offsetX = m_blockSetStartXNext;
			m_pNextHlzWrite->m_header.offsetY = m_blockSetStartYNext;
			m_pNextHlzWrite->m_header.offsetZ = m_fullExtentNext.MinEdge.Z;

			// 当前区域点云相对坐标范围-范围起点默认为(0,0,dminZ)
			m_pHlzWrite->m_header.min_x = (F32)(0.0);
			m_pHlzWrite->m_header.min_y = (F32)(0.0);
			m_pHlzWrite->m_header.min_z = (F32)(dMinZ - m_pHlzWrite->m_header.offsetZ);
			m_pHlzWrite->m_header.max_x = (F32)(m_zoneEndX - m_pHlzWrite->m_header.offsetX);
			m_pHlzWrite->m_header.max_y = (F32)(dMaxY - m_pHlzWrite->m_header.offsetY);
			m_pHlzWrite->m_header.max_z = (F32)(dMaxZ - m_pHlzWrite->m_header.offsetZ);

			// 前一3度带区域点云相对坐标范围
			m_pNextHlzWrite->m_header.min_x = (F32)(0.0);
			m_pNextHlzWrite->m_header.min_y = (F32)(0.0);
			m_pNextHlzWrite->m_header.min_z = (F32)(dMinZ - m_pNextHlzWrite->m_header.offsetZ);
			m_pNextHlzWrite->m_header.max_x = (F32)(dMaxXNext - m_pNextHlzWrite->m_header.offsetX);
			m_pNextHlzWrite->m_header.max_y = (F32)(dMaxYNext - m_pNextHlzWrite->m_header.offsetY);
			m_pNextHlzWrite->m_header.max_z = (F32)(dMaxZ - m_pNextHlzWrite->m_header.offsetZ);

			// 记录更新时间
			time_t timer;
			time(&timer);
			tm* t_tm = localtime(&timer);
			m_pHlzWrite->m_header.file_creation_day = (U16)t_tm->tm_yday + 1;
			m_pHlzWrite->m_header.file_creation_year = (U16)t_tm->tm_year + 1900;
			m_pNextHlzWrite->m_header.file_creation_day = (U16)t_tm->tm_yday + 1;
			m_pNextHlzWrite->m_header.file_creation_year = (U16)t_tm->tm_year + 1900;

			// 打开文件
			if(!m_pHlzWrite->Open(savePath))
			{
				if (processCallback)
				{
					processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
				}

				return false;
			}
			// 打开文件
			if(!m_pNextHlzWrite->Open(savePathNext))
			{
				if (processCallback)
				{
					processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
				}

				return false;
			}
			// 根据点云的范围确定XY方向划分的块数,必须是2的N次方
			CHdLevel* pLevel = new CHdLevel;
			pLevel->m_levelNo = 0;
			m_pCoreData->AddLevelRec(pLevel);

			// 跟据工程点云的实际情况确定对应块的块集编号
			pLevel->m_level.numBlocksetX = (U16)ceil((m_fullExtent.MaxEdge.X - m_fullExtent.MinEdge.X) / m_stepX);
			pLevel->m_level.numBlocksetY = (U16)ceil((m_fullExtent.MaxEdge.Y - m_fullExtent.MinEdge.Y) / m_stepY);
			pLevel->m_level.sizeX = m_stepX;
			pLevel->m_level.sizeY = m_stepY;

			// 根据点云的范围确定XY方向划分的块数,必须是2的N次方
			CHdLevel* pLevelNext = new CHdLevel;
			pLevelNext->m_levelNo = 0;
			m_pCoreDataNext->AddLevelRec(pLevelNext);

			// 跟据工程点云的实际情况确定对应块的块集编号
			pLevelNext->m_level.numBlocksetX = (U16)ceil((m_fullExtentNext.MaxEdge.X - m_blockSetStartXNext) / m_stepX);
			pLevelNext->m_level.numBlocksetY = (U16)ceil((m_fullExtentNext.MaxEdge.Y - m_blockSetStartYNext) / m_stepY);
			pLevelNext->m_level.sizeX = m_stepX;
			pLevelNext->m_level.sizeY = m_stepY;

			double start_time = GetTickCount();

			// 根据规范规则,进行块集切割

			if (SplitInputMultiZoneFiles(m_savePath.c_str()) == false)
			{
				if (processCallback)
				{
					processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
				}

				RemoveTempFiles(m_savePath.c_str());
				return false;
			}

			// 将所有块集文件关闭
			CloseTempFiles();

			double stop_time = GetTickCount();

			printf("划分块集耗时：%f 秒\n", (stop_time - start_time)/((float)1000));

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
			writeLevel0DataNext(pLevelNext);
			stop_time = GetTickCount();
			printf("写入0层数据耗时：%f 秒\n", (stop_time - start_time)/((float)1000));
			// 总点数为第0层的点数
			m_pHlzWrite->m_header.number_of_point_records = pLevel->m_level.pointNum;	
			// 总点数为相邻区块第0层的点数
			m_pNextHlzWrite->m_header.number_of_point_records = pLevelNext->m_level.pointNum;
			
			//当前层的点数小于100w时不再分下一层
			U8 nLevelNo = 1;
			CHdLevel* levelPre = pLevel;
			// 开始计时
			start_time = GetTickCount();
			while (levelPre->m_level.pointNum > 1000000)
			{
				// 层的日志信息
				if(DEBUG_LOG)
				{	char strLv[512] = {0};
				sprintf_s(strLv,"level%d:\n",nLevelNo);
				m_pHlzWrite->WriteLogFile(strLv);
				m_pHlzWrite->WriteLogFile("类型,点数,子块数,范围\n");
				}
				// 下一层
				CHdLevel* pNextLevel = new CHdLevel;
				pNextLevel->m_levelNo = nLevelNo;
				m_pCoreData->AddLevelRec(pNextLevel);
				// 写入下一层数据
				writeNextLevelData(levelPre, nLevelNo, pNextLevel);

				nLevelNo++;
				levelPre = pNextLevel;
			}
			//当前层的点数小于100w时不再分下一层（非本带区）
			U8 nLevelNoNextZone = 1;
			CHdLevel* levelPreNextZone = pLevelNext;
			while (levelPreNextZone->m_level.pointNum > 1000000)
			{
				// 层的日志信息
				if(DEBUG_LOG)
				{	char strLv[512] = {0};
				sprintf_s(strLv,"level%d:\n",nLevelNoNextZone);
				m_pHlzWrite->WriteLogFile(strLv);
				m_pHlzWrite->WriteLogFile("类型,点数,子块数,范围\n");
				}
				// 下一层
				CHdLevel* pNextLevel = new CHdLevel;
				pNextLevel->m_levelNo = nLevelNoNextZone;
				m_pCoreDataNext->AddLevelRec(pNextLevel);
				// 写入下一层数据
				writeNextLevelDataNextZone(levelPreNextZone, nLevelNoNextZone, pNextLevel);

				nLevelNoNextZone++;
				levelPreNextZone = pNextLevel;
			}
			stop_time = GetTickCount();
			printf("写入其它层数据耗时：%f 秒\n", (stop_time - start_time)/((float)1000));

			// 更新头文件
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

			// 写相临区域头文件
			m_pNextHlzWrite->m_header.renderMinX = m_pCoreDataNext->m_hlzHeader.renderMinX;
			m_pNextHlzWrite->m_header.renderMinY = m_pCoreDataNext->m_hlzHeader.renderMinY;
			m_pNextHlzWrite->m_header.renderMinZ = m_pCoreDataNext->m_hlzHeader.renderMinZ;
			m_pNextHlzWrite->m_header.renderMaxX = m_pCoreDataNext->m_hlzHeader.renderMaxX;
			m_pNextHlzWrite->m_header.renderMaxY = m_pCoreDataNext->m_hlzHeader.renderMaxY;
			m_pNextHlzWrite->m_header.renderMaxZ = m_pCoreDataNext->m_hlzHeader.renderMaxZ;
			m_pNextHlzWrite->m_header.intensityMin = m_pCoreDataNext->m_hlzHeader.intensityMin;
			m_pNextHlzWrite->m_header.intensityMax = m_pCoreDataNext->m_hlzHeader.intensityMax;
			m_pNextHlzWrite->m_header.number_of_level = nLevelNoNextZone;
			m_pNextHlzWrite->WriteHeader();
			m_pCoreDataNext->m_hlzHeader = m_pNextHlzWrite->m_header;
			m_pCoreDataNext->UpdateScale();
			m_pNextHlzWrite->WriteIndex(m_pCoreDataNext->GetListLevel());
			// 写入索引文件并关闭数据文件
			m_pNextHlzWrite->Close();

			// 删除临时文件
			RemoveTempFiles(m_savePath.c_str());

			if (processCallback)
			{
				processCallback(0.0, HDSCENE_IDS_CONVERT_FINISH);
			}

		}

		//// 打开文件
		//if(!m_pHlzWrite->Open(savePath))
		//{
		//	if (processCallback)
		//	{
		//		processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
		//	}

		//	return false;
		//}

		//// 根据点云的范围确定XY方向划分的块数,必须是2的N次方
		//CHdLevel* pLevel = new CHdLevel;
		//pLevel->m_levelNo = 0;
		//m_pCoreData->AddLevelRec(pLevel);

		//// 跟据工程点云的实际情况确定对应块的块集编号
		//pLevel->m_level.numBlocksetX = (U16)ceil((m_fullExtent.MaxEdge.X - m_blockSetStartX) / m_stepX);//(U32)pow(2.f,baseNum);
		//pLevel->m_level.numBlocksetY = (U16)ceil((m_fullExtent.MaxEdge.Y - m_blockSetStartY) / m_stepY);//(U32)pow(2.f,baseNum);
		//pLevel->m_level.sizeX = m_stepX;
		//pLevel->m_level.sizeY = m_stepY;

//		double start_time = GetTickCount();
//
//	// 根据规范规则,进行块集切割
//		
//		if (SplitInputFiles(m_savePath.c_str()) == false)
//		{
//			if (processCallback)
//			{
//				processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
//			}
//
//			RemoveTempFiles(m_savePath.c_str());
//			return false;
//		}
//
//		// 将所有块集文件关闭
//		CloseTempFiles();
//
//		double stop_time = GetTickCount();
//
//		printf("划分块集耗时：%f 秒\n", (stop_time - start_time)/((float)1000));
//
//		// 第0层日志信息
//		if(DEBUG_LOG)
//		{
//			char strLv[512];
//			sprintf_s(strLv,"level%d:\n",0);
//			m_pHlzWrite->WriteLogFile(strLv);
//			m_pHlzWrite->WriteLogFile("类型,点数,范围\n");
//		}
//		// 写入第0层
//		start_time = GetTickCount();
//		writeLevel0Data(pLevel);
//		stop_time = GetTickCount();
//		printf("写入0层数据耗时：%f 秒\n", (stop_time - start_time)/((float)1000));
//		// 总点数为第0层的点数
//		m_pHlzWrite->m_header.number_of_point_records = pLevel->m_level.pointNum;	
//
//		// 统计并输出信息
//// 		string strSavePath = savePath; 
//// 		string strInfo = strSavePath.substr(0,strSavePath.length()-4);
//// 		strInfo += "_Info.txt";
//// 		FILE* pInfo = fopen(strInfo.c_str(),"wt");
//// 		U32 preSize;
//// 		if (pInfo != NULL)
//// 		{
//// 			U32 ptNum = pLevel->m_level.pointNum;
//// 			U16 lNo = pLevel->m_levelNo;
//// 			U32 lvsize = ptNum * (sizeof(HdPointXYZ) + sizeof(U8));
//// 			preSize = lvsize;
//// 			fprintf(pInfo, "第%u层信息：\n\t点数：%lu\n\t大小：%lu\n", lNo, ptNum, lvsize);
//// 		}
//
//		//当前层的点数小于100w时不再分下一层
//		U8 nLevelNo = 1;
//		CHdLevel* levelPre = pLevel;
//		//HdLevel levelNext;
//		start_time = GetTickCount();
//		while (levelPre->m_level.pointNum > 1000000)
//		{
//			// 层的日志信息
//			if(DEBUG_LOG)
//			{	char strLv[512] = {0};
//			sprintf_s(strLv,"level%d:\n",nLevelNo);
//			m_pHlzWrite->WriteLogFile(strLv);
//			m_pHlzWrite->WriteLogFile("类型,点数,子块数,范围\n");
//			}
//			// 下一层
//			CHdLevel* pNextLevel = new CHdLevel;
//			pNextLevel->m_levelNo = nLevelNo;
//			m_pCoreData->AddLevelRec(pNextLevel);
//			writeNextLevelData(levelPre, nLevelNo, pNextLevel);
//// 			if (pInfo != NULL)
//// 			{
//// 				U32 ptNum = pNextLevel->m_level.pointNum;
//// 				U16 lNo = pNextLevel->m_levelNo;
//// 				U32 lvsize = ptNum * (sizeof(HdPointXYZ) + sizeof(U8));
//// 				float sacle = (float)((float)preSize/(float)lvsize);
//// 				fprintf(pInfo, "第%u层信息：\n\t点数：%lu\n\t大小：%lu\n\t抽稀比例：%f \n", lNo, ptNum, lvsize, sacle);
//// 				preSize = lvsize;
//// 			}
//
//			nLevelNo++;
//			levelPre = pNextLevel;
//
//		}
//		stop_time = GetTickCount();
//		printf("写入其它层数据耗时：%f 秒\n", (stop_time - start_time)/((float)1000));
//
///*		fclose(pInfo);*/
//
//		m_pHlzWrite->m_header.renderMinX = m_pCoreData->m_hlzHeader.renderMinX;
//		m_pHlzWrite->m_header.renderMinY = m_pCoreData->m_hlzHeader.renderMinY;
//		m_pHlzWrite->m_header.renderMinZ = m_pCoreData->m_hlzHeader.renderMinZ;
//		m_pHlzWrite->m_header.renderMaxX = m_pCoreData->m_hlzHeader.renderMaxX;
//		m_pHlzWrite->m_header.renderMaxY = m_pCoreData->m_hlzHeader.renderMaxY;
//		m_pHlzWrite->m_header.renderMaxZ = m_pCoreData->m_hlzHeader.renderMaxZ;
//		m_pHlzWrite->m_header.intensityMin = m_pCoreData->m_hlzHeader.intensityMin;
//		m_pHlzWrite->m_header.intensityMax = m_pCoreData->m_hlzHeader.intensityMax;
//		m_pHlzWrite->m_header.number_of_level = nLevelNo;
//		m_pHlzWrite->WriteHeader();
//		m_pCoreData->m_hlzHeader = m_pHlzWrite->m_header;
//		m_pCoreData->UpdateScale();
//		m_pHlzWrite->WriteIndex(m_pCoreData->GetListLevel());
//		// 写入索引文件并关闭数据文件
//		m_pHlzWrite->Close();
//
//
//
//		// 删除临时文件
//		RemoveTempFiles(m_savePath.c_str());
//
//		if (processCallback)
//		{
//			processCallback(0.0, HDSCENE_IDS_CONVERT_FINISH);
//		}
		return true;
	}

	CHdBox3df CHlzCloudBuilder::GetBlockListBox(std::vector<BlockSetFileInfo>& bsFileList)
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
	// 写当前带的下一层点云
	U64 CHlzCloudBuilder::writeNextLevelData(CHdLevel* pPreLevel, U8 nLevelNo, CHdLevel* pCurLevel)
	{
		pCurLevel->m_level.numBlocksetX = (U16)ceil((pPreLevel->m_level.numBlocksetX) / 2.0);
		pCurLevel->m_level.numBlocksetY = (U16)ceil((pPreLevel->m_level.numBlocksetY) / 2.0);
		//pCurLevel->m_level.numBlockset = pCurLevel->m_level.numBlocksetX * pCurLevel->m_level.numBlocksetY;
		pCurLevel->m_level.sizeX = pPreLevel->m_level.sizeX * 2.f;
		pCurLevel->m_level.sizeY = pPreLevel->m_level.sizeY * 2.f;
		m_stepX = pCurLevel->m_level.sizeX;
		m_stepY = pCurLevel->m_level.sizeY;
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
		// 计算点云范围包围盒的起点所在上层块集作为当前块集的起点号
		//U32 nBlockNoX = (U32)(m_fullExtent.MinEdge.X - m_zoneStartX) / pCurLevel->m_level.sizeX - 2;
		//U32 nBlockNoY = (U32)(m_fullExtent.MinEdge.Y - m_zoneStartY) / pCurLevel->m_level.sizeY - 2;
		for (int iY = 0 ; iY < pCurLevel->m_level.numBlocksetY; iY++)
		{
			for (int jX = 0; jX < pCurLevel->m_level.numBlocksetX; jX++)
			{			
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
				//m_pHlzWrite->WriteNextLevelBlockset(BsFileName, preBlocksetFiles, curBlockset->m_blockSet,pCurLevel,curBlockset->m_nBlockSetNo);
				m_pHlzWrite->WriteNextLevelBlocksetByMidCloud(BsFileName, preBlocksetFiles, curBlockset->m_blockSet,pCurLevel,curBlockset->m_nBlockSetNo,nLevelNo);
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
				F32 BlockSetOffsetX = curBlockset->m_blockSet.box.MinEdge.X;
				F32 BlockSetOffsetY = curBlockset->m_blockSet.box.MinEdge.Y;
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

				/***********************第一种情况*************************/
				if (curBlockset->m_blockSet.numPoint <= BLOCK_HOLD)
				{
					// 不需要向下分块
					curBlockset->m_blockSet.hasSubBlock = 0;

// 					FILE* pBsFile = fopen(BsFileName,"rb");			
// 					if (pBsFile == NULL)
// 					{
// 						continue;
// 					}
					HANDLE pBsFile = CreateFile(BsFileName,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
					if (pBsFile == INVALID_HANDLE_VALUE)
					{
						continue;
					}

					// 读取到内存
					// 存储点信息
					std::vector<PointXYZIPRGBA> vecBuf;
					try
					{
						vecBuf.resize(curBlockset->m_blockSet.numPoint);
					}
					catch(...)
					{
						::MessageBox(NULL,"内存分配出错，请提高内存配置或采用\"临时文件方式\"", NULL, MB_OK);
						// 删除临时文件
						RemoveTempFiles(m_savePath.c_str());
						return 0;
					}
//					fseek(pBsFile, 0, SEEK_SET);
//					fread(vecBuf._Myfirst, sizeof(PointXYZIPRGBA), curBlockset->m_blockSet.numPoint, pBsFile);
					LARGE_INTEGER li1, li2;
					li1.HighPart = 0;
					li1.LowPart = 0;
					li2.HighPart = 0;
					li2.LowPart = 0;
					SetFilePointerEx(pBsFile, li1, &li2, FILE_BEGIN);
//					SetFilePointer(pBsFile, 0, 0, FILE_BEGIN);
					DWORD numRead;
					ReadFile(pBsFile, vecBuf._Myfirst(), sizeof(PointXYZIPRGBA)*(curBlockset->m_blockSet.numPoint), &numRead, NULL);
					// 关闭块集文件
					if (pBsFile)
					{
						CloseHandle(pBsFile);
						pBsFile = NULL;
					}

					
					HdPointXYZ* pPtXYZ = NULL;
					U8* pPtInten = NULL;
					HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
					if (!m_pHlzWrite->m_header.isCompress)
					{
						// 不压缩，直接写入原始坐标和强度
						// 内存点云拆分为坐标和强度
						//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
						U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), curBlockset->m_blockSet.numPoint, &refPtBS, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
						curBlockset->m_blockSet.numPoint = cntWrite;

						m_pHlzWrite->WriteBlockSetData(curBlockset->m_blockSet, pPtXYZ, pPtInten, pPtColor, curBlockset->m_blockSet.numPoint);
					}
					else
					{
						// 压缩坐标，并写入
						//重排序
						std::sort(vecBuf.begin(), vecBuf.end(), lessByXYZ);
						// 内存点云拆分为坐标和强度
						//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
						U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), curBlockset->m_blockSet.numPoint, &refPtBS, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
						curBlockset->m_blockSet.numPoint = cntWrite;

						CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
						ptXYZEncoder.encodePoints();

						m_pHlzWrite->WriteBlockSetData(curBlockset->m_blockSet, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, curBlockset->m_blockSet.numPoint);
						// 回收内存
						ptXYZEncoder.clear();
					}
					// 写入日志文件
					//char logmsg[512] = {0};
					//sprintf_s(logmsg,"块集,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n", 
					//	curBlockset->m_blockSet.numPoint,
					//	curBlockset->m_blockSet.box.MinEdge.X, curBlockset->m_blockSet.box.MinEdge.Y, curBlockset->m_blockSet.box.MinEdge.Z, 
					//	curBlockset->m_blockSet.box.MaxEdge.X, curBlockset->m_blockSet.box.MaxEdge.Y,curBlockset->m_blockSet.box.MaxEdge.Z);
					//m_pHlzWrite->WriteLogFile(logmsg);

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

				/***********************第二种情况*************************/
				else if (curBlockset->m_blockSet.numPoint > BLOCK_HOLD && curBlockset->m_blockSet.numPoint <= MAX_POINT_NUM)
				{// else分块写入
					// else分块写入
					// 该块集对应的块文件夹
					std::string blocksPath = BsFileName;
					blocksPath = blocksPath.substr(0, blocksPath.find_last_of('.'));

					// 采用二分的思想,对块集点云继续分割为块文件	
					std::vector<BlockFileInfo> arrayBlock;
					SplitBlockset2Block_Buffer(BsInfo,arrayBlock);

					curBlockset->m_blockSet.hasSubBlock = 1;
					curBlockset->m_blockSet.numPoint = 0; // 重新置0,因为内部会自动统计

					// 只写索引文件
					m_pHlzWrite->WriteBlockSetData(curBlockset->m_blockSet, NULL, NULL, NULL, 0);

					CHdBlock* curBlock = NULL;

					for (U32 n=0; n < arrayBlock.size(); n++)
					{
						BlockFileInfo& blockFileInfo = *(arrayBlock._Myfirst() + n);
						std:: map<string, vector<PointXYZIPRGBA *> > :: iterator iterator_block = m_pBlockData.find(blockFileInfo.path.c_str());
						if (iterator_block == m_pBlockData.end())
						{
							continue;
						}
						U32 count =  iterator_block->second.size();

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

							// 当前块文件不需要再切分

							HdPointXYZ* pPtXYZ = NULL;
							U8* pPtInten = NULL;
							HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
							// 写入坐标和强度
						//	m_pHlzWrite->WriteBlockData(curBlock->m_block, pPtXYZ, pPtInten, curBlockset->m_blockSet.numPoint);
							if (!m_pHlzWrite->m_header.isCompress)
							{
								// 不压缩，写入坐标和强度
								// 内存点云拆分为坐标和强度,写入块内点云
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = pPtArray2PtBlock(iterator_block->second._Myfirst(), count, &bkRefPt, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
								curBlockset->m_blockSet.numPoint = cntWrite;

								m_pHlzWrite->WriteBlockData(curBlock->m_block, pPtXYZ, pPtInten, pPtColor, curBlock->m_block.numPoint);
							}
							else
							{
								// 压缩，写入压缩后的数据
								// 重排序
								std::sort(iterator_block->second.begin(), iterator_block->second.end(), pLessByXYZ);
								// 内存点云拆分为坐标和强度,写入块内点云
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = pPtArray2PtBlock(iterator_block->second._Myfirst(), count, &bkRefPt, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
								curBlockset->m_blockSet.numPoint = cntWrite;

								CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
								ptXYZEncoder.encodePoints();

								m_pHlzWrite->WriteBlockData(curBlock->m_block, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, curBlock->m_block.numPoint);
								// 回收内存
								ptXYZEncoder.clear();
							}
							// 写入日志文件
							//char logmsg[512] = {0};
							//sprintf_s(logmsg,"块,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n", 
							//	curBlock->m_block.numPoint,
							//	curBlock->m_block.box.MinEdge.X, curBlock->m_block.box.MinEdge.Y, 
							//	curBlock->m_block.box.MinEdge.Z, curBlock->m_block.box.MaxEdge.X, 
							//	curBlock->m_block.box.MaxEdge.Y, curBlock->m_block.box.MaxEdge.Z);
							//m_pHlzWrite->WriteLogFile(logmsg);

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
							std::string parcelsPath = blockFileInfo.path.c_str(); //sBlockPath.c_str();
							parcelsPath = parcelsPath.substr(0, parcelsPath.find_last_of('.'));

							// 将块切分为包
							std::vector<ParcelFileInfo> splitParcels;
							SplitBlock2Parcel_Buffer(curBlock->m_block.box,blockFileInfo.path.c_str(),parcelsPath.c_str(),splitParcels);

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
								hd::stringc& sParcelPath = (*it).path;
								std:: map<string, vector<PointXYZIPRGBA*> >::iterator iter_parcel = m_ParcelData.find(sParcelPath.c_str());

								// 内存点云拆分为坐标和强度,写入包内点云
								HdPointXYZ* pPtXYZ = NULL;
								U8* pPtInten = NULL;
								HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
								// 写入坐标和强度
							//	m_pHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, parcel->m_parcel.numPoint);
								if (!m_pHlzWrite->m_header.isCompress)
								{
									// 不压缩，写入坐标和强度
									//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
									U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst() , parcel->m_parcel.numPoint, &pclRefPt, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
									parcel->m_parcel.numPoint = cntWrite;

									m_pHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, pPtColor, parcel->m_parcel.numPoint);
								}
								else
								{
									// 压缩，写入压缩后的数据
									// 重排序
									std::sort(iter_parcel->second.begin(), iter_parcel->second.end(), pLessByXYZ);
									//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
									U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst() , parcel->m_parcel.numPoint, &pclRefPt, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
									parcel->m_parcel.numPoint = cntWrite;

									CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
									ptXYZEncoder.encodePoints();
									m_pHlzWrite->WriteParcelData(parcel->m_parcel, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, parcel->m_parcel.numPoint);
									// 回收内存
									ptXYZEncoder.clear();
								}
								curBlock->AddParcel(parcel);

								// 写入日志文件
								//char logmsg[512] = {0};
								//sprintf_s(logmsg,"包,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n", 
								//	parcel->m_parcel.numPoint,
								//	parcel->m_parcel.box.MinEdge.X, parcel->m_parcel.box.MinEdge.Y, 
								//	parcel->m_parcel.box.MinEdge.Z, parcel->m_parcel.box.MaxEdge.X, 
								//	parcel->m_parcel.box.MaxEdge.Y,parcel->m_parcel.box.MaxEdge.Z);
								//m_pHlzWrite->WriteLogFile(logmsg);

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

								// 将包文件删除，节省内存空间
								m_ParcelData.erase(sParcelPath.c_str());

							}//for (std::vector<ParcelFileInfo>::iterator it = splitParcels.begin();	
							curBlock->Update();
						}

						clearParcelData();     //清除包文件

					}
					curBlockset->Update();
					clearBlocksetData();
				}// else if (BLOCK_HOLD<curBlockset->m_blockSet.numPoint <= MAX_POINT_NUM)
				
				// 大于限定值，切分子包写入
				/***********************第三种情况*************************/
				else if (curBlockset->m_blockSet.numPoint > MAX_POINT_NUM)
				{

					// else分块写入
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
					//块文件
					for (U32 n=0; n < arrayBlock.size(); n++)
					{
						BlockFileInfo& blockFileInfo = *(arrayBlock._Myfirst() + n);
// 						FILE* pBlockFile = fopen(blockFileInfo.path.c_str(), "rb");
// 						if (pBlockFile == NULL)
// 						{
// 							continue;
// 						}
// 
// 						fseek(pBlockFile,0,SEEK_END);
// 						U32 fileSize = ftell(pBlockFile);
// 						U32 count = fileSize / (sizeof(PointXYZIPRGBA));
						HANDLE pBlockFile = CreateFile(blockFileInfo.path.c_str(),
							GENERIC_READ,
							FILE_SHARE_READ,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
						if (pBlockFile == INVALID_HANDLE_VALUE)
						{
							continue;
						}
						LARGE_INTEGER li1,li2;
						li1.HighPart = 0;
						li1.LowPart = 0;
						li2.HighPart = 0;
						li2.LowPart = 0;
						SetFilePointerEx(pBlockFile, li1, &li2, FILE_END);
						U64 fileSize = ((U64)li2.HighPart<<32)|li2.LowPart;
						U64 count = fileSize / (sizeof(PointXYZIPRGBA));
						LARGE_INTEGER li3;
						li3.HighPart = 0;
						li3.LowPart = 0;
						SetFilePointerEx(pBlockFile, li1, &li3, FILE_BEGIN);

						// 当前块对象
						curBlock = new CHdBlock;
						curBlock->m_bsNo = curBlockset->m_nBlockSetNo;
						curBlock->m_nBlockNo = blockFileInfo.index;
						curBlockset->AddBlockRec(curBlock);
						// 当前块对象的空间范围
						getBlockBox(centerBS,halfSzBS,curBlock->m_block.box,curBlock->m_nBlockNo);
						HdRefPoint refPtBK;
						refPtBK.FromBox(curBlock->m_block.box);
						CHdVector3df centerBK = curBlock->m_block.box.getCenter();
						CHdVector3df halfSzBK = curBlock->m_block.box.getExtent() / 2.0f;
						if (count <= BLOCK_HOLD)
						{
							//块文件无需切分，直接写入
							curBlock->m_block.hasSubParcel = 0;
							curBlock->m_block.numPoint = count;

							// 当前块文件不需要再切分
							vector<PointXYZIPRGBA> vecBuf;
							vecBuf.resize(count);
// 							fseek(pBlockFile, 0, SEEK_SET);
// 							fread(vecBuf._Myfirst,sizeof(PointXYZIPRGBA),count,pBlockFile);
							// 读取文件
							DWORD numRead;
							ReadFile(pBlockFile, vecBuf._Myfirst(), sizeof(PointXYZIPRGBA)*count, &numRead, NULL);

							HdPointXYZ* pPtXYZ = NULL;
							U8* pPtInten = NULL;
							HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
							// 写入坐标和强度
						//	m_pHlzWrite->WriteBlockData(curBlock->m_block, pPtXYZ, pPtInten, curBlock->m_block.numPoint);
							if (!m_pHlzWrite->m_header.isCompress)
							{
								// 不压缩，写入坐标和强度
								// 内存点云拆分为坐标和强度,写入块内部点
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY,m_iMin, m_iMax);
								curBlock->m_block.numPoint = cntWrite;

								m_pHlzWrite->WriteBlockData(curBlock->m_block, pPtXYZ, pPtInten, pPtColor, curBlock->m_block.numPoint);
							}
							else
							{
								// 压缩，写入压缩后的数据
								// 重排序
								std::sort(vecBuf.begin(), vecBuf.end(), lessByXYZ);
								// 内存点云拆分为坐标和强度,写入块内部点
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
								curBlock->m_block.numPoint = cntWrite;

								CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
								ptXYZEncoder.encodePoints();

								m_pHlzWrite->WriteBlockData(curBlock->m_block, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, curBlock->m_block.numPoint);
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
							vecBuf.clear();
							vecBuf.swap(vector<PointXYZIPRGBA> ());

							if (pBlockFile)
							{
								CloseHandle(pBlockFile);
								pBlockFile = NULL;
							}
							// 删除块文件
							int err = remove(blockFileInfo.path.c_str());
							if (err == -1)
							{
								string strError = "无法删除文件：" ;
								strError = strError + blockFileInfo.path.c_str();
								perror(strError.c_str());
							}
						} // if(count < BLOCK_HOLD)

						//块文件先切分成子块，每个子块的点数小于阈值，然后在内存中对子块进行递归分包
						else
						{
							if(pBlockFile)
							{
								CloseHandle(pBlockFile);
								pBlockFile = NULL;
							}
							// 只写块索引信息
							curBlock->m_block.hasSubParcel = 1;					
							m_pHlzWrite->WriteBlockData(curBlock->m_block, NULL, NULL, NULL, 0);
							//块文件递归切割成子块
							vector<subBlockFileInfo> vecSubBlock;
							SplitBlock2Parts(curBlock->m_block.box, blockFileInfo.path.c_str(), vecSubBlock);
							vector<subBlockFileInfo>::iterator it_subBlk;
							//对每个子块进行分包并写入
//							U16 sbNum = 0;
							for (it_subBlk = vecSubBlock.begin(); it_subBlk != vecSubBlock.end(); it_subBlk++)
							{
								HdPointXYZ* pPtXYZ = NULL;
								U8* pPtInten = NULL;
								HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
								vector<ParcelFileInfo> vecParcelInfo;
								//子块分包
								//SplitSbkFile2Parcel_Buffer(curBlock->m_block.box, it_subBlk->path.c_str(), vecParcelInfo);
								SplitSbkFile2Parcel_Buffer(it_subBlk->box, it_subBlk->path.c_str(), vecParcelInfo);
								// 将切分后包文件的点云写入
								U16 iPcl = 0;
								HdRefPoint refPtPcl;
								for (std::vector<ParcelFileInfo>::iterator it_parcel = vecParcelInfo.begin();
									it_parcel != vecParcelInfo.end(); it_parcel++)
								{
									if ((*it_parcel).numPoint == 0 )
									{
										continue;
									}
									CHdParcel* parcel = new CHdParcel;
									iPcl = curBlock->m_pListParcel.size() + 1;
									parcel->m_nParcelNo = iPcl;// + sbNum*1000;
									parcel->m_bkNo = curBlock->m_nBlockNo;
//									iPcl++;
									parcel->m_parcel.box = (*it_parcel).box;
									parcel->m_parcel.numPoint = (*it_parcel).numPoint;
									refPtPcl.FromBox(parcel->m_parcel.box);

									// 数据点
									const hd::stringc& sParcelPath = (*it_parcel).path;
									//包数据
									std:: map<string , vector<PointXYZIPRGBA* > >:: iterator iter_parcel = m_ParcelData.find(sParcelPath.c_str());

									// 写入坐标和强度
								//	m_pHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, parcel->m_parcel.numPoint);
									if (!m_pHlzWrite->m_header.isCompress)
									{
										// 不压缩，写入坐标和强度
										// 内存点云拆分为坐标和强度
										//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
										U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
										parcel->m_parcel.numPoint = cntWrite;

										m_pHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, pPtColor, parcel->m_parcel.numPoint);
									}
									else
									{
										// 压缩，写入压缩后的数据
										// 重排序
										std::sort(iter_parcel->second.begin(), iter_parcel->second.end(), pLessByXYZ);
										// 内存点云拆分为坐标和强度
										//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
										U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
										parcel->m_parcel.numPoint = cntWrite;

										CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
										ptXYZEncoder.encodePoints();
										m_pHlzWrite->WriteParcelData(parcel->m_parcel, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, parcel->m_parcel.numPoint);
										// 回收内存
										ptXYZEncoder.clear();
									}
									// 添加包索引记录						
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
									// 清除包文件，节约内存
									m_ParcelData.erase(sParcelPath.c_str());
								} // 写入分包

								// 更新块信息
								curBlock->Update();

								// 回收内存
								vecParcelInfo.clear();
								vecParcelInfo.swap(vector<ParcelFileInfo>());

								// 回收内存
								clearParcelData();
								clearBlocksetData();

							} // 块切分成子块
							// 回收内存
							vecSubBlock.clear();
							vecSubBlock.swap(vector<subBlockFileInfo>());
							// 删除块文件
							int err = remove(blockFileInfo.path.c_str());
							if (err == -1)
							{
								string strError = "无法删除文件：" ;
								strError = strError + blockFileInfo.path.c_str();
								perror(strError.c_str());
							}
						}  // else (count <= BLOCK_HOLD)

					}// for (U32 n=0; n < arrayBlock.size(); n++)

					// 回收内存
					arrayBlock.clear();
					arrayBlock.swap(vector<BlockFileInfo>());

				}  // else if (curBlockset->m_blockSet.numPoint > MAX_POINT_NUM)
				curBlockset->Update();
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
	U64 CHlzCloudBuilder::writeNextLevelDataNextZone(CHdLevel* pPreLevel, U8 nLevelNo, CHdLevel* pCurLevel)
	{
		pCurLevel->m_level.numBlocksetX = (U16)ceil((pPreLevel->m_level.numBlocksetX) / 2.0);
		pCurLevel->m_level.numBlocksetY = (U16)ceil((pPreLevel->m_level.numBlocksetY) / 2.0);
		//pCurLevel->m_level.numBlockset = pCurLevel->m_level.numBlocksetX * pCurLevel->m_level.numBlocksetY;
		pCurLevel->m_level.sizeX = pPreLevel->m_level.sizeX * 2.f;
		pCurLevel->m_level.sizeY = pPreLevel->m_level.sizeY * 2.f;
		m_stepX = pCurLevel->m_level.sizeX;
		m_stepY = pCurLevel->m_level.sizeY;
		pCurLevel->m_level.pointNum = (U32)hd_round32(pPreLevel->m_level.pointNum/4.f);		// 大概的点数

		// 写入当前层索引
		m_pNextHlzWrite->WriteLevelInfo(pCurLevel->m_level);
		// 置0后精确统计
		pCurLevel->m_level.pointNum = 0;	

		// 当前层切分的块集字典表, 当前层写完后替换m_BlocksetFilesNext
		std::map<U32, BlockSetFileInfo> curBsFiles;

		int iPreBSCount = m_BlocksetFilesNext.size();
		int iAgrCount = 0;
		int currentCount = 0;
		// 计算点云范围包围盒的起点所在上层块集作为当前块集的起点号
		//U32 nBlockNoX = (U32)(m_fullExtent.MinEdge.X - m_zoneStartX) / pCurLevel->m_level.sizeX - 2;
		//U32 nBlockNoY = (U32)(m_fullExtent.MinEdge.Y - m_zoneStartY) / pCurLevel->m_level.sizeY - 2;
		for (int iY = 0 ; iY < pCurLevel->m_level.numBlocksetY; iY++)
		{
			for (int jX = 0; jX < pCurLevel->m_level.numBlocksetX; jX++)
			{			
				// 获得上一层的四个块集
				std::vector<BlockSetFileInfo> preBlocksetFiles;
				getPreLevelBlocksetNext(jX, iY, pPreLevel, preBlocksetFiles);

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
				m_pNextHlzWrite->WriteNextLevelBlockset(BsFileName, preBlocksetFiles, curBlockset->m_blockSet,pCurLevel,curBlockset->m_nBlockSetNo);
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
				F32 BlockSetOffsetX = curBlockset->m_blockSet.box.MinEdge.X;
				F32 BlockSetOffsetY = curBlockset->m_blockSet.box.MinEdge.Y;
				if(DEBUG_LOG)
				{
					char logmsg[512] = {0};

					sprintf_s(logmsg,"%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
						curBlockset->m_blockSet.box.MinEdge.X,curBlockset->m_blockSet.box.MinEdge.Y,
						curBlockset->m_blockSet.box.MinEdge.Z,curBlockset->m_blockSet.box.MaxEdge.X,
						curBlockset->m_blockSet.box.MaxEdge.Y,curBlockset->m_blockSet.box.MaxEdge.Z);
					m_pNextHlzWrite->WriteLogFile(logmsg);
					sprintf_s(logmsg,"---------------------------------------------\n");
					m_pNextHlzWrite->WriteLogFile(logmsg);
				}

				/***********************第一种情况*************************/
				if (curBlockset->m_blockSet.numPoint <= BLOCK_HOLD)
				{
					// 不需要向下分块
					curBlockset->m_blockSet.hasSubBlock = 0;

// 					FILE* pBsFile = fopen(BsFileName,"rb");			
// 					if (pBsFile == NULL)
// 					{
// 						continue;
// 					}
					HANDLE pBsFile = CreateFile(BsFileName,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
					if (pBsFile == INVALID_HANDLE_VALUE)
					{
						continue;
					}

					// 读取到内存
					// 存储点信息
					std::vector<PointXYZIPRGBA> vecBuf;
					try
					{
						vecBuf.resize(curBlockset->m_blockSet.numPoint);
					}
					catch(...)
					{
						::MessageBox(NULL,"内存分配出错，请提高内存配置或采用\"临时文件方式\"", NULL, MB_OK);
						// 删除临时文件
						RemoveTempFiles(m_savePath.c_str());
						return 0;
					}
//					fseek(pBsFile, 0, SEEK_SET);
//					fread(vecBuf._Myfirst, sizeof(PointXYZIPRGBA), curBlockset->m_blockSet.numPoint, pBsFile);
					LARGE_INTEGER li1, li2;
					li1.HighPart = 0;
					li1.LowPart = 0;
					li2.HighPart = 0;
					li2.LowPart = 0;
					SetFilePointerEx(pBsFile, li1, &li2, FILE_BEGIN);
//					SetFilePointer(pBsFile, 0, 0, FILE_BEGIN);
					DWORD numRead;
					ReadFile(pBsFile, vecBuf._Myfirst(), sizeof(PointXYZIPRGBA)*(curBlockset->m_blockSet.numPoint), &numRead, NULL);
					// 关闭块集文件
					if (pBsFile)
					{
						CloseHandle(pBsFile);
						pBsFile = NULL;
					}

					
					HdPointXYZ* pPtXYZ = NULL;
					U8* pPtInten = NULL;
					HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
					if (!m_pNextHlzWrite->m_header.isCompress)
					{
						// 不压缩，直接写入原始坐标和强度
						// 内存点云拆分为坐标和强度
						//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
						U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), curBlockset->m_blockSet.numPoint, &refPtBS, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
						curBlockset->m_blockSet.numPoint = cntWrite;

						m_pNextHlzWrite->WriteBlockSetData(curBlockset->m_blockSet, pPtXYZ, pPtInten, pPtColor, curBlockset->m_blockSet.numPoint);
					}
					else
					{
						// 压缩坐标，并写入
						//重排序
						std::sort(vecBuf.begin(), vecBuf.end(), lessByXYZ);
						// 内存点云拆分为坐标和强度
						//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
						U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), curBlockset->m_blockSet.numPoint, &refPtBS, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
						curBlockset->m_blockSet.numPoint = cntWrite;

						CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
						ptXYZEncoder.encodePoints();

						m_pNextHlzWrite->WriteBlockSetData(curBlockset->m_blockSet, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, curBlockset->m_blockSet.numPoint);
						// 回收内存
						ptXYZEncoder.clear();
					}
					// 写入日志文件
					//char logmsg[512] = {0};
					//sprintf_s(logmsg,"块集,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n", 
					//	curBlockset->m_blockSet.numPoint,
					//	curBlockset->m_blockSet.box.MinEdge.X, curBlockset->m_blockSet.box.MinEdge.Y, curBlockset->m_blockSet.box.MinEdge.Z, 
					//	curBlockset->m_blockSet.box.MaxEdge.X, curBlockset->m_blockSet.box.MaxEdge.Y,curBlockset->m_blockSet.box.MaxEdge.Z);
					//m_pHlzWrite->WriteLogFile(logmsg);

					// 统计总块数
					m_pNextHlzWrite->m_header.number_of_col++;

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

				/***********************第二种情况*************************/
				else if (curBlockset->m_blockSet.numPoint > BLOCK_HOLD && curBlockset->m_blockSet.numPoint <= MAX_POINT_NUM)
				{// else分块写入
					// else分块写入
					// 该块集对应的块文件夹
					std::string blocksPath = BsFileName;
					blocksPath = blocksPath.substr(0, blocksPath.find_last_of('.'));

					// 采用二分的思想,对块集点云继续分割为块文件	
					std::vector<BlockFileInfo> arrayBlock;
					SplitBlockset2Block_BufferNext(BsInfo,arrayBlock);

					curBlockset->m_blockSet.hasSubBlock = 1;
					curBlockset->m_blockSet.numPoint = 0; // 重新置0,因为内部会自动统计

					// 只写索引文件
					m_pNextHlzWrite->WriteBlockSetData(curBlockset->m_blockSet, NULL, NULL, NULL, 0);

					CHdBlock* curBlock = NULL;

					for (U32 n=0; n < arrayBlock.size(); n++)
					{
						BlockFileInfo& blockFileInfo = *(arrayBlock._Myfirst() + n);
						std:: map<string, vector<PointXYZIPRGBA *> > :: iterator iterator_block = m_pBlockData.find(blockFileInfo.path.c_str());
						if (iterator_block == m_pBlockData.end())
						{
							continue;
						}
						U32 count =  iterator_block->second.size();

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

							// 当前块文件不需要再切分

							HdPointXYZ* pPtXYZ = NULL;
							U8* pPtInten = NULL;
							HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
							// 写入坐标和强度
						//	m_pHlzWrite->WriteBlockData(curBlock->m_block, pPtXYZ, pPtInten, curBlockset->m_blockSet.numPoint);
							if (!m_pNextHlzWrite->m_header.isCompress)
							{
								// 不压缩，写入坐标和强度
								// 内存点云拆分为坐标和强度,写入块内点云
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = pPtArray2PtBlock(iterator_block->second._Myfirst(), count, &bkRefPt, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
								curBlockset->m_blockSet.numPoint = cntWrite;

								m_pNextHlzWrite->WriteBlockData(curBlock->m_block, pPtXYZ, pPtInten, pPtColor, curBlock->m_block.numPoint);
							}
							else
							{
								// 压缩，写入压缩后的数据
								// 重排序
								std::sort(iterator_block->second.begin(), iterator_block->second.end(), pLessByXYZ);
								// 内存点云拆分为坐标和强度,写入块内点云
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = pPtArray2PtBlock(iterator_block->second._Myfirst(), count, &bkRefPt, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
								curBlockset->m_blockSet.numPoint = cntWrite;

								CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
								ptXYZEncoder.encodePoints();

								m_pNextHlzWrite->WriteBlockData(curBlock->m_block, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, curBlock->m_block.numPoint);
								// 回收内存
								ptXYZEncoder.clear();
							}
							// 写入日志文件
							//char logmsg[512] = {0};
							//sprintf_s(logmsg,"块,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n", 
							//	curBlock->m_block.numPoint,
							//	curBlock->m_block.box.MinEdge.X, curBlock->m_block.box.MinEdge.Y, 
							//	curBlock->m_block.box.MinEdge.Z, curBlock->m_block.box.MaxEdge.X, 
							//	curBlock->m_block.box.MaxEdge.Y, curBlock->m_block.box.MaxEdge.Z);
							//m_pHlzWrite->WriteLogFile(logmsg);

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
							std::string parcelsPath = blockFileInfo.path.c_str(); //sBlockPath.c_str();
							parcelsPath = parcelsPath.substr(0, parcelsPath.find_last_of('.'));

							// 将块切分为包
							std::vector<ParcelFileInfo> splitParcels;
							SplitBlock2Parcel_Buffer(curBlock->m_block.box,blockFileInfo.path.c_str(),parcelsPath.c_str(),splitParcels);

							// 只写块索引信息
							curBlock->m_block.hasSubParcel = 1;
							curBlock->m_block.numPoint = 0;		// 重新置0,内部添加包时自动统计
							m_pNextHlzWrite->WriteBlockData(curBlock->m_block, NULL, NULL, NULL, 0);

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
								hd::stringc& sParcelPath = (*it).path;
								std:: map<string, vector<PointXYZIPRGBA*> >::iterator iter_parcel = m_ParcelData.find(sParcelPath.c_str());

								// 内存点云拆分为坐标和强度,写入包内点云
								HdPointXYZ* pPtXYZ = NULL;
								U8* pPtInten = NULL;
								HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
								// 写入坐标和强度
							//	m_pHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, parcel->m_parcel.numPoint);
								if (!m_pNextHlzWrite->m_header.isCompress)
								{
									// 不压缩，写入坐标和强度
									//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
									U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst() , parcel->m_parcel.numPoint, &pclRefPt, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
									parcel->m_parcel.numPoint = cntWrite;

									m_pNextHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, pPtColor, parcel->m_parcel.numPoint);
								}
								else
								{
									// 压缩，写入压缩后的数据
									// 重排序
									std::sort(iter_parcel->second.begin(), iter_parcel->second.end(), pLessByXYZ);
									//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
									U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &pclRefPt, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
									parcel->m_parcel.numPoint = cntWrite;

									CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
									ptXYZEncoder.encodePoints();
									m_pNextHlzWrite->WriteParcelData(parcel->m_parcel, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, parcel->m_parcel.numPoint);
									// 回收内存
									ptXYZEncoder.clear();
								}
								curBlock->AddParcel(parcel);

								// 写入日志文件
								//char logmsg[512] = {0};
								//sprintf_s(logmsg,"包,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n", 
								//	parcel->m_parcel.numPoint,
								//	parcel->m_parcel.box.MinEdge.X, parcel->m_parcel.box.MinEdge.Y, 
								//	parcel->m_parcel.box.MinEdge.Z, parcel->m_parcel.box.MaxEdge.X, 
								//	parcel->m_parcel.box.MaxEdge.Y,parcel->m_parcel.box.MaxEdge.Z);
								//m_pHlzWrite->WriteLogFile(logmsg);

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

								// 将包文件删除，节省内存空间
								m_ParcelData.erase(sParcelPath.c_str());

							}//for (std::vector<ParcelFileInfo>::iterator it = splitParcels.begin();	
							curBlock->Update();
						}

						clearParcelData();     //清除包文件

					}
					curBlockset->Update();
					clearBlocksetData();
				}// else if (BLOCK_HOLD<curBlockset->m_blockSet.numPoint <= MAX_POINT_NUM)
				
				// 大于限定值，切分子包写入
				/***********************第三种情况*************************/
				else if (curBlockset->m_blockSet.numPoint > MAX_POINT_NUM)
				{

					// else分块写入
					// 该块集对应的块文件夹
					std::string blocksPath = BsFileName;
					blocksPath = blocksPath.substr(0, blocksPath.find_last_of('.'));

					// 采用二分的思想,对块集点云继续分割为块文件	
					std::vector<BlockFileInfo> arrayBlock;
					SplitBlockFilesNext(BsInfo,arrayBlock);

					curBlockset->m_blockSet.hasSubBlock = 1;
					curBlockset->m_blockSet.numPoint = 0; // 重新置0,因为内部会自动统计
					// 只写索引文件
					m_pNextHlzWrite->WriteBlockSetData(curBlockset->m_blockSet, NULL, NULL, NULL, 0);

					CHdBlock* curBlock = NULL;
					//块文件
					for (U32 n=0; n < arrayBlock.size(); n++)
					{
						BlockFileInfo& blockFileInfo = *(arrayBlock._Myfirst() + n);
// 						FILE* pBlockFile = fopen(blockFileInfo.path.c_str(), "rb");
// 						if (pBlockFile == NULL)
// 						{
// 							continue;
// 						}
// 
// 						fseek(pBlockFile,0,SEEK_END);
// 						U32 fileSize = ftell(pBlockFile);
// 						U32 count = fileSize / (sizeof(PointXYZIPRGBA));
						HANDLE pBlockFile = CreateFile(blockFileInfo.path.c_str(),
							GENERIC_READ,
							FILE_SHARE_READ,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
						if (pBlockFile == INVALID_HANDLE_VALUE)
						{
							continue;
						}
						LARGE_INTEGER li1,li2;
						li1.HighPart = 0;
						li1.LowPart = 0;
						li2.HighPart = 0;
						li2.LowPart = 0;
						SetFilePointerEx(pBlockFile, li1, &li2, FILE_END);
						U64 fileSize = ((U64)li2.HighPart<<32)|li2.LowPart;
						U64 count = fileSize / (sizeof(PointXYZIPRGBA));
						LARGE_INTEGER li3;
						li3.HighPart = 0;
						li3.LowPart = 0;
						SetFilePointerEx(pBlockFile, li1, &li3, FILE_BEGIN);

						// 当前块对象
						curBlock = new CHdBlock;
						curBlock->m_bsNo = curBlockset->m_nBlockSetNo;
						curBlock->m_nBlockNo = blockFileInfo.index;
						curBlockset->AddBlockRec(curBlock);
						// 当前块对象的空间范围
						getBlockBox(centerBS,halfSzBS,curBlock->m_block.box,curBlock->m_nBlockNo);
						HdRefPoint refPtBK;
						refPtBK.FromBox(curBlock->m_block.box);
						CHdVector3df centerBK = curBlock->m_block.box.getCenter();
						CHdVector3df halfSzBK = curBlock->m_block.box.getExtent() / 2.0f;
						if (count <= BLOCK_HOLD)
						{
							//块文件无需切分，直接写入
							curBlock->m_block.hasSubParcel = 0;
							curBlock->m_block.numPoint = count;

							// 当前块文件不需要再切分
							vector<PointXYZIPRGBA> vecBuf;
							vecBuf.resize(count);
// 							fseek(pBlockFile, 0, SEEK_SET);
// 							fread(vecBuf._Myfirst,sizeof(PointXYZIPRGBA),count,pBlockFile);
							// 读取文件
							DWORD numRead;
							ReadFile(pBlockFile, vecBuf._Myfirst(), sizeof(PointXYZIPRGBA)*count, &numRead, NULL);

							HdPointXYZ* pPtXYZ = NULL;
							U8* pPtInten = NULL;
							HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
							// 写入坐标和强度
						//	m_pHlzWrite->WriteBlockData(curBlock->m_block, pPtXYZ, pPtInten, curBlock->m_block.numPoint);
							if (!m_pNextHlzWrite->m_header.isCompress)
							{
								// 不压缩，写入坐标和强度
								// 内存点云拆分为坐标和强度,写入块内部点
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY,m_iMin, m_iMax);
								curBlock->m_block.numPoint = cntWrite;

								m_pNextHlzWrite->WriteBlockData(curBlock->m_block, pPtXYZ, pPtInten, pPtColor, curBlock->m_block.numPoint);
							}
							else
							{
								// 压缩，写入压缩后的数据
								// 重排序
								std::sort(vecBuf.begin(), vecBuf.end(), lessByXYZ);
								// 内存点云拆分为坐标和强度,写入块内部点
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
								curBlock->m_block.numPoint = cntWrite;

								CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
								ptXYZEncoder.encodePoints();

								m_pNextHlzWrite->WriteBlockData(curBlock->m_block, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, curBlock->m_block.numPoint);
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
							vecBuf.clear();
							vecBuf.swap(vector<PointXYZIPRGBA> ());

							if (pBlockFile)
							{
								CloseHandle(pBlockFile);
								pBlockFile = NULL;
							}
							// 删除块文件
							int err = remove(blockFileInfo.path.c_str());
							if (err == -1)
							{
								string strError = "无法删除文件：" ;
								strError = strError + blockFileInfo.path.c_str();
								perror(strError.c_str());
							}
						} // if(count < BLOCK_HOLD)

						//块文件先切分成子块，每个子块的点数小于阈值，然后在内存中对子块进行递归分包
						else
						{
							if(pBlockFile)
							{
								CloseHandle(pBlockFile);
								pBlockFile = NULL;
							}
							// 只写块索引信息
							curBlock->m_block.hasSubParcel = 1;					
							m_pNextHlzWrite->WriteBlockData(curBlock->m_block, NULL, NULL, NULL, 0);
							//块文件递归切割成子块
							vector<subBlockFileInfo> vecSubBlock;
							SplitBlock2PartsNext(curBlock->m_block.box, blockFileInfo.path.c_str(), vecSubBlock);
							vector<subBlockFileInfo>::iterator it_subBlk;
							//对每个子块进行分包并写入
//							U16 sbNum = 0;
							for (it_subBlk = vecSubBlock.begin(); it_subBlk != vecSubBlock.end(); it_subBlk++)
							{
								HdPointXYZ* pPtXYZ = NULL;
								U8* pPtInten = NULL;
								HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
								vector<ParcelFileInfo> vecParcelInfo;
								//子块分包
								//SplitSbkFile2Parcel_Buffer(curBlock->m_block.box, it_subBlk->path.c_str(), vecParcelInfo);
								SplitSbkFile2Parcel_BufferNext(it_subBlk->box, it_subBlk->path.c_str(), vecParcelInfo);
								// 将切分后包文件的点云写入
								U16 iPcl = 0;
								HdRefPoint refPtPcl;
								for (std::vector<ParcelFileInfo>::iterator it_parcel = vecParcelInfo.begin();
									it_parcel != vecParcelInfo.end(); it_parcel++)
								{
									if ((*it_parcel).numPoint == 0 )
									{
										continue;
									}
									CHdParcel* parcel = new CHdParcel;
									iPcl = curBlock->m_pListParcel.size() + 1;
									parcel->m_nParcelNo = iPcl;// + sbNum*1000;
									parcel->m_bkNo = curBlock->m_nBlockNo;
//									iPcl++;
									parcel->m_parcel.box = (*it_parcel).box;
									parcel->m_parcel.numPoint = (*it_parcel).numPoint;
									refPtPcl.FromBox(parcel->m_parcel.box);

									// 数据点
									const hd::stringc& sParcelPath = (*it_parcel).path;
									//包数据
									std:: map<string , vector<PointXYZIPRGBA* > >:: iterator iter_parcel = m_ParcelData.find(sParcelPath.c_str());

									// 写入坐标和强度
								//	m_pHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, parcel->m_parcel.numPoint);
									if (!m_pNextHlzWrite->m_header.isCompress)
									{
										// 不压缩，写入坐标和强度
										// 内存点云拆分为坐标和强度
										//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
										U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten,&pPtColor,BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
										parcel->m_parcel.numPoint = cntWrite;

										m_pNextHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, pPtColor, parcel->m_parcel.numPoint);
									}
									else
									{
										// 压缩，写入压缩后的数据
										// 重排序
										std::sort(iter_parcel->second.begin(), iter_parcel->second.end(), pLessByXYZ);
										// 内存点云拆分为坐标和强度
										//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
										U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten,&pPtColor,BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
										parcel->m_parcel.numPoint = cntWrite;

										CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
										ptXYZEncoder.encodePoints();
										m_pNextHlzWrite->WriteParcelData(parcel->m_parcel, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, parcel->m_parcel.numPoint);
										// 回收内存
										ptXYZEncoder.clear();
									}
									// 添加包索引记录						
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
									// 清除包文件，节约内存
									m_ParcelData.erase(sParcelPath.c_str());
								} // 写入分包

								// 更新块信息
								curBlock->Update();

								// 回收内存
								vecParcelInfo.clear();
								vecParcelInfo.swap(vector<ParcelFileInfo>());

								// 回收内存
								clearParcelData();
								clearBlocksetData();

							} // 块切分成子块
							// 回收内存
							vecSubBlock.clear();
							vecSubBlock.swap(vector<subBlockFileInfo>());
							// 删除块文件
							int err = remove(blockFileInfo.path.c_str());
							if (err == -1)
							{
								string strError = "无法删除文件：" ;
								strError = strError + blockFileInfo.path.c_str();
								perror(strError.c_str());
							}
						}  // else (count <= BLOCK_HOLD)

					}// for (U32 n=0; n < arrayBlock.size(); n++)

					// 回收内存
					arrayBlock.clear();
					arrayBlock.swap(vector<BlockFileInfo>());

				}  // else if (curBlockset->m_blockSet.numPoint > MAX_POINT_NUM)
				curBlockset->Update();
			}// for j
		}// for i
		pCurLevel->Update();
		pCurLevel->UpdateScale();

		// 写入日志文件
		if(DEBUG_LOG)
		{
			char logmsg[512] = {0};
			sprintf_s(logmsg,"pre:%d,agrC:%d,current:%d\n", iPreBSCount,iAgrCount,currentCount);		
			m_pNextHlzWrite->WriteLogFile(logmsg);
		}

		// 删除上一层块集文件数据
		RemoveBSFiles(m_BlocksetFilesNext);
		// 用当前层替换前一层的块集文件记录
		m_BlocksetFilesNext = curBsFiles;

		return pCurLevel->m_level.pointNum;
	}

	void CHlzCloudBuilder::getPreLevelBlockset(U16 xNo, U16 yNo, CHdLevel* pPreLevel, std::vector<BlockSetFileInfo>& preBlocksetFiles)
	{
		// 当前层格网(xNo, yNo)对应的上一层的格网编号为(xNo*2, yNo*2);
		U16 xPre = xNo*2;
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
		}
	}

	void CHlzCloudBuilder::getPreLevelBlocksetNext(U16 xNo, U16 yNo, CHdLevel* pPreLevel, std::vector<BlockSetFileInfo>& preBlocksetFiles)
	{
		// 当前层格网(xNo, yNo)对应的上一层的格网编号为(xNo*2, yNo*2);
		U16 xPre = xNo*2;
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
				if (m_BlocksetFilesNext.count(index) == 0)
					continue;

				preBlocksetFiles.push_back(m_BlocksetFilesNext[index]);

				if(DEBUG_LOG)
				{
					char logmsg[512] = {0};
					sprintf_s(logmsg,"level-%d-%d:%d,%d,%d,%d\n",pPreLevel->m_level.numBlocksetX,pPreLevel->m_level.numBlocksetY,pPreLevel->m_levelNo,index,xPre+jX,yPre+iY);
					m_pNextHlzWrite->WriteLogFile(logmsg);
				}					
			}
		}
	}
	void CHlzCloudBuilder::writeLevel0Data(CHdLevel* pLevel0)
	{
		// 写入第0层的索引信息
		//pLevel0->m_level.m_numBlockset = m_BlocksetFiles.size();			// 有效块集个数
		m_pHlzWrite->WriteLevelInfo(pLevel0->m_level);

		// 每个临时文件的点对应一个块集的点
		U16 iBS = 0;
		HdPointXYZ* pPtXYZ = NULL;
		U8* pPtInten = NULL;
		HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
		U32 err;
		//对于每个块集文件，点数小于阈值的直接写入文件，大于阈值的，则继续划分成块，存储在内存中
		for (map<U32, BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
			it != m_BlocksetFiles.end(); it++)
		{
// 			FILE* pFile = fopen((it->second).path.c_str(),"rb");			
// 
// 			if (pFile == NULL)
// 			{
// 				continue;
// 			}
			HANDLE pFile = CreateFile(it->second.path.c_str(),
				GENERIC_READ, 
				FILE_SHARE_READ, 
				NULL, 
				OPEN_EXISTING, 
				FILE_ATTRIBUTE_NORMAL, 
				NULL);
			if (pFile == INVALID_HANDLE_VALUE)
			{
				continue;
			}
			if (processCallback)
			{			
				processCallback((float)iBS / m_BlocksetFiles.size(), HDSCENE_IDS_WRITING_ZERO_LEVEL);
			}

			U32 fileNo = it->first;
			// 计算得到本块集的偏移量
			F32 BlockSetoffsetX = it->second.box.MinEdge.X;
			F32 BlockSetoffsetY = it->second.box.MinEdge.Y;
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
			U64 count = 0;
//			fseek(pFile,0,SEEK_END);
// 			long fileSize = ftell(pFile);
// 			count = fileSize / (sizeof(PointXYZIPRGBA));
			LARGE_INTEGER li1,li2;
			li1.HighPart = 0;
			li1.LowPart = 0;
			li2.HighPart = 0;
			li2.LowPart = 0;
			SetFilePointerEx(pFile, li1, &li2, FILE_END);
			U64 fileSize = ((U64)li2.HighPart << 32)|li2.LowPart;
			count = fileSize / (sizeof(PointXYZIPRGBA));
			if (count == 0)
			{
				CloseHandle(pFile);
				pFile = NULL;
				delete pBlockSet;
				pBlockSet = NULL;
				continue;
			}
			if (it->second.numPoint != count)
			{
				it->second.numPoint = count;
			}

			pLevel0->AddBlockSetRec(pBlockSet);

			HdRefPoint refPtBK;
			//一个块集文件
			/****************第一种情况**********************/
			if (count <= BLOCK_HOLD)
			{//块集的中的总点数小于64000，直接写入文件 

				// 读取到内存
				std::vector<PointXYZIPRGBA> vecBuf;// 存储点信息
				try
				{
					vecBuf.resize(count);
				}
				catch(...)
				{
					::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
					// 删除临时文件
					RemoveTempFiles(m_savePath.c_str());
					return ;
				}
				//定位到文件头部
//				fseek(pFile, 0, SEEK_SET);
//				fread(vecBuf._Myfirst,sizeof(PointXYZIPRGBA),count,pFile);
				LARGE_INTEGER li1,li2;
				li1.HighPart = 0;
				li1.LowPart = 0;
				li2.HighPart = 0;
				li2.LowPart = 0;
				SetFilePointerEx(pFile, li1, &li2, FILE_BEGIN);
				DWORD numRead;
				ReadFile(pFile, vecBuf._Myfirst(), count*sizeof(PointXYZIPRGBA), &numRead, NULL);
				// 块集内有数据情况下,需要单独更新点数
				pBlockSet->m_blockSet.hasSubBlock = 0;
				pBlockSet->m_blockSet.numPoint = count;
				// 关闭块集文件
				CloseHandle(pFile);
				pFile = NULL;

				if (!m_pHlzWrite->m_header.isCompress)
				{
					// 不压缩，
					// 写入块集内部点,内存点云拆分为坐标和强度
					//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
					U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBS, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
					pBlockSet->m_blockSet.numPoint = cntWrite;	
					m_pHlzWrite->WriteBlockSetData(pBlockSet->m_blockSet, pPtXYZ, pPtInten, pPtColor, pBlockSet->m_blockSet.numPoint);
				}
				else
				{
					// 压缩坐标，并写入
					// 重排序
					std::sort(vecBuf.begin(), vecBuf.end(),lessByXYZ);
					// 写入块集内部点,内存点云拆分为坐标和强度
					//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
					U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBS, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
					pBlockSet->m_blockSet.numPoint = cntWrite;

					CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
					ptXYZEncoder.encodePoints();

					m_pHlzWrite->WriteBlockSetData(pBlockSet->m_blockSet, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, pBlockSet->m_blockSet.numPoint);
					// 回收内存
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
			} // if (count <= BLOCK_HOLD)

			/****************第二种情况**********************/
			// 块集中的点数小于阈值， 直接在内存中处理
			else if (count >BLOCK_HOLD && count <= MAX_POINT_NUM)
			{
				// else，块集------->块文件(分成8份)，以块为单位写入
				// 关闭块集文件
				if(pFile)
				{
					CloseHandle(pFile);
					pFile = NULL;
				}

				// 采用二分的思想,对块集点云继续分割为块文件
				std::vector<BlockFileInfo> arrayBlock;    //分别存储8个块文件的信息
				//double start_time = GetTickCount();
				SplitBlockset2Block_Buffer(it->second,arrayBlock);
				//double stop_time = GetTickCount();
				//printf("块集到块：%f    ms\n", stop_time-start_time);

				pBlockSet->m_blockSet.hasSubBlock = 1;			
				// 只写块集索引
				m_pHlzWrite->WriteBlockSetData(pBlockSet->m_blockSet, NULL, NULL, NULL, 0);

				// 统计总块集数
				m_pHlzWrite->m_header.number_of_col += arrayBlock.size();
				//对于每个块文件
				for (U32 i=0; i < arrayBlock.size(); i++)
				{
					BlockFileInfo& blockFile = *(arrayBlock._Myfirst() + i);
					if(blockFile.numPoint == 0)
					{
						continue;
					}

					//定位到相应的块数据
					std::map <string , vector<PointXYZIPRGBA* > > :: iterator iter_block = m_pBlockData.find(blockFile.path.c_str());
					if (iter_block == m_pBlockData.end())
					{
						continue;
					}
					count = iter_block->second.size();
					if (count == 0)
					{
						continue;
					}
					// 当前块对象
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
					{// 当前块文件不需要再切分，直接写入

						pCurBlock->m_block.hasSubParcel = 0;
						pCurBlock->m_block.numPoint = count;
						if (!m_pHlzWrite->m_header.isCompress)
						{
							// 不压缩，写入坐标和强度
							// 内存点云拆分为坐标和强度,写入块内部点
							//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
							U32 cntWrite = pPtArray2PtBlock(iter_block->second._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
							pCurBlock->m_block.numPoint = cntWrite;
							m_pHlzWrite->WriteBlockData(pCurBlock->m_block, pPtXYZ, pPtInten, pPtColor, pCurBlock->m_block.numPoint);
						}
						else
						{
							// 重排序
							std::sort(iter_block->second.begin(), iter_block->second.end(),pLessByXYZ);
							// 内存点云拆分为坐标和强度,写入块内部点
							//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
							U32 cntWrite = pPtArray2PtBlock(iter_block->second._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
							pCurBlock->m_block.numPoint = cntWrite;

							// 压缩，写入压缩后的数据
							CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
							ptXYZEncoder.encodePoints();

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
					{// 块文件-------->包文件(八叉树切分)
						// 切分后包的路径
						std::string parcelsPath = blockFile.path.c_str();//sBlockPath.c_str();
						parcelsPath = parcelsPath.substr(0, parcelsPath.find_last_of('.'));
						// 将块切分为包
						std::vector<ParcelFileInfo> splitParcels;
						SplitBlock2Parcel_Buffer(pCurBlock->m_block.box,blockFile.path.c_str(),parcelsPath.c_str(),splitParcels);
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
							const hd::stringc& sParcelPath = (*it).path;
							//包数据
							std:: map<string , vector<PointXYZIPRGBA *> >:: iterator iter_parcel = m_ParcelData.find(sParcelPath.c_str());
							if (!m_pHlzWrite->m_header.isCompress)
							{
								// 内存点云拆分为坐标和强度
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY,m_iMin, m_iMax);
								parcel->m_parcel.numPoint = cntWrite;
								// 不压缩，写入坐标和强度
								m_pHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, pPtColor, parcel->m_parcel.numPoint);
							}
							else
							{
								// 压缩，写入压缩后的数据
								// 重排序
								std::sort(iter_parcel->second.begin(), iter_parcel->second.end(),pLessByXYZ);
								// 内存点云拆分为坐标和强度
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
								parcel->m_parcel.numPoint = cntWrite;

								CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
								ptXYZEncoder.encodePoints();

								m_pHlzWrite->WriteParcelData(parcel->m_parcel, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, parcel->m_parcel.numPoint);
								// 回收内存
								ptXYZEncoder.clear();
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
							// 清除包文件，节约内存
							m_ParcelData.erase(sParcelPath.c_str());
						}
						clearParcelData();
					}// else分包写入

					pCurBlock->Update();
					// 将块文件删除，节省磁盘空间
					m_pBlockData.erase(blockFile.path.c_str());

				}

				clearBlocksetData();    //清空块集数据

				pBlockSet->Update();
			}   // else if (BLOCK_HOLD < count <= MAX_POINT_NUM)	

			/****************第三种情况**********************/
			//块集中的点数大于阈值，分割成小文件，然后再内存中处理小文件
			else if (count > MAX_POINT_NUM)
			{
				// 关闭块集文件
				if(pFile)
				{
					CloseHandle(pFile);
					pFile = NULL;
				}
				// 采用二分的思想,对块集点云继续分割为块文件
				std::vector<BlockFileInfo> arrayBlock;
				SplitBlockFiles(it->second,arrayBlock);

				pBlockSet->m_blockSet.hasSubBlock = 1;			
				// 只写块集索引
				m_pHlzWrite->WriteBlockSetData(pBlockSet->m_blockSet, NULL, NULL, NULL, 0);

				// 统计总块集数
				m_pHlzWrite->m_header.number_of_col += arrayBlock.size();
				// 对于每个块文件，写入或者递归切分
				for (U32 i=0; i < arrayBlock.size(); i++)
				{
					BlockFileInfo& blockFile = *(arrayBlock._Myfirst() + i);
					if(blockFile.numPoint == 0)
						continue;
// 					FILE* pBlockFile = fopen(blockFile.path.c_str(), "rb");
// 					if (pBlockFile == NULL)
// 					{
// 						continue;
// 					}
// 					fseek(pBlockFile,0,SEEK_END);
// 					fileSize = ftell(pBlockFile);
// 					count = fileSize / (sizeof(PointXYZIPRGBA));
					HANDLE pBlockFile = CreateFile(blockFile.path.c_str(),
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
					if (pBlockFile == INVALID_HANDLE_VALUE)
					{
						continue;
					}
					LARGE_INTEGER li1, li2, li3;
					li1.HighPart = 0;
					li1.LowPart = 0;
					li2.HighPart = 0;
					li2.LowPart = 0;
					li3.HighPart = 0;
					li3.LowPart = 0;
					SetFilePointerEx(pBlockFile, li1, &li2, FILE_END);
					fileSize = ((U64)li2.HighPart<<32)|li2.LowPart;
					count = fileSize / (sizeof(PointXYZIPRGBA));
					SetFilePointerEx(pBlockFile, li1, &li3, FILE_BEGIN);
					if (count == 0)
					{
						CloseHandle(pBlockFile);
						pBlockFile = NULL;
						continue;
					}
					// 当前块对象
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
						//块文件无需切分，直接写入
						pCurBlock->m_block.hasSubParcel = 0;
						pCurBlock->m_block.numPoint = count;

						// 当前块文件不需要再切分
						vector<PointXYZIPRGBA> vecBuf;
						vecBuf.resize(count);
// 						fseek(pBlockFile, 0, SEEK_SET);
// 						fread(vecBuf._Myfirst,sizeof(PointXYZIPRGBA),count,pBlockFile);
						// 读取文件
						DWORD numRead;
						ReadFile(pBlockFile, vecBuf._Myfirst(), sizeof(PointXYZIPRGBA)*count, &numRead, NULL);

						if (!m_pHlzWrite->m_header.isCompress)
						{
							// 不压缩，写入坐标和强度
							// 内存点云拆分为坐标和强度,写入块内部点
							//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
							U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
							pCurBlock->m_block.numPoint = cntWrite;
							m_pHlzWrite->WriteBlockData(pCurBlock->m_block, pPtXYZ, pPtInten,pPtColor, pCurBlock->m_block.numPoint);
						}
						else
						{
							// 压缩，写入压缩后的数据
							// 重排序
							std::sort(vecBuf.begin(), vecBuf.end(), lessByXYZ);
							// 内存点云拆分为坐标和强度,写入块内部点
							//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
							U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
							pCurBlock->m_block.numPoint = cntWrite;

							CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
							ptXYZEncoder.encodePoints();

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
						if (pBlockFile)
						{
							CloseHandle(pBlockFile);
							pBlockFile = NULL;
						}
						vecBuf.clear();
						vecBuf.swap(vector<PointXYZIPRGBA> ());
						// 删除块文件
						int err = remove(blockFile.path.c_str());
						if (err == -1)
						{
							string strError = "无法删除文件：" ;
							strError = strError + blockFile.path.c_str();
							perror(strError.c_str());
						}
					} // if(count < BLOCK_HOLD)

					//块文件先切分成子块，每个子块的点数小于阈值，然后在内存中对子块进行递归分包
					else
					{
						if (pBlockFile)
						{
							CloseHandle(pBlockFile);
							pBlockFile = NULL;
						}
						// 只写块索引信息
						pCurBlock->m_block.hasSubParcel = 1;					
						m_pHlzWrite->WriteBlockData(pCurBlock->m_block, NULL, NULL, NULL, 0);
						//块文件递归切割成子块
						vector<subBlockFileInfo> vecSubBlock;
						SplitBlock2Parts(pCurBlock->m_block.box, blockFile.path.c_str(), vecSubBlock);
						vector<subBlockFileInfo>::iterator it_subBlk;
						//对每个子块进行分包并写入
//						U16 sbNum = 0;
						for (it_subBlk = vecSubBlock.begin(); it_subBlk != vecSubBlock.end(); it_subBlk++)
						{
							vector<ParcelFileInfo> vecParcelInfo;
							//子块分包
							//SplitSbkFile2Parcel_Buffer(pCurBlock->m_block.box, it_subBlk->path.c_str(), vecParcelInfo);
							SplitSbkFile2Parcel_Buffer(it_subBlk->box, it_subBlk->path.c_str(), vecParcelInfo);
							// 将切分后包文件的点云写入
							U16 iPcl = 0;
							HdRefPoint refPtPcl;
							for (std::vector<ParcelFileInfo>::iterator it_parcel = vecParcelInfo.begin();
								it_parcel != vecParcelInfo.end(); it_parcel++)
							{
								if ((*it_parcel).numPoint == 0 )
								{
									continue;
								}
								CHdParcel* parcel = new CHdParcel;
								iPcl = pCurBlock->m_pListParcel.size() + 1;
								parcel->m_nParcelNo = iPcl;// + sbNum*1000;
								parcel->m_bkNo = pCurBlock->m_nBlockNo;
//								iPcl++;
								parcel->m_parcel.box = (*it_parcel).box;
								parcel->m_parcel.numPoint = (*it_parcel).numPoint;
								refPtPcl.FromBox(parcel->m_parcel.box);

								// 数据点
								const hd::stringc& sParcelPath = (*it_parcel).path;
								//包数据
								std:: map<string , vector<PointXYZIPRGBA *> >:: iterator iter_parcel = m_ParcelData.find(sParcelPath.c_str());

								if (!m_pHlzWrite->m_header.isCompress)
								{
									// 不压缩，写入坐标和强度
									// 内存点云拆分为坐标和强度
									//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
									U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten,&pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
									parcel->m_parcel.numPoint = cntWrite;

									m_pHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, pPtColor, parcel->m_parcel.numPoint);
								}
								else
								{
									// 压缩，写入压缩后的数据
									// 重排序
									std::sort(iter_parcel->second.begin(), iter_parcel->second.end(), pLessByXYZ);
									// 内存点云拆分为坐标和强度
									//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
									U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
									parcel->m_parcel.numPoint = cntWrite;

									CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
									ptXYZEncoder.encodePoints();

									m_pHlzWrite->WriteParcelData(parcel->m_parcel, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, parcel->m_parcel.numPoint);
									// 回收内存
									ptXYZEncoder.clear();
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
								// 清除包文件，节约内存
								m_ParcelData.erase(sParcelPath.c_str());
							} // 写入分包

//							sbNum ++;

							//更新块信息
							pCurBlock->Update();

							// 回收内存
							vecParcelInfo.clear();
							vecParcelInfo.swap(vector<ParcelFileInfo>());
							clearParcelData();
							clearBlocksetData();

						} // 块切分成子块
						// 回收内存
						vecSubBlock.clear();
						vecSubBlock.swap(vector<subBlockFileInfo>());


						int err = remove(blockFile.path.c_str());
						if (err == -1)
						{
							string strError = "无法删除文件：" ;
							strError = strError + blockFile.path.c_str();
							perror(strError.c_str());
						}

					}  // else (count > BLOCK_HOLD)

				} // for (U32 i=0; i < arrayBlock.size(); i++)

				// 回收内存
				arrayBlock.clear();
				arrayBlock.swap(vector<BlockFileInfo>());

			}  // else if (count > MAX_POINT_NUM)

			pBlockSet->Update();
		}
		pLevel0->Update();
		pLevel0->UpdateScale();
	}
	// 相邻投影带块集数据写入
	void CHlzCloudBuilder::writeLevel0DataNext(CHdLevel* pLevel0)
	{
		// 写入第0层的索引信息
		//pLevel0->m_level.m_numBlockset = m_BlocksetFiles.size();			// 有效块集个数
		m_pNextHlzWrite->WriteLevelInfo(pLevel0->m_level);

		// 每个临时文件的点对应一个块集的点
		U16 iBS = 0;
		HdPointXYZ* pPtXYZ = NULL;
		U8* pPtInten = NULL;
		HdPtColor* pPtColor = NULL;  //新增颜色分量  袁亮  20160625
		U32 err;
		//对于每个块集文件，点数小于阈值的直接写入文件，大于阈值的，则继续划分成块，存储在内存中
		for (map<U32, BlockSetFileInfo>::iterator it = m_BlocksetFilesNext.begin();
			it != m_BlocksetFilesNext.end(); it++)
		{
// 			FILE* pFile = fopen((it->second).path.c_str(),"rb");			
// 
// 			if (pFile == NULL)
// 			{
// 				continue;
// 			}
			HANDLE pFile = CreateFile(it->second.path.c_str(),
				GENERIC_READ, 
				FILE_SHARE_READ, 
				NULL, 
				OPEN_EXISTING, 
				FILE_ATTRIBUTE_NORMAL, 
				NULL);
			if (pFile == INVALID_HANDLE_VALUE)
			{
				continue;
			}
			if (processCallback)
			{			
				processCallback((float)iBS / m_BlocksetFilesNext.size(), HDSCENE_IDS_WRITING_ZERO_LEVEL);
			}

			U32 fileNo = it->first;
			// 计算得到本块集的偏移量
			F32 BlockSetoffsetX = it->second.box.MinEdge.X;
			F32 BlockSetoffsetY = it->second.box.MinEdge.Y;
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
			U64 count = 0;
//			fseek(pFile,0,SEEK_END);
// 			long fileSize = ftell(pFile);
// 			count = fileSize / (sizeof(PointXYZIPRGBA));
			LARGE_INTEGER li1,li2;
			li1.HighPart = 0;
			li1.LowPart = 0;
			li2.HighPart = 0;
			li2.LowPart = 0;
			SetFilePointerEx(pFile, li1, &li2, FILE_END);
			U64 fileSize = ((U64)li2.HighPart << 32)|li2.LowPart;
			count = fileSize / (sizeof(PointXYZIPRGBA));
			if (count == 0)
			{
				CloseHandle(pFile);
				pFile = NULL;
				delete pBlockSet;
				pBlockSet = NULL;
				continue;
			}
			if (it->second.numPoint != count)
			{
				it->second.numPoint = count;
			}

			pLevel0->AddBlockSetRec(pBlockSet);

			HdRefPoint refPtBK;
			//一个块集文件
			/****************第一种情况**********************/
			if (count <= BLOCK_HOLD)
			{//块集的中的总点数小于64000，直接写入文件 

				// 读取到内存
				std::vector<PointXYZIPRGBA> vecBuf;// 存储点信息
				try
				{
					vecBuf.resize(count);
				}
				catch(...)
				{
					::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
					// 删除临时文件
					RemoveTempFiles(m_savePath.c_str());
					return ;
				}
				//定位到文件头部
//				fseek(pFile, 0, SEEK_SET);
//				fread(vecBuf._Myfirst,sizeof(PointXYZIPRGBA),count,pFile);
				LARGE_INTEGER li1,li2;
				li1.HighPart = 0;
				li1.LowPart = 0;
				li2.HighPart = 0;
				li2.LowPart = 0;
				SetFilePointerEx(pFile, li1, &li2, FILE_BEGIN);
				DWORD numRead;
				ReadFile(pFile, vecBuf._Myfirst(), count*sizeof(PointXYZIPRGBA), &numRead, NULL);
				// 块集内有数据情况下,需要单独更新点数
				pBlockSet->m_blockSet.hasSubBlock = 0;
				pBlockSet->m_blockSet.numPoint = count;
				// 关闭块集文件
				CloseHandle(pFile);
				pFile = NULL;

				if (!m_pNextHlzWrite->m_header.isCompress)
				{
					// 不压缩，
					// 写入块集内部点,内存点云拆分为坐标和强度
					//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
					U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBS, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
					pBlockSet->m_blockSet.numPoint = cntWrite;	
					m_pNextHlzWrite->WriteBlockSetData(pBlockSet->m_blockSet, pPtXYZ, pPtInten, pPtColor, pBlockSet->m_blockSet.numPoint);
				}
				else
				{
					// 压缩坐标，并写入
					// 重排序
					std::sort(vecBuf.begin(), vecBuf.end(),lessByXYZ);
					// 写入块集内部点,内存点云拆分为坐标和强度
					//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
					U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBS, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
					pBlockSet->m_blockSet.numPoint = cntWrite;

					CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
					ptXYZEncoder.encodePoints();

					m_pNextHlzWrite->WriteBlockSetData(pBlockSet->m_blockSet, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, pBlockSet->m_blockSet.numPoint);
					// 回收内存
					ptXYZEncoder.clear();
				}
				// 统计总块数
				m_pNextHlzWrite->m_header.number_of_col++;

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
			} // if (count <= BLOCK_HOLD)

			/****************第二种情况**********************/
			// 块集中的点数小于阈值， 直接在内存中处理
			else if (count >BLOCK_HOLD && count <= MAX_POINT_NUM)
			{
				// else，块集------->块文件(分成8份)，以块为单位写入
				// 关闭块集文件
				if(pFile)
				{
					CloseHandle(pFile);
					pFile = NULL;
				}

				// 采用二分的思想,对块集点云继续分割为块文件
				std::vector<BlockFileInfo> arrayBlock;    //分别存储8个块文件的信息
				//double start_time = GetTickCount();
				SplitBlockset2Block_BufferNext(it->second,arrayBlock);
				//double stop_time = GetTickCount();
				//printf("块集到块：%f    ms\n", stop_time-start_time);

				pBlockSet->m_blockSet.hasSubBlock = 1;			
				// 只写块集索引
				m_pNextHlzWrite->WriteBlockSetData(pBlockSet->m_blockSet, NULL, NULL, NULL, 0);

				// 统计总块集数
				m_pNextHlzWrite->m_header.number_of_col += arrayBlock.size();
				//对于每个块文件
				for (U32 i=0; i < arrayBlock.size(); i++)
				{
					BlockFileInfo& blockFile = *(arrayBlock._Myfirst() + i);
					if(blockFile.numPoint == 0)
					{
						continue;
					}

					//定位到相应的块数据
					std::map <string , vector<PointXYZIPRGBA* > > :: iterator iter_block = m_pBlockData.find(blockFile.path.c_str());
					if (iter_block == m_pBlockData.end())
					{
						continue;
					}
					count = iter_block->second.size();
					if (count == 0)
					{
						continue;
					}
					// 当前块对象
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
					{// 当前块文件不需要再切分，直接写入

						pCurBlock->m_block.hasSubParcel = 0;
						pCurBlock->m_block.numPoint = count;
						if (!m_pNextHlzWrite->m_header.isCompress)
						{
							// 不压缩，写入坐标和强度
							// 内存点云拆分为坐标和强度,写入块内部点
							//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
							U32 cntWrite = pPtArray2PtBlock(iter_block->second._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
							pCurBlock->m_block.numPoint = cntWrite;
							m_pNextHlzWrite->WriteBlockData(pCurBlock->m_block, pPtXYZ, pPtInten, pPtColor, pCurBlock->m_block.numPoint);
						}
						else
						{
							// 重排序
							std::sort(iter_block->second.begin(), iter_block->second.end(),pLessByXYZ);
							// 内存点云拆分为坐标和强度,写入块内部点
							//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
							U32 cntWrite = pPtArray2PtBlock(iter_block->second._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
							pCurBlock->m_block.numPoint = cntWrite;

							// 压缩，写入压缩后的数据
							CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
							ptXYZEncoder.encodePoints();

							m_pNextHlzWrite->WriteBlockData(pCurBlock->m_block, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, pCurBlock->m_block.numPoint);
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
					{// 块文件-------->包文件(八叉树切分)
						// 切分后包的路径
						std::string parcelsPath = blockFile.path.c_str();//sBlockPath.c_str();
						parcelsPath = parcelsPath.substr(0, parcelsPath.find_last_of('.'));
						// 将块切分为包
						std::vector<ParcelFileInfo> splitParcels;
						SplitBlock2Parcel_Buffer(pCurBlock->m_block.box,blockFile.path.c_str(),parcelsPath.c_str(),splitParcels);
						// 只写块索引信息
						pCurBlock->m_block.hasSubParcel = 1;					
						m_pNextHlzWrite->WriteBlockData(pCurBlock->m_block, NULL, NULL, NULL, 0);

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
							const hd::stringc& sParcelPath = (*it).path;
							//包数据
							std:: map<string , vector<PointXYZIPRGBA *> >:: iterator iter_parcel = m_ParcelData.find(sParcelPath.c_str());
							if (!m_pNextHlzWrite->m_header.isCompress)
							{
								// 内存点云拆分为坐标和强度
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY,m_iMin, m_iMax);
								parcel->m_parcel.numPoint = cntWrite;
								// 不压缩，写入坐标和强度
								m_pNextHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, pPtColor, parcel->m_parcel.numPoint);
							}
							else
							{
								// 压缩，写入压缩后的数据
								// 重排序
								std::sort(iter_parcel->second.begin(), iter_parcel->second.end(),pLessByXYZ);
								// 内存点云拆分为坐标和强度
								//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
								U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
								parcel->m_parcel.numPoint = cntWrite;

								CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
								ptXYZEncoder.encodePoints();

								m_pNextHlzWrite->WriteParcelData(parcel->m_parcel, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, parcel->m_parcel.numPoint);
								// 回收内存
								ptXYZEncoder.clear();
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
							// 清除包文件，节约内存
							m_ParcelData.erase(sParcelPath.c_str());
						}
						clearParcelData();
					}// else分包写入

					pCurBlock->Update();
					// 将块文件删除，节省磁盘空间
					m_pBlockData.erase(blockFile.path.c_str());

				}

				clearBlocksetData();    //清空块集数据

				pBlockSet->Update();
			}   // else if (BLOCK_HOLD < count <= MAX_POINT_NUM)	

			/****************第三种情况**********************/
			//块集中的点数大于阈值，分割成小文件，然后再内存中处理小文件
			else if (count > MAX_POINT_NUM)
			{
				// 关闭块集文件
				if(pFile)
				{
					CloseHandle(pFile);
					pFile = NULL;
				}
				// 采用二分的思想,对块集点云继续分割为块文件
				std::vector<BlockFileInfo> arrayBlock;
				SplitBlockFilesNext(it->second,arrayBlock);

				pBlockSet->m_blockSet.hasSubBlock = 1;			
				// 只写块集索引
				m_pNextHlzWrite->WriteBlockSetData(pBlockSet->m_blockSet, NULL, NULL, NULL, 0);

				// 统计总块集数
				m_pNextHlzWrite->m_header.number_of_col += arrayBlock.size();
				// 对于每个块文件，写入或者递归切分
				for (U32 i=0; i < arrayBlock.size(); i++)
				{
					BlockFileInfo& blockFile = *(arrayBlock._Myfirst() + i);
					if(blockFile.numPoint == 0)
						continue;
// 					FILE* pBlockFile = fopen(blockFile.path.c_str(), "rb");
// 					if (pBlockFile == NULL)
// 					{
// 						continue;
// 					}
// 					fseek(pBlockFile,0,SEEK_END);
// 					fileSize = ftell(pBlockFile);
// 					count = fileSize / (sizeof(PointXYZIPRGBA));
					HANDLE pBlockFile = CreateFile(blockFile.path.c_str(),
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
					if (pBlockFile == INVALID_HANDLE_VALUE)
					{
						continue;
					}
					LARGE_INTEGER li1, li2, li3;
					li1.HighPart = 0;
					li1.LowPart = 0;
					li2.HighPart = 0;
					li2.LowPart = 0;
					li3.HighPart = 0;
					li3.LowPart = 0;
					SetFilePointerEx(pBlockFile, li1, &li2, FILE_END);
					fileSize = ((U64)li2.HighPart<<32)|li2.LowPart;
					count = fileSize / (sizeof(PointXYZIPRGBA));
					SetFilePointerEx(pBlockFile, li1, &li3, FILE_BEGIN);
					if (count == 0)
					{
						CloseHandle(pBlockFile);
						pBlockFile = NULL;
						continue;
					}
					// 当前块对象
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
						//块文件无需切分，直接写入
						pCurBlock->m_block.hasSubParcel = 0;
						pCurBlock->m_block.numPoint = count;

						// 当前块文件不需要再切分
						vector<PointXYZIPRGBA> vecBuf;
						vecBuf.resize(count);
// 						fseek(pBlockFile, 0, SEEK_SET);
// 						fread(vecBuf._Myfirst,sizeof(PointXYZIPRGBA),count,pBlockFile);
						// 读取文件
						DWORD numRead;
						ReadFile(pBlockFile, vecBuf._Myfirst(), sizeof(PointXYZIPRGBA)*count, &numRead, NULL);

						if (!m_pNextHlzWrite->m_header.isCompress)
						{
							// 不压缩，写入坐标和强度
							// 内存点云拆分为坐标和强度,写入块内部点
							//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
							U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
							pCurBlock->m_block.numPoint = cntWrite;

							m_pNextHlzWrite->WriteBlockData(pCurBlock->m_block, pPtXYZ, pPtInten, pPtColor, pCurBlock->m_block.numPoint);;
						}
						else
						{
							// 压缩，写入压缩后的数据
							// 重排序
							std::sort(vecBuf.begin(), vecBuf.end(), lessByXYZ);
							// 内存点云拆分为坐标和强度,写入块内部点
							//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
							U32 cntWrite = PtArray2PtBlock(vecBuf._Myfirst(), count, &refPtBK, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
							pCurBlock->m_block.numPoint = cntWrite;

							CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
							ptXYZEncoder.encodePoints();

							m_pNextHlzWrite->WriteBlockData(pCurBlock->m_block, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, pCurBlock->m_block.numPoint);
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
						if (pBlockFile)
						{
							CloseHandle(pBlockFile);
							pBlockFile = NULL;
						}
						vecBuf.clear();
						vecBuf.swap(vector<PointXYZIPRGBA> ());
						// 删除块文件
						int err = remove(blockFile.path.c_str());
						if (err == -1)
						{
							string strError = "无法删除文件：" ;
							strError = strError + blockFile.path.c_str();
							perror(strError.c_str());
						}
					} // if(count < BLOCK_HOLD)

					//块文件先切分成子块，每个子块的点数小于阈值，然后在内存中对子块进行递归分包
					else
					{
						if (pBlockFile)
						{
							CloseHandle(pBlockFile);
							pBlockFile = NULL;
						}
						// 只写块索引信息
						pCurBlock->m_block.hasSubParcel = 1;					
						m_pNextHlzWrite->WriteBlockData(pCurBlock->m_block, NULL, NULL, NULL, 0);
						//块文件递归切割成子块
						vector<subBlockFileInfo> vecSubBlock;
						SplitBlock2PartsNext(pCurBlock->m_block.box, blockFile.path.c_str(), vecSubBlock);
						vector<subBlockFileInfo>::iterator it_subBlk;
						//对每个子块进行分包并写入
//						U16 sbNum = 0;
						for (it_subBlk = vecSubBlock.begin(); it_subBlk != vecSubBlock.end(); it_subBlk++)
						{
							vector<ParcelFileInfo> vecParcelInfo;
							//子块分包
							//SplitSbkFile2Parcel_Buffer(pCurBlock->m_block.box, it_subBlk->path.c_str(), vecParcelInfo);
							SplitSbkFile2Parcel_BufferNext(it_subBlk->box, it_subBlk->path.c_str(), vecParcelInfo);
							// 将切分后包文件的点云写入
							U16 iPcl = 0;
							HdRefPoint refPtPcl;
							for (std::vector<ParcelFileInfo>::iterator it_parcel = vecParcelInfo.begin();
								it_parcel != vecParcelInfo.end(); it_parcel++)
							{
								if ((*it_parcel).numPoint == 0 )
								{
									continue;
								}
								CHdParcel* parcel = new CHdParcel;
								iPcl = pCurBlock->m_pListParcel.size() + 1;
								parcel->m_nParcelNo = iPcl;// + sbNum*1000;
								parcel->m_bkNo = pCurBlock->m_nBlockNo;
//								iPcl++;
								parcel->m_parcel.box = (*it_parcel).box;
								parcel->m_parcel.numPoint = (*it_parcel).numPoint;
								refPtPcl.FromBox(parcel->m_parcel.box);

								// 数据点
								const hd::stringc& sParcelPath = (*it_parcel).path;
								//包数据
								std:: map<string , vector<PointXYZIPRGBA *> >:: iterator iter_parcel = m_ParcelData.find(sParcelPath.c_str());

								if (!m_pNextHlzWrite->m_header.isCompress)
								{
									// 不压缩，写入坐标和强度
									// 内存点云拆分为坐标和强度
									//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
									U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
									parcel->m_parcel.numPoint = cntWrite;

									m_pNextHlzWrite->WriteParcelData(parcel->m_parcel, pPtXYZ, pPtInten, pPtColor, parcel->m_parcel.numPoint);
								}
								else
								{
									// 压缩，写入压缩后的数据
									// 重排序
									std::sort(iter_parcel->second.begin(), iter_parcel->second.end(), pLessByXYZ);
									// 内存点云拆分为坐标和强度
									//拆分出颜色数据，保存为pPtColor中    袁亮   20160625
									U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &refPtPcl, &pPtXYZ, &pPtInten, &pPtColor, BlockSetoffsetX,BlockSetoffsetY, m_iMin, m_iMax);
									parcel->m_parcel.numPoint = cntWrite;

									CPtXYZEncoder ptXYZEncoder(pPtXYZ, cntWrite);
									ptXYZEncoder.encodePoints();

									m_pNextHlzWrite->WriteParcelData(parcel->m_parcel, ptXYZEncoder.getBuffer(), ptXYZEncoder.getBufferSize(), pPtInten, pPtColor, parcel->m_parcel.numPoint);
									// 回收内存
									ptXYZEncoder.clear();
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
								// 清除包文件，节约内存
								m_ParcelData.erase(sParcelPath.c_str());
							} // 写入分包

//							sbNum ++;

							//更新块信息
							pCurBlock->Update();

							// 回收内存
							vecParcelInfo.clear();
							vecParcelInfo.swap(vector<ParcelFileInfo>());
							clearParcelData();
							clearBlocksetData();

						} // 块切分成子块
						// 回收内存
						vecSubBlock.clear();
						vecSubBlock.swap(vector<subBlockFileInfo>());


						int err = remove(blockFile.path.c_str());
						if (err == -1)
						{
							string strError = "无法删除文件：" ;
							strError = strError + blockFile.path.c_str();
							perror(strError.c_str());
						}

					}  // else (count > BLOCK_HOLD)

				} // for (U32 i=0; i < arrayBlock.size(); i++)

				// 回收内存
				arrayBlock.clear();
				arrayBlock.swap(vector<BlockFileInfo>());

			}  // else if (count > MAX_POINT_NUM)

			pBlockSet->Update();
		}
		pLevel0->Update();
		pLevel0->UpdateScale();
	}
	// 根据块集划分块文件,并保存在内存中
	bool CHlzCloudBuilder::SplitBlockset2Block_Buffer(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks)//
	{
		FILE* pFile = fopen(procBlockSet.path.c_str(), "rb");
		if (pFile == NULL)
		{
			return false;
		}

		fseek(pFile, 0, SEEK_SET);// 移到文件头
// 		HANDLE pFile = CreateFile(procBlockSet.path.c_str(),
// 			GENERIC_READ,
// 			FILE_SHARE_READ,
// 			NULL,
// 			OPEN_EXISTING,
// 			FILE_ATTRIBUTE_NORMAL,
// 			NULL);
// 		if (pFile == INVALID_HANDLE_VALUE)
// 		{
// 			return false;
// 		}
		// 创建快文件标识ID，为了保证ID的惟一性，使用存储目录作为ID
		std::string dir = procBlockSet.path;
		PointXYZIPRGBA* ptTmp;
		// 当前点所在的块序号
		CHdVector3df center = procBlockSet.box.getCenter();
		CHdVector3df halfSize = procBlockSet.box.getExtent() /2.0f;
		//CHdVector3df centerSlice(halfSize.X,halfSize.Y,center.Z);

		CHdVector3df centerSlice(0,0,0);
		int nFileX = (int)center.X/m_stepX;
		int nFileY = (int)center.Y/m_stepX;
		centerSlice.X =center.X - nFileX * m_stepX;
		centerSlice.Y =center.Y - nFileY * m_stepY;
		centerSlice.Z = center.Z;
		arrayBlocks.resize(8);

		//一次性读取整个块集文件中的数据
		if(m_pHlzWrite->ReadAllPts(pFile, m_blsDataBuf) <= 0)
			return false;

		// 切分生成的块文件对应的点
		vector<SplitMemoryBuf> blockPoints;
		blockPoints.resize(8);

		//开始切分
		for (I32 i=0; i < m_blsDataBuf.size(); i++)
		{
			ptTmp = &(*(m_blsDataBuf._Myfirst() + i));
			U8 blockIdx = 0;			
			//getPtBlockIndex(*ptTmp, center, blockIdx);
			getPtBlockIndex(*ptTmp, centerSlice, blockIdx);
			SplitMemoryBuf& memPoints = *(blockPoints._Myfirst() + blockIdx);

			if (memPoints.m_count >= memPoints.m_vecBuf.size())
			{
				memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
			}

			PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
			memPoints.m_count++;
			pPtMem = m_blsDataBuf._Myfirst() + i;
		}//for (I32 i=0; i < m_blsDataBuf.size(); i++)

		// 将内存中的点一次写入到块文件中
		for (I32 i = 0;i < 8;i++)
		{
			SplitMemoryBuf& blkPoints = *(blockPoints._Myfirst() + i);
			int nCount = blkPoints.m_count;
			if(nCount <= 0)
				continue;
			char blockDataID_temp[512] = {0};
			sprintf_s(blockDataID_temp, "%s\\block_%d.tmp", dir.c_str(), i);

			BlockFileInfo& blkInfo = *(arrayBlocks._Myfirst() + i);
			blkInfo.path = blockDataID_temp;
			blkInfo.numPoint += nCount;
			blkInfo.index = i;
			getBlockBox(center,halfSize,blkInfo.box,(U16)i);
			//分配内存空间
			int preSize = m_pBlockData[blkInfo.path.c_str()].size();  //已有大小
			m_pBlockData[blkInfo.path.c_str()].resize(preSize + nCount);
			std::map<string , vector<PointXYZIPRGBA *> > ::iterator iter_block = m_pBlockData.find(blkInfo.path.c_str());
// 			for (I32 j = 0; j < nCount; j++)
// 			{
// 				*(iter_block->second._Myfirst() + j + preSize) = *(blkPoints.m_vecBuf._Myfirst() + j);
// 			}
			memcpy(iter_block->second._Myfirst() + preSize, blkPoints.m_vecBuf._Myfirst(), nCount*sizeof(PointXYZIPRGBA*));
		}

//		CloseHandle(pFile);
		fclose(pFile);
		pFile = NULL;

		return true;
	}

	// 根据块集划分块文件,并保存在内存中(相邻块集)
	bool CHlzCloudBuilder::SplitBlockset2Block_BufferNext(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks)//
	{
		FILE* pFile = fopen(procBlockSet.path.c_str(), "rb");
		if (pFile == NULL)
		{
			return false;
		}

		fseek(pFile, 0, SEEK_SET);// 移到文件头
// 		HANDLE pFile = CreateFile(procBlockSet.path.c_str(),
// 			GENERIC_READ,
// 			FILE_SHARE_READ,
// 			NULL,
// 			OPEN_EXISTING,
// 			FILE_ATTRIBUTE_NORMAL,
// 			NULL);
// 		if (pFile == INVALID_HANDLE_VALUE)
// 		{
// 			return false;
// 		}
		// 创建快文件标识ID，为了保证ID的惟一性，使用存储目录作为ID
		std::string dir = procBlockSet.path;
		PointXYZIPRGBA* ptTmp;
		// 当前点所在的块序号
		CHdVector3df center = procBlockSet.box.getCenter();
		CHdVector3df halfSize = procBlockSet.box.getExtent() /2.0f;
		//CHdVector3df centerSlice(halfSize.X,halfSize.Y,center.Z);

		CHdVector3df centerSlice(0,0,0);
		int nFileX = (int)center.X/m_stepX;
		int nFileY = (int)center.Y/m_stepX;
		centerSlice.X =center.X - nFileX * m_stepX;
		centerSlice.Y =center.Y - nFileY * m_stepY;
		centerSlice.Z = center.Z;
		arrayBlocks.resize(8);

		//一次性读取整个块集文件中的数据
		if(m_pNextHlzWrite->ReadAllPts(pFile, m_blsDataBuf) <= 0)
			return false;

		// 切分生成的块文件对应的点
		vector<SplitMemoryBuf> blockPoints;
		blockPoints.resize(8);

		//开始切分
		for (I32 i=0; i < m_blsDataBuf.size(); i++)
		{
			ptTmp = &(*(m_blsDataBuf._Myfirst() + i));
			U8 blockIdx = 0;			
			//getPtBlockIndex(*ptTmp, center, blockIdx);
			getPtBlockIndex(*ptTmp, centerSlice, blockIdx);
			SplitMemoryBuf& memPoints = *(blockPoints._Myfirst() + blockIdx);

			if (memPoints.m_count >= memPoints.m_vecBuf.size())
			{
				memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
			}

			PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
			memPoints.m_count++;
			pPtMem = m_blsDataBuf._Myfirst() + i;
		}//for (I32 i=0; i < m_blsDataBuf.size(); i++)

		// 将内存中的点一次写入到块文件中
		for (I32 i = 0;i < 8;i++)
		{
			SplitMemoryBuf& blkPoints = *(blockPoints._Myfirst() + i);
			int nCount = blkPoints.m_count;
			if(nCount <= 0)
				continue;
			char blockDataID_temp[512] = {0};
			sprintf_s(blockDataID_temp, "%s\\block_%d.tmp", dir.c_str(), i);

			BlockFileInfo& blkInfo = *(arrayBlocks._Myfirst() + i);
			blkInfo.path = blockDataID_temp;
			blkInfo.numPoint += nCount;
			blkInfo.index = i;
			getBlockBox(center,halfSize,blkInfo.box,(U16)i);
			//分配内存空间
			int preSize = m_pBlockData[blkInfo.path.c_str()].size();  //已有大小
			m_pBlockData[blkInfo.path.c_str()].resize(preSize + nCount);
			std::map<string , vector<PointXYZIPRGBA *> > ::iterator iter_block = m_pBlockData.find(blkInfo.path.c_str());
// 			for (I32 j = 0; j < nCount; j++)
// 			{
// 				*(iter_block->second._Myfirst() + j + preSize) = *(blkPoints.m_vecBuf._Myfirst() + j);
// 			}
			memcpy(iter_block->second._Myfirst() + preSize, blkPoints.m_vecBuf._Myfirst(), nCount*sizeof(PointXYZIPRGBA*));
		}

//		CloseHandle(pFile);
		fclose(pFile);
		pFile = NULL;

		return true;
	}
	BOOL CHlzCloudBuilder::RemoveTempFiles(const char* path)
	{
		// 删除hls2hlz目录下所有临时文件
		//const char* path = m_savePath.c_str();

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
	BOOL CHlzCloudBuilder::RemoveBSFiles(std::map<U32, BlockSetFileInfo>& bsFiles)
	{
		for (map<U32,BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
			it != m_BlocksetFiles.end();it++)
		{	
			remove((it->second).path.c_str());
		}
		return TRUE;
	}

	void CHlzCloudBuilder::CloseTempFiles()
	{
		return;
		for (map<U32,/*pair<FILE*,string>*/BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
			it != m_BlocksetFiles.end();it++)

		{	
			string strtmp = (it->second).path;
			//if ((it->second).pFile != NULL)
			{
				// 关闭文件，同时把文件指针置空
				//fclose((it->second).pFile);
				//(it->second).pFile = NULL;
			}
		}
	}

	void CHlzCloudBuilder::getBlockBox(
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

	void CHlzCloudBuilder::getPtBlockIndex(PointXYZIPRGBA& pt, const CHdVector3df& center, U8& index)
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
	void CHlzCloudBuilder::StatIntensity(
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

	//!清空临时块集文件的内存空间
	void CHlzCloudBuilder :: clearBlocksetData()
	{
		//清空数据
		if (m_blsDataBuf.size()>0)
		{
			m_blsDataBuf.clear();
			m_blsDataBuf.swap(vector<PointXYZIPRGBA> ());
		}
		if(m_pBlockData.size() > 0)
		{
			std:: map<string, vector<PointXYZIPRGBA *> > :: iterator it;
			for (it = m_pBlockData.begin(); it != m_pBlockData.end(); it ++)
			{
				it->second.clear();
				it->second.swap(std::vector<PointXYZIPRGBA *>());    //使用swap函数来释放vector的空间
			}
			m_pBlockData.clear();
			m_pBlockData.swap(map<string , vector<PointXYZIPRGBA *> > ());
		}
	}

	//清空包文件
	void CHlzCloudBuilder :: clearParcelData()
	{
		if (m_ParcelData.size() > 0)
		{
			std::map<string, vector<PointXYZIPRGBA*> > :: iterator it;
			for (it = m_ParcelData.begin(); it != m_ParcelData.end(); it++)
			{
				it->second.clear();
				it->second.swap(std::vector<PointXYZIPRGBA*>());
			}
			m_ParcelData.clear();
			m_ParcelData.swap(map<string , vector<PointXYZIPRGBA*> > ());
		}
	}

	void CHlzCloudBuilder::clearSplitMemBuf(vector<SplitMemoryBuf> & vecMemBuf)
	{
		for (vector<SplitMemoryBuf>::iterator it = vecMemBuf.begin(); 
			it != vecMemBuf.end(); it ++)
		{
			it->clear();
		}
		vecMemBuf.clear();
		vecMemBuf.swap(vector<SplitMemoryBuf>());
	}

	void CHlzCloudBuilder::SetResampleZero( bool bResample,double dGridPrecision )
	{
		// 参数设置
		m_bResampleZero = bResample;
		m_dGridPrecision = dGridPrecision;
	}
}

// 计算HLS文件中的数据的最大值、最小值 
void CHlzCloudBuilder::calHlsExtnt(hd::IHLSReader* reader, double& x_min, double& x_max,double& y_min,double& y_max,double& z_min, double& z_max)
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
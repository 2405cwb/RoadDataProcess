#include "HlzMemCloudBuilder.h"
#include "HdLevel.h"
#include "HdBlockset.h"
#include "HdBlock.h"
#include "HdParcel.h"
#include "HdCoreData.h"
#include "..\hdCommon\hdSceneStr.h"
#include "..\hdGeoPosition\PositionTranfrom.h"
#include "..\hdHlslib\PtEncoder.h"
#include <direct.h>
#include <time.h>
#include <algorithm>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

#define BLOCK_HOLD 64000
#define DEBUG_LOG 0

namespace hd
{
	CHlzMemCloudBuilder::CHlzMemCloudBuilder(void)
		:m_pHlzWrite(NULL), m_pNextHlzWrite(NULL), m_pCoreData(NULL), m_pCoreDataNext(NULL),
		 m_blockSetStartX(0.0), m_blockSetStartY(0.0), m_blockSetStartXNext(0.0), m_blockSetStartYNext(0.0),
		 m_zoneStartX(0.0), m_zoneStartY(0.0), m_zoneEndX(0.0), m_zoneEndY(0.0), m_dCenter(0.0),
		 m_stepX(64.0), m_stepY(64.0), m_MinX(F64_MAX), m_MinY(F64_MAX), m_MinZ(F64_MAX),
		 m_MaxX(F64_MIN), m_MaxY(F64_MIN), m_MaxZ(F64_MIN), m_iMin(0xffff), m_iMax(0), m_pointNum(0), m_pointNumNext(0),
		 m_bResampleZero(false), m_dGridPrecision(1.0/256.0), processCallback(NULL)
	{
	}


	CHlzMemCloudBuilder::~CHlzMemCloudBuilder(void)
	{
		CloseFile();
	}

	bool CHlzMemCloudBuilder::OpenFile(const char* savePath, const char* savePathNext, const CHdBox3dd& fullExtent, double centerLongitude, bool compress /* = false */,  void (*procCallback)(float, const char*) /* = NULL*/)
	{
		//// 对该接口加密
		//string strSoftName = "hdVector";
		//char strMsg[256] = {0};
		//if (!CheckLicense(strSoftName.c_str(), strMsg))
		//{
		//	::MessageBox(NULL,"请向武汉汉宁轨道交通技术有限公司申请加密狗！", "提示",MB_OK);
		//	return false;
		//}

		if(savePath == NULL || savePathNext == NULL)
		{
			return false;
		}

		m_fullExtent = fullExtent;
		m_dCenter = centerLongitude;
		processCallback = procCallback;

		m_pHlzWrite = new CHLZWriter;
		m_pHlzWrite->m_header.isCompress = compress;
		m_pHlzWrite->SetGridPrecision(m_dGridPrecision);
	    m_pCoreData = new CHdCoreData;

		char drive[MAX_PATH] = {0};
		char dir[MAX_PATH] = {0};
		char filename[MAX_PATH] = {0};
		char ext[MAX_PATH] = {0};
		char path[MAX_PATH] = {0};

		_splitpath(savePath, drive, dir, filename, ext);
		if(strcmp(ext, ".hlz") != 0)
		{
			if(processCallback != NULL)
			{
				processCallback(0.0, HDSCENE_IDS_BUILDER_DATA_WORING);
				return false;
			}
		}
		
		_makepath(path, drive, dir, filename, NULL);
		m_savePath = path;
		m_savePath += "__temp";   //缓存文件存放目录
		_mkdir(m_savePath.c_str());

		//计算投影带的空间范围
		double dLongitude = m_dCenter - 1.5;
		double dLongitudeNext = dLongitude + 3.0 - 0.00000000001;
		double dLatitude = 0.0;
		CGeoPositionTran positionTrans;
		positionTrans.TranslateDegree2Gauss(dLongitude, dLatitude, m_zoneStartX, m_zoneStartY);
		positionTrans.TranslateDegree2Gauss(dLongitudeNext, dLatitude, m_zoneEndX, m_zoneEndY);

		//判断是否跨带，统计当前/分区hlz文件空间范围、起始块集坐标
		//a.未跨带，只生成一个hlz文件
		if(m_zoneStartX < m_fullExtent.MinEdge.X && m_fullExtent.MaxEdge.X < m_zoneEndX)
		{
			U32 nBlockSetNoX = (U32)(m_fullExtent.MinEdge.X - m_zoneStartX) / 64;
			U32 nBlockSetNoY = (U32)(m_fullExtent.MinEdge.Y - m_zoneStartY) / 64;
			m_blockSetStartX = m_zoneStartX + nBlockSetNoX * 64;
			m_blockSetStartY = m_zoneStartY + nBlockSetNoY * 64;

			CHdLevel* pLevel = new CHdLevel;
			pLevel->m_levelNo = 0;
			pLevel->m_level.sizeX = m_stepX;
			pLevel->m_level.sizeY = m_stepY;
			pLevel->m_level.numBlocksetX = (U16)ceil((m_fullExtent.MaxEdge.X - m_blockSetStartX) / m_stepX);
			pLevel->m_level.numBlocksetY = (U16)ceil((m_fullExtent.MaxEdge.Y - m_blockSetStartY) / m_stepY);
			m_pCoreData->AddLevelRec(pLevel);

			m_pHlzWrite->m_header.iZoneID = (U8)(m_dCenter / 3 + 0.5);
		}
		//b.跨过上一个3度带
		else if(m_fullExtent.MinEdge.X < m_zoneStartX)
		{
			m_pCoreDataNext = new CHdCoreData;
			m_pNextHlzWrite = new CHLZWriter;

			double dLongitudePre = 0.0;
			double dLatitudePre = 0.0;
			double dMinXPre = 0.0;
			double dMinYPre = 0.0;
			positionTrans.TranslateGauss2Degree(m_fullExtent.MinEdge.X, m_fullExtent.MinEdge.Y, m_dCenter, dLongitudePre, dLatitudePre);
			positionTrans.TranslateDegree2Gauss(dLongitudePre, dLatitudePre, dMinXPre, dMinYPre);

			m_fullExtent.MinEdge.X = m_zoneStartX;
			m_fullExtentNext.MinEdge.X = dMinXPre;
			m_fullExtentNext.MinEdge.Y = dMinYPre;
			m_fullExtentNext.MinEdge.Z = m_fullExtent.MinEdge.Z;
			m_fullExtentNext.MaxEdge.X = m_zoneEndX;
			m_fullExtentNext.MaxEdge.Y = m_fullExtent.MaxEdge.Y;
			m_fullExtentNext.MaxEdge.Z = m_fullExtent.MaxEdge.Z;

			U32 nBlockSetNoX = (U32)floor((m_fullExtent.MinEdge.X - m_zoneStartX) / 64);
			U32 nBlockSetNoY = (U32)floor((m_fullExtent.MinEdge.Y - m_zoneStartY) / 64);
			m_blockSetStartX = m_zoneStartX + nBlockSetNoX * 64;
			m_blockSetStartY = m_zoneStartY + nBlockSetNoY * 64;

			U32 nBlockSetNoXPre = (U32)floor((dMinXPre - m_zoneStartX) / 64);
			U32 nBlockSetNoYPre = (U32)floor((dMinYPre - m_zoneStartY) / 64);
			m_blockSetStartXNext = m_zoneStartX + nBlockSetNoXPre * 64;
			m_blockSetStartYNext = m_zoneStartY + nBlockSetNoYPre * 64;

			CHdLevel* pLevel = new CHdLevel;
			pLevel->m_levelNo = 0;
			pLevel->m_level.sizeX = m_stepX;
			pLevel->m_level.sizeY = m_stepY;
			pLevel->m_level.numBlocksetX = (U16)ceil((m_fullExtent.MaxEdge.X - m_fullExtent.MinEdge.X) / m_stepX);
			pLevel->m_level.numBlocksetY = (U16)ceil((m_fullExtent.MaxEdge.Y - m_fullExtent.MinEdge.Y) / m_stepY);
			m_pCoreData->AddLevelRec(pLevel);

			CHdLevel* pLevelNext = new CHdLevel;
			pLevelNext->m_levelNo = 0;
			pLevelNext->m_level.sizeX = m_stepX;
			pLevelNext->m_level.sizeY = m_stepY;
			pLevelNext->m_level.numBlocksetX = (U16)ceil((m_fullExtentNext.MaxEdge.X - m_blockSetStartXNext) / m_stepX);
			pLevelNext->m_level.numBlocksetY = (U16)ceil((m_fullExtentNext.MaxEdge.Y - m_blockSetStartYNext) / m_stepY);
			m_pCoreDataNext->AddLevelRec(pLevelNext);

			m_pHlzWrite->m_header.iZoneID = (U8)(m_dCenter / 3 + 0.5);
			m_pNextHlzWrite->m_header.iZoneID = m_pHlzWrite->m_header.iZoneID - 1;
		}
		//c.跨过下一个3度带
		else if(m_fullExtent.MinEdge.X > m_zoneStartX && m_fullExtent.MaxEdge.X > m_zoneEndX)
		{
			m_pCoreDataNext = new CHdCoreData;
			m_pNextHlzWrite = new CHLZWriter;

			double dLongitudeNext = 0.0;
			double dLatitudeNext = 0.0;
			double dMaxXNext = 0.0;
			double dMaxYNext = 0.0;
			positionTrans.TranslateGauss2Degree(m_fullExtent.MaxEdge.X,m_fullExtent.MaxEdge.Y, m_dCenter, dLongitudeNext,dLatitudeNext);
			positionTrans.TranslateDegree2Gauss(dLongitudeNext,dLatitudeNext,dMaxXNext,dMaxYNext);

			m_fullExtent.MaxEdge.X = m_zoneEndX;
			m_fullExtentNext.MinEdge.X = m_zoneStartX;
			m_fullExtentNext.MinEdge.Y = m_fullExtent.MinEdge.Y;
			m_fullExtentNext.MinEdge.Z = m_fullExtent.MinEdge.Z;
			m_fullExtentNext.MaxEdge.X = dMaxXNext;
			m_fullExtentNext.MaxEdge.Y = dMaxYNext;
			m_fullExtentNext.MaxEdge.Z = m_fullExtent.MaxEdge.Z;

			U32 nBlockSetNoX = (U32)floor((m_fullExtent.MinEdge.X - m_zoneStartX) / 64);
			U32 nBlockSetNoY = (U32)floor((m_fullExtent.MinEdge.Y - m_zoneStartY) / 64);
			m_blockSetStartX = m_zoneStartX + nBlockSetNoX * 64;
			m_blockSetStartY = m_zoneStartY + nBlockSetNoY * 64;

			U32 nBlockSetNoXNext = (U32)floor((m_fullExtentNext.MinEdge.X - m_zoneStartX) / 64);
			U32 nBlockSetNoYNext = (U32)floor((m_fullExtentNext.MinEdge.Y- m_zoneStartY) / 64);
			m_blockSetStartXNext = m_zoneStartX + nBlockSetNoXNext * 64;
			m_blockSetStartYNext = m_zoneStartY + nBlockSetNoYNext * 64;

			CHdLevel* pLevel = new CHdLevel;
			pLevel->m_levelNo = 0;
			pLevel->m_level.sizeX = m_stepX;
			pLevel->m_level.sizeY = m_stepY;
			pLevel->m_level.numBlocksetX = (U16)ceil((m_fullExtent.MaxEdge.X - m_fullExtent.MinEdge.X) / m_stepX);
			pLevel->m_level.numBlocksetY = (U16)ceil((m_fullExtent.MaxEdge.Y - m_fullExtent.MinEdge.Y) / m_stepY);
			m_pCoreData->AddLevelRec(pLevel);

			CHdLevel* pLevelNext = new CHdLevel;
			pLevelNext->m_levelNo = 0;
			pLevelNext->m_level.sizeX = m_stepX;
			pLevelNext->m_level.sizeY = m_stepY;
			pLevelNext->m_level.numBlocksetX = (U16)ceil((m_fullExtentNext.MaxEdge.X - m_blockSetStartXNext) / m_stepX);
			pLevelNext->m_level.numBlocksetY = (U16)ceil((m_fullExtentNext.MaxEdge.Y - m_blockSetStartYNext) / m_stepY);
			m_pCoreDataNext->AddLevelRec(pLevelNext);

			m_pHlzWrite->m_header.iZoneID = (U8)(m_dCenter / 3 + 0.5);
			m_pNextHlzWrite->m_header.iZoneID = m_pHlzWrite->m_header.iZoneID + 1;
		}

		//将范围信息写入hlz索引文件
		m_pHlzWrite->m_header.offsetX = m_blockSetStartX;
		m_pHlzWrite->m_header.offsetY = m_blockSetStartY;
		m_pHlzWrite->m_header.offsetZ = m_fullExtent.MinEdge.Z;
		m_pHlzWrite->m_header.min_x = (F32)0.0;
		m_pHlzWrite->m_header.min_y = (F32)0.0;
		m_pHlzWrite->m_header.min_z = (F32)0.0;
		m_pHlzWrite->m_header.max_x = (F32)(m_fullExtent.MaxEdge.X - m_pHlzWrite->m_header.offsetX);
		m_pHlzWrite->m_header.max_y = (F32)(m_fullExtent.MaxEdge.Y - m_pHlzWrite->m_header.offsetY);
		m_pHlzWrite->m_header.max_z = (F32)(m_fullExtent.MaxEdge.Z - m_pHlzWrite->m_header.offsetZ);

		time_t timer;
		time(&timer);
		tm* t_tm = localtime(&timer);
		m_pHlzWrite->m_header.file_creation_day = (U16)t_tm->tm_yday + 1;
		m_pHlzWrite->m_header.file_creation_year = (U16)t_tm->tm_year + 1900;

		if(!m_pHlzWrite->Open(savePath))
		{
			if (processCallback)
			{
				processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED); 
			}

			return false;
		}

		//当前hlz坐标范围统计
		F64 xRange = m_fullExtent.MaxEdge.X - m_fullExtent.MinEdge.X;
		F64 yRange = m_fullExtent.MaxEdge.Y - m_fullExtent.MinEdge.Y;
		F64 zRange = m_fullExtent.getExtent().Z;

		m_coordStats.xRcp = (F32)(1000.0f / (xRange));
		m_coordStats.yRcp = (F32)(1000.0f / (yRange));
		m_coordStats.zRcp = (F32)(1000.0f / (zRange));

		if(m_pCoreDataNext != NULL)
		{
			m_pNextHlzWrite->m_header.isCompress = compress;
			m_pNextHlzWrite->SetGridPrecision(m_dGridPrecision);

			m_pNextHlzWrite->m_header.offsetX = m_blockSetStartXNext;
			m_pNextHlzWrite->m_header.offsetY = m_blockSetStartYNext;
			m_pNextHlzWrite->m_header.offsetZ = m_fullExtentNext.MinEdge.Z;
			m_pNextHlzWrite->m_header.min_x = (F32)0.0;
			m_pNextHlzWrite->m_header.min_y = (F32)0.0;
			m_pNextHlzWrite->m_header.min_z = (F32)0.0;
			m_pNextHlzWrite->m_header.max_x = (F32)(m_fullExtentNext.MaxEdge.X - m_pNextHlzWrite->m_header.offsetX);
			m_pNextHlzWrite->m_header.max_y = (F32)(m_fullExtentNext.MaxEdge.Y - m_pNextHlzWrite->m_header.offsetY);
			m_pNextHlzWrite->m_header.max_z = (F32)(m_fullExtentNext.MaxEdge.Z - m_pNextHlzWrite->m_header.offsetZ);
			m_pNextHlzWrite->m_header.file_creation_day = (U16)t_tm->tm_yday + 1;
			m_pNextHlzWrite->m_header.file_creation_year = (U16)t_tm->tm_year + 1900;

			if(!m_pNextHlzWrite->Open(savePathNext))
			{
				if (processCallback)
				{
					processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
				}

				return false;
			}

			//分区hlz坐标范围统计
			F64 xRangeNext = m_fullExtentNext.MaxEdge.X - m_fullExtentNext.MinEdge.X;
			F64 yRangeNext = m_fullExtentNext.MaxEdge.Y - m_fullExtentNext.MinEdge.Y;
			F64 zRangeNext = m_fullExtentNext.getExtent().Z;

			m_coordStatsNext.xRcp = (F32)(1000.0f / (xRangeNext));
			m_coordStatsNext.yRcp = (F32)(1000.0f / (yRangeNext));
			m_coordStatsNext.zRcp = (F32)(1000.0f / (zRangeNext));
		}

		return true;
	}

	bool CHlzMemCloudBuilder::CreateHlz(const char* savePath, const CHdBox3dd& fullExtent, bool compress /* = false */,  void (*procCallback)(float, const char*) /* = NULL*/)
	{
		if(savePath == NULL)
		{
			return false;
		}

		m_fullExtent = fullExtent;
		processCallback = procCallback;

		m_pHlzWrite = new CHLZWriter;
		m_pHlzWrite->m_header.isCompress = compress;
		m_pHlzWrite->SetGridPrecision(m_dGridPrecision);
		m_pCoreData = new CHdCoreData;

		char drive[MAX_PATH] = {0};
		char dir[MAX_PATH] = {0};
		char filename[MAX_PATH] = {0};
		char ext[MAX_PATH] = {0};
		char path[MAX_PATH] = {0};

		_splitpath(savePath, drive, dir, filename, ext);
		if(strcmp(ext, ".hlz") != 0)
		{
			if(processCallback != NULL)
			{
				processCallback(0.0, HDSCENE_IDS_BUILDER_DATA_WORING);
				return false;
			}
		}

		_makepath(path, drive, dir, filename, NULL);
		m_savePath = path;
		m_savePath += "__temp";   //缓存文件存放目录
		_mkdir(m_savePath.c_str());

		I32 nBlockSetNoX = (I32)floor(m_fullExtent.MinEdge.X / m_stepX) ;
		I32 nBlockSetNoY = (I32)floor(m_fullExtent.MinEdge.Y / m_stepY) ;
		m_blockSetStartX = nBlockSetNoX * m_stepX;
		m_blockSetStartY = nBlockSetNoY * m_stepY;

		CHdLevel* pLevel = new CHdLevel;
		pLevel->m_levelNo = 0;
		pLevel->m_level.sizeX = m_stepX;
		pLevel->m_level.sizeY = m_stepY;
		pLevel->m_level.numBlocksetX = (U16)ceil((m_fullExtent.MaxEdge.X - m_blockSetStartX) / m_stepX);
		pLevel->m_level.numBlocksetY = (U16)ceil((m_fullExtent.MaxEdge.Y - m_blockSetStartY) / m_stepY);
		m_pCoreData->AddLevelRec(pLevel);

		//将范围信息写入hlz索引文件
		m_pHlzWrite->m_header.offsetX = m_blockSetStartX;
		m_pHlzWrite->m_header.offsetY = m_blockSetStartY;
		m_pHlzWrite->m_header.offsetZ = m_fullExtent.MinEdge.Z;
		m_pHlzWrite->m_header.min_x = (F32)0.0;
		m_pHlzWrite->m_header.min_y = (F32)0.0;
		m_pHlzWrite->m_header.min_z = (F32)0.0;
		m_pHlzWrite->m_header.max_x = (F32)(m_fullExtent.MaxEdge.X - m_pHlzWrite->m_header.offsetX);
		m_pHlzWrite->m_header.max_y = (F32)(m_fullExtent.MaxEdge.Y - m_pHlzWrite->m_header.offsetY);
		m_pHlzWrite->m_header.max_z = (F32)(m_fullExtent.MaxEdge.Z - m_pHlzWrite->m_header.offsetZ);

		time_t timer;
		time(&timer);
		tm* t_tm = localtime(&timer);
		m_pHlzWrite->m_header.file_creation_day = (U16)t_tm->tm_yday + 1;
		m_pHlzWrite->m_header.file_creation_year = (U16)t_tm->tm_year + 1900;

		if(!m_pHlzWrite->Open(savePath))
		{
			if (processCallback)
			{
				processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED); 
			}

			return false;
		}

		//当前hlz坐标范围统计
		F64 xRange = m_fullExtent.MaxEdge.X - m_fullExtent.MinEdge.X;
		F64 yRange = m_fullExtent.MaxEdge.Y - m_fullExtent.MinEdge.Y;
		F64 zRange = m_fullExtent.getExtent().Z;

		m_coordStats.xRcp = (F32)(1000.0f / (xRange));
		m_coordStats.yRcp = (F32)(1000.0f / (yRange));
		m_coordStats.zRcp = (F32)(1000.0f / (zRange));

		return true;
	}

	void CHlzMemCloudBuilder::SetResampleZero( bool bResample,double dGridPrecision )
	{
		// 参数设置
		m_bResampleZero = bResample;
		m_dGridPrecision = dGridPrecision;
	}

	bool CHlzMemCloudBuilder::WriteFile(const vector<PointXYZIPRGBA_D>& points)
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

		//创建第0层缓存目录
		char dir[MAX_PATH] = {0};
		sprintf_s(dir, "%s\\level0", m_savePath.c_str());
		if(_access(dir, 00) != 0)
		{
			_mkdir(dir);
		}

        CHdLevel* pLevel = m_pCoreData->GetLevelRec(0);
		if(pLevel == NULL)
		{
			pLevel = new CHdLevel;
			pLevel->m_levelNo = 0;
			m_pCoreData->AddLevelRec(pLevel);
		}

		CHdLevel* pLevelNext = NULL;
		if(m_pCoreDataNext != NULL)
		{
		    pLevelNext = m_pCoreDataNext->GetLevelRec(0);
			if(pLevelNext == NULL)
			{
				pLevelNext = new CHdLevel;
				pLevelNext->m_levelNo = 0;
				m_pCoreDataNext->AddLevelRec(pLevelNext);
			}
		}

		if(processCallback != NULL)
		{
			processCallback(0.0, HDSCENE_IDS_BEGIN_BLOCKSET_SEGMENTATION);
		}

		int wSize = 50000;
		PointXYZIPRGBA* pPtBuf4Write = new PointXYZIPRGBA[wSize];
		PointXYZIPRGBA* pPtBuf4WriteNext = new PointXYZIPRGBA[wSize];

		map<U32, SplitMemoryBuf>    BsPointsArray;
		map<U32, SplitMemoryBuf>    BsPointsArrayNext;
		U32 pointNum = points.size();
		std::vector<PointXYZIPRGBA> f32Points;
		f32Points.resize(pointNum);
		for(U32 i = 0; i < pointNum; i++)
		{
			PointXYZIPRGBA_D& pt_d = *(points._Myfirst() + i);
			PointXYZIPRGBA& pt_f = *(f32Points._Myfirst() + i);
			if(!pt_d.isValid())
				continue;

			//a.统计实际空间范围和强度范围
			m_MinX = MIN(m_MinX, pt_d.x);
			m_MinY = MIN(m_MinY, pt_d.y);
			m_MinZ = MIN(m_MinZ, pt_d.z);
			m_iMin = MIN(m_iMin, pt_d.intensity);
			m_MaxX = MAX(m_MaxX, pt_d.x);
			m_MaxY = MAX(m_MaxY, pt_d.y);
			m_MaxZ = MAX(m_MaxZ, pt_d.z);
			m_iMax = MAX(m_iMax, pt_d.intensity);

			//b.对当前数据进行块集分拨
		    if(pt_d.x < m_zoneStartX || pt_d.x > m_zoneEndX)    	  //当前点位于上(下)一个投影带
			{
				//b.0 当前坐标转换为对应投影带内的坐标值
				double dLongitude = 0.0;
				double dLatitude = 0.0;
				double xNext = 0.0;
				double yNext = 0.0;
				CGeoPositionTran  positionTrans;
				positionTrans.TranslateGauss2Degree(pt_d.x, pt_d.y, m_dCenter, dLongitude, dLatitude);
				positionTrans.TranslateDegree2Gauss(dLongitude, dLatitude, xNext, yNext);

				//b.1 统计坐标分布情况
				U32 selNum = (U32)((xNext - m_fullExtentNext.MinEdge.X) * m_coordStatsNext.xRcp);
				selNum = clamp(selNum, (U32)0, (U32)999);
				m_coordStatsNext.xStepStat[selNum]++;

				selNum = (U32)((yNext - m_fullExtentNext.MinEdge.Y) * m_coordStatsNext.yRcp);
				selNum = clamp(selNum, (U32)0, (U32)999);
				m_coordStatsNext.yStepStat[selNum]++;

				selNum = (U32)((pt_d.z - m_fullExtentNext.MinEdge.Z) * m_coordStatsNext.zRcp);
				selNum = clamp(selNum, (U32)0, (U32)999);
				m_coordStatsNext.zStepStat[selNum]++;
				m_pointNumNext++;

				//b.2 计算点所在块集坐标，点坐标转换为相对所属块集网格左下角的基点坐标
				U32 xNo = (U32)((xNext - m_blockSetStartXNext) / m_stepX);
				U32 yNo = (U32)((yNext - m_blockSetStartYNext) / m_stepY);
				U32 fileNo = yNo * pLevelNext->m_level.numBlocksetX + xNo;
				pt_f.x = (hd::f32)(xNext - m_blockSetStartXNext - xNo * m_stepX);
				pt_f.y = (hd::f32)(yNext - m_blockSetStartYNext - yNo * m_stepY);
				pt_f.z = (hd::f32)(pt_d.z - m_fullExtentNext.MinEdge.Z);
				pt_f.intensity = pt_d.intensity;
				pt_f.c_color = pt_d.c_color;

				//b.3 写入到分拨结果中
				SplitMemoryBuf& memPoints = BsPointsArrayNext[fileNo];
				if(memPoints.m_count >= memPoints.m_vecBuf.size())
				{
					try
					{
						memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE);				
					}
					catch(...)
					{
						::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
						RemoveTempFiles(m_savePath.c_str());
						return false;
					}
				}
				PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
				memPoints.m_count++;
				pPtMem = &pt_f;
			}
			else    //本投影带内点
			{
				//b.1 统计坐标分布情况
				U32 selNum = (U32)((pt_d.x - m_fullExtent.MinEdge.X) * m_coordStats.xRcp);
				selNum = clamp(selNum, (U32)0, (U32)999);
				m_coordStats.xStepStat[selNum]++;

				selNum = (U32)((pt_d.x - m_fullExtent.MinEdge.Y) * m_coordStats.yRcp);
				selNum = clamp(selNum, (U32)0, (U32)999);
				m_coordStats.yStepStat[selNum]++;

				selNum = (U32)((pt_d.z - m_fullExtent.MinEdge.Z) * m_coordStats.zRcp);
				selNum = clamp(selNum, (U32)0, (U32)999);
				m_coordStats.zStepStat[selNum]++;
				m_pointNum++;

				//b.2 计算点所在块集坐标，点坐标转换为相对所属块集网格左下角的基点坐标
				U32 xNo = (U32)((pt_d.x - m_blockSetStartX) / m_stepX);
				U32 yNo = (U32)((pt_d.y - m_blockSetStartY) / m_stepY);
				U32 fileNo = yNo * pLevel->m_level.numBlocksetX + xNo;
				pt_f.x = (hd::f32)(pt_d.x - m_blockSetStartX - xNo * m_stepX);
				pt_f.y = (hd::f32)(pt_d.y - m_blockSetStartY - yNo * m_stepY);
				pt_f.z = (hd::f32)(pt_d.z - m_fullExtent.MinEdge.Z);
				pt_f.intensity = pt_d.intensity;
				pt_f.c_color = pt_d.c_color;

				//b.3 写入到分拨结果中
				SplitMemoryBuf& memPoints = BsPointsArray[fileNo];
				if(memPoints.m_count >= memPoints.m_vecBuf.size())
				{
					try
					{
						memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE);
					}
					catch(...)
					{
						::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
						RemoveTempFiles(m_savePath.c_str());
						return false;
					}
				}
				PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
				memPoints.m_count++;
				pPtMem = &pt_f;
			}
		}

		//c.分拨结果写入到块集缓存文件中
		U32 nProcNum = 0;
		U32 nTotalNum = BsPointsArray.size() + BsPointsArrayNext.size();
		for(auto it = BsPointsArray.begin(); it != BsPointsArray.end(); it++)
		{
			SplitMemoryBuf& bsPoints = it->second;
			U32 nCount = bsPoints.m_count;
			if(nCount <= 0)
			{
				continue;
			}

			CHdBox3df&  blocksetBox = m_BlocksetFiles[it->first].box;

			sprintf_s(dir, "%s\\level0\\blockset_%04d.tmp", m_savePath.c_str(), it->first);
			HANDLE pFile = CreateFile(dir, 
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
			if(pFile != NULL)
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

				LARGE_INTEGER li1, li2;
				li1.HighPart = 0;
				li1.LowPart = 0;
				li2.HighPart = 0;
				li2.LowPart = 0;
				SetFilePointerEx(pFile, li1, &li2, FILE_END);

				int k = 0;
				DWORD numWrite;

				if (m_bResampleZero)
				{
					// 条件写入
					for (U32 m = 0; m < simpleSize; m++)
					{
						int nTmpIndex = *(vecTargetIndex._Myfirst() + m);
						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + nTmpIndex);
				
						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

						*(pPtBuf4Write + k) = *pPt;
						k++;

						if (k >= wSize)
						{
							::WriteFile(pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
							k = 0;
						}
					}
				}
				else
				{
					for(U32 m = 0; m < nCount; m++)
					{
						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + m);
						*(pPtBuf4Write + k) = *pPt;
						k++;

						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

						if(k >= wSize)
						{
							::WriteFile(pFile, pPtBuf4Write, wSize * sizeof(PointXYZIPRGBA), &numWrite, NULL);
							k = 0;
						}
					}
				}

				if ( k > 0)
				{
					DWORD dwResult;
					::WriteFile(pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);
				}

				m_BlocksetFiles[it->first].path = dir;
				if (m_bResampleZero)
				{
					m_BlocksetFiles[it->first].numPoint += simpleSize;
				}
				else
				{
					m_BlocksetFiles[it->first].numPoint += nCount;
				}
			}

			if(pFile)
			{
				CloseHandle(pFile);
				pFile = NULL;
			}

			if(processCallback != NULL)
			{
				nProcNum++;
				processCallback(nProcNum*1.0f/nTotalNum, HDSCENE_IDS_BLOCKSET_SEGMENTATION);
			}
		}

		for(auto it_next = BsPointsArrayNext.begin(); it_next != BsPointsArrayNext.end(); it_next++)
		{
			SplitMemoryBuf& bsPoints = it_next->second;
			U32 nCount = bsPoints.m_count;
			if(nCount <= 0)
			{
				continue;
			}

			CHdBox3df&  blocksetBox = m_BlocksetFilesNext[it_next->first].box;

			sprintf_s(dir, "%s\\level0\\blocksetNext_%04d.tmp", m_savePath.c_str(), it_next->first);
			HANDLE pFile = CreateFile(dir, 
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
			if(pFile != NULL)
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

				LARGE_INTEGER li1, li2;
				li1.HighPart = 0;
				li1.LowPart = 0;
				li2.HighPart = 0;
				li2.LowPart = 0;
				SetFilePointerEx(pFile, li1, &li2, FILE_END);

				int k = 0;
				DWORD numWrite;

				if (m_bResampleZero)
				{
					// 条件写入
					for (U32 m = 0; m < simpleSize; m++)
					{
						int nTmpIndex = *(vecTargetIndex._Myfirst() + m);
						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + nTmpIndex);
		
						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

						*(pPtBuf4WriteNext + k) = *pPt;
						k++;

						if (k >= wSize)
						{
							::WriteFile(pFile, pPtBuf4WriteNext, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
							k = 0;
						}
					}
				}
				else
				{
					for (U32 m = 0; m < nCount; m++)
					{
						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + m);
				
						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

						*(pPtBuf4WriteNext + k) = *pPt;
						k++;

						if (k >= wSize)
						{
							::WriteFile(pFile, pPtBuf4WriteNext, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
							k = 0;
						}
					}
				}

				if ( k > 0)
				{
					DWORD dwResult;
					::WriteFile (pFile, pPtBuf4WriteNext, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);
				}

				m_BlocksetFilesNext[it_next->first].path = dir;
				if (m_bResampleZero)
				{
					m_BlocksetFilesNext[it_next->first].numPoint += simpleSize;
				}
				else
				{
					m_BlocksetFilesNext[it_next->first].numPoint += nCount;
				}
			}

			if(pFile)
			{
				CloseHandle(pFile);
				pFile = NULL;
			}

			if(processCallback != NULL)
			{
				nProcNum++;
				processCallback(nProcNum*1.0f/nTotalNum, HDSCENE_IDS_BLOCKSET_SEGMENTATION);
			}
		}

		if(pPtBuf4Write != NULL)
		{
			delete[] pPtBuf4Write;
			pPtBuf4Write = NULL;
		}

		if(pPtBuf4WriteNext != NULL)
		{
			delete[] pPtBuf4WriteNext;
			pPtBuf4WriteNext = NULL;
		}

		if(processCallback != NULL)
		{
			processCallback(1.0, HDSCENE_IDS_FINISH_BLOCKSET_SEGMENTATION);
		}

		//d.更新每个块集fileinfo的包围盒信息
		for(auto it = m_BlocksetFiles.begin(); it != m_BlocksetFiles.end(); it++)
		{
			U32 bsIndex = it->first;
			U32 yNo = bsIndex / pLevel->m_level.numBlocksetX;
			U32 xNo = bsIndex - yNo * pLevel->m_level.numBlocksetX;
			BlockSetFileInfo& BsInfo = it->second;
			BsInfo.box.MinEdge.X = xNo * m_stepX;
			BsInfo.box.MinEdge.Y = yNo * m_stepY;
			BsInfo.box.MaxEdge.X = (xNo+1) * m_stepX;
			BsInfo.box.MaxEdge.Y = (yNo+1) * m_stepY;
		}

		for(auto it_next = m_BlocksetFilesNext.begin(); it_next != m_BlocksetFilesNext.end(); it_next++)
		{
			U32 bsIndex = it_next->first;
			U32 yNo = bsIndex / pLevelNext->m_level.numBlocksetX;
			U32 xNo = bsIndex - yNo * pLevelNext->m_level.numBlocksetX;
			BlockSetFileInfo& BsInfo = it_next->second;
			BsInfo.box.MinEdge.X = xNo * m_stepX;
			BsInfo.box.MinEdge.Y = yNo * m_stepY;
			BsInfo.box.MaxEdge.X = (xNo+1) * m_stepX;
			BsInfo.box.MaxEdge.Y = (yNo+1) * m_stepY;
		}

		return true;
	}

	bool CHlzMemCloudBuilder::AppendData(const vector<PointXYZIPRGBA_D>& points)
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

		//创建第0层缓存目录
		char dir[MAX_PATH] = {0};
		sprintf_s(dir, "%s\\level0", m_savePath.c_str());
		if(_access(dir, 00) != 0)
		{
			_mkdir(dir);
		}

		CHdLevel* pLevel = m_pCoreData->GetLevelRec(0);
		if(pLevel == NULL)
		{
			pLevel = new CHdLevel;
			pLevel->m_levelNo = 0;
			m_pCoreData->AddLevelRec(pLevel);
		}

		if(processCallback != NULL)
		{
			processCallback(0.0, HDSCENE_IDS_BEGIN_BLOCKSET_SEGMENTATION);
		}

		int wSize = 50000;
		PointXYZIPRGBA* pPtBuf4Write = new PointXYZIPRGBA[wSize];
		map<U32, SplitMemoryBuf>    BsPointsArray;

		U32 pointNum = points.size();
		std::vector<PointXYZIPRGBA> f32Points;
		f32Points.resize(pointNum);
		for(U32 i = 0; i < pointNum; i++)
		{
			PointXYZIPRGBA_D& pt_d = *(points._Myfirst() + i);
			PointXYZIPRGBA& pt_f = *(f32Points._Myfirst() + i);
			if(!pt_d.isValid())
				continue;

			//a.统计实际空间范围和强度范围
			m_MinX = MIN(m_MinX, pt_d.x);
			m_MinY = MIN(m_MinY, pt_d.y);
			m_MinZ = MIN(m_MinZ, pt_d.z);
			m_iMin = MIN(m_iMin, pt_d.intensity);
			m_MaxX = MAX(m_MaxX, pt_d.x);
			m_MaxY = MAX(m_MaxY, pt_d.y);
			m_MaxZ = MAX(m_MaxZ, pt_d.z);
			m_iMax = MAX(m_iMax, pt_d.intensity);

			//b.1 统计坐标分布情况
			U32 selNum = (U32)((pt_d.x - m_fullExtent.MinEdge.X) * m_coordStats.xRcp);
			selNum = clamp(selNum, (U32)0, (U32)999);
			m_coordStats.xStepStat[selNum]++;

			selNum = (U32)((pt_d.x - m_fullExtent.MinEdge.Y) * m_coordStats.yRcp);
			selNum = clamp(selNum, (U32)0, (U32)999);
			m_coordStats.yStepStat[selNum]++;

			selNum = (U32)((pt_d.z - m_fullExtent.MinEdge.Z) * m_coordStats.zRcp);
			selNum = clamp(selNum, (U32)0, (U32)999);
			m_coordStats.zStepStat[selNum]++;
			m_pointNum++;

			//b.2 计算点所在块集坐标，点坐标转换为相对所属块集网格左下角的基点坐标
			U32 xNo = (U32)((pt_d.x - m_blockSetStartX) / m_stepX);
			U32 yNo = (U32)((pt_d.y - m_blockSetStartY) / m_stepY);
			U32 fileNo = yNo * pLevel->m_level.numBlocksetX + xNo;
			pt_f.x = (hd::f32)(pt_d.x - m_blockSetStartX - xNo * m_stepX);
			pt_f.y = (hd::f32)(pt_d.y - m_blockSetStartY - yNo * m_stepY);
			pt_f.z = (hd::f32)(pt_d.z - m_fullExtent.MinEdge.Z);
			pt_f.intensity = pt_d.intensity;
			pt_f.c_color = pt_d.c_color;

			//b.3 写入到分拨结果中
			SplitMemoryBuf& memPoints = BsPointsArray[fileNo];
			if(memPoints.m_count >= memPoints.m_vecBuf.size())
			{
				try
				{
					memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE);
				}
				catch(...)
				{
					::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
					RemoveTempFiles(m_savePath.c_str());
					return false;
				}
			}
			PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst() + memPoints.m_count);
			memPoints.m_count++;
			pPtMem = &pt_f;
		}

		//c.分拨结果写入到块集缓存文件中
		U32 nProcNum = 0;
		U32 nTotalNum = BsPointsArray.size();
		for(auto it = BsPointsArray.begin(); it != BsPointsArray.end(); it++)
		{
			SplitMemoryBuf& bsPoints = it->second;
			U32 nCount = bsPoints.m_count;
			if(nCount <= 0)
			{
				continue;
			}

			CHdBox3df&  blocksetBox = m_BlocksetFiles[it->first].box;

			sprintf_s(dir, "%s\\level0\\blockset_%04d.tmp", m_savePath.c_str(), it->first);
			HANDLE pFile = CreateFile(dir, 
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
			if(pFile != NULL)
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

				LARGE_INTEGER li1, li2;
				li1.HighPart = 0;
				li1.LowPart = 0;
				li2.HighPart = 0;
				li2.LowPart = 0;
				SetFilePointerEx(pFile, li1, &li2, FILE_END);

				int k = 0;
				DWORD numWrite;

				if (m_bResampleZero)
				{
					// 条件写入
					for (U32 m = 0; m < simpleSize; m++)
					{
						int nTmpIndex = *(vecTargetIndex._Myfirst() + m);
						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + nTmpIndex);

						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

						*(pPtBuf4Write + k) = *pPt;
						k++;

						if (k >= wSize)
						{
							::WriteFile(pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
							k = 0;
						}
					}
				}
				else
				{
					for(U32 m = 0; m < nCount; m++)
					{
						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst() + m);
						*(pPtBuf4Write + k) = *pPt;
						k++;

						blocksetBox.MinEdge.Z = min(pPt->z, blocksetBox.MinEdge.Z);
						blocksetBox.MaxEdge.Z = max(pPt->z, blocksetBox.MaxEdge.Z);

						if(k >= wSize)
						{
							::WriteFile(pFile, pPtBuf4Write, wSize * sizeof(PointXYZIPRGBA), &numWrite, NULL);
							k = 0;
						}
					}
				}

				if ( k > 0)
				{
					DWORD dwResult;
					::WriteFile(pFile, pPtBuf4Write, sizeof(PointXYZIPRGBA) * k, &dwResult, NULL);
				}

				m_BlocksetFiles[it->first].path = dir;
				if (m_bResampleZero)
				{
					m_BlocksetFiles[it->first].numPoint += simpleSize;
				}
				else
				{
					m_BlocksetFiles[it->first].numPoint += nCount;
				}
			}

			if(pFile)
			{
				CloseHandle(pFile);
				pFile = NULL;
			}

			if(processCallback != NULL)
			{
				nProcNum++;
				processCallback(nProcNum*1.0f/nTotalNum, HDSCENE_IDS_BLOCKSET_SEGMENTATION);
			}
		}

		if(pPtBuf4Write != NULL)
		{
			delete[] pPtBuf4Write;
			pPtBuf4Write = NULL;
		}

		if(processCallback != NULL)
		{
			processCallback(1.0, HDSCENE_IDS_FINISH_BLOCKSET_SEGMENTATION);
		}

		//d.更新每个块集fileinfo的包围盒信息
		for(auto it = m_BlocksetFiles.begin(); it != m_BlocksetFiles.end(); it++)
		{
			U32 bsIndex = it->first;
			U32 yNo = bsIndex / pLevel->m_level.numBlocksetX;
			U32 xNo = bsIndex - yNo * pLevel->m_level.numBlocksetX;
			BlockSetFileInfo& BsInfo = it->second;
			BsInfo.box.MinEdge.X = xNo * m_stepX;
			BsInfo.box.MinEdge.Y = yNo * m_stepY;
			BsInfo.box.MaxEdge.X = (xNo+1) * m_stepX;
			BsInfo.box.MaxEdge.Y = (yNo+1) * m_stepY;
		}

		return true;
	}

	bool CHlzMemCloudBuilder::Flush()
	{
		//1.根据坐标分布情况，剔除边缘少数点，调整按坐标值渲染范围
		int selMax = 0;
		int selMin = 0;
		U64 selNum = 0;
		CHdVector3dd szBox = m_fullExtent.getExtent();
		m_pCoreData->m_hlzHeader.number_of_point_records = m_pointNum;

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

		m_pCoreData->m_hlzHeader.renderMinZ = (F32)(selMin * (szBox.Z) / 1000.0f);
		m_pCoreData->m_hlzHeader.renderMaxZ = (F32)(selMax * (szBox.Z) / 1000.0f);
		//强度范围分布没有统计，暂时使用完整范围
		m_pCoreData->m_hlzHeader.intensityMin = 0;
		m_pCoreData->m_hlzHeader.intensityMax = 255;

		if(m_pCoreDataNext != NULL)
		{
			int selMaxNext = 0;
			int selMinNext = 0;
			U64 selNumNext = 0;
			CHdVector3dd szBoxNext = m_fullExtentNext.getExtent();
			m_pCoreDataNext->m_hlzHeader.number_of_point_records = m_pointNumNext;

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

			//块集包围盒退化为外包矩形（z维度丢失）时,手动将z坐标向外扩展0.001m，避免包围盒退化。     袁亮   20161013
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

			m_pCoreDataNext->m_hlzHeader.renderMinZ = (F32)(selMinNext * (szBoxNext.Z) / 1000.0f);
			m_pCoreDataNext->m_hlzHeader.renderMaxZ = (F32)(selMaxNext * (szBoxNext.Z) / 1000.0f);
			//强度范围分布没有统计，暂时使用完整范围
	        m_pCoreDataNext->m_hlzHeader.intensityMin = 0;
			m_pCoreDataNext->m_hlzHeader.intensityMax = 255;
		}

		// 第0层日志信息
		if(DEBUG_LOG)
		{
			char strLv[512];
			sprintf_s(strLv,"level%d:\n",0);
			m_pHlzWrite->WriteLogFile(strLv);
			m_pHlzWrite->WriteLogFile("类型,点数,范围\n");
		}

		// 写入第0层
		CHdLevel* pLevel = m_pCoreData->GetLevelRec(0);
		writeLevel0Data(pLevel);
		m_pHlzWrite->m_header.number_of_point_records = pLevel->m_level.pointNum;	

		U8 nLevelNo = 1;
		CHdLevel* levelPre = pLevel;
		while (levelPre->m_level.pointNum > 1000000)
		{
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

			nLevelNo++;
			levelPre = pNextLevel;

		}

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

		if(m_pCoreDataNext != NULL)
		{
			CHdLevel* pLevelNext = m_pCoreDataNext->GetLevelRec(0);
			writeLevel0DataNext(pLevelNext);
			m_pNextHlzWrite->m_header.number_of_point_records = pLevelNext->m_level.pointNum;

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
				writeNextLevelDataNextZone(levelPreNextZone, nLevelNoNextZone, pNextLevel);

				nLevelNoNextZone++;
				levelPreNextZone = pNextLevel;
			}

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

		}

		RemoveTempFiles(m_savePath.c_str());

		if (processCallback != NULL)
		{
			processCallback(1.0, HDSCENE_IDS_CONVERT_FINISH);
		}

		return true;
	}

	void CHlzMemCloudBuilder::CloseFile()
	{
		if(m_pHlzWrite != NULL)
		{
			delete m_pHlzWrite;
			m_pHlzWrite = NULL;
		}

		if(m_pNextHlzWrite != NULL)
		{
			delete m_pNextHlzWrite;
			m_pNextHlzWrite = NULL;
		}

		if(m_pCoreData != NULL)
		{
			m_pCoreData->Close();
			delete m_pCoreData;
			m_pCoreData = NULL;
		}

		if(m_pCoreDataNext != NULL)
		{
			m_pCoreDataNext->Close();
			delete m_pCoreDataNext;
			m_pCoreDataNext = NULL;
		}
	}

	bool CHlzMemCloudBuilder::RemoveTempFiles(const char* path)
	{
		if(_access(path, 00) != 0)
		{
			return false;
		}

		char buffer[MAX_PATH] = {0};
		sprintf_s(buffer, "%s\\*.*", path);
		WIN32_FIND_DATAA fd;
		HANDLE hFind = ::FindFirstFileA(buffer, &fd);
		if(hFind == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		do 
		{
			sprintf_s(buffer, "%s\\%s", path, fd.cFileName);
			if(strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
			{
				::RemoveDirectoryA(buffer);
			}
			else if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				RemoveTempFiles(buffer);
			}
			else
			{
				::DeleteFileA(buffer);
			}

		} while (::FindNextFileA(hFind, &fd));

		::FindClose(hFind);
		RemoveDirectoryA(path);

		return true;
	}

	bool CHlzMemCloudBuilder::RemoveBSFiles(std::map<U32, BlockSetFileInfo>& bsFiles)
	{
		for (map<U32,BlockSetFileInfo>::iterator it = m_BlocksetFiles.begin();
			it != m_BlocksetFiles.end();it++)
		{	
			remove((it->second).path.c_str());
		}
		return TRUE;
	}

	void CHlzMemCloudBuilder::getPtBlockIndex(PointXYZIPRGBA& pt, const CHdVector3df& center, U8& index)
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

	void CHlzMemCloudBuilder::getBlockBox(const CHdVector3df& mid, const CHdVector3df& halfSize, CHdBox3df& box, U16 index)
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

	void CHlzMemCloudBuilder::getPreLevelBlockset(U16 xNo, U16 yNo, CHdLevel* pPreLevel, std::vector<BlockSetFileInfo>& preBlocksetFiles)
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

	void CHlzMemCloudBuilder::getPreLevelBlocksetNext(U16 xNo, U16 yNo, CHdLevel* pPreLevel, std::vector<BlockSetFileInfo>& preBlocksetFiles)
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

	void CHlzMemCloudBuilder::clearBlocksetData()
	{
		if (m_blsDataBuf.size()>0)
		{
			m_blsDataBuf.clear();
			m_blsDataBuf.swap(vector<PointXYZIPRGBA> ());
		}
		if(m_pBlockData.size() > 0)
		{
			std:: map<string, vector<PointXYZIPRGBA *> >::iterator it;
			for (it = m_pBlockData.begin(); it != m_pBlockData.end(); it ++)
			{
				it->second.clear();
				it->second.swap(std::vector<PointXYZIPRGBA *>());    
			}
			m_pBlockData.clear();
		}
	}

	void CHlzMemCloudBuilder::clearParcelData()
	{
		if (m_ParcelData.size() > 0)
		{
			std::map<string, vector<PointXYZIPRGBA*> >::iterator it;
			for (it = m_ParcelData.begin(); it != m_ParcelData.end(); it++)
			{
				it->second.clear();
				it->second.swap(std::vector<PointXYZIPRGBA*>());
			}
			m_ParcelData.clear();
		}
	}

	void CHlzMemCloudBuilder::clearSplitMemBuf(vector<SplitMemoryBuf> & vecMemBuf)
	{
		for (vector<SplitMemoryBuf>::iterator it = vecMemBuf.begin(); 
			it != vecMemBuf.end(); it ++)
		{
			it->clear();
		}
		vecMemBuf.clear();
		vecMemBuf.swap(vector<SplitMemoryBuf>());
	}

	bool CHlzMemCloudBuilder::SplitBlockset2Block_Buffer(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks)
	{
		FILE* pFile = fopen(procBlockSet.path.c_str(), "rb");
		if (pFile == NULL)
		{
			return false;
		}

		fseek(pFile, 0, SEEK_SET);// 移到文件头
		
		// 创建快文件标识ID，为了保证ID的惟一性，使用存储目录作为ID
		std::string dir = procBlockSet.path;
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
			memcpy(iter_block->second._Myfirst() + preSize, blkPoints.m_vecBuf._Myfirst(), nCount*sizeof(PointXYZIPRGBA*));
		}

		fclose(pFile);
		pFile = NULL;

		return true;
	}

	bool CHlzMemCloudBuilder::SplitBlockset2Block_BufferNext(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks)//
	{
		FILE* pFile = fopen(procBlockSet.path.c_str(), "rb");
		if (pFile == NULL)
		{
			return false;
		}

		fseek(pFile, 0, SEEK_SET);// 移到文件头
	
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
			memcpy(iter_block->second._Myfirst() + preSize, blkPoints.m_vecBuf._Myfirst(), nCount*sizeof(PointXYZIPRGBA*));
		}

		fclose(pFile);
		pFile = NULL;

		return true;
	}

	bool CHlzMemCloudBuilder::SplitSbkFile2Parcel_BufferNext(const CHdBox3df& blockBox, const string strSbkFile, std::vector<ParcelFileInfo>& vecParcelInfo)
	{
		// 打开分块文件
		FILE* pBlockFile = fopen(strSbkFile.c_str(), "rb");

		if (!pBlockFile) // 打开失败返回
		{
			return false;
		}

		string savePath = strSbkFile.substr(0, strSbkFile.find_last_of('.'));
		// 读取子块中的所有点
		m_pNextHlzWrite->ReadAllPts(pBlockFile, m_blsDataBuf);
		if (pBlockFile)
		{
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

	bool CHlzMemCloudBuilder::SplitBlock2Parcel_Buffer(const CHdBox3df& blockBox, const char* strBlockID, const char* strPacelID, std::vector<ParcelFileInfo>& arrayParcels)
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

			sprintf_s(parcelDataID, "%s\\parcel_%d.tmp", strPacelID, i);
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

	bool CHlzMemCloudBuilder::SplitParcel2Parcel_Buffer(const CHdBox3df& blockBox, const char* srcParcelID, const string destParcelID, std::vector<ParcelFileInfo>& arrayParcels)
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

	bool CHlzMemCloudBuilder::SplitBlockFiles(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks)
	{
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
				
				char* blockFilePath = new char[dir.length()+13];
				sprintf_s(blockFilePath, dir.length()+13, "%s\\block_%d.tmp", dir.c_str(), i);
				
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
					*(pPtBuf4Write + k++) = *pPt;
					if (k == wSize)
					{
						DWORD numWrite;
						::WriteFile(pBlockFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
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
					::WriteFile (pBlockFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*leftnum, &dwResult, NULL);
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

	bool CHlzMemCloudBuilder::SplitBlockFilesNext(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks)
	{
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
	
				char* blockFilePath = new char[dir.length()+13];
				sprintf_s(blockFilePath, dir.length()+13, "%s\\block_%d.tmp", dir.c_str(), i);

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
					*(pPtBuf4Write + k++) = *pPt;
					if (k == wSize)
					{
						DWORD numWrite;
						::WriteFile(pBlockFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
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
					::WriteFile (pBlockFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*leftnum, &dwResult, NULL);
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

	bool CHlzMemCloudBuilder::SplitBlock2Parts(const CHdBox3df& blockBox, const string strBlockFile, std::vector<subBlockFileInfo>& subBlkInfo)
	{
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
					*(pPtBuf4Write + k++) = *pPt;
					if (k == wSize)
					{
						DWORD numWrite;
						::WriteFile(pSubBlkFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
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
					::WriteFile (pSubBlkFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*leftnum, &dwResult, NULL);
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

	bool CHlzMemCloudBuilder::SplitBlock2PartsNext(const CHdBox3df& blockBox, const string strBlockFile, std::vector<subBlockFileInfo>& subBlkInfo)
	{
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

		while (m_pNextHlzWrite->ReadPtsBySize(pBlockFile, bufferPts, MAX_POINT_NUM) > 0)
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
						::WriteFile(pSubBlkFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*wSize, &numWrite, NULL);
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
					::WriteFile (pSubBlkFile, pPtBuf4Write, sizeof(PointXYZIPRGBA)*leftnum, &dwResult, NULL);
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

	bool CHlzMemCloudBuilder::SplitSbkFile2Parcel_Buffer(const CHdBox3df& blockBox, const string strSbkFile, std::vector<ParcelFileInfo>& vecParcelInfo)
	{
		// 打开分块文件
		FILE* pBlockFile = fopen(strSbkFile.c_str(), "rb");

		if (!pBlockFile) // 打开失败返回
		{
			return false;
		}

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

	void CHlzMemCloudBuilder::writeLevel0Data(CHdLevel* pLevel0)
	{
		// 写入第0层的索引信息
		//pLevel0->m_level.m_numBlockset = m_BlocksetFiles.size();			// 有效块集个数
		m_pHlzWrite->WriteLevelInfo(pLevel0->m_level);

		if(processCallback != NULL)
		{
			processCallback(0.0, HDSCENE_IDS_BEGIN_WRITE_ZERO_LEVEL);
		}

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

		if(processCallback != NULL)
		{
			processCallback(1.0, HDSCENE_IDS_FINISH_WRITE_ZERO_LEVEL);
		}
	}

	void CHlzMemCloudBuilder::writeLevel0DataNext(CHdLevel* pLevel0)
	{
		// 写入第0层的索引信息
		//pLevel0->m_level.m_numBlockset = m_BlocksetFiles.size();			// 有效块集个数
		m_pNextHlzWrite->WriteLevelInfo(pLevel0->m_level);

		if(processCallback != NULL)
		{
			processCallback(0.0, HDSCENE_IDS_BEGIN_WRITE_ZERO_LEVEL);
		}

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

							m_pNextHlzWrite->WriteBlockData(pCurBlock->m_block, pPtXYZ, pPtInten, pPtColor, pCurBlock->m_block.numPoint);
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

		if(processCallback != NULL)
		{
			processCallback(1.0, HDSCENE_IDS_FINISH_WRITE_ZERO_LEVEL);
		}
	}

	U64 CHlzMemCloudBuilder::writeNextLevelData(CHdLevel* pPreLevel, U8 nLevelNo, CHdLevel* pCurLevel)
	{
		pCurLevel->m_level.numBlocksetX = (U16)ceil((pPreLevel->m_level.numBlocksetX) / 2.0);
		pCurLevel->m_level.numBlocksetY = (U16)ceil((pPreLevel->m_level.numBlocksetY) / 2.0);
		//pCurLevel->m_level.numBlockset = pCurLevel->m_level.numBlocksetX * pCurLevel->m_level.numBlocksetY;
		pCurLevel->m_level.sizeX = pPreLevel->m_level.sizeX * 2.f;
		pCurLevel->m_level.sizeY = pPreLevel->m_level.sizeY * 2.f;
		m_stepX = pCurLevel->m_level.sizeX;
		m_stepY = pCurLevel->m_level.sizeY;
		pCurLevel->m_level.pointNum = (U32)hd_round32(pPreLevel->m_level.pointNum/4.f);		// 大概的点数

		char msgBuf[100] = {0};
		sprintf_s(msgBuf, "%s%d%s", HDSCENE_IDS_BEGIN_WRITE,nLevelNo,HDSCENE_IDS_LEVEL);
		if(processCallback != NULL)
		{
			processCallback(0.0, msgBuf);
		}

		// 写入当前层索引
		m_pHlzWrite->WriteLevelInfo(pCurLevel->m_level);
		// 置0后精确统计
		pCurLevel->m_level.pointNum = 0;	

		// 当前层切分的块集字典表, 当前层写完后替换m_BlocksetFiles
		std::map<U32, BlockSetFileInfo> curBsFiles;

		int iPreBSCount = m_BlocksetFiles.size();
		int iAgrCount = 0;
		int currentCount = 0;
	
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
					sprintf_s(msgBuf, "%s%d%s", HDSCENE_IDS_WRITING,nLevelNo,HDSCENE_IDS_LEVEL);
					processCallback((float)nBsIndex / (pCurLevel->m_level.numBlocksetX * pCurLevel->m_level.numBlocksetY), msgBuf);
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
				m_pHlzWrite->WriteNextLevelBlocksetByMidCloud(BsFileName, preBlocksetFiles, curBlockset->m_blockSet,pCurLevel,curBlockset->m_nBlockSetNo, nLevelNo);
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

					LARGE_INTEGER li1, li2;
					li1.HighPart = 0;
					li1.LowPart = 0;
					li2.HighPart = 0;
					li2.LowPart = 0;
					SetFilePointerEx(pBsFile, li1, &li2, FILE_BEGIN);

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
									U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &pclRefPt, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
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

		if(processCallback != NULL)
		{
			sprintf_s(msgBuf, "%s%d%s", HDSCENE_IDS_WRITE_NO,nLevelNo,HDSCENE_IDS_LEVEL_COMPLETE);
			processCallback(1.0, msgBuf);
		}

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

	U64 CHlzMemCloudBuilder::writeNextLevelDataNextZone(CHdLevel* pPreLevel, U8 nLevelNo, CHdLevel* pCurLevel)
	{
		pCurLevel->m_level.numBlocksetX = (U16)ceil((pPreLevel->m_level.numBlocksetX) / 2.0);
		pCurLevel->m_level.numBlocksetY = (U16)ceil((pPreLevel->m_level.numBlocksetY) / 2.0);
		//pCurLevel->m_level.numBlockset = pCurLevel->m_level.numBlocksetX * pCurLevel->m_level.numBlocksetY;
		pCurLevel->m_level.sizeX = pPreLevel->m_level.sizeX * 2.f;
		pCurLevel->m_level.sizeY = pPreLevel->m_level.sizeY * 2.f;
		m_stepX = pCurLevel->m_level.sizeX;
		m_stepY = pCurLevel->m_level.sizeY;
		pCurLevel->m_level.pointNum = (U32)hd_round32(pPreLevel->m_level.pointNum/4.f);		// 大概的点数

		char msgBuf[100] = {0};
		sprintf_s(msgBuf, "%s%d%s", HDSCENE_IDS_BEGIN_WRITE,nLevelNo,HDSCENE_IDS_LEVEL);
		if(processCallback != NULL)
		{
			processCallback(0.0, msgBuf);
		}

		// 写入当前层索引
		m_pNextHlzWrite->WriteLevelInfo(pCurLevel->m_level);
		// 置0后精确统计
		pCurLevel->m_level.pointNum = 0;	

		// 当前层切分的块集字典表, 当前层写完后替换m_BlocksetFilesNext
		std::map<U32, BlockSetFileInfo> curBsFiles;

		int iPreBSCount = m_BlocksetFilesNext.size();
		int iAgrCount = 0;
		int currentCount = 0;
	
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
					sprintf_s(msgBuf, "%s%d%s", HDSCENE_IDS_WRITING,nLevelNo,HDSCENE_IDS_LEVEL);
					processCallback((float)nBsIndex / (pCurLevel->m_level.numBlocksetX * pCurLevel->m_level.numBlocksetY), msgBuf);
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
				//m_pNextHlzWrite->WriteNextLevelBlockset(BsFileName, preBlocksetFiles, curBlockset->m_blockSet,pCurLevel,curBlockset->m_nBlockSetNo);
				m_pHlzWrite->WriteNextLevelBlocksetByMidCloud(BsFileName, preBlocksetFiles, curBlockset->m_blockSet,pCurLevel,curBlockset->m_nBlockSetNo, nLevelNo);
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
				
					LARGE_INTEGER li1, li2;
					li1.HighPart = 0;
					li1.LowPart = 0;
					li2.HighPart = 0;
					li2.LowPart = 0;
					SetFilePointerEx(pBsFile, li1, &li2, FILE_BEGIN);
					
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
									U32 cntWrite = pPtArray2PtBlock(iter_parcel->second._Myfirst(), parcel->m_parcel.numPoint, &pclRefPt, &pPtXYZ, &pPtInten, &pPtColor, BlockSetOffsetX,BlockSetOffsetY, m_iMin, m_iMax);
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

		if(processCallback != NULL)
		{
			sprintf_s(msgBuf, "%s%d%s", HDSCENE_IDS_WRITE_NO,nLevelNo,HDSCENE_IDS_LEVEL_COMPLETE);
			processCallback(1.0, msgBuf);
		}

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
}

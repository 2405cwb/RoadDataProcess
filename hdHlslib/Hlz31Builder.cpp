//#include "Hlz31Builder.h"
//#include "IHLSReader.h"
//#include "HdCoreData.h"
//#include "HdLevel31.h"
//#include "HdBlockset31.h"
//#include "HdBlock31.h"
//#include "HdParcel31.h"
//#include "HlzDefs31.h"
//#include "inc\lasreader.hpp"
//#include "..\hdcore\hdBlkArray.h"
//#include "..\hdCommon\hdSceneStr.h"
//#include "HLSReadOpener.h"
//#include "PtEncoder.h"
//#include <ppl.h>
//#include <concurrent_vector.h>
//
//#include <direct.h>
//#include <time.h>
//#include <algorithm>
//
//#ifdef _DEBUG
//#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
//#endif
//
//const U8 BlkIdx2BlkNo[][3] = 
//{
//	{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0},
//	{0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1} 
//};
//
//#define SAFE_DELETE_ARY(ary)  { try{ delete[] ary;} catch(...) { ::MessageBox(NULL, "内存释放失败!", NULL, MB_OK); } ary = NULL; }
//#define SAFE_DELETE_PTR(ptr)   { try{ delete ptr;} catch(...) { ::MessageBox(NULL, "内存释放失败!", NULL, MB_OK);} ptr = NULL; }
//
//namespace hd
//{
//	template<class T>
//	CHlz31Builder<T>::CHlz31Builder(void)
//		:m_gridLevel(4), m_ptNum(0), m_iMin(0xFFFF), m_iMax(0), 
//	     m_pHlzWrite(NULL), m_callback(NULL), m_logFile(NULL)
//	{
//		GetConvertConfig();
//	}
//
//	template<class T>
//	CHlz31Builder<T>::~CHlz31Builder(void)
//	{
//		Close();
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::buildHlz(const char* savePath, bool isCompress, void (*processCallback)(float, const char*))
//	{
//		if(m_hlsFileList.empty())
//		{
//			if(processCallback != NULL)
//			{
//				processCallback(0.0, "转换文件列表为空，转换失败!");
//			}
//			return false;
//		}
//
//		char msg[MAX_PATH] = {0};
//
//		char drive[256] = {0};// 磁盘
//		char dir[256] = {0};// 文件夹
//		char filename[256] = {0};// 文件名
//		char ext[256] = {0};// 文件格式
//		char path[256] = {0};// 文件路径
//		
//		_splitpath(savePath, drive, dir, filename, ext);
//		m_hlzFileName = filename;
//		m_hlzFileName += ext;
//
//		if (strcmp(ext,".hlz") != 0)
//		{
//			if (processCallback != NULL)
//			{
//				processCallback(0.0, "输出文件后缀名不正确，转换失败!");
//			}
//			return false;
//		}
//
//		m_logFile = fopen("d:\\dbg.txt", "wt");
//
//		_makepath(path, drive, dir, NULL, NULL);
//		m_savePath = path;
//		m_savePath += "\\__temp_to_hlz";
//		_mkdir(m_savePath.c_str());
//
//		SAFE_DELETE_PTR(m_pHlzWrite);
//		m_pHlzWrite = new CHLZ31Writer;
//		m_pHlzWrite->m_header.isCompress = isCompress;
//		
//		m_callback = processCallback;
//
//		// 统计强度范围，坐标范围
//		U16 minInt = 0xffff, maxInt = 0;
//		double dMinX = F64_MAX,dMinY = F64_MAX,dMinZ = F64_MAX;
//		double dMaxX = F64_MIN,dMaxY = F64_MIN,dMaxZ = F64_MIN;
//		double minx = F64_MAX, miny = F64_MAX, minz = F64_MAX;
//		double maxx = F64_MIN, maxy = F64_MIN, maxz = F64_MIN;
//		U32 nFileNum = m_hlsFileList.size();
//		for (U32 i=0; i < nFileNum; i++)
//		{
//			const hd::stringc& strPath = m_hlsFileList[i].c_str();
//			hd::stringc strExt = strPath.subString(strPath.size()-3, 3);
//			strExt.make_lower();
//
//			//如果是hls文件
//			if (strExt == "hls")
//			{
//				CHLSReadOpener hlsOpen;
//				IHLSReader* pHlsReader = hlsOpen.Open(strPath.c_str());
//				if (pHlsReader == NULL)
//				{
//					continue;
//				}
//
//				// 统计坐标
//				pHlsReader->m_header.getGlobalExtent(minx,miny,minz,maxx,maxy,maxz);
//				dMinX = MIN(dMinX,minx);
//				dMinY = MIN(dMinY,miny);
//				dMinZ = MIN(dMinZ,minz);
//
//				dMaxX = MAX(dMaxX,maxx);
//				dMaxY = MAX(dMaxY,maxy);
//				dMaxZ = MAX(dMaxZ,maxz);
//
//				// 统计强度
//				StatIntensity(pHlsReader,minInt,maxInt);
//				m_iMin = MIN(m_iMin,minInt);
//				m_iMax = MAX(m_iMax,maxInt);
//
//				if (dMinX == dMaxX && dMinY == dMaxY && dMinZ == dMaxZ )
//				{
//					CalHlsExtent(pHlsReader, dMinX, dMaxX, dMinY, dMaxY, dMinZ, dMaxZ);
//				}
//
//				SAFE_DELETE_PTR(pHlsReader);
//			}
//			else if(strExt == "xyz")
//			{
//				//暂不支持
//			}
//			else if(strExt == "las")
//			{
//				// las文件打开对象
//				LASreadOpener lasreadopener;
//
//				// 设置不合并
//				lasreadopener.set_merged(FALSE);
//				lasreadopener.set_populate_header(FALSE);
//
//				// 设置文件名
//				lasreadopener.set_file_name(strPath.c_str());
//				if (!lasreadopener.active())
//				{
//					return false;
//				}
//				// 打开las文件
//				LASreader* lasRead = lasreadopener.open();
//
//				// 统计坐标
//				minx = lasRead->get_min_x();
//				maxx = lasRead->get_max_x();
//				miny = lasRead->get_min_y();
//				maxy = lasRead->get_max_y();
//				minz = lasRead->get_min_z();
//				maxz = lasRead->get_max_z();
//
//				dMinX = MIN(dMinX,minx);
//				dMinY = MIN(dMinY,miny);
//				dMinZ = MIN(dMinZ,minz);
//
//				dMaxX = MAX(dMaxX,maxx);
//				dMaxY = MAX(dMaxY,maxy);
//				dMaxZ = MAX(dMaxZ,maxz);
//
//				// 统计强度
//				m_iMin = MIN(m_iMin,0);
//				m_iMax = MAX(m_iMax,4095);
//
//				SAFE_DELETE_PTR(lasRead);
//			}
//		}
//
//		m_fullExtent.MinEdge.X = dMinX;
//		m_fullExtent.MinEdge.Y = dMinY;
//		m_fullExtent.MinEdge.Z = dMinZ;
//		m_fullExtent.MaxEdge.X = dMaxX;
//		m_fullExtent.MaxEdge.Y = dMaxY;
//		m_fullExtent.MaxEdge.Z = dMaxZ;
//
//		// 文件头信息, 偏移量为点云最小点所在块集的最小点
//		HLZheader& hlzHeader = m_pHlzWrite->m_header;
//		hlzHeader.offsetX = m_fullExtent.MinEdge.X;
//		hlzHeader.offsetY = m_fullExtent.MinEdge.Y;
//		hlzHeader.offsetZ = m_fullExtent.MinEdge.Z;
//		hlzHeader.min_x = 0.0f;
//		hlzHeader.min_y = 0.0f;
//		hlzHeader.min_z = 0.0f;
//		hlzHeader.max_x = (F32)(dMaxX - hlzHeader.offsetX);
//		hlzHeader.max_y = (F32)(dMaxY - hlzHeader.offsetY);
//		hlzHeader.max_z = (F32)(dMaxZ - hlzHeader.offsetZ);
//
//		// 记录更新时间
//		time_t timer;
//		time(&timer);
//		tm* t_tm = localtime(&timer);
//		hlzHeader.file_creation_day = (U16)t_tm->tm_yday + 1;
//		hlzHeader.file_creation_year = (U16)t_tm->tm_year + 1900;
//
//		// 打开文件
//		if(!m_pHlzWrite->Open(savePath))
//		{
//			if (m_callback != NULL)
//			{
//				m_callback(0.0, "转换失败！");
//			}
//
//			return false;
//		}
//
//		// 根据规范规则,进行块集切割
//		if (!SplitInputFiles(m_savePath.c_str()))
//		{
//			if (m_callback != NULL)
//			{
//				m_callback(0.0, "转换失败！");
//			}
//
//			RemoveTempFiles(m_savePath.c_str());
//			return false;
//		}
//
//		U8 levelNo = 0;
//		CHdLevel31* pLevel = NULL;
//		do 
//		{
//			pLevel = new CHdLevel31;
//			pLevel->m_levelNo = levelNo;
//			pLevel->m_level.levelNo = m_gridLevel + levelNo;
//			m_pLevels.insert(make_pair(pLevel->m_levelNo, pLevel));
//			levelNo++;
//		} while (writeLevelData(pLevel) > m_level_point_threshold);
//
//		fflush(m_logFile);
//		fclose(m_logFile);
//
//		hlzHeader.number_of_level = levelNo;
//		m_pHlzWrite->WriteHeader();
//		m_pHlzWrite->WriteIndex(m_pLevels);
//		// 写入索引文件并关闭数据文件
//		m_pHlzWrite->Close();
//
//		// 删除临时文件
//		RemoveTempFiles(m_savePath.c_str());
//
//		if (m_callback != NULL)
//		{
//			m_callback(1.0, "转换完成！");
//		}
//
//		return true;
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::SplitInputFiles(const char* savePath)
//	{
//		int selMax = 0;
//		int selMin = 0;
//		m_coordStats.Clear();
//
//		F64 xRange = m_fullExtent.getExtent().X;
//		F64 yRange = m_fullExtent.getExtent().Y;
//		F64 zRange = m_fullExtent.getExtent().Z;
//
//		m_coordStats.xRcp = (F32)(1000.0f / (xRange));
//		m_coordStats.yRcp = (F32)(1000.0f / (yRange));
//		m_coordStats.zRcp = (F32)(1000.0f / (zRange));
//		m_coordStats.iRcp = (F32)(1000.0f / (m_iMax - m_iMin));
//
//		U32 nFileNum = m_hlsFileList.size();
//		for (U32 i = 0;i< nFileNum; i++)
//		{
//			hd::stringc strExt = m_hlsFileList[i].substr(m_hlsFileList[i].size()-3, 3).c_str();
//			strExt.make_lower();
//
//			//如果是hls文件
//			if (strExt == "hls")
//			{
//				if(!SplitBlockSetFiles(savePath, m_hlsFileList[i]))
//					return false;
//			}
//			else if(strExt == "xyz")
//			{
//				//暂不支持
//			}
//			else if(strExt == "las")
//			{
//				SplitBlockSetLas(savePath, m_hlsFileList[i]);
//			}
//		}
//
//		HLZheader& hlzHeader = m_pHlzWrite->m_header;
//		hlzHeader.number_of_point_records = m_ptNum;
//
//		//调整X坐标分布，将点数较少部分剔除不参与统计，获得新的最大最小X值
//		U32 selNum = 0;
//		CHdVector3dd szBox = m_fullExtent.getExtent();
//		for (int j = 999;j >= 0;j--)
//		{
//			selNum += m_coordStats.xStepStat[j];
//			if (selNum >= m_ptNum * 0.03)
//			{
//				selMax = j;
//				break;
//			}
//		}
//
//		selNum = 0;
//		for (int j = 0;j <= 999;j++)
//		{
//			selNum += m_coordStats.xStepStat[j];
//			if (selNum >= m_ptNum * 0.03)
//			{
//				selMin = j;
//				break;
//			}
//		}
//
//		hlzHeader.renderMinX = (F32)(selMin * (szBox.X) / 1000.0f);
//		hlzHeader.renderMaxX = (F32)(selMax * (szBox.X) / 1000.0f);
//
//		// 调整y坐标分布
//		selNum = 0;
//		for (int j = 999;j >= 0;j--)
//		{
//			selNum += m_coordStats.yStepStat[j];
//			if (selNum >= m_ptNum* 0.03)
//			{
//				selMax = j;
//				break;
//			}
//		}
//
//		selNum = 0;
//		for (int j = 0;j <= 999;j++)
//		{
//			selNum += m_coordStats.yStepStat[j];
//			if (selNum >= m_ptNum * 0.03)
//			{
//				selMin = j;
//				break;
//			}
//		}
//		hlzHeader.renderMinY = (F32)(selMin * (szBox.Y) / 1000.0f);
//		hlzHeader.renderMaxY = (F32)(selMax * (szBox.Y) / 1000.0f);
//
//		// 调整z坐标分布
//		selNum = 0;
//		for (int j = 999;j >= 0;j--)
//		{
//			selNum += m_coordStats.zStepStat[j];
//			if (selNum >= m_ptNum * 0.03)
//			{
//				selMax = j;
//				break;
//			}
//		}
//
//		selNum = 0;
//		for (int j = 0;j <= 999;j++)
//		{
//			selNum += m_coordStats.zStepStat[j];
//			if (selNum >= m_ptNum * 0.03)
//			{
//				selMin = j;
//				break;
//			}
//		}
//		hlzHeader.renderMinZ = (F32)(selMin * (szBox.Z) / 1000.0f);
//		hlzHeader.renderMaxZ = (F32)(selMax * (szBox.Z) / 1000.0f);
//
//		// 调整强度分布
//		selNum = 0;
//		for (int j = 999;j >= 0;j--)
//		{
//			selNum += m_coordStats.intStepStat[j];
//			if (selNum >= m_ptNum * 0.03)
//			{
//				selMax = j;
//				break;
//			}
//		}
//
//		selNum = 0;
//		for (int j = 0;j <= 999;j++)
//		{
//			selNum += m_coordStats.intStepStat[j];
//			if (selNum >= m_ptNum * 0.03)
//			{
//				selMin = j;
//				break;
//			}
//		}
//
//		U32 iRange = m_iMax - m_iMin;
//		U32 iMin,iMax;
//		iMin = (U32)(selMin * (iRange) / 1000.0f);
//		iMax = (U32)(selMax * (iRange) / 1000.0f);
//		hlzHeader.intensityMin = (U8)(iMin * 255 / (F32)iRange);
//		hlzHeader.intensityMax = (U8)(iMax * 255 / (F32)iRange);
//
//		return true;
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::SplitBlockSetFiles(const char* savePath, const std::string& strHlsFile)
//	{
//		CHLSReadOpener hlsOpen;
//		IHLSReader* pHlsReader = hlsOpen.Open(strHlsFile.c_str());
//		if (pHlsReader == NULL)
//		{
//			return false;
//		}
//
//		std::string strInfo = "正在切割";
//		int nPos = strHlsFile.find_last_of('\\');
//		strInfo += strHlsFile.substr(nPos + 1);
//
//		// 进度条开始
//		if (m_callback != NULL)
//		{
//			m_callback(0.0f, strInfo.c_str());
//		}
//
//		// 创建第0层临时文件目录
//		char dir[MAX_PATH] = {0};
//		sprintf_s(dir, "%s\\level0", savePath);
//		I32 ret = _mkdir(dir);
//
//		char filePath[MAX_PATH] = {0};
//
//		//使用压缩点数据结构  袁亮  20161101
//		int wSize = 50000;
//		T* pPtBuf4Write = new T[wSize];
//
//		I32 gridSize = 2 << m_gridLevel;
//		F32 antiStep = 1.0f / gridSize;
//		F32 fScaleIntensity = 255.0f / (m_iMax - m_iMin);
//
//		// 构造全局坐标转换矩阵
//		double m[16];
//		pHlsReader->m_header.computeMatrix(m);
//
//		// 读取200圈点云后一次写入到块集文件
//		U32 nCurLp = 0;
//		U32 nLoopSize = 200;
//		U32 nCount = 0;
//		I32  ptsNum = 0;
//		DWORD dwResult = 0;
//		T    ptTemp;
//
//		I32 xNo = 0, yNo = 0, zNo = 0;
//		double x = 0.0, y = 0.0, z = 0.0;
//		F64 minX = m_fullExtent.MinEdge.X;
//		F64 minY = m_fullExtent.MinEdge.Y;
//		F64 minZ = m_fullExtent.MinEdge.Z;
//
//		EntityIndex bsIndex;
//		std::map<EntityIndex, DivideMemoryBuf<PointXYZIPRGBA>> BsPointsArray;
//		hdBlkArray<PointXYZIPRGBA> loopPtsArray;
//		loopPtsArray.setBlockCount(nLoopSize);
//		
//		LARGE_INTEGER li1,li2;
//		li1.HighPart = 0;
//		li1.LowPart = 0;
//		li2.HighPart = 0;
//		li2.LowPart = 0;
//
//		// 获取圈索引与圈索引
//		const CLoopIndex* pLoopIdx = pHlsReader->GetLoopIndex();
//		int loopCount = pHlsReader->GetLoopCount();
//		while((loopCount - nCurLp) > 0)
//		{
//			// 内存中的200圈点云
//			if ((loopCount - nCurLp) < nLoopSize)
//			{
//				nLoopSize = (loopCount - nCurLp);
//				loopPtsArray.setBlockCount(nLoopSize);
//			}
//		
//			// 从文件中读满loopPtsArray
//			for (U32 n = 0; n < nLoopSize; n++)
//			{
//				hdVector<PointXYZIPRGBA>& vecPts = loopPtsArray.getBlock(n);
//				if(!pHlsReader->ReadLoop(vecPts, n+nCurLp))
//					continue;
//			}
//
//			// 更新当前圈
//			nCurLp += nLoopSize;
//
//			for (I32 i=0; i < nLoopSize; i++)
//			{
//				hdVector<PointXYZIPRGBA>& vecPts = loopPtsArray.getBlock(i);
//				ptsNum = vecPts.size();
//				for (I32 k = 0;k < ptsNum; k++)
//				{
//					PointXYZIPRGBA& pt = *(vecPts._Myfirst + k);
//					//IHLSReader::ReadLoop（）读取的圈数据中没有无效点，不需要判断  20161028
//					if(!pt.isValid())
//						continue;
//
//					// 转为全局坐标
//					x = pt.x;
//					y = pt.y;
//					z = pt.z;
//					hdHomogeneousTransformPoint(m,x,y,z);
//
//					//点所落在的全局块集编号
//					xNo = (I32)floor(x * antiStep);
//					yNo = (I32)floor(y * antiStep);
//					zNo = (I32)floor(z * antiStep);
//
//					//全局坐标转换为块集内相对坐标
//					pt.x = (hd::f32)(x - xNo * gridSize);
//					pt.y = (hd::f32)(y - yNo * gridSize);
//					pt.z = (hd::f32)(z - zNo * gridSize);
//
//					// 计算块集编号
//					bsIndex.xNo = xNo;
//					bsIndex.yNo = yNo;
//					bsIndex.zNo = zNo;
//					
//					DivideMemoryBuf<PointXYZIPRGBA>& memPoints = BsPointsArray[bsIndex];
//
//					if (memPoints.m_count >= memPoints.m_vecBuf.size())
//					{
//						try
//						{
//							memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
//						}
//						catch(...)
//						{
//							::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
//							// 删除临时文件
//							RemoveTempFiles(m_savePath.c_str());
//							return false;
//						}
//					}
//
//					PointXYZIPRGBA*& pPtMem = *(memPoints.m_vecBuf._Myfirst + memPoints.m_count);
//					memPoints.m_count++;
//					pPtMem = &pt;
//
//					// 统计x分布
//					I32 selNum = (I32)((x - minX) * m_coordStats.xRcp);
//					selNum = clamp(selNum, 0, 999);
//					m_coordStats.xStepStat[selNum]++;
//
//					// 统计y分布
//					selNum = (I32)((y - minY) * m_coordStats.yRcp);
//					selNum = clamp(selNum, 0, 999);
//					m_coordStats.yStepStat[selNum]++;
//
//					// 统计z分布
//					selNum = (I32)((z - minZ) * m_coordStats.zRcp);
//					selNum = clamp(selNum, 0, 999);
//					m_coordStats.zStepStat[selNum]++;
//
//					// 统计强度分布
//					selNum = (I32)((pt.getIntensity() - m_iMin) * m_coordStats.iRcp);
//					selNum = clamp(selNum, 0, 999);
//					m_coordStats.intStepStat[selNum]++;
//					m_ptNum++;
//				}//for (I32 k = 0;k < ptsNum; k++)
//			}//for (I32 i=0; i < nLoopSize; i++)
//
//			// 将内存中的点写入到块集文件中
//			for (auto it = BsPointsArray.begin(); it != BsPointsArray.end(); it++)
//			{
//				const EntityIndex& bsIndex = (it->first);
//				DivideMemoryBuf<PointXYZIPRGBA>& bsPoints = (it->second);
//				nCount = bsPoints.m_count;
//				if (nCount <= 0)
//				{
//					continue;
//				}
//
//				BlockSetFileInfo31& bsinfo31 = m_BlocksetFiles[bsIndex];
//
//				sprintf(filePath, "%s\\level0\\blockset_%04d_%04d_%04d.tmp", savePath, bsIndex.xNo, bsIndex.yNo, bsIndex.zNo);
//				HANDLE hFile = CreateFile(filePath,
//					GENERIC_READ|GENERIC_WRITE,
//					FILE_SHARE_READ|FILE_SHARE_WRITE, 
//					NULL,
//					OPEN_ALWAYS, 
//					FILE_ATTRIBUTE_NORMAL, 
//					NULL);
//				if(hFile == INVALID_HANDLE_VALUE)
//				{
//					continue;
//				}
//				if (hFile != NULL)
//				{					
//					SetFilePointerEx(hFile, li1, &li2, FILE_END);
//					//50000个点为单位写入
//					int ptCount = 0;
//					int k = 0;
//					for (U32 m=0; m < nCount; )
//					{
//						PointXYZIPRGBA* pPt = *(bsPoints.m_vecBuf._Myfirst + m++);
//						//float坐标转换为unsigned short坐标
//						ptTemp.x = (hd::u16)(pPt->x * antiStep * 65536);
//						ptTemp.y = (hd::u16)(pPt->y * antiStep * 65536);
//						ptTemp.z = (hd::u16)(pPt->z * antiStep * 65536);
//						//强度线性映射到0-255
//						ptTemp.intensity = hd_round32((pPt->intensity - m_iMin) * fScaleIntensity);
//						//获取分类属性值
//						ptTemp.prop = pPt->prop;
//						//颜色转换为R5G6B5
//						ptTemp.setColor(pPt->r, pPt->g, pPt->b); 
//			
//						*(pPtBuf4Write + k++) = ptTemp;
//
//						if (k == wSize)
//						{
//							WriteFile(hFile, pPtBuf4Write, sizeof(T)*wSize, &dwResult, NULL);
//							ptCount ++;
//							k = 0;
//						}
//					}
//
//					int leftnum = nCount - ptCount*wSize;
//					if ( leftnum>0)
//					{
//						WriteFile(hFile, pPtBuf4Write, sizeof(T)*leftnum, &dwResult, NULL);
//					}
//
//					// 更新文件名
//					bsinfo31.path = filePath;
//					// 更新点数
//					bsinfo31.numPoint += nCount;
//				}
//
//				if (hFile)
//				{
//					CloseHandle(hFile);
//					hFile = NULL;
//				}
//			}//for (auto it = BsPointsArray.begin(); 
//
//			FreeSplitBuffer(BsPointsArray);
//
//			if (m_callback != NULL)
//			{			
//				m_callback((float)nCurLp / loopCount, strInfo.c_str());
//			}
//		}
//
//		SAFE_DELETE_ARY(pPtBuf4Write);
//		SAFE_DELETE_PTR(pHlsReader);
//
//		return true;
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::SplitBlockSetLas(const char* savePath, const std::string& strLasFile)
//	{
//		// las文件打开对象
//		LASreadOpener lasreadopener;
//
//		// 设置不合并
//		lasreadopener.set_merged(FALSE);
//		lasreadopener.set_populate_header(FALSE);
//
//		// 设置文件名
//		lasreadopener.set_file_name(strLasFile.c_str());
//		if (!lasreadopener.active())
//		{
//			return false;
//		}
//		// 打开las文件
//		LASreader* lasRead = lasreadopener.open();
//		// 打开失败、返回
//		if (lasRead == NULL)
//		{
//			return false;
//		}
//
//		I64 npoint = lasRead->npoints;
//		std::string strInfo = "正在切割";
//		int nPos = strLasFile.find_last_of('\\');
//		strInfo += strLasFile.substr(nPos + 1);
//
//		// 进度条开始
//		if (m_callback != NULL)
//		{
//			m_callback(0.0f, strInfo.c_str());
//		}
//
//		// 创建第0层临时文件目录
//		char dir[MAX_PATH] = {0};
//		sprintf_s(dir, "%s\\level0", savePath);
//		I32 ret = _mkdir(dir);
//
//		char filePath[MAX_PATH] = {0};
//
//		int wSize = 50000;
//		T* pPtBuf4Write = new T[wSize];
//
//		I32 gridSize = 2 << m_gridLevel;
//		F32 antiStep = 1.0f / gridSize;
//		F32 antiIntensity = 1.0f / 4095.0f;
//
//		BOOL bRet = FALSE;
//		BOOL bReadEnd = FALSE;
//
//		// 读取500000点云后一次写入到块集文件
//		U32 nBufferSize = 5000000;
//		I64 nCurLp = 0;
//		I32 selNum = 0;
//
//		LARGE_INTEGER li1,li2;
//		li1.HighPart = 0;
//		li1.LowPart = 0;
//		li2.HighPart = 0;
//		li2.LowPart = 0;
//
//		F64 x = 0.0, y = 0.0, z = 0.0;
//		I32 xNo = 0, yNo = 0, zNo = 0;
//		u8 r = 0, g = 0, b = 0;
//		U16 intensity = 0;
//		U32 nCount = 0;
//		DWORD dwResult = 0;
//
//		F64 minX = m_fullExtent.MinEdge.X;
//		F64 minY = m_fullExtent.MinEdge.Y;
//		F64 minZ = m_fullExtent.MinEdge.Z;
//
//		EntityIndex bsIndex;
//		std::map<EntityIndex, DivideMemoryBuf<T>> BsPointsArray;
//
//		hdVector<T> bufArray;
//		try
//		{
//			bufArray.resize(nBufferSize);
//		}
//		catch(...)
//		{
//			::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
//			// 删除临时文件
//			RemoveTempFiles(m_savePath.c_str());
//			return false;
//		}
//
//		while((npoint - nCurLp) >= 0)
//		{
//			for(I32 i = 0;i < nBufferSize;i++)
//			{
//				bRet = lasRead->read_point();
//				if(!bRet)
//				{
//					bReadEnd = TRUE;
//					break;
//				}
//
//				const LASpoint& lasPt = lasRead->point;
//
//				T& pt = *(bufArray._Myfirst + i);
//				x = lasRead->get_x();
//				y = lasRead->get_y();
//				z = lasRead->get_z();
//				intensity = lasPt.intensity * COORD_REF * 4095;
//
//				xNo = (I32)floor(x * antiStep);
//				yNo = (I32)floor(y * antiStep);
//				zNo = (I32)floor(z * antiStep);
//
//				pt.x = (hd::u16)((x - xNo * gridSize) * antiStep * 65536);
//				pt.y = (hd::u16)((y - yNo * gridSize) * antiStep * 65536);
//				pt.z = (hd::u16)((z - zNo * gridSize) * antiStep * 65536);
//				pt.intensity = (hd::u8)(intensity * antiIntensity * 255);
//				pt.prop = lasPt.classification;
//
//				nCurLp++;
//				if(nCurLp >= npoint)
//				{
//					break;
//				}
//
//				// rgb颜色信息
//				if (lasPt.have_rgb)
//				{
//					if (lasPt.rgb[0] > 255 || 
//						lasPt.rgb[1] > 255 ||
//						lasPt.rgb[2] > 255 )
//					{
//						r = (hd::u8)(lasPt.rgb[0] * COORD_REF * 255);
//						g = (hd::u8)(lasPt.rgb[1] * COORD_REF * 255);
//						b = (hd::u8)(lasPt.rgb[2] * COORD_REF * 255);
//						pt.setColor(r, g, b); 
//					}
//					else
//					{
//						r = (u8)lasPt.rgb[0];
//						g = (u8)lasPt.rgb[1];
//						b = (u8)lasPt.rgb[2];
//						pt.setColor(r, g, b);
//					}
//				}
//
//				// 计算块集编号
//				bsIndex.xNo = xNo;
//				bsIndex.yNo = yNo;
//				bsIndex.zNo = zNo;
//				DivideMemoryBuf<T>& memPoints = BsPointsArray[bsIndex];
//
//				if (memPoints.m_count >= memPoints.m_vecBuf.size())
//				{
//					try
//					{
//						memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
//					}
//					catch(...)
//					{
//						::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
//						// 删除临时文件
//						RemoveTempFiles(m_savePath.c_str());
//						return false;
//					}
//				}
//
//				T*& pPtMem = *(memPoints.m_vecBuf._Myfirst + memPoints.m_count);
//				memPoints.m_count++;
//				pPtMem = &pt;				
//
//				// 统计x分布
//				selNum = (I32)((x - minX)* m_coordStats.xRcp);
//				selNum = clamp(selNum, 0, 999);
//				m_coordStats.xStepStat[selNum]++;
//
//				// 统计y分布
//				selNum = (I32)((y - minY) * m_coordStats.yRcp);
//				selNum = clamp(selNum, 0, 999);
//				m_coordStats.yStepStat[selNum]++;
//
//				// 统计z分布
//				selNum = (I32)((z - minZ) * m_coordStats.zRcp);
//				selNum = clamp(selNum, 0, 999);
//				m_coordStats.zStepStat[selNum]++;
//
//				// 统计强度分布
//				selNum = (I32)((intensity - m_iMin) * m_coordStats.iRcp);
//				selNum = clamp(selNum, 0, 999);
//				m_coordStats.intStepStat[selNum]++;
//				m_ptNum++;
//			}
//
//			// 将内存中的点写入到块集文件中
//			for (auto it = BsPointsArray.begin(); it != BsPointsArray.end(); it++)
//			{
//				const EntityIndex& bsIndex = (it->first);
//				DivideMemoryBuf<T>& bsPoints = (it->second);
//				U32 nCount = bsPoints.m_count;
//				if (nCount <= 0)
//				{
//					continue;
//				}
//
//				BlockSetFileInfo31& bsInfo31 = m_BlocksetFiles[bsIndex];
//
//				sprintf(filePath, "%s\\level0\\blockset_%04d_%04d_%04d.tmp", savePath, bsIndex.xNo, bsIndex.yNo, bsIndex.zNo);
//				HANDLE hFile = CreateFile(filePath,
//					GENERIC_READ|GENERIC_WRITE,
//					FILE_SHARE_READ|FILE_SHARE_WRITE,
//					NULL,
//					OPEN_ALWAYS, 
//					FILE_ATTRIBUTE_NORMAL, 
//					NULL);
//				if (hFile != INVALID_HANDLE_VALUE)
//				{			
//					
//					SetFilePointerEx(hFile, li1, &li2, FILE_END);
//					//50000个点为单位写入
//					int ptCount = 0;
//					int k = 0;
//					for (U32 m=0; m < nCount; m++)
//					{
//						T* pPt = *(bsPoints.m_vecBuf._Myfirst + m);
//						*(pPtBuf4Write + k++) = *pPt;
//
//						if (k == wSize)
//						{
//							WriteFile(hFile, pPtBuf4Write, sizeof(T)*wSize, &dwResult, NULL);
//							ptCount ++;
//							k = 0;
//						}
//					}
//					int leftnum = nCount - ptCount*wSize;
//					if ( leftnum>0)
//					{
//						WriteFile (hFile, pPtBuf4Write, sizeof(T)*leftnum, &dwResult, NULL);
//					}
//
//					// 更新文件名
//					bsInfo31.path = filePath;
//					bsInfo31.numPoint += nCount;
//				}
//
//				if (hFile)
//				{
//					CloseHandle(hFile);
//					hFile = NULL;
//				}
//			}
//
//			FreeSplitBuffer(BsPointsArray);
//
//			if (m_callback != NULL)
//			{			
//				m_callback((float)nCurLp / npoint, strInfo.c_str());
//			}
//
//			if(bReadEnd)
//			{
//				break;
//			}
//		}
//
//		SAFE_DELETE_ARY(pPtBuf4Write);
//		SAFE_DELETE_PTR(lasRead);
//
//		return true;
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::SplitBlockFiles(BlockSetFileInfo31& procBlockSet, std::vector<BlockFileInfo31>& arrayBlocks, U8 BSNo, BlockSetFileInfo31& nextBSInfo)
//	{
//		HANDLE hFile = CreateFile(procBlockSet.path.c_str(),
//			GENERIC_READ,
//			FILE_SHARE_READ,
//			NULL,
//			OPEN_EXISTING,
//			FILE_ATTRIBUTE_NORMAL,
//			NULL);
//		if (hFile == INVALID_HANDLE_VALUE)
//		{
//			return false;
//		}
//
//		// 创建存储目录
//		std::string dir = procBlockSet.path;
//		dir = dir.substr(0, dir.find_last_of('.'));
//		I32 ret = _mkdir(dir.c_str());
//
//		// 一次读取m_memory_point_throughput个点到内存，块划分完成后写入到文件
//		std::vector<T> bufferPts;
//
//		CHdVector3di center = procBlockSet.box.getCenter();
//		CHdVector3di halfSize = procBlockSet.box.getExtent() /2;
//		arrayBlocks.resize(8);
//
//		int wSize = 50000;
//		T* pPtBuf4Write = new T[wSize];
//
//		DWORD nNumRead = 0;
//		U32 nReadNum = m_memory_point_throughput;
//		U32 nPtNum = procBlockSet.numPoint;
//		while(nPtNum > 0)
//		{
//			if(nPtNum < nReadNum)
//			{
//				nReadNum = nPtNum;
//			}
//
//			try
//			{
//				bufferPts.resize(nReadNum);
//				ReadFile(hFile, bufferPts._Myfirst, nReadNum * sizeof(T), &nNumRead, NULL);
//			}
//			catch(...)
//			{
//				::MessageBox(NULL, "内存不足!", NULL, MB_OK);
//				CloseHandle(hFile);
//				SAFE_DELETE_ARY(pPtBuf4Write);
//				return false;
//			}
//
//			simplePoints(bufferPts, BSNo, nextBSInfo);
//		/*}
//		while (m_pHlzWrite->ReadPtsBySize(pFile, bufferPts, m_memory_point_throughput) > 0)
//		{*/
//			// 切分生成的块文件对应的点
//			vector<DivideMemoryBuf<T>> blockPoints;
//			blockPoints.resize(8);
//			I32 nCurPtNum = bufferPts.size();
//			for (I32 i=0; i < nCurPtNum; i++)
//			{
//				T* ptTemp = bufferPts._Myfirst + i;
//
//				U8 blockIdx = 0;			
//				getPtBlockIndex(*ptTemp, center, blockIdx);
//				DivideMemoryBuf<T>& memPoints = *(blockPoints._Myfirst + blockIdx);
//				if (memPoints.m_count >= memPoints.m_vecBuf.size())
//				{
//					memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
//				}
//				
//				T*& pPtMem = *(memPoints.m_vecBuf._Myfirst + memPoints.m_count);
//				memPoints.m_count++;
//				pPtMem = bufferPts._Myfirst + i;
//			}//for (I32 i=0; i < nCurPtNum; i++)
//
//			// 将内存中的点一次写入到块文件中
//			for (I32 i = 0;i < 8;i++)
//			{
//				DivideMemoryBuf<T>& blkPoints = *(blockPoints._Myfirst + i);
//				int nCount = blkPoints.m_count;
//				if(nCount <= 0)
//					continue;
//				
//				char* blockFilePath = new char[dir.length()+13];
//				sprintf_s(blockFilePath, dir.length()+13, "%s\\block_%d.tmp", dir.c_str(), i);
//
//				HANDLE hBlockFile =  CreateFile(blockFilePath,
//					GENERIC_READ|GENERIC_WRITE,
//					FILE_SHARE_READ|FILE_SHARE_WRITE,
//					NULL,
//					OPEN_ALWAYS,
//					FILE_ATTRIBUTE_NORMAL,
//					NULL);
//				if (hBlockFile == INVALID_HANDLE_VALUE)
//				{
//					continue;
//				}
//				LARGE_INTEGER li1,li2;
//				li1.HighPart = 0;
//				li1.LowPart = 0;
//				li2.HighPart = 0;
//				li2.LowPart = 0;
//				SetFilePointerEx(hBlockFile, li1, &li2, FILE_END);
//
//				BlockFileInfo31& blkInfo = *(arrayBlocks._Myfirst + i);
//				blkInfo.path = blockFilePath;
//				blkInfo.numPoint += nCount;
//				blkInfo.index = i;
//				getBlockBox(center,halfSize,blkInfo.box,(U16)i);
//
//				//50000个点为单位写入
//				int ptCount = 0;
//				int k = 0;
//				for (I32 j = 0; j < nCount; j++)
//				{
//					T* pPt = *(blkPoints.m_vecBuf._Myfirst + j);
//					*(pPtBuf4Write + k++) = *pPt;
//					if (k == wSize)
//					{
//						DWORD numWrite;
//						WriteFile(hBlockFile, pPtBuf4Write, sizeof(T)*wSize, &numWrite, NULL);
//						ptCount ++;
//						k = 0;
//					}
//				}
//				int leftnum = nCount - ptCount * wSize;
//				if ( leftnum>0)
//				{
//					DWORD dwResult;
//					WriteFile (hBlockFile, pPtBuf4Write, sizeof(T)*leftnum, &dwResult, NULL);
//				}
//
//				SAFE_DELETE_ARY(blockFilePath);
//				
//				if (hBlockFile)
//				{
//					CloseHandle(hBlockFile);
//					hBlockFile = NULL;
//				}			
//			}//for (i = 0;i < 8;i++)将内存中的点一次写入到块文件中
//
//			bufferPts.clear();
//			bufferPts.swap(std::vector<T>());
//			ClearSplitMemBuf(blockPoints);
//
//			nPtNum -= nReadNum;
//		}//while(nPtNum > 0)
//
//		//关闭并删除块集缓存文件
//		CloseHandle(hFile);
//		remove(procBlockSet.path.c_str());
//		SAFE_DELETE_ARY(pPtBuf4Write);
//
//		return true;
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::SplitBlockset2Block_Buffer(BlockSetFileInfo31& procBlockSet, std::vector<BlockFileInfo31>& arrayBlocks, U8 BSNo, BlockSetFileInfo31& nextBSInfo)
//	{
//		HANDLE hFile = CreateFile(procBlockSet.path.c_str(),
//			GENERIC_READ,
//			FILE_SHARE_READ,
//			NULL,
//			OPEN_EXISTING,
//			FILE_ATTRIBUTE_NORMAL,
//			NULL);
//		if (hFile == INVALID_HANDLE_VALUE)
//		{
//			return false;
//		}
//		// 创建快文件标识ID，为了保证ID的惟一性，使用存储目录作为ID
//		std::string dir = procBlockSet.path;
//
//		// 当前点所在的块序号
//		CHdVector3di center = procBlockSet.box.getCenter();
//		CHdVector3di halfSize = procBlockSet.box.getExtent() / 2;
//		arrayBlocks.resize(8);
//
//		//一次性读取整个块集文件中的数据
//		DWORD nNumRead = 0;
//		U32 nPtCount = procBlockSet.numPoint;
//		try
//		{
//			m_blsDataBuf.resize(nPtCount);
//			ReadFile(hFile, m_blsDataBuf._Myfirst, sizeof(T)*nPtCount, &nNumRead, NULL);
//		}
//		catch(...)
//		{
//			::MessageBox(NULL, "内存不足!", NULL, MB_OK);
//			CloseHandle(hFile);
//			return false;
//		}
//
//		//关闭并删除当前块集文件
//		CloseHandle(hFile);
//		remove(procBlockSet.path.c_str());
//		//抽稀到对应下层块集缓存文件
//		simplePoints(m_blsDataBuf, BSNo, nextBSInfo);
//
//		// 切分生成的块文件对应的点
//		vector<DivideMemoryBuf<T>> blockPoints;
//		blockPoints.resize(8);
//
//		//开始切分
//		I32 nPtNum = m_blsDataBuf.size();
//		for (I32 i=0; i < nPtNum; i++)
//		{
//			T* ptTemp = m_blsDataBuf._Myfirst + i;
//
//			U8 blockIdx = 0;			
//			getPtBlockIndex(*ptTemp, center, blockIdx);
//			DivideMemoryBuf<T>& memPoints = *(blockPoints._Myfirst + blockIdx);
//
//			if (memPoints.m_count >= memPoints.m_vecBuf.size())
//			{
//				memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
//			}
//
//			T*& pPtMem = *(memPoints.m_vecBuf._Myfirst + memPoints.m_count);
//			memPoints.m_count++;
//			pPtMem = m_blsDataBuf._Myfirst + i;
//		}//for (I32 i=0; i < nPtNum; i++)
//
//		// 将内存中的点一次写入到块文件中
//		for (I32 i = 0;i < 8;i++)
//		{
//			DivideMemoryBuf<T>& blkPoints = *(blockPoints._Myfirst + i);
//			int nCount = blkPoints.m_count;
//			if(nCount <= 0)
//				continue;
//
//			char blockDataID_temp[512] = {0};
//			sprintf_s(blockDataID_temp, "%s\\block_%d.tmp", dir.c_str(), i);
//
//			BlockFileInfo31& blkInfo = *(arrayBlocks._Myfirst + i);
//			blkInfo.path = blockDataID_temp;
//			blkInfo.numPoint += nCount;
//			blkInfo.index = i;
//			getBlockBox(center,halfSize,blkInfo.box,(U16)i);
//			//分配内存空间
//			int preSize = m_pBlockData[blkInfo.path.c_str()].size();  //已有大小
//			m_pBlockData[blkInfo.path.c_str()].resize(preSize + nCount);
//			auto iter_block = m_pBlockData.find(blkInfo.path.c_str());
//			memcpy(iter_block->second._Myfirst + preSize, blkPoints.m_vecBuf._Myfirst, nCount*sizeof(T*));
//		}
//
//		return true;
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::SplitBlock2Parcel_Buffer(const CHdBox3di& blockBox, const char* strBlockID, const char* strParcelID, std::vector<ParcelFileInfo31>& arrayParcels)
//	{
//		//找到块集内存中对应的的块文件
//		auto iter_block = m_pBlockData.find(strBlockID);
//		if (iter_block == m_pBlockData.end()) 
//		{
//			//该块中不存在数据
//			return false;
//		}
//
//		// 对块文件进行切分
//		std::map<U8, ParcelFileInfo31> mapSplitParcels;	// 一个块分割出来的包文件
//
//		CHdVector3di center = blockBox.getCenter();
//		CHdVector3di halfSize = blockBox.getExtent() / 2;
//
//		// 切分生成的包文件的点
//		vector<DivideMemoryBuf<T>>  parcelPoints;
//		parcelPoints.resize(8);
//
//		I32 nBlockPtNum = iter_block->second.size();
//		for (I32 i=0; i < nBlockPtNum; i++)
//		{
//			T* ptTemp = *(iter_block->second._Myfirst + i);
//
//			// 该点所在的包序号
//			U8 parcelIdx = 0;
//			getPtBlockIndex(*ptTemp, center, parcelIdx);
//			DivideMemoryBuf<T>& memPoints = *(parcelPoints._Myfirst + parcelIdx);
//
//			if (memPoints.m_count >= memPoints.m_vecBuf.size())
//			{
//				try
//				{
//					memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
//				}
//				catch(...)
//				{
//					::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
//					// 删除临时文件
//					RemoveTempFiles(m_savePath.c_str());
//					return false;
//				}
//			}
//	
//			T*& pPtMem = *(memPoints.m_vecBuf._Myfirst + memPoints.m_count);
//			memPoints.m_count++;
//			pPtMem = *(iter_block->second._Myfirst + i);
//		}  //for (I32 i=0; i < nBlockPtNum; i++)
//
//		char parcelDataID[512] = {0};	
//		// 1.将内存中的点一次写入到包文件中; 2.统计包文件信息;
//		U32 parcelPtNum = parcelPoints.size();
//		for (I32 i = 0;i < parcelPtNum; i++)
//		{			
//			DivideMemoryBuf<T>&  pacPoints = *(parcelPoints._Myfirst + i);
//			int nCount = pacPoints.m_count;
//			if (nCount <= 0)
//			{
//				continue;
//			}
//
//			sprintf_s(parcelDataID, "%s\\parcel_%d.tmp", strParcelID, i);
//			hd::stringc strParcelDataID = parcelDataID;		
//			//写入数据
//			int preSize = m_ParcelData[strParcelDataID.c_str()].size();
//			//重新分配内存空间
//			try
//			{
//				m_ParcelData[strParcelDataID.c_str()].resize(preSize + nCount);
//			}
//			catch(...)
//			{
//				::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
//				// 删除临时文件
//				RemoveTempFiles(m_savePath.c_str());
//				return false;
//			}
//
//			auto iter_parcel = m_ParcelData.find(strParcelDataID.c_str());
//			memcpy(iter_parcel->second._Myfirst + preSize, pacPoints.m_vecBuf._Myfirst, nCount * sizeof(T*));
//
//			// 收集包文件信息
//			if (!mapSplitParcels.count(i))
//			{
//				ParcelFileInfo31 info;
//				//getBlockBox(parcelFilePath, c, info.box,(U16)i);		// 包的范围
//				getBlockBox(center,halfSize,info.box,(U16)i);
//				info.path = parcelDataID;
//				info.numPoint = nCount;
//				mapSplitParcels.insert(make_pair(i, info));
//			}
//			else
//			{
//				ParcelFileInfo31& parcelInfo = mapSplitParcels[i];
//				parcelInfo.numPoint += nCount;
//			}
//		}
//
//		// 对包文件进行递归切分
//		for (auto it = mapSplitParcels.begin(); it != mapSplitParcels.end(); it++)
//		{
//			if ((it->second).numPoint <= m_entity_point_threshold)
//			{
//				// 包文件不需要再往下切分
//				arrayParcels.push_back(it->second);
//			}
//			else
//			{
//				std::string dir = (it->second).path.c_str();
//				dir = dir.substr(0, dir.find_last_of('.'));
//				I32 ret = _mkdir(dir.c_str());
//
//				char splitPath[MAX_PATH] = {0};
//				sprintf_s(splitPath,"%s", dir.c_str());
//
//				// 继续往下切分
//				if(SplitParcel2Parcel_Buffer((it->second).box, (it->second).path.c_str(), splitPath, arrayParcels))
//				{
//					//切分完成后从内存中删除该包的数据
//					m_ParcelData.erase(it->second.path.c_str());
//				}
//			}
//		}
//
//		return true;
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::SplitParcel2Parcel_Buffer(const CHdBox3di& blockBox, const char* srcParcelID, const string destPacelID, std::vector<ParcelFileInfo31>& arrayParcels)
//	{
//		//找到需要切分的包文件
//		auto iter_parcel = m_ParcelData.find(srcParcelID);
//		if (iter_parcel == m_ParcelData.end()) 
//		{
//			//该块中不存在数据
//			return false;
//		}
//
//		// 对块文件进行切分
//		std::map<U8, ParcelFileInfo31> mapSplitParcels;	// 一个块分割出来的包文件
//
//		CHdVector3di center = blockBox.getCenter();
//		CHdVector3di halfSize = blockBox.getExtent() / 2;
//
//		// 切分生成的包文件的点
//		vector<DivideMemoryBuf<T>> parcelPoints;
//		parcelPoints.resize(8);
//
//		I32 nPtNum = iter_parcel->second.size();
//		for (I32 i=0; i < nPtNum; i++)
//		{
//			T* ptTemp = *(iter_parcel->second._Myfirst + i);
//
//			// 该点所在的包序号
//			U8 parcelIdx = 0;
//			getPtBlockIndex(*ptTemp, center, parcelIdx);
//			DivideMemoryBuf<T>& memPoints = *(parcelPoints._Myfirst + parcelIdx);
//
//			if (memPoints.m_count >= memPoints.m_vecBuf.size())
//			{
//				try
//				{
//					memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);	
//				}
//				catch(...)
//				{
//					::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
//					// 删除临时文件
//					RemoveTempFiles(m_savePath.c_str());
//					return false;
//				}
//			}
//		
//			T*& pPtMem = *(memPoints.m_vecBuf._Myfirst + memPoints.m_count);
//			memPoints.m_count++;
//			pPtMem = *(iter_parcel->second._Myfirst + i);
//		}//for (I32 i=0; i < nPtNum; i++)
//
//		char* parcelDataID = NULL;	
//
//		// 1.将内存中的点一次写入到包文件中; 2.统计包文件信息;
//		for (I32 i = 0;i < 8;i++)
//		{			
//			DivideMemoryBuf<T>& pacPoints = *(parcelPoints._Myfirst + i);
//			int nCount = pacPoints.m_count;
//			if (nCount <= 0)
//			{
//				continue;
//			}
//
//			parcelDataID = new char[destPacelID.length()+14];
//			sprintf_s(parcelDataID, destPacelID.length()+14,"%s\\parcel_%d.tmp", destPacelID.c_str(), i);
//			hd::stringc strParcelDataID = parcelDataID;				
//			//写入数据
//			int preSize = m_ParcelData[strParcelDataID.c_str()].size();
//			//重新分配内存空间
//			try
//			{
//				m_ParcelData[strParcelDataID.c_str()].resize(preSize + nCount);
//			}
//			catch(...)
//			{
//				::MessageBox(NULL, "内存分配出错，请提高内存分配或者使用“临时文件方式”", NULL, MB_OK);
//				// 删除临时文件
//				RemoveTempFiles(m_savePath.c_str());
//				return false;
//			}
//
//			auto iter_parcel = m_ParcelData.find(strParcelDataID.c_str());
//			memcpy(iter_parcel->second._Myfirst + preSize, pacPoints.m_vecBuf._Myfirst, nCount*sizeof(T*));
//
//			// 收集包文件信息
//			if (!mapSplitParcels.count(i))
//			{
//				ParcelFileInfo31 info;
//				//getBlockBox(parcelFilePath, c, info.box,(U16)i);		// 包的范围
//				getBlockBox(center,halfSize,info.box,(U16)i);
//				info.path = parcelDataID;
//				info.numPoint = nCount;
//				mapSplitParcels.insert(make_pair(i, info));
//			}
//			else
//			{
//				ParcelFileInfo31& parcelInfo = mapSplitParcels[i];
//				parcelInfo.numPoint += nCount;
//			}
//
//			SAFE_DELETE_ARY(parcelDataID);
//		}
//
//		// 对包文件进行递归切分
//		for (auto it = mapSplitParcels.begin(); it != mapSplitParcels.end(); it++)
//		{
//			CHdVector3di extent = (it->second).box.getExtent();
//			//细分到0.0001，将不再继续细分
//			if (extent.X<0.0001 && extent.Y < 0.01 && extent.Z < 0.0001)
//			{
//				U64 num = (it->second).numPoint;
//				(it->second).numPoint = num < m_entity_point_threshold ? num:m_entity_point_threshold;
//			}
//			// 点数小于阈值，不进行递归切分
//			if ((it->second).numPoint <= m_entity_point_threshold)
//			{
//				// 包文件不需要再往下切分
//				arrayParcels.push_back(it->second);
//			}
//			else
//			{
//				std::string splitPath = (it->second).path.c_str();
//				splitPath = splitPath.substr(0, splitPath.find_last_of('.'));
//
//				// 继续往下切分
//				if(SplitParcel2Parcel_Buffer((it->second).box, (it->second).path.c_str(), splitPath.c_str(), arrayParcels))
//				{
//					//切分完成后从内存中删除该包的数据
//					m_ParcelData.erase(it->second.path.c_str());
//				}
//			}
//		}
//
//		return true;
//	}
//
//	// 将块文件划分成子块
//	template <class T>
//	bool CHlz31Builder<T>::SplitBlock2Parts(const CHdBox3di& blockBox, const string strBlockFile, std::vector<subBlockFileInfo31>& subBlkInfo)
//	{
//		HANDLE hBlockFile = CreateFile(strBlockFile.c_str(),
//			GENERIC_READ,
//			FILE_SHARE_READ,
//			NULL,
//			OPEN_EXISTING,
//			FILE_ATTRIBUTE_NORMAL,
//			NULL);
//		if (hBlockFile == INVALID_HANDLE_VALUE)
//		{
//			hBlockFile = NULL;
//		}
//
//		string savePath = strBlockFile.substr(0, strBlockFile.find_last_of('.'));
//		_mkdir(savePath.c_str());
//
//		// 对块文件进行切分
//		std::map<U8, subBlockFileInfo31> mapSplitBlk;	// 一个块分割出来的包文件
//
//		//std::vector<PointXYZIPRGBA> bufferPts;
//		std::vector<T> bufferPts;
//
//		CHdVector3di center = blockBox.getCenter();
//		CHdVector3di halfSize = blockBox.getExtent() / 2;
//
//		int wSize = 50000;
//		T* pPtBuf4Write = new T[wSize];
//
//		DWORD sizeHigh = 0;
//		U64 fileSize = GetFileSize(hBlockFile, &sizeHigh);
//		if(sizeHigh != 0)
//		{
//			fileSize = ((U64)sizeHigh << 32) | fileSize;
//		}
//
//		DWORD nNumRead = 0;
//		U32 nReadNum = m_memory_point_throughput;
//		U64 nPtNum = fileSize / sizeof(T);
//		while(nPtNum > 0)
//		{
//			if(nPtNum < nReadNum)
//			{
//				nReadNum = nPtNum;
//			}
//
//			try
//			{
//				bufferPts.resize(nReadNum);
//				ReadFile(hBlockFile, bufferPts._Myfirst, sizeof(T)*nReadNum, &nNumRead, NULL);
//			}
//			catch(...)
//			{
//				::MessageBox(NULL, "内存不足!", NULL, MB_OK);
//				CloseHandle(hBlockFile);
//				SAFE_DELETE_ARY(pPtBuf4Write);
//				return false;
//			}
//			
//		/*while (m_pHlzWrite->ReadPtsBySize(hBlockFile, bufferPts, m_memory_point_throughput) > 0)
//		{*/
//			// 切分生成的包文件的点
//			vector<DivideMemoryBuf<T>> parcelPoints;
//			parcelPoints.resize(8);
//			I32 nCurPtNum = bufferPts.size();
//			for (I32 i=0; i < nCurPtNum; i++)
//			{
//				T* ptTemp = bufferPts._Myfirst + i;
//		
//				// 该点所在的包序号
//				U8 parcelIdx = 0;
//				getPtBlockIndex(*ptTemp, center, parcelIdx);
//				DivideMemoryBuf<T>& memPoints = *(parcelPoints._Myfirst + parcelIdx);
//				if (memPoints.m_count >= memPoints.m_vecBuf.size())
//				{
//					try
//					{
//						memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
//					}
//					catch(...)
//					{
//						string msg = "内存分配失败";//"内存分配失败，请提高内存配置或者使用“临时文件方式”\n(" + strBlockFile +")";
//						::MessageBox(NULL, msg.c_str(), NULL, MB_OK);
//						continue;
//					}
//				}
//
//				T*& pPtMem = *(memPoints.m_vecBuf._Myfirst + memPoints.m_count);
//				memPoints.m_count++;
//				pPtMem = bufferPts._Myfirst + i;
//			}//for (I32 i=0; i < nCurPtNum; i++)
//
//			// 1.将内存中的点一次写入到包文件中; 2.统计包文件信息;
//			U32 nPclPtNum = parcelPoints.size();
//			for (U32 i = 0;i < nPclPtNum; i++)
//			{			
//				DivideMemoryBuf<T>& pacPoints = *(parcelPoints._Myfirst + i);
//				int nCount = pacPoints.m_count;
//				if (nCount <= 0)
//				{
//					continue;
//				}
//				char* subBlkFilePath = new char[savePath.length() + 14];		
//				sprintf_s(subBlkFilePath, savePath.length() + 14, "%s\\subBlk_%d.tmp", savePath.c_str(), i);
//
//				HANDLE hSubBlkFile = CreateFile(subBlkFilePath,
//					GENERIC_READ|GENERIC_WRITE,
//					FILE_SHARE_READ|FILE_SHARE_WRITE,
//					NULL, 
//					OPEN_ALWAYS,
//					FILE_ATTRIBUTE_NORMAL,
//					NULL);
//				if (hSubBlkFile == INVALID_HANDLE_VALUE)
//				{
//					continue;
//				}
//				// 定位到文件尾部
//				LARGE_INTEGER li1,li2;
//				li1.HighPart = 0;
//				li1.LowPart = 0;
//				li2.HighPart = 0;
//				li2.LowPart = 0;
//				SetFilePointerEx(hSubBlkFile, li1, &li2, FILE_END);
//				//50000个点为单位写入
//				int ptCount = 0;
//				int k = 0;
//				for (I32 j= 0; j < nCount; j++)
//				{
//					T* pPt = *(pacPoints.m_vecBuf._Myfirst + j);
//					*(pPtBuf4Write + k++) = *pPt;
//					if (k == wSize)
//					{
//						DWORD numWrite;
//						WriteFile(hSubBlkFile, pPtBuf4Write, sizeof(T)*wSize, &numWrite, NULL);
//						ptCount ++;
//						k = 0;
//					}
//				}
//				int leftnum = nCount - ptCount * wSize;
//				if ( leftnum>0)
//				{
//					DWORD dwResult;
//					WriteFile(hSubBlkFile, pPtBuf4Write, sizeof(T)*leftnum, &dwResult, NULL);
//				}
//
//				CloseHandle(hSubBlkFile);
//				hSubBlkFile = NULL;
//
//				// 收集包文件信息
//				if (!mapSplitBlk.count(i))
//				{
//					subBlockFileInfo31 info;
//					//getBlockBox(parcelFilePath, c, info.box,(U16)i);		// 包的范围
//					getBlockBox(center,halfSize,info.box,(U16)i);
//					info.path = subBlkFilePath;
//					info.numPoint = nCount;
//					mapSplitBlk.insert(make_pair(i, info));
//				}
//				else
//				{
//					subBlockFileInfo31& parcelInfo = mapSplitBlk[i];
//					parcelInfo.numPoint += nCount;
//				}
//
//				SAFE_DELETE_ARY(subBlkFilePath);
//			}//for (I32 i = 0;i < nPclPtNum;i++)
//
//			// 回收内存
//			bufferPts.clear();
//			bufferPts.swap(vector<T>());
//			ClearSplitMemBuf(parcelPoints);
//
//			nPtNum -= nReadNum;
//		}//while(nPtNum > 0)
//
//		// 回收内存
//		SAFE_DELETE_ARY(pPtBuf4Write);
//
//		// 关闭块文件
//		if (hBlockFile)
//		{
//			CloseHandle(hBlockFile);
//			hBlockFile = NULL;
//		}
//
//		// 删除块文件
//		int err = remove(strBlockFile.c_str());
//		if (err == -1)
//		{
//			string strError = "无法删除文件：" ;
//			strError = strError + strBlockFile.c_str();
//			perror(strError.c_str());
//		}
//
//		// 对包文件进行递归切分
//		for (auto it = mapSplitBlk.begin(); it != mapSplitBlk.end(); it++)
//		{
//			if ((it->second).numPoint <= m_memory_point_throughput)
//			{
//				// 包文件不需要再往下切分
//				subBlkInfo.push_back(it->second);
//			}
//			else
//			{
//				// 继续往下切分
//				SplitBlock2Parts((it->second).box, (it->second).path.c_str(), subBlkInfo);
//			}
//		}
//
//		mapSplitBlk.clear();
//		return true;
//	}
//
//	// 子块文件分包
//	template <class T>
//	bool CHlz31Builder<T>::SplitSbkFile2Parcel_Buffer(const CHdBox3di& blockBox, const string strSbkFile, std::vector<ParcelFileInfo31>& vecParcelInfo)
//	{
//		HANDLE hBlockFile = CreateFile(strSbkFile.c_str(),
//			GENERIC_READ,
//			FILE_SHARE_READ,
//			NULL,
//			OPEN_EXISTING,
//			FILE_ATTRIBUTE_NORMAL,
//			NULL);
//		if (hBlockFile == INVALID_HANDLE_VALUE)
//		{
//			return false;
//		}
//
//		string savePath = strSbkFile.substr(0, strSbkFile.find_last_of('.'));
//
//		// 读取子块中的所有点
//		DWORD sizeHigh = 0;
//		U64 fileSize = GetFileSize(hBlockFile, &sizeHigh);
//		if(sizeHigh != 0)
//		{
//			fileSize = ((U64)sizeHigh << 32) | fileSize;
//		}
//
//		U32 nPtNum = fileSize / sizeof(T);
//		try
//		{
//
//			m_blsDataBuf.resize(nPtNum);
//			ReadFile(hBlockFile, m_blsDataBuf._Myfirst, sizeof(T) * nPtNum, &sizeHigh, NULL);
//		}
//		catch(...)
//		{
//			::MessageBox(NULL, "内存不足!", NULL, MB_OK);
//			CloseHandle(hBlockFile);
//			return false;
//		}
//		
//		if (hBlockFile != NULL)
//		{
//			CloseHandle(hBlockFile);
//			hBlockFile = NULL;
//		}
//
//		// 初始化块数据指针
//		m_pBlockData[savePath].resize(m_blsDataBuf.size());
//		auto iter_block = m_pBlockData.find(savePath);
//		int nPtCount = iter_block->second.size();
//		for (int i=0; i< nPtCount; i++)
//		{
//			T*& pPt = *(iter_block->second._Myfirst + i);
//			pPt = m_blsDataBuf._Myfirst + i;
//		}
//
//		// 对块文件进行切分
//		std::map<U8, ParcelFileInfo31> mapSplitParcels;	// 一个块分割出来的包文件
//
//		CHdVector3di center = blockBox.getCenter();
//		CHdVector3di halfSize = blockBox.getExtent() / 2;
//
//		// 切分生成的包文件的点
//		vector<DivideMemoryBuf<T>> parcelPoints;
//		parcelPoints.resize(8);
//
//		I32 nBlockCount = iter_block->second.size();
//		for (I32 i=0; i < nBlockCount; i++)
//		{
//			T* ptTemp = *(iter_block->second._Myfirst + i);
//
//			// 该点所在的包序号
//			U8 parcelIdx = 0;
//			getPtBlockIndex(*ptTemp, center, parcelIdx);
//
//			DivideMemoryBuf<T>& memPoints = *(parcelPoints._Myfirst + parcelIdx);
//			if (memPoints.m_count >= memPoints.m_vecBuf.size())
//			{
//				try
//				{
//					memPoints.m_vecBuf.resize(memPoints.m_vecBuf.size() + MEMORYBUF_SIZE,0);
//				}
//				catch(...)
//				{
//					::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
//					// 删除临时文件
//					RemoveTempFiles(m_savePath.c_str());
//					return false;
//				}
//			}
//
//			T*& pPtMem = *(memPoints.m_vecBuf._Myfirst + memPoints.m_count);
//			memPoints.m_count++;
//			pPtMem = *(iter_block->second._Myfirst + i);
//		}  //for (I32 i=0; i < nBlockCount; i++)
//
//		char parcelDataID[512] = {0};	
//		// 1.将内存中的点一次写入到包文件中; 2.统计包文件信息;
//		U32 pclPoints = parcelPoints.size();
//		for (U32 i = 0;i < pclPoints; i++)
//		{			
//			DivideMemoryBuf<T>& pacPoints = *(parcelPoints._Myfirst + i);
//			int nCount = pacPoints.m_count;
//			if (nCount <= 0)
//			{
//				continue;
//			}
//
//			sprintf_s(parcelDataID, "%s\\parcel_%d.tmp", savePath.c_str(), i);
//			hd::stringc strParcelDataID = parcelDataID;		
//			//写入数据
//			int preSize = m_ParcelData[strParcelDataID.c_str()].size();
//			//重新分配内存空间
//			try
//			{
//				m_ParcelData[strParcelDataID.c_str()].resize(preSize + nCount);
//			}
//			catch(...)
//			{
//				::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
//				// 删除临时文件
//				RemoveTempFiles(m_savePath.c_str());
//				return false;
//			}
//
//			auto iter_parcel = m_ParcelData.find(strParcelDataID.c_str());
//			memcpy(iter_parcel->second._Myfirst + preSize, pacPoints.m_vecBuf._Myfirst, nCount * sizeof(T*));
//
//			// 收集包文件信息
//			if (!mapSplitParcels.count(i))
//			{
//				ParcelFileInfo31 info;
//				//getBlockBox(parcelFilePath, c, info.box,(U16)i);		// 包的范围
//				getBlockBox(center,halfSize,info.box,(U16)i);
//				info.path = parcelDataID;
//				info.numPoint = nCount;
//				mapSplitParcels.insert(make_pair(i, info));
//			}
//			else
//			{
//				ParcelFileInfo31& parcelInfo = mapSplitParcels[i];
//				parcelInfo.numPoint += nCount;
//			}
//		}
//		//清空分包的临时数据
//		ClearSplitMemBuf(parcelPoints);
//
//		// 删除子块文件
//		int err = remove(strSbkFile.c_str());
//		if (err == -1)
//		{
//			string strError = "无法删除文件：" ;
//			strError = strError + strSbkFile.c_str();
//			perror(strError.c_str());
//		}
//
//		// 对包文件进行递归切分
//		for (auto it = mapSplitParcels.begin(); it != mapSplitParcels.end(); it++)
//		{
//			if ((it->second).numPoint <= m_entity_point_threshold)
//			{
//				// 包文件不需要再往下切分
//				vecParcelInfo.push_back(it->second);
//			}
//			else
//			{
//				std::string dir = (it->second).path.c_str();
//				dir = dir.substr(0, dir.find_last_of('.'));
//				I32 ret = _mkdir(dir.c_str());
//
//				char splitPath[MAX_PATH] = {0};
//				sprintf_s(splitPath,"%s", dir.c_str());
//
//				// 继续往下切分
//				if(SplitParcel2Parcel_Buffer((it->second).box, (it->second).path.c_str(), splitPath, vecParcelInfo))
//				{
//					//切分完成后从内存中删除该包的数据
//					m_ParcelData.erase(it->second.path.c_str());
//				}
//			}
//		}
//
//		return true;
//	}
//
//	template <class T>
//	U64 CHlz31Builder<T>::writeLevelData(CHdLevel31* pLevel)
//	{
//		I32 gridSize = 2 << (m_gridLevel + pLevel->m_levelNo);
//
//		//创建下层块集缓存目录
//		char nextBSDir[MAX_PATH] = {0};
//		sprintf_s(nextBSDir, "%s\\level%d", m_savePath.c_str(), pLevel->m_levelNo + 1);
//		_mkdir(nextBSDir);
//
//		U16 iBS = 0;
//		U8  BSNo = 0;
//		EntityIndex nextIndex;
//		// 当前层切分的块集字典表, 当前层写完后替换m_BlocksetFiles
//		std::map<EntityIndex, BlockSetFileInfo31> nextBsFiles;
//	
//		U8* pPtBaseAttri = NULL;
//		HdPtColor* pPtColor = NULL;  
//		//对于每个块集文件，点数小于阈值的直接写入文件，大于阈值的，则继续划分成块，存储在内存中
//		for (auto it = m_BlocksetFiles.begin(); it != m_BlocksetFiles.end(); it++)
//		{
//			if (m_callback != NULL)
//			{	
//				char msg[MAX_PATH] = {0};
//				sprintf_s(msg, "正在写入%s第%d层数据", m_hlzFileName.c_str(), pLevel->m_levelNo);
//				m_callback((float)iBS / m_BlocksetFiles.size(), msg);
//			}
//
//			U64 count = it->second.numPoint;
//			if (count == 0)
//			{
//				iBS++;
//				continue;
//			}
//			
//			//计算所属下层块集的编号及子空间序号
//			const EntityIndex& curIndex = it->first;
//			getNextLevelBSIndex(curIndex, nextIndex, BSNo);
//
//			//对应下层块集缓存文件名
//			char nextBSPath[MAX_PATH] = {0};
//			sprintf_s(nextBSPath, "%s\\blockset_%04d_%04d_%04d.tmp", nextBSDir, nextIndex.xNo, nextIndex.yNo, nextIndex.zNo);
//			//加入到下层块集中
//			if(nextBsFiles.count(nextIndex) == 0)
//			{
//				BlockSetFileInfo31 bsinfo;
//				bsinfo.path = nextBSPath;
//				nextBsFiles.insert(make_pair(nextIndex, bsinfo));
//			}
//			
//			BlockSetFileInfo31& nextBS = nextBsFiles[nextIndex];
//
//			CHdVector3di centerBS = it->second.box.getCenter();
//			CHdVector3di halfSzBS = it->second.box.getExtent() / 2;
//
//			// 块集对象
//			CHdBlockset31* pBlockSet = new CHdBlockset31;
//			HdBlockset31& blkset31 = pBlockSet->m_blockSet;
//			pBlockSet->m_nBlockSetNo = iBS;
//			blkset31.XNo = curIndex.xNo;
//			blkset31.YNo = curIndex.yNo;
//			blkset31.ZNo = curIndex.zNo;
//			blkset31.setHasColor(T::hasColor());
//
//#ifdef _LOG
//			CHdBox3dd blksetBox;
//			blksetBox.MinEdge = CHdVector3dd(curIndex.xNo*gridSize, curIndex.yNo*gridSize, curIndex.zNo*gridSize);
//			blksetBox.MaxEdge = CHdVector3dd((curIndex.xNo+1)*gridSize, (curIndex.yNo+1)*gridSize, (curIndex.zNo+1)*gridSize);
//			CHdVector3dd mid = blksetBox.getCenter();
//			CHdVector3dd half = blksetBox.getExtent() / 2.0;
//#endif
//
//			iBS++;
//			pBlockSet->m_levelNo = pLevel->m_levelNo;
//			pLevel->AddBlockSetRec(pBlockSet);
//
//			//一个块集文件
//			/****************第一种情况**********************/
//			if (count <= m_entity_point_threshold)
//			{//块集的中的总点数小于64000，直接写入文件 
//				HANDLE hFile = CreateFile(it->second.path.c_str(),
//					GENERIC_READ, 
//					FILE_SHARE_READ, 
//					NULL, 
//					OPEN_EXISTING, 
//					FILE_ATTRIBUTE_NORMAL, 
//					NULL);
//				if (hFile == INVALID_HANDLE_VALUE)
//				{
//					continue;
//				}
//
//				// 读取到内存
//				std::vector<T> vecBuf;
//				try
//				{
//					vecBuf.resize(count);
//				}
//				catch(...)
//				{
//					::MessageBox(NULL, "内存分配出错，请提高电脑配置或者使用“临时文件方式”", NULL, MB_OK);
//					// 删除临时文件
//					RemoveTempFiles(m_savePath.c_str());
//					return 0;
//				}
//				
//				DWORD numRead;
//				ReadFile(hFile, vecBuf._Myfirst, count*sizeof(T), &numRead, NULL);
//				// 关闭块集文件并删除块集缓存文件
//				CloseHandle(hFile);
//				remove(it->second.path.c_str());
//				//抽稀到对应下层块集缓存文件
//				simplePoints(vecBuf, BSNo, nextBS);
//
//				// 块集内有数据情况下,需要单独更新点数
//				blkset31.setBlockNum(0);
//				blkset31.numPoint = count;
//
//				std::vector<T*>  sortedPts;
//				std::vector<SubEntityIndex>  subIndices;
//				//子空间划分，对vecBuf重新排序，并获得子空间实体索引
//				subDivide(vecBuf, subIndices, sortedPts);
//				//子空间实体索引个数存入块集结构体中
//				blkset31.subNum = subIndices.size();
//
//				U32 cmpLen = 0;
//				U32 baseAttriLen = 0;
//				bool compress = m_pHlzWrite->m_header.isCompress;
//
//				U32 cntWrite = SplitPt2Buffers(subIndices, sortedPts._Myfirst, count,  compress, &pPtBaseAttri,  baseAttriLen, cmpLen, &pPtColor);
//				FreeIndexBuffer(subIndices, sortedPts);
//				blkset31.numPoint = cntWrite;	
//				m_pHlzWrite->WriteBlockSetData(blkset31, pPtBaseAttri, baseAttriLen, pPtColor, blkset31.numPoint, cmpLen);
//
//#ifdef _LOG
//				CHdBox3dd realBox;
//				GetBoundBox(curIndex.xNo, curIndex.yNo, curIndex.zNo, pLevel->m_levelNo, vecBuf, realBox);
//				if(!realBox.isFullInside(blksetBox))
//				{
//					CHdVector3dd min = realBox.MinEdge - blksetBox.MinEdge;
//					CHdVector3dd max = blksetBox.MaxEdge - realBox.MinEdge;
//					fprintf(m_logFile, "[LEVEL%d][BLOCKSET]: minX = %.3f, minY = %.3f, minZ = %.3f, maxX = %.3f, maxY = %.3f, maxZ = %.3f\n",
//						pLevel->m_levelNo, min.X, min.Y, min.Z, max.X, max.Y, max.Z);
//				}
//#endif
//
//				// 统计总块数
//				m_pHlzWrite->m_header.number_of_col++;
//
//				SAFE_DELETE_ARY(pPtBaseAttri);
//				SAFE_DELETE_ARY(pPtColor);
//
//			} // if (count <= m_entity_point_threshold)
//			/****************第二种情况**********************/
//			// 块集中的点数小于阈值， 直接在内存中处理
//			else if (count > m_entity_point_threshold && count <= m_memory_point_throughput)
//			{
//				// 采用二分的思想,对块集点云继续分割为块文件
//				std::vector<BlockFileInfo31> arrayBlock;    //分别存储8个块文件的信息
//				SplitBlockset2Block_Buffer(it->second, arrayBlock, BSNo, nextBS);		
//
//				// 统计总块集数
//				m_pHlzWrite->m_header.number_of_col += arrayBlock.size();
//
//				//对于每个块文件
//				U32 nBlockCount = arrayBlock.size();
//				for (U32 i=0; i < nBlockCount; i++)
//				{
//					BlockFileInfo31& blockFile = *(arrayBlock._Myfirst + i);
//					if(blockFile.numPoint == 0)
//					{
//						continue;
//					}
//
//					//定位到相应的块数据
//					auto iter_block = m_pBlockData.find(blockFile.path.c_str());
//					if (iter_block == m_pBlockData.end())
//					{
//						continue;
//					}
//
//					count = iter_block->second.size();
//					if (count == 0)
//					{
//						continue;
//					}
//
//
//#ifdef _LOG
//					CHdBox3dd blkBox;
//					getBlockBox(mid, half, blkBox, blockFile.index);
//#endif
//
//					// 当前块对象
//					U16 blkIdx = blockFile.index;
//					CHdBlock31* pCurBlock = new CHdBlock31;
//					HdBlock31& block31 = pCurBlock->m_block;
//					pCurBlock->m_bsNo = pBlockSet->m_nBlockSetNo;
//					pCurBlock->m_nBlockNo = blkIdx;
//					block31.setXNo(BlkIdx2BlkNo[blkIdx][0]);
//					block31.setYNo(BlkIdx2BlkNo[blkIdx][1]);
//					block31.setZNo(BlkIdx2BlkNo[blkIdx][2]);
//					block31.setHasColor(T::hasColor());
//					pBlockSet->AddBlockRec(pCurBlock);
//
//					// 当前块文件不需要再切分，直接写入
//					if (count <= m_entity_point_threshold)
//					{
//						block31.numParcel = 0;
//						block31.numPoint = count;
//
//						std::vector<T*>  sortedPts;
//						std::vector<SubEntityIndex>  subIndices;
//						//子空间划分，对vecBuf重新排序，并获得子空间实体索引
//						subDivide(iter_block->second, subIndices, sortedPts);
//						//子空间实体索引个数存入块结构体中
//						block31.subNum = subIndices.size();
//
//						U32 cmpLen = 0;
//						U32 baseAttriLen = 0;
//						bool compress = m_pHlzWrite->m_header.isCompress;
//
//						U32 cntWrite = SplitPt2Buffers(subIndices, sortedPts._Myfirst, count, compress, &pPtBaseAttri, baseAttriLen, cmpLen, &pPtColor);
//						FreeIndexBuffer(subIndices, sortedPts);
//						block31.numPoint = cntWrite;	
//						m_pHlzWrite->WriteBlockData(block31, pPtBaseAttri, baseAttriLen,  pPtColor, block31.numPoint, cmpLen);
//
//#ifdef _LOG
//						CHdBox3dd realBox;
//						GetBoundBox(curIndex.xNo, curIndex.yNo, curIndex.zNo, pLevel->m_levelNo, iter_block->second._Myfirst, count, realBox);
//						if(!realBox.isFullInside(blkBox))
//						{
//							CHdVector3dd min = realBox.MinEdge - blkBox.MinEdge;
//							CHdVector3dd max = blkBox.MaxEdge - realBox.MinEdge;
//							fprintf(m_logFile, "[LEVEL%d][BLOCK]: minX = %.3f, minY = %.3f, minZ = %.3f, maxX = %.3f, maxY = %.3f, maxZ = %.3f\n",
//								pLevel->m_levelNo, min.X, min.Y, min.Z, max.X, max.Y, max.Z);
//						}
//#endif
//
//						SAFE_DELETE_ARY(pPtBaseAttri);
//						SAFE_DELETE_ARY(pPtColor);
//					}
//					// 块文件-------->包文件(八叉树切分)
//					else
//					{
//						// 切分后包的路径
//						std::string parcelsPath = blockFile.path.c_str();//sBlockPath.c_str();
//						parcelsPath = parcelsPath.substr(0, parcelsPath.find_last_of('.'));
//						// 将块切分为包
//						std::vector<ParcelFileInfo31> splitParcels;
//						SplitBlock2Parcel_Buffer(blockFile.box, blockFile.path.c_str(), parcelsPath.c_str(), splitParcels);
//
//						// 将切分后包文件的点云写入
//						U16 iPcl = 0;
//						for (auto it = splitParcels.begin(); it != splitParcels.end(); it++)
//						{
//							if ((*it).numPoint == 0 )
//							{
//								continue;
//							}
//
//							CHdParcel31* parcel = new CHdParcel31;
//							HdParcel31& pcl31 = parcel->m_parcel;
//							parcel->m_nParcelNo = iPcl++;
//							parcel->m_bkNo = pCurBlock->m_nBlockNo;
//							pcl31.numPoint = (*it).numPoint;
//
//							U8 divCount = 0;
//							U16 xNo = 0, yNo = 0, zNo = 0;
//							getParcelNo(blockFile.box, (*it).box, divCount, xNo, yNo, zNo);
//							pcl31.XNo = xNo;
//							pcl31.YNo = yNo;
//							pcl31.ZNo = zNo;
//							pcl31.setDivCount(divCount);
//							pcl31.setHasColor(T::hasColor());
//							pCurBlock->AddParcel(parcel);
//
//#ifdef _LOG
//							CHdBox3dd parcelBox;
//							getParcelBox(parcelBox, blkBox, divCount+1, xNo, yNo, zNo);
//#endif
//							const hd::stringc& sParcelPath = (*it).path;
//							auto iter_parcel = m_ParcelData.find(sParcelPath.c_str());
//
//							std::vector<T*>  sortedPts;
//							std::vector<SubEntityIndex>  subIndices;
//							//子空间划分，对包数据重新排序，并获得子空间实体索引
//							subDivide(iter_parcel->second, subIndices, sortedPts);
//							//子空间实体索引个数存入块结构体中
//							pcl31.subNum = subIndices.size();
//
//							U32 cmpLen = 0;
//							U32 baseAttriLen = 0;
//							bool compress = m_pHlzWrite->m_header.isCompress;
//
//							U32 cntWrite = SplitPt2Buffers(subIndices, sortedPts._Myfirst, pcl31.numPoint,  compress, &pPtBaseAttri, baseAttriLen, cmpLen, &pPtColor);
//							FreeIndexBuffer(subIndices, sortedPts);
//							pcl31.numPoint = cntWrite;	
//							m_pHlzWrite->WriteParcelData(pcl31, pPtBaseAttri, baseAttriLen, pPtColor, pcl31.numPoint, cmpLen);
//
//#ifdef _LOG
//							CHdBox3dd realBox;
//							GetBoundBox(curIndex.xNo, curIndex.yNo, curIndex.zNo, pLevel->m_levelNo, iter_parcel->second._Myfirst, pcl31.numPoint, realBox);
//							if(!realBox.isFullInside(parcelBox))
//							{
//								CHdVector3dd min = realBox.MinEdge - parcelBox.MinEdge;
//								CHdVector3dd max = blkBox.MaxEdge - parcelBox.MinEdge;
//								fprintf(m_logFile, "[LEVEL%d][PARCEL]: minX = %.3f, minY = %.3f, minZ = %.3f, maxX = %.3f, maxY = %.3f, maxZ = %.3f\n",
//									pLevel->m_levelNo, min.X, min.Y, min.Z, max.X, max.Y, max.Z);
//							}
//#endif
//
//							// 清除包文件，节约内存
//							m_ParcelData.erase(sParcelPath.c_str());
//
//							SAFE_DELETE_ARY(pPtBaseAttri);
//							SAFE_DELETE_ARY(pPtColor);
//						}
//
//						ClearParcelData();
//					}// else分包写入
//
//					pCurBlock->Update();
//					// 将块文件删除，节省磁盘空间
//					m_pBlockData.erase(blockFile.path.c_str());
//				}
//
//				ClearBlocksetData();    //清空块集数据
//				pBlockSet->Update();
//			}
//			/****************第三种情况**********************/
//			//块集中的点数大于阈值，分割成小文件，然后再内存中处理小文件
//			else if (count > m_memory_point_throughput)
//			{
//				// 采用二分的思想,对块集点云继续分割为块文件
//				std::vector<BlockFileInfo31> arrayBlock;
//				SplitBlockFiles(it->second, arrayBlock, BSNo, nextBS);
//
//				// 统计总块集数
//				m_pHlzWrite->m_header.number_of_col += arrayBlock.size();
//
//				// 对于每个块文件，写入或者递归切分
//				U32 blockCount = arrayBlock.size();
//				for (U32 i=0; i < blockCount; i++)
//				{
//					BlockFileInfo31& blockFile = *(arrayBlock._Myfirst + i);
//					count = blockFile.numPoint;
//					if(blockFile.numPoint == 0)
//						continue;
// 					
//#ifdef _LOG
//					CHdBox3dd blkBox;
//					getBlockBox(mid, half, blkBox, blockFile.index);
//#endif
//					// 当前块对象
//					U16 blkIdx = blockFile.index;
//					CHdBlock31* pCurBlock = new CHdBlock31;
//					HdBlock31& block31 = pCurBlock->m_block;
//					pCurBlock->m_bsNo = pBlockSet->m_nBlockSetNo;
//					pCurBlock->m_nBlockNo = blkIdx;
//					block31.setXNo(BlkIdx2BlkNo[blkIdx][0]);
//					block31.setYNo(BlkIdx2BlkNo[blkIdx][1]);
//					block31.setZNo(BlkIdx2BlkNo[blkIdx][2]);
//					block31.setHasColor(T::hasColor());
//					pBlockSet->AddBlockRec(pCurBlock);
//
//					if (count <= m_entity_point_threshold)
//					{
//						HANDLE hBlockFile = CreateFile(blockFile.path.c_str(),
//							GENERIC_READ,
//							FILE_SHARE_READ,
//							NULL,
//							OPEN_EXISTING,
//							FILE_ATTRIBUTE_NORMAL,
//							NULL);
//						if (hBlockFile == INVALID_HANDLE_VALUE)
//						{
//							continue;
//						}
//
//						//块文件无需切分，直接写入
//						block31.numParcel = 0;
//						block31.numPoint = count;
//
//						// 当前块文件不需要再切分
//						vector<T> vecBuf;
//						vecBuf.resize(count);
//
//						// 读取文件
//						DWORD numRead;
//						ReadFile(hBlockFile, vecBuf._Myfirst, sizeof(T)*count, &numRead, NULL);
//
//						std::vector<T*>  sortedPts;
//						std::vector<SubEntityIndex>  subIndices;
//						//子空间划分，对vecBuf重新排序，并获得子空间实体索引
//						subDivide(vecBuf, subIndices, sortedPts);
//						//子空间实体索引个数存入块结构体中
//						block31.subNum = subIndices.size();
//
//						U32 cmpLen = 0;
//						U32 baseAttriLen = 0;
//						bool compress = m_pHlzWrite->m_header.isCompress;
//
//						U32 cntWrite = SplitPt2Buffers(subIndices, sortedPts._Myfirst, count, compress, &pPtBaseAttri, baseAttriLen, cmpLen, &pPtColor);
//						FreeIndexBuffer(subIndices, sortedPts);
//						block31.numPoint = cntWrite;	
//						m_pHlzWrite->WriteBlockData(block31, pPtBaseAttri, baseAttriLen, pPtColor, block31.numPoint, cmpLen);
//
//#ifdef _LOG
//						CHdBox3dd realBox;
//						GetBoundBox(curIndex.xNo, curIndex.yNo, curIndex.zNo, pLevel->m_levelNo, vecBuf, realBox);
//						if(!realBox.isFullInside(blkBox))
//						{
//							CHdVector3dd min = realBox.MinEdge - blkBox.MinEdge;
//							CHdVector3dd max = blkBox.MaxEdge - realBox.MinEdge;
//							fprintf(m_logFile, "[LEVEL%d][BLOCK]: minX = %.3f, minY = %.3f, minZ = %.3f, maxX = %.3f, maxY = %.3f, maxZ = %.3f\n",
//								pLevel->m_levelNo, min.X, min.Y, min.Z, max.X, max.Y, max.Z);
//						}
//#endif
//
//						SAFE_DELETE_ARY(pPtBaseAttri);
//						SAFE_DELETE_ARY(pPtColor);
//						
//						if (hBlockFile)
//						{
//							CloseHandle(hBlockFile);
//							hBlockFile = NULL;
//						}
//
//						vecBuf.clear();
//						vecBuf.swap(vector<T>());
//						// 删除块文件
//						int err = remove(blockFile.path.c_str());
//						if (err == -1)
//						{
//							string strError = "无法删除文件：" ;
//							strError = strError + blockFile.path.c_str();
//							perror(strError.c_str());
//						}
//					} // if(count < m_entity_point_threshold)
//					//块文件先切分成子块，每个子块的点数小于阈值，然后在内存中对子块进行递归分包
//					else
//					{
//						//块文件递归切割成子块
//						vector<subBlockFileInfo31> vecSubBlock;
//						//SplitBlock2Parts(pCurBlock->m_block.box, blockFile.path.c_str(), vecSubBlock);
//						SplitBlock2Parts(blockFile.box, blockFile.path.c_str(), vecSubBlock);
//
//						//对每个子块进行分包并写入
//						for (auto it_subBlk = vecSubBlock.begin(); it_subBlk != vecSubBlock.end(); it_subBlk++)
//						{
//							vector<ParcelFileInfo31> vecParcelInfo;
//							//子块分包
//							//SplitSbkFile2Parcel_Buffer(pCurBlock->m_block.box, it_subBlk->path.c_str(), vecParcelInfo);
//							SplitSbkFile2Parcel_Buffer(it_subBlk->box, it_subBlk->path.c_str(), vecParcelInfo);
//							// 将切分后包文件的点云写入
//							U16 iPcl = 0;
//							for (auto it_parcel = vecParcelInfo.begin(); it_parcel != vecParcelInfo.end(); it_parcel++)
//							{
//								if ((*it_parcel).numPoint == 0 )
//								{
//									continue;
//								}
//
//								CHdParcel31* parcel = new CHdParcel31;
//								HdParcel31& pcl31 = parcel->m_parcel;
//								iPcl = pCurBlock->m_pListParcel.size() + 1;
//								parcel->m_nParcelNo = iPcl;
//								parcel->m_bkNo = pCurBlock->m_nBlockNo;
//								pcl31.numPoint = (*it_parcel).numPoint;
//
//								U8 divCount = 0;
//								U16 xNo = 0, yNo = 0, zNo = 0;
//								getParcelNo(blockFile.box, (*it_parcel).box, divCount, xNo, yNo, zNo);
//								pcl31.XNo = xNo;
//								pcl31.YNo = yNo;
//								pcl31.ZNo = zNo;
//								pcl31.setDivCount(divCount);
//								pcl31.setHasColor(T::hasColor());
//								pCurBlock->AddParcel(parcel);
//
//#ifdef _LOG
//								CHdBox3dd parcelBox;
//								getParcelBox(parcelBox, blkBox, divCount+1, xNo, yNo, zNo);
//#endif
//
//								const hd::stringc& sParcelPath = (*it_parcel).path;
//								auto iter_parcel = m_ParcelData.find(sParcelPath.c_str());
//
//								std::vector<T*>  sortedPts;
//								std::vector<SubEntityIndex>  subIndices;
//								//子空间划分，对包数据重新排序，并获得子空间实体索引
//								subDivide(iter_parcel->second, subIndices, sortedPts);
//								//子空间实体索引个数存入包结构体中
//								pcl31.subNum = subIndices.size();
//
//								U32 cmpLen = 0;
//								U32 baseAttriLen = 0;
//								bool compress = m_pHlzWrite->m_header.isCompress;
//
//								U32 cntWrite = SplitPt2Buffers(subIndices, sortedPts._Myfirst, pcl31.numPoint, compress, &pPtBaseAttri, baseAttriLen, cmpLen, &pPtColor);
//								FreeIndexBuffer(subIndices, sortedPts);
//								pcl31.numPoint = cntWrite;	
//								m_pHlzWrite->WriteParcelData(pcl31, pPtBaseAttri, baseAttriLen, pPtColor, pcl31.numPoint);
//#ifdef _LOG
//								CHdBox3dd realBox;
//								GetBoundBox(curIndex.xNo, curIndex.yNo, curIndex.zNo, pLevel->m_levelNo, iter_parcel->second._Myfirst, pcl31.numPoint, realBox);
//								if(!realBox.isFullInside(parcelBox))
//								{
//									CHdVector3dd min = realBox.MinEdge - parcelBox.MinEdge;
//									CHdVector3dd max = parcelBox.MaxEdge - realBox.MinEdge;
//									fprintf(m_logFile, "[LEVEL%d][PARCEL]: minX = %.3f, minY = %.3f, minZ = %.3f, maxX = %.3f, maxY = %.3f, maxZ = %.3f\n",
//										pLevel->m_levelNo, min.X, min.Y, min.Z, max.X, max.Y, max.Z);
//								}
//#endif
//
//								// 清除包文件，节约内存
//								m_ParcelData.erase(sParcelPath.c_str());
//
//								SAFE_DELETE_ARY(pPtBaseAttri);
//								SAFE_DELETE_ARY(pPtColor);
//							} // 写入分包
//
//							//更新块信息
//							pCurBlock->Update();
//
//							// 回收内存
//							vecParcelInfo.clear();
//							vecParcelInfo.swap(vector<ParcelFileInfo31>());
//							ClearParcelData();
//							ClearBlocksetData();
//
//						} // 块切分成子块
//						// 回收内存
//						vecSubBlock.clear();
//						vecSubBlock.swap(vector<subBlockFileInfo31>());
//
//						int err = remove(blockFile.path.c_str());
//						if (err == -1)
//						{
//							string strError = "无法删除文件：" ;
//							strError = strError + blockFile.path.c_str();
//							perror(strError.c_str());
//						}
//
//					}  // else (count > m_entity_point_threshold)
//
//				} // for (U32 i=0; i < arrayBlock.size(); i++)
//
//				// 回收内存
//				arrayBlock.clear();
//				arrayBlock.swap(vector<BlockFileInfo31>());
//
//			}  // else if (count > m_memory_point_throughput)
//
//			pBlockSet->Update();
//		}
//
//		pLevel->Update();
//		m_BlocksetFiles = nextBsFiles;
//
//		return pLevel->m_level.pointNum;
//	}
//
//	template <class T>
//	void CHlz31Builder<T>::Close()
//	{
//		SAFE_DELETE_PTR(m_pHlzWrite);
//
//		for(auto it = m_pLevels.begin(); it != m_pLevels.end(); ++it)
//		{
//			SAFE_DELETE_PTR(it->second);
//		}
//		m_pLevels.clear();
//	}
//
//	template <class T>
//	void CHlz31Builder<T>::GetBoundBox(I32 xNo, I32 yNo, I32 zNo, U8 level, const std::vector<T>& pts, CHdBox3dd& realBox)
//	{
//		I32 gridSize = 2 << (m_gridLevel + level);
//		F64 xFrom = xNo * gridSize;
//		F64 yFrom = yNo * gridSize;
//		F64 zFrom = zNo * gridSize;
//
//		F64 xMin, yMin, zMin, xMax, yMax, zMax;
//		xMin = yMin = zMin = F64_MAX;
//		xMax = yMax = zMax = F64_MIN;
//		//fprintf(m_logFile, "====================Begin Coordinate===========================\r\n");
//		for(auto it = pts.begin(); it != pts.end(); it++)
//		{
//			T pt = *it;
//			F64 x = pt.x * COORD_REF * gridSize + xFrom;
//			F64 y = pt.y * COORD_REF * gridSize + yFrom;
//			F64 z = pt.z * COORD_REF * gridSize + zFrom;
//			xMin = MIN(x, xMin);
//			yMin = MIN(y, yMin);
//			zMin = MIN(z, zMin);
//			xMax = MAX(x, xMax);
//			yMax = MAX(y, yMax);
//			zMax = MAX(z, zMax);
//			//fprintf(m_logFile, "%.4f,%.4f,%.4f\n", x, y, z);
//		}
//        realBox.MinEdge = CHdVector3dd(xMin, yMin, zMin);
//		realBox.MaxEdge = CHdVector3dd(xMax, yMax, zMax);
//		//fprintf(m_logFile, "====================End Coordinate===========================\r\n\r\n");
//	}
//
//	template <class T>
//	void CHlz31Builder<T>::GetBoundBox(I32 xNo, I32 yNo, I32 zNo, U8 level, T* pts[], int count, CHdBox3dd& realBox)
//	{
//		I32 gridSize = 2 << (m_gridLevel + level);
//		F64 xFrom = xNo * gridSize;
//		F64 yFrom = yNo * gridSize;
//		F64 zFrom = zNo * gridSize;
//
//		F64 xMin, yMin, zMin, xMax, yMax, zMax;
//		xMin = yMin = zMin = F64_MAX;
//		xMax = yMax = zMax = F64_MIN;
//		//fprintf(m_logFile, "====================Begin Coordinate===========================\r\n");
//		for(int i=0; i<count; i++)
//		{
//			T* pt = pts[i];
//			F64 x = pt->x * COORD_REF * gridSize + xFrom;
//			F64 y = pt->y * COORD_REF * gridSize + yFrom;
//			F64 z = pt->z * COORD_REF * gridSize + zFrom;
//			xMin = MIN(x, xMin);
//			yMin = MIN(y, yMin);
//			zMin = MIN(z, zMin);
//			xMax = MAX(x, xMax);
//			yMax = MAX(y, yMax);
//			zMax = MAX(z, zMax);
//			//fprintf(m_logFile, "%.4f,%.4f,%.4f\n", x, y, z);
//		}
//		realBox.MinEdge = CHdVector3dd(xMin, yMin, zMin);
//		realBox.MaxEdge = CHdVector3dd(xMax, yMax, zMax);
//		//fprintf(m_logFile, "====================End Coordinate===========================\r\n\r\n");
//	}
//
//	template <class T>
//	void CHlz31Builder<T>::GetConvertConfig()
//	{
//		char path[MAX_PATH] = {0};
//		GetModuleFileName(NULL, path, MAX_PATH);
//		string iniPath = path;
//		iniPath = iniPath.substr(0, iniPath.find_last_of("\\")) + "\\cvt.ini";
//		m_entity_point_threshold = GetPrivateProfileInt("Threshold", "EntityMaxPointNum", 10000, iniPath.c_str());
//		m_level_point_threshold = GetPrivateProfileInt("Threshold", "LevelMinPointNum", 1000000, iniPath.c_str());
//		m_memory_point_throughput = GetPrivateProfileInt("Threshold", "MemoryMaxPointNum", 30000000, iniPath.c_str());
//		m_sample_size = GetPrivateProfileInt("Simple", "SampleSize", 120000, iniPath.c_str());
//		m_sample_bat_num= GetPrivateProfileInt("Simple", "SampleBatNum", 20, iniPath.c_str());
//		m_sample_gran= GetPrivateProfileInt("Simple", "SampleGran", 5, iniPath.c_str());
//	}
//
//	//清空包文件
//	template <class T>
//	void CHlz31Builder<T>::ClearParcelData()
//	{
//		if (!m_ParcelData.empty())
//		{
//			for (auto it = m_ParcelData.begin(); it != m_ParcelData.end(); it++)
//			{
//				it->second.clear();
//				it->second.swap(vector<T*>());
//			}
//			m_ParcelData.clear();
//		}
//	}
//
//	//!清空临时块集文件的内存空间
//	template <class T>
//	void CHlz31Builder<T>::ClearBlocksetData()
//	{
//		if (!m_blsDataBuf.empty())
//		{
//			m_blsDataBuf.clear();
//			m_blsDataBuf.swap(vector<T>());
//		}
//		if(!m_pBlockData.empty())
//		{
//			for (auto it = m_pBlockData.begin(); it != m_pBlockData.end(); it ++)
//			{
//				it->second.clear();
//				it->second.swap(vector<T*>());  //使用swap函数来释放vector的空间
//			}
//			m_pBlockData.clear();
//		}
//	}
//
//	template <class T>
//	void CHlz31Builder<T>::ClearSplitMemBuf(vector<DivideMemoryBuf<T>> & vecMemBuf)
//	{
//		for (auto it = vecMemBuf.begin(); it != vecMemBuf.end(); it ++)
//		{
//			it->clear();
//		}
//
//		vecMemBuf.clear();
//		vecMemBuf.swap(vector<DivideMemoryBuf<T>>());
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::RemoveTempFiles(const char* path)
//	{
//		WIN32_FIND_DATAA finddata;
//		HANDLE hFind;
//
//		char * pdir=new char[strlen(path)+10];
//		strcpy(pdir,path);
//		if(path[strlen(path)-1]!='\\')
//			strcat(pdir,"\\*.*");
//		else
//			strcat(pdir,"*.*");
//
//		hFind = FindFirstFileA(pdir,&finddata);
//		if(hFind == INVALID_HANDLE_VALUE)
//			return false;
//
//		SAFE_DELETE_ARY(pdir);
//		do
//		{
//			pdir = new char[strlen(path)+strlen(finddata.cFileName)+2];
//			sprintf(pdir,"%s\\%s",path,finddata.cFileName);
//			if(strcmp(finddata.cFileName,".")==0
//				||strcmp(finddata.cFileName,"..")==0)
//			{
//				RemoveDirectoryA(pdir);
//				SAFE_DELETE_ARY(pdir);
//				continue;
//			}
//
//			if((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
//				DeleteFileA(pdir);
//			else
//				RemoveTempFiles(pdir);
//			SAFE_DELETE_ARY(pdir);
//		}while(FindNextFileA(hFind,&finddata));
//
//		FindClose(hFind);
//		return RemoveDirectoryA(path);
//	}
//
//	//! 删除块集文件
//	template <class T>
//	BOOL CHlz31Builder<T>::RemoveBSFiles(std::map<U64, BlockSetFileInfo31>& bsFiles)
//	{
//		for (auto it = m_BlocksetFiles.begin(); it != m_BlocksetFiles.end(); it++)
//		{	
//			remove((it->second).path.c_str());
//		}
//
//		return TRUE;
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::StatIntensity(IHLSReader* pHlsReader, U16& min, U16& max)
//	{
//		if (NULL == pHlsReader)
//		{
//			return false;
//		}
//
//		CLoopIndex* pLoopIndex = pHlsReader->GetLoopIndex();
//		U32 loopCount = pHlsReader->GetLoopCount();
//		for (U32 i = 0;i < loopCount;i++)
//		{
//			min = MIN(pLoopIndex[i].m_loopIdx.minIntensity,min);
//			max = MAX(pLoopIndex[i].m_loopIdx.maxIntensity,max);
//		}
//
//		return true;
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::CalHlsExtent(hd::IHLSReader* reader, double& x_min, double& x_max,double& y_min,double& y_max,double& z_min, double& z_max)
//	{
//		if(NULL == reader)
//		{
//			return false;
//		}
//
//		// 首先利用文件头来取得最大值、最小值、
//		double minX,maxX,minY,maxY,minZ,maxZ;
//		reader->m_header.getGlobalExtent(minX, minY, minZ, maxX, maxY, maxZ);
//
//		x_min = MIN(x_min, MIN(minX, maxX));
//		y_min = MIN(y_min, MIN(minY, maxY));
//		z_min = MIN(z_min, MIN(minZ, maxZ));
//
//		x_max = MAX(x_max, MAX(maxX, minX));
//		y_max = MAX(y_max, MAX(maxY, minY));
//		z_max = MAX(z_max, MAX(maxZ, minZ));
//
//		//如果头文件中最大值、最小值都为0
//		// 那么对数据进行遍历分析，得到最大值、最小值
//		if ( x_min ==  x_max && y_min ==  y_max &&
//			x_min == 0 &&  y_max == 0 )
//		{
//
//			double m[16];
//			reader->m_header.computeMatrix(m);
//
//			hdVector<PointXYZIPRGBA> vecPts;
//
//			double xyztmp[3];
//			double xyzmin[3];  // 最小值
//			double xyzmax[3];  // 最大值
//
//			int flag = 0; // 用于标记第一次初始化xyz。
//			int loopCount = reader->GetLoopCount();
//			for (int n = 0;n < loopCount;n++)
//			{
//				// 从文件读取坐标,n是文件中的点序号
//				if(!reader->ReadLoop(vecPts,n))
//					continue;
//
//				int ptNum = vecPts.size();
//				for (int k = 0;k < ptNum;k++)
//				{
//					PointXYZIPRGBA& pt = *(vecPts._Myfirst + k);
//					if(!pt.isValid())
//						continue;
//
//					xyztmp[0]  =  pt.x;
//					xyztmp[1]  =  pt.y;
//					xyztmp[2]  =  pt.z;
//					// 获得全局坐标
//					hdHomogeneousTransformPoint(m,xyztmp[0],xyztmp[1],xyztmp[2]);
//					flag++;
//
//					// 第一次赋值
//					if ( flag == 1)
//					{
//						VecCopy3fv(xyzmin, xyztmp);
//						VecCopy3fv(xyzmax, xyztmp);
//					}
//					// 进行数据更新
//					else
//					{
//						VecUpdateMinMax3dv(xyzmin, xyzmax, xyztmp);
//					}
//				}
//			}
//
//			// 重新更新m_boundBox最大值和最小值
//			x_min = MIN(x_min, xyzmin[0]);
//			y_min = MIN(y_min, xyzmin[1]);
//			z_min = MIN(z_min, xyzmin[2]);
//			x_max = MAX(x_max, xyzmax[0]);
//			y_max = MAX(y_max, xyzmax[1]);
//			z_max = MAX(z_max, xyzmax[2]);
//		}
//
//		return true;
//	}
//
//	template <class T>
//	void CHlz31Builder<T>::getPtBlockIndex(T& pt, const CHdVector3di& center, U8& index)
//	{	
//		if (pt.x < center.X)
//		{
//			if (pt.y < center.Y)
//			{
//				if (pt.z < center.Z)
//				{
//					index = 0;		// 第0块
//				}
//				else
//				{
//					index = 4;		// 第4块
//				}
//			}
//			else
//			{
//				if (pt.z < center.Z)
//				{
//					index = 2;		// 第2块
//				}
//				else
//				{
//					index = 6;		// 第6块
//				}
//			}
//		}
//		else if (pt.x >= center.X)
//		{
//			if (pt.y < center.Y)
//			{
//				if (pt.z < center.Z)
//				{
//					index = 1;		// 第1块
//				}
//				else
//				{
//					index = 5;		// 第5块
//				}
//			}
//			else
//			{
//				if (pt.z < center.Z)
//				{
//					index = 3;		// 第3块
//				}
//				else
//				{
//					index = 7;		// 第7块
//				}
//			}
//		}
//	}
//
//	template <class T> template<class X>
//	void CHlz31Builder<T>::getBlockBox(const CHdVector3d<X>& mid, const CHdVector3d<X>& halfSize,	CHdBox3d<X>& box,	U16 index)
//	{
//		switch (index)
//		{
//		case 0:
//			{
//				box.MinEdge = mid - halfSize;
//				box.MaxEdge = mid;
//			}
//			break;
//		case 1:
//			{
//				box.MinEdge.X = mid.X;
//				box.MinEdge.Y = mid.Y - halfSize.Y;
//				box.MinEdge.Z = mid.Z - halfSize.Z;
//
//				box.MaxEdge = box.MinEdge + halfSize;		
//			}
//			break;
//		case 2:
//			{
//				box.MinEdge.X = mid.X - halfSize.X;
//				box.MinEdge.Y = mid.Y;
//				box.MinEdge.Z = mid.Z - halfSize.Z;
//
//				box.MaxEdge = box.MinEdge + halfSize;
//			}
//			break;
//		case 3:
//			{
//				box.MinEdge.X = mid.X;
//				box.MinEdge.Y = mid.Y;
//				box.MinEdge.Z = mid.Z - halfSize.Z;
//
//				box.MaxEdge = box.MinEdge + halfSize;
//			}
//			break;
//		case 4:
//			{
//				box.MinEdge.X = mid.X - halfSize.X;
//				box.MinEdge.Y = mid.Y - halfSize.Y;
//				box.MinEdge.Z = mid.Z;
//
//				box.MaxEdge = box.MinEdge + halfSize;
//			}
//			break;
//		case 5:
//			{
//				box.MinEdge.X = mid.X;
//				box.MinEdge.Y = mid.Y - halfSize.Y;
//				box.MinEdge.Z = mid.Z;
//
//				box.MaxEdge = box.MinEdge + halfSize;
//			}
//			break;
//		case 6:
//			{
//				box.MinEdge.X = mid.X - halfSize.X;
//				box.MinEdge.Y = mid.Y;
//				box.MinEdge.Z = mid.Z;
//
//				box.MaxEdge = box.MinEdge + halfSize;
//			}
//			break;
//		case 7:
//			{
//				box.MinEdge = mid;
//
//				box.MaxEdge = box.MinEdge + halfSize;			
//			}
//			break;
//		default:
//			break;
//		}
//	}
//
//	template <class T>
//	void CHlz31Builder<T>::getParcelNo(const CHdBox3di& blockBox, const CHdBox3di& parcelBox, U8& divCount, U16& xNo, U16& yNo, U16& zNo)
//	{
//		divCount = 0;
//		U32 blockExtent = blockBox.getExtent().X / 2;
//		U32 parcelExtent = parcelBox.getExtent().X;
//
//		while(blockExtent > parcelExtent) 
//		{
//			blockExtent /= 2;
//			divCount++;
//		}
//
//		U32 xOffset = parcelBox.MinEdge.X - blockBox.MinEdge.X;
//		U32 yOffset = parcelBox.MinEdge.Y - blockBox.MinEdge.Y;
//		U32 zOffset = parcelBox.MinEdge.Z - blockBox.MinEdge.Z;
//
//		xNo = (U16)(xOffset / parcelExtent);
//		yNo = (U16)(yOffset / parcelExtent);
//		zNo = (U16)(zOffset / parcelExtent);
//	}
//
//	template <class T>
//	void CHlz31Builder<T>::getNextLevelBSIndex(const EntityIndex& curIndex, EntityIndex& nextIndex, U8& BSNo)
//	{
//		U8 xIndex = (curIndex.xNo % 2 == 0) ? 0 : 1;
//		U8 yIndex = (curIndex.yNo % 2 == 0) ? 0 : 1;
//		U8 zIndex = (curIndex.zNo % 2 == 0) ? 0 : 1;
//
//		BSNo = 4 * zIndex + 2 * yIndex + xIndex;
//
//		nextIndex.xNo = (curIndex.xNo >= 0) ? curIndex.xNo / 2 : (curIndex.xNo - 1) / 2;
//		nextIndex.yNo = (curIndex.yNo >= 0) ? curIndex.yNo / 2 : (curIndex.yNo - 1) / 2;
//		nextIndex.zNo = (curIndex.zNo >= 0) ? curIndex.zNo / 2 : (curIndex.zNo - 1) / 2;
//	}
//
//	template <class T>
//	void CHlz31Builder<T>::getParcelBox(CHdBox3dd& parcelBox, const CHdBox3dd& blockBox, U8 divCount, U16 xNo, U16 yNo, U16 zNo)
//	{
//		F64 extent = blockBox.getExtent().X;
//		while(divCount > 0)
//		{
//			extent /= 2.0;
//			divCount--;
//		}
//
//		F64 xMin = extent * xNo + blockBox.MinEdge.X;
//		F64 yMin = extent * yNo + blockBox.MinEdge.Y;
//		F64 zMin = extent * zNo + blockBox.MinEdge.Z;
//		parcelBox.MinEdge = CHdVector3dd(xMin, yMin, zMin);
//		parcelBox.MaxEdge = CHdVector3dd(xMin+extent, yMin+extent, zMin+extent);
//	}
//
//	//根据当前块集在下级块集中的编号，将当前块集中点坐标转换为下级块集坐标
//	template <class T>
//	bool CHlz31Builder<T>::GetNextLevelPos(const T& prePos, U8 index, T& curPos)
//	{
//		curPos = prePos;
//		switch(index)
//		{
//		case 0:
//			curPos.x = curPos.x >> 1;
//			curPos.y = curPos.y >> 1;
//			curPos.z = curPos.z >> 1;
//			return true;
//		case 1:
//			curPos.x = (curPos.x + 65535) >> 1;
//			curPos.y = curPos.y >> 1;
//			curPos.z = curPos.z >> 1;
//			return true;
//		case 2:
//			curPos.x = curPos.x >> 1;
//			curPos.y = (curPos.y + 65535) >> 1;
//			curPos.z = curPos.z >> 1;
//			return true;
//		case 3:
//			curPos.x = (curPos.x + 65535) >> 1;
//			curPos.y = (curPos.y + 65535) >> 1;
//			curPos.z = curPos.z >> 1;
//			return true;
//		case 4:
//			curPos.x = curPos.x >> 1;
//			curPos.y = curPos.y >> 1;
//			curPos.z = (curPos.z + 65535) >> 1;
//			return true;
//		case 5:
//			curPos.x = (curPos.x + 65535) >> 1;
//			curPos.y = curPos.y >> 1;
//			curPos.z = (curPos.z + 65535) >> 1;
//			return true;
//		case 6:
//			curPos.x = curPos.x >> 1;
//			curPos.y = (curPos.y + 65535) >> 1;
//			curPos.z = (curPos.z + 65535) >> 1;
//			return true;
//		case 7:
//			curPos.x = (curPos.x + 65535) >> 1;
//			curPos.y = (curPos.y + 65535) >> 1;
//			curPos.z = (curPos.z + 65535) >> 1;
//			return true;
//		default:
//			return false;
//		}
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::simplePoints(const std::vector<T>& pts, U8 BSNo, BlockSetFileInfo31& BSInfo)
//	{
//		HANDLE hFile = CreateFile(BSInfo.path.c_str(),
//			GENERIC_WRITE,
//			FILE_SHARE_WRITE, 
//			NULL,
//			OPEN_ALWAYS, 
//			FILE_ATTRIBUTE_NORMAL, 
//			NULL);
//		if(hFile == INVALID_HANDLE_VALUE)
//		{
//			return false;
//		}
//
//		u32 ptCount = min(pts.size(), m_sample_size * m_sample_bat_num);
//		std::vector<T> samplePts;
//		samplePts.reserve(ptCount);
//		Concurrency::concurrent_vector<int> samplePtIndices;
//
//		T curPos, prePos;
//		double  sample_grid_size = pow(2.0, m_sample_gran) - 1;
//		I32  ptNum = pts.size();
//		I32  batNum = (ptNum - 1) / m_sample_size + 1;
//		I32  batBegin = 0, batEnd = 0;
//		while(batEnd < batNum)
//		{
//			batEnd = batBegin + m_sample_bat_num;
//			if(batEnd > batNum)
//			{
//				batEnd = batNum;
//			}
//
//			Concurrency::parallel_for<int>(batBegin, batEnd, [&](int i)
//			{
//				U32 from = i * m_sample_size;
//				U32 to = (i+1) * m_sample_size;
//				if(to > ptNum)
//				{
//					to = ptNum;
//				}
//
//				vector<PosKey>  posKeys;
//				posKeys.resize(to - from);
//
//				U16 dMinX = U16_MAX;
//				U16 dMinY = U16_MAX;
//				U16 dMinZ = U16_MAX;
//				U16 dMaxX = 0;
//				U16 dMaxY = 0;
//				U16 dMaxZ = 0;
//
//				for (U32 n = from; n < to; n++)
//				{
//					T& pt = *(pts._Myfirst + n);
//
//					dMinX = MIN(dMinX,pt.x);
//					dMinY = MIN(dMinY,pt.y);
//					dMinZ = MIN(dMinZ,pt.z);
//					dMaxX = MAX(dMaxX,pt.x);
//					dMaxY = MAX(dMaxY,pt.y);
//					dMaxZ = MAX(dMaxZ,pt.z);
//				}
//
//				// 根据范围及精度确定格网个数
//				int xRows = ((dMaxX - dMinX) >> m_sample_gran) + 1;
//				int yRows = ((dMaxY - dMinY) >> m_sample_gran) + 1;
//				int zRows = ((dMaxZ - dMinZ) >> m_sample_gran) + 1;
//
//				// 格网划分处理
//				for (U32 n = from; n < to; n++)
//				{
//					T& pt = *(pts._Myfirst + n);
//
//					// 计算对应格网索引值
//					int xStep = (pt.x - dMinX) >> m_sample_gran;
//					int yStep = (pt.y - dMinY) >> m_sample_gran;
//					int zStep = (pt.z - dMinZ) >> m_sample_gran;
//
//					double xOffset = fabs(dMinX + (xStep + 0.5) * sample_grid_size - pt.x);
//					double yOffset = fabs(dMinY + (yStep + 0.5) * sample_grid_size - pt.y);
//					double zOffset = fabs(dMinZ + (zStep + 0.5) * sample_grid_size - pt.z);
//
//					// 对应键为
//					U64 mapPin = zStep * (xRows * yRows) + yStep * xRows + xStep;
//
//					// 插入值
//					PosKey posKey;
//					posKey.nIndex = n;
//
//					// 计算获取距离box中心最近点
//
//					posKey.dDist = xOffset + yOffset + zOffset;
//					posKey.gridIndex = mapPin;
//					*(posKeys._Myfirst + n - from) = posKey;
//				}
//
//				// 针对vec进行处理排序
//				std::sort(posKeys.begin(), posKeys.end(), SortByGrid);
//
//				PosKey& pos = *(posKeys._Myfirst + 0);
//				U64 nCurGridPos = pos.gridIndex;
//				samplePtIndices.push_back(pos.nIndex);
//
//				for (U32 nn = 1;nn < to - from; nn++)
//				{
//					PosKey& pos = *(posKeys._Myfirst + nn);
//					if (pos.gridIndex == nCurGridPos)
//					{
//						// 之前已按距离排序，同一格网距离中心最小为最前索引值
//						continue;
//					}
//					else
//					{
//						// 处理到下一个格网时，记录
//						nCurGridPos = pos.gridIndex;
//						samplePtIndices.push_back(pos.nIndex);
//					}
//				}
//			}
//			);
//
//			for(auto it = samplePtIndices.begin(); it != samplePtIndices.end(); ++it)
//			{
//				//将当前块集中抽取点坐标转换为下层块集内坐标
//				prePos = *(pts._Myfirst + *it);
//				GetNextLevelPos(prePos, BSNo, curPos);
//				samplePts.push_back(curPos);
//			}
//
//			// 将抽取的点写入			
//			DWORD reslt;
//			LARGE_INTEGER li, li1;
//			li.HighPart = li.LowPart = 0;
//			li1.HighPart = li1.LowPart = 0;
//			SetFilePointerEx(hFile,li,&li1,FILE_END);
//			WriteFile(hFile, samplePts._Myfirst, sizeof(T)*samplePts.size(), &reslt, NULL);
//
//			BSInfo.numPoint += samplePts.size();
//
//			samplePtIndices.clear();
//			samplePts.clear();
//			batBegin = batEnd;
//		}
//
//		CloseHandle(hFile);
//		hFile = NULL;
//
//		return true;
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::subDivide(std::vector<T>& pts, std::vector<SubEntityIndex>& subIndices, std::vector<T*>& sortedPts)
//	{	
//		EntityIndex index;
//		std::map<EntityIndex, std::vector<T*>> divideResult;
//
//		U8 xNo = 0, yNo = 0, zNo = 0;
//		U16 ptNum = pts.size();
//		for(U16 i=0; i<ptNum; i++)
//		{
//			T& pt = *(pts._Myfirst + i);
//			xNo = pt.x  >> 8;
//			yNo = pt.y  >> 8;
//			zNo = pt.z  >> 8;
//			pt.x -= (U16)xNo << 8;
//			pt.y -= (U16)yNo << 8;
//			pt.z -= (U16)zNo << 8;
//
//			index.xNo = xNo;
//			index.yNo = yNo;
//			index.zNo = zNo;
//
//			if(divideResult.count(index) == 0)
//			{
//				std::vector<T*> pts;
//				pts.reserve(ptNum);
//				divideResult.insert(make_pair(index, pts));
//			}
//
//			divideResult[index].push_back(&pt);
//		}
//
//		U32 i = 0, offset = 0;
//		sortedPts.resize(ptNum);
//		subIndices.resize(divideResult.size());
//		for(auto it = divideResult.begin(); it != divideResult.end(); ++it)
//		{
//			const EntityIndex& entry = it->first;
//			const std::vector<T*>& value = it->second;
//
//			SubEntityIndex& subIndex = *(subIndices._Myfirst + i++);
//			subIndex.xNo = entry.xNo;
//			subIndex.yNo = entry.yNo;
//			subIndex.zNo = entry.zNo;
//			subIndex.ptNum = value.size();
//
//			memcpy(sortedPts._Myfirst + offset, value._Myfirst, value.size() * sizeof(T*));
//			offset += subIndex.ptNum;
//		}
//
//		return true;
//	}
//
//	template <class T>
//	bool CHlz31Builder<T>::subDivide(std::vector<T*>& pts, std::vector<SubEntityIndex>& subIndices, std::vector<T*>& sortedPts)
//	{	
//		EntityIndex index;
//		std::map<EntityIndex, std::vector<T*>> divideResult;
//
//		U8 xNo = 0, yNo = 0, zNo = 0;
//		U16 ptNum = pts.size();
//		for(U16 i=0; i<ptNum; i++)
//		{
//			T*& pt = *(pts._Myfirst + i);
//			xNo = pt->x  >> 8;
//			yNo = pt->y  >> 8;
//			zNo = pt->z  >> 8;
//			pt->x -= (U16)xNo << 8;
//			pt->y -= (U16)yNo << 8;
//			pt->z -= (U16)zNo << 8;
//
//			index.xNo = xNo;
//			index.yNo = yNo;
//			index.zNo = zNo;
//
//			if(divideResult.count(index) == 0)
//			{
//				std::vector<T*> pts;
//				pts.reserve(ptNum);
//				divideResult.insert(make_pair(index, pts));
//			}
//
//			divideResult[index].push_back(pt);
//		}
//
//		U32 i = 0, offset = 0;
//		sortedPts.resize(ptNum);
//		subIndices.resize(divideResult.size());
//		for(auto it = divideResult.begin(); it != divideResult.end(); ++it)
//		{
//			const EntityIndex& entry = it->first;
//			const std::vector<T*>& value = it->second;
//
//			SubEntityIndex& subIndex = *(subIndices._Myfirst + i++);
//			subIndex.xNo = entry.xNo;
//			subIndex.yNo = entry.yNo;
//			subIndex.zNo = entry.zNo;
//			subIndex.ptNum = value.size();
//
//			memcpy(sortedPts._Myfirst + offset, value._Myfirst, value.size() * sizeof(T*));
//			offset += subIndex.ptNum;
//		}
//
//		return true;
//	}
//
//	template <class T>
//	int CHlz31Builder<T>::SplitPt2Buffers(const std::vector<SubEntityIndex>& subIndices, T* ppPtAry[], int count, bool compress, U8** pPtBaseAttri, U32& baseAttriLen, U32& cmpLen, HdPtColor** pPtColor)
//	{
//		return compress ? SplitPt2Buffers_Cmp(subIndices, ppPtAry, count, pPtBaseAttri, baseAttriLen, cmpLen, pPtColor)
//								  : SplitPt2Buffers_Normal(subIndices, ppPtAry, count, pPtBaseAttri, baseAttriLen, pPtColor);
//	}
//
//	template <class T>
//	int CHlz31Builder<T>::SplitPt2Buffers_Normal(const std::vector<SubEntityIndex>& subIndices, T* ppPtAry[], int count, U8** pPtBaseAttri, U32& baseAttriLen, HdPtColor** pPtColor)
//	{
//		if (ppPtAry == NULL || count <= 0)
//		{
//			return 0;
//		}
//
//		//基本属性buffer长度=变长子索引长度+坐标/强度/分类数据长度
//		U32 indexLen = subIndices.size() * sizeof(SubEntityIndex);
//		U32 dataLen = count * (sizeof(XYZ_S) + 2 * sizeof(U8));
//		baseAttriLen = indexLen + dataLen;
//
//		U8* pTmpBaseAttri = new U8[baseAttriLen];
//		(*pPtBaseAttri) = pTmpBaseAttri;
//
//		//1.先写入子索引到基本属性buffer首部
//		memcpy(pTmpBaseAttri, subIndices._Myfirst, indexLen);
//
//		//2.按照坐标、强度、分类顺序写入到基本属性buffer中
//		XYZ_S* pCoord = (XYZ_S*)(pTmpBaseAttri + indexLen);
//		for (int i = 0;i < count;i++)
//		{
//			pCoord[i].x = ppPtAry[i]->x;
//			pCoord[i].y = ppPtAry[i]->y;
//			pCoord[i].z = ppPtAry[i]->z;
//		}
//
//		U8* pInten = (U8*)(pCoord + count);
//		for(int i = 0; i < count; i++)
//		{
//			pInten[i] = ppPtAry[i]->intensity;
//		}
//
//		U8* pProp = pInten + count;
//		for(int i = 0; i < count; i++)
//		{
//			pProp[i] = ppPtAry[i]->prop;
//		}
//
//		//3.再处理扩展属性
//		if(!T::hasColor())
//		{
//			*pPtColor = NULL;
//		}
//		else
//		{
//			HdPtColor* pTmpColors = NULL;
//			pTmpColors = new HdPtColor[count];
//			(*pPtColor) = pTmpColors;
//
//			for (int i = 0;i < count;i++)
//			{
//				pTmpColors[i].color16 = ppPtAry[i]->getColor();
//			}
//		}
//
//		return count;
//	}
//
//	template <class T>
//	int CHlz31Builder<T>::SplitPt2Buffers_Cmp(const std::vector<SubEntityIndex>& subIndices, T* ppPtAry[], int count, U8** pPtBaseAttri, U32& baseAttriLen, U32& cmpLen, HdPtColor** pPtColor)
//	{
//		if(ppPtAry == NULL || count <= 0)
//		{
//			return 0;
//		}
//
//		//1.获取坐标数据
//		XYZ_S* pCoord = new XYZ_S[count];
//		for(int i=0; i < count; i++)
//		{
//			pCoord[i].x = ppPtAry[i]->x;
//			pCoord[i].y = ppPtAry[i]->y;
//			pCoord[i].z = ppPtAry[i]->z;
//		}
//
//		//2.坐标数据压缩
//		CPtIEncoder coordEncoder((HdIntensity*)pCoord, count * 3);
//		coordEncoder.encodeInten();
//		SAFE_DELETE_ARY(pCoord);
//
//		//3.获取压缩长度、基本属性总长度
//		cmpLen = coordEncoder.getBufferSize();
//		U32 indexLen = subIndices.size() * sizeof(SubEntityIndex);
//		baseAttriLen = indexLen + cmpLen + count * sizeof(U8) * 2;
//
//		//4.按照子索引、压缩坐标、强度、分类顺序写入到基本属性buffer中
//		U8* pTmpBaseAttri = new U8[baseAttriLen];
//		(*pPtBaseAttri) = pTmpBaseAttri;
//
//		memcpy(pTmpBaseAttri, subIndices._Myfirst, indexLen);
//		memcpy(pTmpBaseAttri + indexLen, coordEncoder.getBuffer(), cmpLen);
//		coordEncoder.clear();
//
//		U8* pInten = (U8*)(pTmpBaseAttri + indexLen + cmpLen);
//		for(int i = 0; i < count; i++)
//		{
//			pInten[i] = ppPtAry[i]->intensity;
//		}
//
//		U8* pProp = pInten + count;
//		for(int i = 0; i < count; i++)
//		{
//			pProp[i] = ppPtAry[i]->prop;
//		}
//
//		//5.再处理扩展属性
//		if(!T::hasColor())
//		{
//			*pPtColor = NULL;
//		}
//		else
//		{
//			HdPtColor* pTmpColors = NULL;
//			pTmpColors = new HdPtColor[count];
//			(*pPtColor) = pTmpColors;
//
//			for (int i = 0;i < count;i++)
//			{
//				pTmpColors[i].color16 = ppPtAry[i]->getColor();
//			}
//		}
//
//		return count;
//
//	}
//
//	template <class T> template <class X>
//	void CHlz31Builder<T>::FreeSplitBuffer(std::map<EntityIndex, DivideMemoryBuf<X>>& BsPointsArray)
//	{
//		for(auto it = BsPointsArray.begin(); it != BsPointsArray.end(); ++it)
//		{
//			it->second.clear();
//		}
//
//		BsPointsArray.clear();
//	}
//
//	template <class T>
//	void CHlz31Builder<T>::FreeIndexBuffer(std::vector<SubEntityIndex>& subIndices, std::vector<T*>& sortedPts)
//	{
//		subIndices.clear();
//		subIndices.swap(std::vector<SubEntityIndex>());
//
//		sortedPts.clear();
//		sortedPts.swap(std::vector<T*>());
//	}
//}

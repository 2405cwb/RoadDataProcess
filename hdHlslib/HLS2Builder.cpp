#include "HLS2Builder.h"
#include "..\hdHlslib\HLSReader.h"
#include "..\hdHlslib\IHLSReader.h"
#include "..\hdHlslib\HLSReadOpener.h"
#include "..\hdHlslib\inc\lasreader.hpp"
#include "..\hdCommon\point_types.h"
#include "..\hdCommon\hdLandMarkTransform.h"
#include <io.h>
#include <direct.h>
#include <algorithm>
#include <time.h>
#include "..\hdCommon\hdSceneStr.h"
#include <math.h>
#include "vector3d.h"

// 使用::messagebox,否则会报LNK2019 fengjing
#pragma comment(lib,"User32.lib")

#undef IO_BUFFERSIZE
#define IO_BUFFERSIZE	(200000)
#undef PT_BUFFERSIZE
#define PT_BUFFERSIZE	(2 << 12)	//点缓存大小

CHLS2Builder::CHLS2Builder(void)
{
	m_boundBox.MinEdge.set(F64_MAX,F64_MAX,F64_MAX);
	m_boundBox.MaxEdge.set(F64_MIN,F64_MIN,F64_MIN);
	m_totalCount = 0;
	m_maxPtInCube = 1024;
	m_stepX = m_stepY = 0.0;
	m_bufIndex = 0;
	m_blockCount = 0;
	processCallback = NULL;
	m_readPointCnt = 0;
	m_nLoadSimple = 1000000;
	m_bneedcx = FALSE;
	m_3DGridSize = 0.1f;

	m_bSpatialCombine = TRUE;
}


CHLS2Builder::~CHLS2Builder(void)
{
}

//! 设置每个立方盒包含点数
void CHLS2Builder::setMaxCount( int maxPtInCube )
{
	m_maxPtInCube = maxPtInCube;
}

// 计算包围盒
bool CHLS2Builder::calucBoundBox()
{
	if (m_hlsFileList.size() == 0)
	{
		return false;
	}

	m_boundBox.MinEdge.set(F64_MAX,F64_MAX,F64_MAX);
	m_boundBox.MaxEdge.set(F64_MIN,F64_MIN,F64_MIN);
	m_totalCount = 0;

	if (processCallback)
	{
		processCallback(0.0f,HDSCENE_IDS_BUILDER_STAT_RANGE);
	}
	for (unsigned int i = 0;i< m_hlsFileList.size();i++)
	{
		hd::stringc strExt = m_hlsFileList[i].subString(m_hlsFileList[i].size()-3, 3);		
		strExt.make_lower();

		//如果是hls文件
		if (strExt == "hls")
		{
			hd::HLSreader reader;
			BOOL bRet = reader.open(m_hlsFileList[i].c_str());
			if(!bRet)continue;

			m_totalCount += reader.m_header.number_of_point_records;

			double minX,maxX,minY,maxY,minZ,maxZ;
			reader.m_header.getGlobalExtent(minX,minY,minZ,maxX,maxY,maxZ);

			m_boundBox.MinEdge.X = MIN(m_boundBox.MinEdge.X,MIN(minX,maxX));
			m_boundBox.MinEdge.Y = MIN(m_boundBox.MinEdge.Y,MIN(minY,maxY));
			m_boundBox.MinEdge.Z = MIN(m_boundBox.MinEdge.Z,MIN(minZ,maxZ));

			m_boundBox.MaxEdge.X = MAX(m_boundBox.MaxEdge.X,MAX(maxX,minX));
			m_boundBox.MaxEdge.Y = MAX(m_boundBox.MaxEdge.Y,MAX(maxY,minY));
			m_boundBox.MaxEdge.Z = MAX(m_boundBox.MaxEdge.Z,MAX(maxZ,minZ));

			/*calHlsExtnt(m_hlsFileList[i]);*/
			// 蔡红云 2013/10/14 
			// 加入限定条件，如果点云为进行拼接过，则跳出，不进行文件合并
			if (m_boundBox.MinEdge.X == 0.0 && m_boundBox.MinEdge.Y == 0.0 && m_boundBox.MaxEdge.X == 0.0
				&& m_boundBox.MaxEdge.Y == 0.0 )
			{
				return false;
			}

		}
		else if (strExt == "las" )
		{
			LASreadOpener lasreadopener;
			lasreadopener.set_merged(FALSE);
			lasreadopener.set_populate_header(FALSE);
			lasreadopener.set_file_name(m_hlsFileList[i].c_str());

			if (lasreadopener.active())
			{
				LASreader* lasRead = lasreadopener.open();
				if (lasRead == 0)
				{
					continue;
				}

				m_totalCount += lasRead->header.number_of_point_records;
				m_boundBox.MinEdge.X = MIN(m_boundBox.MinEdge.X,lasRead->get_min_x());
				m_boundBox.MinEdge.Y = MIN(m_boundBox.MinEdge.Y,lasRead->get_min_y());
				m_boundBox.MinEdge.Z = MIN(m_boundBox.MinEdge.Z,lasRead->get_min_z());

				m_boundBox.MaxEdge.X = MAX(m_boundBox.MaxEdge.X,lasRead->get_max_x());
				m_boundBox.MaxEdge.Y = MAX(m_boundBox.MaxEdge.Y,lasRead->get_max_y());
				m_boundBox.MaxEdge.Z = MAX(m_boundBox.MaxEdge.Z,lasRead->get_max_z());

				lasRead->close();
				delete lasRead;
				lasRead = NULL;
			}
		}
		// 蔡红云 2013/9/18
		// bin 数据打开头文件计算地面站小坐标数据最大值、最小值
		// 偏差可能会很大。故采用偏移量来替代最小值计算

		else if (strExt ==  "bin")
		{
			LASreadOpener lasreadopener;
			lasreadopener.set_merged(FALSE);
			lasreadopener.set_populate_header(FALSE);
			lasreadopener.set_file_name(m_hlsFileList[i].c_str());

			if (lasreadopener.active())
			{
				LASreader* lasRead = lasreadopener.open();
				if (lasRead == 0)
				{
					continue;
				}

				F64 xmin =0, ymin =0, zmin = 0, xmax = 0, ymax = 0,zmax = 0;
				m_totalCount += lasRead->header.number_of_point_records;

				if (abs(lasRead->header.x_offset) < 10000)
				{

					xmin= lasRead->get_min_x() + lasRead->header.x_offset;
					ymin= lasRead->get_min_y() + lasRead->header.y_offset;
					zmin= lasRead->get_min_z() + lasRead->header.z_offset;
					xmax= lasRead->get_max_x() + lasRead->header.x_offset;
					ymax= lasRead->get_max_y() + lasRead->header.y_offset;
					zmax= lasRead->get_max_z() + lasRead->header.z_offset;
					m_boundBox.MinEdge.X = MIN(m_boundBox.MinEdge.X,xmin);
					m_boundBox.MinEdge.Y = MIN(m_boundBox.MinEdge.Y,ymin);
					m_boundBox.MinEdge.Z = MIN(m_boundBox.MinEdge.Z,zmin);

					m_boundBox.MaxEdge.X = MAX(m_boundBox.MaxEdge.X,xmax);
					m_boundBox.MaxEdge.Y = MAX(m_boundBox.MaxEdge.Y,ymax);
					m_boundBox.MaxEdge.Z = MAX(m_boundBox.MaxEdge.Z,zmax);

				}
				else

				{
					m_boundBox.MinEdge.X = MIN(m_boundBox.MinEdge.X,lasRead->get_min_x());
					m_boundBox.MinEdge.Y = MIN(m_boundBox.MinEdge.Y,lasRead->get_min_y());
					m_boundBox.MinEdge.Z = MIN(m_boundBox.MinEdge.Z,lasRead->get_min_z());

					m_boundBox.MaxEdge.X = MAX(m_boundBox.MaxEdge.X,lasRead->get_max_x());
					m_boundBox.MaxEdge.Y = MAX(m_boundBox.MaxEdge.Y,lasRead->get_max_y());
					m_boundBox.MaxEdge.Z = MAX(m_boundBox.MaxEdge.Z,lasRead->get_max_z());
				}

				lasRead->close();
				delete lasRead;
				lasRead = NULL;
			}
		}
	}

	return true;
}
//! 关闭所有文件
void CHLS2Builder::CloseTempFiles()
{

	for (map<U32,pair<FILE*,string>>::iterator it = m_dicFile.begin();
		it != m_dicFile.end();it++)

	{	
		string strtmp = (it->second).second;
		if ((it->second).first != NULL)
		{
			// 关闭文件，同时把文件指针置空
			fclose((it->second).first);
			(it->second).first = NULL;
		}
	}

}

// 对hls文件进行划分格网
bool CHLS2Builder::normalSplitHlsFile(const char* savePath, hd::stringc& strHlsFile)
{
	//hd::HLSreader reader;
	//BOOL bRet = reader.open(strHlsFile.c_str());
	//if(!bRet) return false;
	// hls 文件读写对象指针
	hd::IHLSReader* reader = NULL;
	// hls 文件打开对象
	hd::CHLSReadOpener readerOpen;
	// 打开hls文件
	reader = readerOpen.Open(strHlsFile.c_str());
	// 打开失败、返回
	if(reader == NULL)
		return false;
	hd::stringc strInfo = HDSCENE_IDS_PROCESSING;
	int nPos = strHlsFile.findLast('\\');
	strInfo += strHlsFile.subString(nPos + 1,strHlsFile.size() - nPos - 1);
	// 进度条开始
	if (processCallback)
	{
		processCallback(0.0f, strInfo.c_str());
	}

	//求step倒数,便于后续使用乘法运行,提高效率
	char filePath[MAX_PATH] = {0};

	// 构造全局坐标转换矩阵
	double m[16];
	reader->m_header.computeMatrix(m);
	
	// 存储点
	hdVector<PointXYZIPRGBA> vecPts;
	
	// 获取圈数
	int loopCount = reader->GetLoopCount();
	
	// 获取圈索引
	reader->GetLoopIndex();
	int	n;
	u32 k;
	
	double x,y,z;// xyz坐标
	s64 toLoadCount = (s64)floor(reader->m_header.number_of_point_records  + 0.5f);
	int  loadSimple = m_nLoadSimple/*1000000*/; // 每个点云100w为阈值
	
	// 抽稀步长
	u32 simpleLevel = 1;

	// 如果需要抽稀，则重新计算抽稀阈值
	if (m_bneedcx)
	{
		simpleLevel = toLoadCount / (loadSimple * 2) + 1;
	}
	
	// 逐圈进行遍历
	for (n = 0; n < loopCount; n += simpleLevel)
	{
		// 从文件读取坐标,n是文件中的点序号
		if(!reader->ReadLoop(vecPts, n))
			continue;
		
		// 逐点进行遍历
		for (k = 0;k < vecPts.size();k +=simpleLevel)
		{
			PointXYZIPRGBA& pt = *(vecPts._Myfirst + k);
			if(!pt.isValid())
				continue;
		
			// 计算所在格网
			x = pt.x;
			y = pt.y;
			z = pt.z;
		
			// 转为全局坐标
			hdHomogeneousTransformPoint(m,x,y,z);
			
			// 进行坐标偏移
			x -= m_boundBox.MinEdge.X;
			y -= m_boundBox.MinEdge.Y;
			z -= m_boundBox.MinEdge.Z;
			

			// 写入内存缓存
			U32 xNo = (U32)(x / m_stepX);
			U32 yNo = (U32)(y / m_stepY);
			U32 fileNo = xNo | (yNo << 16);
			
			/*	FILE*& file = m_dicFile[fileNo];*/
			FILE*& file = m_dicFile[fileNo].first;
			if (file == NULL)
			{
				sprintf(filePath,"%s\\block-%04d_%04d.tmp",savePath,xNo,yNo);
				file = fopen(filePath,"ab+");
				
				// 如果打开文件失败，可能是打开文件过多，需要关闭文件
				if (file == NULL)
				{
					CloseTempFiles();
					file = fopen(filePath,"ab+");
				}
			
				// 更新文件名
				string str(filePath);
				m_dicFile[fileNo].second = str;
			}
			
			// 如果文件为空、返回
			if (file == NULL)
			{
				if (reader)
				{
					delete reader;
					reader = NULL;
				}
				
				return false;
			}
			
			// 更新点坐标
			pt.x = (hd::f32)(x);
			pt.y = (hd::f32)(y);
			pt.z = (hd::f32)(z);
			
			// 写入点
			fwrite(&pt, sizeof(PointXYZIPRGBA), 1, file);

			// 进行空间合并，则写入该点对应的hls文件索引以及该点对应的圈号
			if (m_bSpatialCombine)
			{
				// 写入点所在的圈号
				fwrite(&n, sizeof(int), 1, file);

				hd::u8 nHlsIdx = 0;

				// 写入点对应的点云文件序号
				fwrite(&nHlsIdx, sizeof(hd::u8), 1, file);
			}
		
		}
		
		// 进度控制
		if (processCallback && (n % 10) == 0)
		{
			processCallback((float)n / loopCount,strInfo.c_str());
		}

		vecPts.clear();
	}

	if (reader)
	{
		delete reader;
		reader = NULL;
	}

	// 成功
	return true;
}
// 分割las文件
bool CHLS2Builder::normalSplitLasFile(const char* savePath, hd::stringc& strLasFile)
{
	// las文件打开对象
	LASreadOpener lasreadopener;

	// 设置不合并
	lasreadopener.set_merged(FALSE);
	lasreadopener.set_populate_header(FALSE);
	
	// 设置文件名
	lasreadopener.set_file_name(strLasFile.c_str());
	if (lasreadopener.active())
	{
		// 打开las文件
		LASreader* lasRead = lasreadopener.open();
		
		// 打开失败、返回
		if (lasRead == 0)
		{
			return false;
		}
		hd::stringc strInfo = HDSCENE_IDS_PROCESSING;
		int nPos = strLasFile.findLast('\\');
		strInfo += strLasFile.subString(nPos + 1,strLasFile.size() - nPos - 1);
	
		// 进度控制
		if (processCallback)
		{
			processCallback(0.0f,strInfo.c_str());
		}

		char filePath[MAX_PATH] = {0};
		
		// 支持合并LAS
		U32 n = 0;
		while (lasRead->read_point())
		{
			// 从文件读取坐标,n是文件中的点序号
			F64 x = lasRead->get_x() - m_boundBox.MinEdge.X;
			F64 y = lasRead->get_y() - m_boundBox.MinEdge.Y;
			F64 z = lasRead->get_z() - m_boundBox.MinEdge.Z;
			n++;
			if (processCallback && (n % 10000) == 0)
			{
				processCallback((float)n /lasRead->header.number_of_point_records,strInfo.c_str());
			}
			
			// 写入内存缓存
			//U32 fileNo = (U32)(m_bStripByX ? x / m_step : y / m_step);
			F64 xTmp = x / m_stepX;
			F64 yTmp = y / m_stepY;

			// 如果存在负数时，强制转换为垃圾值，造成fileNo是一个垃圾值，
			// 造成后续删除临时文件删 不干净，故取绝对值，
			U32 xNo = (U32)hd_round32(abs(xTmp));
			U32 yNo = (U32)hd_round32(abs(yTmp));
			U32 fileNo = xNo | (yNo << 16);
			
			/*	FILE*& file = m_dicFile[fileNo];*/
			FILE*& file = m_dicFile[fileNo].first;
			if (file == NULL)
			{
				sprintf(filePath,"%s\\block-%04d_%04d.tmp",savePath,xNo,yNo);
				file = fopen(filePath,"ab+");
			
				// 如果打开文件失败，可能是打开文件过多，需要关闭文件
				if (file == NULL)
				{
					CloseTempFiles();
					file = fopen(filePath,"ab+");
				}
				string str(filePath);
				m_dicFile[fileNo].second = str;

			}
			
			// 如果文件为空、返回
			if (file == NULL)
			{
				// 关闭删除lasRead
				lasRead->close();
				delete lasRead;
				lasRead = NULL;
				return false;
			}
			
			// hls文件点
			PointXYZIPRGBA pt;
			
			// xyz坐标
			pt.x = (float)x;
			pt.y = (float)y;
			pt.z = (float)z;

			pt.prop =  lasRead->point.classification;
			
			// 强度信息
			//pt.intensity = lasRead->point.intensity;
			
			// 强度信息需要转换 hdScene中的有效强度范围是 0-4096 [2014/05/15 危迟]
			pt.intensity = int((lasRead->point.intensity / 65535.0f)* 4095);

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
			
			// 写入点
			fwrite(&pt,sizeof(PointXYZIPRGBA),1,file);
		}
		
		// 关闭删除lasRead
		lasRead->close();
		delete lasRead;
		lasRead = NULL;
		return true;
	}

	// 关闭删除lasRead
	return false;
}

// 文件分割
bool CHLS2Builder::normalSplit(const char* savePath)
{
	// 如果文件个数为0、返回
	if (m_hlsFileList.size() == 0)
	{
		return false;
	}
	// 进度条开始
	if (processCallback)
	{
		processCallback(0.0f, HDSCENE_IDS_BUILDER_DIVIDE_GRID);
	}

	CHdVector3dd extent = m_boundBox.getExtent();
	// 计算块数及步长
	m_blockCount = m_totalCount / IO_BUFFERSIZE + (m_totalCount % IO_BUFFERSIZE == 0 ? 0 : 1);
	m_bStripByX = extent.X >= extent.Y;

	u32 statCount = 1;
	m_stepX = extent.X;
	m_stepY = extent.Y;

	while(statCount < m_blockCount)
	{
		if (m_stepX > m_stepY)
		{
			m_stepX /= 2.0;
		}
		else if (m_stepY > m_stepX )
		{
			m_stepY /= 2.0;
		}

		statCount = (statCount << 1);
	}

	//m_step = MAX(extent.X,extent.Y) / m_blockCount;
	//m_step = MAX(extent.X,extent.Y) / m_blockCount;
	m_bufIndex = 0;

	hd::stringc strInfo;
	for (unsigned int i = 0;i< m_hlsFileList.size();i++)
	{
		hd::stringc strExt = m_hlsFileList[i].subString(m_hlsFileList[i].size()-3, 3);
		strExt.make_lower();

		//如果是hls文件
		if (strExt == "hls")
		{
			if(!normalSplitHlsFile(savePath, m_hlsFileList[i])/*, hd::u8(i)*/)
				return false;
		}
		else if (strExt == "las" || strExt == "bin")
		{
			if(!normalSplitLasFile(savePath, m_hlsFileList[i]))
				return false;
		}
	}

	return true;
}

bool CHLS2Builder::buildHls(const char* saveCombineHlsPath, BOOL bCombine)
{
	//// 对该接口加密
	//string strSoftName = "hdVector";
	//char strMsg[256] = {0};
	//if (!CheckLicense(strSoftName.c_str(), strMsg))
	//{
	//	::MessageBox(NULL,"请向武汉汉宁轨道交通技术有限公司申请加密狗！", "提示",MB_OK);
	//	return false;
	//}

	m_bSpatialCombine = bCombine;

	char drive[256] = {0};// 磁盘
	char dir[256] = {0};// 文件夹
	char filename[256] = {0};// 文件名
	char ext[256] = {0};// 文件格式
	char path[256] = {0};// 文件路径
	// 解析路径得到m_savePath,m_name
	_splitpath(saveCombineHlsPath, drive, dir, filename, ext);
	if (strcmp(ext,".HLS") != 0 && strcmp(ext,".hls") != 0)
	{
		// 必须判空，部分进度条已经换成多线程进度条 fengjing
		if (processCallback)
		{
			processCallback(0.0, HDSCENE_IDS_BUILDER_DATA_WORING);
		}

		// 对于使用多线程对话框调用的时候弹出提示 fengjing
		::MessageBox(NULL,HDSCENE_IDS_BUILDER_DATA_WORING,HDSCENE_IDS_PROMPT,MB_OK);
		return false;
	}
	// 创建新的文件路径
	_makepath(path,drive,dir,NULL,NULL);
	m_name = filename;
	m_savePath = path;

	if (calucBoundBox() == false)
	{
				// 必须判空，部分进度条已经换成多线程进度条 fengjing
		if (processCallback)
		{
			processCallback(0.0, HDSCENE_IDS_BUILDER_DATA_WORING);
		}

		// 对于使用多线程对话框调用的时候弹出提示 fengjing
		::MessageBox(NULL,HDSCENE_IDS_BUILDER_DATA_WORING,HDSCENE_IDS_PROMPT,MB_OK);
		return false;
	}

	// 文件头偏移量
	m_hlsWrite.m_header.offsetX = m_boundBox.MinEdge.X;
	m_hlsWrite.m_header.offsetY = m_boundBox.MinEdge.Y;
	m_hlsWrite.m_header.offsetZ = m_boundBox.MinEdge.Z;
	m_hlsWrite.SetPointFormat(HLS2_POINTFORMAT_XYZIRGBP);

	// 记录更新时间
	time_t timer;
	time(&timer);
	tm* t_tm = localtime(&timer);
	m_hlsWrite.m_header.file_creation_day = (U16)t_tm->tm_yday + 1;
	m_hlsWrite.m_header.file_creation_year = (U16)t_tm->tm_year + 1900;

	// 打开文件
	if(!m_hlsWrite.Open(saveCombineHlsPath))
		return false;

	// 读取待合并hls文件对应的Lin文件[zfei 7/26]
	int nValidLin = 0;
	if (m_vecHlsPos.size() == 0)
	{
		m_vecHlsPos.resize(m_hlsCorrLinFileList.size());
		for (unsigned int i = 0;i< m_hlsCorrLinFileList.size();i++)
		{
			// 序列化
			HD_SCANHDIINFO::Serialize(m_hlsCorrLinFileList[i].c_str(),m_vecHlsPos[i]);
			nValidLin++;
		}
	}

	// lin文件数量小于等于1，则不能进行空间合并
	if (nValidLin <= 1)
	{
		m_bSpatialCombine = FALSE;
	}

	// 分割文件
	if (normalSplit(m_savePath.c_str()) == false)
	{
		//return false;
	}

	char filePath[MAX_PATH] = {0};
	U32 fileNo = 0;
	U32 index = 0;

	// 对临时文件容器进行遍历
	for (map<U32, pair<FILE*, string>>::iterator it = m_dicFile.begin();
		it != m_dicFile.end();it++)
	{
		FILE* pFile = (it->second).first;
		if (pFile == NULL)
		{
			pFile = fopen((it->second).second.c_str(),"rb");			
		}
		if (pFile == NULL)
		{
			continue;
		}
		fileNo = it->first;

		// 存储原始点信息
		vector<PointXYZIPRGBA> vecBuf;
 		

		// 空间合并点信息
		vector<PointXYZIPRGBAEX> vecPntsEx;
		u32 count = 0;
		//if (m_bSpatialCombine)
		{
			fseek(pFile, 0, SEEK_SET);// 移到文件头

			while(!feof(pFile))
			{
				PointXYZIPRGBA ptTmp;
				fread(&ptTmp, sizeof(PointXYZIPRGBA), 1, pFile);
				vecBuf.push_back(ptTmp);

				// 读取点坐在的圈号
				int nPtLoop = 0;
				if (m_bSpatialCombine)
				{
					fread(&nPtLoop, sizeof(int), 1, pFile);
				}

				// 读取点所属的hls文件
				hd::u8 uHlsIdx = 0;
				if (m_bSpatialCombine)
				{
					fread(&uHlsIdx, sizeof(hd::u8), 1, pFile);
				}

				PointXYZIPRGBAEX pntEx;
				pntEx.pt = ptTmp;
				pntEx.uHlsIdx = uHlsIdx;
				pntEx.LoopIndex = nPtLoop;
				vecPntsEx.push_back(pntEx);

				count++;
			}
		}

		// 空间范围
		CHdBox3dd gridBoundBox;
		gridBoundBox.MinEdge.set(F32_MAX,F32_MAX,F32_MAX);
		gridBoundBox.MaxEdge.set(F32_MIN,F32_MIN,F32_MIN);
		for (hd::u32 i =0; i < vecBuf.size(); i++)
		{
			PointXYZIPRGBA& ptTmp = vecBuf[i];
			if (ptTmp.isValid())
			{
				gridBoundBox.MinEdge.X = MIN(gridBoundBox.MinEdge.X, ptTmp.x);
				gridBoundBox.MinEdge.Y = MIN(gridBoundBox.MinEdge.Y, ptTmp.y);
				gridBoundBox.MinEdge.Z = MIN(gridBoundBox.MinEdge.Z, ptTmp.z);

				gridBoundBox.MaxEdge.X = MAX(gridBoundBox.MaxEdge.X, ptTmp.x);
				gridBoundBox.MaxEdge.Y = MAX(gridBoundBox.MaxEdge.Y, ptTmp.y);
				gridBoundBox.MaxEdge.Z = MAX(gridBoundBox.MaxEdge.Z, ptTmp.z);
			}
		}

		// 空间合并抽稀，合并与否根据m_bSpatialCombine决定，但抽稀必做
		PtsCombineOpti(vecPntsEx, gridBoundBox);

		// 修改vecBuf
		if (vecBuf.size() == vecPntsEx.size())
		{
			for (hd::u32 i = 0; i < vecBuf.size(); i++)
			{
				PointXYZIPRGBA& pt = vecBuf.at(i);
				PointXYZIPRGBAEX& ptEx = vecPntsEx.at(i);
				pt = ptEx.pt;
			}
		}

		// 进行排序
		if (m_bStripByX)
		{
			sort(vecBuf.begin(),vecBuf.end(),lessByX);
		}
		else
		{
			sort(vecBuf.begin(),vecBuf.end(),lessByY);
		}

		// 循环写入块
		hd::u32 i = 0;
		hd::u32 loopSize = 1024;
		for (i = 0;i + loopSize <= count;i+=loopSize)
		{
			m_hlsWrite.WriteLoop(vecBuf._Myfirst() + i,loopSize);
		}

		// 写入最后一块
		if (i < count)
		{
			m_hlsWrite.WriteLoop(vecBuf._Myfirst() + i,count - i);
		}
		vecBuf.clear();

		// 必须首先关闭文件，否则后续删除不掉文件
		fclose(pFile);
		index++;
		if (processCallback)
		{
			processCallback((f32)index/m_dicFile.size(),HDSCENE_IDS_TOHLS_WRITING);
		}
	}
	if (processCallback)
	{
		processCallback(0.0,HDSCENE_IDS_FINISH);
	}
	m_hlsWrite.Close();

	// 删除临时文件
	RemoveTempFiles();
	return true;
}

// 建立hls文件
bool CHLS2Builder::buildHls( const char* savePath)
{
	//// 对该接口加密
	//string strSoftName = "hdVector";
	//char strMsg[256] = {0};
	//if (!CheckLicense(strSoftName.c_str(), strMsg))
	//{
	//	::MessageBox(NULL,"请向武汉汉宁轨道交通技术有限公司申请加密狗！", "提示",MB_OK);
	//	return false;
	//}

	m_bSpatialCombine = FALSE;

	char drive[256] = {0};// 磁盘
	char dir[256] = {0};// 文件夹
	char filename[256] = {0};// 文件名
	char ext[256] = {0};// 文件格式
	char path[256] = {0};// 文件路径
	// 解析路径得到m_savePath,m_name
	_splitpath(savePath, drive, dir, filename, ext);
	if (strcmp(ext,".HLS") != 0 && strcmp(ext,".hls") != 0)
	{
		// 必须判空，部分进度条已经换成多线程进度条 fengjing
		if (processCallback)
		{
			processCallback(0.0, HDSCENE_IDS_BUILDER_DATA_WORING);
		}

		// 对于使用多线程对话框调用的时候弹出提示 fengjing
		::MessageBox(NULL,HDSCENE_IDS_BUILDER_DATA_WORING,HDSCENE_IDS_PROMPT,MB_OK);
		return false;
	}
	// 创建新的文件路径
	_makepath(path,drive,dir,NULL,NULL);
	m_name = filename;
	m_savePath = path;

	if (calucBoundBox() == false)
	{
		// 必须判空，部分进度条已经换成多线程进度条 fengjing
		if (processCallback)
		{
			processCallback(0.0, HDSCENE_IDS_BUILDER_DATA_WORING);
		}

		// 对于使用多线程对话框调用的时候弹出提示 fengjing
		::MessageBox(NULL,HDSCENE_IDS_BUILDER_DATA_WORING,HDSCENE_IDS_PROMPT,MB_OK);
		
		if (processCallback)
		{
			processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
		}
		
		return false;
	}

	// 文件头偏移量
	m_hlsWrite.m_header.offsetX = m_boundBox.MinEdge.X;
	m_hlsWrite.m_header.offsetY = m_boundBox.MinEdge.Y;
	m_hlsWrite.m_header.offsetZ = m_boundBox.MinEdge.Z;
	m_hlsWrite.SetPointFormat(HLS2_POINTFORMAT_XYZIRGBP);

	// 记录更新时间
	time_t timer;
	time(&timer);
	tm* t_tm = localtime(&timer);
	m_hlsWrite.m_header.file_creation_day = (U16)t_tm->tm_yday + 1;
	m_hlsWrite.m_header.file_creation_year = (U16)t_tm->tm_year + 1900;
	
	// 打开文件
	if(!m_hlsWrite.Open(savePath))
	{
		if (processCallback)
		{
			processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
		}

		return false;
	}
	
	// 分割文件
	if (normalSplit(m_savePath.c_str()) == false)
	{
		if (processCallback)
		{
			processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
		}

		RemoveTempFiles();
		return false;
	}

	char filePath[MAX_PATH] = {0};
	U32 fileNo = 0;
	U32 index = 0;
	
	// 对临时文件容器进行遍历
	for (map<U32, pair<FILE*, string>>::iterator it = m_dicFile.begin();
		it != m_dicFile.end();it++)
	{
		FILE* pFile = (it->second).first;
		if (pFile == NULL)
		{
			pFile = fopen((it->second).second.c_str(),"rb");			
		}
		if (pFile == NULL)
		{
			continue;
		}
		fileNo = it->first;

		// 存储点信息
		vector<PointXYZIPRGBA> vecBuf;

		hd::u32 count = 0;
		fseek(pFile,0,SEEK_END);
		long fileSize = ftell(pFile);
		count = fileSize / (sizeof(PointXYZIPRGBA));

		// 可能会resize失败-zhubo.11.15
		try
		{
			vecBuf.resize(count);
		}
		catch (...)
		{
			if (processCallback)
			{
				processCallback(0.0, HDSCENE_IDS_CONVERT_FAILED);
			}

			RemoveTempFiles();
			return false;
		}
		fseek(pFile, 0, SEEK_SET);// 移到文件头
			
		// 读入文件
		fread(vecBuf._Myfirst(),sizeof(PointXYZIPRGBA),count,pFile);

		// 进行排序
		if (m_bStripByX)
		{
			sort(vecBuf.begin(),vecBuf.end(),lessByX);
		}
		else
		{
			sort(vecBuf.begin(),vecBuf.end(),lessByY);
		}

		// 循环写入块
		U32 i = 0;
		U32 loopSize = 1024;
		for (i = 0;i + loopSize <= count;i+=loopSize)
		{
			m_hlsWrite.WriteLoop(vecBuf._Myfirst() + i,loopSize);
		}
		
		// 写入最后一块
		if (i < count)
		{
			m_hlsWrite.WriteLoop(vecBuf._Myfirst() + i,count - i);
		}
		vecBuf.clear();
		
		// 必须首先关闭文件，否则后续删除不掉文件
		fclose(pFile);
		index++;
		if (processCallback)
		{
			processCallback((f32)index/m_dicFile.size(),HDSCENE_IDS_TOHLS_WRITING);
		}
	}
	if (processCallback)
	{
		processCallback(0.0,HDSCENE_IDS_FINISH);
	}
	m_hlsWrite.Close();
	
	// 删除临时文件
	RemoveTempFiles();
	return true;
}

// 空间合并抽稀
void CHLS2Builder::PtsCombineOpti(vector<PointXYZIPRGBAEX>& ptGrid, CHdBox3dd& boundBox)
{
	//**********************对区域内的点建空间索引***************************//
	hd::f32 fRangeX = (hd::f32)(boundBox.MaxEdge.X - boundBox.MinEdge.X);
	hd::f32 fRangeY = (hd::f32)(boundBox.MaxEdge.Y - boundBox.MinEdge.Y);
	hd::f32 fRangeZ = (hd::f32)(boundBox.MaxEdge.Z - boundBox.MinEdge.Z);
	hd::f32 maxAxis = hd::max_(fRangeX, fRangeY, fRangeZ);
	hd::f32 fGirdNum = maxAxis / m_3DGridSize;
	
	int nTmp = 1;		// 最小为1
	if (fGirdNum >= 1)
	{
		// 2的对数
		nTmp = (int)ceil(log10f(fGirdNum)/log10f(2) + 1e-5);
	}

	// 八叉树节点的数量必须为2的指数倍
	int nOcTreeNum = (int)pow(2.0f, nTmp);

	// 八叉树空间索引
	hd::Octree<std::vector<PointXYZIPRGBAEX*>> ocTree;
	ocTree.setSize(nOcTreeNum);

	hd::f32 fStartX = (hd::f32)boundBox.MinEdge.X;
	hd::f32 fStartY = (hd::f32)boundBox.MinEdge.Y;
	hd::f32 fStartZ = (hd::f32)boundBox.MinEdge.Z;

	int nCount = ptGrid.size();
	for (int i = 0; i < nCount; i++)
	{
		PointXYZIPRGBAEX& ptEx = ptGrid[i];
		if (!ptEx.pt.isValid())
		{
			continue;
		}

		// 获得点所在的格网位置
		hd::BlockPos3D pos;
		pos.x = (int)floor((ptEx.pt.x - fStartX)/m_3DGridSize);
		pos.y = (int)floor((ptEx.pt.y - fStartY)/m_3DGridSize);
		pos.z = (int)floor((ptEx.pt.z - fStartZ)/m_3DGridSize);
		pos.x = clamp(pos.x, 0, nOcTreeNum-1);
		pos.y = clamp(pos.y, 0, nOcTreeNum-1);
		pos.z = clamp(pos.z, 0, nOcTreeNum-1);

		ocTree(pos.x, pos.y, pos.z).push_back(&ptEx);
	}
	//*****************************************************//

	// 遍历八叉树叶子节点
	std::vector<std::vector<PointXYZIPRGBAEX*> > vecVaildNodes;
	int nSize = ocTree.SearchLeafNodes(ocTree.root(),vecVaildNodes);

#pragma region 确定节点内的唯一点

	///***********************按照合并和抽稀规则确定节点内的点**********************//
	for (std::vector<std::vector<PointXYZIPRGBAEX*>>::iterator itF = vecVaildNodes.begin();
		itF != vecVaildNodes.end(); itF++)
	{
		std::vector<PointXYZIPRGBAEX*>& vecLeafPts = *itF;
		if (vecLeafPts.size() <= 1)
		{
			continue;
		}

		// 格网中心点
		irr::core::vector3df blockCenterPos;
		blockCenterPos.X = (/*i + */0.5f)*m_3DGridSize + fStartX;
		blockCenterPos.Y = (/*j + */0.5f)*m_3DGridSize + fStartY;
		blockCenterPos.Z = (/*k + */0.5f)*m_3DGridSize + fStartZ;

		if (!m_bSpatialCombine)
		{
			// 不需要空间合并，选取离格网中心最近的一个点
			hd::u32 nMinIdx = 0;
			hd::f32 fDistMin = 2.0f * m_3DGridSize;
			for (hd::u32 i = 0; i < vecLeafPts.size(); i++)
			{
				PointXYZIPRGBAEX* pPtEx = vecLeafPts[i];
				irr::core::vector3df ptTmp(pPtEx->pt.x, pPtEx->pt.y, pPtEx->pt.z);
				hd::f32 fDist = ptTmp.getDistanceFrom(blockCenterPos);

				if (fDist < fDistMin)
				{
					nMinIdx = i;
					fDistMin = fDist;
				}
			}

			// 其他点置为无效点
			for (hd::u32 i = 0; i < vecLeafPts.size(); i++)
			{
				PointXYZIPRGBAEX* pPtEx = vecLeafPts[i];
				if (!pPtEx->pt.isValid())
				{
					continue;
				}
				if (i != nMinIdx)
				{
					//pPtEx->pt.x = 0;
					//pPtEx->pt.y = 0;
					//pPtEx->pt.z = 0;

					pPtEx->pt.setSelected();
				}
			}
		}
		else 
		{
			// 用于统计距离的map
			std::map<int, std::vector<hd::f32>> mapCurrDist2Pos;

			for (std::vector<PointXYZIPRGBAEX*>::iterator it = vecLeafPts.begin();
				it != vecLeafPts.end(); it++)
			{
				PointXYZIPRGBAEX* pPtEx = *it;
				if (!pPtEx->pt.isValid())
				{
					continue;
				}

				std::map<int, std::vector<hd::f32>>::iterator itM = mapCurrDist2Pos.find(pPtEx->uHlsIdx);

				// 不存在则在最后插入一个
				std::vector<hd::f32> vecTmp;
				if (itM == mapCurrDist2Pos.end())
				{
					mapCurrDist2Pos.insert(make_pair(pPtEx->uHlsIdx, vecTmp));
					itM = --mapCurrDist2Pos.end();
				}

				// 已存在后计算距离
				std::vector<hd::f32>& vecDist = itM->second;
				hd::f32 dist = GetDistToPos((pPtEx->pt), pPtEx->LoopIndex, pPtEx->uHlsIdx);
				vecDist.push_back(dist);
			}

			if (mapCurrDist2Pos.size() >= 2)
			{
				//　>=2个工程的点云，取离POS中心最近的一类点云
				hd::f32 dAvegDistMin = INT_MAX;		// 每类点云距离的平均值
				int nNearHlsIdx = 0;
				for (hd::u32 i = 0; i < mapCurrDist2Pos.size(); i++)
				{
					std::vector<hd::f32>& vecDist = mapCurrDist2Pos[i];
					hd::f32 dAvegDist = 0.f;
					for (std::vector<hd::f32>::iterator itD = vecDist.begin();
						itD != vecDist.end(); itD++)
					{
						dAvegDist += *itD;
					}
					dAvegDist /= (vecDist.size());

					if (dAvegDist > 0.f && dAvegDist < dAvegDistMin)
					{
						nNearHlsIdx = i;
						dAvegDistMin = dAvegDist;
					}
				}

				// 清除节点内不属于最近那个点云的其他点
				for (std::vector<PointXYZIPRGBAEX*>::iterator it = vecLeafPts.begin();
					it != vecLeafPts.end(); it++)
				{
					PointXYZIPRGBAEX* pPtEx = *it;
					if (pPtEx->uHlsIdx != nNearHlsIdx)
					{
						pPtEx->pt.setSelected();
						//pPtEx->pt.x = 0.f;
						//pPtEx->pt.y = 0.f;
						//pPtEx->pt.z = 0.f;
					}
				}

				// 清除map中的其他工程点云
				for (std::map<int, std::vector<hd::f32>>::iterator iter = mapCurrDist2Pos.begin();
					iter != mapCurrDist2Pos.end(); iter++)
				{
					int nHls = iter->first;
					if (nHls != nNearHlsIdx)
					{
						iter = mapCurrDist2Pos.erase(iter);
						iter--;
					}
				}
			}

			if (mapCurrDist2Pos.size() == 1)
			{
				// 选取离格网中心最近的一个点
				hd::u32 nMinIdx = 0;
				hd::f32 fDistMin = 2.0f * m_3DGridSize;
				for (hd::u32 i = 0; i < vecLeafPts.size(); i++)
				{
					PointXYZIPRGBAEX* pPtEx = vecLeafPts[i];
					if (!pPtEx->pt.isValid() || pPtEx->pt.isSelected())
					{
						continue;
					}
					irr::core::vector3df ptTmp(pPtEx->pt.x, pPtEx->pt.y, pPtEx->pt.z);
					hd::f32 fDist = ptTmp.getDistanceFrom(blockCenterPos);

					if (fDist < fDistMin)
					{
						nMinIdx = i;
						fDistMin = fDist;
					}
				}

				// 其他点置为无效点
				for (hd::u32 i = 0; i < vecLeafPts.size(); i++)
				{
					PointXYZIPRGBAEX* pPtEx = vecLeafPts[i];
					if (!pPtEx->pt.isValid())
					{
						continue;
					}
					if (i != nMinIdx)
					{
						//pPtEx->pt.x = 0;
						//pPtEx->pt.y = 0;
						//pPtEx->pt.z = 0;

						pPtEx->pt.setSelected();
					}
				}
			}
		}
	}
	//*************************************************************************//
#pragma endregion 确定节点内的唯一点

	return;
}


//! 删除所有临时文件
void CHLS2Builder::RemoveTempFiles()
{
	for (map<U32,pair<FILE*,string>>::iterator it = m_dicFile.begin();
		it != m_dicFile.end();it++)

	{	
		string strtmp = (it->second).second;
		int err = remove(strtmp.c_str());
		if (err == -1)
		{
			perror("无法删除文件");
		}
	}
}

// 计算HLS文件中的数据的最大值、最小值 
void CHLS2Builder::calHlsExtnt( hd::stringc& strHlsFile)
{
	// 打开文件
	hd::IHLSReader* reader = NULL;
	hd::CHLSReadOpener readerOpen;
	reader = readerOpen.Open(strHlsFile.c_str());
	reader->GetHeader();
	if(reader == NULL)
		return ;
	// 点记录数
	m_totalCount += reader->m_header.number_of_point_records;

	// 首先利用文件头来取得最大值、最小值、
	double minX,maxX,minY,maxY,minZ,maxZ;
	reader->m_header.getGlobalExtent(minX, minY, minZ, maxX, maxY, maxZ);

	m_boundBox.MinEdge.X = MIN(m_boundBox.MinEdge.X, MIN(minX, maxX));
	m_boundBox.MinEdge.Y = MIN(m_boundBox.MinEdge.Y, MIN(minY, maxY));
	m_boundBox.MinEdge.Z = MIN(m_boundBox.MinEdge.Z, MIN(minZ, maxZ));

	m_boundBox.MaxEdge.X = MAX(m_boundBox.MaxEdge.X, MAX(maxX, minX));
	m_boundBox.MaxEdge.Y = MAX(m_boundBox.MaxEdge.Y, MAX(maxY, minY));
	m_boundBox.MaxEdge.Z = MAX(m_boundBox.MaxEdge.Z, MAX(maxZ, minZ));

	//如果头文件中最大值、最小值都为0
	// 那么对数据进行遍历分析，得到最大值、最小值
	if ( m_boundBox.MinEdge.X ==  m_boundBox.MaxEdge.X && m_boundBox.MinEdge.Y ==  m_boundBox.MaxEdge.Y &&
		m_boundBox.MinEdge.X == 0 && m_boundBox.MinEdge.Y == 0 )
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
				PointXYZIPRGBA& pt = *(vecPts._Myfirst + k);
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
		m_boundBox.MinEdge.X = MIN(m_boundBox.MinEdge.X, xyzmin[0]);
		m_boundBox.MinEdge.Y = MIN(m_boundBox.MinEdge.Y, xyzmin[1]);
		m_boundBox.MinEdge.Z = MIN(m_boundBox.MinEdge.Z, xyzmin[2]);
		m_boundBox.MaxEdge.X = MAX(m_boundBox.MaxEdge.X, xyzmax[0]);
		m_boundBox.MaxEdge.Y = MAX(m_boundBox.MaxEdge.Y, xyzmax[1]);
		m_boundBox.MaxEdge.Z = MAX(m_boundBox.MaxEdge.Z, xyzmax[2]);

	}
	if (reader)
	{
		delete reader;
		reader = NULL;
	}

}

hd::f32 CHLS2Builder::GetDistToPos( PointXYZIPRGBA& pt, hd::u32 nLoopIndex, hd::u32 nPosFileIndex )
{
	hd::f32 fDist = -1.0f;
	if (nPosFileIndex >= 0 && nPosFileIndex < m_vecHlsPos.size())
	{
		std::vector<HD_SCANHDIINFO>& vecCurPos = m_vecHlsPos.at(nPosFileIndex);
		if (nLoopIndex >= 0 && nLoopIndex < vecCurPos.size())
		{
			// 计算比例
			hd::f32 dScale = (hd::f32)nLoopIndex / vecCurPos.size();		
			int scanPtIndex = (int)(dScale * vecCurPos.size());
			const HD_SCANHDIINFO& scanPos = vecCurPos[scanPtIndex];

			// 转换点到绝对坐标
			double M[16];
			m_hlsWrite.m_header.computeMatrix(M);
			double x = pt.x;
			double y = pt.y;
			double z = pt.z;
			hdHomogeneousTransformPoint(M,x,y,z);

			// 计算距离
			double cx = scanPos.dX;
			double cy = scanPos.dY;
			double cz = scanPos.dZ;
			irr::core::vector3dd pp((x - cx),(y - cy),(z - cz));

			fDist = (hd::f32)sqrt(pp.X*pp.X + pp.Y*pp.Y + pp.Z*pp.Z);
		}
	}

	return fDist;
}

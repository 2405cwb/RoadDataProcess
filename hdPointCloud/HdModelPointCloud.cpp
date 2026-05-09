/*! HdModelPointCloud.cpp
********************************************************************************
<PRE>
模块名       : hdPointCloud
文件名       : HdModelPointCloud.cpp
相关文件     : HdModelPointCloud.h
文件实现功能 : 点云模型DEM内存结构
作者         : 研发部 冯晶
版本         : 1.0
版权		 : CopyRight @ 2013 海达数云
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容  
2014/02/11     1.0        冯晶                 新建
</PRE>
*******************************************************************************/
#include "StdAfx.h"
#include "HdModelPointCloud.h"
#include "..\hdHlslib\IHLSReader.h"
#include "..\hdHlslib\HLSReadOpener.h"
#include "..\hdCommon\LtTriangulate.h"
#include "..\hdHLSlib\HLSReadOpener.h"
#include "..\hdHlslib\inc\lasreader.hpp"
#include "..\hdCommon\hdSceneStr.h"
#include "..\3rd\GDAL\Include\gdal.h"
#include "..\3rd\GDAL\Include\gdal.h"
#include "..\3rd\GDAL\Include\gdal_alg.h"
#include "..\3rd\GDAL\Include\cpl_conv.h"
#include "..\3rd\GDAL\Include\cpl_string.h"
#include "..\3rd\GDAL\Include\ogr_api.h"
#include "..\3rd\GDAL\Include\ogr_srs_api.h"
#include "..\3rd\GDAL\Include\gdal_priv.h"

#include <stdlib.h>
#include <math.h>
#include <algorithm>

namespace hd
{
	// 向上取整
	static inline int Ceil(const float x)
	{
		int ceilx = (int)x;
		if (x>ceilx) ceilx++;
		return ceilx;
	}

	// 向下取整
	static inline int Floor(const float x)
	{
		int floorx = (int)x;
		if (x<floorx) floorx--;
		return floorx;
	}

	// 构造
	CHdModelPointCloud::CHdModelPointCloud()
		: m_pPtBuffer(NULL)		,
		m_npoints(0)			,
		m_nrows(0)				,
		m_ncols(0)				,
		m_noDataValue(-9999.0f)	,
		m_stepx(1.0f)			,
		m_stepy(1.0f)			,
		m_minx(1e30f)			,
		m_miny(1e30f)			,
		m_minz(1e30f)			,
		m_maxx(1e30f)			,
		m_maxy(1e30f)			,
		m_maxz(1e30f)			,
		m_ModelX(0.0)			,
		m_ModelY(0.0)			,
		m_ProjectStyle(2)			// 默认最低点
	{
		m_bCutFill = false;
		m_vecCutFillBox.clear();
	}


	// 析构
	CHdModelPointCloud::~CHdModelPointCloud()
	{
		// 释放
		if (m_pPtBuffer)
		{
			delete[] m_pPtBuffer;
			m_pPtBuffer = NULL;
		}
	}

	// 获取数据范围
	void CHdModelPointCloud::GetBoudingBox(core::aabbox3df& box)
	{
		box.MaxEdge.X = (float)m_maxx;
		box.MaxEdge.Y = (float)m_maxy;
		box.MaxEdge.Z = (float)m_maxz;

		box.MinEdge.X = (float)m_minx;
		box.MinEdge.Y = (float)m_miny;
		box.MinEdge.Z = (float)m_minz;
	}

	// 设置包围盒
	void CHdModelPointCloud::SetBoudingBox(core::aabbox3df box)
	{
		m_maxx = box.MaxEdge.X;
		m_maxy = box.MaxEdge.Y;
		m_maxz = box.MaxEdge.Z;

		m_minx = box.MinEdge.X;
		m_miny = box.MinEdge.Y;
		m_minz = box.MinEdge.Z;		
	}

	// 获取全局坐标
	void CHdModelPointCloud::GetGlobalCoord(double& gx, double& gy)
	{
		gx += m_ModelX;
		gy += m_ModelY;
	}

	// 获取相对坐标, 传进来的参数为绝对坐标，传出去的坐标为相对坐标
	void CHdModelPointCloud::GetabsCoord(double& x, double& y)
	{
		x -= m_ModelX;
		y -= m_ModelY;
	}

	// 加载文件
	int CHdModelPointCloud::loadFile(const char* filepath, void (*loadCallback)(float,const char*))
	{
		// 输入文件路径
		std::string filePath = filepath;  

		// 判空
		if (filePath == "")
		{
			return -1;
		}

		// 将文件名化为小写
		std::transform(filePath.begin(),
			filePath.end(),
			filePath.begin(),tolower);

		// 根据文件后缀读入文件
		if (filePath.find(".hls") != -1)
		{
			// 投影
			//HLSProGrid(filepath, loadCallback);
			loadProHlsFile(filepath,loadCallback);

			// 插值
			return Interpolation(DEM_UNDER,loadCallback);
		}
		else if (filePath.find(".las") != -1 
			|| filePath.find(".laz") != -1 
			|| filePath.find(".bin") != -1)
		{
			// 加载las，laz，bin文件
			return loadLasFile(filePath.c_str(), loadCallback);
		}
		else if (filePath.find(".tif") != -1 )
		{
			// 加载tif文件
			return loadTifFile(filePath.c_str(), loadCallback);
		}
		else if (filePath.find(".asc") != -1)
		{
			// 加载asc文件
			return loadAscFile(filePath.c_str(), loadCallback);
		}

		return 0;
	}

	// 将asc文件加载到内存
	int CHdModelPointCloud::loadAscFile(const char* ascFile, void (*loadCallback)(float,const char*))
	{
		// 输入文件路径
		m_filePath = ascFile;  

		// 判空
		if (m_filePath == "")
		{
			if (loadCallback)
			{
				loadCallback(1.0,HDSCENE_IDS_TOTIN_OPENTIF_FAILED);
			}
			return FALSE;
		}

		// 使用GDAL读文件
		FILE* pFile = fopen(ascFile,"r");
		if (!pFile)
		{
			// 设置进度条
			if (loadCallback)
			{
				loadCallback(1.0,HDSCENE_IDS_TODEM_FAILED_GET_TIFDIRVER);
			}
			return -1;
		}

		float xMin = 0.0f;			// x最小值
		float yMin = 0.0f;			// y最小值
		float cellSize = 0.0f;		// 格网大小

		// 开始读文件
		fscanf(pFile,"%*s%d\n", &m_ncols);
		fscanf(pFile,"%*s%d\n", &m_nrows);
		fscanf(pFile,"%*s%f\n", &xMin);
		fscanf(pFile,"%*s%f\n", &yMin);
		fscanf(pFile,"%*s%f\n", &m_stepx);
		fscanf(pFile,"%*s%f\n", &m_noDataValue);

		// 偏移
		m_ModelX = xMin;
		m_ModelY = yMin;

		// 记录demmodel相对到绝对坐标转换的model并计算矩阵
		m_transModel.m_fOffset[0] = m_ModelX;
		m_transModel.m_fOffset[1] = m_ModelY;
		m_transModel.m_fOffset[2] = 0.0f;
		m_transModel.Parameter2matrix();

		// 设置最值以及网格大小
		m_minx = xMin;
		m_miny = yMin;
		m_stepy = m_stepx;

		// 采用内存缓存，并采用nodata进行初始化
		m_vecDemData.resize(m_nrows);

		// 对每个数据设置初值
		for (int i = 0;i < m_nrows;i++)
		{
			vector<float>& vecPts = m_vecDemData[i];
			vecPts.resize(m_ncols);

			// 初值为Nodata
			for (int j = 0;j < m_ncols;j++)
			{
				vecPts[j] = m_noDataValue;
			}
		}

		// 从文件中读取Z值
		for (int i = 0;i < m_nrows ;i++)
		{
			for (int j = 0;j < m_ncols;j++)
			{
				fscanf(pFile,"%f \n", &m_vecDemData[i][j]);
			}

			// 设置进度条
			if (loadCallback)
			{
				loadCallback((float)(i)/m_nrows,HDSCENE_IDS_PROCESS_READING);
			}
		}

		// 完成
		if (loadCallback)
		{
			loadCallback(0.0f, " ");
		}
		return TRUE;	
	}

	// 遍历hls导出tif
	int CHdModelPointCloud::HLSProGrid(const char* hlsfile, void (*loadCallback)(float, const char*))
	{		
		// 设置文件路径
		m_filePath = hlsfile;

		// 判空
		if (m_filePath == "")
		{
			return -1;
		}

		// 读取文件头
		IHLSReader* reader;
		CHLSReadOpener hlsOpen;

		// 打开文件
		reader = hlsOpen.Open(hlsfile);

		// 判空
		if (reader == NULL)
		{
			return 0;
		}

		// 获取索引
		reader->GetLoopIndex(loadCallback);

		int npoints = 0;							// 点数
		hdVector<PointXYZIPRGBA> pTmpBuf;			// 有效点数
		int i = 0;									// 计数器

		// 先遍历获取有效点数
		while(reader->ReadLoop(pTmpBuf,i++))
		{
			npoints += pTmpBuf.size();
		}

		// 获取绝对范围
		reader->m_header.getGlobalExtent(m_minx, m_miny, m_minz, m_maxx, m_maxy, m_maxz);
		if (m_maxx <= m_minx || m_maxy <= m_miny || m_maxz <= m_minz)
		{
			if (reader)
			{
				delete reader;
				reader = NULL;
			}

			// 数据范围不正确则退出
			::MessageBox(NULL,HDSCENE_IDS_TOLASGRID_RANGE_INCORRECT,HDSCENE_IDS_PROMPT,MB_OK);
			return 0;
		}

		// 统计行列号
		int nCols = hd_round32(fabs((m_maxx - m_minx))/m_stepx);	
		int nRows = hd_round32(fabs((m_maxy - m_miny))/m_stepy);

		// 判断合法性
		if (nCols <= 0 || nRows <= 0 || nCols > 10000 * 100 || nRows > 10000 * 100)
		{
			if (reader)
			{
				delete reader;
				reader = NULL;
			}

			// 行列号非法退出
			::MessageBox(NULL,HDSCENE_IDS_TOLASGRID_RANGE_INCORRECT,HDSCENE_IDS_PROMPT,MB_OK);
			return 0;
		}

		// 如果是平均值，需要记录每个格网中的总点数
		int* nMidCount = NULL;
		if (m_ProjectStyle == DEM_MID)
		{
			nMidCount = new int[nCols * nRows];
			memset(nMidCount,0,sizeof(int)*nCols*nRows);
		}

		// 点云
		vector<vector<PointXYZI>> vecPcd;
		vecPcd.resize(nRows);
		for (int i = 0;i < nRows;i++)
		{
			vector<PointXYZI>& vecPts = vecPcd[i];
			vecPts.resize(nCols);
		}

		// 释放内存
		if (m_pPtBuffer)
		{
			delete 	m_pPtBuffer	;
			m_pPtBuffer = NULL;
		}

		// 申请坐标存储空间,使用相对坐标
		m_pPtBuffer = (float*)malloc(sizeof(float)*3*npoints);

		U32 loopCount = reader->GetLoopCount(); // 圈数
		hdVector<PointXYZIPRGBA> pts;			// 点云
		U32 loopIndex = 0;						// 圈索引

		// 初始化tin，申请总点数所需内存，未申请到直接返回
		bool bRet = TINclean(npoints);
		if (!bRet)
		{
			if (reader)
			{
				delete reader;
				reader = NULL;
			}

			// 弹出提示
			::MessageBox(NULL,HDSCENE_IDS_OUTMEMORY_DOPROJECT,HDSCENE_IDS_PROMPT,MB_OK);
			return -1;
		}

		// 点数计数器
		int count = 0;

		// 遍历点云
		while(reader->ReadLoop(pts,loopIndex++))
		{
			for (int i = 0;i < (int)pts.size();i++ )
			{
				const PointXYZIPRGBA& ptRd = *(pts._Myfirst + i);
				if(!ptRd.isValid())
				{
					// 过滤无效点
					continue;
				}

				// 读取点必须为全局坐标
				double x = 0.0,y = 0.0,z = 0.0;
				reader->GetCoordinate(ptRd.x,ptRd.y,ptRd.z,x,y,z);

				// 计算当期点所在网格行列号
				int col = (int)(fabs((x - m_minx)) / m_stepx);
				int row = (int)(fabs((y - m_miny)) / m_stepy);

				if (col < 0 || row < 0 || col >= nCols || row >= nRows)
				{
					// 行列非法，返回
					continue;
				}

				// 根据投影方式计算投影后的点
				vector<PointXYZI>& vecPts = vecPcd[row];
				PointXYZI& pt = vecPts[col];
				// 最低点
				if (m_ProjectStyle == DEM_UNDER)
				{
					if (pt.x == 0.0f && pt.y == 0.0f && pt.z == 0.0f)
					{
						pt.x = (float)(x - m_minx);
						pt.y = (float)(y - m_miny);
						pt.z = (float)z;
						pt.intensity = ptRd.getIntensity();
					}
					else
					{
						if (z < pt.z)
						{
							pt.z = (float)z;
							pt.x = (float)(x - m_minx);
							pt.y = (float)(y - m_miny);
							pt.intensity = ptRd.getIntensity();
						}
					}
				}
				// 最高点
				else if (m_ProjectStyle == DEM_ABOVE)
				{
					if (pt.x == 0.0f && pt.y == 0.0f && pt.z == 0.0f)
					{
						pt.x = (float)(x - m_minx);
						pt.y = (float)(y - m_miny);
						pt.z = (float)z;
						pt.intensity = ptRd.getIntensity();
					}
					else
					{
						if (z > pt.z)
						{
							pt.z = (float)z;
							pt.x = (float)(x - m_minx);
							pt.y = (float)(y - m_miny);
							pt.intensity = ptRd.getIntensity();
						}
					}
				}
				// 平均值
				else if (m_ProjectStyle == DEM_MID)
				{
					if (pt.x == 0.0f && pt.y == 0.0f && pt.z == 0.0f)
					{
						pt.x = (float)(x - m_minx);
						pt.y = (float)(y - m_miny);
						pt.z = (float)z;
						pt.intensity = ptRd.getIntensity();
						nMidCount[row * nCols + col]++;
					}
					else
					{
						pt.z += (float)z;
						pt.x += (float)(x - m_minx);
						pt.y += (float)(y - m_miny);
						pt.intensity += ptRd.getIntensity();
						nMidCount[row * nCols + col]++;
					}
				}	

				// 将当前点加入缓存
				// 采用局部坐标计算
				m_pPtBuffer[3*count+0] = pt.x;
				m_pPtBuffer[3*count+1] = pt.y;
				m_pPtBuffer[3*count+2] = pt.z;

				// add the point to the triangulation
				TINadd(&(m_pPtBuffer[3*count]));
				count++;
			}

			// 设置进度条
			if (loadCallback && (loopIndex % 10) == 0)
			{
				loadCallback((float)loopIndex / loopCount,HDSCENE_IDS_PROCESS_CONVERTING);
			}
		}
		// 完成tin
		TINfinish();

		// 设置行列以及总点数
		m_nrows = nRows;
		m_ncols = nCols;
		m_npoints = npoints;

		// 释放
		if (reader)
		{
			delete reader;
			reader = NULL;
		}

		// 释放平均值中间变量
		if (nMidCount != NULL)
		{
			delete []nMidCount;
			nMidCount = NULL;
		}

		return 1;
	}

	// 插值
	int CHdModelPointCloud::Interpolation(DEM_PRO_STYLE proStyle, void (*loadCallback)(float, const char*))
	{
		// 判断合法性
		if (m_ncols < 0 || m_nrows < 0)
		{
			return -1;
		}

		// 临时文件名称
		string strTempPath = m_filePath + ".temp";

		// 行列比在vecPcd范围内使用内存vector存储，超过选择临时文件存储
		m_vecDemData.clear();

		// 声明文件指针
		FILE* pTempFile = NULL;

		// 判断是否采内存缓存 或 文件缓存
		if (m_ncols * m_nrows <= 2048*2048)
		{
			// 采用内存缓存，并采用nodata进行初始化
			m_bBufferInMemory = true;

			// 申请内存，并初始化
			m_vecDemData.resize(m_nrows);
			for (int i = 0;i < m_nrows;i++)
			{
				vector<float>& vecPts = m_vecDemData[i];
				vecPts.resize(m_ncols);
				// 初值为Nodata
				for (int j = 0;j < m_ncols;j++)
				{
					vecPts[j] = m_noDataValue;
				}
			}
		}
		else
		{
			// 采用文件缓存，此处进行打开临时文件
			pTempFile = fopen(strTempPath.c_str(),"w+b");
			if (!pTempFile)
			{
				return -2;
			}

			// 将无效值写进临时文件
			for (int i = 0;i < m_nrows; i++)
			{
				// 定位到每一行
				fseek(pTempFile,(m_ncols*i)*sizeof(float),SEEK_SET);

				// 写每一列
				for (int j = 0; j < m_ncols; j++)
				{	
					fwrite(&m_noDataValue, sizeof(float),1,pTempFile);
				}
			}
			m_bBufferInMemory = false;
		}

		// 转换坐标
		double coordinates[3];
		for (int i = 0; i < m_npoints; i++)
		{
			// 缓存中点转换至局部坐标
			coordinates[0] = m_minx + m_pPtBuffer[3*i+0];
			coordinates[1] = m_miny + m_pPtBuffer[3*i+1];
			coordinates[2] = m_pPtBuffer[3*i+2];

			// 转换至栅格坐标
			world_to_raster(coordinates, &(m_pPtBuffer[3*i+0]));
		}

		// 通过三角网取点得到格网点存储至对应vector或临时文件
		float kill_threshold_squared = 10000.0f;

		// 获取三角形指针
		TINtriangle* t = TINget_triangle(0);

		// 三角形计数器
		int count = 0;
		for (count = 0; count < TINget_size(); count++, t++)
		{
			if (t->next < 0) // if not deleted
			{
				if (t->V[0]) // if not infinite
				{
					// 根据缓存方式，计算每个三角形与格网中心线的交线获得该格网处的高程值，并放入对应格式缓存中
					if (m_bBufferInMemory)
					{
						// 将三角网转换写至二进制临时文件
						raster_triangle_to_memory(m_vecDemData,kill_threshold_squared,t->V[0], t->V[1], t->V[2]);
					}
					else
					{
						// 将三角网转换至对应内存格网
						raster_triangle_to_file(pTempFile,kill_threshold_squared,t->V[0], t->V[1], t->V[2]);
					}
				}
			}

			if (loadCallback && count %1000 == 0)
			{
				loadCallback((F32)count/TINget_size(),HDSCENE_IDS_TODEM_WRITING_DEM);
			}
		}

		// 文件缓存关闭临时文件
		if (!m_bBufferInMemory)
		{
			fclose(pTempFile);
		}

		// 保存文件后缀名,网格大小以及投影方式
		char c[10] = {0};
		_itoa((int)m_stepx,c,10);
		string strStep(c);
		_itoa(proStyle,c,10);
		string strPro(c);

		// 导出tif
		int nPos = m_filePath.find_last_of('.');								// 查找最后一个.的位置
		string filePath;														// 保存路径
		if (nPos != -1)
		{	
			filePath = m_filePath.substr(0, nPos);								// 取子串
			filePath = filePath + "_DEM_" + strStep + "_" + strPro + ".tif";	// 加上文件后缀
		}

		// 文件不存在
		if (!FileExists(filePath.data()))
		{
			if (m_bBufferInMemory)
			{
				// 内存到tif
				BufferMemory2Tif(filePath.data(), loadCallback);
			}
			else
			{
				// 临时文件到tif
				pTempFile = fopen(strTempPath.c_str(),"rb");
				BufferFile2Tif(pTempFile, filePath.data(), loadCallback);
				fclose(pTempFile);
			}					
		}

		// 文件缓存删除临时文件
		if (!m_bBufferInMemory)
		{
			remove(strTempPath.c_str());
		}

		// 删除三角形
		TINdestroy();

		// 释放
		free(m_pPtBuffer);
		m_pPtBuffer = NULL;

		return 1;
	}

	// 加载投影点云文件
	int CHdModelPointCloud::loadProHlsFile(const char* hlsFile, void (*loadCallback)(float,const char*))
	{
		// 输入文件路径
		m_filePath = hlsFile;  

		// 判空
		if (m_filePath == "")
		{
			return -1;
		}

		// 读取HLS格式文件
		IHLSReader* hlsReader = NULL;
		CHLSReadOpener hlsOpen;

		// 打开文件
		hlsReader = hlsOpen.Open(m_filePath.c_str());

		// 判空
		if(hlsReader == NULL)
		{
			return -1;
		}

		// 点计数器
		int count = 0;

		// 获取HLS2_LOOPINDEX
		hlsReader->GetLoopIndex();

		// 总点数
		U64 npoints = 0;//hlsReader->m_header.number_of_point_records;

		// 圈数
		U32 loopCount = hlsReader->GetLoopCount();

		hdVector<PointXYZIPRGBA> pTmpBuf;			// 中间变量
		double x,y,z;								// 中间变量
		int i = 0;									// 计数器

		// 先遍历获取有效点数
		while(hlsReader->ReadLoop(pTmpBuf,i++))
		{
			npoints += pTmpBuf.size();
		}

		// 申请内存之前先进行判断
		if (m_pPtBuffer)
		{
			free(m_pPtBuffer);
			m_pPtBuffer = NULL;
		}

		// 申请坐标存储空间,使用相对坐标
		m_pPtBuffer = (float*)malloc(sizeof(float)*3*npoints);

		// 初始化tin，申请总点数所需内存，未申请到直接返回
		bool bRet = TINclean((int)npoints);
		if (!bRet)
		{
			if (hlsReader)
			{
				delete hlsReader;
				hlsReader = NULL;
			}

			::MessageBox(NULL,HDSCENE_IDS_OUTMEMORY_DOPROJECT,HDSCENE_IDS_PROMPT,MB_OK);
			return -1;
		}

		// 获得点云文件全局坐标范围
		hlsReader->m_header.getGlobalExtent(m_minx,m_miny,m_minz,m_maxx,m_maxy,m_maxz);

		// 按圈读取HLS文件，每个点计算最佳三角网
		i = 0;
		while(hlsReader->ReadLoop(pTmpBuf,i++))
		{
			for (U32 n = 0;n < pTmpBuf.size();n++)
			{
				const PointXYZIPRGBA& pt = *(pTmpBuf._Myfirst + n);//pTmpBuf[n];
				hlsReader->GetCoordinate(pt.x,pt.y,pt.z,x,y,z);

				// 采用局部坐标计算
				m_pPtBuffer[3*count+0] = (float)(x - m_minx);
				m_pPtBuffer[3*count+1] = (float)(y - m_miny);
				m_pPtBuffer[3*count+2] = (float)(z);

				// add the point to the triangulation
				TINadd(&(m_pPtBuffer[3*count]));

				count++;

				// 设置进度条
				if ((count % 10000) == 0 && loadCallback)
				{
					loadCallback((F32)count/npoints,HDSCENE_IDS_TODEM_CREATING_TIN);
				}
			}
		}

		// 完成TIN
		TINfinish();

		// 根据范围计算所需格网大小
		m_nrows = (int)((m_maxy - m_miny) / m_stepy);
		m_ncols = (int)((m_maxx - m_minx) / m_stepx);
		if (((m_maxy - m_miny) / m_stepy) > (float)m_nrows)
		{
			m_nrows += 1;
		}
		if (((m_maxx - m_minx) / m_stepx))
		{
			m_ncols += 1;
		}

		// 释放
		if (hlsReader)
		{
			delete hlsReader;
			hlsReader = NULL;
		}

		// 设置点数
		m_npoints = count;

		return count;
	}

	// 加载HLS文件
	int CHdModelPointCloud::loadHlsFile(const char* hlsFile,void (*loadCallback)(float,const char*))
	{
		// 输入文件路径
		m_filePath = hlsFile;  

		// 判空
		if (m_filePath == "")
		{
			return -1;
		}

		// 读取HLS格式文件
		IHLSReader* hlsReader = NULL;
		CHLSReadOpener hlsOpen;

		// 打开文件
		hlsReader = hlsOpen.Open(m_filePath.c_str());

		// 判空
		if(hlsReader == NULL)
		{
			return -1;
		}

		// 点计数器
		int count = 0;

		// 获取圈索引
		hlsReader->GetLoopIndex();

		// 总点数
		U64 npoints = 0;//hlsReader->m_header.number_of_point_records;
		U32 loopCount = hlsReader->GetLoopCount();  // 圈号
		hdVector<PointXYZIPRGBA> pTmpBuf;			// 中间变量
		double x,y,z;								// 中间变量
		int i = 0;									// 圈计数器

		// 先遍历获取有效点数
		while(hlsReader->ReadLoop(pTmpBuf,i++))
		{
			npoints += pTmpBuf.size();
		}

		// 获得点云文件全局坐标范围
		hlsReader->m_header.getGlobalExtent(m_minx,m_miny,m_minz,m_maxx,m_maxy,m_maxz);

		// 按圈读取HLS文件，每个点计算最佳三角网
		i = 0;
		while(hlsReader->ReadLoop(pTmpBuf,i++))
		{
			for (U32 n = 0;n < pTmpBuf.size();n++)
			{
				const PointXYZIPRGBA& pt = *(pTmpBuf._Myfirst + n);//pTmpBuf[n];
				hlsReader->GetCoordinate(pt.x,pt.y,pt.z,x,y,z);

				S3DVertex2TCoords DemPcd;
				DemPcd.Pos.X = (float)x;
				DemPcd.Pos.Y = (float)y;
				DemPcd.Pos.Z = (float)z;
				m_pts.push_back(DemPcd);		// 设置tin数据

				count++;
				if ((count % 10000) == 0 && loadCallback)
				{
					loadCallback((F32)count/npoints,HDSCENE_IDS_TODEM_CREATING_TIN);
				}
			}
		}

		// 根据范围计算所需格网大小
		m_nrows = (int)((m_maxy - m_miny) / m_stepy);
		m_ncols = (int)((m_maxx - m_minx) / m_stepx);
		if (((m_maxy - m_miny) / m_stepy) > (float)m_nrows)
		{
			m_nrows += 1;
		}
		if (((m_maxx - m_minx) / m_stepx))
		{
			m_ncols += 1;
		}

		// 关闭文件
		if (hlsReader)
		{
			delete hlsReader;
			hlsReader = NULL;
		}

		// 设置总点数
		m_npoints = count;
		return count;
	}

	// 加载las文件
	int CHdModelPointCloud::loadLasFile(const char* lasFile,void (*loadCallback)(float,const char*))
	{
		return 1;
	}

	// 加载tif文件
	BOOL CHdModelPointCloud::loadTifFile(const char* tifFile, void (*loadCallback)(float,const char*))
	{
		// 输入文件路径
		m_filePath = tifFile;  

		// 判空
		if (m_filePath == "")
		{
			if (loadCallback)
			{
				loadCallback(1.0,HDSCENE_IDS_TOTIN_OPENTIF_FAILED);
			}
			return FALSE;
		}

		// 使用GDAL读文件
		CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");
		GDALDataset *poDataset;

		float xMin = 0.0f;			// x最小值
		float yMin = 0.0f;			// y最小值
		float cellSize = 0.0f;		// 格网大小
		char tmp[512] = {0};		// 临时字符串

		// 打开
		poDataset = (GDALDataset*)GDALOpen(tifFile, GA_ReadOnly );

		// 打开失败
		if (poDataset == NULL)
		{
			// 设置进度条
			if (loadCallback)
			{
				loadCallback(1.0,HDSCENE_IDS_TODEM_FAILED_GET_TIFDIRVER);
			}
			return FALSE;
		}

		// 设置坐标系,与arcgis相同，写左手系坐标,x、y坐标,左上角为xMin、yMin对应0列0行
		double adfGeoTransform[6] = 
		{m_minx,		// 左上角 x
		m_stepx,	// x方向一个像素代表地理范围
		0,			// 旋转,0代表上面为正北
		m_miny,		// 左上角 y
		0,			// 旋转,0代表上面为正北
		m_stepy};	// y方向一个像素代表地理范围

		// 获取坐标系
		GDALGetGeoTransform(poDataset,adfGeoTransform);

		// 偏移
		m_ModelX = adfGeoTransform[0];
		m_ModelY = adfGeoTransform[3];

		// 记录demmodel相对到绝对坐标转换的model并计算矩阵
		m_transModel.m_fOffset[0] = m_ModelX;
		m_transModel.m_fOffset[1] = m_ModelY;
		m_transModel.m_fOffset[2] = 0.0f;
		m_transModel.Parameter2matrix();

		// 设置范围间距
		m_minx = adfGeoTransform[0];
		m_stepx = (float)adfGeoTransform[1];
		m_miny = adfGeoTransform[3];
		m_stepy = (float)adfGeoTransform[5];

		// 设置行列号
		m_ncols = poDataset->GetRasterXSize();
		m_nrows = poDataset->GetRasterYSize();

		// 采用内存缓存，并采用nodata进行初始化
		m_vecDemData.resize(m_nrows);
		for (int i = 0;i < m_nrows;i++)
		{
			vector<float>& vecPts = m_vecDemData[i];
			vecPts.resize(m_ncols);
			// 初值为Nodata
			for (int j = 0;j < m_ncols;j++)
			{
				vecPts[j] = m_noDataValue;
			}
		}

		// 一次读取一行数据
		float* pPixelBuf = new float[m_ncols];
		memset(pPixelBuf,0,sizeof(float) * m_ncols);
		for (int i = 0;i < m_nrows;i++)
		{
			GDALDatasetRasterIO(poDataset,GF_Read,0,i,m_ncols,1,pPixelBuf,
				m_ncols,1,GDT_Float32,1,0,0,0,0);

			// 将读取的一行数据放入vec容器中
			vector<float>& vecPts = m_vecDemData[i];
			for (int j = 0;j < m_ncols;j++)
			{
				vecPts[j] = pPixelBuf[j];
			}

			// 设置进度条
			if (loadCallback)
			{
				loadCallback((float)(i)/m_nrows,HDSCENE_IDS_PROCESS_READING);
			}
		}

		// 释放
		if (pPixelBuf)
		{
			delete []pPixelBuf;
			pPixelBuf = NULL;
		}

		// 关闭GDAL
		GDALClose( poDataset );

		// 加载该tif文件时对其进行初始化
		m_vecdHInTif.resize(m_nrows);
		for (int i = 0;i < m_nrows;i++)
		{
			vector<float>& vecPts = m_vecdHInTif[i];
			vecPts.resize(m_ncols);
		}

		// 完成
		if (loadCallback)
		{
			loadCallback(0.0f, HDSCENE_IDS_FINISH);
		}

		return TRUE;
	}

	// 设置间距
	void CHdModelPointCloud::SetStepSize(float xStep)
	{
		m_stepx = m_stepy = xStep;
	}

	// 转换至栅格坐标
	void CHdModelPointCloud::world_to_raster( const double* world, float* raster )
	{
		raster[0] = (float)((world[0] - m_minx) / m_stepx) - 0.5f; 
		raster[1] = (float)((m_maxy - world[1]) / m_stepy) - 0.5f; 
		raster[2] = (float)world[2];
	}

	// 将三角网转换至对应内存格网
	void CHdModelPointCloud::raster_triangle_to_memory( vector<vector<float>>& vecPcd,float kill_threshold_squared,const float* a, const float* b, const float* c )
	{
		const float* t;

		// SORT VERTICES BY Y VALUES, breaking ties by x
#define SWAP(a,b,t) t=a,a=b,b=t
#define LEXGREATER(a,b) (a[1]>b[1] || (a[1] == b[1] && a[0]>b[0]))

		// enforce ay<=by<=cy
		if (LEXGREATER(a,c)) 
		{
			SWAP(a,c,t);
		}
		if (LEXGREATER(a,b))
		{
			SWAP(a,b,t);
		}
		else if (LEXGREATER(b,c))
		{
			SWAP(b,c,t);
		}

		// start just above lowest point = a
		int iy = Floor(a[1])+1;    

		if (iy > c[1])
		{
#ifdef COLLECT_STATISTICS
			count_early_exits++;
#endif
			return; // triangles has no rasters
		}

		// Move so b is origin. 
		float Ax=a[0]-b[0], Ay=a[1]-b[1];
		float Cx=c[0]-b[0], Cy=c[1]-b[1];
		float ACx=c[0]-a[0], ACy=c[1]-a[1];

		// orientation determinant: + if A->C is ccw around b. 0 for degenerate line triangle;
		double det = Ax*Cy - Ay*Cx; 

		// triangle is too badly shaped
		if (det == 0 || Ax*Ax + Ay*Ay > kill_threshold_squared || ACx*ACx + ACy*ACy > kill_threshold_squared || Cx*Cx + Cy*Cy > kill_threshold_squared)
		{
			return;  
		}

		double ACxy = ACx/ACy;  // know ACy>0
		double Az = a[2]-b[2], Cz = c[2]-b[2];
		double Dx = (Az*Cy-Ay*Cz)/det;  // linear interp: Z = (x-b0)*Dx+(y-b1)*Dy+b2
		double Dy = (Ax*Cz-Az*Cx)/det; 
		int ix, Xlimit; 

		if (iy <= b[1]) // do a_y to b_y range only if iy<=b_y
		{          
			double xy = Ax/Ay;        // NB: here we know Ay = a_y-b_y < 0.

			for (; iy <= b[1] ; iy++) 
			{        
				// do scanline iy
				int abx = Ceil(b[0] + (float)(xy*(iy-b[1])));  // ix and Xlimit 
				int acx = Ceil(a[0]+(float)(ACxy*(iy-a[1])));  //   for 2 subcases:
				if (det > 0) 
				{
					 // -triangle right of b
					ix = abx; Xlimit = acx;
				} 
				else 
				{
					 // -triangle left of b
					ix = acx; 
					Xlimit = abx;
				};   

				if (ix < Xlimit)
				{
					// starting Z
					double Z = b[2] + Dy*(iy-b[1]) + Dx*(ix-b[0]); 
					for (; ix < Xlimit; ix++) 
					{
						vector<float>& vecPts = vecPcd[iy];
						vecPts[ix] = (float)Z;
						Z += Dx;
					}
				}
			} 
		}

		// do b_y to c_y range only if b_y<c_y
		if (iy <= c[1]) 
		{
			// NB: know Cy = c_y-b_y > 0.
			double xy = Cx/Cy;          

			// do scanline iy
			for (; iy <= c[1]; iy++)
			{  
				int cbx = Ceil(b[0] + (float)(xy*(iy-b[1])));  // ix and Xlimit 
				int cax = Ceil(c[0]+(float)(ACxy*(iy-c[1])));  //   for 2 subcases:
				if (det>0)
				{
					// -triangle right of b
					ix = cbx; 
					Xlimit = cax;
				}  
				else 
				{
					// -triangle left of b
					ix = cax; 
					Xlimit = cbx;
				};    

				if (ix<=Xlimit)
				{
					// starting Z
					double Z = b[2] + Dy*(iy-b[1]) + Dx*(ix-b[0]); 
					for (; ix < Xlimit; ix++)
					{
						vector<float>& vecPts = vecPcd[iy];
						vecPts[ix] = (float)Z;
						Z += Dx;
					}
				}
			}
		}
	}

	// 将三角网转换写至二进制临时文件
	void CHdModelPointCloud::raster_triangle_to_file( FILE* pTempFile,float kill_threshold_squared,const float* a, const float* b, const float* c )
	{
		const float* t;

		// SORT VERTICES BY Y VALUES, breaking ties by x
#define SWAP(a,b,t) t=a,a=b,b=t
#define LEXGREATER(a,b) (a[1]>b[1] || (a[1] == b[1] && a[0]>b[0]))

		// enforce ay<=by<=cy
		if (LEXGREATER(a,c)) 
		{
			SWAP(a,c,t);
		}
		if (LEXGREATER(a,b)) 
		{
			SWAP(a,b,t);
		}
		else if (LEXGREATER(b,c))
		{
			SWAP(b,c,t);
		}

		// start just above lowest point = a
		int iy = Floor(a[1])+1;     
		if (iy > c[1])
		{
#ifdef COLLECT_STATISTICS
			count_early_exits++;
#endif
			return; // triangles has no rasters
		}

		// Move so b is origin. 
		float Ax=a[0]-b[0], Ay=a[1]-b[1];
		float Cx=c[0]-b[0], Cy=c[1]-b[1];
		float ACx=c[0]-a[0], ACy=c[1]-a[1];

		// orientation determinant: + if A->C is ccw around b. 0 for degenerate line triangle;
		double det = Ax*Cy - Ay*Cx; 

		// triangle is too badly shaped
		if (det == 0 || Ax*Ax + Ay*Ay > kill_threshold_squared || ACx*ACx + ACy*ACy > kill_threshold_squared || Cx*Cx + Cy*Cy > kill_threshold_squared)
		{
			return;  
		}

		double ACxy = ACx/ACy;  // know ACy>0
		double Az = a[2]-b[2], Cz = c[2]-b[2];
		double Dx = (Az*Cy-Ay*Cz)/det;  // linear interp: Z = (x-b0)*Dx+(y-b1)*Dy+b2
		double Dy = (Ax*Cz-Az*Cx)/det; 
		int ix, Xlimit; 

		if (iy <= b[1]) // do a_y to b_y range only if iy<=b_y
		{          
			double xy = Ax/Ay;        // NB: here we know Ay = a_y-b_y < 0.

			 // do scanline iy
			for (; iy <= b[1] ; iy++) 
			{       
				int abx = Ceil(b[0] + (float)(xy*(iy-b[1])));  // ix and Xlimit 
				int acx = Ceil(a[0]+(float)(ACxy*(iy-a[1])));  //   for 2 subcases:
				if (det>0) 
				{
					 // -triangle right of b
					ix = abx; 
					Xlimit = acx;
				} 
				else 
				{ 
					// -triangle left of b
					ix = acx; 
					Xlimit = abx;
				};   

				if (ix < Xlimit)
				{
					// starting Z
					double Z = b[2] + Dy*(iy-b[1]) + Dx*(ix-b[0]); 
					for (; ix < Xlimit; ix++) 
					{
						// 定位到iy行，ix列位置
						fseek(pTempFile, (iy*m_ncols + ix)*sizeof(float),SEEK_SET);
						float zValue = (float)Z;
						fwrite(&zValue,sizeof(float),1,pTempFile);
						Z += Dx;
					}
				}
			} 
		}

		if (iy <= c[1]) // do b_y to c_y range only if b_y<c_y
		{
			double xy = Cx/Cy;          // NB: know Cy = c_y-b_y > 0.

			for (; iy <= c[1]; iy++) {    // do scanline iy

				int cbx = Ceil(b[0] + (float)(xy*(iy-b[1])));  // ix and Xlimit 
				int cax = Ceil(c[0]+(float)(ACxy*(iy-c[1])));  //   for 2 subcases:
				if (det>0) 
				{
					// -triangle right of b
					ix = cbx;
					Xlimit = cax;
				}
				else 
				{
					// -triangle left of b
					ix = cax; 
					Xlimit = cbx;
				};    

				if (ix<=Xlimit)
				{
					// starting Z
					double Z = b[2] + Dy*(iy-b[1]) + Dx*(ix-b[0]); 
					for (; ix < Xlimit; ix++)
					{
						// 定位到iy行，ix列位置
						fseek(pTempFile, (iy*m_ncols + ix)*sizeof(float),SEEK_SET);
						float zValue = (float)Z;
						fwrite(&zValue,sizeof(float),1,pTempFile);
						Z += Dx;
					}
				}
			}
		}
	}

	// 内存流数据写入TIF格式文件
	void CHdModelPointCloud::BufferMemory2Tif(const char* savePath,void (*processCallback)(float,const char*))
	{
		// 使用GDAL读tif文件
		CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");

		// 获取GDAL驱动
		OGRSFDriverH hDriver = (OGRSFDriverH)GDALGetDriverByName("Gtiff");

		// 判空
		if (hDriver == NULL)
		{
			if (processCallback)
			{
				processCallback(1.0,HDSCENE_IDS_TODEM_FAILED_GET_TIFDIRVER);
			}
			return;
		}

		// 创建DATESET
		char** papszMetadata = GDALGetMetadata(hDriver,NULL);	
		GDALDatasetH poDataset = GDALCreate(hDriver,savePath,m_ncols,m_nrows,1,GDT_Float32,papszMetadata);
		
		// 判空
		if (poDataset == NULL)
		{
			if (processCallback)
			{
				processCallback(1.0,HDSCENE_IDS_TODEM_FAILED_GET_TIFDIRVER);
			}
			return;
		}

		// paque type used for the C bindings 
		GDALRasterBandH hBand = GDALGetRasterBand( poDataset, 1 );

		// 判空
		if (hBand == NULL)
		{
			return;
		}

		// 设置无效值
		GDALSetRasterNoDataValue(hBand,m_noDataValue);

		// 设置坐标系,设置坐标系,与arcgis相同，写左手系坐标,x、y坐标,左上角为xMin、yMin对应0列0行
		double adfGeoTransform[6] = 
		{m_minx,		// 坐上角 x
		m_stepx,	// x方向一个像素代表地理范围
		0,			// 旋转,0代表上面为正北
		m_miny,		// 左上角 y
		0,			// 旋转,0代表上面为正北
		m_stepy};	// y方向一个像素代表地理范围
		GDALSetGeoTransform(poDataset,adfGeoTransform);

		// 按行遍历
		float* pBuffer = new float[m_ncols];
		for (int i = 0;i < m_nrows;i++)
		{
			// 按行读进缓存
			vector<float>& vecPts = m_vecDemData[i];
			for (int j = 0;j < m_ncols;j++)
			{
				pBuffer[j] = vecPts[j];
			}

			// 写进tif
			GDALDatasetRasterIO(poDataset,GF_Write,0,m_nrows - i - 1,m_ncols,1,pBuffer,
				m_ncols,1,GDT_Float32,1,0,0,0,0);

			// 设置进度条
			if (processCallback)
			{
				processCallback((float)(i)/m_nrows,HDSCENE_IDS_TODEM_CREATING_TIF);
			}
		}

		// 完成
		if (processCallback)
		{
			processCallback(0,HDSCENE_IDS_FINISH);
		}

		// 关闭
		GDALClose( poDataset );

		// 判空并释放
		if (pBuffer)
		{
			delete []pBuffer;
			pBuffer = NULL;
		}	
	}

	// 临时文件写入TIF格式文件
	void CHdModelPointCloud::BufferFile2Tif( FILE* pTempFile,const char* savePath,void (*processCallback)(float,const char*) /*= NULL*/ )
	{
		// 判断文件指针是否为空
		if (!pTempFile)
		{
			return;
		}

		// 使用GDAL读tif文件
		CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");

		// 获取GDAL驱动
		OGRSFDriverH hDriver = (OGRSFDriverH)GDALGetDriverByName("Gtiff");

		// 判空
		if (hDriver == NULL)
		{
			if (processCallback)
			{
				processCallback(1.0,HDSCENE_IDS_TODEM_FAILED_GET_TIFDIRVER);
			}
			return;
		}

		// 创建DATESET
		char** papszMetadata = GDALGetMetadata(hDriver,NULL);	
		GDALDatasetH poDataset = GDALCreate(hDriver,savePath,m_ncols,m_nrows,1,GDT_Float32,papszMetadata);
		
		// 判空
		if (poDataset == NULL)
		{
			if (processCallback)
			{
				processCallback(1.0,HDSCENE_IDS_TOTIN_CREATEFAILED);
			}
			return;
		}

		// paque type used for the C bindings 
		GDALRasterBandH hBand = GDALGetRasterBand( poDataset, 1 );

		// 判空
		if (hBand == NULL)
		{
			return;
		}

		// 设置无效值
		GDALSetRasterNoDataValue(hBand,m_noDataValue);

		// 设置坐标系,设置坐标系,与arcgis相同，写左手系坐标,x、y坐标,左上角为xMin、yMin对应0列0行
		double adfGeoTransform[6] = 
		{m_minx,		// 坐上角 x
		m_stepx,	// x方向一个像素代表地理范围
		0,			// 旋转,0代表上面为正北
		m_miny,		// 左上角 y
		0,			// 旋转,0代表上面为正北
		m_stepy};	// y方向一个像素代表地理范围
		GDALSetGeoTransform(poDataset,adfGeoTransform);

		// 按行遍历
		float* pBuffer = new float[m_ncols];
		for (int i = 0;i < m_nrows;i++)
		{
			// 按行读进缓存
			fseek(pTempFile,(m_ncols*i)*sizeof(float),SEEK_SET);
			fread(pBuffer,m_ncols*sizeof(float),1,pTempFile);

			// 写进tif
			GDALDatasetRasterIO(poDataset,GF_Write,0,m_nrows - i - 1,m_ncols,1,pBuffer,
				m_ncols,1,GDT_Float32,1,0,0,0,0);

			// 设置进度条
			if (processCallback)
			{
				processCallback((float)(i)/m_nrows,HDSCENE_IDS_TODEM_CREATING_TIF);
			}
		}

		// 完成
		if (processCallback)
		{
			processCallback(0,HDSCENE_IDS_FINISH);
		}

		// 关闭
		GDALClose( poDataset );

		// 判空并清楚缓存
		if (pBuffer)
		{
			delete []pBuffer;
			pBuffer = NULL;
		}	
	}

	// 临时文件写入asc格式文件
	void CHdModelPointCloud::BufferMemory2Asc(const char* savePath,void (*processCallback)(float,const char*) /*= NULL*/ )
	{
		// 打开文件指针
		FILE* pFile = fopen(savePath,"w+t");

		// 判空
		if (!pFile)
		{
			return;
		}

		//数据头文件
		float noData =-9999.0f;//表示该处为NODATA
		fprintf(pFile,"%s","ncols         ");			
		fprintf(pFile,"%d\n",m_ncols);					// 列
		fprintf(pFile,"%s","nrows         ");
		fprintf(pFile,"%d\n",m_nrows);					// 行
		fprintf(pFile,"%s","xllcorner     ");
		fprintf(pFile,"%f\n",m_minx);					// x最小值
		fprintf(pFile,"%s","yllcorner     ");
		fprintf(pFile,"%f\n",m_miny);					// y最小值
		fprintf(pFile,"%s","cellsize      ");
		fprintf(pFile,"%f\n",m_stepx);					// 格网大小
		fprintf(pFile,"%s","NODATA_value  ");
		fprintf(pFile,"%f\n",m_noDataValue);			// 无效值

		// z临时变量
		float buffer = 0.0f;

		// 遍历
		for (int i = 0;i < m_nrows;i++)
		{
			// 遍历内存，写进asc
			vector<float>& vecPts = m_vecDemData[i];
			for (int j = 0;j < m_ncols;j++)
			{
				buffer = vecPts[j];
				fprintf(pFile,"%f ",buffer);
			}
			fprintf(pFile,"\n");

			// 设置进度条
			if (processCallback)
			{
				processCallback((float)(i)/m_nrows,HDSCENE_IDS_TODEM_CREATING_ASC);
			}
		}

		// 关闭文件指针
		fclose(pFile);
	}

	// 内存流数据写入asc格式文件
	void CHdModelPointCloud::BufferFile2Asc( FILE* pTempFile,const char* savePath,void (*processCallback)(float,const char*) /*= NULL*/ )
	{
		// 打开文件指针
		FILE* pFile = fopen(savePath,"w+t");

		// 判空
		if (!pFile)
		{
			return;
		}

		//数据头文件
		float noData =-9999.0f;//表示该处为NODATA
		fprintf(pFile,"%s","ncols         ");
		fprintf(pFile,"%d\n",m_ncols);						// 列
		fprintf(pFile,"%s","nrows         ");
		fprintf(pFile,"%d\n",m_nrows);						// 行
		fprintf(pFile,"%s","xllcorner     ");
		fprintf(pFile,"%f\n",m_minx);						// x最小值
		fprintf(pFile,"%s","yllcorner     ");
		fprintf(pFile,"%f\n",m_miny);						// y最小值
		fprintf(pFile,"%s","cellsize      ");
		fprintf(pFile,"%f\n",m_stepx);						// 格网大小
		fprintf(pFile,"%s","NODATA_value  ");
		fprintf(pFile,"%f\n",m_noDataValue);				// 无效值

		// z中间缓存
		float buffer = 0.0f;

		// 遍历
		for (int i = 0;i < m_nrows;i++)
		{
			for (int j = 0;j < m_ncols;j++)
			{
				// 从文件中按行列查找，读取Z值
				fseek(pTempFile,(m_ncols * i + j )*sizeof(float),SEEK_SET);
				fread(&buffer,sizeof(float),1,pTempFile);

				// 写进asc
				if (buffer == 0.0f)
				{
					// 无效值
					fprintf(pFile,"%f ",m_noDataValue);
				}
				else
				{
					// 有效值
					fprintf(pFile,"%f ",buffer);
				}
			}
			fprintf(pFile,"\n");

			// 设置进度条
			if (processCallback)
			{
				processCallback((float)(i)/m_nrows,HDSCENE_IDS_TODEM_CREATING_ASC);
			}
		}

		// 关闭文件指针
		fclose(pFile);
	}

	// 导出tif or asc
	void CHdModelPointCloud::Write2Dem(const char* savePath,void (*processCallback)(float,const char*))
	{
		// 文件路径
		string strPath = savePath;

		// 文件路径名称
		string strTempPath = strPath + ".temp";
		std::transform(strPath.begin(),strPath.begin(),strPath.end(),tolower);

		// 判断文件合法性
		if (strPath.find(".bil") == -1 && strPath.find(".tif") == -1 && strPath.find(".asc") == -1)
		{
			return ;
		}

		// 判断行列合法性
		if (m_ncols < 0 || m_nrows < 0)
		{
			return;
		}

		// 行列比在vecPcd范围内使用内存vector存储，超过选择临时文件存储
		vector<vector<float>> vecPcd;
		FILE* pTempFile = NULL;

		// 判断是否使用文件缓存或内存缓存
		if (m_ncols * m_nrows <= 2048*2048)
		{
			//采用内存缓存，并采用nodata进行初始化
			m_bBufferInMemory = true;

			// 初始化缓存内存
			vecPcd.resize(m_nrows);
			for (int i = 0;i < m_nrows;i++)
			{
				vector<float>& vecPts = vecPcd[i];
				vecPts.resize(m_ncols);

				//初值为Nodata
				for (int j = 0;j < m_ncols;j++)
				{
					vecPts[j] = m_noDataValue;
				}
			}
		}
		else
		{
			//采用文件缓存，此处进行打开临时文件
			pTempFile = fopen(strTempPath.c_str(),"w+b");
			m_bBufferInMemory = false;
			fclose(pTempFile);
		}

		// 将内存(临时文件)点写入对应格式DEM文件
		if (strPath.find(".tif") != -1)
		{
			if (m_bBufferInMemory)
			{
				// 内存到tif
				BufferMemory2Tif(savePath,processCallback);
			}
			else
			{
				// 临时文件到tif
				pTempFile = fopen(strTempPath.c_str(),"rb");
				BufferFile2Tif(pTempFile,savePath,processCallback);
				fclose(pTempFile);
			}
		}
		else if (strPath.find(".asc") != -1)
		{
			if (m_bBufferInMemory)
			{
				// 内存到asc
				BufferMemory2Asc(savePath,processCallback);
			}
			else
			{
				// 临时文件到asc
				pTempFile = fopen(strTempPath.c_str(),"rb");
				BufferFile2Asc(pTempFile,savePath,processCallback);
				fclose(pTempFile);
			}
		}

		// 文件缓存删除临时文件
		if (!m_bBufferInMemory)
		{
			remove(strTempPath.c_str());
		}

		// 删除构建的三角形
		TINdestroy();
	}

	bool CHdModelPointCloud::BuildTriangleInDemFile()
	{
		// 首先清除之前构建的三角形索引
		//m_TriangleIndexInDem.clear();
		m_TrianlgeIndex.clear();

		// 定义中间变量
		float fIndexZ[4];
		int tmpIndex,count;

		// 逐点进行遍历生成三角形
		for (int i = 0;i < m_nrows-1;i++)
		{
			for (int j = 0;j < m_ncols-1;j++)
			{
				// 去除无效点
				// 如果主对角线上的两个点无效，则整个索引无效
				// 如果只有一个副对角线上的点无效，则可以添加一个三角形
				s32 index11,index12,index21,index22;
				index11 = i * m_ncols + j;
				index12 = i * m_ncols + j + 1;
				index21 = (i+1) * m_ncols + j;
				index22 = (i+1) * m_ncols + j + 1;

				//hdTrianglePtIndex index11,index12,index21,index22;
				//index11.col = j;
				//index11.row = i;

				//index12.col = j+1;
				//index12.row = i;

				//index21.col = j;
				//index21.row = i+1;

				//index22.col = j+1;
				//index22.row = i+1;

				fIndexZ[0] = GetZ(j,i);
				fIndexZ[1] = GetZ(j+1,i);
				fIndexZ[2] = GetZ(j,i+1);
				fIndexZ[3] = GetZ(j+1,i+1);

				// 判断顶点，是否存在无效值,存在无效点时，做对应处理
				tmpIndex = -1;
				count = getValidCount(fIndexZ,tmpIndex);

				// 尝试捕获异常
				try
				{
					if (count == 0)
					{
						// 将三角形插入到渲染索引数组
						m_TrianlgeIndex.push_back(index12);
						m_TrianlgeIndex.push_back(index11);
						m_TrianlgeIndex.push_back(index22);
						m_TrianlgeIndex.push_back(index22);
						m_TrianlgeIndex.push_back(index11);
						m_TrianlgeIndex.push_back(index21);
					}
					else if (count == 1)
					{
						if (tmpIndex == 0)
						{
							m_TrianlgeIndex.push_back(index12);
							m_TrianlgeIndex.push_back(index21);
							m_TrianlgeIndex.push_back(index22);
						}
						else if (tmpIndex == 1)
						{
							m_TrianlgeIndex.push_back(index11);
							m_TrianlgeIndex.push_back(index21);
							m_TrianlgeIndex.push_back(index22);
						}
						else if (tmpIndex == 2)
						{
							m_TrianlgeIndex.push_back(index11);
							m_TrianlgeIndex.push_back(index12);
							m_TrianlgeIndex.push_back(index22);
						}
						else if (tmpIndex == 3)
						{
							m_TrianlgeIndex.push_back(index11);
							m_TrianlgeIndex.push_back(index12);
							m_TrianlgeIndex.push_back(index21);
						}
					}
					else
					{
						continue;
					}
				}
				catch(...)
				{
					::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);
					return false;
				}
			}
		}

		return true;
	}

	float CHdModelPointCloud::GetZ( int col,int row )
	{
		int nCount = m_vecDemData.size();
		if (row >= 0 && row < nCount)
		{
			vector<float>& vecPts = m_vecDemData[row];
			return vecPts[col];
		}
		return 0.0f;
	}

	int CHdModelPointCloud::getValidCount( float* fIndexZ,int& index )
	{
		int count = 0;
		for (int i = 0;i < 4;i++)
		{
			if (fIndexZ[i] <= m_noDataValue)
			{
				index = i;
				count++;
			}
		}

		return count;
	}

	// 获得该坐标为攫夺坐标
	irr::video::S3DVertex2TCoords CHdModelPointCloud::GetValue( u32 index )
	{
		S3DVertex2TCoords pt;

		int row,col;
		row = index / m_ncols;
		col = index % m_ncols;

		int nCount = m_vecDemData.size();
		if (row >= 0 && row < nCount)
		{
			vector<float>& vecPts = m_vecDemData[row];
			pt.Pos.Z = vecPts[col];

			pt.Pos.X = (float)(m_ModelX + col * m_stepx);
			pt.Pos.Y = (float)(m_ModelY + row * m_stepy);
		}

		return pt;
	}

	void CHdModelPointCloud::CalculateCutfillBoxes()
	{
		// 首先进行清空处理
		m_vecCutFillBox.clear();

		// 以显示的每个点为中心，绘制一个长方体
		double dx,dy,dz;
		dx = dy = dz = 0.0;
		double sizeStepX = m_stepx;
		double sizeStepY = m_stepy;

		// 遍历点绘制长方体
		for (unsigned int i = 0;i < m_vecdHInTif.size();i++)
		{
			vector<float>& vecPts = m_vecdHInTif[i];
			for (unsigned int j = 0; j < vecPts.size();j++)
			{
				float dH = vecPts[j];

				// 变化过小的不显示
				if (abs(dH) < 0.000001)
				{
					continue;
				}

				// 计算框体,此处获得为绝对坐标
				core::aabbox3df box;
				S3DVertex2TCoords coords = GetValue(i * m_ncols + j);

				// 赋值
				dx = coords.Pos.X;
				dy = coords.Pos.Y;
				dz = coords.Pos.Z;

				// 转换至显示坐标下
				GetabsCoord(dx,dy);

				// 设置box
				if (dH > 0.0f)
				{
					box.MinEdge.set(float(dx - sizeStepX / 2.0f),float(dy - sizeStepY / 2.0f),float(dz));
					box.MaxEdge.set(float(dx + sizeStepX / 2.0f),float(dy + sizeStepY / 2.0f),float(dz + dH));
				}
				else
				{
					box.MinEdge.set(float(dx - sizeStepX / 2.0f),float(dy - sizeStepY / 2.0f),float(dz + dH));
					box.MaxEdge.set(float(dx + sizeStepX / 2.0f),float(dy + sizeStepY / 2.0f),float(dz));
				}

				m_vecCutFillBox.push_back(box);
			}
		}
	}

}

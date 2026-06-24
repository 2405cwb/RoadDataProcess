/*! HdTINPointCloud.cpp
********************************************************************************
<PRE>
模块名       : hdPointCloud
文件名       : HdTINPointCloud.cpp
相关文件     : HdTINPointCloud.h
文件实现功能 : 点云模型TIN内存结构
作者         : 研发部 冯晶
版本         : 1.0
版权		 : CopyRight @ 2013 海达数云
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容  
2014/03/05   1.0        冯晶                 新建
</PRE>
*******************************************************************************/
#include "StdAfx.h"
#include "HdTINPointCloud.h"
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
#include "..\hdCommon\hdTin.h"
#include "vtk\VtkReader.h"
#include "vtk\VtkWriter.h"

#include <stdlib.h>
#include <math.h>
#include <algorithm>

namespace hd
{
	// 构造
	CHdTINPointCloud::CHdTINPointCloud()
		: m_nIndicesToRender(0),
		m_TriagleQuadIndex(NULL),
		m_dx(0.0),
		m_dy(0.0),
		m_dz(0.0),
		m_bisTif2Obj(false)
	{
		m_MaxGridSize = 2.0;
		m_MaxTriaSIzeLen = 2.0;
	}

	// 带参数构造
	CHdTINPointCloud::CHdTINPointCloud(bool isTif2Obj)
		: m_nIndicesToRender(0),
		m_TriagleQuadIndex(NULL),
		m_dx(0.0),
		m_dy(0.0),
		m_dz(0.0),
		m_bisTif2Obj(isTif2Obj)
	{
		m_MaxGridSize = 2.0;
		m_MaxTriaSIzeLen = 2.0;
	}

	// 析构
	CHdTINPointCloud::~CHdTINPointCloud()
	{
		if (m_TriagleQuadIndex)
		{
			delete m_TriagleQuadIndex;
			m_TriagleQuadIndex = NULL;
		}
	}

	// 创建索引
	void CHdTINPointCloud::CreateQuadIndex()
	{
		// 判断合法性
		if (m_TriagleQuadIndex)
		{
			delete m_TriagleQuadIndex;
			m_TriagleQuadIndex = NULL;
		}

		// 创建索引对象
		m_TriagleQuadIndex = new CHdQuadSourceIndex(m_pts._Myfirst(), m_TrianlgeIndex._Myfirst(), m_TrianlgeIndex.size());

		// 新建索引
		m_TriagleQuadIndex->BuildQuadTree(m_box);
	}

	// 使三角形按照邻接位置关系进行排序
	void CHdTINPointCloud::GetOrderTriaIndex(vector<u32>& items)
	{
		if (items.size() == 0)
		{
			return;
		}
		// 将初始的三角形索引拷贝一份
		vector<u32> OrginItems = items;

		// 包围盒
		core::aabbox3df	box(F32_MAX,F32_MAX,F32_MAX,F32_MIN,F32_MIN,F32_MIN);				

		// 统计当前三角形的包围盒
		for (unsigned int i = 0; i < OrginItems.size(); i+=3)
		{
			// 取三角形的三个顶点
			core::vector3df point0(m_pts[OrginItems[i]].Pos.X, m_pts[OrginItems[i]].Pos.Y, m_pts[OrginItems[i]].Pos.Z);
			core::vector3df point1(m_pts[OrginItems[i + 1]].Pos.X, m_pts[OrginItems[i + 1]].Pos.Y, m_pts[OrginItems[i + 1]].Pos.Z);
			core::vector3df point2(m_pts[OrginItems[i + 2]].Pos.X, m_pts[OrginItems[i + 2]].Pos.Y, m_pts[OrginItems[i + 2]].Pos.Z);

			box.addInternalPoint(point0);
			box.addInternalPoint(point1);
			box.addInternalPoint(point2);
		}

		// 创建索引
		CHdQuadSourceIndex TriagleQuadIndex(m_pts._Myfirst(), OrginItems._Myfirst(), OrginItems.size());
		TriagleQuadIndex.BuildQuadTree(box);

		int count = OrginItems.size() / 3;

		// 获取第一个三角形

		unsigned int i0 = OrginItems[0];
		unsigned int i1 = OrginItems[1];
		unsigned int i2 = OrginItems[2];

		items.clear();
		items.push_back(i0);
		items.push_back(i1);
		items.push_back(i2);

		bool isReOrder = false;
		// 统计当前三角形的包围盒
		while(count > 0)
		{
			// 找到最远的三角形，使用纹理位进行标记
			m_pts[i0].TCoords.set(i1, i2);

			vector<u32> AdjionTrias;
			TriagleQuadIndex.SearchAdjionTria(TriagleQuadIndex.GetQuardRoot(), i0, i1, i2, AdjionTrias);
			// 当前点
			core::vector3df point((m_pts[i0].Pos.X + m_pts[i1].Pos.X + m_pts[i2].Pos.X) / 3, 
				(m_pts[i0].Pos.Y + m_pts[i1].Pos.Y + m_pts[i2].Pos.Y) / 3,
				(m_pts[i0].Pos.Z + m_pts[i1].Pos.Z + m_pts[i2].Pos.Z) / 3);

			float maxdist = 0.f;
			int maxIndex = -1;
			bool isfist = true;
			bool isBreak = false;
			int itemSize = items.size();

			for (int i = 0; i < AdjionTrias.size(); i += 3)
			{
				for (int j = 0; j < items.size(); j+=3)
				{
					if (items[j] == AdjionTrias[i] && items[j + 1] == AdjionTrias[i + 1] && items[j + 2] == AdjionTrias[i + 2] )
					{	
						isBreak = true;
						break;

					}
				}

				if (isBreak)
				{
					isBreak = false;
					continue;
				}

				/*	if ((int)m_pts[AdjionTrias[i]].TCoords.X == AdjionTrias[i + 1] && (int)m_pts[AdjionTrias[i]].TCoords.Y == AdjionTrias[i + 2])
				{
				continue;
				}
				*/
				core::vector3df OtherPT((m_pts[AdjionTrias[i]].Pos.X + m_pts[AdjionTrias[i + 1]].Pos.X + m_pts[AdjionTrias[i + 2]].Pos.X) / 3, 
					(m_pts[AdjionTrias[i]].Pos.Y + m_pts[AdjionTrias[i + 1]].Pos.Y + m_pts[AdjionTrias[i + 2]].Pos.Y) / 3,
					(m_pts[AdjionTrias[i]].Pos.Z + m_pts[AdjionTrias[i + 1]].Pos.Z + m_pts[AdjionTrias[i + 2]].Pos.Z) / 3);


				float dist = point.getDistanceFrom(OtherPT);
				if (isfist)
				{
					maxdist = dist;
					maxIndex = i;
					isfist = false;
				}

				// 记录最大值
				if (maxdist < dist)
				{
					maxdist = dist;
					maxIndex = i;
				}
				//m_pts[AdjionTrias[i]].TCoords.set(AdjionTrias[i + 1], AdjionTrias[i + 2]);
			}


			// 没有找到下一个处理的三角形
			if (maxIndex == -1)
			{
				// 表示连续的三角形搜索完毕, 对于一簇连续的三角形，只逆序一次
				if (isReOrder)
				{
					int nNoSelect = 0;
					bool bfirst = false;
					// 统计当前三角形的包围盒
					for (unsigned int j  = 0; j < OrginItems.size(); j+=3)
					{
						// 选中
						if ((int)m_pts[OrginItems[j]].TCoords.X == OrginItems[j + 1] && (int)m_pts[OrginItems[j]].TCoords.Y == OrginItems[j + 2])
						{
							continue;
						}
						else
						{
							if (!bfirst)
							{
								i0 = OrginItems[j];
								i1 = OrginItems[j + 1];
								i2 = OrginItems[j + 2];
								bfirst = true;
							}
							nNoSelect++;
						}
					}
					if (nNoSelect <= 2)
					{
						break;
					}
					else
					{
						isReOrder = false;
					}
				}
				else
				{
					// 更换查找方向
					i0 = items[0];
					i1 = items[1];
					i2 = items[2];

					// 逆序
					for (u32 j = 0 ; j < items.size() / 2 + 1; j++)
					{
						u32 cur = items[items.size()-1-j];
						items[items.size()-1-j] = items[j];
						items[j] = cur;
					}
					isReOrder = true;
				}

			}
			else
			{

				// 找到最远的三角形，使用纹理位进行标记
				//m_pts[AdjionTrias[maxIndex]].TCoords.set(AdjionTrias[maxIndex + 1], AdjionTrias[maxIndex + 2]);

				// 下一个待查三角形
				i0 = AdjionTrias[maxIndex];
				i1 = AdjionTrias[maxIndex+1];
				i2 = AdjionTrias[maxIndex+2];
				items.push_back(i0);
				items.push_back(i1);
				items.push_back(i2);
				count--;

			}
		}
	}

	// 按范围进行搜索
	void CHdTINPointCloud::SearchQuadIndex(const core::aabbox3df& SelBox, vector<u32>& items)
	{
		// 判断合法性
		if (!m_TriagleQuadIndex)
		{
			return;
		}

		// 遍历四叉树进行搜索
		m_TriagleQuadIndex->SearchQuadTree(m_TriagleQuadIndex->GetQuardRoot(), SelBox, items);
	}

	// 按视椎体进行搜索
	void  CHdTINPointCloud::SearchQuadIndex(const irr::scene::SViewFrustum& frustum, vector<u32>& items)
	{
		// 判断合法性
		if (!m_TriagleQuadIndex)
		{
			return;
		}

		// 遍历四叉树进行搜索
		m_TriagleQuadIndex->SearchQuadTree(m_TriagleQuadIndex->GetQuardRoot(), frustum, items);
	}

	// 删除指定三角形
	bool CHdTINPointCloud::DeleteQuadTriaIndex(vector<u32>& items)
	{
		// 判断合法性
		if (!m_TriagleQuadIndex)
		{
			return false;
		}

		// 删除三角形索引
		return m_TriagleQuadIndex->DeleteQuadTree(items);
	}

	// 获取全局坐标
	void CHdTINPointCloud::GetGlobalCoord(double& gx, double& gy, double& gz)
	{
		gx += m_dx;
		gy += m_dy;
		gz += m_dz;
	}

	// 获取相对坐标, 传进来的参数为绝对坐标，传出去的坐标为相对坐标
	void CHdTINPointCloud::GetabsCoord(double& x, double& y, double& z)
	{
		x -= m_dx;
		y -= m_dy;
		z -= m_dz;
	}

	// 加载点云文件，支持格式hls，las，laz，bin
	int CHdTINPointCloud::loadFile(const char* hlsFile, void (*loadCallback)(float,const char*)/* = NULL*/)
	{
		// 输入文件路径
		std::string filePath = hlsFile;  

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
			// hls文件
			return loadHlsFile(filePath.c_str(), loadCallback);
		}
		else if (filePath.find(".las") != -1 
			|| filePath.find(".laz") != -1 
			|| filePath.find(".bin") != -1)
		{
			return loadLasFile(filePath.c_str(), loadCallback);
		}
		else if (filePath.find(".tif") != -1)
		{
			// tif文件
			return loadTifFile(filePath.c_str(), loadCallback);
		}
		else if (filePath.find(".obj") != -1)
		{
			// obj文件
			return loadObjFile(filePath.c_str(), loadCallback);
		}
		else if (filePath.find(".asc") != -1)
		{
			// asc文件
			return loadAscFile(filePath.c_str(), loadCallback);
		}
		else if (filePath.find(".vtk") != -1)
		{
			return LoadVtkFile(filePath.c_str(), loadCallback);
		}
		else if (filePath.find(".dat") != -1)
		{
			return loadDatFile(filePath.c_str(), loadCallback);
		}
		return 0;
	}

	// 将hls文件一次性加载到内存
	int CHdTINPointCloud::loadHlsFile(const char* hlsFile, void (*loadCallback)(float,const char*)/* = NULL*/)
	{
		// 输入文件路径
		m_filePath = hlsFile; 

		// 文件路径为空
		if (m_filePath == "")
		{
			return -1;
		}

		// 读取HLS格式文件
		IHLSReader* hlsReader = NULL;
		CHLSReadOpener hlsOpen;

		// 打开文件
		hlsReader = hlsOpen.Open(m_filePath.c_str());
		if(hlsReader == NULL)
		{
			// 打开失败，返回
			return -1;
		}

		int count = 0;								// 计数器
		hlsReader->GetLoopIndex();					// 读取圈索引
		U64 npoints = 0;							// 有效点数
		U32 loopCount = hlsReader->GetLoopCount();	// 圈数	
		hdVector<PointXYZIPRGBA> pTmpBuf;			// 每圈有效点数
		double x,y,z;								// 临时点遍历
		int i = 0;									// 点计数器

		// 先遍历获取有效点数
		while(hlsReader->ReadLoop(pTmpBuf,i++))
		{
			npoints += pTmpBuf.size();
		}

		// 包围盒定义
		double minx, miny, minz, maxx, maxy, maxz;

		// 获得点云文件全局坐标范围
		hlsReader->m_header.getGlobalExtent(minx,miny,minz,maxx,maxy,maxz);
		core::aabbox3df box((float)minx,(float)miny,(float)minz,(float)maxx,(float)maxy,(float)maxz);

		// 设置当前包围盒
		m_box = box;

		// 调用TIN的生成接口
		CHdTin tin;		

		// 点云不为空且未构建三角形
		if (npoints > 0 && tin.get_size() == 0)
		{
			// 初始化
			bool ret = tin.initial((int)npoints);

			// 判断合法性
			if (!ret)
			{
				if (hlsReader)
				{
					delete hlsReader;
					hlsReader = NULL;
				}

				// 分配失败
				::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);
				return -1;
			}
		}

		// 分配空间
		try
		{
			m_pts.resize((u32)npoints);
		}
		catch (...)
		{
			// 分配失败，退出
			int sz = m_pts.size();
			if (sz)
			{
				// 清除已经分配
				vector<S3DVertex2TCoords> ().swap(m_pts);
			}

			if (hlsReader)
			{
				delete hlsReader;
				hlsReader = NULL;
			}

			// 弹出提示对话框
			::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);

			// 状态栏设为完成
			if (loadCallback)
			{
				loadCallback(0,HDSCENE_IDS_FAILED);
			}
			return 0;
		}

		// 重置圈数计数器
		i = 0;		

		// 按圈遍历点云
		while(hlsReader->ReadLoop(pTmpBuf,i++))
		{
			// 遍历每圈点云，生成TIN
			for (U32 n = 0;n < pTmpBuf.size();n++)
			{
				// 当前点
				const PointXYZIPRGBA& pt = *(pTmpBuf._Myfirst + n);

				// 获取当前点全局坐标
				hlsReader->GetCoordinate(pt.x,pt.y,pt.z,x,y,z);

				// 设置为模型数据
				S3DVertex2TCoords& TinPcd = m_pts[count];
				TinPcd.Pos.X = (float)x;
				TinPcd.Pos.Y = (float)y;
				TinPcd.Pos.Z = (float)z;

				// 构建tin
				tin.add((float*)&TinPcd.Pos.X,count);

				// 模型数据计数器
				count++;

				// 进度条提示
				if ((count % 10000) == 0 && loadCallback)
				{
					loadCallback((F32)count/npoints,HDSCENE_IDS_TODEM_CREATING_TIN);
				}
			}
		}

		// 构建TIN完成
		tin.finish();

		// 计算法向量
		CalculateNormal(loadCallback);

		// 给三角形索引创建索引
		CreateQuadIndex();

		// 加载完成
		if (loadCallback)
		{
			loadCallback(0,HDSCENE_IDS_FINISH);
		}

		// 关闭
		if (hlsReader)
		{
			delete hlsReader;
			hlsReader = NULL;
		}

		return count;
	}

	// 将内存中的点云加载至模型
	int CHdTINPointCloud::loadHlsFile(PointCloud* pts, void (*loadCallback)(float,const char*))
	{
		// 判断合法性
		if (!pts)
		{
			return -1;
		}

		int count = 0;								// 计数器
		U64 npoints = 0;							// 有效点数
		int i = 0;									// 点计数器

		// 包围盒定义
		double minx, miny, minz, maxx, maxy, maxz;

		// 获得点云文件全局坐标范围
		pts->m_header.getGlobalExtent(minx,miny,minz,maxx,maxy,maxz);
		core::aabbox3df box((float)minx,(float)miny,(float)minz,(float)maxx,(float)maxy,(float)maxz);

		// 设置当前包围盒
		m_box = box;

		npoints = pts->count();

		// 调用TIN的生成接口
		CHdTin tin;		

		// 点云不为空且未构建三角形
		if (npoints > 0 && tin.get_size() == 0)
		{
			// 初始化
			bool ret = tin.initial((int)npoints);

			// 判断合法性
			if (!ret)
			{
				// 分配失败
				::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);
				return -1;
			}
		}

		// 分配空间
		try
		{
			m_pts.resize((u32)npoints);
		}
		catch (...)
		{
			// 分配失败，退出
			int sz = m_pts.size();
			if (sz)
			{
				// 清除已经分配
				vector<S3DVertex2TCoords> ().swap(m_pts);
			}

			// 弹出提示对话框
			::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);

			// 状态栏设为完成
			if (loadCallback)
			{
				loadCallback(0,HDSCENE_IDS_FAILED);
			}
			return 0;
		}


		hdBlkArray<PointXYZIPRGBA>& Ppts = pts->getPoints();

		for (U32 n = 0;n < npoints;n++)
		{
			// 当前点
			PointXYZI_D pt;

			// 获取当前点全局坐标
			pts->getGlobal(n, pt);

			// 设置为模型数据
			S3DVertex2TCoords& TinPcd = m_pts[count];
			TinPcd.Pos.X = (float)pt.x;
			TinPcd.Pos.Y = (float)pt.y;
			TinPcd.Pos.Z = (float)pt.z;

			// 构建tin
			tin.add((float*)&TinPcd.Pos.X,count);

			// 模型数据计数器
			count++;

			// 进度条提示
			if ((count % 10000) == 0 && loadCallback)
			{
				loadCallback((F32)count/npoints,HDSCENE_IDS_TODEM_CREATING_TIN);
			}
		}


		// 构建TIN完成
		tin.finish();

		// 计算法向量
		CalculateNormal(loadCallback);

		//// 给三角形索引创建索引
		//CreateQuadIndex();

		// 加载完成
		if (loadCallback)
		{
			loadCallback(0,HDSCENE_IDS_FINISH);
		}

		return count;
	}

	// 按圈数抽稀加载
	int CHdTINPointCloud::loadHlsFile(const char* hlsFile,int loop,void (*loadCallback)(float,const char*))
	{
		// 输入文件路径
		m_filePath = hlsFile; 

		// 文件路径为空
		if (m_filePath == "")
		{
			return -1;
		}

		// 读取HLS格式文件
		IHLSReader* hlsReader = NULL;
		CHLSReadOpener hlsOpen;

		// 打开文件
		hlsReader = hlsOpen.Open(m_filePath.c_str());
		if(hlsReader == NULL)
		{
			// 打开失败，返回
			return -1;
		}

		int count = 0;								// 计数器
		hlsReader->GetLoopIndex();					// 读取圈索引
		U64 npoints = 0;							// 有效点数
		U32 loopCount = hlsReader->GetLoopCount();	// 圈数	
		hdVector<PointXYZIPRGBA> pTmpBuf;			// 每圈有效点数
		double x,y,z;								// 临时点遍历
		int i = 0;									// 点计数器

		// 先遍历获取有效点数
		while(hlsReader->ReadLoop(pTmpBuf,i))
		{
			npoints += pTmpBuf.size();
			i+=loop;
		}

		// 包围盒定义
		double minx, miny, minz, maxx, maxy, maxz;

		// 获得点云文件全局坐标范围
		hlsReader->m_header.getGlobalExtent(minx,miny,minz,maxx,maxy,maxz);
		core::aabbox3df box((float)minx, (float)miny, (float)minz, (float)maxx, (float)maxy, (float)maxz);

		// 设置当前包围盒
		m_box = box;

		// 调用TIN的生成接口
		CHdTin tin;		

		// 点云不为空且未构建三角形
		if (npoints > 0 && tin.get_size() == 0)
		{
			// 初始化
			bool ret = tin.initial((int)npoints);

			// 判断合法性
			if (!ret)
			{
				if (hlsReader)
				{
					delete hlsReader;
					hlsReader = NULL;
				}

				// 分配失败
				::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);
				return -1;
			}
		}

		// 分配空间
		try
		{
			m_pts.resize((int)npoints);
		}
		catch (...)
		{
			// 分配失败，退出
			int sz = m_pts.size();
			if (sz)
			{
				// 清除已经分配
				m_pts.clear();
			}

			if (hlsReader)
			{
				delete hlsReader;
				hlsReader = NULL;
			}

			// 弹出提示对话框
			::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);

			// 状态栏设为完成
			if (loadCallback)
			{
				loadCallback(0,HDSCENE_IDS_FAILED);
			}
			return 0;
		}

		// 重置圈数计数器
		i = 0;		

		// 按圈遍历点云
		while(hlsReader->ReadLoop(pTmpBuf,i))
		{
			// 遍历每圈点云，生成TIN
			for (U32 n = 0;n < pTmpBuf.size();n++)
			{
				// 当前点
				const PointXYZIPRGBA& pt = *(pTmpBuf._Myfirst + n);

				// 获取当前点全局坐标
				hlsReader->GetCoordinate(pt.x,pt.y,pt.z,x,y,z);

				// 设置为模型数据
				S3DVertex2TCoords& TinPcd = m_pts[count];
				TinPcd.Pos.X = (float)x;
				TinPcd.Pos.Y = (float)y;
				TinPcd.Pos.Z = (float)z;

				// 构建tin
				tin.add((float*)&TinPcd.Pos.X,count);

				// 模型数据计数器
				count++;

				// 按间隔递增圈数
				i += loop;

				// 进度条提示
				if ((count % 10000) == 0 && loadCallback)
				{
					loadCallback((F32)count/npoints,HDSCENE_IDS_TODEM_CREATING_TIN);
				}	
			}
		}

		// 构建TIN完成
		tin.finish();

		// 计算法向量
		CalculateNormal(loadCallback);

		// 加载完成
		if (loadCallback)
		{
			loadCallback(0,HDSCENE_IDS_FINISH);
		}

		// 释放文件阅读指针
		if (hlsReader)
		{
			delete hlsReader;
			hlsReader = NULL;
		}

		return count;
	}
	//// 计算垂足
	//void CHdTINPointCloud::CalcuPoint2Line(irr::core::vector3dd& vec, int id0, int id1, int id2)
	//{
	//	// id0到线id1，id2的距离
	//	double a,b,c;
	//	double A,B,C;

	//	a = m_pts[id2].Pos.X - m_pts[id1].Pos.X;
	//	b = m_pts[id2].Pos.Y - m_pts[id1].Pos.Y;
	//	c = m_pts[id2].Pos.Z - m_pts[id1].Pos.Z;

	//	A = a * m_pts[id0].Pos.X + b * m_pts[id0].Pos.Y + c * m_pts[id0].Pos.Z;
	//	B = b * m_pts[id1].Pos.X - a * m_pts[id1].Pos.Y;
	//	C = c * m_pts[id1].Pos.X - a * m_pts[id1].Pos.Z;

	//	if (a != 0)
	//	{
	//		vec.X = (A * a + B +b + C * c) / (a * a + b * b + c * c);
	//		vec.Y = (b * vec.X - B) / a;
	//		vec.Z = (c * vec.X - C) / a;
	//	}
	//	else
	//	{
	//		double D,temp;
	//		D = c * m_pts[id1].Pos.Y - b * m_pts[id1].Pos.Z;
	//		temp = b * b + c * c;

	//		vec.Y = (A * a + D * c) / temp;
	//		vec.Z = (A * c - D * b) / temp;
	//		vec.X = (B + a * vec.Y) / b;
	//	}
	//}

	//// 对于dem到tin的数据，通过包围盒过滤狭长的三角形
	//bool CHdTINPointCloud::CalculateTriBox(irr::core::vector3df id0, irr::core::vector3df id1, irr::core::vector3df id2)
	//{
	//	// 判断参数的合法性
	//	if (m_pts.size() <= 0 || id0 >= m_pts.size() || id1 >= m_pts.size() || id2 >= m_pts.size())
	//	{
	//		return false;
	//	}
	//	/*
	//	1 对于hls-tin，过滤掉一定边长的三角形
	//	2 对于tif-tin，通过外包围盒的x与y方向的间隔值较大的三角形
	//	*/
	//	// 如果是hls转为obj
	//	if (!m_bisTif2Obj)
	//	{
	//		// 三角形的三个顶点
	//		//core::vector3df point0(m_pts[id0].Pos.X, m_pts[id0].Pos.Y,m_pts[id0].Pos.Z);
	//		//core::vector3df point1(m_pts[id1].Pos.X, m_pts[id1].Pos.Y,m_pts[id1].Pos.Z);
	//		//core::vector3df point2(m_pts[id2].Pos.X, m_pts[id2].Pos.Y,m_pts[id2].Pos.Z);

	//		// 计算三条边长，
	//		float dis01 = id0.getDistanceFrom(id1);
	//		float dis12 = id1.getDistanceFrom(id2);
	//		float dis02 = id0.getDistanceFrom(id2);

	//		// 属于数据量较大的数据的阈值，测试隧道数据时将距离设置的比较小。
	//		//if(m_pts.size() > 1000000)
	//		//{
	//		//	// 边长大于0.5则删除该三角形
	//		//	if (dis01 > 0.5 || dis12 > 0.5 || dis02 > 0.5)
	//		//	{
	//		//		return false;
	//		//	}
	//		//}
	//		//else  
	//		//{
	//			// 边长大于5的三角形
	//			if (dis01 > m_MaxTriaSIzeLen || dis12 > m_MaxTriaSIzeLen || dis02 > m_MaxTriaSIzeLen)
	//			{
	//				return false;
	//			}
	//		//}

	//	}
	//	// tif-obj
	//	else
	//	{
	//		// 统计当前三角形的外包围盒
	//		core::aabbox3df	box(m_pts[id0].Pos.X, m_pts[id0].Pos.Y,m_pts[id0].Pos.Z,m_pts[id0].Pos.X, m_pts[id0].Pos.Y,m_pts[id0].Pos.Z);					// 包围盒
	//		box.addInternalPoint(m_pts[id0].Pos.X, m_pts[id0].Pos.Y,m_pts[id0].Pos.Z);
	//		box.addInternalPoint(m_pts[id1].Pos.X, m_pts[id1].Pos.Y,m_pts[id1].Pos.Z);
	//		box.addInternalPoint(m_pts[id2].Pos.X, m_pts[id2].Pos.Y,m_pts[id2].Pos.Z);

	//		// 计算外包围盒的边长
	//		double dx = box.MaxEdge.X - box.MinEdge.X;
	//		double dy = box.MaxEdge.Y - box.MinEdge.Y;

	//		// 过滤掉边长大于2m的三角形
	//		if (dx > m_MaxGridSize || dy > m_MaxGridSize)
	//		{
	//			return false;
	//		}
	//	}

	//	return true;


	//	//// 计算垂足
	//	//irr::core::vector3df vec;
	//	//core::line3df line(m_pts[id1].Pos, m_pts[id2].Pos);
	//	//line.getFOPFromPointP(m_pts[id0].Pos, vec);

	//	//float fDist = 0.0f;
	//	//float fHorDist = 0.0f;
	//	//float fSlopeAngle = 0.0f;

	//	//// 计算in0垂线的长度
	//	//fDist = fabs(m_pts[id0].Pos.Z - vec.Z);
	//	//fHorDist = sqrt(pow(m_pts[id0].Pos.X - vec.X, 2)
	//	//	+ pow(m_pts[id0].Pos.Y - vec.Y, 2) + pow(m_pts[id0].Pos.Z - vec.Z, 2));


	//	//if (fHorDist != 0.0f)
	//	//{
	//	//	fSlopeAngle = atan2(fDist,fHorDist);
	//	//}
	//	//else
	//	//{
	//	//	fSlopeAngle = 0.0f;
	//	//}

	//	//// 将弧度转化为度
	//	//fSlopeAngle = fSlopeAngle * 180.0 / 3.14159265359f;
	//	//fSlopeAngle = fabs(fSlopeAngle);
	//	//if (180.0 - fSlopeAngle - 15.0 < 0.000000001)
	//	//{
	//	//	// 若当前角度小于15°，删除该三角形
	//	//	return false;
	//	//}

	//	/////////////////////////////////////////////////////////
	//	//core::line3df line1(m_pts[id2].Pos, m_pts[id0].Pos);
	//	//line1.getFOPFromPointP(m_pts[id1].Pos, vec);

	//	//// 计算in0垂线的长度
	//	//fDist = fabs(m_pts[id1].Pos.Z - vec.Z);
	//	//fHorDist = sqrt(pow(m_pts[id1].Pos.X - vec.X, 2)
	//	//	+ pow(m_pts[id1].Pos.Y - vec.Y, 2));


	//	//if (fHorDist != 0.0f)
	//	//{
	//	//	fSlopeAngle = atan2(fDist,fHorDist);
	//	//}
	//	//else
	//	//{
	//	//	fSlopeAngle = 0.0f;
	//	//}

	//	//// 将弧度转化为度
	//	//fSlopeAngle = fSlopeAngle * 180.0 / 3.14159265359f;
	//	//fSlopeAngle = fabs(fSlopeAngle);
	//	//if (180.0 - fSlopeAngle - 15.0 < 0.000000001)
	//	//{
	//	//	// 若当前角度小于15°，删除该三角形
	//	//	return false;
	//	//}
	//	//	
	//	///////////////////////////////////////////////////////////
	//	//core::line3df line2(m_pts[id0].Pos, m_pts[id1].Pos);
	//	//line2.getFOPFromPointP(m_pts[id2].Pos, vec);

	//	//// 计算in0垂线的长度
	//	//fDist = fabs(m_pts[id2].Pos.Z - vec.Z);
	//	//fHorDist = sqrt(pow(m_pts[id2].Pos.X - vec.X, 2)
	//	//	+ pow(m_pts[id2].Pos.Y - vec.Y, 2));


	//	//if (fHorDist != 0.0f)
	//	//{
	//	//	fSlopeAngle = atan2(fDist,fHorDist);
	//	//}
	//	//else
	//	//{
	//	//	fSlopeAngle = 0.0f;
	//	//}

	//	//// 将弧度转化为度
	//	//fSlopeAngle = fSlopeAngle * 180.0 / 3.14159265359f;
	//	//fSlopeAngle = fabs(fSlopeAngle);
	//	//if (180.0 - fSlopeAngle - 15.0 < 0.000000001)
	//	//{
	//	//	// 若当前角度小于15°，删除该三角形
	//	//	return false;
	//	//}

	//	return true;
	//}

	// 计算法向量
	void CHdTINPointCloud::CalculateNormal(void (*loadCallback)(float,const char*)/* = NULL*/)
	{
		// 获得顶点平面范围
		float fMinX,fMinY,fMaxX,fMaxY;
		fMinX = fMinY = F32_MAX;
		fMaxX = fMaxY = F32_MIN;

		// 统计平面范围
		StatCoordInLoadFile(fMinX,fMaxX,fMinY,fMaxY);

		// 计算步长
		float fTmp = MIN(fMaxX - fMinX,fMaxY - fMinY);
		float fStep = fTmp / 4.0f;

		// 定义三角形顶点的中间变量行列row，col
		int row_a,row_b,row_c,col_a,col_b,col_c;

		// 获取三角形指针
		TINtriangle* t = TINget_triangle(0);

		// 获取三角形的个数
		int size =  TINget_size();		

		// 计算法向量所需中间变量
		irr::core::vector3df vec_a, vec_b, vec_c,vec_t;

		// 索引编号
		int idx = 0;

		// 遍历三角形，统计法向量
		for (int i = 0; i < size; i++, t++)
		{
			if (t->next < 0)
			{
				if (t->V[0])
				{
					vec_a.set(t->V[0][0],t->V[0][1],t->V[0][2]);
					vec_b.set(t->V[1][0],t->V[1][1],t->V[1][2]);
					vec_c.set(t->V[2][0],t->V[2][1],t->V[2][2]);

					vec_t = (vec_b - vec_a).crossProduct(vec_c - vec_a);
					vec_t.normalize();

					// 获取三角形三个顶点的索引值
					u32 index1 = t->index[0];
					u32 index2 = t->index[1];
					u32 index3 = t->index[2];

					// 计算三条边长，过滤大于设定值的边长
					float dis01 = vec_a.getDistanceFrom(vec_b);
					float dis12 = vec_b.getDistanceFrom(vec_c);
					float dis02 = vec_a.getDistanceFrom(vec_c);

					if (dis01 <= m_MaxTriaSIzeLen && dis02 <= m_MaxTriaSIzeLen && dis12 <= m_MaxTriaSIzeLen)
					{
						// 计算顶点对应格网中行列
						col_a = vec_a.X / fStep;
						row_a = vec_a.Y / fStep;

						col_b = vec_b.X / fStep;
						row_b = vec_b.Y / fStep;

						col_c = vec_c.X / fStep;
						row_c = vec_c.Y / fStep;

						// 三角形顶点必须为相邻格网
						if (abs(col_a - col_b) <= 1 && abs(col_a - col_c) <= 1 && abs(col_b - col_c) <= 1
							&& abs(row_a - row_b) <= 1 && abs(row_a - row_c) <= 1 && abs(row_b - row_c) <= 1)
						{
							// 添加顶点索引值到meshbuffer
							m_TrianlgeIndex.push_back(index1);
							m_TrianlgeIndex.push_back(index2);
							m_TrianlgeIndex.push_back(index3);

							// 更新索引
							m_nIndicesToRender += 3;
						}
					}	

					// 设定顶点的法向量
					video::S3DVertex2TCoords& vertex1= m_pts[index1];
					vertex1.Normal += vec_t;
					video::S3DVertex2TCoords& vertex2= m_pts[index2];
					vertex2.Normal += vec_t;
					video::S3DVertex2TCoords& vertex3= m_pts[index3];
					vertex3.Normal += vec_t;

					// 设置进度条
					if ((i % 10000) == 0 && loadCallback)
					{
						loadCallback((F32)i/size,HDSCENE_IDS_LOADDATA_CALCULATING_NORMAL);
					}
				}
			}
		}

		// 归一化每个顶点的法向量
		for (unsigned int i = 0; i < m_pts.size();i++)
		{
			video::S3DVertex2TCoords& vertex= m_pts[i];
			vertex.Normal.normalize();
		}

		// 构建完法向量后删除三角形
		TINdestroy();
	}

	// 将las点云文件一次性加载到内存
	int CHdTINPointCloud::loadLasFile(const char* lasFile, void (*loadCallback)(float,const char*)/* = NULL*/)
	{
		return 1;
	}

	// 将asc文件加载到内存
	BOOL CHdTINPointCloud::loadAscFile(const char* ascFile, void (*loadCallback)(float,const char*))
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

		int nCols = 0;						// 列号
		int nRows = 0;						// 行号
		float xMin = 0.0f;					// x最小值
		float yMin = 0.0f;					// y最小值
		float cellSize = 0.0f;				// 格网大小
		float noData = -9999.0f;			// 无效值

		// 依次从文件中读取对应值
		fscanf(pFile,"%*s%d\n", &nCols);
		fscanf(pFile,"%*s%d\n", &nRows);
		fscanf(pFile,"%*s%f\n", &xMin);
		fscanf(pFile,"%*s%f\n", &yMin);
		fscanf(pFile,"%*s%f\n", &cellSize);
		fscanf(pFile,"%*s%f\n", &noData);

		// 遍历文件，转换为模型的顶点
		float buff = 0.0f;
		for (int i = 0;i < nRows ;i++)
		{
			for (int j = 0;j < nCols;j++)
			{
				fscanf(pFile,"%f \n", &buff);

				// 过滤无效点
				if (buff == noData)
				{
					continue;
				}

				S3DVertex2TCoords TinPcd; 
				TinPcd.Pos.X = (float)(xMin + j * cellSize);
				TinPcd.Pos.Y = (float)(yMin + i * cellSize);
				TinPcd.Pos.Z = buff;

				// 捕捉异常
				try
				{
					m_pts.push_back(TinPcd);
				}
				catch(...)
				{
					m_pts.clear();
					// 分配失败
					::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);
					return FALSE;
				}	
			}

			// 设置进度条
			if (loadCallback)
			{
				loadCallback((float)(i)/nRows,HDSCENE_IDS_PROCESS_READING);
			}
		}

		// 构建tin
		BuildTriangleMesh();

		// 设置进度条
		if (loadCallback)
		{
			loadCallback(0.0,HDSCENE_IDS_FINISH);
		}

		return TRUE;	
	}

	// 将tif文件加载进内存
	BOOL CHdTINPointCloud::loadTifFile(const char* tifFile, void (*loadCallback)(float,const char*)/* = NULL*/)
	{
		// 输入文件路径
		m_filePath = tifFile;  

		// 判断合法性
		if (m_filePath == "")
		{
			if (loadCallback)
			{
				loadCallback(1.0,HDSCENE_IDS_TOTIN_OPENTIF_FAILED);
			}
			return FALSE;
		}

		int nCols = 0;						// 列号
		int nRows = 0;						// 行号
		float xMin = 0.0f;					// x最小值
		float yMin = 0.0f;					// y最小值
		float cellSize = 0.0f;				// 格网大小
		float noData = -9999.0f;			// 无效值

		// 使用GDAL读取tif文件
		CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");
		GDALDataset *poDataset;

		// 打开
		poDataset = (GDALDataset*)GDALOpen( m_filePath.c_str(), GA_ReadOnly );

		// 判空
		if (poDataset == NULL)
		{
			// 弹出相应提示
			::MessageBox(NULL,HDSCENE_IDS_TOTIN_OPENTIF_FAILED,HDSCENE_IDS_PROMPT,MB_OK);
			return -1;
		}

		// 获得坐标系,与arcgis相同，写左手系坐标,x、y坐标,左上角为xMin、yMin对应0列0行
		double adfGeoTransform[6] = 
		{xMin,		// 左上角 x
		cellSize,	// x方向一个像素代表地理范围
		0,			// 旋转,0代表上面为正北
		yMin,		// 左上角 y
		0,			// 旋转,0代表上面为正北
		cellSize};	// y方向一个像素代表地理范围

		// 获取坐标系
		GDALGetGeoTransform(poDataset,adfGeoTransform);

		// 获取行列号
		nCols = poDataset->GetRasterXSize();
		nRows = poDataset->GetRasterYSize();

		int i = 0;			// 行号计数器
		int j = 0;			// 列号计数器

		// 初始化范围
		double xminD = F64_MAX;
		double yminD = F64_MAX;
		double zminD = F64_MAX;
		double xmaxD = F64_MIN;
		double ymaxD = F64_MIN;
		double zmaxD = F64_MIN;
		bool isFirstPoint = true;

		// 从文件读取一行数据
		float* pPixelBuf = new float[nCols];
		memset(pPixelBuf,0,sizeof(float) * nCols);

		// 一行一行的进行读取
		for (i = 0;i < nRows;i++)
		{
			GDALDatasetRasterIO(poDataset,GF_Read,0,i,nCols,1,pPixelBuf,
				nCols,1,GDT_Float32,1,0,0,0,0);

			// 对每个点进行计算
			for (j = 0;j < nCols;j++)
			{
				// 过滤无效点
				if (pPixelBuf[j] <= noData)
				{
					continue;
				}
				// 将计算后的点存进模型数据
				else
				{
					S3DVertex2TCoords TinPcd; 
					TinPcd.Pos.X = (float)(adfGeoTransform[0] + j * adfGeoTransform[1]);
					TinPcd.Pos.Y = (float)(adfGeoTransform[3] + i * adfGeoTransform[5]);
					TinPcd.Pos.Z = (float)(pPixelBuf[j]);

					// 以第一个点作为偏移量
					if (isFirstPoint)
					{
						m_dx = TinPcd.Pos.X;
						m_dy = TinPcd.Pos.Y;
						m_dz = TinPcd.Pos.Z;

						// 偏移量记录在transmodel中
						m_transModel.m_fOffset[0] = m_dx;
						m_transModel.m_fOffset[1] = m_dy;
						m_transModel.m_fOffset[2] = m_dz;

						// 重新计算矩阵
						m_transModel.Parameter2matrix();

						isFirstPoint = false;
					}

					// 对于范围比较大的点做一个偏移
					TinPcd.Pos.X -= (float)m_dx;
					TinPcd.Pos.Y -= (float)m_dy;
					TinPcd.Pos.Z -= (float)m_dz;

					// 统计包围盒
					if (xmaxD <= TinPcd.Pos.X)
					{
						xmaxD = TinPcd.Pos.X;
					}
					if (xminD > TinPcd.Pos.X)
					{
						xminD = TinPcd.Pos.X;
					}
					if (ymaxD <= TinPcd.Pos.Y)
					{
						ymaxD = TinPcd.Pos.Y;
					}
					if (yminD > TinPcd.Pos.Y)
					{
						yminD = TinPcd.Pos.Y;
					}
					if (zmaxD <= TinPcd.Pos.Z)
					{
						zmaxD = TinPcd.Pos.Z;
					}
					if (zminD > TinPcd.Pos.Z)
					{
						zminD = TinPcd.Pos.Z;
					}

					// 捕捉异常
					try
					{
						m_pts.push_back(TinPcd);
					}
					catch(...)
					{
						m_pts.clear();
						// 分配失败
						::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);
						return FALSE;
					}
				}
			}

			// 设置包围盒
			m_box.MaxEdge.X = (float)xmaxD;
			m_box.MaxEdge.Y = (float)ymaxD;
			m_box.MaxEdge.Z = (float)zmaxD;
			m_box.MinEdge.X = (float)xminD;
			m_box.MinEdge.Y = (float)yminD;
			m_box.MinEdge.Z = (float)zminD;

			// 设置进度条
			if ((i % 10) == 0 && loadCallback)
			{
				loadCallback((F32)i/nRows,HDSCENE_IDS_PROCESS_READING);
			}
		}

		// 删除缓存
		if (pPixelBuf)
		{
			delete []pPixelBuf;
			pPixelBuf = NULL;
		}

		// 退出GDAL
		GDALClose( poDataset );

		// 构建tin
		BuildTriangleMesh();

		// 设置进度条
		if (loadCallback)
		{
			loadCallback(0,HDSCENE_IDS_FINISH);
		}
		return TRUE;
	}

	// 将dat文件加载进内存
	BOOL CHdTINPointCloud::loadDatFile(const char* datFile, void (*loadCallback)(float,const char*)/* = NULL*/)
	{
		m_filePath = datFile; 

		// 判断合法性
		if (m_filePath == "")
		{
			if (loadCallback)
			{
				loadCallback(1.0,HDSCENE_IDS_TOTIN_OPENTIF_FAILED);
			}
			return FALSE;
		}

		// 读取数据并解析数据
		FILE* fp = fopen(datFile,"r");
		char buffer[256];
		memset(buffer,0,256);
		fseek(fp, 0, SEEK_SET);
		vector<float> pPointList;
		while (!feof(fp))
		{
			fgets(buffer,256,fp);

			const char *d = ",";
			char *p ;
			p = strtok(buffer,d);
			while(p)
			{
				pPointList.push_back(atof(p));
				p = strtok(NULL,d);
			}
		}
		fclose(fp);

		// 初始化范围
		double xminD = F64_MAX;
		double yminD = F64_MAX;
		double zminD = F64_MAX;
		double xmaxD = F64_MIN;
		double ymaxD = F64_MIN;
		double zmaxD = F64_MIN;
		bool isFirstPoint = true;

		// 将计算后的点存进模型数据
		S3DVertex2TCoords TinPcd; 
		for (int i = 0; i != pPointList.size() / 4; ++i)
		{
			TinPcd.Pos.X = pPointList[4 * i + 1];
			TinPcd.Pos.Y = pPointList[4 * i + 2];
			TinPcd.Pos.Z = pPointList[4 * i + 3];

			// 以第一个点作为偏移量
			if (isFirstPoint)
			{
				m_dx = TinPcd.Pos.X;
				m_dy = TinPcd.Pos.Y;
				m_dz = TinPcd.Pos.Z;

				// 偏移量记录在transmodel中
				m_transModel.m_fOffset[0] = m_dx;
				m_transModel.m_fOffset[1] = m_dy;
				m_transModel.m_fOffset[2] = m_dz;

				// 重新计算矩阵
				m_transModel.Parameter2matrix();

				isFirstPoint = false;
			}

			// 对于范围比较大的点做一个偏移
			TinPcd.Pos.X -= (float)m_dx;
			TinPcd.Pos.Y -= (float)m_dy;
			TinPcd.Pos.Z -= (float)m_dz;

			// 统计包围盒
			if (xmaxD <= TinPcd.Pos.X)
			{
				xmaxD = TinPcd.Pos.X;
			}
			if (xminD > TinPcd.Pos.X)
			{
				xminD = TinPcd.Pos.X;
			}
			if (ymaxD <= TinPcd.Pos.Y)
			{
				ymaxD = TinPcd.Pos.Y;
			}
			if (yminD > TinPcd.Pos.Y)
			{
				yminD = TinPcd.Pos.Y;
			}
			if (zmaxD <= TinPcd.Pos.Z)
			{
				zmaxD = TinPcd.Pos.Z;
			}
			if (zminD > TinPcd.Pos.Z)
			{
				zminD = TinPcd.Pos.Z;
			}

			// 捕捉异常
			try
			{
				m_pts.push_back(TinPcd);
			}
			catch(...)
			{
				m_pts.clear();
				return FALSE;
			}
		}

		// 设置包围盒
		m_box.MaxEdge.X = (float)xmaxD;
		m_box.MaxEdge.Y = (float)ymaxD;
		m_box.MaxEdge.Z = (float)zmaxD;
		m_box.MinEdge.X = (float)xminD;
		m_box.MinEdge.Y = (float)yminD;
		m_box.MinEdge.Z = (float)zminD;

		// 构建tin
		BuildTriangleMesh();

		// 设置进度条
		if (loadCallback)
		{
			loadCallback(0,HDSCENE_IDS_FINISH);
		}

		return TRUE;
	}

	// 将obj文件加载进内存
	BOOL CHdTINPointCloud::loadObjFile(const char* tifFile, void (*loadCallback)(float,const char*))
	{
		// 文件指针
		m_filePath = tifFile;

		FILE* fp = fopen(tifFile, "r");

		// 判空
		if (!fp)
		{
			return FALSE;
		}

		// 进度条开始
		if (loadCallback)
		{
			loadCallback(0.0f,HDSCENE_IDS_PROCESS_READING);
		}

		int cv = 0;				// 点数
		int cn = 0;				// 法向量
		int cf = 0;				// 三角形数
		fseek(fp, 0, SEEK_SET);	// 将文件指针放在初始位置

		char c;					// 中间缓存
		// 读文件
		while (!feof(fp))
		{
			// 获取首字符
			c = fgetc(fp);

			// #开头，为hdScene导出文件时直接获取点数与三角形数量
			if (c == '#')
			{
				c = getc(fp);
				if (c == 'v')
				{
					// 获取点数
					fscanf(fp," %d", &cv);
					cn = cv;
				}
				else if (c == 'f')
				{
					// 获取三角形数量
					fscanf(fp," %d", &cf);
				}
				else
				{
					// 读取前缀
					while ((c = getc(fp)) != '\n');
				}

				// 如果为hdscene导出obj，已经记录了点数与三角形数
				if (cv != 0 && cf != 0)
				{
					break;
				}
			}

			// v开头，对于常规obj文件需要统计点数与三角形数量
			else if (c == 'v')
			{
				// 获取点数
				c = getc(fp);
				if (c == ' ')
				{
					// 统计点数
					cv++;
				}
				else if (c == 'n')	
				{
					// 统计法向量数量
					cn++;
				}	
			}
			else if (c == 'f')
			{
				// 获取三角形数
				cf++;
			}
		}

		// 分配内存
		try
		{
			m_pts.resize(cv);
			m_TrianlgeIndex.resize(cf * 3);
			m_nIndicesToRender = cf * 3;
		}
		catch(...)
		{
			// 分配失败
			::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);
			return FALSE;
		}

		// 移动文件指针到开始部位
		fseek(fp, 0, SEEK_SET);

		// 初始化范围
		double xminD = F64_MAX;
		double yminD = F64_MAX;
		double zminD = F64_MAX;
		double xmaxD = F64_MIN;
		double ymaxD = F64_MIN;
		double zmaxD = F64_MIN;


		bool isFirstPoint = true;	// 是否更新偏移量
		int i = 0;					// 点计数器
		int k = 0;					// 法向量计数
		int j = 0;					// 三角形计数器

		// 读文件
		while (!feof(fp))
		{
			// 获取首字符
			c = fgetc(fp);

			// #开头，跳过
			if (c == '#')
			{
				// 读取前缀
				while ((c = getc(fp)) != '\n');
			}

			// v字开头，将读取的点存进模型数据
			else if (c == 'v')
			{
				// 读取点
				c = fgetc(fp);

				if (c == ' ')
				{
					// 三维点
					S3DVertex2TCoords& TinPcd = m_pts[i];
					fscanf(fp,"%f %f %f", &TinPcd.Pos.X, &TinPcd.Pos.Y, &TinPcd.Pos.Z);

					// 以第一个点作为偏移量
					if (isFirstPoint)
					{
						m_dx = TinPcd.Pos.X;
						m_dy = TinPcd.Pos.Y;
						m_dz = TinPcd.Pos.Z;

						// 偏移量记录在transmodel中
						m_transModel.m_fOffset[0] = m_dx;
						m_transModel.m_fOffset[1] = m_dy;
						m_transModel.m_fOffset[2] = m_dz;
						
						// 重新计算矩阵
						m_transModel.Parameter2matrix();

						isFirstPoint = false;
					}

					// 对于范围比较大的点做一个偏移
					TinPcd.Pos.X -= (float)m_dx;
					TinPcd.Pos.Y -= (float)m_dy;
					TinPcd.Pos.Z -= (float)m_dz;

					i++;

					// 统计包围盒
					if (xmaxD <= TinPcd.Pos.X)
					{
						xmaxD = TinPcd.Pos.X;
					}
					if (xminD > TinPcd.Pos.X)
					{
						xminD = TinPcd.Pos.X;
					}
					if (ymaxD <= TinPcd.Pos.Y)
					{
						ymaxD = TinPcd.Pos.Y;
					}
					if (yminD > TinPcd.Pos.Y)
					{
						yminD = TinPcd.Pos.Y;
					}
					if (zmaxD <= TinPcd.Pos.Z)
					{
						zmaxD = TinPcd.Pos.Z;
					}
					if (zminD > TinPcd.Pos.Z)
					{
						zminD = TinPcd.Pos.Z;
					}
				}
				else if (c == 'n')
				{
					// 读取法向量
					S3DVertex2TCoords& TinPcd = m_pts[k++];
					fscanf(fp," %f %f %f", &TinPcd.Normal.X, &TinPcd.Normal.Y, &TinPcd.Normal.Z);
				}

				// 设置进度条
				if ((i % 10000) == 0 && loadCallback)
				{
					loadCallback((F32)i/cv,HDSCENE_IDS_TODEM_CREATING_TIN);
				}
			}

			// f开头，读进三角形
			else if (c == 'f')
			{
				// 文件中的三角形索引从1开始，geomagic等软件都是如此，但是内存中数据仍然是从0开始
				// 读取三角形
				u32 index = 0;
				fscanf(fp," %u", &index);
				u32& TrianlgeIndex0 = m_TrianlgeIndex[j];

				// 防止添加以前生成的数据，对于索引0自减后为负，导致软件崩溃
				if (index == 0)
				{
					TrianlgeIndex0 = 0;
				}
				else
				{
					TrianlgeIndex0 = index - 1;
				}

				fscanf(fp," %u", &index);
				u32& TrianlgeIndex1 = m_TrianlgeIndex[j + 1];

				// 防止添加以前生成的数据，对于索引0自减后为负，导致软件崩溃
				if (index == 0)
				{
					TrianlgeIndex1 = 0;
				}
				else
				{
					TrianlgeIndex1 = index - 1;
				}

				fscanf(fp," %u", &index);
				u32& TrianlgeIndex2 = m_TrianlgeIndex[j + 2];

				// 防止添加以前生成的数据，对于索引0自减后为负，导致软件崩溃
				if (index == 0)
				{
					TrianlgeIndex2 = 0;
				}
				else
				{
					TrianlgeIndex2 = index - 1;
				}

				j += 3;

				// 设置进度条
				if ((j % 10000) == 0 && loadCallback)
				{
					loadCallback((F32)j/cf * 3,HDSCENE_IDS_TODEM_CREATING_TIN);
				}	
			}
		}

		// 设置包围盒
		m_box.MaxEdge.X = (float)xmaxD;
		m_box.MaxEdge.Y = (float)ymaxD;
		m_box.MaxEdge.Z = (float)zmaxD;
		m_box.MinEdge.X = (float)xminD;
		m_box.MinEdge.Y = (float)yminD;
		m_box.MinEdge.Z = (float)zminD;

		// 判断是否需要构建TIN
		if (cn == 0 || cf == 0)
		{
			// 构建三角形
			BuildTriangleMesh();
		}

		// 设置进度条
		if (loadCallback)
		{
			loadCallback(0,HDSCENE_IDS_TODEM_CREATING_TIN);
		}

		// 关闭文件指针
		fclose(fp);

		// 给三角形索引创建索引
		CreateQuadIndex();

		if (loadCallback)
		{
			loadCallback(0,HDSCENE_IDS_FINISH);
		}

		return TRUE;
	}

	// 将模型写进obj
	void CHdTINPointCloud::WriteObjFile(const char* objFile, void (*loadCallback)(float,const char*))
	{
		// 文件指针
		FILE* fp = fopen(objFile, "w");

		// 判空
		if (!fp)
		{
			return;
		}

		// 文件前缀
		fputs("#Wavefront OBJ File", fp);
		fputs("\n", fp);
		fputs("#convert by HDSY HDScene v1.0", fp);
		fputs("\n", fp);

		// 写入点个数
		if (m_pts.size() > 0)
		{
			fprintf(fp,"#v %d", m_pts.size());
			fputs("\n", fp);
		}

		// 写入三角形个数
		if (m_nIndicesToRender > 0)
		{
			fprintf(fp,"#f %d", m_nIndicesToRender / 3);
			fputs("\n", fp);
		}

		// 写入三角形的顶点
		int i = 0;							// 三角形索引
		int j = 0;							// 三角形三个顶点索引
		int ptCount = 0;					// 点数

		// 遍历模型数据将点写入obj
		for (i = 0; i < m_pts.size(); i++)
		{	
			// 将相对坐标转为全局坐标
			m_pts[i].Pos.X += (float)m_dx;
			m_pts[i].Pos.Y += (float)m_dy;
			m_pts[i].Pos.Z += (float)m_dz;

			fprintf(fp,"v %.6f %.6f %.6f \n", m_pts[i].Pos.X, m_pts[i].Pos.Y, m_pts[i].Pos.Z);
			ptCount++;

			if ((i % 10000) == 0 && loadCallback)
			{
				loadCallback((F32)i/m_pts.size(),HDSCENE_IDS_PROCESS_WRITING);
			}
		}

		// 写入点个数
		fprintf(fp,"# %d vertices\n", ptCount);
		fputs("\n", fp);

		// 写入顶点的法向量
		ptCount = 0;

		// 遍历模型数据将法向量写入obj
		for (i = 0; i < m_pts.size(); i++)
		{	
			fprintf(fp,"vn %f %f %f \n", m_pts[i].Normal.X, m_pts[i].Normal.Y, m_pts[i].Normal.Z);
			ptCount++;

			if ((i % 10000) == 0 && loadCallback)
			{
				loadCallback((F32)i/m_pts.size(),HDSCENE_IDS_PROCESS_WRITING);
			}
		}

		// 写入法向量个数
		fprintf(fp,"# %d normals\n", ptCount);
		fputs("\n", fp);

		// 写入三角形索引
		int idx = 0;

		// 遍历索引数组将三角形写入obj
		for (i = 0; i < m_nIndicesToRender; i += 3)
		{
			fprintf(fp,"f %u %u %u\n",m_TrianlgeIndex[i] + 1, m_TrianlgeIndex[i + 1] + 1, m_TrianlgeIndex[i + 2] + 1);
			idx++;

			if ((i % 10000) == 0 && loadCallback)
			{
				loadCallback((F32)i * 3/m_nIndicesToRender,HDSCENE_IDS_PROCESS_WRITING);
			}
		}

		// 三角形个数
		fprintf(fp,"# %d triangles", idx);
		fputs("\n", fp);

		// 设置进度条
		if (loadCallback)
		{
			loadCallback(0.0," ");
		}

		// 关闭文件指针
		fclose(fp);
	}

	// 加载vtk文件
	BOOL CHdTINPointCloud::LoadVtkFile(const char* vtkFile, void (*loadCallback)(float,const char*))
	{
		// 文件指针
		FILE* fp = fopen(vtkFile, "r");

		// 判空
		if (!fp)
		{
			return FALSE;
		}

		CVtkReader vtkReader;
		if (!vtkReader.open(fp))
		{
			return FALSE;
		}

		// 初始化范围
		double xminD = F64_MAX;
		double yminD = F64_MAX;
		double zminD = F64_MAX;
		double xmaxD = F64_MIN;
		double ymaxD = F64_MIN;
		double zmaxD = F64_MIN;

		int nverts = vtkReader.GetVerticesCounts();

		int nfaces = vtkReader.GetFaceCounst();

		// 分配内存
		try
		{
			m_pts.resize(nverts);
		}
		catch(...)
		{
			// 分配失败
			::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);
			return FALSE;
		}
		unsigned int i = 0;

		IOEvent eventype = vtkReader.read_event();
		int index = 0;
		bool isFirstPoint = true;
		for (i = 0; i < nverts; i++)
		{
			if (eventype ==  VTKIO_VERTEX)
			{
				// 三维点
				S3DVertex2TCoords& TinPcd = m_pts[i];
				vtkReader.GetVertice(index,TinPcd.Pos.X, TinPcd.Pos.Y, TinPcd.Pos.Z);

				// 以第一个点作为偏移量
				if (isFirstPoint)
				{
					m_dx = TinPcd.Pos.X;
					m_dy = TinPcd.Pos.Y;
					m_dz = TinPcd.Pos.Z;
					isFirstPoint = false;
				}

				// 对于范围比较大的点做一个偏移
				TinPcd.Pos.X -= (float)m_dx;
				TinPcd.Pos.Y -= (float)m_dy;
				TinPcd.Pos.Z -= (float)m_dz;

				// 统计包围盒
				if (xmaxD <= TinPcd.Pos.X)
				{
					xmaxD = TinPcd.Pos.X;
				}
				if (xminD > TinPcd.Pos.X)
				{
					xminD = TinPcd.Pos.X;
				}
				if (ymaxD <= TinPcd.Pos.Y)
				{
					ymaxD = TinPcd.Pos.Y;
				}
				if (yminD > TinPcd.Pos.Y)
				{
					yminD = TinPcd.Pos.Y;
				}
				if (zmaxD <= TinPcd.Pos.Z)
				{
					zmaxD = TinPcd.Pos.Z;
				}
				if (zminD > TinPcd.Pos.Z)
				{
					zminD = TinPcd.Pos.Z;
				}
			}
			eventype = vtkReader.read_event();
		}

		// 设置包围盒
		m_box.MaxEdge.X = (float)xmaxD;
		m_box.MaxEdge.Y = (float)ymaxD;
		m_box.MaxEdge.Z = (float)zmaxD;
		m_box.MinEdge.X = (float)xminD;
		m_box.MinEdge.Y = (float)yminD;
		m_box.MinEdge.Z = (float)zminD;

		//while (eventype ==  VTKIO_TRIANGLE)
		//{
		//	// 三维点
		//	u32 TrianlgeIndex0;
		//	u32 TrianlgeIndex1;
		//	u32 TrianlgeIndex2;
		//	vtkReader.GetFace(TrianlgeIndex0, TrianlgeIndex1, TrianlgeIndex2);

		//	m_TrianlgeIndex.push_back(TrianlgeIndex0);
		//	m_TrianlgeIndex.push_back(TrianlgeIndex1);
		//	m_TrianlgeIndex.push_back(TrianlgeIndex2);

		//	eventype = vtkReader.read_event();
		//}

		//m_nIndicesToRender = m_TrianlgeIndex.size();

		//// 判断是否需要构建TIN
		//if (m_nIndicesToRender == 0)
		//{
			// 构建三角形
			BuildTriangleMesh();
		//}

		// 设置进度条
		if (loadCallback)
		{
			loadCallback(0,HDSCENE_IDS_TODEM_CREATING_TIN);
		}

		// 关闭文件指针
		fclose(fp);

		// 给三角形索引创建索引
		CreateQuadIndex();

		if (loadCallback)
		{
			loadCallback(0,HDSCENE_IDS_FINISH);
		}
	
		return TRUE;
	}

	// 保存vtk文件
	void CHdTINPointCloud::WriteVtkFile(const char* vtkFile, void (*loadCallback)(float,const char*))
	{
		// 文件指针
		FILE* fp = fopen(vtkFile, "w");

		// 判空
		if (!fp)
		{
			return;
		}

		CVtkWriter vtkWriter;
		if (!vtkWriter.open(fp))
		{
			return;
		}

		// 点数
		vtkWriter.set_nverts(m_pts.size());

		// 三角形数
		vtkWriter.set_nfaces(m_TrianlgeIndex.size() / 3);

		
		int i = 0;							// 三角形索引
		int j = 0;							// 三角形三个顶点索引
		int ptCount = 0;					// 点数

		// 写入三角形的顶点
		for (i = 0; i < m_pts.size(); i++)
		{	
			// 将相对坐标转为全局坐标
			m_pts[i].Pos.X += (float)m_dx;
			m_pts[i].Pos.Y += (float)m_dy;
			m_pts[i].Pos.Z += (float)m_dz;
			
			// 将顶点插入vtk
			vtkWriter.write_vertex(&m_pts[i].Pos.X);

			if ((i % 10000) == 0 && loadCallback)
			{
				loadCallback((F32)i/m_pts.size(),HDSCENE_IDS_PROCESS_WRITING);
			}
		}

		// 写入三角形 索引从0开始
		int triagles[3];
		for (i = 0; i < m_nIndicesToRender; i += 3)
		{
			triagles[0] = m_TrianlgeIndex[i];
			triagles[1] = m_TrianlgeIndex[i + 1];
			triagles[2] = m_TrianlgeIndex[i + 2];

			// 将面插入vtk
			vtkWriter.write_triangle(triagles);
			if ((i % 10000) == 0 && loadCallback)
			{
				loadCallback((F32)i * 3/m_nIndicesToRender,HDSCENE_IDS_PROCESS_WRITING);
			}
		}

		vtkWriter.close();
		fclose(fp);
	}

	// 根据三维点云坐标构建三角形 fengjing
	bool CHdTINPointCloud::BuildTriangleMesh()
	{
		// 调用TIN的生成接口
		CHdTin tin;	
		// 点云不为空且未构建三角形
		if (m_pts.size() > 0 && tin.get_size() == 0)
		{
			// 初始化tin
			bool ret = tin.initial(m_pts.size());

			// 初始化失败
			if (!ret)
			{
				// 分配失败
				::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);
				return false;
			}
		}
		else
		{
			// 分配失败
			::MessageBox(NULL, HDSCENE_IDS_VISUAL_SETTING_BUILDTIN_FAILED,HDSCENE_IDS_PROMPT,MB_OK);
			return false;
		}

		// 遍历点坐标构建tin
		for (int i = 0; i < m_pts.size(); i++)
		{
			// 构建tin
			tin.add((float*)&m_pts[i].Pos.X, i);
		}

		// 完成构建TIN
		tin.finish();

		// 计算法向量法向量
		CalculateNormal();

		return 1;
	}

	void CHdTINPointCloud::StatCoordInLoadFile( float& fMinX,float& fMaxX,float& fMinY,float& fMaxY )
	{
		// 获取三角形指针
		TINtriangle* t = TINget_triangle(0);

		// 获取三角形的个数
		int size =  TINget_size();		

		// 定义中间变量
		irr::core::vector3df vec_a;

		// 遍历三角形，统计法向量
		for (int i = 0; i < size; i++, t++)
		{
			if (t->next < 0)
			{
				if (t->V[0])
				{
					// 统计所有顶点的平面坐标范围
					for (int j = 0;j < 3;j++)
					{
						fMinX = MIN(fMinX,t->V[j][0]);
						fMinY = MIN(fMinY,t->V[j][1]);

						fMaxX = MAX(fMaxX,t->V[j][0]);
						fMaxY = MAX(fMaxY,t->V[j][1]);
					}
				} // if (t->V[0])
			} // if (t->next < 0)
		} // for (int i = 0; i < size; i++, t++)
	}
}


#include "stdafx.h"
#include "point_cloud.h"
#include "hdPtCloudfilter_p.h"
#include "..\hdHLSlib\HLSReadOpener.h"

#include "..\hdCommon\hdkdtree.hpp"
#include "..\hdCommon\hdLandMarkTransform.h"
//#include "hdSysSetting.h"
#include "..\hdCommon\eigen.h"
#include "..\hdCommon\hdSceneStr.h"

using namespace hdkdtree;

namespace hd
{
	//#define SIMPLE_LOAD_COUNT	5000000

	// 定义三维点云kdtree索引
	typedef KDTreeSingleIndexAdaptor<
		L2_Simple_Adaptor<float, PointCloud >,
		PointCloud,
		3 /* dim */
	> hd_kd_tree_t;

	bool PointCloud::buildIndex(bool bFullIndex)
	{
		try
		{
			bool bNeedBuild = false;
			// 根据不同选择点集，需要重新构建索引 [2013/12/14 危迟]
			if(m_pIndex)
			{
				hd_kd_tree_t* idx = (hd_kd_tree_t*)m_pIndex;

				int msize = idx->size();

				if (m_bChange || m_bEdited || m_fullIndex != bFullIndex || msize != m_selectCount)
				{
					delete m_pIndex;
					m_pIndex = NULL;
				}
			}

			// 如果要根据选中点构建索引 且当前点云没有选中点 则错误 [2013/12/24 危迟]
			if (!bFullIndex && getSelectCount() == 0)
			{
				return false;
			}

			m_fullIndex = bFullIndex;


			if(m_pIndex == NULL)
			{
				m_pIndex = new hd_kd_tree_t(3,*this,KDTreeSingleIndexAdaptorParams(10));
				bNeedBuild = true;
			}
			if(bNeedBuild)
			{
				hd_kd_tree_t* idx = (hd_kd_tree_t*)m_pIndex;
				idx->buildIndex();
			}
			m_bChange = false;
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	//! 加载索引
	void PointCloud::loadIndex(const char* path)
	{
		if (m_pIndex && _access(path,04) == 0)
		{
			hd_kd_tree_t* idx = (hd_kd_tree_t*)m_pIndex;
			FILE* pFile = fopen(path,"rb");
			idx->loadIndex(pFile);
			fclose(pFile);
		}
	}
	//! 保存索引
	void PointCloud::saveIndex(const char* path)
	{
		if(m_pIndex)
		{
			FILE* pFile = fopen(path,"w+b");
			hd_kd_tree_t* idx = (hd_kd_tree_t*)m_pIndex;
			idx->saveIndex(pFile);
			fclose(pFile);
		}
	}

	size_t PointCloud::radiusSearch( const float *query_point,const float radius,std::vector<std::pair<size_t,float>>& IndicesDists, bool sort )
	{
		if(m_pIndex)
		{
			SearchParams searchParams;
			searchParams.sorted = sort;
			hd_kd_tree_t* idx = (hd_kd_tree_t*)m_pIndex;
			return idx->radiusSearch(query_point,radius,IndicesDists,searchParams);
		}
		else 
			return 0;
	}

	void PointCloud::knnSearch( const float *query_point,int neighbour,size_t* ref_index,float* ref_dist_sqr )
	{
		if(m_pIndex)
		{
			hd_kd_tree_t* idx = (hd_kd_tree_t*)m_pIndex;
			idx->knnSearch(query_point,neighbour,ref_index,ref_dist_sqr);
		}
		else 
			return ;
	}


	//! 获取当前读取的点,坐标系是全局坐标
	//void PointCloud::get_point(PointXYZI_D& pt)
	//{
	//	// 获取原始坐标
	//	pt.x = m_hlsReader->m_point.x;
	//	pt.y = m_hlsReader->m_point.y;
	//	pt.z = m_hlsReader->m_point.z;
	//	// 获取反射强度
	//	pt.intensity = m_hlsReader->m_point.intensity;
	//	// 转换全局坐标
	//	if (pt.isValid())
	//	{
	//		double m[16];
	//		m_header.computeMatrix(m);
	//		hdHomogeneousTransformPoint(m,pt.x,pt.y,pt.z);
	//	}
	//}
	//! 获取当前读取的点,坐标系是本地坐标
	//void PointCloud::get_point(PointXYZIPRGBA& pt)
	//{
	//	pt.x = m_hlsReader->m_point.x;
	//	pt.y = m_hlsReader->m_point.y;
	//	pt.z = m_hlsReader->m_point.z;
	//	// 获取反射强度
	//	pt.intensity = m_hlsReader->m_point.intensity;
	//	// 获取最大反射强度
	//	m_maxIntensity = MAX(m_hlsReader->m_point.intensity,m_maxIntensity);
	//	if (m_hlsReader->m_point.intensity != 0)//无效点不参与计算最小反射值
	//	{
	//		m_minIntensity = MIN(m_hlsReader->m_point.intensity,m_minIntensity);
	//	}
	//	if (pt.isValid())
	//	{
	//		m_validCount++;
	//	}
	//	// 获取最大最小距离
	//	float dist = m_hlsReader->m_point.x * m_hlsReader->m_point.x + 
	//		m_hlsReader->m_point.y * m_hlsReader->m_point.y + 
	//		m_hlsReader->m_point.z * m_hlsReader->m_point.z;
	//	m_maxDistance = MAX(dist,m_maxDistance);
	//	m_minDistance = MIN(dist,m_minDistance);

	//	// 获取r,g,b
	//	pt.r = m_hlsReader->m_point.rgb[0];
	//	pt.g = m_hlsReader->m_point.rgb[1];
	//	pt.b = m_hlsReader->m_point.rgb[2];
	//	//pt._unused = m_hlsReader->m_point.rgb[3];
	//	//获取属性
	//	U8 is_last = 0;//(hlsReader.m_point.return_number == hlsReader.m_point.number_of_returns_of_given_pulse ? 128 : 0);
	//	U8 is_first = 0;//(hlsReader.m_point.return_number == 1 ? 64 : 0);			
	//	pt.prop = is_last | is_first | (m_hlsReader->m_point.classification & 63);

	//}

	//! 设置编辑模式
	void PointCloud::setEditMode(s32 mode)
	{
		if (mode != 1 && mode != 0)
		{
			return;
		}
		m_editMode = mode;
	}

	//! 获取有效点个数,在点云未加载之前,获取有效点数 fengjing
	u64 PointCloud::getValidCount(const char* hlsFile) 
	{
		if (m_HlsValidCount == 0)
		{
			if (m_hlsReader == NULL)
			{
				CHLSReadOpener hlsOpener;
				m_hlsReader = hlsOpener.Open(hlsFile);

				if(!m_hlsReader)
				{
					return FALSE;
				}
			}

			// 获取圈索引
			CLoopIndex* pLoop = m_hlsReader->GetLoopIndex();
			if (!pLoop)
			{
				return m_HlsValidCount;
			}

			int loopCount = m_hlsReader->GetLoopCount();
			for (u32 i = 0; i < loopCount; i++)
			{
				m_HlsValidCount += pLoop[i].m_loopIdx.count;
			}
		}
		return m_HlsValidCount;
	}

	//! 加载hls点云
	BOOL PointCloud::loadHlsFile( const char* hlsFile,void (*loadCallback)(float,const char*) )
	{
		if (m_load_simple_mode == E_LOAD_SPACE_SIMPLE)
		{
			return loadSpaceSimple(hlsFile, m_load_simple_dist, loadCallback);
		}
		
		// 最大灰度值序号
		m_maxIntensity = 0;
		m_minIntensity = 99999;
		if (m_hlsReader && strcmp(m_hlsReader->GetFilePath(),hlsFile) != 0)
		{
			delete m_hlsReader;
			m_hlsReader = NULL;
		}
		if (m_hlsReader == NULL)
		{
			CHLSReadOpener hlsOpener;
			m_hlsReader = hlsOpener.Open(hlsFile);
			if(m_hlsReader == NULL)
				return FALSE;
		}
		int loopCount = m_hlsReader->GetLoopCount();

		// 获取索引
		CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex(loadCallback);

		// 判断有效性 [2014/06/07 危迟]
		if (!pLoopIdx)
		{
			return FALSE;
		}
		
		// 获取文件中的有效点数
		getValidCount(hlsFile);

		// 统计强度范围
		for (int i = 0;i<loopCount;i++)
		{
			CLoopIndex& loopIdx = pLoopIdx[i];
			m_maxIntensity = MAX(loopIdx.m_loopIdx.maxIntensity,m_maxIntensity);
			m_minIntensity = MIN(loopIdx.m_loopIdx.minIntensity,m_minIntensity);
		}

		m_pts.clear();	
		m_header = m_hlsReader->m_header;

		s64 toLoadCount = (s64)floor(m_hlsReader->m_header.number_of_point_records * (m_endScale - m_startScale) + 0.5);
		u32 nStartCol = (u32)floor(m_startScale * m_hlsReader->GetLoopCount() + 0.5);
		u32 nEndCol = (u32)floor(m_endScale * m_hlsReader->GetLoopCount() + 0.5);

		//m_simpleLevel = toLoadCount / (m_loadSimple * 2) + 1;
		m_simpleLevel = (u32)sqrt(1.0 * toLoadCount / m_loadSimple) + 1;

		s32 iReadLoop = 0;
		s32 loopIndex = nStartCol;
		u32 loopSimpleCount = 0;
		if (nStartCol == 0 && nEndCol == m_hlsReader->GetLoopCount())
		{
			loopSimpleCount = m_hlsReader->GetLoopCount() / m_simpleLevel;
		}
		else
		{
			loopSimpleCount = (nEndCol - nStartCol) / m_simpleLevel;
		}

		m_simpleHeader = m_header;
		bool bIsNormalPcd = isNormalPointCloud();// && m_simpleLevel == 1
		if (m_editMode == 1)
		{
			// 设置总圈数
			m_pts.setBlockCount(loopSimpleCount);
			m_simpleHeader.number_of_col = loopSimpleCount;
			// 列已抽稀,行暂未抽稀
			if (bIsNormalPcd)
			{
				m_simpleHeader.number_of_row = m_header.number_of_row / m_simpleLevel;
				m_simpleHeader.number_of_point_records = m_simpleHeader.number_of_col * m_simpleHeader.number_of_row;
			}
		}
		else
		{
			// 设置总圈数
			m_pts.setBlockCount(m_hlsReader->GetLoopCount());
		}

		// 标记，当某一圈点云new失败时，直接返回
		bool bLoadLoopSuc = true;

		m_validCount = 0;
		u32 iBlock = 0;
		u32 blkCount = m_pts.blockCount();
		for (u32 i = nStartCol;i<nEndCol && iBlock < blkCount;)
		{
			CLoopIndex& loopIdx = pLoopIdx[i];
			hdVector<PointXYZIPRGBA>& loopBuf = m_pts.getBlock(iBlock);
			BOOL bRet = FALSE;
			if (bIsNormalPcd && m_editMode == 1)
			{
				if(m_hlsReader->ReadLoopFull(loopBuf,i,m_simpleLevel))
				{
					//m_validCount += loopIdx.m_loopIdx.count;
					bRet = TRUE;

					// 以点云头文件记录的偏移量统计计算加载的点云最大最小距离以应用于按点云按距离渲染-zhubo
					// pt点坐标取值为局部坐标，此处应该修改，直接计算局部坐标到零点的距离即可
					for (int n = 0;n < loopBuf.size();n++)
					{
						const PointXYZIPRGBA& pt = *(loopBuf._Myfirst + n);
						if (!pt.isValid())
							continue;
						f32 dist = (pt.x * pt.x + pt.y * pt.y + pt.z * pt.z);
						m_minDistance = MIN(m_minDistance,dist);
						m_maxDistance = MAX(m_maxDistance,dist);

						// 统计加载至内存的总有效点个数
						m_validCount++;
					}
				}
			}
			else
			{
				if(m_hlsReader->ReadLoop(loopBuf,i))
				{
					//m_validCount += loopIdx.m_loopIdx.count;
					bRet = TRUE;

					// 以点云头文件记录的偏移量统计计算加载的点云最大最小距离以应用于按点云按距离渲染-zhubo
					for (int n = 0;n < loopBuf.size();n++)
					{
						const PointXYZIPRGBA& pt = *(loopBuf._Myfirst + n);
						if (!pt.isValid())
							continue;
						f32 dist = (pt.x * pt.x + pt.y * pt.y + pt.z * pt.z);
						m_minDistance = MIN(m_minDistance,dist);
						m_maxDistance = MAX(m_maxDistance,dist);

						// 统计加载至内存的总有效点个数
						m_validCount++;
					}
				}
			}
			if (!bRet)
			{
				bLoadLoopSuc = false;
				m_pts.clearBlock(iBlock);
				break;
			}
			m_pts.setLoopIdx(iBlock,i);
			// 设置块下标
			iBlock = (m_editMode == 1) ? iBlock + 1:iBlock+ m_simpleLevel;

			iReadLoop++;
			i+= m_simpleLevel;
			
			// 蔡红云 2013.7.9 注释的原因是，操作执行完成之后，进度条不满。
			if (loadCallback && (iReadLoop % 20) == 0)
			{
				loadCallback(iReadLoop / (float)loopSimpleCount,HDSCENE_IDS_PROCESS_LOADPOINTCLOUD);
			}
		}

		// 更新m_pts的块和下标对应关系
		m_pts.update();

		// 点云加载某一圈失败时，需将整个m_pts清除，即点云已加载的所有数据移除，外部应提示内存不足--add by zhubo 2014.07.02
		if (bLoadLoopSuc == false)
		{
			string strFile = hlsFile;
			clear();
			loadHlsFileHeader(strFile.data());
			return FALSE;
		}

		if (loadCallback)
		{
			loadCallback(0.0f,HDSCENE_IDS_FINISH);
		}
		m_transModel.m_fOffset[0] = m_header.offsetX;
		m_transModel.m_fOffset[1] = m_header.offsetY;
		m_transModel.m_fOffset[2] = m_header.offsetZ;
		m_transModel.m_fAngle[0] = m_header.rotateX;
		m_transModel.m_fAngle[1] = m_header.rotateY;
		m_transModel.m_fAngle[2] = m_header.rotateZ;
		m_transModel.m_fScale = m_header.scale;
		m_transModel.Angle2RotateMatrix();
		m_transModel.Parameter2matrix();

		// 增加在加载点云时，计算每圈点云的obb box，方向为指向车行前进方向
		if (bIsNormalPcd)
		{
			// 根据系统标示符进行判断，标示符为3LS表明为地面站，计算box
			if (strcmp(m_header.system_identifier,"HD 3LS 001") == 0
				|| strcmp(m_header.system_identifier,"HD 3LS ZFS") == 0)
			{
				CalculatObbBoxInStation();
			}
			else
			{
				CalculateObbBox();
			}
		}

		// 使用过滤器过滤
		if (m_pFilterManager && m_pFilterManager->getFilterCount() > 0)
		{
			ApplyFilters(true);
		}

		m_bChange = true;
		return TRUE;
	}

	//! 将hls点云文件一次性加载到内存,通过空间抽稀加载
	BOOL PointCloud::loadSpaceSimple(const char* hlsFile,float filter_dist,void (*loadCallback)(float,const char*))
	{		
		if (filter_dist < 0.00001 && filter_dist >0.5)
		{
			return FALSE;
		}
		// 简化圈
		int simpleLoop = 50;
		// 最大灰度值序号
		m_maxIntensity = 0;
		m_minIntensity = 99999;
		if (m_hlsReader && strcmp(m_hlsReader->GetFilePath(),hlsFile) != 0)
		{
			delete m_hlsReader;
			m_hlsReader = NULL;
		}
		if (m_hlsReader == NULL)
		{
			CHLSReadOpener hlsOpener;
			m_hlsReader = hlsOpener.Open(hlsFile);
			if(m_hlsReader == NULL)
				return FALSE;
		}
		// 空间抽稀距离
		space_filter simpleFilter;
		simpleFilter.set_ptcloud_count(m_hlsReader->m_header.number_of_point_records);
		simpleFilter.set_simple_space(filter_dist);

		int loopCount = m_hlsReader->GetLoopCount();

		// 获取索引
		CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex(loadCallback);

		// 判断有效性
		if (!pLoopIdx)
		{
			return FALSE;
		}

		// 统计强度范围
		for (int i = 0;i<loopCount;i++)
		{
			CLoopIndex& loopIdx = pLoopIdx[i];
			m_maxIntensity = MAX(loopIdx.m_loopIdx.maxIntensity,m_maxIntensity);
			m_minIntensity = MIN(loopIdx.m_loopIdx.minIntensity,m_minIntensity);
			// 统计文件中有效点数
			m_HlsValidCount += pLoopIdx[i].m_loopIdx.count;
		}

		m_pts.clear();	
		m_header = m_hlsReader->m_header;

		s64 toLoadCount = (s64)floor(m_hlsReader->m_header.number_of_point_records * (m_endScale - m_startScale) + 0.5);
		u32 nStartCol = (u32)floor(m_startScale * m_hlsReader->GetLoopCount() + 0.5);
		u32 nEndCol = (u32)floor(m_endScale * m_hlsReader->GetLoopCount() + 0.5);
				

		s32 iReadLoop = 0;
		s32 loopIndex = nStartCol;
		u32 loopSimpleCount = nEndCol - nStartCol/* + 1*/;//m_hlsReader->GetLoopCount() / simpleLoop;
		
		m_simpleHeader = m_header;
		
		m_pts.setBlockCount(loopSimpleCount);
		m_simpleHeader.number_of_col = loopSimpleCount;

		// 标记，当某一圈点云new失败时，直接返回
		bool bLoadLoopSuc = true;

		m_validCount = 0;
		u32 iBlock = 0;
		u32 blkCount = m_pts.blockCount();
		for (u32 i = nStartCol;i<nEndCol && iBlock < blkCount;)
		{
			CLoopIndex& loopIdx = pLoopIdx[i];
			hdVector<PointXYZIPRGBA>& loopBuf = m_pts.getBlock(iBlock);
			BOOL bRet = m_hlsReader->ReadLoop(loopBuf,i);
			
			if (!bRet)
			{
				bLoadLoopSuc = false;
				m_pts.clearBlock(iBlock);
				break;
			}
			int filterd = simpleFilter.simple_points_f<PointXYZIPRGBA>(loopBuf._Myfirst,loopBuf.size());
			loopBuf.resize(filterd);
			m_pts.setLoopIdx(iBlock,i);
			// 设置块下标
			iBlock++;

			iReadLoop++;
			//i+= m_simpleLevel;
			i++;

			// 更新内存中有效点
			m_validCount += filterd;

			if (loadCallback && (iReadLoop % 20) == 0)
			{
				loadCallback(iReadLoop / (float)loopSimpleCount,HDSCENE_IDS_PROCESS_LOADPOINTCLOUD);
			}
		}

		// 更新m_pts的块和下标对应关系
		m_pts.update();

		// 点云加载某一圈失败时，需将整个m_pts清除，即点云已加载的所有数据移除，外部应提示内存不足--add by zhubo 2014.07.02
		if (bLoadLoopSuc == false)
		{
			string strFile = hlsFile;
			clear();
			loadHlsFileHeader(strFile.data());
			return FALSE;
		}

		if (loadCallback)
		{
			loadCallback(0.0f,HDSCENE_IDS_FINISH);
		}
		m_transModel.m_fOffset[0] = m_header.offsetX;
		m_transModel.m_fOffset[1] = m_header.offsetY;
		m_transModel.m_fOffset[2] = m_header.offsetZ;
		m_transModel.m_fAngle[0] = m_header.rotateX;
		m_transModel.m_fAngle[1] = m_header.rotateY;
		m_transModel.m_fAngle[2] = m_header.rotateZ;
		m_transModel.m_fScale = m_header.scale;
		m_transModel.Angle2RotateMatrix();
		m_transModel.Parameter2matrix();
		
		// 采样计算浮动0.1个点

		double temp_sample = 1.0 * m_HlsValidCount / m_validCount;

		if (temp_sample < 1.1)
		{
			m_simpleLevel = 1;
		}
		else
		{
			m_simpleLevel = (u32)sqrt(1.0 * m_HlsValidCount / m_validCount) + 1;
		}
		

		// 增加在加载点云时，计算每圈点云的obb box，方向为指向车行前进方向
		bool bIsNormalPcd = isNormalPointCloud();
		if (bIsNormalPcd)
		{
			// 根据系统标示符进行判断，标示符为3LS表明为地面站，计算box
			if (strcmp(m_header.system_identifier,"HD 3LS 001") == 0
				|| strcmp(m_header.system_identifier,"HD 3LS ZFS") == 0)
			{
				CalculatObbBoxInStation();
			}
			else
			{
				CalculateObbBox();
			}
		}

		// 使用过滤器过滤
		if (m_pFilterManager && m_pFilterManager->getFilterCount() > 0)
		{
			ApplyFilters(true);
		}
	
		m_bChange = true;
		return TRUE;
	}

	//! 获取圈范围
	void PointCloud::getLoopExtent(u32 index,f32& xmin,f32& ymin,f32& zmin,f32& xmax,f32& ymax,f32& zmax)
	{
		xmin = ymin = zmin = xmax = ymax = zmax = 0.0f;
		if(index >= m_pts.blockCount() || m_hlsReader == NULL)
			return;
		CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex(NULL);
		if(pLoopIdx == NULL)
			return;
		s32 loopIndex = m_pts.getLoopIdx(index);
		s32 nLoopCount = m_hlsReader->GetLoopCount();

		//if (m_editMode == 1)
		//{
		//	// 计算该内存中圈数在文件对应的圈索引值
		//	float fDataScale = loopIndex * 1.0f / getLoopCount();
		//	loopIndex = floor(nLoopCount * fDataScale + 0.5);
		//}
		//else
		//{
		//	loopIndex = index;
		//}

		if(loopIndex >= nLoopCount)
			return;

		xmin = pLoopIdx[loopIndex].m_loopIdx.xmin;
		ymin = pLoopIdx[loopIndex].m_loopIdx.ymin;
		zmin = pLoopIdx[loopIndex].m_loopIdx.zmin;

		xmax = pLoopIdx[loopIndex].m_loopIdx.xmax;
		ymax = pLoopIdx[loopIndex].m_loopIdx.ymax;
		zmax = pLoopIdx[loopIndex].m_loopIdx.zmax;
	}

	// 判断当前列是否在视窗内
	BOOL PointCloud::IsLoopInView(
		PointXYZIPRGBA* ptBuf,					// 点数组
		u32	count,
		BOOL (*ViewTrans)(const f32&,const f32&,const f32&,s32&,s32&),		// 三维坐标转屏幕坐标回调函数
		s32 srcWidth,									// 视口宽度
		s32 srcHeight,									// 视口高度
		u32 uStart,										// 起始点号
		u32 uStep,										// 采样判断步长
		HRGN srcRgn)										
	{
		u32 ptCount = count;
		// 如果一直递归循环到每隔ptCount / 20个点进行判断时,则认为当前列不在视窗内
		if (uStep <= ptCount / 20)
		{
			return FALSE;
		}
		BOOL bInView = FALSE;

		for (u32 i = uStart;i < ptCount;i+=uStep)
		{
			const PointXYZIPRGBA& pt = ptBuf[i];
			if(!pt.isValid())
				continue;
			s32 srcX = -1,srcY = -1;
			if (ViewTrans(pt.x,pt.y,pt.z,srcX,srcY))
			{
				if (srcRgn)
				{				
					i = 0;
					while(!bInView && i < ptCount)
					{
						const PointXYZIPRGBA& pt = ptBuf[i++];
						if(ViewTrans(pt.x,pt.y,pt.z,srcX,srcY))
						{
							bInView = ::PtInRegion(srcRgn,srcX,srcY);
						}
					}
				}
				else
					bInView = TRUE;
				break;
			}
		}
		//return bInView;
		if (bInView)
		{
			return TRUE;
		}
		else
			return IsLoopInView(ptBuf,count,ViewTrans,srcWidth,srcHeight,uStep / 2,uStart == 0? uStep:uStep / 2,srcRgn);
	}

	BOOL PointCloud::LoadByViewPort( 
		/*std::vector<u8>& vecInview ,*/
		bool (*IsCubeIn)(irr::core::aabbox3df& cube),
		s32 srcWidth,													/* 视口宽度 */ 
		s32 srcHeight,													/* 视口高度 */ 
		const CHdBox3df& box,											/* 视锥体外边框 */ 
		const bool isOrthogonal,										/* 是否为正射投影*/ 
		HRGN srcRgn /*= NULL*/ )
	{
		if(m_hlsReader == NULL || IsCubeIn == NULL || m_editMode == 1)
			return FALSE;

		// 表明未更新数据，返回false
		if(m_hlsReader->m_header.version_major < 2)
			return FALSE;

		// 获取索引
		CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex(NULL);
#ifdef _DEBUG
		LARGE_INTEGER tFreq;
		QueryPerformanceFrequency(&tFreq);
		LARGE_INTEGER tStartQ;
		QueryPerformanceCounter(&tStartQ);
#endif
		// 检查列范围,每隔多少列判断一次,如果是块存储,则不能抽稀判断
		BOOL bIsMesh = m_hlsReader->m_header.is_mesh();
		u32 checkStep = 1;//bIsMesh ? 1:20;	

		// 在视图内点个数
		u64 inViewCount = 0;
		
		// 在视图内列个数
		u32 inViewColCount = 0;
		
		// 记录扫描圈是否在范围内的数组
		std::vector<u8> vecInview;
		vecInview.resize(m_hlsReader->m_header.number_of_col);

		s32 srcX = -1,srcY = -1;

		// 定义顶点数组
		f32 vertex[24] = {0.0};

		//CHdobBox3d loopBox;
		irr::core::aabbox3df loopBox;
		for (u32 i = 0;i < m_hlsReader->m_header.number_of_col;i++)
		{
			CLoopIndex& loopIdx = pLoopIdx[i];

			loopBox.MinEdge.X = loopIdx.m_loopIdx.xmin;
			loopBox.MinEdge.Y = loopIdx.m_loopIdx.ymin;
			loopBox.MinEdge.Z = loopIdx.m_loopIdx.zmin;
			loopBox.MaxEdge.X = loopIdx.m_loopIdx.xmax;
			loopBox.MaxEdge.Y = loopIdx.m_loopIdx.ymax;
			loopBox.MaxEdge.Z = loopIdx.m_loopIdx.zmax;

			//// 初始化节点		
			//memset(vertex,0.0,sizeof(f32) * 24);
			//
			//// 得到顶点相对坐标并赋值至box
			//GetObbVertex(i,vertex);
			//loopBox.SetVertex(vertex);

			// 判断是否相交
			bool bInView = IsCubeIn(loopBox);
			if(bInView)
			{
				vecInview[i] = 1;
				inViewCount += loopIdx.m_loopIdx.count;
				inViewColCount++;
			}
		}

		BOOL bChange = FALSE;
		if(inViewCount > 0)
		{
#ifdef _DEBUG
			LARGE_INTEGER tStartR;
			QueryPerformanceCounter(&tStartR);
#endif // _DEBUG
			// 计算抽稀级别
			m_simpleLevel = 1;
			if (inViewCount <= m_loadSimple)
			{
				m_simpleLevel = 1;
			}
			else
			{
				m_simpleLevel = (u32)inViewCount / m_loadSimple + 1;
			}
			
			// 清理不在范围内的列
			for (u32 i = 0;i<vecInview.size();i++)
			{
				if(vecInview[i] == 0)
				{
					hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
					if (loopPts.size() > 0)
					{
						m_pts.clearBlock(i);
						bChange = TRUE;
					}
				}
			}

			// 加载范围内的列
			//bool bIsNormalPcd = isNormalPointCloud();// && m_simpleLevel == 1;
			u32 iRead = 0;
			for (u32 i = 0;i<vecInview.size();)
			{				
				// 获取后续连续m_simpleLevel个在范围内的列
				u32 nextInView = 0;
				u32 j;
				for (j = i;j < vecInview.size();j++)
				{
					if(vecInview[j] == 1)
					{
						nextInView++;
					}
					if (nextInView >= m_simpleLevel)
					{
						j++;
						break;
					}
				}
				
				// 判断后续连续m_simpleLevel个在范围内的列,在内存有多少列
				u32 inMemCount = 0;
				for (u32 k = i;k < j;k++)
				{
					hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(k);
					if (loopPts.size() > 0)
					{
						inMemCount++;
					}
				}

				if (inMemCount == 0)// 内存没有,则加载第i列
				{
					hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
					m_hlsReader->ReadLoop(loopPts,i);

					m_pts.setLoopIdx(i,i);
					iRead++;
					bChange = TRUE;
				}
				//else if (inMemCount == 1)// 内存刚好有1列,则不处理
				//{;}
				else if (inMemCount > 1) // 内存多于1列,卸载其他列
				{
					inMemCount = 0;
					for (u32 k = i;k < j;k++)
					{
						hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(k);
						if (loopPts.size() > 0)
						{
							inMemCount++;
						}
						if (inMemCount > 1)
						{
							m_pts.clearBlock(k);
							bChange = TRUE;
						}
					}
				}
				i = j;
			}
			m_pts.update();

			// 更新内存中有效点个数
			m_validCount = 0;

			// 获取范围内的列号
			m_queryResult.clear();
			m_queryResult.resize(m_hlsReader->m_header.number_of_col);
			u32 index = 0;
			for (u32 i = 0;i<vecInview.size();i++)
			{
				hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
				if (loopPts.size() > 0)
				{
					m_queryResult[index] = i;

					CLoopIndex& loopIdx = pLoopIdx[i];
					m_validCount += loopIdx.m_loopIdx.count;

					index++;
				}
			}
			m_queryResult.resize(index);
#ifdef _DEBUG
			LARGE_INTEGER tEndR;
			QueryPerformanceCounter(&tEndR);
			char strDbg[256] = {0};
			sprintf(strDbg,"query:%lf\tload:%lf\trdLoop:%d\n",
				(double)(tStartR.QuadPart - tStartQ.QuadPart)/tFreq.QuadPart,
				(double)(tEndR.QuadPart - tStartR.QuadPart)/tFreq.QuadPart,iRead);
			OutputDebugString(strDbg);
#endif
		}
		return bChange;
	}

	BOOL PointCloud::LoadByEnvelope(double xmin, double xmax, double ymin, double ymax)
	{
		// 表明未更新数据，返回false
		if(m_hlsReader->m_header.version_major < 2)
			return FALSE;

		// 获取索引
		CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex(NULL);
#ifdef _DEBUG
		LARGE_INTEGER tFreq;
		QueryPerformanceFrequency(&tFreq);
		LARGE_INTEGER tStartQ;
		QueryPerformanceCounter(&tStartQ);
#endif
		// 检查列范围,每隔多少列判断一次,如果是块存储,则不能抽稀判断
		BOOL bIsMesh = m_hlsReader->m_header.is_mesh();
		u32 checkStep = 1;//bIsMesh ? 1:20;	

		// 在视图内点个数
		u64 inViewCount = 0;
		// 在视图内列个数
		u32 inViewColCount = 0;
		// 记录扫描圈是否在范围内的数组
		std::vector<u8> vecInview;
		vecInview.resize(m_hlsReader->m_header.number_of_col);

		// 遍历圈，统计可视化的点数和圈数
		s32 srcX = -1,srcY = -1;
		const CBursaWolfModel& transModel = GetModel();
		irr::core::aabbox3df fullExtent((float)xmin, (float)ymin, 0.f, (float)xmax, (float)ymax, 0.f);
		irr::core::aabbox3df loopBox;
		for (u32 i = 0;i < m_hlsReader->m_header.number_of_col;i++)
		{
			CLoopIndex& loopIdx = pLoopIdx[i];
			float xminf = loopIdx.m_loopIdx.xmin;
			float yminf = loopIdx.m_loopIdx.ymin;
			float zminf = loopIdx.m_loopIdx.zmin;
			float xmaxf = loopIdx.m_loopIdx.xmax;
			float ymaxf = loopIdx.m_loopIdx.ymax;
			float zmaxf = loopIdx.m_loopIdx.zmax;

			transModel.Translate(xminf, yminf, zminf);
			transModel.Translate(xmaxf, ymaxf, zmaxf);

			loopBox.MinEdge.X = xminf;
			loopBox.MinEdge.Y = yminf;
			loopBox.MinEdge.Z = 0.0;
			loopBox.MaxEdge.X = xmaxf;
			loopBox.MaxEdge.Y = ymaxf;
			loopBox.MaxEdge.Z = 0,0;

			bool bInView = fullExtent.intersectsWithBox(loopBox);

			if(bInView)
			{
				vecInview[i] = 1;
				inViewCount += loopIdx.m_loopIdx.count;
				inViewColCount++;
			}
		}

		BOOL bChange = FALSE;
		if(inViewCount > 0)
		{
#ifdef _DEBUG
			LARGE_INTEGER tStartR;
			QueryPerformanceCounter(&tStartR);
#endif // _DEBUG
			// 计算抽稀级别
			u32 m_simpleLevel = 1;
			if (inViewCount <= m_loadSimple)
			{
				m_simpleLevel = 1;
			}
			else
			{
				m_simpleLevel = (u32)inViewCount / m_loadSimple + 1;
			}

			// 清理不在范围内的列
			for (u32 i = 0;i < vecInview.size();i++)
			{
				if(vecInview[i] == 0)
				{
					hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
					if (loopPts.size() > 0)
					{
						m_pts.clearBlock(i);
						bChange = TRUE;
					}
				}
			}

			//// 记录视图范围内的起始圈 
			//u32 nStartInView = U32_MAX;
			//u32 iRead = 0;

			//// 清理不在范围内的列，记录在范围内的起始圈，将需要保留的圈读取，被抽稀的直接移除
			//for (u32 i = 0;i<vecInview.size();i++)
			//{
			//	if(*(vecInview._Myfirst + i) == 0)
			//	{
			//		hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
			//		if (loopPts.size() > 0)
			//		{
			//			m_pts.clearBlock(i);
			//		}
			//	}
			//	else
			//	{
			//		// 可见的第一圈记录，作为起始抽稀
			//		if (i < nStartInView)
			//		{
			//			nStartInView = i;
			//		}

			//		if ((i - nStartInView) % m_simpleLevel == 0)
			//		{
			//			// 已经在内存，不做处理，否则需要加载
			//			hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
			//			if (loopPts.size() > 0)
			//			{
			//				continue;
			//			}
			//			else
			//			{
			//				m_hlsReader->ReadLoop(loopPts,i);
			//				m_pts.setLoopIdx(i,i);
			//			}
			//		}
			//		else
			//		{
			//			// 已经在内存中，应该清除
			//			hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
			//			if (loopPts.size() > 0)
			//			{
			//				m_pts.clearBlock(i);
			//			}

			//			// 将该圈在此范围内标记为0
			//			*(vecInview._Myfirst + i) = 0;
			//		}
			//	}
			//}

			// 加载范围内的列
			//bool bIsNormalPcd = isNormalPointCloud();// && m_simpleLevel == 1;
			u32 iRead = 0;
			for (u32 i = 0;i < vecInview.size();)
			{				
				// 获取后续连续m_simpleLevel个在范围内的列
				u32 nextInView = 0;
				u32 j;
				for (j = i;j < vecInview.size();j++)
				{
					if(vecInview[j] == 1)
					{
						nextInView++;
					}
					if (nextInView >= m_simpleLevel)
					{
						j++;
						break;
					}
				}
				// 判断后续连续m_simpleLevel个在范围内的列,在内存有多少列
				u32 inMemCount = 0;
				for (u32 k = i;k < j;k++)
				{
					hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(k);
					if (loopPts.size() > 0)
					{
						inMemCount++;
					}
				}

				if (inMemCount == 0)// 内存没有,则加载第i列
				{
					hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
					m_hlsReader->ReadLoop(loopPts,i);

					m_pts.setLoopIdx(i,i);
					iRead++;
					bChange = TRUE;
				}
				//else if (inMemCount == 1)// 内存刚好有1列,则不处理
				//{;}
				else if (inMemCount > 1) // 内存多于1列,卸载其他列
				{
					inMemCount = 0;
					for (u32 k = i;k < j;k++)
					{
						hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(k);
						if (loopPts.size() > 0)
						{
							inMemCount++;
						}
						if (inMemCount > 1)
						{
							m_pts.clearBlock(k);
							bChange = TRUE;
						}
					}
				}
				i = j;
			}
			m_pts.update();

			// 获取范围内的列号
			m_queryResult.clear();
			m_queryResult.resize(m_hlsReader->m_header.number_of_col);
			u32 index = 0;
			for (u32 i = 0;i<vecInview.size();i++)
			{
				hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
				if (loopPts.size() > 0)
				{
					m_queryResult[index] = i;
					index++;
				}
			}
			m_queryResult.resize(index);
#ifdef _DEBUG
			LARGE_INTEGER tEndR;
			QueryPerformanceCounter(&tEndR);
			char strDbg[256] = {0};
			sprintf(strDbg,"query:%lf\tload:%lf\trdLoop:%d\n",
				(double)(tStartR.QuadPart - tStartQ.QuadPart)/tFreq.QuadPart,
				(double)(tEndR.QuadPart - tStartR.QuadPart)/tFreq.QuadPart,iRead);
			OutputDebugString(strDbg);
#endif
		}
		return bChange;
	}

	inline u32 PointCloud::QueryByExtent(f64 xmin,f64 ymin,f64 zmin,f64 xmax,f64 ymax,f64 zmax)
	{
		if(m_hlsReader == NULL)
			return 0;
		const CBursaWolfModel& transModel = GetModel();
		// 底平面4个点
		double x1 = xmin,y1 = ymin,z1 = zmin;
		double x2 = xmax,y2 = ymin,z2 = zmin;
		double x3 = xmax,y3 = ymax,z3 = zmin;
		double x4 = xmin,y4 = ymax,z4 = zmin;
		// 顶平面4个点
		double x5 = xmin,y5 = ymin,z5 = zmax;
		double x6 = xmax,y6 = ymin,z6 = zmax;
		double x7 = xmax,y7 = ymax,z7 = zmax;
		double x8 = xmin,y8 = ymax,z8 = zmax;

		transModel.AntiTranslate(x1,y1,z1);
		transModel.AntiTranslate(x2,y2,z2);
		transModel.AntiTranslate(x3,y3,z3);
		transModel.AntiTranslate(x4,y4,z4);

		transModel.AntiTranslate(x5,y5,z5);
		transModel.AntiTranslate(x6,y6,z6);
		transModel.AntiTranslate(x7,y7,z7);
		transModel.AntiTranslate(x8,y8,z8);

		xmin = MIN(MIN(MIN(MIN(x1,x2),x3),x4),MIN(MIN(MIN(x5,x6),x7),x8));
		ymin = MIN(MIN(MIN(MIN(y1,y2),y3),y4),MIN(MIN(MIN(y5,y6),y7),y8));
		zmin = MIN(MIN(MIN(MIN(z1,z2),z3),z4),MIN(MIN(MIN(z5,z6),z7),z8));

		xmax = MAX(MAX(MAX(MAX(x1,x2),x3),x4),MAX(MAX(MAX(x5,x6),x7),x8));
		ymax = MAX(MAX(MAX(MAX(y1,y2),y3),y4),MAX(MAX(MAX(y5,y6),y7),y8));
		zmax = MAX(MAX(MAX(MAX(z1,z2),z3),z4),MAX(MAX(MAX(z5,z6),z7),z8));

		return QueryByExtent((f32)xmin,(f32)ymin,(f32)zmin,(f32)xmax,(f32)ymax,(f32)zmax);
	}
	//! 统计矩形框范围内点云的高度范围,xmin,xmax,ymin,ymax是输入参数,zmin和zmax是计算结果
	BOOL PointCloud::QueryZRange(f64 xmin,f64 xmax,f64 ymin,f64 ymax,bool bStatSel,f64& zmin,f64& zmax)
	{

		if(m_hlsReader == NULL)
		{
			return FALSE;
		}
			
		const CBursaWolfModel& transModel = GetModel();

		// 底平面4个点
		double x1 = xmin,y1 = ymin,z1 = 0;
		double x2 = xmax,y2 = ymin,z2 = 0;
		double x3 = xmax,y3 = ymax,z3 = 0;
		double x4 = xmin,y4 = ymax,z4 = 0;

		// 顶平面4个点
		double x5 = xmin,y5 = ymin,z5 = 0;
		double x6 = xmax,y6 = ymin,z6 = 0;
		double x7 = xmax,y7 = ymax,z7 = 0;
		double x8 = xmin,y8 = ymax,z8 = 0;

		transModel.AntiTranslate(x1,y1,z1);
		transModel.AntiTranslate(x2,y2,z2);
		transModel.AntiTranslate(x3,y3,z3);
		transModel.AntiTranslate(x4,y4,z4);

		transModel.AntiTranslate(x5,y5,z5);
		transModel.AntiTranslate(x6,y6,z6);
		transModel.AntiTranslate(x7,y7,z7);
		transModel.AntiTranslate(x8,y8,z8);

		xmin = MIN(MIN(MIN(MIN(x1,x2),x3),x4),MIN(MIN(MIN(x5,x6),x7),x8));
		ymin = MIN(MIN(MIN(MIN(y1,y2),y3),y4),MIN(MIN(MIN(y5,y6),y7),y8));

		xmax = MAX(MAX(MAX(MAX(x1,x2),x3),x4),MAX(MAX(MAX(x5,x6),x7),x8));
		ymax = MAX(MAX(MAX(MAX(y1,y2),y3),y4),MAX(MAX(MAX(y5,y6),y7),y8));

		CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex();
		if (pLoopIdx == NULL || m_hlsReader->m_header.number_of_col <= 0)
		{
			return FALSE;
		}

		f32 zmintmp = 999999999.f;
		f32 zmaxtmp = -999999999.f;

		BOOL flag = FALSE;
		u32 i,j;
		for (i = 0;i< m_pts.blockCount();i++)
		{
			u32 loopIndex = m_pts.getLoopIdx(i);
			CLoopIndex& loopIdx = pLoopIdx[loopIndex];
			if (loopIdx.m_loopIdx.xmax < xmin || loopIdx.m_loopIdx.xmin > xmax ||
				loopIdx.m_loopIdx.ymax < ymin || loopIdx.m_loopIdx.ymin > ymax )
			{
				continue;
			}

			const hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
			for (j = 0;j < loopPts.size();j++)
			{
				const PointXYZIPRGBA& pt = *(loopPts._Myfirst + j);
				if (pt.x < xmin || pt.x > xmax ||
					pt.y < ymin || pt.y > ymax )
				{
					continue;
				}

				if (!pt.isSelected() && bStatSel)
				{
					continue;
				}
				zmintmp = min(zmintmp, pt.z);
				zmaxtmp = max(zmaxtmp, pt.z);
			}

			flag = true;
		}

		double zmintmpd = zmintmp;
		double zmaxtmpd = zmaxtmp;


		// 统计成功
		if (flag)
		{
			transModel.Translate(xmin, ymin, zmintmpd);
			transModel.Translate(xmax, ymax, zmaxtmpd);

			zmin = zmintmpd;
			zmax = zmaxtmpd;
		}		
		//  统计失败
		else
		{
			zmin = 0.0;
			zmax = 0.0;
		}
		return flag;
	}

	u32 PointCloud::getIndexBySelectionID(int selIndex) const
	{
		// 链表的位置
		int nListPos = selIndex / (int)0xffff;

		// 链表内部的位置
		int nListInnerPos = selIndex % (int)0xffff;

		// 迭代器移动到指定的链表位置
		list<vector<u32>>::const_iterator it = m_listSelectionIDs.begin();
// 		for (int i = 0; i < nListPos; i++)
// 		{
// 			it++;
// 		}
		std::advance(it, nListPos);		// 这种方式比上面的for循环快8倍左右[zfei 2014/12/22]

		// 获得点的序号
		const vector<u32>& vecSelIDs = *(it);

		return vecSelIDs[nListInnerPos];
	}

	hd::u32 PointCloud::QueryByExtent( f32 xmin,f32 ymin,f32 zmin,f32 xmax,f32 ymax,f32 zmax )
	{
		if(m_hlsReader == NULL)
			return 0;
		CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex();
		if (pLoopIdx == NULL || m_hlsReader->m_header.number_of_col <= 0)
		{
			return 0;
		}
		m_queryResult.clear();
		m_queryResult.resize(m_hlsReader->m_header.number_of_col);

		u32 index = 0;
		u32 colPtCount = 0;
		for (u32 i = 0;i<m_hlsReader->m_header.number_of_col;i++)
		{
			CLoopIndex& loopIdx = pLoopIdx[i];
			if (loopIdx.m_loopIdx.xmax < xmin || loopIdx.m_loopIdx.xmin > xmax ||
				loopIdx.m_loopIdx.ymax < ymin || loopIdx.m_loopIdx.ymin > ymax ||
				loopIdx.m_loopIdx.zmax < zmin || loopIdx.m_loopIdx.zmin > zmax)
			{
				continue;
			}
			m_queryResult[index] = i;

			index++;
		}
		m_queryResult.resize(index);
		m_curRoop = 0;
		return index;
	}

	//! 查询所有列或者块
	hd::u32 PointCloud::QueryAll()
	{
		if(m_hlsReader == NULL || !m_hlsReader->IsOpen())
			return 0;
		CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex();
		if (pLoopIdx == NULL || m_hlsReader->m_header.number_of_col <= 0)
		{
			return 0;
		}
		m_queryResult.clear();
		m_queryResult.resize(m_hlsReader->m_header.number_of_col);

		u32 i = 0;
		for (i = 0;i<m_hlsReader->m_header.number_of_col;i++)
		{		
			m_queryResult[i] = i;
		}

		m_curRoop = 0;
		return m_hlsReader->m_header.number_of_col;
	}

	void PointCloud::ResetRead()
	{
		m_curRoop = 0;
	}

	//! 读取下一个扫描列,返回是否成功
	BOOL PointCloud::ReadNext(hdVector<PointXYZIPRGBA>& pPtBuf)
	{
		if(m_hlsReader == NULL || !m_hlsReader->IsOpen())
			return FALSE;

		if(m_queryResult.size() == 0 || m_curRoop >= m_queryResult.size())
		{
			return FALSE;
		}
		u32 loopIndex = m_queryResult[m_curRoop++];
		// 返回不包括无效点的当前圈
		return m_hlsReader->ReadLoop(pPtBuf,loopIndex);
	}

	void PointCloud::GetExtent( f32& xmin,f32& ymin,f32& zmin,f32& xmax,f32& ymax,f32& zmax )
	{
		if(m_queryResult.size() == 0 || m_curRoop >= m_queryResult.size())
		{
			return;
		}
		u32 loopIndex = m_queryResult[m_curRoop];
		CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex();
		const CLoopIndex& loopIdx = pLoopIdx[m_curRoop];
		xmin = loopIdx.m_loopIdx.xmin;
		ymin = loopIdx.m_loopIdx.ymin;
		zmin = loopIdx.m_loopIdx.zmin;
		xmax = loopIdx.m_loopIdx.xmax;
		ymax = loopIdx.m_loopIdx.ymax;
		zmax = loopIdx.m_loopIdx.zmax;
	}

	void PointCloud::SelectPoint(HRGN hRgn,
		s32 srcWidth,									// 视口宽度
		s32 srcHeight,									// 视口高度
		BOOL (*ViewTrans)(const f32&,const f32&,const f32&,s32&,s32&))		// 三维坐标转屏幕坐标回调函数);
	{
		if(m_hlsReader == NULL || !m_hlsReader->IsOpen())
			return ;
		if(m_queryResult.size() == 0 )
		{
			return ;
		}

#ifdef _DEBUG
		LARGE_INTEGER tFreq;
		QueryPerformanceFrequency(&tFreq);
		LARGE_INTEGER tStartQ;
		QueryPerformanceCounter(&tStartQ);
#endif

		u32 blkCount = m_pts.blockCount();
		u32 inRgn = 0;

		for (int i = 0;i<blkCount;i++)
		{
			hdVector<PointXYZIPRGBA>& vecPts = m_pts.getBlock(i);
			u32 ptCount = vecPts.size();
			BOOL bInView = FALSE;
			const PointXYZIPRGBA& pt = *(vecPts._Myfirst + ptCount-1);
			s32 srcX = -1,srcY = -1;
			if (!ViewTrans(pt.x,pt.y,pt.z,srcX,srcY))
			{
				// 判断其他点是否在范围内
				bInView = IsLoopInView(vecPts._Myfirst,ptCount,ViewTrans,srcWidth,srcHeight,0,ptCount / 2,hRgn);			
			}
			else
			{
				for (u32 i = 0;i<ptCount;i++)
				{
					PointXYZIPRGBA& pt = *(vecPts._Myfirst + i);
					ViewTrans(pt.x,pt.y,pt.z,srcX,srcY);
					if(PtInRegion(hRgn,srcX,srcY))
					{
						pt.setSelected();
					}
				}
				bInView = TRUE;
			}
			if (bInView)
			{
				inRgn++;
			}
		}

#ifdef _DEBUG
		LARGE_INTEGER tEnd;
		QueryPerformanceCounter(&tEnd);
		char strDbg[256] = {0};
		sprintf(strDbg,"select:%lf\tinRgn:%d\n",
			(double)(tEnd.QuadPart - tStartQ.QuadPart)/tFreq.QuadPart,inRgn);
		OutputDebugString(strDbg);
#endif
	}

	BOOL PointCloud::open()
	{
		BOOL bRet = FALSE;
		if(m_hlsReader)
		{
			std::string strPath = m_hlsReader->GetFilePath();
			delete m_hlsReader;
			CHLSReadOpener hlsOpener;
			m_hlsReader = hlsOpener.Open(strPath.c_str());
			if (m_hlsReader)
			{
				m_header = m_hlsReader->m_header;
				bRet = TRUE;
			}
		}
		return bRet;
	}

	BOOL PointCloud::loadHlsFileHeader(const char* hlsFile)
	{
		if(m_hlsReader == NULL)
		{
			CHLSReadOpener hlsOpener;
			m_hlsReader = hlsOpener.Open(hlsFile);
		}

		if(m_hlsReader == NULL)
			return FALSE;
		m_header = m_hlsReader->GetHeader();
		m_simpleHeader = m_header;
		m_transModel.m_fOffset[0] = m_header.offsetX;
		m_transModel.m_fOffset[1] = m_header.offsetY;
		m_transModel.m_fOffset[2] = m_header.offsetZ;
		m_transModel.m_fAngle[0] = m_header.rotateX;
		m_transModel.m_fAngle[1] = m_header.rotateY;
		m_transModel.m_fAngle[2] = m_header.rotateZ;
		m_transModel.m_fScale = m_header.scale;
		m_transModel.Angle2RotateMatrix();
		m_transModel.Parameter2matrix();
		return TRUE;
	}

	BOOL PointCloud::loadHlsData(void (*loadCallback)(float,const char*))
	{
		if (m_hlsReader && _access(m_hlsReader->GetFilePath(), 04) == 0)
		{
			m_startScale = 0.0;
			m_endScale = 1.0;
			return loadHlsFile(m_hlsReader->GetFilePath(),loadCallback);
		}
		return FALSE;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	unsigned int PointCloud::computeMeanAndCovarianceMatrix (
		const std::vector<size_t> &indices,
		Eigen::Matrix3f &covariance_matrix,
		Eigen::Vector4f &centroid)
	{
		// create the buffer on the stack which is much faster than using cloud.points[indices[i]] and centroid as a buffer
		Eigen::Matrix<float, 1, 9, Eigen::RowMajor> accu = Eigen::Matrix<float, 1, 9, Eigen::RowMajor>::Zero ();
		size_t point_count;

		point_count = indices.size ();
		for (std::vector<size_t>::const_iterator iIt = indices.begin (); iIt != indices.end (); ++iIt)
		{
			//const PointXYZIPRGBA& point = m_fullIndex ? *(m_pts._Myfirst + (*iIt)) :
			//	*(m_pts._Myfirst + (*(m_selectionIDs._Myfirst + (*iIt))));;//*(m_pts._Myfirst + (*iIt));
			if (*iIt >= m_pts.count())
			{
				continue;
			}
			const PointXYZIPRGBA& point = m_fullIndex ? m_pts[(*iIt)] :
										m_pts[getIndexBySelectionID(*iIt)];
										//m_pts[(*(m_selectionIDs._Myfirst + (*iIt)))];//*(m_pts._Myfirst + (*iIt));

			accu [0] += point.x * point.x;
			accu [1] += point.x * point.y;
			accu [2] += point.x * point.z;
			accu [3] += point.y * point.y;
			accu [4] += point.y * point.z;
			accu [5] += point.z * point.z;
			accu [6] += point.x;
			accu [7] += point.y;
			accu [8] += point.z;
		}			

		accu /= static_cast<float> (point_count);
		centroid[0] = accu[6]; centroid[1] = accu[7]; centroid[2] = accu[8];
		centroid[3] = 0;
		covariance_matrix.coeffRef (0) = accu [0] - accu [6] * accu [6];
		covariance_matrix.coeffRef (1) = accu [1] - accu [6] * accu [7];
		covariance_matrix.coeffRef (2) = accu [2] - accu [6] * accu [8];
		covariance_matrix.coeffRef (4) = accu [3] - accu [7] * accu [7];
		covariance_matrix.coeffRef (5) = accu [4] - accu [7] * accu [8];
		covariance_matrix.coeffRef (8) = accu [5] - accu [8] * accu [8];
		covariance_matrix.coeffRef (3) = covariance_matrix.coeff (1);
		covariance_matrix.coeffRef (6) = covariance_matrix.coeff (2);
		covariance_matrix.coeffRef (7) = covariance_matrix.coeff (5);

		return (static_cast<unsigned int> (point_count));
	}

	void PointCloud::solveScatterParmeters(const Eigen::Matrix3f &covariance_matrix,
		float &nLine, float &nPlane, float &nSphere)
	{
		nLine = 0.f;
		nPlane = 0.f;
		nSphere = 0.f;
		Eigen::Vector3f eVals;		
		hd::eigen33(covariance_matrix, eVals);		// 计算特征值，升序排列
		if (fabs(eVals[2]) > 0.f)
		{
			nLine = (eVals[2] - eVals[1])/eVals[2];
			nPlane = (eVals[1] - eVals[0])/eVals[2];
			nSphere = eVals[0]/eVals[2];
		}
	}

	void PointCloud::solvePlaneParameters(const Eigen::Matrix3f &covariance_matrix,
		float &nx, float &ny, float &nz, float &curvature)
	{
		Eigen::Vector3f::Scalar eigen_value;
		Eigen::Vector3f eigen_vector;
		hd::eigen33 (covariance_matrix, eigen_value, eigen_vector);

		nx = eigen_vector [0];
		ny = eigen_vector [1];
		nz = eigen_vector [2];

		// Compute the curvature surface change
		float eig_sum = covariance_matrix.coeff (0) + covariance_matrix.coeff (4) + covariance_matrix.coeff (8);
		if (eig_sum != 0)
			curvature = fabsf (eigen_value / eig_sum);
		else
			curvature = 0;
	}

	void PointCloud::flipNormalTowardsViewpoint (const PointXYZIPRGBA &point, float vp_x, float vp_y, float vp_z,
		float &nx, float &ny, float &nz)
	{
		// See if we need to flip any plane normals
		vp_x -= point.x;
		vp_y -= point.y;
		vp_z -= point.z;

		// Dot product between the (viewpoint - point) and the plane normal
		float cos_theta = (vp_x * nx + vp_y * ny + vp_z * nz);

		// Flip the plane normal
		if (cos_theta < 0)
		{
			nx *= -1;
			ny *= -1;
			nz *= -1;
		}
	}

	void PointCloud::computeScatter( int k, float dist, void (*loadCallback)(float,const char*) , bool bFull )
	{
		if (k == 0)// && dist < 0.000001
		{
			return;
		}
		m_fullIndex = bFull;
		if (loadCallback)
		{
			loadCallback(0.0,HDSCENE_IDS_LOADDATA_CREATING_INDEX);
		}

		if (m_pIndex == NULL)
		{
			if (!buildIndex(bFull))
			{
				return;
			}
		}

		std::vector<size_t> nn_indices (k);
		std::vector<float> nn_dists (k);

		// Iterating over the entire index vector
		u64 ptcount = bFull? m_pts.count():m_selectCount;
		m_normal.clear();
		m_normal.resize(ptcount);

		//float vpx = m_header.centerX;
		//float vpy = m_header.centerY;
		//float vpz = m_header.centerZ;
		hd_kd_tree_t* kdIndex = (hd_kd_tree_t*)m_pIndex;
		float* fPt;

		for (size_t idx = 0; idx < ptcount; ++idx)
		{
			PointXYZIPRGBA& pt = bFull ? m_pts[idx] ://*(m_pts._Myfirst + idx) :
				m_pts[getIndexBySelectionID(idx)];
				//m_pts[*(m_selectionIDs._Myfirst + idx)];//*(m_pts._Myfirst + (*(m_selectionIDs._Myfirst + idx)));
		if (!pt.isValid())
		{
			continue;
		}

		fPt = (float*)(&pt);
		kdIndex->knnSearch(fPt,k,&nn_indices[0], &nn_dists[0]);

		// 暂时用法向变量存储
		Normal& normal = m_normal[idx];//*(m_normal._Myfirst + idx);//bFull ? *(m_normal._Myfirst + idx) :
		//*(m_normal._Myfirst + (*(m_selectionIDs._Myfirst + idx)));

		computePointScatter(nn_indices,
			normal.nx, normal.ny, normal.nz);

		/*			flipNormalTowardsViewpoint (pt, 0.0,0.0, 0.0,
		normal.nx, normal.ny, normal.nz);*/

		if (loadCallback && (idx % 10000) == 0)
		{
			loadCallback(idx / (float)ptcount,HDSCENE_IDS_LOADDATA_CALCULATING_SCATTER);
		}
		}

		if (loadCallback)
		{
			loadCallback(0.0f,HDSCENE_IDS_FINISH);
		}
	}

	void PointCloud::computePointScatter( const std::vector<size_t> &indices, float &nLine, float &nPlane, float &nShpere )
	{
		if (computeMeanAndCovarianceMatrix (indices, m_covariance_matrix, m_xyz_centroid) == 0)
		{
			nLine = nPlane = nShpere = std::numeric_limits<float>::quiet_NaN ();
			return;
		}
		solveScatterParmeters(m_covariance_matrix, nLine, nPlane, nShpere);
	}

	void PointCloud::computePointNormal ( const std::vector<size_t> &indices, float &nx, float &ny, float &nz, float &curvature)
	{
		if (computeMeanAndCovarianceMatrix (indices, m_covariance_matrix, m_xyz_centroid) == 0)
		{
			nx = ny = nz = curvature = std::numeric_limits<float>::quiet_NaN ();
			return;
		}

		// Get the plane normal and surface curvature
		solvePlaneParameters (m_covariance_matrix, nx, ny, nz, curvature);
	}

	void PointCloud::computeNormal(int k,float dist,void (*loadCallback)(float,const char*),bool bFull)
	{
		if (k == 0)// && dist < 0.000001
		{
			return;
		}
		m_fullIndex = bFull;
		if (loadCallback)
		{
			loadCallback(0.0,HDSCENE_IDS_LOADDATA_CREATING_INDEX);
		}

		// 判断是否需要重新计算索引在buildIndex内部有判断，外部不应处理
		if (!buildIndex(bFull))
		{
			return;
		}

		std::vector<size_t> nn_indices (k);
		std::vector<float> nn_dists (k);

		// Iterating over the entire index vector
		u64 ptcount = bFull? m_pts.count():m_selectCount;
		m_normal.clear();
		m_normal.resize(ptcount);

		//float vpx = m_header.centerX;
		//float vpy = m_header.centerY;
		//float vpz = m_header.centerZ;
		hd_kd_tree_t* kdIndex = (hd_kd_tree_t*)m_pIndex;
		float* fPt;

		for (size_t idx = 0; idx < ptcount; ++idx)
		{
			PointXYZIPRGBA& pt = bFull ? m_pts[idx] ://*(m_pts._Myfirst + idx) :
				m_pts[getIndexBySelectionID(idx)];
				//m_pts[*(m_selectionIDs._Myfirst + idx)];//*(m_pts._Myfirst + (*(m_selectionIDs._Myfirst + idx)));
		if (!pt.isValid())
		{
			continue;
		}

		fPt = (float*)(&pt);
		kdIndex->knnSearch(fPt,k,&nn_indices[0], &nn_dists[0]);

		Normal& normal = m_normal[idx];//*(m_normal._Myfirst + idx);//bFull ? *(m_normal._Myfirst + idx) :
		//*(m_normal._Myfirst + (*(m_selectionIDs._Myfirst + idx)));

		computePointNormal (nn_indices,
			normal.nx, normal.ny, normal.nz, normal.curvature);

		/*			flipNormalTowardsViewpoint (pt, 0.0,0.0, 0.0,
		normal.nx, normal.ny, normal.nz);*/

		if (loadCallback && (idx % 10000) == 0)
		{
			loadCallback(idx / (float)ptcount,HDSCENE_IDS_LOADDATA_CALCULATING_NORMAL);
		}
		}

		if (loadCallback)
		{
			loadCallback(0.0f,HDSCENE_IDS_FINISH);
		}
	}

	bool PointCloud::isNormalPointCloud() const
	{
		if (m_simpleHeader.number_of_col == 0 ||
			m_simpleHeader.number_of_col == 1 ||
			m_simpleHeader.number_of_row == 0 ||
			m_simpleHeader.number_of_row == 1 ||
			m_simpleHeader.number_of_col * m_simpleHeader.number_of_row
			!= m_simpleHeader.number_of_point_records)
		{
			return false;
		}
		return true;
	}

	void PointCloud::deleteSelectPoints()
	{
		u32 delCount = 0;
		m_validCount = 0;
		u32 loopCount = m_pts.blockCount();
		for (u32 n = 0;n < loopCount;n++)
		{
			hdVector<PointXYZIPRGBA>& pts = getLoop(n);
			u64 ptCount = pts.size();
			for (u32 i = 0;i < ptCount;i++)
			{
				PointXYZIPRGBA& pt = *(pts._Myfirst + i);
				if (!pt.isValid())
					continue;

				if (pt.isSelected())
				{
					pt.setDeleted();
					delCount++;
					m_delPcdNum++;
				}
				else
				{
					m_validCount++;
				}
			}
		}

		//m_selectionIDs.clear();
		m_listSelectionIDs.clear();
		m_ClassSelectionIDs.clear();
		m_selectCount = 0;
		if (delCount > 0)
		{
			m_bEdited = true;
		}
	}

	// 设置选择点数
	bool PointCloud::setSelectCount()
	{
 		try  //以list<vector>代替vector，避免分配不到连续内存[zfei 2014/7/8]
 		{
 			u32 idsSize = 0xffff;		// 65535
 
			m_listSelectionIDs.clear();		// 先清空
 			m_listSelectionIDs.resize(1);
 			u32 ptcount = (u32)m_pts.count();
 			u32 ptIndex = 0;
 
 			for (u32 i = 0; i < ptcount;i++)
 			{
 				PointXYZIPRGBA& pt = m_pts[i];
 				if(!pt.isValid() || !pt.isSelected())
 					continue;
 
 				// 取链表的最后一个vector
 				std::vector<U32>& vecSelIDs = m_listSelectionIDs.back();
 				bool bNewVec = false;
 				if (vecSelIDs.size() >= idsSize)
 				{
 					// 最后一个vector已满，增加一个新的vector
 					m_listSelectionIDs.resize(m_listSelectionIDs.size() + 1);
 
 					bNewVec = true;
 				}
 				if (!bNewVec)
 				{
 					vecSelIDs.push_back(i);
 				}
 				else
 				{
 					std::vector<U32>& vecNewSelIDs = m_listSelectionIDs.back();
 					vecNewSelIDs.push_back(i);
 				}
 
 				ptIndex++;
 				
 			}
 
 			m_selectCount = ptIndex;
 
 			return true;
 			
 		}
 		catch (...)
 		{
 			// 清除选择
 			SetUnSelect();	
 
 			// 给出提示
 			::MessageBox(NULL, HDSCENE_IDS_MEMORY_REQUSTFAILED, HDSCENE_IDS_ERROR, MB_OK);	
 
 			return false;
 		}

	}

	// 统计第一次选择点数
	bool PointCloud::setSelectCountF()
	{
		try
		{
			// 获取选择集ID
			u32 idsSize = 0xffff;
			m_ClassSelectionIDs.clear();
			m_ClassSelectionIDs.resize(1);

			u32 ptcount = (u32)m_pts.count();
			u32 ptIndex = 0;

			for (u32 i = 0; i < ptcount;i++)
			{
				PointXYZIPRGBA& pt = m_pts[i];
				if(!pt.isValid())
					continue;

				// 统计分类选中
				if (pt.isClassifySelectedF())
				{		
					// 取链表的最后一个vector
					std::vector<U32>& vecSelIDs = m_ClassSelectionIDs.back();
					bool bNewVec = false;
					if (vecSelIDs.size() >= idsSize)
					{
						// 最后一个vector已满，增加一个新的vector
						m_ClassSelectionIDs.resize(m_ClassSelectionIDs.size() + 1);

						bNewVec = true;
					}
					if (!bNewVec)
					{
						vecSelIDs.push_back(i);
					}
					else
					{
						std::vector<U32>& vecNewSelIDs = m_ClassSelectionIDs.back();
						vecNewSelIDs.push_back(i);
					}

					ptIndex++;
				}
			}
			m_selectCountF = ptIndex;
			return true;
		}
		catch(...)
		{
			// 清除选择
			SetUnSelectF();	

			return false;
		}
	
	}

	BOOL PointCloud::loadHlsByScale( 
		double startScale, /* 起始比例 0.0-1.0 */ 
		double endScale, /* 终止比例 0.0-1.0 endScale >= startScale */ 
		void (*loadCallback)(float,const char*) /*= NULL*/ )
	{
		m_startScale = startScale;
		m_endScale = endScale;
		return loadHlsFile(m_hlsReader->GetFilePath(),loadCallback);
	}

	// 获取绝对坐标范围
	const void PointCloud::GetGlobalExtent(double& xmin,double& ymin,double& zmin,double& xmax,double& ymax,double& zmax)
	{
		xmin = m_header.min_x;
		ymin = m_header.min_y;
		zmin = m_header.min_z;

		xmax = m_header.max_x;
		ymax = m_header.max_y;
		zmax = m_header.max_z;

		m_transModel.TranslateExtent(xmin,ymin,zmin,xmax,ymax,zmax);
	}

	void PointCloud::DeselectPts()
	{
		u32 selCount = getSelectCount();
		for (u32 i = 0;i < selCount;i++)
		{
			PointXYZIPRGBA& pt = getSelectionPoint(i);
			pt.setUnSelected();
		}
		//SetPointCloudChanged(true);
		setSelectCount();
	}

	hd::u32 PointCloud::GetValidCountInLoop( u32 index )
	{
		// 文件指针为空直接返回
		if (m_hlsReader == NULL)
		{
			return 0;
		}

		// 记录该圈的有效点个数
		u32 nValidCountInLoop = 0;

		// 获得文件中的总圈数
		u32 nFileLoopCount = m_hlsReader->GetLoopCount();

		// 获取索引
		CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex();
		if (pLoopIdx == NULL)
		{
			return 0;
		}

		// 计算该内存中圈数在文件对应的圈索引值
		f32 fDataScale = index * 1.0f / getLoopCount();
		u32 loopIndex = (u32)floor(nFileLoopCount * fDataScale + 0.5);
		
		// 得到该圈有效点个数
		if (index >= 0 && index < nFileLoopCount)
		{
			CLoopIndex& loopIdx = pLoopIdx[index];
			nValidCountInLoop = loopIdx.m_loopIdx.count;
		}

		return nValidCountInLoop;
	}

	// 读取LIN文件，临时使用
	void PointCloud::ReadLin( const char* strLinPath,std::vector<HD_SCANHDIINFO>& vecScanInfo )
	{
		char strBuf[1024];		// 保存每一行字符
		vecScanInfo.clear();
		vecScanInfo.resize(5000);

		// 读取解析pos文件
		FILE* pPosFile = fopen(strLinPath,"rt");
		if (pPosFile == NULL)
		{
			return;
		}

		// 忽略第一行
		fgets(strBuf,1024,pPosFile);	
		unsigned int count = 0;

		// 循环读取
		while(!feof(pPosFile))
		{
			// 读取之前进行初始化
			memset(strBuf,0,1024);
			HD_SCANHDIINFO& hdi = vecScanInfo[count];
			fgets(strBuf,1024,pPosFile);
			if (hdi.Serialize(strBuf))
				count++;
			if (count >= vecScanInfo.size())
			{
				vecScanInfo.resize(vecScanInfo.size() + 5000);
			}
		}
		vecScanInfo.resize(count);
		fclose(pPosFile);
	}

	// 计算每圈点云的obb box
	void PointCloud::CalculateObbBox()
	{
		// lin文件是否存在进行判断
		bool bLinExist = false;
		string strLinPath = GetPosFileByHlsPath(bLinExist);
		m_bLinExist = bLinExist;
		if (!bLinExist)
		{
			return;
		}

		// 定义存放lin文件中pos信息的vec
		std::vector<HD_SCANHDIINFO> vecScanInfo;

		// 从文件中读取lin
		ReadLin( strLinPath.data(),vecScanInfo );

		// 此处应获得文件的总圈数，而不是内存中总圈数
		//u32 nLoopCount = getLoopCount();
		U32 nLoopCount = m_hlsReader->GetLoopCount();
		if (nLoopCount <= 0)
		{
			return;
		}

		f64 posX,posY,posZ;
		posX = posY = posZ = 0.0;
		
		f64 targetX,targetY,targetZ;
		targetX = targetY = targetZ = 0.0;

		// lin文件为全部圈数据，应根据比例取值
		f32 fDataScale = 0.0f;
		u32 hdiIndex = 0;
		u32 nHdiCount = vecScanInfo.size();

		// 将vec清空
		m_vecObbVertex.clear();

		// 逐圈计算obb box范围
		for (u32 i = 0;i < nLoopCount;i++)
		{
			fDataScale = (float)1.0 * i / nLoopCount;
			hdiIndex = (u32)floor(nHdiCount * fDataScale + 0.5);
			if (hdiIndex >= nHdiCount)
			{
				return;
			}


			HD_SCANHDIINFO& hdi = vecScanInfo[hdiIndex];

			// 该圈pos中心点
			posX = hdi.dX;
			posY = hdi.dY;
			posZ = hdi.dZ;

			// 下一圈中心点,如果当前圈是最后一圈，则下一圈指向前一圈，法向量取其反方向
			if ((i == nLoopCount - 1) && nLoopCount > 1)
			{
				HD_SCANHDIINFO& hdiTag = vecScanInfo[hdiIndex-1];
				targetX = hdiTag.dX;
				targetY = hdiTag.dY;
				targetZ = hdiTag.dZ;
			}
			else
			{
				HD_SCANHDIINFO& hdiTag = vecScanInfo[hdiIndex+1];
				targetX = hdiTag.dX;
				targetY = hdiTag.dY;
				targetZ = hdiTag.dZ;
			}

			//posX = *(m_vecLoopCenter._Myfirst + i*3);
			//posY = *(m_vecLoopCenter._Myfirst + i*3+1);
			//posZ = *(m_vecLoopCenter._Myfirst + i*3+2);

			//if (i == nLoopCount - 1)
			//{
			//	targetX = *(m_vecLoopCenter._Myfirst + (i-1)*3);
			//	targetY = *(m_vecLoopCenter._Myfirst + (i-1)*3+1);
			//	targetZ = *(m_vecLoopCenter._Myfirst + (i-1)*3+2);
			//}
			//else
			//{
			//	targetX = *(m_vecLoopCenter._Myfirst + (i+1)*3);
			//	targetY = *(m_vecLoopCenter._Myfirst + (i+1)*3+1);
			//	targetZ = *(m_vecLoopCenter._Myfirst + (i+1)*3+2);
			//}

			// 转换相对坐标
			m_transModel.AntiTranslate(posX,posY,posZ);
			m_transModel.AntiTranslate(targetX,targetY,targetZ);

			// 计算每圈obb box 8个角点值,target点Z值应与pos相同，以确保obb box平行与XOY平面
			targetZ = posZ;
			CalculLoopObbBox(i, (float)posX, (float)posY, (float)posZ, (float)targetX, (float)targetY, (float)targetZ);
		}

		// 将vec大小重新设置
		m_vecObbVertex.resize(nLoopCount);
	}

	void PointCloud::CalculLoopObbBox( u32 loopIndex, f32 posX,f32 posY,f32 posZ,f32 targetX,f32 targetY,f32 targetZ )
	{
		// 结构体定义
		LoopObboxVertex structVertex;

		// 获得当前圈的最大最小Z值
		int loopCount = getLoopCount();

		// 获取索引
		CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex();
		CLoopIndex& loopIdx = pLoopIdx[loopIndex];

		// 获得Z的范围值
		// obb box底平面顺时针分别为索引0、1、2、3,底平面高程值为dMinZ
		// obb box顶平面顺时针分别为索引4、5、6、7,顶平面高程值为dMaxZ
		f32 fMaxZ = loopIdx.m_loopIdx.zmax;
		f32 fMinZ = loopIdx.m_loopIdx.zmin;

		f32 fMaxX = loopIdx.m_loopIdx.xmax;
		f32 fMinX = loopIdx.m_loopIdx.xmin;
		f32 fMaxY = loopIdx.m_loopIdx.ymax;
		f32 fMinY = loopIdx.m_loopIdx.ymin;


		// 定义临时中间变量，用于记录角点
		f32 fTempX,fTempY,fTempZ;

		// 计算变化值
		fTempX = targetX - posX;
		fTempY = targetY - posY;
		fTempZ = targetZ - posZ;

		// 计算前后面距离pos中心点的距离值
		f32 fDist = sqrt(pow(fTempX,2) 
			+ pow(fTempY,2) + pow(fTempZ,2));
		
		// 防止除0情况
		if (fDist <= 0.f)
		{
			fDist = 0.02f;
		}
		// 法向量,并单位化
		f32 normalX,normalY,normalZ;
		normalX = fTempX;
		normalY = fTempY;
		normalZ = fTempZ;
		normalX /= fDist;
		normalY /= fDist;
		normalZ /= fDist;

		// 需要注意，obb box的主轴线向量设计为平行与XOY平面，法向量Z方向始终为0
		// 由主轴线方向法向量（a,b,0）可知0、3角点构成的法向量为（b,-a，0）

		// 定义obb box长宽高
		f32 dHeight,dWidth,dLength;
		dHeight = dWidth = dLength = 0.0;

		// 0、3角点构成的边长长为330 * 2，2、3角点构成的边长宽为2* fDist，0、4角点构成的边长高为fMaxZ-fMinZ
		dHeight = fMaxZ - fMinZ;
		dWidth = 2 * fDist;
		//dLength = 2 * 330.0;

		f32 ftmp = sqrt(pow(fMaxX - fMinX,2) + pow(fMaxY - fMinY,2));
		dLength = ftmp;

		// 求索引为0的角点坐标，可知其Z为fMinZ
		fTempX = ( dLength / 2.0f ) * normalY + posX - fDist * normalX;
		fTempY = ( dLength / 2.0f ) * (-1 * normalX) + posY - fDist * normalY;
		fTempZ = fMinZ;

		// 角点0 
		structVertex.vertex0[0] = fTempX;
		structVertex.vertex0[1] = fTempY;
		structVertex.vertex0[2] = fTempZ;

		// 角点4坐标X.Y与角点1相同，Z坐标为fMinZ
		fTempZ = fMaxZ;
		structVertex.vertex4[0] = fTempX;
		structVertex.vertex4[1] = fTempY;
		structVertex.vertex4[2] = fTempZ;

		// 求角点3坐标,向量方向与指向0方向反向
		fTempX = -1 * ( dLength / 2.0f ) * normalY + posX - fDist * normalX;
		fTempY = -1 * ( dLength / 2.0f ) * (-1 * normalX) + posY - fDist * normalY;
		fTempZ = fMinZ;

		structVertex.vertex3[0] = fTempX;
		structVertex.vertex3[1] = fTempY;
		structVertex.vertex3[2] = fTempZ;

		// 求角点7坐标，X、Y与角点3相同，Z坐标为fMaxZ
		fTempZ = fMaxZ;
		structVertex.vertex7[0] = fTempX;
		structVertex.vertex7[1] = fTempY;
		structVertex.vertex7[2] = fTempZ;

		// 求角点1坐标
		fTempX = ( dLength / 2.0f ) * normalY + posX + fDist * normalX;
		fTempY = ( dLength / 2.0f ) * (-1 * normalX) + posY + fDist * normalY;
		fTempZ = fMinZ;

		structVertex.vertex1[0] = fTempX;
		structVertex.vertex1[1] = fTempY;
		structVertex.vertex1[2] = fTempZ;

		// 求角点5，X、Y与角点1相同，Z坐标为fMaxZ
		fTempZ = fMaxZ;
		structVertex.vertex5[0] = fTempX;
		structVertex.vertex5[1] = fTempY;
		structVertex.vertex5[2] = fTempZ;

		// 求角点2坐标，向量方向与指向1反向
		fTempX = -1 * ( dLength / 2.0f ) * normalY + posX + fDist * normalX;
		fTempY = -1 * ( dLength / 2.0f ) * (-1 * normalX) + posY + fDist * normalY;
		fTempZ = fMinZ;

		structVertex.vertex2[0] = fTempX;
		structVertex.vertex2[1] = fTempY;
		structVertex.vertex2[2] = fTempZ;

		// 求角点6，X、Y与角点2相同，Z坐标为fMaxZ
		fTempZ = fMaxZ;
		structVertex.vertex6[0] = fTempX;
		structVertex.vertex6[1] = fTempY;
		structVertex.vertex6[2] = fTempZ;

		m_vecObbVertex.push_back(structVertex);

	}

	// 获得顶点
	void PointCloud::GetObbVertex( u32 index,f32* vertex )
	{
		// 文件总圈数判断
		u32 nLoopCount = m_hlsReader->GetLoopCount();
		if (nLoopCount <= 0)
		{
			return;
		}

		// 文件的圈点云obb box已计算，此处直接取点
		u32 loopIndex = 0;
		f32 fDataScale = 0.0f;

		//loopIndex = m_pts.getLoopIdx(index);
		
		if (m_editMode == 1)
		{
			// 计算该内存中圈数在文件对应的圈索引值
			fDataScale = index * 1.0f / getLoopCount();
			u32 lpIndex = (u32)floor(nLoopCount * fDataScale + 0.5);

			if (m_startScale == 0.0f && m_endScale == 1.0f)
			{
				loopIndex = lpIndex;
			}
			else
			{
				loopIndex = m_pts.getLoopIdx(lpIndex);
			}
		}
		else
		{
			loopIndex = index;
		}

		//判断该索引值是否在范围内
		if (loopIndex < 0 || loopIndex >= nLoopCount)
		{
			return;
		}

		LoopObboxVertex& structVertex = m_vecObbVertex[loopIndex];

		// 顶点赋值
		// 角点0
		vertex[0] = structVertex.vertex0[0];
		vertex[1] = structVertex.vertex0[1];
		vertex[2] = structVertex.vertex0[2];

		// 角点1
		vertex[3] = structVertex.vertex1[0];
		vertex[4] = structVertex.vertex1[1];
		vertex[5] = structVertex.vertex1[2];

		// 角点2
		vertex[6] = structVertex.vertex2[0];
		vertex[7] = structVertex.vertex2[1];
		vertex[8] = structVertex.vertex2[2];

		// 角点3
		vertex[9] = structVertex.vertex3[0];
		vertex[10] = structVertex.vertex3[1];
		vertex[11] = structVertex.vertex3[2];

		// 角点4
		vertex[12] = structVertex.vertex4[0];
		vertex[13] = structVertex.vertex4[1];
		vertex[14] = structVertex.vertex4[2];

		// 角点5
		vertex[15] = structVertex.vertex5[0];
		vertex[16] = structVertex.vertex5[1];
		vertex[17] = structVertex.vertex5[2];

		// 角点6
		vertex[18] = structVertex.vertex6[0];
		vertex[19] = structVertex.vertex6[1];
		vertex[20] = structVertex.vertex6[2];

		// 角点7
		vertex[21] = structVertex.vertex7[0];
		vertex[22] = structVertex.vertex7[1];
		vertex[23] = structVertex.vertex7[2];
	}

	//void PointCloud::CalculLoopCenter()
	//{
	//	// 总圈数
	//	u32 nLoopCount = getLoopCount();

	//	m_vecLoopCenter.clear();

	//	// 获取索引
	//	CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex();

	//	// 定义临时变量值
	//	f32 fTempX,fTempY,fTempZ;
	//	fTempX = fTempY = fTempZ = 0.0f;

	//	// 遍历计算每圈点云中心位置
	//	for (u32 i = 0;i < nLoopCount;i++)
	//	{
	//		CLoopIndex& loopIdx = pLoopIdx[i];

	//		fTempX = (loopIdx.m_loopIdx.xmin + loopIdx.m_loopIdx.xmax)/2.0f;
	//		fTempY = (loopIdx.m_loopIdx.ymin + loopIdx.m_loopIdx.ymax)/2.0f;
	//		fTempZ = (loopIdx.m_loopIdx.zmin + loopIdx.m_loopIdx.zmax)/2.0f;

	//		m_vecLoopCenter.push_back(fTempX);
	//		m_vecLoopCenter.push_back(fTempY);
	//		m_vecLoopCenter.push_back(fTempZ);
	//	}
	//}

	// //统计pos中心
	//void PointCloud::StatLoopForPos()
	//{
	//	// 获得总loop圈数,初始化中心点vec
	//	u32 nLoopCount = getLoopCount();
	//	m_vecLoopCenter.clear();

	//	// 定义圈分段数组,定义相邻两点间距离值变量
	//	int statCountMap[36];

	//	// 定义记录最小距离点坐标的临时变量
	//	f32 fTempX,fTempY,fTempZ;
	//	fTempX = fTempY = fTempZ = 0.0f;
	//	u32 nTemp = U32_MIN;

	//	CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex();

	//	// 遍历统计，依次计算每个点到上一个点的距离
	//	for (u32 n = 0;n < nLoopCount;n++)
	//	{
	//		// 初始化statCountMap
	//		memset(statCountMap,0,sizeof(int)*36);

	//		// 获取每一圈点，遍历计算
	//		const hdVector<PointXYZIPRGBA>& pts = getLoop(n);
	//		
	//		// 获得该圈总点数，并对其进行分段
	//		u32 ptCount = pts.size();
	//		u32 ptAver = floor(ptCount * 1.0 / 36);

	//		for (u32 i = 0;i < pts.size();i++)
	//		{
	//			// 取点
	//			const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
	//			if (pt.isValid())
	//			{
	//				u32 step = i / ptAver;

	//				// 在此范围内，统计计数
	//				if (step >=0 && step < 36)
	//				{
	//					statCountMap[step]++;
	//				}
	//			}

	//			//// 读取前一个点，如果前一个点为无效点，则两相邻点距离必定大于两相邻有效点间距离，不需进行计算
	//			//const PointXYZIPRGBA& ptPre = *(pts._Myfirst + i-1);
	//			//if (!ptPre.isValid())
	//			//	continue;

	//			//// 获得点，无效点不需进行计算
	//			//const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
	//			//if (!pt.isValid())
	//			//	continue;

	//			//// 逐点计算
	//			//fTemp = sqrt(pow((pt.x - ptPre.x),2) 
	//			//	+ pow((pt.y - ptPre.y),2) + pow((pt.z - ptPre.z),2));

	//			//// 取距离最小点为pos点，记录下来
	//			//if (fTemp < fDist)
	//			//{
	//			//	fTempX = pt.x;
	//			//	fTempY = pt.y;
	//			//	fTempZ = pt.z;
	//			//	
	//			//	fDist = fTemp;
	//			//}
	//		}

	//		// 根据该圈点范围分布，获得有效点最多的那段
	//		u32 MaxStep = 0;
	//		for (u32 k = 0;k < 36;k++)
	//		{
	//			if (nTemp < statCountMap[k])
	//			{
	//				nTemp = statCountMap[k];
	//				MaxStep = k;
	//			}
	//		}

	//		fTempX = fTempY = fTempZ = 0.0f;

	//		// 再次重新读取该段点云，以其平均值作为该圈点云的pos中心
	//		for (u32 m = MaxStep*ptAver;m < (MaxStep+1)*ptAver;m++)
	//		{
	//			// 求总和
	//			const PointXYZIPRGBA& pt = *(pts._Myfirst + m);
	//			if (pt.isValid())
	//			{
	//				fTempX += pt.x;
	//				fTempY += pt.y;
	//				fTempZ += pt.z;
	//			}
	//		}

	//		// 求平均值
	//		fTempX /= statCountMap[MaxStep];
	//		fTempY /= statCountMap[MaxStep];
	//		fTempZ /= statCountMap[MaxStep];

	//		// 一圈遍历完后，记录中心点
	//		m_vecLoopCenter.push_back(fTempX);
	//		m_vecLoopCenter.push_back(fTempY);
	//		m_vecLoopCenter.push_back(fTempZ);
	//	}
	//}

	string PointCloud::GetPosFileByHlsPath( bool& bLinExist )
	{
		// 初始化为false
		bLinExist = false;

		// 获取点云路径
		string strPath = GetPointCloudPath();
		if (strPath == "")
		{
			return "";
		}

		std::transform(strPath.begin(),strPath.end(),strPath.begin(),tolower);

		// 字符串处理
		string strLinPath = strPath;
		int pos = strLinPath.find_last_of("\\");
		string strName = strLinPath.substr(pos + 1);
		int count = 0;

		// 查询是否含有lin文件
		if (pos != -1)
		{
			pos = strName.find(".hls");
			strName.replace(pos,4,".lin");
			std::transform(strName.begin(),strName.end(),strName.begin(),tolower);
			pos = strName.find("-pcd-");
			if (pos != -1)
			{
				strName.replace(pos,5,"-pos-");
				pos = strPath.find_last_of("\\");
				strLinPath = strPath.substr(0,pos + 1) + strName;

				if (_access(strLinPath.c_str(), 04) == 0)
				{
					// 有lin文件情况下的提取
					bLinExist = true;

					return strLinPath;
				}
			}
		}

		return "";
	}

	BOOL PointCloud::LoadByObbViewPort( CHdobBox3d viewBox,irr::core::aabbox3df viewAabbox/*,std::vector<u8>& vecInview*/ )
	{

// 		string strTstFile = "D:\\statisticTime.txt";
// 		FILE* pFile = fopen(strTstFile.c_str(),"w+t");

		//long clock1,clock2,clock3,clock4;


		if(m_hlsReader == NULL || m_editMode == 1)
			return FALSE;

		// 表明未更新数据，返回false
		if(m_hlsReader->m_header.version_major < 2)
			return FALSE;

		// 获取索引
		CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex(NULL);
#ifdef _DEBUG
		LARGE_INTEGER tFreq;
		QueryPerformanceFrequency(&tFreq);
		LARGE_INTEGER tStartQ;
		QueryPerformanceCounter(&tStartQ);
#endif
		// 检查列范围,每隔多少列判断一次,如果是块存储,则不能抽稀判断
		BOOL bIsMesh = m_hlsReader->m_header.is_mesh();
		u32 checkStep = 1;//bIsMesh ? 1:20;	

		// 在视图内点个数
		u64 inViewCount = 0;
		
		// 在视图内列个数
		u32 inViewColCount = 0;
		
		// 记录扫描圈是否在范围内的数组
		std::vector<u8> vecInview;
		vecInview.resize(m_hlsReader->m_header.number_of_col);

		// 定义顶点数组，圈的方向box
		f32 vertex[24] = {0.0};
		CHdobBox3d loopBox;

		// 首先用aabbox相交进行判断，如果相交，再用obbox进一步判断
		irr::core::aabbox3df aabLoopBox;
		f32 xmin,ymin,zmin,xmax,ymax,zmax;

		//clock1 = clock();

		// 遍历判断相交
		for (u32 i = 0;i < m_hlsReader->m_header.number_of_col;i++)
		{
			xmin = ymin = zmin = xmax = ymax = zmax = 0.0f;

			CLoopIndex& loopIdx = pLoopIdx[i];
			xmin = loopIdx.m_loopIdx.xmin;
			ymin = loopIdx.m_loopIdx.ymin;
			zmin = loopIdx.m_loopIdx.zmin;

			xmax = loopIdx.m_loopIdx.xmax;
			ymax = loopIdx.m_loopIdx.ymax;
			zmax = loopIdx.m_loopIdx.zmax;


			aabLoopBox.MinEdge.set((f32)xmin,(f32)ymin,(f32)zmin);
			aabLoopBox.MaxEdge.set((f32)xmax,(f32)ymax,(f32)zmax);

			// 初始化节点		
			memset(vertex,0,sizeof(f32) * 24);
			
			// 得到顶点相对坐标并赋值至box
			GetObbVertex(i,vertex);
			loopBox.SetVertex(vertex);

			// 判断是否相交
			if (viewAabbox.intersectsWithBox(aabLoopBox))
			{
				if(viewBox.BoxIntersect(&loopBox) == 1)
				{
					vecInview[i] = 1;
					inViewCount += loopIdx.m_loopIdx.count;
					inViewColCount++;
				}
			}
		}

		//clock2 = clock();
		//long calTime = clock2 - clock1;
		int loadCount =0;
		int unLoadCount = 0;
		BOOL bChange = FALSE;
		if(inViewCount > 0)
		{
#ifdef _DEBUG
			LARGE_INTEGER tStartR;
			QueryPerformanceCounter(&tStartR);
#endif // _DEBUG
			// 计算抽稀级别
			m_simpleLevel = 1;
			if (inViewCount <= m_loadSimple)
			{
				m_simpleLevel = 1;
			}
			else
			{
				m_simpleLevel = (u32)inViewCount / m_loadSimple + 1;
			}

			// 记录视图范围内的起始圈 
			u32 nStartInView = U32_MAX;
			u32 iRead = 0;

			//clock1 = clock();

			// 清理不在范围内的列，记录在范围内的起始圈，将需要保留的圈读取，被抽稀的直接移除
			for (u32 i = 0;i<vecInview.size();i++)
			{
				if(vecInview[i] == 0)
				{
					hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
					if (loopPts.size() > 0)
					{
						m_pts.clearBlock(i);
					}
				}
				else
				{
					// 可见的第一圈记录，作为起始抽稀
					if (i < nStartInView)
					{
						nStartInView = i;
					}

					if ((i - nStartInView) % m_simpleLevel == 0)
					{
						// 已经在内存，不做处理，否则需要加载
						hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
						if (loopPts.size() > 0)
						{
							continue;
						}
						else
						{
							//clock3 = clock();
							m_hlsReader->ReadLoop(loopPts,i);
							m_pts.setLoopIdx(i,i);
							
							//clock4 = clock();
							loadCount++;
							//fprintf(pFile,"%s%d%s:%d\n","加载第",i,"圈耗时", clock4 - clock3);
						}
					}
					else
					{
						// 已经在内存中，应该清除
						hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
						if (loopPts.size() > 0)
						{
							//clock3 = clock();

							m_pts.clearBlock(i);
							unLoadCount++;
							//clock4 = clock();
							//fprintf(pFile,"%s%d%s:%d\n","清空第",i,"圈耗时", clock4 - clock3);
						}

						// 将该圈在此范围内标记为0
						vecInview[i] = 0;
					}
				}
			}

			//clock2 = clock();
			//fprintf(pFile,"%s:%d\n","统计视锥相交耗时",calTime);
			//fprintf(pFile,"%s:%d\n","加载数据次数",loadCount);
			//fprintf(pFile,"%s:%d\n","卸载数据次数",unLoadCount);
			//fprintf(pFile,"%s:%d\n","加载卸载数据总耗时",clock2 - clock1);

			bChange = TRUE;
			//// 使用obb box采用原抽稀方式会导致越是被抽稀过的地方越要被抽稀，出现空洞
			//// 由于采用obb box读取lin,点云应该是连续一段在视锥内，因此应该固定抽稀
			//u32 iRead = 0;

			//// 直接从在范围内的起始圈开始读
			//for (u32 i = nStartInView;i < vecInview.size();i++)
			//{
			//	// 不在视图范围内不做处理
			//	if (*(vecInview._Myfirst + i) == 0)
			//		continue;

			//	// 在范围内，nStartInView+m_simpleLevel处应该加载，其他地方应该卸载
			//	if ((i - nStartInView) % m_simpleLevel == 0)
			//	{
			//		// 已经在内存，不做处理，否则需要加载
			//		hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
			//		if (loopPts.size() > 0)
			//		{
			//			continue;
			//		}
			//		else
			//		{
			//			m_hlsReader->ReadLoop(loopPts,i);
			//			m_pts.setLoopIdx(i,i);
			//			bChange = TRUE;
			//		}
			//	}
			//	else
			//	{
			//		// 已经在内存中，应该清除
			//		hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
			//		if (loopPts.size() > 0)
			//		{
			//			m_pts.clearBlock(i);
			//			bChange = TRUE;
			//		}

			//		// 将该圈在此范围内标记为0
			//		*(vecInview._Myfirst + i) = 0;
			//	}
			//}

			m_pts.update();
///**************************原抽稀方式 开始****************************/
//			u32 iRead = 0;
//			
//			// 加载范围内的列
//			for (u32 i = 0;i<vecInview.size();)
//			{				
//				// 获取后续连续m_simpleLevel个在范围内的列
//				u32 nextInView = 0;
//				u32 j;
//				for (j = i;j < vecInview.size();j++)
//				{
//					if(*(vecInview._Myfirst + j) == 1)
//					{
//						nextInView++;
//					}
//					if (nextInView >= m_simpleLevel)
//					{
//						j++;
//						break;
//					}
//				}
//
//				// 判断后续连续m_simpleLevel个在范围内的列,在内存有多少列
//				u32 inMemCount = 0;
//				for (u32 k = i;k < j;k++)
//				{
//					hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(k);
//					if (loopPts.size() > 0)
//					{
//						inMemCount++;
//					}
//				}
//
//				if (inMemCount == 0)// 内存没有,则加载第i列
//				{
//					hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
//					m_hlsReader->ReadLoop(loopPts,i);
//
//					m_pts.setLoopIdx(i,i);
//					iRead++;
//					bChange = TRUE;
//				}
//				//else if (inMemCount == 1)// 内存刚好有1列,则不处理
//				//{;}
//				else if (inMemCount > 1) // 内存多于1列,卸载其他列
//				{
//					inMemCount = 0;
//					for (u32 k = i;k < j;k++)
//					{
//						hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(k);
//						if (loopPts.size() > 0)
//						{
//							inMemCount++;
//						}
//						if (inMemCount > 1)
//						{
//							m_pts.clearBlock(k);
//							bChange = TRUE;
//						}
//					}
//				}
//				i = j;
//			}
//			m_pts.update();
//
///**************************原抽稀方式 结束****************************/

			 //获取范围内的列号
			m_queryResult.clear();

			// 统计内存中有效点数
			m_validCount = 0;

			m_queryResult.resize(m_hlsReader->m_header.number_of_col);
			u32 index = 0;
			for (u32 i = 0;i<vecInview.size();i++)
			{
				hdVector<PointXYZIPRGBA>& loopPts = m_pts.getBlock(i);
				if (loopPts.size() > 0)
				{
					m_queryResult[index] = i;

					// 更新有效点个数
					CLoopIndex& loopIdx = pLoopIdx[i];
					m_validCount += loopIdx.m_loopIdx.count;

					index++;
				}
			}
			m_queryResult.resize(index);

//  			fprintf(pFile,"%s\n","显示总圈数");
//  			fprintf(pFile,"%d\n",m_queryResult.size());
//  			fprintf(pFile,"%s\n","******");
//  			fprintf(pFile,"%s\n","******");
 
 			//fclose(pFile);

#ifdef _DEBUG
			LARGE_INTEGER tEndR;
			QueryPerformanceCounter(&tEndR);
			char strDbg[256] = {0};
			sprintf(strDbg,"query:%lf\tload:%lf\trdLoop:%d\n",
				(double)(tStartR.QuadPart - tStartQ.QuadPart)/tFreq.QuadPart,
				(double)(tEndR.QuadPart - tStartR.QuadPart)/tFreq.QuadPart,iRead);
			OutputDebugString(strDbg);
#endif
		}
		return bChange;
	}

	// 外部设置，清除所有选择
	void PointCloud::SetUnSelect()
	{
		// 内存不足返回时，应将之前选择点全部清空--zhubo.2014.06.16
		m_listSelectionIDs.clear();
		m_selectCount = 0;

		for (u32 i = 0; i < m_pts.count();i++)
		{
			PointXYZIPRGBA& pt = m_pts[i];
			if(!pt.isValid())
				continue;
			if (pt.isSelected())
			{
				pt.setUnSelected();
			}
		}
	}
	void PointCloud::SetUnSelectF()
	{
		// 内存不足返回时，应将之前选择点全部清空--zhubo.2014.06.16
		m_ClassSelectionIDs.clear();

		for (u32 i = 0; i < m_pts.count();i++)
		{
			PointXYZIPRGBA& pt = m_pts[i];
			if(!pt.isValid())
				continue;
			if (pt.isClassifySelectedF())
			{
				pt.setUnClassifySelectedF();
			}
		}

		m_selectCountF = 0;//[caihy 2015-2-7 清空时，要更新变量]
	}

	void PointCloud::CalculatObbBoxInStation()
	{
		// 此处应获得文件的总圈数，而不是内存中总圈数
		U32 nLoopCount = m_hlsReader->GetLoopCount();
		if (nLoopCount <= 0)
		{
			return;
		}

		// 获取索引
		CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex();

		// 定义临时中间变量，用于记录角点
		f32 fTempX,fTempY,fTempZ,fTemp,fDist,posX,posY,posZ;
		f32 fMaxZ,fMinZ,fMaxX,fMinX,fMaxY,fMinY;

		// 遍历，计算每圈的obbbox
		for (U32 loopIndex = 0; loopIndex < nLoopCount;loopIndex++)
		{
			// 获取索引
			CLoopIndex& loopIdx = pLoopIdx[loopIndex];

			// 获得Z的范围值
			// obb box底平面顺时针分别为索引0、1、2、3,底平面高程值为dMinZ
			// obb box顶平面顺时针分别为索引4、5、6、7,顶平面高程值为dMaxZ
			fMaxZ = loopIdx.m_loopIdx.zmax;
			fMinZ = loopIdx.m_loopIdx.zmin;

			fMaxX = loopIdx.m_loopIdx.xmax;
			fMinX = loopIdx.m_loopIdx.xmin;
			fMaxY = loopIdx.m_loopIdx.ymax;
			fMinY = loopIdx.m_loopIdx.ymin;

			// 从文件中获取该圈点云索引值最小的有效点
			PointXYZIPRGBA pt;
			GetNearestPtInLoop(loopIndex,pt);
			if (!pt.isValid())
			{
				int tst =0.0;
			}

			// 获取坐标值
			fTempX = pt.x;
			fTempY = pt.y;
			fTempZ = pt.z;

			// 以xy平面平均值为pos位置
			posX = (fMinX + fMaxX) / 2.0f;
			posY = (fMinY + fMaxY) / 2.0f;
			posZ = (fMinZ + fMaxZ) / 2.0f;

			// 防止除0情况，此处暂时取固定值
			//if (fDist <= 0.f)
			{
				fDist = 0.01f;
			}

			fTemp = sqrt(pow(fTempX,2) + pow(fTempY,2));
			if (fTemp <= 0.0f)
			{
				fTemp = 1.0f;
			}

			// 计算法向量,并单位化(地面站点云初始圈方向为本地坐标系的Y方向)，由此可计算每圈点云的法向量
			f32 normalX,normalY,normalZ;
			normalX = fTempY;
			normalY = -1.0 * fTempX;
			normalX /= fTemp;
			normalY /= fTemp;
			normalZ = 0.0;

			// 需要注意，obb box的主轴线向量设计为平行与XOY平面，法向量Z方向始终为0
			// 由主轴线方向法向量（a,b,0）可知0、3角点构成的法向量为（b,-a，0）

			// 定义obb box长宽高
			f32 dHeight,dWidth,dLength;
			dHeight = dWidth = dLength = 0.0;

			// 0、3角点构成的边长长为2* fDist，2、3角点构成的边长宽为dLength，0、4角点构成的边长高为fMaxZ-fMinZ
			dHeight = fMaxZ - fMinZ;
			dWidth = 2 * fDist;

			f32 ftmp = sqrt(pow(fMaxX - fMinX,2) + pow(fMaxY - fMinY,2));
			dLength = ftmp;

			// 结构体定义
			LoopObboxVertex structVertex;

			// 求索引为0的角点坐标，可知其Z为fMinZ
			fTempX = ( dLength / 2.0f ) * normalY + posX - fDist * normalX;
			fTempY = ( dLength / 2.0f ) * (-1 * normalX) + posY - fDist * normalY;
			fTempZ = fMinZ;

			// 角点0 
			structVertex.vertex0[0] = fTempX;
			structVertex.vertex0[1] = fTempY;
			structVertex.vertex0[2] = fTempZ;

			// 角点4坐标X.Y与角点1相同，Z坐标为fMinZ
			fTempZ = fMaxZ;
			structVertex.vertex4[0] = fTempX;
			structVertex.vertex4[1] = fTempY;
			structVertex.vertex4[2] = fTempZ;

			// 求角点3坐标,向量方向与指向0方向反向
			fTempX = -1 * ( dLength / 2.0f ) * normalY + posX - fDist * normalX;
			fTempY = -1 * ( dLength / 2.0f ) * (-1 * normalX) + posY - fDist * normalY;
			fTempZ = fMinZ;

			structVertex.vertex3[0] = fTempX;
			structVertex.vertex3[1] = fTempY;
			structVertex.vertex3[2] = fTempZ;

			// 求角点7坐标，X、Y与角点3相同，Z坐标为fMaxZ
			fTempZ = fMaxZ;
			structVertex.vertex7[0] = fTempX;
			structVertex.vertex7[1] = fTempY;
			structVertex.vertex7[2] = fTempZ;

			// 求角点1坐标
			fTempX = ( dLength / 2.0f ) * normalY + posX + fDist * normalX;
			fTempY = ( dLength / 2.0f ) * (-1 * normalX) + posY + fDist * normalY;
			fTempZ = fMinZ;

			structVertex.vertex1[0] = fTempX;
			structVertex.vertex1[1] = fTempY;
			structVertex.vertex1[2] = fTempZ;

			// 求角点5，X、Y与角点1相同，Z坐标为fMaxZ
			fTempZ = fMaxZ;
			structVertex.vertex5[0] = fTempX;
			structVertex.vertex5[1] = fTempY;
			structVertex.vertex5[2] = fTempZ;

			// 求角点2坐标，向量方向与指向1反向
			fTempX = -1 * ( dLength / 2.0f ) * normalY + posX + fDist * normalX;
			fTempY = -1 * ( dLength / 2.0f ) * (-1 * normalX) + posY + fDist * normalY;
			fTempZ = fMinZ;

			structVertex.vertex2[0] = fTempX;
			structVertex.vertex2[1] = fTempY;
			structVertex.vertex2[2] = fTempZ;

			// 求角点6，X、Y与角点2相同，Z坐标为fMaxZ
			fTempZ = fMaxZ;
			structVertex.vertex6[0] = fTempX;
			structVertex.vertex6[1] = fTempY;
			structVertex.vertex6[2] = fTempZ;

			//// 计算角点坐标值
			//// 角点0 
			//structVertex.vertex0[0] = fPosX - fDist * normalX;
			//structVertex.vertex0[1] = fPosY + fDist * normalY;
			//structVertex.vertex0[2] = fMinZ;

			//// 角点4对应
			//structVertex.vertex4[0] = structVertex.vertex0[0];
			//structVertex.vertex4[1] = structVertex.vertex0[1];
			//structVertex.vertex4[2] = fMaxZ;

			//// 角点3
			//structVertex.vertex3[0] = fPosX + fDist * normalX;
			//structVertex.vertex3[1] = fPosY - fDist * normalY;
			//structVertex.vertex3[2] = fMinZ;

			//// 角点7
			//structVertex.vertex7[0] = structVertex.vertex3[0];
			//structVertex.vertex7[1] = structVertex.vertex3[1];
			//structVertex.vertex7[2] = fMaxZ;

			//// 角点1
			//structVertex.vertex1[0] = structVertex.vertex0[0] + dLength * normalY;
			//structVertex.vertex1[1] = structVertex.vertex0[1] - dLength * normalX;
			//structVertex.vertex1[2] = fMinZ;

			//// 角点5
			//structVertex.vertex5[0] = structVertex.vertex1[0];
			//structVertex.vertex5[1] = structVertex.vertex1[1];
			//structVertex.vertex5[2] = fMaxZ;

			//// 角点2
			//structVertex.vertex2[0] = structVertex.vertex3[0] + dLength * normalY;
			//structVertex.vertex2[1] = structVertex.vertex3[1] - dLength * normalX;
			//structVertex.vertex2[2] = fMinZ;

			//// 角点6
			//structVertex.vertex6[0] = structVertex.vertex2[0];
			//structVertex.vertex6[1] = structVertex.vertex2[1];
			//structVertex.vertex6[2] = fMaxZ;

			m_vecObbVertex.push_back(structVertex);
		}

		m_bLinExist = true;
	}

	void PointCloud::GetNearestPtInLoop( int loopInexInFile,PointXYZIPRGBA& pt )
	{
		// 获得文件总圈数
		U32 nLoopCount = m_hlsReader->GetLoopCount();
		if (nLoopCount <= 0 || loopInexInFile >= nLoopCount)
		{
			return;
		}

		// 获取起始、终止索引值
		U32 nStartIndex = loopInexInFile * m_header.number_of_row;
		U32 nEndIndex = (loopInexInFile + 1) * m_header.number_of_row;

		/*U32 nIndex = 0;*/

		//// 获取索引
		//CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex();
		//if (!pLoopIdx)
		//{
		//	return;
		//}

		//// 遍历获取在当前圈之前文件中总点数
		//for (U32 i = 0;i < loopInexInFile;i++)
		//{
		//	CLoopIndex& loopIdx = pLoopIdx[i];
		//	nIndex += loopIdx.m_loopIdx.rowCount;
		//}

		//U32 nPtCount = pLoopIdx[loopInexInFile].m_loopIdx.rowCount;

		// 获取当前圈总点数，遍历获取
		for (U32 i = nStartIndex;i < nEndIndex;i++)
		{
			// 从文件中取点
			BOOL bRet = getPoint(i,pt);

			// 获取得到有效点即返回
			if (bRet && pt.isValid())
			{
				break;
			}
			else
			{
				continue;
			}
		}

	}

	//! 导出选择过滤范围数据至文件（hls）
	bool PointCloud::export_filter(const char* save_hls_file, CBursaWolfModel* render_transformation,
		ptcloud::hdFilterManager* filter_manager, u32 simple_size/*=1*/, void (*callBack)(float,const char*) /*= NULL*/)
	{
		return false;
		/*if (filter_manager==NULL || filter_manager->getFilterCount()==0)
		{
			return FALSE;
		}
		if (m_ptr_ptcloud_reader==NULL)
		{
			return FALSE;
		}

		//获得HLS读指针及文件头
		hdReaderHLS* src_reader = static_cast<hdReaderHLS*>(m_ptr_ptcloud_reader);
		if (!src_reader)
		{
			return FALSE;
		}
		hdHeaderHLS* src_header = static_cast<hdHeaderHLS*>(src_reader->getHeader());
		if (!src_header)
		{
			return FALSE;
		}

		// 获取点云xyz全局坐标范围
		f64 min_x, min_y, min_z, max_x, max_y, max_z;
		src_header->getGlobalExtent(min_x, min_y, min_z, max_x, max_y, max_z);

		//获得写hls文件头
		hdHeaderHLS hls_header;
		//hls_header.setBoundingBox(min_x, min_y, min_z, max_x, max_y, max_z);
		hls_header.m_offset_x = src_header->m_offset_x;//min_x;
		hls_header.m_offset_y = src_header->m_offset_y;//min_y;
		hls_header.m_offset_z = src_header->m_offset_z;//min_z;
		hls_header.setPointFormat(HLS2_POINTFORMAT_XYZIRGBP);

		//获得hls写指针
		IPtCloudWriterOpener writer_opener;
		IPtCloudWriter* writer = writer_opener.open(save_hls_file, &hls_header);
		if (!writer)
		{
			return FALSE;
		}
		hdWriterHLS* hls_writer = static_cast<hdWriterHLS*>(writer);
		if (!hls_writer)
		{
			return FALSE;
		}

		// 获得物理圈数
		u32 loop_count = src_reader->getLoopCount();
		//循环写数据
		for (u32 i=0; i<loop_count; i++)
		{
			hdPtArray<hdPointXYZF> ptr_tmp_array;
			hdPointIntensity* ptr_tmp_intensity = NULL;
			hdPointRGBP* ptr_tmp_rgbp = NULL;
			//读取一圈数据
			if (src_reader->readLoop(ptr_tmp_array, i, ptr_tmp_intensity, ptr_tmp_rgbp, true, false, simple_size))
			{
				//过滤处理
				filter_manager->doFilter(&ptr_tmp_array, ptr_tmp_intensity, render_transformation, &m_ptcloud_transformation);

				//统计过滤处理后选中的点个数
				int point_num = ptr_tmp_array.size();
				int dst_pt_num = 0;
				for (int j=0; j<point_num; j++)
				{
					if (ptr_tmp_intensity[j].isSelected())
					{
						dst_pt_num++;
					}
				}

				//过滤处理完成后写入hls文件
				if (dst_pt_num > 0)
				{
					//填充数据
					hdPointXYZIPRGBA* points = new hdPointXYZIPRGBA[dst_pt_num];
					int dst_idx = 0;
					for (int k=0; k<point_num; k++)
					{
						if (ptr_tmp_intensity[k].isSelected())
						{
							hdPointXYZF& pt = *(ptr_tmp_array._Myfirst + k);
							points[dst_idx].x = pt.pos.X;// - hls_header.m_offset_x;
							points[dst_idx].y = pt.pos.Y;// - hls_header.m_offset_y;
							points[dst_idx].z = pt.pos.Z;// - hls_header.m_offset_z;
							points[dst_idx].intensity = ptr_tmp_intensity[k].getIntensity();
							dst_idx++;
						}
					}
					//将数据写入
					hls_writer->writeLoop(points, dst_pt_num);
					//释放内存
					delete [] points;
					points = NULL;
				}
			}
			//释放内存
			ptr_tmp_array.clear();
			if (ptr_tmp_intensity)
			{
				delete [] ptr_tmp_intensity;
				ptr_tmp_intensity =NULL;
			}
			if (ptr_tmp_rgbp)
			{
				delete [] ptr_tmp_rgbp;
				ptr_tmp_rgbp =NULL;
			}
			//返回进度
			if (callBack != NULL)
			{
				callBack(i/(float)loop_count, "");
			}
		}

		//关闭文件
		if (hls_writer)
		{
			hls_writer->close();
			delete hls_writer;
			hls_writer = NULL;
		}
		//返回进度
		if (callBack != NULL)
		{
			callBack(1.0f, "");
		}

		return TRUE;*/
	}
// 用于点云格网抽稀
struct hdPosKey 
{
	//! 格网序号值
	u64 grid_index;

	//! 点云索引
	u32 index;

	//! 点云点到该格网中心的距离
	double distance;

	hdPosKey()
	{
		grid_index = 0;
		index = 0;
		distance = 0.0;
	}

	bool operator< (const hdPosKey& other) const
	{
		if(grid_index == other.grid_index)
		{
			return distance < other.distance;
		}

		return grid_index < other.grid_index;
	}
};

//点坐标数组按照距离范围进行格网抽稀，每个格网中仅保留与格网中心点最近的点
template <class T>
u32 simpleByDistance(
	const hdVector<T>& raw_pts,		// 输入点数组
	f64 distance,					// 简化距离
	hdVector<T>& simple_pts,		// 简化后点数组
	CHdBox3df* pBox)			// 输入点数组空间范围,允许为空
{
	f64 distance_rec = 1.0 / distance;
	u32 raw_pt_num = raw_pts.size();
	f64 min_x = F64_MAX, min_y = F64_MAX, min_z = F64_MAX;
	f64 max_x = F64_MIN, max_y = F64_MIN, max_z = F64_MIN;

	//1.统计完整空间范围
	if (pBox == NULL)
	{
		for(u32 i = 0; i < raw_pt_num; i++)
		{
			const T& pt = *(raw_pts._Myfirst + i);
			min_x = min(pt.x, min_x);
			min_y = min(pt.y, min_y);
			min_z = min(pt.z, min_z);
			max_x = max(pt.x, max_x);
			max_y = max(pt.y, max_y);
			max_z = max(pt.z, max_z);
		}
	}
	else
	{
		min_x = pBox->MinEdge.X;
		min_y = pBox->MinEdge.X;
		min_z = pBox->MinEdge.X;
		max_x = pBox->MaxEdge.X;
		max_y = pBox->MaxEdge.Y;
		max_z = pBox->MaxEdge.Z;
	}

	//2.计算格网尺寸
	u32 x_num = (u32)ceil((max_x - min_x) * distance_rec);
	u32 y_num = (u32)ceil((max_y - min_y) * distance_rec);
	u32 z_num = (u32)ceil((max_z - min_z) * distance_rec);
	u32 x_no = 0, y_no = 0, z_no = 0;
	f64 x_offset = 0, y_offset = 0, z_offset = 0;

	std::vector<hdPosKey> pos_key;
	pos_key.resize(raw_pt_num);
	simple_pts.resize(raw_pt_num);
	//3.计算每个点所落在的格网编号及与格网中心点的距离
	for(u32 i = 0; i < raw_pt_num; i++)
	{
		const T& pt = *(raw_pts._Myfirst + i);
		hdPosKey& key = *(pos_key._Myfirst + i);

		x_no = (u32)floor((pt.x - min_x) * distance_rec);
		y_no = (u32)floor((pt.y - min_y) * distance_rec);
		z_no = (u32)floor((pt.z - min_z) * distance_rec);

		x_offset = pt.x - (x_no + 0.5) * distance;
		y_offset = pt.y - (y_no + 0.5) * distance;
		z_offset = pt.z - (z_no + 0.5) * distance;

		key.grid_index = z_no * x_num * y_num + y_no * x_num + x_no;
		key.distance = fabs(x_offset) + fabs(y_offset) + fabs(z_offset);
		key.index = i;
	}

	//4.按照先格网编号由小到大，后与格网中心点距离由近到远的顺序排列
	std::sort(pos_key.begin(), pos_key.end());

	//5.以排序后第一个点的格网编号为基准，选取每个格网中与格网中心距离最近点
	u64 base_index = pos_key[0].grid_index;
	simple_pts[0] = raw_pts[pos_key[0].index];
	u32 simple_num = 1;
	for(u32 i = 1; i < raw_pt_num; i++)
	{
		const hdPosKey& key = *(pos_key._Myfirst + i);
		//同一个格网，drop掉
		if(key.grid_index == base_index)
			continue;
		//新格网，更新基准格网编号，抽取该点
		base_index = key.grid_index;
		simple_pts[simple_num++] = raw_pts[key.index];
	}

	simple_pts.resize(simple_num);
	return simple_num;

}

bool PointCloud::ApplyFilters(bool bUseAll)
{
	if (m_pFilterManager)
	{
		m_selectCount = 0;
		u32 loopCount = getLoopCount();
		for (int k=0; k<loopCount; k++)
		{
			hdVector< PointXYZIPRGBA >& vLoopPoint = getLoop(k);
			if (bUseAll)
			{
				m_pFilterManager->doFilter(&vLoopPoint, NULL, NULL);
			}
			else
			{
				m_pFilterManager->doFilterFresh(&vLoopPoint, NULL, NULL);
			}

			unsigned int nPointCount = vLoopPoint.size();
			for (unsigned int i = 0; i < nPointCount; i ++)
			{
				if (vLoopPoint[i].isSelected())
				{
					m_selectCount ++;
				}
			}
		}
	}
	return true;
}

}
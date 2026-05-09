#include "stdafx.h"
#include "point_cloud2.h"
#include "..\hdHLSlib\HLSReadOpener.h"

#include "..\hdCommon\hdkdtree.hpp"
#include "..\hdCommon\hdLandMarkTransform.h"
//#include "hdSysSetting.h"
#include "..\hdCommon\eigen.h"

using namespace hdkdtree;

namespace hd
{
//#define SIMPLE_LOAD_COUNT	5000000

	// 定义三维点云kdtree索引
	typedef KDTreeSingleIndexAdaptor<
		L2_Simple_Adaptor<float, PointCloud2 >,
		PointCloud2,
		3 /* dim */
	> hd_kd_tree_t;

		void PointCloud2::buildIndex(bool bFullIndex)
		{
			m_fullIndex = bFullIndex;
			if(m_bChange && m_pIndex)
			{
				delete m_pIndex;
				m_pIndex = NULL;
			}

			if(m_pIndex == NULL)
			{
				m_pIndex = new hd_kd_tree_t(3,*this,KDTreeSingleIndexAdaptorParams(10));
			}
			if(m_bChange)
			{
				hd_kd_tree_t* idx = (hd_kd_tree_t*)m_pIndex;
				idx->buildIndex();
				
			}
			m_bChange = false;
		}

		//! 加载索引
		void PointCloud2::loadIndex(const char* path)
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
		void PointCloud2::saveIndex(const char* path)
		{
			if(m_pIndex)
			{
				FILE* pFile = fopen(path,"w+b");
				hd_kd_tree_t* idx = (hd_kd_tree_t*)m_pIndex;
				idx->saveIndex(pFile);
				fclose(pFile);
			}
		}

		size_t PointCloud2::radiusSearch( const float *query_point,const float radius,std::vector<std::pair<size_t,float>>& IndicesDists, bool sort )
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
		
		void PointCloud2::knnSearch( const float *query_point,int neighbour,size_t* ref_index,float* ref_dist_sqr )
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
		//void PointCloud2::get_point(PointXYZI_D& pt)
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
		//void PointCloud2::get_point(PointXYZIPRGBA& pt)
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

		BOOL PointCloud2::loadHlsFile( const char* hlsFile,void (*loadCallback)(float,const char*) )
		{
			// 最大灰度值序号
			m_maxIntensity = 0;
			m_minIntensity = 99999;
			if (m_hlsReader)
			{
				delete m_hlsReader;
				m_hlsReader = NULL;
			}
			CHLSReadOpener hlsOpener;
			m_hlsReader = hlsOpener.Open(hlsFile);
			if(m_hlsReader == NULL)
				return FALSE;
			// 获取索引
			CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex(loadCallback);

			m_pts.clear();				
			m_header = m_hlsReader->m_header;
			m_simpleHeader = m_header;

			s64 toLoadCount = floor(m_hlsReader->m_header.number_of_point_records * (m_endScale - m_startScale) + 0.5);
			u32 nStartCol = floor(m_startScale * m_hlsReader->m_header.number_of_col + 0.5);
			u32 nEndCol = floor(m_endScale * m_hlsReader->m_header.number_of_col + 0.5);

			s32 simpleLevel = toLoadCount / m_loadSimple;
			s32 iReadLoop = 0;
			s32 loopIndex = nStartCol;
			// 将每圈点数设置为块大小
			//m_pts.s(m_hlsReader->GetCountInLoop());
			u32 loopCount = m_hlsReader->GetLoopCount() / simpleLevel;
			// 设置总点数
			m_pts.resize(loopCount * m_hlsReader->GetCountInLoop());
			for (u32 i = nStartCol;i<nEndCol;i++)
			{
				hdVector<PointXYZIPRGBA>& loopBuf = m_pts.getBlock(iReadLoop);
				if(m_hlsReader->ReadLoop(loopBuf,loopIndex))
				{
					loopIndex+=simpleLevel;
					iReadLoop++;
				}
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
			
			m_bChange = true;
			return TRUE;
		}

		// 判断当前列是否在视窗内
		BOOL PointCloud2::IsLoopInView(
			hdVector<PointXYZIPRGBA>& ptBuf,							// 点数组
			BOOL (*ViewTrans)(const f32&,const f32&,const f32&,s32&,s32&),		// 三维坐标转屏幕坐标回调函数
			s32 srcWidth,									// 视口宽度
			s32 srcHeight,									// 视口高度
			u32 uStart,										// 起始点号
			u32 uStep,										// 采样判断步长
			HRGN srcRgn)										
		{
			u32 ptCount = ptBuf.size();
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
							const PointXYZIPRGBA& pt = *(ptBuf._Myfirst + (i++));
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
				return IsLoopInView(ptBuf,ViewTrans,srcWidth,srcHeight,uStep / 2,uStart == 0? uStep:uStep / 2,srcRgn);
		}

		BOOL PointCloud2::LoadByViewPort( 
			BOOL (*ViewTrans)(const f32&,const f32&,const f32&,s32&,s32&),	/* 三维坐标转屏幕坐标函数指针*/ 
			s32 srcWidth,													/* 视口宽度 */ 
			s32 srcHeight,													/* 视口高度 */ 
			const CHdBox3df& box,											/* 视锥体外边框 */ 
			const bool isOrthogonal,										/* 是否为正射投影*/ 
			HRGN srcRgn /*= NULL*/ )
		{
			if(m_hlsReader == NULL || ViewTrans == NULL)
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
			u32 checkStep = bIsMesh ? 1:20;	
			u32 ptCount;
			// 在视图内点个数
			u64 inViewCount = 0;
			// 在视图内列个数
			u32 inViewColCount = 0;
			// 记录扫描圈是否在范围内的数组
			std::vector<u8> vecInview;
			vecInview.resize(m_hlsReader->m_header.number_of_col);

			s32 srcX = -1,srcY = -1;
			u32 startLoop,endLoop;
			// 每隔checkStep圈进行判断加载
			hdVector<PointXYZIPRGBA> vecPts;
			vecPts.resize(m_hlsReader->GetCountInLoop());

			for (u32 i = checkStep / 2;i < m_hlsReader->m_header.number_of_col;i+=checkStep)
			{
				CLoopIndex& loopIdx = pLoopIdx[i];
				
				if(!box.intersectsWithBox(loopIdx.m_loopIdx.xmin,loopIdx.m_loopIdx.ymin,
					loopIdx.m_loopIdx.zmin,loopIdx.m_loopIdx.xmax,
					loopIdx.m_loopIdx.ymax,loopIdx.m_loopIdx.zmax))
					continue;
				BOOL bInView = TRUE;
				if(!bIsMesh && !isOrthogonal)// 如果是按块存储,不需要判断点在视图内
				{
					if(!m_hlsReader->ReadLoopFull(vecPts,i))
						continue;
					ptCount = vecPts.size();
					// 先判断最后一个点是否在范围内
					const PointXYZIPRGBA& pt = *(vecPts._Myfirst + ptCount -1);
					if (!pt.isValid() || !ViewTrans(pt.x,pt.y,pt.z,srcX,srcY))
					{
						// 判断其他点是否在范围内
						bInView = IsLoopInView(vecPts,ViewTrans,srcWidth,srcHeight,0,ptCount / 2);			
					}
					else
					{
						bInView = TRUE;
					}
				}

				// 如果在范围内,则统计点数
				if (bInView)
				{
					*(vecInview._Myfirst + i) = 1;
					if (checkStep == 1)
					{
						loopIdx = pLoopIdx[i];
						inViewCount += loopIdx.m_loopIdx.count;
						inViewColCount++;
					}
					else
					{
						// 统计相邻loopStep列的点数
						startLoop = i - checkStep / 2;
						endLoop = MIN((i + checkStep / 2),m_hlsReader->m_header.number_of_col);
						for (u32 k = startLoop;k < endLoop;k++)
						{
							loopIdx = pLoopIdx[k];
							inViewCount += loopIdx.m_loopIdx.count;
						}
						inViewColCount += (endLoop - startLoop);
					}
				}
			}
			if(inViewCount ==0)
				return FALSE;
#ifdef _DEBUG
			LARGE_INTEGER tStartR;
			QueryPerformanceCounter(&tStartR);
#endif // _DEBUG
			m_simpleLevel = 1;
			if (inViewCount <= m_loadSimple)
			{
				m_simpleLevel = 1;
			}
			else
			{
				m_simpleLevel = inViewCount / m_loadSimple + 1;
			}

			s32 remedy = 0;
			u32 colPtCount = 0;
			BOOL bChange = FALSE;
			
			// 获取范围内的列号
			m_queryResult.clear();
			m_queryResult.resize(m_hlsReader->m_header.number_of_col);
			
			m_pts.clear();
			//m_pts.setCountInBlock(m_hlsReader->GetCountInLoop());
			u32 colInMemo = inViewColCount / m_simpleLevel;
			m_pts.resize(colInMemo);
			u32 index = 0;
			for (u32 i = 0;i< vecInview.size() && index < colInMemo;i++)
			{
				u8 uIsInView = *(vecInview._Myfirst + i);
				if (uIsInView != 1)
				{			
					continue;
				}

				// 加载相邻loopStep列的点数
				if (checkStep == 1)
				{
					//CLoopIndex& loopIdx = pLoopIdx[i];
					//colPtCount = loopIdx.m_loopIdx.count;
					*(m_queryResult._Myfirst + index) = i;
					if(m_hlsReader->ReadLoopFull(m_pts.getBlock(index),i))
					{
						index++;
					}
				}
				else
				{
					s32 k = 0;
					s32 startLoop = i - checkStep / 2 + remedy;
					s32 endLoop = MIN((i + checkStep / 2),m_hlsReader->m_header.number_of_col);
					for (k = startLoop;k < endLoop;k++)
					{
						//CLoopIndex& loopIdx = pLoopIdx[k];
						//colPtCount = loopIdx.m_loopIdx.count;

						if ((k - startLoop) % m_simpleLevel == 0)
						{
							*(m_queryResult._Myfirst + index) = k;
							if(m_hlsReader->ReadLoopFull(m_pts.getBlock(index),i))
							{
								index++;
							}
						}
					}
					remedy = i + checkStep / 2 - k;
				}
			}
			//*(m_vecLoopSuf._Myfirst + index) = m_count;
			
			m_queryResult.resize(index);
#ifdef _DEBUG
			LARGE_INTEGER tEndR;
			QueryPerformanceCounter(&tEndR);
			char strDbg[256] = {0};
			sprintf(strDbg,"query:%lf\tload:%lf\n",
				(double)(tStartR.QuadPart - tStartQ.QuadPart)/tFreq.QuadPart,
				(double)(tEndR.QuadPart - tStartR.QuadPart)/tFreq.QuadPart);
			OutputDebugString(strDbg);
#endif

			return bChange;
		}

		hd::u32 PointCloud2::QueryByExtent( f32 xmin,f32 ymin,f32 zmin,f32 xmax,f32 ymax,f32 zmax )
		{
			if(m_hlsReader == NULL)
				return 0;
			CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex();
			if (pLoopIdx == NULL || m_hlsReader->m_header.number_of_col <= 0 || m_hlsReader->m_header.number_of_point_records)
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
				*(m_queryResult._Myfirst + index) = i;
				
				index++;
			}
			m_queryResult.resize(index);
			m_curRoop = 0;
			return index;
		}

		//! 查询所有列或者块
		hd::u32 PointCloud2::QueryAll()
		{
			if(m_hlsReader == NULL || !m_hlsReader->IsOpen())
				return 0;
			CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex();
			if (pLoopIdx == NULL || m_hlsReader->m_header.number_of_col <= 0 || m_hlsReader->m_header.number_of_point_records)
			{
				return 0;
			}
			m_queryResult.clear();
			m_queryResult.resize(m_hlsReader->m_header.number_of_col);
			
			u32 i = 0;
			for (i = 0;i<m_hlsReader->m_header.number_of_col;i++)
			{		
				*(m_queryResult._Myfirst + i) = i;
			}

			m_curRoop = 0;
			return m_hlsReader->m_header.number_of_col;
		}

		void PointCloud2::ResetRead()
		{
			m_curRoop = 0;
		}

		//! 读取下一个扫描列,返回是否成功
		BOOL PointCloud2::ReadNext(hdVector<PointXYZIPRGBA>& pPtBuf)
		{
			if(m_hlsReader == NULL || !m_hlsReader->IsOpen())
				return FALSE;
			
			if(m_queryResult.size() == 0 || m_curRoop >= m_queryResult.size())
			{
				return FALSE;
			}
			u32 loopIndex = *(m_queryResult._Myfirst + (m_curRoop++));
			// 返回不包括无效点的当前圈
			return m_hlsReader->ReadLoop(pPtBuf,loopIndex);
		}

		void PointCloud2::GetExtent( f32& xmin,f32& ymin,f32& zmin,f32& xmax,f32& ymax,f32& zmax )
		{
			if(m_queryResult.size() == 0 || m_curRoop >= m_queryResult.size())
			{
				return;
			}
			u32 loopIndex = *(m_queryResult._Myfirst + m_curRoop);
			CLoopIndex* pLoopIdx = m_hlsReader->GetLoopIndex();
			const CLoopIndex& loopIdx = pLoopIdx[m_curRoop];
			xmin = loopIdx.m_loopIdx.xmin;
			ymin = loopIdx.m_loopIdx.ymin;
			zmin = loopIdx.m_loopIdx.zmin;
			xmax = loopIdx.m_loopIdx.xmax;
			ymax = loopIdx.m_loopIdx.ymax;
			zmax = loopIdx.m_loopIdx.zmax;
		}

		void PointCloud2::SelectPoint(HRGN hRgn,
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
					bInView = IsLoopInView(vecPts,ViewTrans,srcWidth,srcHeight,0,ptCount / 2,hRgn);			
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

		BOOL PointCloud2::loadHlsFileHeader(const char* hlsFile)
		{
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

		BOOL PointCloud2::loadHlsData(void (*loadCallback)(float,const char*))
		{
			if (_access(m_hlsReader->GetFilePath(), 04) == 0)
			{
				m_startScale = 0.0;
				m_endScale = 1.0;
				return loadHlsFile(m_hlsReader->GetFilePath(),loadCallback);
			}
			return FALSE;
		}

		//////////////////////////////////////////////////////////////////////////////////////////////
		unsigned int PointCloud2::computeMeanAndCovarianceMatrix (
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
				const PointXYZIPRGBA& point = m_fullIndex ? m_pts[(*iIt)] :
					m_pts[(*(m_selectionIDs._Myfirst + (*iIt)))];;//*(m_pts._Myfirst + (*iIt));

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

		void PointCloud2::solvePlaneParameters(const Eigen::Matrix3f &covariance_matrix,
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

		void PointCloud2::flipNormalTowardsViewpoint (const PointXYZIPRGBA &point, float vp_x, float vp_y, float vp_z,
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

		void PointCloud2::computePointNormal ( const std::vector<size_t> &indices, float &nx, float &ny, float &nz, float &curvature)
		{
			if (computeMeanAndCovarianceMatrix (indices, m_covariance_matrix, m_xyz_centroid) == 0)
			{
				nx = ny = nz = curvature = std::numeric_limits<float>::quiet_NaN ();
				return;
			}

			// Get the plane normal and surface curvature
			solvePlaneParameters (m_covariance_matrix, nx, ny, nz, curvature);
		}

		void PointCloud2::computeNormal(int k,float dist,void (*loadCallback)(float,const char*),bool bFull)
		{
			if (k == 0)// && dist < 0.000001
			{
				return;
			}
			m_fullIndex = bFull;
			if (loadCallback)
			{
				loadCallback(0.0,"构建索引...");
			}
			
			if (m_pIndex == NULL)
			{
				buildIndex(bFull);
			}

			std::vector<size_t> nn_indices (k);
			std::vector<float> nn_dists (k);

			// Iterating over the entire index vector
			int ptcount = bFull? m_pts.count():m_selectCount;
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
					m_pts[*(m_selectionIDs._Myfirst + idx)];//*(m_pts._Myfirst + (*(m_selectionIDs._Myfirst + idx)));
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
					loadCallback(idx / (float)ptcount,"正在计算法向量...");
				}
			}

		}

		bool PointCloud2::isNormalPointCloud() const
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

		void PointCloud2::deleteSelectPoints()
		{
			u64 count = m_pts.count();
			m_validCount = 0;
			for (u64 i = 0; i < count;i++)
			{
				PointXYZIPRGBA& pt = m_pts[i];
				if (pt.isSelected())
				{
					pt.setDeleted();
				}
				if (pt.isValid())
				{
					m_validCount++;
				}
			}
			m_selectionIDs.clear();
			m_selectCount = 0;
		}

		void PointCloud2::setSelectCount( u32 count )
		{
			if (count >= 0 && count <= m_pts.count())
			{
				m_selectCount = count;
				// 获取选择集ID
				m_selectionIDs.resize(m_selectCount);
				u32 ptcount = m_pts.count();
				u32 ptIndex = 0;
				for (u32 i = 0; i < ptcount;i++)
				{
					PointXYZIPRGBA& pt = m_pts[i];//*(m_pts._Myfirst + i);
					if(!pt.isValid())
						continue;
					if (pt.isSelected())
					{
						u32& selection = *(m_selectionIDs._Myfirst + ptIndex);
						selection = i;
						ptIndex++;
					}
				}
			}
			
		}

		BOOL PointCloud2::loadHlsByScale( 
			double startScale, /* 起始比例 0.0-1.0 */ 
			double endScale, /* 终止比例 0.0-1.0 endScale >= startScale */ 
			void (*loadCallback)(float,const char*) /*= NULL*/ )
		{
			m_startScale = startScale;
			m_endScale = endScale;
			return loadHlsFile(m_hlsReader->GetFilePath(),loadCallback);
		}

		// 获取绝对坐标范围
		const void PointCloud2::GetGlobalExtent(double& xmin,double& ymin,double& zmin,double& xmax,double& ymax,double& zmax)
		{
			xmin = m_header.min_x;
			ymin = m_header.min_y;
			zmin = m_header.min_z;

			xmax = m_header.max_x;
			ymax = m_header.max_y;
			zmax = m_header.max_z;

			m_transModel.TranslateExtent(xmin,ymin,zmin,xmax,ymax,zmax);
		}

}
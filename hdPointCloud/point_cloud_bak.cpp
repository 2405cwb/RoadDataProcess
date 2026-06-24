#include "stdafx.h"
#include "point_cloud.h"
#include "..\hdHLSlib\HLSReader.h"
#include "..\hdHLSlib\inc\lasreader.hpp"
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
		L2_Simple_Adaptor<float, PointCloud >,
		PointCloud,
		3 /* dim */
	> hd_kd_tree_t;

		void PointCloud::buildIndex(bool bFullIndex)
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

		BOOL PointCloud::loadLasFile( const char* lasFile,void (*loadCallback)(float,const char*) )
		{
			LASreadOpener lasreadopener;

			lasreadopener.set_merged(FALSE);
			lasreadopener.set_populate_header(FALSE);
			lasreadopener.set_file_name(lasFile);

			while (lasreadopener.active())
			{
				LASreader* lasreader = lasreadopener.open();
				if (lasreader == 0)
				{
					fprintf(stderr, "ERROR: could not open lasreader\n");
					return FALSE;
				}

				m_pts.clear();
				m_header.clean();
				//m_pointCloud.header.clean();
				//获取点数属性，其他属性暂时不用
				m_header.number_of_point_records = lasreader->header.number_of_point_records;				
				m_header.min_x = lasreader->header.min_x;
				m_header.max_x = lasreader->header.max_x;
				m_header.min_y = lasreader->header.min_y;
				m_header.max_y = lasreader->header.max_y;
				m_header.min_z = lasreader->header.min_z;
				m_header.max_z = lasreader->header.max_z;

				try
				{
					m_pts.resize(lasreader->header.number_of_point_records);
				}
				catch (...)
				{
					::MessageBox(NULL,"内存不足","错误",MB_OK);
					m_header.clean();
					m_simpleHeader.clean();
					return FALSE;
				}
				
				m_maxIntensity = 0;
				m_minIntensity = 999999;

				u32 i = 0;
				f32 dist;

				while (lasreader->read_point())
				{
					// 获取坐标
					PointXYZIPRGBA& pt = m_pts[i];//(*(m_pts._Myfirst + i));
					pt.x = lasreader->get_x();
					pt.y = lasreader->get_y();
					pt.z = lasreader->get_z();
					// 获取反射强度
					pt.intensity = lasreader->point.intensity;
					m_maxIntensity = MAX(lasreader->point.intensity,m_maxIntensity);
					if (lasreader->point.intensity != 0)//无效点不参与计算最小反射值
					{
						m_minIntensity = MIN(lasreader->point.intensity,m_minIntensity);
					}
					
					// 获取最大最小距离
					dist = pt.x * pt.x + 
						pt.y * pt.y + pt.z * pt.z;
					m_maxDistance = MAX(dist,m_maxDistance);
					m_minDistance = MIN(dist,m_minDistance);

					// 获取r,g,b
					pt.r = lasreader->point.rgb[0];
					pt.g = lasreader->point.rgb[1];
					pt.b = lasreader->point.rgb[2];
					//pt._unused = lasreader->point.rgb[3];
					//获取属性
					U8 is_last = (lasreader->point.return_number == lasreader->point.number_of_returns_of_given_pulse ? 128 : 0);
					U8 is_first = (lasreader->point.return_number == 1 ? 64 : 0);
					pt.prop = (is_last | is_first | (lasreader->point.classification & 63));

					i++;
					if (pt.isValid())
					{
						m_validCount++;
					}
					if (loadCallback && (i % 10000) == 0)
					{
						loadCallback(i / (float)lasreader->header.number_of_point_records,"正在加载点云...");
					}
				}

				lasreader->close();
				delete lasreader;

				/*if (loadCallback)
				{
					loadCallback(0,"正在构建索引...");
				}*/
				//buildIndex();
			}

			m_bChange = true;
			return TRUE;
		}

		//! 获取当前读取的点,坐标系是全局坐标
		void PointCloud::get_point(PointXYZI_D& pt)
		{
			// 获取原始坐标
			pt.x = m_hlsReader.m_point.x;
			pt.y = m_hlsReader.m_point.y;
			pt.z = m_hlsReader.m_point.z;
			// 获取反射强度
			pt.intensity = m_hlsReader.m_point.intensity;
			// 转换全局坐标
			if (pt.isValid())
			{
				double m[16];
				m_header.computeMatrix(m);
				hdHomogeneousTransformPoint(m,pt.x,pt.y,pt.z);
			}
		}
		//! 获取当前读取的点,坐标系是本地坐标
		void PointCloud::get_point(PointXYZIPRGBA& pt)
		{
			pt.x = m_hlsReader.m_point.x;
			pt.y = m_hlsReader.m_point.y;
			pt.z = m_hlsReader.m_point.z;
			// 获取反射强度
			pt.intensity = m_hlsReader.m_point.intensity;
			// 获取最大反射强度
			m_maxIntensity = MAX(m_hlsReader.m_point.intensity,m_maxIntensity);
			if (m_hlsReader.m_point.intensity != 0)//无效点不参与计算最小反射值
			{
				m_minIntensity = MIN(m_hlsReader.m_point.intensity,m_minIntensity);
			}
			if (pt.isValid())
			{
				m_validCount++;
			}
			// 获取最大最小距离
			float dist = m_hlsReader.m_point.x * m_hlsReader.m_point.x + 
				m_hlsReader.m_point.y * m_hlsReader.m_point.y + 
				m_hlsReader.m_point.z * m_hlsReader.m_point.z;
			m_maxDistance = MAX(dist,m_maxDistance);
			m_minDistance = MIN(dist,m_minDistance);

			// 获取r,g,b
			pt.r = m_hlsReader.m_point.rgb[0];
			pt.g = m_hlsReader.m_point.rgb[1];
			pt.b = m_hlsReader.m_point.rgb[2];
			//pt._unused = m_hlsReader.m_point.rgb[3];
			//获取属性
			U8 is_last = 0;//(hlsReader.m_point.return_number == hlsReader.m_point.number_of_returns_of_given_pulse ? 128 : 0);
			U8 is_first = 0;//(hlsReader.m_point.return_number == 1 ? 64 : 0);			
			pt.prop = is_last | is_first | (m_hlsReader.m_point.classification & 63);

		}

		BOOL PointCloud::loadHlsFile( const char* hlsFile,void (*loadCallback)(float,const char*) )
		{
			// 最大灰度值序号
			m_maxIntensity = 0;
			m_minIntensity = 99999;
			
			if(m_hlsReader.open(hlsFile))
			{		
				m_pts.clear();
				
				m_header = m_hlsReader.m_header;
				m_simpleHeader = m_header;

				s64 toLoadCount = floor(m_hlsReader.m_npoints * (m_endScale - m_startScale) + 0.5);
				bool bSimple = toLoadCount > m_loadSimple;
				int simpleCount = toLoadCount;
				// 计算抽稀步长
				f32 colStep = 1.0f,rowStep = 1.0f;	//行列顺序的点云,按行列抽稀
				f32 step = 1.0f;					//无序点云,按序号抽稀
				if (bSimple && m_header.number_of_point_records == m_header.number_of_col * m_header.number_of_row)
				{
					I32 colI = 0;
					I32 rowI = 0;
					f32 step = 2.0;
					while(true)
					{
						int maxRowCol = MAX(m_hlsReader.m_header.number_of_col,m_hlsReader.m_header.number_of_row);
						if (maxRowCol == m_hlsReader.m_header.number_of_col)
						{
							colStep = step;
							f32 colF = m_hlsReader.m_header.number_of_col / (colStep);
							colI = I32_CEIL(colF);

							rowStep = colStep / 2.0;
							f32 rowF = m_hlsReader.m_header.number_of_row / rowStep;
							rowI = I32_CEIL(rowF);
						}
						else
						{
							rowStep = step;
							f32 rowF = m_hlsReader.m_header.number_of_row / rowStep;
							rowI = I32_CEIL(rowF);

							colStep = rowStep / 2.0;
							f32 colF = m_hlsReader.m_header.number_of_col / (colStep);
							colI = I32_CEIL(colF);
						}					

						simpleCount = (rowI) * (colI);
						if (simpleCount <= m_loadSimple)
						{
							m_simpleHeader.number_of_point_records = simpleCount;
							m_simpleHeader.number_of_row = (u32)rowI;
							m_simpleHeader.number_of_col = (u32)colI;
							break;
						}
						step += 1.0;
					}
				}
				else if (bSimple && m_header.number_of_point_records != m_header.number_of_col * m_header.number_of_row)
				{
					simpleCount = m_loadSimple;
					step = (F64)m_header.number_of_point_records / (F64)simpleCount;
				}

				try
				{
					m_pts.resize(simpleCount);
				}
				catch (...)
				{
					MessageBox(NULL,"内存不足","错误",MB_OK);
					m_header.clean();
					m_simpleHeader.clean();
					return FALSE;
				}

				u32 i = 0;
				f32 fIndex = 0.0f;
				u32 iCol = 0;
				u32 iRow = 0;
				f32 fCol = 0.0f;
				f32 fRow = 0.0f;
				BOOL bRead = TRUE;
				PointXYZIPRGBA lastPt;

				if (m_header.number_of_point_records == m_header.number_of_col * m_header.number_of_row)
				{//按行列顺序储存的点云

					u32 nStart = floor(m_startScale * m_hlsReader.m_npoints + 0.5);
					u32 nEnd = floor(m_endScale * m_hlsReader.m_npoints + 0.5);
					//for (u32 n = 0;n<m_hlsReader.m_npoints;)
					for (u32 n = nStart;n<nEnd;)
					{
						// 从文件读取坐标,n是文件中的点序号
						if (m_header.get_pointformat() <= HLS_POINTFORMAT_RHVI)
						{
							bRead = m_hlsReader.read_fast(n);
						}
						else
						{
							bRead = m_hlsReader.read_point(n);
						}
						if(!bRead)
							break;
						// 获取点坐标,i是内存m_pts数组序号
						if (i >= simpleCount)
						{
							break;
						}
						//get_point(*(m_pts._Myfirst + i));
						//if (m_hlsReader.m_point.x == 0.0f && m_hlsReader.m_point.y == 0.0f && m_hlsReader.m_point.z == 0.0f)
						//{
						//	fRow += 1.0f;
						//}
						//else
						{
							get_point(m_pts[i]);
							i++;
							fRow += rowStep;
						}
						iRow = (u32)fRow;
						if (iRow >= m_hlsReader.m_header.number_of_row)
						{
							fCol += colStep;
							iCol = (u32)fCol;

							fRow = 0.0;
							iRow = (u32)fRow;
						}
						// 计算读取点的序号
						n = iCol * m_hlsReader.m_header.number_of_row + iRow;

						if (loadCallback && (n % 10000) == 0)
						{
							loadCallback(i / (float)simpleCount,"正在加载点云...");
						}
					}
				}
				else//无序点云
				{
					F64 nF = 0;
					for (u32 n = 0;n<m_hlsReader.m_npoints;)
					{
						// 从文件读取坐标,n是文件中的点序号
						bRead = m_hlsReader.read_point(n);
						if(!bRead)
							break;
						// 获取点坐标,i是内存m_pts数组序号
						if (i >= simpleCount)
						{
							break;
						}
						//get_point(*(m_pts._Myfirst + i));
						if (m_hlsReader.m_point.x == 0.0f && m_hlsReader.m_point.y == 0.0f && m_hlsReader.m_point.z == 0.0f)
						{
							nF += 1.0;
						}
						else
						{
							get_point(m_pts[i]);
							i++;
							nF += step;
						}
						n = nF;

						if (loadCallback && (n % 10000) == 0)
						{
							loadCallback(i / (float)simpleCount,"正在加载点云...");
						}
					}
				}
				m_pts.resize(i);
				// 如果是距离角度记录,需要重新赋值,因为内部重新统计了范围
				if (m_hlsReader.m_header.get_pointformat() == HLS_POINTFORMAT_RHVI)
				{
					m_header = m_hlsReader.m_header;
					m_simpleHeader.max_x = m_header.max_x;
					m_simpleHeader.min_x = m_header.min_x;
					m_simpleHeader.max_y = m_header.max_y;
					m_simpleHeader.min_y = m_header.min_y;
					m_simpleHeader.max_z = m_header.max_z;
					m_simpleHeader.min_z = m_header.min_z;
				}
				/*if (loadCallback)
				{
					loadCallback(0,"正在构建索引...");
				}*/
				//buildIndex();
				//computeNormal(100,0.0f);

				if (loadCallback)
				{
					loadCallback(0,"完成");
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
			}
			m_bChange = true;
			return TRUE;
		}

		BOOL PointCloud::loadHlsFileHeader(const char* hlsFile)
		{
			if(m_hlsReader.open(hlsFile))
			{		
				m_header = m_hlsReader.m_header;
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

			return FALSE;
		}

		BOOL PointCloud::loadHlsData(void (*loadCallback)(float,const char*))
		{
			if (_access(m_hlsReader.m_filepath, 04) == 0)
			{
				m_startScale = 0.0;
				m_endScale = 1.0;
				return loadHlsFile(m_hlsReader.m_filepath,loadCallback);
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
			u32 count = m_pts.count();
			m_validCount = 0;
			for (u32 i = 0; i < count;i++)
			{
				PointXYZIPRGBA& pt = m_pts[i];//*(m_pts._Myfirst + i);
				if (pt.isSelected())
				{
					//pt.select = 4;//(pt.select | 0x4);		// 标记删除 二进制 00000100
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

		void PointCloud::setSelectCount( u32 count )
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

		BOOL PointCloud::loadHlsByScale( 
			double startScale, /* 起始比例 0.0-1.0 */ 
			double endScale, /* 终止比例 0.0-1.0 endScale >= startScale */ 
			void (*loadCallback)(float,const char*) /*= NULL*/ )
		{
			m_startScale = startScale;
			m_endScale = endScale;
			return loadHlsFile(m_hlsReader.m_filepath,loadCallback);
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

}
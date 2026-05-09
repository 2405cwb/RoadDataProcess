/*! @PanoTileSphere
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : PanoTileSphere.h
相关文件     : PanoTileSphere.cpp, CPanoSceneNode
文件实现功能 : 全景切片显示球控制类
作者         : 软件部，朱旭波
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2016/05/03   1.0      朱旭波              新增加内容
</PRE>
*******************************************************************************/
#include "StdAfx.h"
#include "PanoTileSphere.h"

#include "..\hd3DEngine\include\IImageWriter.h"
#include "..\hd3DEngine\CImageWriterJPG.h"

// 定义水平、垂直分割个数宏,控制为2的n次方,水平需为垂直2倍
#define HORI_POLY_COUNT 128
#define VERT_POLY_COUNT 64

namespace hd
{
	namespace scene
	{
		// 构造
		CPanoTileSphere::CPanoTileSphere(IHdView* pView,int nHoriCount,int nVertCount,float radius)
		{
			// 视图传入，方便操作纹理
			m_pView = pView;
			m_nHoriCount = nHoriCount;
			m_nVertCount = nVertCount;
			Buffer_0 = NULL;
			m_vecBuffer.clear();

			// 赋值
			m_Radius = radius;

			// 为0级，则采用原方式构建
			if (nHoriCount * nVertCount == 1)
			{
				// new一个buffer
				Buffer_0 = new SMeshBuffer();
				Buffer_0->Material.Lighting = false;
				Buffer_0->Material.ZBuffer = video::ECFN_NEVER; 
				Buffer_0->Material.ZWriteEnable = false;
				Buffer_0->Material.AntiAliasing = video::EAAM_OFF;
				Buffer_0->Material.setTexture(0, NULL);
				Buffer_0->BoundingBox.MaxEdge.set(radius,radius,radius);
				Buffer_0->BoundingBox.MinEdge.set(-radius,-radius,-radius);

				// 构建纹理
				GenerateZero();
			}
			else
			{
				m_vecInView.resize(m_nHoriCount * m_nVertCount);

				// mat材质设置
				video::SMaterial mat;
				mat.Lighting = false;
				mat.ZBuffer = video::ECFN_NEVER;
				mat.ZWriteEnable = false;
				mat.AntiAliasing = 0;
				mat.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
				mat.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;

				// 全景角度固定，水平0~360，垂直90~-90，不需考虑地面站全景数据信息
				f32 HoriStartAngle, HoriEndAngle, VertStartAngle, VertEndAngle;
				HoriStartAngle = 0.0;
				HoriEndAngle = 360.0f * core::PI /180.0f;
				VertStartAngle = 90.0f * core::PI /180.0f;
				VertEndAngle = -90.0f * core::PI /180.0f;

				// 总体步长变化
				const f32 azimuth_step = (HoriEndAngle - HoriStartAngle) / nHoriCount;
				const f32 elevation_step = (VertStartAngle - VertEndAngle)/ (f32)nVertCount;
				f32 azimuth = 0.0f;

				// 记录单个切片内部水平、垂直步长变化
				const f32 fHoriPoly_step = (HoriEndAngle - HoriStartAngle) / HORI_POLY_COUNT;
				const f32 fVertPoly_step = (VertStartAngle - VertEndAngle) / VERT_POLY_COUNT;

				// 单个切片线长度
				int nTileHoriCount = HORI_POLY_COUNT / nHoriCount;
				int nTileVertCount = VERT_POLY_COUNT / nVertCount;

				// 顶点中间变量
				video::S3DVertex vtx;
				vtx.Color.set(255,255,255,255);
				vtx.Normal.set(0.0f,-1.f,0.0f);

				// 计算纹理坐标
				const f32 tcV = 1.0f / nVertCount;
				azimuth = HoriStartAngle;
				for (int nHori = 0;nHori < nHoriCount;nHori++) // 水平
				{
					// 切片水平起始角度
					f32 elevation = VertStartAngle;
					//const f32 tcU = 1.0f - (f32)nHori / (f32)nHoriCount;
					//const f32 sinA = sinf(azimuth);
					//const f32 cosA = cosf(azimuth);

					//// 切片水平终止角度
					//const f32 tcU_1 = 1.0f - (f32)(nHori+1) / (f32)nHoriCount;
					//const f32 sinA_1 = sinf(azimuth + azimuth_step);
					//const f32 cosA_1 = cosf(azimuth + azimuth_step);

					// 垂直
					for (int nVert = 0;nVert < nVertCount;nVert++)
					{
						// new mesh，并存储
						SMeshBuffer* pBuffer = new SMeshBuffer();
						m_vecBuffer.push_back(pBuffer);

						// 设置材质属性，new相关顶点、索引内存
						int nBufIndex = nHori * nVertCount + nVert;
						pBuffer->Material = mat;
						pBuffer->Vertices.clear();
						pBuffer->Indices.clear();
						pBuffer->Vertices.reallocate( (nTileHoriCount + 1) * (nTileVertCount + 1) );
						pBuffer->Indices.reallocate(3 * 2 * nTileVertCount * nTileHoriCount);

						// 遍历逐顶点赋值
						for (int nH = 0;nH <= nTileHoriCount;nH++)
						{
							// 中间变量
							float fTileAzimuth = azimuth + nH * fHoriPoly_step;
							const f32 sinA = sinf(fTileAzimuth);
							const f32 cosA = cosf(fTileAzimuth);

							// 水平纹理坐标计算
							float fTCoordX = 1.0f * nH / nTileHoriCount;

							// 遍历处理
							for (int nV = 0;nV <= nTileVertCount;nV++)
							{
								// 中间变量
								float fTileElevation = elevation - nV * fVertPoly_step;
								const f32 cosEr = m_Radius * cosf(fTileElevation);
								const f32 sinEr = m_Radius * sinf(fTileElevation);

								// 垂直纹理坐标计算
								float fTCoordY = 1.0f * nV / nTileVertCount;

								// 水平、垂直角度量计算
								vtx.Pos.set(cosEr*cosA, -cosEr*sinA ,sinEr); // 由于点云计算角度时Y坐标取负值，此处需要统一坐标系
								vtx.TCoords.set(fTCoordX, fTCoordY); // 纹理坐标也做更改
								vtx.Color = video::SColor(255,255,255,255);
								vtx.Normal = -vtx.Pos;
								vtx.Normal.normalize();

								// 增加顶点
								pBuffer->Vertices.push_back(vtx);
							}
						}

						// 顶点增加完成后，构建三角网索引
						for (int nH = 0;nH < nTileHoriCount;nH++)
						{
							for (int nV = 0;nV < nTileVertCount;nV++)
							{
								// 中间变量定义
								int veticeIndex_0;
								int veticeIndex_1;
								int veticeIndex_2;
								int veticeIndex_3;

								// 索引值计算
								veticeIndex_0 = nH * (nTileVertCount + 1) + nV;
								veticeIndex_1 = (nH + 1) * (nTileVertCount + 1) + nV;
								veticeIndex_2 = (nH + 1) * (nTileVertCount + 1) + nV + 1;
								veticeIndex_3 = nH * (nTileVertCount + 1)+ nV + 1;

								// 增加索引值
								pBuffer->Indices.push_back(veticeIndex_0);
								pBuffer->Indices.push_back(veticeIndex_1);
								pBuffer->Indices.push_back(veticeIndex_2);

								// 增加索引值
								pBuffer->Indices.push_back(veticeIndex_2);
								pBuffer->Indices.push_back(veticeIndex_3);
								pBuffer->Indices.push_back(veticeIndex_0);
							}
						}

						////m_Material[nHori * nVertCount + nVert] = mat;

						//// 计算

						////    1------2
						////   /      /
						////  /      /
						//// 3------4
						//// 四个顶点赋值-1
						//const f32 cosEr = m_Radius * cosf(elevation);
						//const f32 sinEr = m_Radius * sinf(elevation);
						//int nVerticeIndex = (nHori * nVertCount + nVert) * 4;

						//m_Vertices[nVerticeIndex].Pos.set(cosEr*cosA, -cosEr*sinA ,sinEr); // 由于点云计算角度时Y坐标取负值，此处需要统一坐标系
						////m_Vertices[nVerticeIndex].TCoords.set(0.0, 0.0); // 纹理坐标也做更改
						//m_Vertices[nVerticeIndex].TCoords.set(o, o); // 纹理坐标也做更改
						//m_Vertices[nVerticeIndex].Color = video::SColor(255,255,255,255);
						////m_Vertices[nVerticeIndex].Normal = -m_Vertices[nVerticeIndex].Pos;
						//m_Vertices[nVerticeIndex].Normal = -m_Vertices[nVerticeIndex].Pos;
						//m_Vertices[nVerticeIndex].Normal.normalize();

						//// 四个顶点赋值-2
						//m_Vertices[nVerticeIndex + 1].Pos.set(cosEr*cosA_1, -cosEr*sinA_1 ,sinEr); // 由于点云计算角度时Y坐标取负值，此处需要统一坐标系
						////m_Vertices[nVerticeIndex + 1].TCoords.set(1.0, 0.0); // 纹理坐标也做更改
						//m_Vertices[nVerticeIndex + 1].TCoords.set(t, o); // 纹理坐标也做更改
						//m_Vertices[nVerticeIndex + 1].Color = video::SColor(255,255,255,255);
						////m_Vertices[nVerticeIndex + 1].Normal = -m_Vertices[nVerticeIndex + 1].Pos;
						//m_Vertices[nVerticeIndex + 1].Normal = -m_Vertices[nVerticeIndex + 1].Pos;
						//m_Vertices[nVerticeIndex + 1].Normal.normalize();

						//// 中间参数计算
						//const f32 cosEr_1 = m_Radius * cosf(elevation - elevation_step);
						//const f32 sinEr_1 = m_Radius * sinf(elevation - elevation_step);

						//// 四个顶点赋值-3
						//m_Vertices[nVerticeIndex + 2].Pos.set(cosEr_1*cosA_1, -cosEr_1*sinA_1 ,sinEr_1); // 由于点云计算角度时Y坐标取负值，此处需要统一坐标系
						////m_Vertices[nVerticeIndex + 2].TCoords.set(1.0, 1.0); // 纹理坐标也做更改
						//m_Vertices[nVerticeIndex + 2].TCoords.set(t, t); // 纹理坐标也做更改
						//m_Vertices[nVerticeIndex + 2].Color = video::SColor(255,255,255,255);
						////m_Vertices[nVerticeIndex + 2].Normal = -m_Vertices[nVerticeIndex + 2].Pos;
						//m_Vertices[nVerticeIndex + 2].Normal = -m_Vertices[nVerticeIndex + 2].Pos;
						//m_Vertices[nVerticeIndex + 2].Normal.normalize();

						//// 四个顶点赋值-4
						//m_Vertices[nVerticeIndex + 3].Pos.set(cosEr_1*cosA, -cosEr_1*sinA ,sinEr_1); // 由于点云计算角度时Y坐标取负值，此处需要统一坐标系
						////m_Vertices[nVerticeIndex + 3].TCoords.set(0.0, 1.0); // 纹理坐标也做更改
						//m_Vertices[nVerticeIndex + 3].TCoords.set(o, t); // 纹理坐标也做更改
						//m_Vertices[nVerticeIndex + 3].Color = video::SColor(255,255,255,255);
						////m_Vertices[nVerticeIndex + 3].Normal = -m_Vertices[nVerticeIndex + 3].Pos;
						//m_Vertices[nVerticeIndex + 3].Normal = -m_Vertices[nVerticeIndex + 3].Pos;
						//m_Vertices[nVerticeIndex + 3].Normal.normalize();

						elevation -= elevation_step;

					}

					azimuth += azimuth_step;
				}
			}
		}

		// 析构
		CPanoTileSphere::~CPanoTileSphere(void)
		{
			// 视图存在
			if (m_pView)
			{
				ISceneView* pSceneView = (ISceneView*)(m_pView);
				if (pSceneView)
				{
					// 纹理清除，内存释放
					if (Buffer_0)
					{
						pSceneView->GetSceneManager()->getVideoDriver()->removeTexture(Buffer_0->Material.getTexture(0));
						//pSceneView->GetSceneManager()->getVideoDriver()->removeHardwareBuffer(Buffer_0);
						Buffer_0->drop();
						Buffer_0 = NULL;
					}

					// 纹理清除，内存释放;
					for (unsigned int n = 0;n < m_vecBuffer.size();n++)
					{
						// 获得mesh;
						SMeshBuffer*& pMeshBuf = m_vecBuffer.at(n);
						if (!pMeshBuf)
						{
							continue;
						}

						// 纹理清除
						pSceneView->GetSceneManager()->getVideoDriver()->removeTexture(pMeshBuf->Material.getTexture(0));
						//pSceneView->GetSceneManager()->getVideoDriver()->removeHardwareBuffer(pMeshBuf);
						pMeshBuf->drop();
						pMeshBuf = NULL;
					}

					m_vecBuffer.clear();
				}
			}

		}

		// 清除所有材质纹理
		void CPanoTileSphere::clearAllTextures()
		{
			ISceneView* pSceneView = (ISceneView*)(m_pView);
			if (!pSceneView)
			{
				return;
			}

			int nMaterialCount = m_nHoriCount * m_nVertCount;
			if (nMaterialCount == 1)
			{
				// 获取纹理
				ITexture* pCurText = Buffer_0->Material.getTexture(0);
				if (pCurText)
				{
					// 移除之前的纹理，设置为空纹理
					pSceneView->GetSceneManager()->getVideoDriver()->removeTexture(pCurText);
					Buffer_0->Material.setTexture(0,NULL);
				}
			}
			else
			{
				// 遍历清除纹理，纹理坐标等信息不做更改
				for (int m = 0;m < m_vecBuffer.size();m++)
				{
					// 获得mesh
					SMeshBuffer*& pMeshBuf = m_vecBuffer.at(m);
					if (!pMeshBuf)
					{
						continue;
					}

					// 获取纹理
					ITexture* pCurText = pMeshBuf->Material.getTexture(0);
					if (pCurText)
					{
						// 移除之前的纹理，设置为空纹理
						pSceneView->GetSceneManager()->getVideoDriver()->removeTexture(pCurText);
						pMeshBuf->Material.setTexture(0,NULL);
					}
				}
			}
		}

		// 0级球构建纹理
		void CPanoTileSphere::GenerateZero()
		{
			// 全景角度固定，水平0~360，垂直90~-90，不需考虑地面站全景数据信息
			f32 HoriStartAngle, HoriEndAngle, VertStartAngle, VertEndAngle;
			HoriStartAngle = 0.0;
			HoriEndAngle = 360.0f * core::PI /180.0f;
			VertStartAngle = 90.0f * core::PI /180.0f;
			VertEndAngle = -90.0f * core::PI /180.0f;

			u32 HorizontalResolution = (u32)floor(360.0f / 5.0f);
			u32 VerticalResolution = (u32)(floor( 180.0f / 5.0f));

			f32 azimuth;
			u32 k;

			Buffer_0->Vertices.clear();
			Buffer_0->Indices.clear();

			// 设置buffer材质 半径较小时，当前相机为3D camera，设置Zbuffer 打开
			//				  半径较大时，当前相机为quick camera，设置Zbuffer 关闭 [2013/08/05 危迟] 
			if (m_Radius == 2.f)
			{
				Buffer_0->Material.setFlag(video::EMF_ZBUFFER, true);
			}
			else
			{
				Buffer_0->Material.setFlag(video::EMF_ZBUFFER, false);
			}

			const f32 azimuth_step = (HoriEndAngle - HoriStartAngle) / HorizontalResolution;
			const f32 elevation_step = (VertStartAngle - VertEndAngle)/ (f32)VerticalResolution;

			Buffer_0->Vertices.reallocate( (HorizontalResolution + 1) * (VerticalResolution + 1) );
			Buffer_0->Indices.reallocate(3 * (2*VerticalResolution - 1) * HorizontalResolution);

			video::S3DVertex vtx;
			vtx.Color.set(255,255,255,255);
			vtx.Normal.set(0.0f,-1.f,0.0f);

			const f32 tcV = 1.0f / VerticalResolution;
			for (k = 0, azimuth = HoriStartAngle; k <= HorizontalResolution; ++k)
			{
				f32 elevation = VertStartAngle;
				const f32 tcU = 1.0f - (f32)k / (f32)HorizontalResolution;
				const f32 sinA = sinf(azimuth);
				const f32 cosA = cosf(azimuth);
				for (u32 j = 0; j <= VerticalResolution; ++j)
				{
					const f32 cosEr = m_Radius * cosf(elevation);
					//vtx.Pos.set(cosEr*sinA, Radius*sinf(elevation), cosEr*cosA);
					vtx.Pos.set(cosEr*cosA, -cosEr*sinA ,m_Radius*sinf(elevation)); // 由于点云计算角度时Y坐标取负值，此处需要统一坐标系
					vtx.TCoords.set(1 - tcU, j*tcV); // 纹理坐标也做更改

					vtx.Normal = -vtx.Pos;
					vtx.Normal.normalize();

					Buffer_0->Vertices.push_back(vtx);
					elevation -= elevation_step;
				}
				azimuth += azimuth_step;
			}

			for (k = 0; k < HorizontalResolution; ++k)
			{
				// 设置三角索引时，根据坐标系方向，右手系时设置为顺时针方向纹理显示正确
				Buffer_0->Indices.push_back(1 + (VerticalResolution + 1)*k);
				Buffer_0->Indices.push_back(0 + (VerticalResolution + 1)*k);
				Buffer_0->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k);

				for (u32 j = 1; j < VerticalResolution; ++j)
				{
					Buffer_0->Indices.push_back(1 + (VerticalResolution + 1)*k + j);
					Buffer_0->Indices.push_back(0 + (VerticalResolution + 1)*k + j);
					Buffer_0->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k + j);

					Buffer_0->Indices.push_back(0 + (VerticalResolution + 1)*k + j);
					Buffer_0->Indices.push_back(VerticalResolution + 1 + (VerticalResolution + 1)*k + j);
					Buffer_0->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k + j);
				}
			}

			////Buffer->setHardwareMappingHint(EHM_NEVER,EBT_VERTEX_AND_INDEX);

			//// 设置硬件映射模式为dynamic [2013/07/31 危迟]
			//pMshBuf->setHardwareMappingHint(irr::scene::EHM_DYNAMIC);
			//// 标记buffer的顶点及顶点索引已经改变，更新硬件中的buffer [2013/07/31 危迟] 
			//pMshBuf->setDirty(EBT_VERTEX_AND_INDEX);
			//重新计算boundingBox
			Buffer_0->recalculateBoundingBox();
		}

		// 设置0级纹理
		void CPanoTileSphere::SetZeroTexture( CHdSvTileInfoBuffer* pZeroInfo )
		{
			// 条件判断
			if (!pZeroInfo)
			{
				return;
			}

			// 获得切片
			HD_SV_TILEINFO* pInfo = pZeroInfo->GetTileInfo();

			// 切片二进制数据存在则添加纹理
			if (!pInfo || pZeroInfo->GetSize() <= 0)
			{
				return;
			}

			// 视图判断
			if (!m_pView)
			{
				return;
			}

			// 视图获得
			ISceneView* pSceneView = (ISceneView*)(m_pView);
			if (!pSceneView)
			{
				return;
			}

			// 若纹理已存在，则不作处理
			ITexture* pOldTexture = Buffer_0->Material.getTexture(0);
			if (pOldTexture)
			{
				return;
			}

			// 构建纹理数据
			irr::io::IReadFile* pReadFile = irr::io::createMemoryReadFile(pInfo->pTileData,
				pInfo->nSize,pInfo->strTileID,false);
			IVideoDriver* driver = pSceneView->GetSceneManager()->getVideoDriver();
			ITexture* pTileTexture = driver->getTexture(pReadFile);

			// 设置纹理
			if (pTileTexture)
			{
				Buffer_0->Material.setTexture(0,pTileTexture);
			}
			
			// drop
			pReadFile->drop();
		}

		// 设置球体所在位置
		void CPanoTileSphere::SetPanoSnPosition( irr::core::vector3df pos )
		{
			m_panoPosition = pos;
		}

		// 由角度获得球体三维坐标，传入参数为度
		void CPanoTileSphere::GetSpherePos( const core::vector2df angle, core::vector3df& spherePos )
		{
			f32 cosR = m_Radius*cosf(angle.Y*core::DEGTORAD);
			spherePos.X = cosR*cosf(angle.X*core::DEGTORAD);
			spherePos.Y = cosR*sinf(angle.X*core::DEGTORAD);
			spherePos.Y = -spherePos.Y;
			spherePos.Z = m_Radius*sinf(angle.Y*core::DEGTORAD);
		}

		// 由水平、垂直角度获得屏幕坐标(参照PANO SN实现)
		void CPanoTileSphere::GetScreenPosByAngle( const core::vector2df angle,core::position2di& screenPos )
		{
			// 视图判断
			if (!m_pView)
			{
				return;
			}

			// 转换
			ISceneView* pSceneView = (ISceneView*)(m_pView);
			if (!pSceneView)
			{
				return;
			}

			// 记录三维坐标
			core::vector3df pos;

			// 由角度计算三维坐标,传入坐标为度
			GetSpherePos(angle, pos);

			// 存在hpr旋转，应获得sn的相对旋转矩阵
			core::matrix4 m = m_ralativeMatrix;

			double x = pos.X * m(0,0) + pos.Y * m(1,0) + pos.Z * m(2,0);
			double y = pos.X * m(0,1) + pos.Y * m(1,1) + pos.Z * m(2,1);
			double z = pos.X * m(0,2) + pos.Y * m(1,2) + pos.Z * m(2,2);

			// 获得三维视图坐标
			pos.X = x + m_panoPosition.X;
			pos.Y = y + m_panoPosition.Y;
			pos.Z = z + m_panoPosition.Z;

			// 由视图获得屏幕坐标
			screenPos = pSceneView->GetSceneManager()->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(pos);
		}

		// 设置相对位置矩阵
		void CPanoTileSphere::SetPanoSnMatrix( core::matrix4 m )
		{
			m_ralativeMatrix = m;
		}

		// 计算当前视口范围内切片
		vector<int>& CPanoTileSphere::CalInView()
		{
			// 视图判断
			if (!m_pView)
			{
				return m_vecInView;
			}

			//// 测试代码
			//for (int nHori = 0;nHori < m_nHoriCount;nHori++)
			//{
			//	// 垂直
			//	for (int nVert = 0;nVert < m_nVertCount;nVert++)
			//	{
			//		m_vecInView[nHori * m_nVertCount + nVert] = 1;
			//	}
			//}

			//return m_vecInView;

			// 获得视图
			ISceneView* pSceneView = (ISceneView*)(m_pView);

			// 获得屏幕大小(像素值)
			video::IVideoDriver* driver = pSceneView->GetSceneManager()->getVideoDriver();
			core::dimension2di size = (core::dimension2di)driver->getCurrentRenderTargetSize();

			// 判断点是否在屏幕范围内
			irr::core::recti rect;
			rect.UpperLeftCorner.set(0,0);
			rect.LowerRightCorner.set(size.Width,size.Height);

			// 全景角度固定，水平0~360，垂直90~-90，不需考虑地面站全景数据信息
			float HoriStartAngle, HoriEndAngle, VertStartAngle, VertEndAngle;
			HoriStartAngle = 0.0;
			HoriEndAngle = 360.0f;
			VertStartAngle = 90.0f;
			VertEndAngle = -90.0f;

			// 角度步长
			float fHoriStep = (HoriEndAngle - HoriStartAngle) / m_nHoriCount;
			float fVertStep = (VertStartAngle - VertEndAngle) / m_nVertCount;

			// 计算中间变量
			float fAngleX = 0.0f;
			float fTmpX = 0.0f;
			float fAngleY = 0.0f;
			float fTmpY = 0.0f;
			

			// 计算
			for (int nHori = 0;nHori < m_nHoriCount;nHori++)
			{
				// 垂直
				for (int nVert = 0;nVert < m_nVertCount;nVert++)
				{
					// 四个角点进行计算-0
					fAngleX = nHori * fHoriStep;
					fAngleY = VertStartAngle - nVert * fVertStep;

					// 设置等分段
					float fStepX = fHoriStep / 6;
					float fStepY = fVertStep / 6;

					// 标记是否在范围内
					bool bIn = false;

					// 遍历处理
					for (int w = 0;w <= 6;w++)
					{
						// 只要存在部分在视图内，则跳出循环
						if (bIn)
						{
							break;
						}

						float tmpW = w * fStepX;
						for (int h = 0;h <= 6;h++)
						{
							float tmpH = h * fStepY;

							// 角度值
							fTmpX = fAngleX + tmpW;
							fTmpY = fAngleY - tmpH;

							// 计算中间变量
							core::vector2df angle;
							core::position2di screenPos;

							// 计算-1
							angle.set(fTmpX,fTmpY);
							GetScreenPosByAngle(angle,screenPos);
							bIn = rect.isPointInside(screenPos);
							if (bIn) // 在视图范围内则不再继续进行计算
							{
								break;
								//m_vecInView[nHori * m_nVertCount + nVert] = 1;
								//continue;
							}
						}
					}

					// 根据是否在视口范围内设置显示
					if (bIn)
					{
						m_vecInView[nHori * m_nVertCount + nVert] = 1;
					}
					else
					{
						m_vecInView[nHori * m_nVertCount + nVert] = 0;
					}

					//fTmpX = (nHori + 1) * fHoriStep;
					//fTmpY = VertStartAngle - (nVert + 1) * fVertStep;

					//// 计算中间变量
					//core::vector2df angle;
					//core::position2di screenPos;

					//// 计算-1
					//angle.set(fAngleX,fAngleY);
					//GetScreenPosByAngle(angle,screenPos);
					//bool bIn = rect.isPointInside(screenPos);
					//if (bIn) // 在视图范围内则不再继续进行计算
					//{
					//	m_vecInView[nHori * m_nVertCount + nVert] = 1;
					//	continue;
					//}

					//// 计算-2
					//angle.set(fTmpX,fAngleY);
					//GetScreenPosByAngle(angle,screenPos);
					//bIn = rect.isPointInside(screenPos);
					//if (bIn) // 在视图范围内则不再继续进行计算
					//{
					//	m_vecInView[nHori * m_nVertCount + nVert] = 1;
					//	continue;
					//}

					//// 计算-3
					//angle.set(fAngleX,fTmpY);
					//GetScreenPosByAngle(angle,screenPos);
					//bIn = rect.isPointInside(screenPos);
					//if (bIn) // 在视图范围内则不再继续进行计算
					//{
					//	m_vecInView[nHori * m_nVertCount + nVert] = 1;
					//	continue;
					//}

					//// 计算-4
					//angle.set(fTmpX,fTmpY);
					//GetScreenPosByAngle(angle,screenPos);
					//bIn = rect.isPointInside(screenPos);
					//if (bIn) // 在视图范围内则不再继续进行计算
					//{
					//	m_vecInView[nHori * m_nVertCount + nVert] = 1;
					//	continue;
					//}

					//// 计算执行到此处，表示均不在视口范围内，予以标记
					//m_vecInView[nHori * m_nVertCount + nVert] = 0;
				}
			}

			// 返回计算结果
			return m_vecInView;
		}

		// 获得纹理，不论存在与否
		ITexture* CPanoTileSphere::GetTexture( int index )
		{
			// 不考虑0级切片
			if ((m_nVertCount * m_nHoriCount == 1) || (index < 0) || (index >= m_nVertCount * m_nHoriCount))
			{
				return NULL;
			}

			// 获得mesh
			SMeshBuffer*& pMeshBuf = m_vecBuffer.at(index);
			if (!pMeshBuf)
			{
				return NULL;
			}

			// 获取处理
			ITexture* pCurText = pMeshBuf->Material.getTexture(0);
			return pCurText;
		}

		// 外部设置单个SMaterial纹理，适用于三、四级
		void CPanoTileSphere::SetTexture( int index,CHdSvTileInfoBuffer* pInfoBuf )
		{
			// 不考虑0级切片
			if ((m_nVertCount * m_nHoriCount == 1) || (index < 0) || (index >= m_nVertCount * m_nHoriCount))
			{
				return;
			}

			// 获得切片
			HD_SV_TILEINFO* pInfo = pInfoBuf->GetTileInfo();

			// 切片二进制数据存在则添加纹理
			if (!pInfo || pInfo->nSize <= 0)
			{
				return;
			}

			// 视图判断
			if (!m_pView)
			{
				return;
			}

			// 视图获得
			ISceneView* pSceneView = (ISceneView*)(m_pView);
			if (!pSceneView)
			{
				return;
			}

			// 构建纹理数据
			irr::io::IReadFile* pReadFile = irr::io::createMemoryReadFile(pInfo->pTileData,
				pInfo->nSize,pInfo->strTileID,false);
			IVideoDriver* driver = pSceneView->GetSceneManager()->getVideoDriver();
			ITexture* pTileTexture = driver->getTexture(pReadFile);

			// 设置纹理
			if (pTileTexture)
			{
				// 获得mesh
				SMeshBuffer*& pMeshBuf = m_vecBuffer.at(index);
				if (!pMeshBuf)
				{
					return;
				}

				// 设置纹理
				pMeshBuf->Material.setTexture(0,pTileTexture);
			}

			// drop
			pReadFile->drop();
		}

		// 刷新时调用
		void CPanoTileSphere::Render()
		{
			// 视图条件判断
			if (!m_pView)
			{
				return;
			}

			// 视图条件判断
			ISceneView* pSceneView = (ISceneView*)(m_pView);
			if (!pSceneView)
			{
				return;
			}

			// 获得driver
			video::IVideoDriver* driver = pSceneView->GetSceneManager()->getVideoDriver();

			// 根据条件进行渲染
			if (m_nHoriCount * m_nVertCount == 1)
			{
				driver->setMaterial(Buffer_0->Material);
				driver->drawMeshBuffer(Buffer_0);
			}
			else
			{
				// 三四级切片渲染
				for (int nHori = 0;nHori < m_nHoriCount;nHori++)
				{
					// 垂直
					for (int nVert = 0;nVert < m_nVertCount;nVert++)
					{
						// 索引计算
						int index = nHori * m_nVertCount + nVert;

						//// 不在范围内不渲染
						//if (m_vecInView[index] == 0)
						//{
						//	continue;
						//}

						SMeshBuffer*& pMeshBuf = m_vecBuffer.at(index);
						if (!pMeshBuf)
						{
							continue;
						}

						// 查看纹理是否存在，若不存在则不渲染
						ITexture* pTexture = pMeshBuf->Material.getTexture(0);
						if (!pTexture)
						{
							continue;
						}

						// 渲染
						driver->setMaterial(pMeshBuf->Material);
						driver->drawMeshBuffer(pMeshBuf);

						//// 存在，并在视口范围内，进行渲染
						//driver->setMaterial(m_Material[index]);
						//driver->drawIndexedTriangleFan(&m_Vertices[index*4], 4, m_Indices, 2);
					}
				}
			}
		}

		// 外部获取0级切片纹理
		ITexture* CPanoTileSphere::GetZeroTexture()
		{
			// 获取0级切片纹理
			ITexture* pTexture = Buffer_0->Material.getTexture(0);
			return pTexture;
		}

		// 获得包围盒
		core::aabbox3d<f32>& CPanoTileSphere::getBoundingBox()
		{
			m_box.MaxEdge.set(m_Radius,m_Radius,m_Radius);
			m_box.MinEdge.set(-m_Radius,-m_Radius,-m_Radius);
			return m_box;
		}

	}
}


#include "StdAfx.h"
#include "hd3DView.h"
#include "CScanSceneNode.h"
#include "hd3DCamera.h"
#include "..\hdPointCloud\Point_Cloud.h"
#include "HdSeaDataSceneNode.h"
#include <time.h>
#include "CRoutePointSceneNode.h"
#include "OverViewDataManger.h"
#include "CDomSymSceneNode.h"
#include "CIMeshSceneNode.h"
#include "HdDomSceneNode.h"
#include "HdDemSceneNode.h"
#include "HdRasterDataset.h"
#include "CScanPartPointsSceneNode.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

using namespace hd;
using namespace irr::video;

namespace hd
{
	namespace scene
	{		
		CHd3DView::CHd3DView( void )
			:ISceneView()
		{
			m_irrDevice = NULL;
			m_camera = NULL;
			m_bShowFPS = false;
			m_bShowLOD = false;
			m_bShowAxis = true;
			m_viewType = E_HVT_3D;
			m_eBoxOperate = E_HBO_BOTH;

            m_OverViewDataManger = new COverViewDataManger();
		}

		CHd3DView::~CHd3DView( void )
		{
			if (m_OverViewDataManger)
			{
                delete m_OverViewDataManger;
                m_OverViewDataManger = NULL;
			}
		}
				
		//! 设置最近显示距离
		void CHd3DView::SetNearValue(f32 zn)
		{
			ISceneManager* sceneMng = GetSceneManager();
			if (sceneMng)
			{
				if(sceneMng->getActiveCamera())
				{
					sceneMng->getActiveCamera()->setNearValue(zn);
				}
			}
		}
		//! 设置最远显示距离
		void CHd3DView::SetFarValue(f32 zf)
		{
			ISceneManager* sceneMng = GetSceneManager();
			if (sceneMng)
			{
				if(sceneMng->getActiveCamera())
				{
					sceneMng->getActiveCamera()->setFarValue(zf);
				}
			}
		}
		//! 获取最近显示距离
		f32 CHd3DView::GetNearValue()
		{
			ISceneManager* sceneMng = GetSceneManager();
			if (sceneMng)
			{
				if(sceneMng->getActiveCamera())
				{
					return sceneMng->getActiveCamera()->getNearValue();
				}
			}

			return 0.0f;
		}
		//! 获取最远显示距离
		f32 CHd3DView::GetFarValue()
		{
			ISceneManager* sceneMng = GetSceneManager();
			if (sceneMng)
			{
				if(sceneMng->getActiveCamera())
				{
					return sceneMng->getActiveCamera()->getFarValue();
				}
			}
			return 0.0f;
		}

		// 缩放至指定大小
		void CHd3DView::ZoomToCurrentScene(float extent)
		{
			ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
			if (!cam)
			{
				return;
			}
			if (cam->isOrthogonal())
			{
				cam->setHeightofViewVolume(extent);
				cam->setWidthofViewVolume(extent);
			}
			else
			{
				f32 fov = cam->getFOV();
				f32 nearD = cam->getNearValue();

				f32 fHeight = 2*tan(fov/2.0f)*nearD;
				f32 fWidth = fHeight*(f32)m_width/m_height;

				f32 Dx = extent*nearD/fWidth;
				f32 Dz = extent*nearD/fHeight;

				f32 distCT = ((Dx>Dz)?Dx:Dz) + extent/2.0f;
				core::vector3df pos = cam->getPosition();
				core::vector3df target = cam->getTarget();

				core::vector3df offsetNormal = pos - target;
				offsetNormal.normalize();

				core::vector3df newPos = target + offsetNormal*distCT;

				cam->setPosition(newPos);
			}

		}

		// 缩放到指定范围
		void CHd3DView::ZoomToSpcExtent(const core::aabbox3df& extent)
		{
			core::aabbox3d<f32> bbox = extent;

			ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
			cam->setFarValue(2000);
			cam->setFOV(core::PI / 2.5f);	// Field of view, in radians.
			core::vector3df offsetNormal(0.0f,0.0f,1.0f);

			if (!cam->isOrthogonal())
			{
				f32 fov = cam->getFOV();
				f32 nearD = cam->getNearValue();

				f32 fHeight = 2*tan(fov/2.0f)*nearD;
				f32 fWidth = fHeight*(f32)m_width/m_height;

				f32 Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*nearD/fWidth;
				f32 Dz = (bbox.MaxEdge.Z - bbox.MinEdge.Z)*nearD/fHeight;

				f32 distCT = ((Dx>Dz)?Dx:Dz) + (bbox.MaxEdge.Y - bbox.MinEdge.Y)/2.0f;

				core::vector3df newTarget((bbox.MaxEdge.X + bbox.MinEdge.X)/2.0f, 
					(bbox.MaxEdge.Y + bbox.MinEdge.Y)/2.0f, 
					(bbox.MaxEdge.Z + bbox.MinEdge.Z)/2.0f);

				core::vector3df newPos = newTarget + offsetNormal*distCT;
				m_oriTarget = newTarget;
				m_oriPosition = newPos;

				cam->setPosition(m_oriPosition);
				cam->setTarget(m_oriTarget);

				//将初始的旋转中心设为target点
				m_rotCentre = m_oriTarget;
				cam->setUpVector(core::vector3df(0.0f, 1.0f, 0.0f));

				float distFar = m_oriTarget.getDistanceFrom(m_oriPosition) * 2;
				if (distFar > cam->getFarValue())
				{
					cam->setFarValue(distFar);
				}
			}
			else
			{
				// 正交模式下 看到全部的对象 将外包盒子 作为视景体
				// 设置视景体宽度和高度
				f32 vWidth = cam->getWidthofViewVolume();
				f32 vHeight = cam->getHeightofViewVolume();
				f32 NearD = cam->getNearValue();
				f32 FarD = cam->getFarValue();

				f32 Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*FarD/vWidth;
				f32 Dz = (bbox.MaxEdge.Z - bbox.MinEdge.Z)*FarD/vHeight;
				f32 distCT = 0.0f;

				// 默认俯视图下，宽方向应为X，高方向应为Y而不是Z
				Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*FarD/vWidth;
				Dz = (bbox.MaxEdge.Y - bbox.MinEdge.Y)*FarD/vHeight;

				if (bbox.MaxEdge.X - bbox.MinEdge.X > 0 && bbox.MaxEdge.Y - bbox.MinEdge.Y > 0)
				{
					// 水平方向为X，垂直方向为Y，深度为Z
					cam->setWidthofViewVolume((bbox.MaxEdge.X - bbox.MinEdge.X) * 1.2f);
					cam->setHeightofViewVolume((bbox.MaxEdge.Y - bbox.MinEdge.Y) * 1.2f);
				}

				// 俯视图下深度方向为Z
				distCT = ((Dx>Dz)?Dx:Dz) + (bbox.MaxEdge.Z - bbox.MinEdge.Z)/2.0f;


				core::vector3df newTarget((bbox.MaxEdge.X + bbox.MinEdge.X)/2.0f, 
					(bbox.MaxEdge.Y + bbox.MinEdge.Y)/2.0f, 
					(bbox.MaxEdge.Z + bbox.MinEdge.Z)/2.0f);

				core::vector3df newPos = newTarget + offsetNormal*distCT;
				m_oriTarget = newTarget;
				m_oriPosition = newPos;


				cam->setPosition(m_oriPosition);
				cam->setTarget(m_oriTarget);

				//将初始的旋转中心设为target点
				m_rotCentre = m_oriTarget;
				cam->setUpVector(core::vector3df(0.0f, 1.0f, 0.0f));

				float distFar = m_oriTarget.getDistanceFrom(m_oriPosition) * 2;
				if (distFar > cam->getFarValue())
				{
					cam->setFarValue(distFar);
				}
			}

			CHd3DCamera* phd3DCam = (CHd3DCamera*)getCameraTool();

			// 2014/12/18 初始化时根据设置相机视角类型及相机旋转中心 
			if (phd3DCam)
			{
				phd3DCam->SetCameraPosType(E_CP_TOP);

				phd3DCam->SetRotateCenter(m_oriTarget);
			}


		}

		//! 设置相机的位置的目标，使相机能够看到视图中的所有对象
		void CHd3DView::ZoomToFullExtent(bool bVertical)
		{

			bool bOnlyVisibleApply = isOnlyZoomToVisiblePcds();
			bool bFirstBox = true;
			int i;
			core::array<ISceneNode*> sceneList;
			GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

			int nSNCount = sceneList.size();
			core::aabbox3d<f32> bbox;

			//得到所有测站点云的最大外包Box
			int scanCount = 0;
			core::vector3df firstScanPos;
			for (i = 0; i<nSNCount; i++)
			{
				if (sceneList[i]->getType() == ESNT_SCAN_POINT)
				{
					// 如果标记该视图只对可见点云zoom，此处做处理
					CScanSceneNode* pScanSN = dynamic_cast<CScanSceneNode*>(sceneList[i]);
					if (bOnlyVisibleApply && (pScanSN->isVisible() == false))
					{
						continue;
					}

					const core::aabbox3d<f32>& boxx = sceneList[i]->getBoundingBox();
					core::aabbox3d<f32> box;
					core::aabbox3d<f32> boxRender; // 渲染model转换后的box
					boxRender.MaxEdge.set(F32_MIN,F32_MIN,F32_MIN);
					boxRender.MinEdge.set(F32_MAX,F32_MAX,F32_MAX);
					box = boxx ;

					if (pScanSN && pScanSN->IsTrans())
					{
						CBursaWolfModel renderModel = pScanSN->GetRenderModel();
						core::vector3df edges[8];
						box.getEdges(edges);
						for (int i =0 ; i!=8;i++)
						{
							renderModel.Translate(edges[i].X,edges[i].Y,edges[i].Z);
							boxRender.addInternalPoint(edges[i]);
						}
						box = boxRender;
					}
					if (bFirstBox)
					{
						CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
						if(pScanSn && pScanSn->GetPointCloud() && pScanSn->isVisible())
						{
							PointCloud* pcd = pScanSn->GetPointCloud();
							if ((pcd->m_header.number_of_col * pcd->m_header.number_of_row == pcd->m_header.number_of_point_records)
								&&strcmp(pcd->m_header.file_signature,"HLSF") == 0)  // 对hls1.0格式有效
							{
								// 规则点云用测站原点
								firstScanPos.X = 0.0f;/*pcd->m_header.offsetX*///pcd->m_header.centerX;
								firstScanPos.Y = 0.0f;/*pcd->m_header.offsetY*///pcd->m_header.centerY;
								firstScanPos.Z = 0.0f;/*pcd->m_header.offsetZ*///pcd->m_header.centerZ;
							}
							else
							{
								firstScanPos.X = box.getCenter().X;
								firstScanPos.Y = box.getCenter().Y;
								firstScanPos.Z = box.getCenter().Z;
							}
							//hdApplication::getAppInstance()->getDocument()->m_pHdSceneScans[m_nScanIndex]->transModel.Translate(firstScanPos.X,firstScanPos.Y,firstScanPos.Z);
						}
						else
						{
							firstScanPos.X = box.getCenter().X;
							firstScanPos.Y = box.getCenter().Y;
							firstScanPos.Z = box.getCenter().Z;
						}
						bbox = box;
						bFirstBox = false;
					}
					else
						bbox.addInternalBox(box);
					scanCount++;
				}
			}

			// 如果存在海量点云节点，那么统计海量点云节点包围盒
			GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneList);

			nSNCount = sceneList.size();
			int SeaDCount = 0; // 海量点云节点个数
			for (i = 0; i<nSNCount; i++)
			{
				if (sceneList[i]->getType() == ESNT_HD_SEADATA_POINT)
				{
					// 如果标记该视图只对可见点云zoom，此处做处理
					CHdSeaDataSceneNode* pSdSN = dynamic_cast<CHdSeaDataSceneNode*>(sceneList[i]);
					if (bOnlyVisibleApply && (pSdSN->isVisible() == false))
					{
						continue;
					}

					// 海量点云加载显示最顶层数据
					pSdSN->LoadTopData();

					const core::aabbox3d<f32>& boxx = sceneList[i]->getBoundingBox();
					core::aabbox3d<f32> box = boxx ; // box内部变换为渲染坐标

					if (bFirstBox)
					{

						firstScanPos.X = box.getCenter().X;
						firstScanPos.Y = box.getCenter().Y;
						firstScanPos.Z = box.getCenter().Z;

						bbox = box;
						bFirstBox = false;
					}
					else
						bbox.addInternalBox(box);
					scanCount++;
				}
			}

			// 如果存在海量点云节点，那么统计海量点云节点包围盒
			GetSceneManager()->getSceneNodesFromType(ESNT_PART_SCAN_POINT,sceneList);

			nSNCount = sceneList.size();
			int PartDCount = 0; // 海量点云节点个数
			for (i = 0; i<nSNCount; i++)
			{
				if (sceneList[i]->getType() == ESNT_PART_SCAN_POINT)
				{
					// 如果标记该视图只对可见点云zoom，此处做处理
					CScanPartPointsSceneNode* pSdSN = dynamic_cast<CScanPartPointsSceneNode*>(sceneList[i]);
					if (bOnlyVisibleApply && (pSdSN->isVisible() == false))
					{
						continue;
					}

					const core::aabbox3d<f32>& boxx = sceneList[i]->getBoundingBox();
					core::aabbox3d<f32> box = boxx ; // box内部变换为渲染坐标

					if (bFirstBox)
					{

						firstScanPos.X = box.getCenter().X;
						firstScanPos.Y = box.getCenter().Y;
						firstScanPos.Z = box.getCenter().Z;

						bbox = box;
						bFirstBox = false;
					}
					else
						bbox.addInternalBox(box);
					scanCount++;
				}
			}

			if(scanCount == 0)
				return;

			ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
			cam->setFarValue(2000);
			cam->setFOV(core::PI / 2.5f);	// Field of view, in radians.
			core::vector3df offsetNormal(0.0f,0.0f,1.0f);

			if (!bVertical)
			{
				offsetNormal.X = 0.f;
				offsetNormal.Y = 1.f;
				offsetNormal.Z = 1.f;
			}

			if (!cam->isOrthogonal())
			{
				f32 fov = cam->getFOV();
				f32 nearD = cam->getNearValue();

				f32 fHeight = 2*tan(fov/2.0f)*nearD;
				f32 fWidth = fHeight*(f32)m_width/m_height;

				f32 Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*nearD/fWidth;
				f32 Dz = (bbox.MaxEdge.Z - bbox.MinEdge.Z)*nearD/fHeight;

				f32 distCT = ((Dx>Dz)?Dx:Dz) + (bbox.MaxEdge.Y - bbox.MinEdge.Y)/2.0f;

				if (scanCount == 1)
				{
					// 只有一站,设置到原点
					core::vector3df newPos = firstScanPos + offsetNormal*distCT;//core::vector3df(0.0f,-1.0f,0.0f)
					m_oriTarget = firstScanPos;
					m_oriPosition = newPos;
				}
				else
				{
					core::vector3df newTarget((bbox.MaxEdge.X + bbox.MinEdge.X)/2.0f, 
						(bbox.MaxEdge.Y + bbox.MinEdge.Y)/2.0f, 
						(bbox.MaxEdge.Z + bbox.MinEdge.Z)/2.0f);

					core::vector3df newPos = newTarget + offsetNormal*distCT;//core::vector3df(0.0f,-1.0f,0.0f)
					m_oriTarget = newTarget;
					m_oriPosition = newPos;
				}

				cam->setPosition(m_oriPosition);
				cam->setTarget(m_oriTarget);
				//将初始的旋转中心设为target点
				m_rotCentre = m_oriTarget;
				cam->setUpVector(core::vector3df(0.0f, 1.0f, 0.0f));

				float distFar = m_oriTarget.getDistanceFrom(m_oriPosition) * 2;
				if (distFar > cam->getFarValue())
				{
					cam->setFarValue(distFar);
				}
			}
			else
			{
				// 正交模式下 看到全部的对象 将外包盒子 作为视景体
				// 设置视景体宽度和高度
				f32 vWidth = cam->getWidthofViewVolume();
				f32 vHeight = cam->getHeightofViewVolume();
				f32 NearD = cam->getNearValue();
				f32 FarD = cam->getFarValue();

				f32 Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*FarD/vWidth;
				f32 Dz = (bbox.MaxEdge.Z - bbox.MinEdge.Z)*FarD/vHeight;
				f32 distCT = 0.0f;

				// 默认俯视图下，宽方向应为X，高方向应为Y而不是Z
				if (bVertical)
				{
					Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*FarD/vWidth;
					Dz = (bbox.MaxEdge.Y - bbox.MinEdge.Y)*FarD/vHeight;

					if (bbox.MaxEdge.X - bbox.MinEdge.X > 0 && bbox.MaxEdge.Y - bbox.MinEdge.Y > 0)
					{
						// 水平方向为X，垂直方向为Y，深度为Z
						cam->setWidthofViewVolume((bbox.MaxEdge.X - bbox.MinEdge.X) * 1.2f);
						cam->setHeightofViewVolume((bbox.MaxEdge.Y - bbox.MinEdge.Y) * 1.2f);
					}

					// 俯视图下深度方向为Z
					distCT = ((Dx>Dz)?Dx:Dz) + (bbox.MaxEdge.Z - bbox.MinEdge.Z)/2.0f;
				}
				else
				{
					if (bbox.MaxEdge.X - bbox.MinEdge.X > 0 && bbox.MaxEdge.Z - bbox.MinEdge.Z > 0)
					{
						// 2013/11/12 蔡红云 把视锥体的长宽方大1.8倍，解决ISCAN视图下查看全部点云，看不完全的
						// 问题
						// 进一步放大视锥
						cam->setHeightofViewVolume((bbox.MaxEdge.X - bbox.MinEdge.X) * 1.8f);
						cam->setWidthofViewVolume((bbox.MaxEdge.Z - bbox.MinEdge.Z) * 1.8f);

					}

					distCT = ((Dx>Dz)?Dx:Dz) + (bbox.MaxEdge.Y - bbox.MinEdge.Y)/2.0f;
				}

				//target点设置在box中心
				if (scanCount == 1)
				{
					// 只有一站,设置到原点
					core::vector3df newPos = firstScanPos + offsetNormal*distCT;
					m_oriTarget = firstScanPos;
					m_oriPosition = newPos;
				}
				else
				{
					core::vector3df newTarget((bbox.MaxEdge.X + bbox.MinEdge.X)/2.0f, 
						(bbox.MaxEdge.Y + bbox.MinEdge.Y)/2.0f, 
						(bbox.MaxEdge.Z + bbox.MinEdge.Z)/2.0f);

					core::vector3df newPos = newTarget + offsetNormal*distCT;
					m_oriTarget = newTarget;
					m_oriPosition = newPos;
				}

				cam->setPosition(m_oriPosition);
				cam->setTarget(m_oriTarget);

				//将初始的旋转中心设为target点
				m_rotCentre = m_oriTarget;
				cam->setUpVector(core::vector3df(0.0f, 1.0f, 0.0f));

				float distFar = m_oriTarget.getDistanceFrom(m_oriPosition) * 2;
				if (distFar > cam->getFarValue())
				{
					cam->setFarValue(distFar);
				}
			}

			CHd3DCamera* phd3DCam = dynamic_cast<CHd3DCamera*>(getCameraTool());

			//2014/12/18 初始化时根据设置相机视角类型及相机旋转中心 
			if (phd3DCam)
			{
				phd3DCam->SetCameraPosType(E_CP_TOP);

				phd3DCam->SetRotateCenter(m_oriTarget);
			}
		}
		

		void CHd3DView::ZoomToFullExtent1(bool bVertical)
		{
			bool bFirstBox = true;
			int i;
			core::array<ISceneNode*> sceneList;
			GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneList);

			int nSNCount = sceneList.size();
			core::aabbox3d<f32> bbox;

	
			//得到所有测站点云的最大外包Box
			int scanCount = 0;
			core::vector3df firstScanPos;
			for (i = 0; i<nSNCount; i++)
			{
				if (sceneList[i]->getType() == ESNT_HD_SEADATA_POINT)
				{
					const core::aabbox3d<f32>& boxx = sceneList[i]->getBoundingBox();
					core::aabbox3d<f32> box;
					core::aabbox3d<f32> boxRender; // 渲染model转换后的box
					boxRender.MaxEdge.set(F32_MIN,F32_MIN,F32_MIN);
                    boxRender.MinEdge.set(F32_MAX,F32_MAX,F32_MAX);
				
					box = boxx ;
					CHdSeaDataSceneNode* pScanSN = dynamic_cast<CHdSeaDataSceneNode*>(sceneList[i]);
					if (pScanSN && pScanSN->IsTrans())
					{
						//irr::core::vector3df center = box.getCenter();
						//irr::core::vector3df extent = box.getExtent();

						CBursaWolfModel renderModel = pScanSN->GetRenderModel();
						//renderModel.Translate(center.X,center.Y,center.Z);

						//box.MinEdge.X = center.X - extent.X/2.f;
						//box.MaxEdge.X = center.X + extent.X/2.f;
						//box.MinEdge.Y = center.Y - extent.Y/2.f;
						//box.MaxEdge.Y = center.Y + extent.Y/2.f;
						//box.MinEdge.Z = center.Z - extent.Z/2.f;
						//box.MaxEdge.Z = center.Z + extent.Z/2.f;

						// 旋转矩阵存在时，外包围盒显示不正确,更新八个顶点重新生成外包围盒[2014/9/24] 蔡红云 
						core::vector3df edges[8];
						box.getEdges(edges);
						for (int i =0 ; i!=8;i++)
						{
							renderModel.Translate(edges[i].X,edges[i].Y,edges[i].Z);
							boxRender.addInternalPoint(edges[i]);
						}
						box = boxRender;

					}
					if (bFirstBox)
					{

							firstScanPos.X = box.getCenter().X;
							firstScanPos.Y = box.getCenter().Y;
							firstScanPos.Z = box.getCenter().Z;
					
						bbox = box;
						bFirstBox = false;
					}
					else
						bbox.addInternalBox(box);
					scanCount++;
				}
			}
			if(scanCount == 0/* && pMlsSn == NULL*/)
				return;
	
			ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
			cam->setFarValue(2000);
			cam->setFOV(core::PI / 2.5f);	// Field of view, in radians.
			core::vector3df offsetNormal(0.0f,0.0f,1.0f);

			if (!bVertical)
			{
				offsetNormal.X = 0.f;
				offsetNormal.Y = 1.f;
				offsetNormal.Z = 1.f;
			}

			if (!cam->isOrthogonal())
			{
				f32 fov = cam->getFOV();
				f32 nearD = cam->getNearValue();

				f32 fHeight = 2*tan(fov/2.0f)*nearD;
				f32 fWidth = fHeight*(f32)m_width/m_height;

				f32 Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*nearD/fWidth;
				f32 Dz = (bbox.MaxEdge.Z - bbox.MinEdge.Z)*nearD/fHeight;

				f32 distCT = ((Dx>Dz)?Dx:Dz) + (bbox.MaxEdge.Y - bbox.MinEdge.Y)/2.0f;

				if (scanCount == 1)
				{
					// 只有一站,设置到原点
					core::vector3df newPos = firstScanPos + offsetNormal*distCT;//core::vector3df(0.0f,-1.0f,0.0f)
					m_oriTarget = firstScanPos;
					m_oriPosition = newPos;
				}
				else
				{
					core::vector3df newTarget((bbox.MaxEdge.X + bbox.MinEdge.X)/2.0f, 
						(bbox.MaxEdge.Y + bbox.MinEdge.Y)/2.0f, 
						(bbox.MaxEdge.Z + bbox.MinEdge.Z)/2.0f);

					core::vector3df newPos = newTarget + offsetNormal*distCT;//core::vector3df(0.0f,-1.0f,0.0f)
					m_oriTarget = newTarget;
					m_oriPosition = newPos;
				}

				cam->setPosition(m_oriPosition);
				cam->setTarget(m_oriTarget);
				//将初始的旋转中心设为target点
				m_rotCentre = m_oriTarget;
				cam->setUpVector(core::vector3df(0.0f, 1.0f, 0.0f));

				float distFar = m_oriTarget.getDistanceFrom(m_oriPosition) * 2;
				if (distFar > cam->getFarValue())
				{
					cam->setFarValue(distFar);
				}
			}
			else
			{
				// 正交模式下 看到全部的对象 将外包盒子 作为视景体
				// 设置视景体宽度和高度
				f32 vWidth = cam->getWidthofViewVolume();
				f32 vHeight = cam->getHeightofViewVolume();
				f32 NearD = cam->getNearValue();
				f32 FarD = cam->getFarValue();
				f32 Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*FarD/vWidth;
				f32 Dz = (bbox.MaxEdge.Z - bbox.MinEdge.Z)*FarD/vHeight;

				if (bbox.MaxEdge.X - bbox.MinEdge.X > 0 && bbox.MaxEdge.Z - bbox.MinEdge.Z > 0)
				{
					// 2013/11/12 蔡红云 把视锥体的长宽方大1.8倍，解决ISCAN视图下查看全部点云，看不完全的
					// 问题
					cam->setHeightofViewVolume((bbox.MaxEdge.X - bbox.MinEdge.X) * 1.8f);
					cam->setWidthofViewVolume((bbox.MaxEdge.Z - bbox.MinEdge.Z) * 1.8f);

					//cam->setWidthofViewVolume((bbox.MaxEdge.X - bbox.MinEdge.X) * 1.2f);
					//cam->setHeightofViewVolume((bbox.MaxEdge.Y - bbox.MinEdge.Y) * 1.2f);

				}

				f32 distCT = ((Dx>Dz)?Dx:Dz) + (bbox.MaxEdge.Y - bbox.MinEdge.Y)/2.0f;
				
				//target点设置在box中心
				if (scanCount == 1)
				{
					// 只有一站,设置到原点
					core::vector3df newPos = firstScanPos + offsetNormal*distCT;
					m_oriTarget = firstScanPos;
					m_oriPosition = newPos;
				}
				else
				{
					core::vector3df newTarget((bbox.MaxEdge.X + bbox.MinEdge.X)/2.0f, 
						(bbox.MaxEdge.Y + bbox.MinEdge.Y)/2.0f, 
						(bbox.MaxEdge.Z + bbox.MinEdge.Z)/2.0f);

					core::vector3df newPos = newTarget + offsetNormal*distCT;
					m_oriTarget = newTarget;
					m_oriPosition = newPos;
				}
				
				cam->setPosition(m_oriPosition);
				cam->setTarget(m_oriTarget);

				//将初始的旋转中心设为target点
				m_rotCentre = m_oriTarget;
				cam->setUpVector(core::vector3df(0.0f, 1.0f, 0.0f));

				float distFar = m_oriTarget.getDistanceFrom(m_oriPosition) * 2;
				if (distFar > cam->getFarValue())
				{
					cam->setFarValue(distFar);
				}
			}

			CHd3DCamera* phd3DCam = (CHd3DCamera*)getCameraTool();

			// 2014/12/18 初始化时根据设置相机视角类型及相机旋转中心 
			if (phd3DCam)
			{
				phd3DCam->SetCameraPosType(E_CP_TOP);

				phd3DCam->SetRotateCenter(m_oriTarget);
			}
		}
		
		void CHd3DView::SetRotateCentre(core::vector3df rotCentre)
		{
			m_rotCentre = rotCentre;
			ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
			if (cam)
			{
				cam->setTarget(m_rotCentre);

				RefreshViewBySendMessage();
				ReloadData();
			}
		}
		// 2013/8/2 蔡红云 修改视图切换跳跃变化时修改
		void CHd3DView::SetCamTarPos(core::vector3df tar)
		{
			m_rotCentre = tar;
			ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
			if (cam)
			{
				cam->setTarget(m_rotCentre);
			}
		}

		//! 获取视图数据范围
		core::aabbox3df CHd3DView::GetViewExtent()
		{
			core::aabbox3df fullBox;
			GetCenter(fullBox);
			return fullBox;
		}

		//! 设置旋转中心点到视图数据中心
		void CHd3DView::SetRotate2DataCenter()
		{
			// 自动设置旋转中心点为当前数据中心位置
			core::aabbox3df viewBox;
			core::vector3df center = GetDataCenter(viewBox);

			SetRotateCentre(center);
		}

		// 获取三维视图视角
		ENUM_HD_VIEW_ANGLE CHd3DView::GetViewAngle()
		{
			// 获取相机位置
			ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
			if (!cam)
			{
				return E_HVA_ISO;
			}

			core::vector3df pos = cam->getPosition();
			core::vector3df tar = cam->getTarget();

			// 获取相机位置与目标点的连线
			core::vector3df lineOrt = tar - pos;
			lineOrt.normalize();

			// 判断射线方向与XOY平面是否垂直
			core::vector3df xoyOrt(0.f,0.f,1.f);
			core::vector3df yozOrt(1.f,0.f,0.f);
			core::vector3df xozOrt(0.f,1.f,0.f);

			float angle = lineOrt.getAngleTo(xoyOrt);

			// 设定容差值 0.5度以内
			if (fabs(angle) < 0.0001f || fabs(fabs(angle) - 180.0f) < 0.0001f)
			{
				return E_HVA_XOY;
			}
			else
			{
				angle = lineOrt.getAngleTo(yozOrt);
				if (fabs(angle) < 0.0001f || fabs(fabs(angle) - 180.0f) < 0.0001f)
				{
					return E_HVA_YOZ;
				}
				else
				{
					angle = lineOrt.getAngleTo(xozOrt);
					if (fabs(angle) < 0.0001f || fabs(fabs(angle) - 180.0f) < 0.0001f)
					{
						return E_HVA_XOZ;
					}
				}
			}

			return E_HVA_ISO;
		}

		// 获取三维视图视角 严格约束
		ENUM_HD_VIEW_ANGLE CHd3DView::GetViewAngleStrict()
		{
			// 获取相机位置
			ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
			if (!cam)
			{
				return E_HVA_ISO;
			}

			core::vector3df pos = cam->getPosition();
			core::vector3df tar = cam->getTarget();

			// 获取相机位置与目标点的连线
			core::vector3df lineOrt = tar - pos;
			lineOrt.normalize();

			// 判断射线方向与XOY平面是否垂直
			core::vector3df xoyOrt(0.f,0.f,1.f);
			core::vector3df yozOrt(1.f,0.f,0.f);
			core::vector3df xozOrt(0.f,1.f,0.f);

			float angle = lineOrt.getAngleTo(xoyOrt);

			// 设定容差值 
			if (angle == 0.f || angle == 180.f)
			{
				return E_HVA_XOY;
			}
			else
			{
				angle = lineOrt.getAngleTo(yozOrt);
				if (angle == 0.f || angle == 180.f)
				{
					return E_HVA_YOZ;
				}
				else
				{
					angle = lineOrt.getAngleTo(xozOrt);
					if (angle == 0.f || angle == 180.f)
					{
						return E_HVA_XOZ;
					}
				}
			}

			return E_HVA_ISO;
		}

		bool CHd3DView::RemoveSceneNode(IObjectSceneNode* pSceneNode)
		{
			if (!pSceneNode)
			{
				return false;
			}

            // 移除Dom数据
            CDomSymbSceneNode* pDomSymbSN = dynamic_cast<CDomSymbSceneNode*>(pSceneNode);
            if (pDomSymbSN)
            {
                m_OverViewDataManger->DeleteImage(pDomSymbSN->GetDomTexturePah().data());
            }

            pSceneNode->remove();

			return true;
		}

		void CHd3DView::RemoveSceneNodeFromType(ESCENE_NODE_TYPE type)
		{
			core::array<ISceneNode*> sceneList;

			GetSceneManager()->getSceneNodesFromType(type,sceneList);

			int sceneCount = sceneList.size();

			for (int i = 0; i< sceneCount;i++)
			{
				if (sceneList[i])
				{
					sceneList[i]->remove();
				}
			}

		}

		// 若三维视图中不存在点云，则zoom to hdi范围，若存在点云则不做处理
		void CHd3DView::ZoomToHdiIfNeed(bool bVertical)
		{
			// 获得视图中点云sn
			core::array<ISceneNode*> sceneList;
			GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);
			int nSNCount = sceneList.size();
			if (nSNCount <= 0)
			{
				GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneList);
				nSNCount = sceneList.size();
			}

			// 若存在任何点云sn，不做处理
			if (nSNCount > 0)
			{
				return;
			}

			// 不存在任何点云sn，获取轨迹点sn
			GetSceneManager()->getSceneNodesFromType(ESNT_HD_ROUTEPOINT,sceneList);
			nSNCount = sceneList.size();
			if (nSNCount <= 0)
			{
				return;
			}

			// 外包围盒设置
			core::aabbox3d<f32> bbox;
			int scanCount = 0;
			core::vector3df firstScanPos;
			bool bFirstBox = true;

			nSNCount = sceneList.size();

			// 遍历处理
			for (int i = 0; i<nSNCount; i++)
			{
				// 如果标记该视图只对可见点云zoom，此处做处理
				CRoutePointSceneNode* pSdSN = dynamic_cast<CRoutePointSceneNode*>(sceneList[i]);
				if (!pSdSN)
				{
					continue;
				}

				const core::aabbox3d<f32>& boxx = sceneList[i]->getBoundingBox();
				core::aabbox3d<f32> box = boxx ; // box内部变换为渲染坐标

				if (bFirstBox)
				{

					firstScanPos.X = box.getCenter().X;
					firstScanPos.Y = box.getCenter().Y;
					firstScanPos.Z = box.getCenter().Z;

					bbox = box;
					bFirstBox = false;
				}
				else
					bbox.addInternalBox(box);
				scanCount++;
			}

			// 存在性处理
			if(scanCount == 0)
				return;

			// 设置相机fov
			ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
			cam->setFarValue(2000);
			cam->setFOV(core::PI / 2.5f);	// Field of view, in radians.
			core::vector3df offsetNormal(0.0f,0.0f,1.0f);

			if (!bVertical)
			{
				offsetNormal.X = 0.f;
				offsetNormal.Y = 1.f;
				offsetNormal.Z = 1.f;
			}

			// 透视投影
			if (!cam->isOrthogonal())
			{
				f32 fov = cam->getFOV();
				f32 nearD = cam->getNearValue();

				f32 fHeight = 2*tan(fov/2.0f)*nearD;
				f32 fWidth = fHeight*(f32)m_width/m_height;

				f32 Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*nearD/fWidth;
				f32 Dz = (bbox.MaxEdge.Z - bbox.MinEdge.Z)*nearD/fHeight;

				f32 distCT = ((Dx>Dz)?Dx:Dz) + (bbox.MaxEdge.Y - bbox.MinEdge.Y)/2.0f;

				if (scanCount == 1)
				{
					// 只有一站,设置到原点
					core::vector3df newPos = firstScanPos + offsetNormal*distCT;//core::vector3df(0.0f,-1.0f,0.0f)
					m_oriTarget = firstScanPos;
					m_oriPosition = newPos;
				}
				else
				{
					core::vector3df newTarget((bbox.MaxEdge.X + bbox.MinEdge.X)/2.0f, 
						(bbox.MaxEdge.Y + bbox.MinEdge.Y)/2.0f, 
						(bbox.MaxEdge.Z + bbox.MinEdge.Z)/2.0f);

					core::vector3df newPos = newTarget + offsetNormal*distCT;//core::vector3df(0.0f,-1.0f,0.0f)
					m_oriTarget = newTarget;
					m_oriPosition = newPos;
				}

				cam->setPosition(m_oriPosition);
				cam->setTarget(m_oriTarget);
				//将初始的旋转中心设为target点
				m_rotCentre = m_oriTarget;
				cam->setUpVector(core::vector3df(0.0f, 1.0f, 0.0f));

				float distFar = m_oriTarget.getDistanceFrom(m_oriPosition) * 2;
				if (distFar > cam->getFarValue())
				{
					cam->setFarValue(distFar);
				}
			}
			else
			{
				// 正交模式下 看到全部的对象 将外包盒子 作为视景体
				// 设置视景体宽度和高度
				f32 vWidth = cam->getWidthofViewVolume();
				f32 vHeight = cam->getHeightofViewVolume();
				f32 NearD = cam->getNearValue();
				f32 FarD = cam->getFarValue();

				f32 Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*FarD/vWidth;
				f32 Dz = (bbox.MaxEdge.Z - bbox.MinEdge.Z)*FarD/vHeight;
				f32 distCT = 0.0f;

				// 默认俯视图下，宽方向应为X，高方向应为Y而不是Z
				if (bVertical)
				{
					Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*FarD/vWidth;
					Dz = (bbox.MaxEdge.Y - bbox.MinEdge.Y)*FarD/vHeight;

					if (bbox.MaxEdge.X - bbox.MinEdge.X > 0 && bbox.MaxEdge.Y - bbox.MinEdge.Y > 0)
					{
						// 水平方向为X，垂直方向为Y，深度为Z
						cam->setWidthofViewVolume((bbox.MaxEdge.X - bbox.MinEdge.X) * 1.2f);
						cam->setHeightofViewVolume((bbox.MaxEdge.Y - bbox.MinEdge.Y) * 1.2f);
					}

					// 俯视图下深度方向为Z
					distCT = ((Dx>Dz)?Dx:Dz) + (bbox.MaxEdge.Z - bbox.MinEdge.Z)/2.0f;
				}
				else
				{
					if (bbox.MaxEdge.X - bbox.MinEdge.X > 0 && bbox.MaxEdge.Z - bbox.MinEdge.Z > 0)
					{
						// 2013/11/12 蔡红云 把视锥体的长宽方大1.8倍，解决ISCAN视图下查看全部点云，看不完全的
						// 问题
						// 进一步放大视锥
						cam->setHeightofViewVolume((bbox.MaxEdge.X - bbox.MinEdge.X) * 1.8f);
						cam->setWidthofViewVolume((bbox.MaxEdge.Z - bbox.MinEdge.Z) * 1.8f);

					}

					distCT = ((Dx>Dz)?Dx:Dz) + (bbox.MaxEdge.Y - bbox.MinEdge.Y)/2.0f;
				}

				//target点设置在box中心
				if (scanCount == 1)
				{
					// 只有一站,设置到原点
					core::vector3df newPos = firstScanPos + offsetNormal*distCT;
					m_oriTarget = firstScanPos;
					m_oriPosition = newPos;
				}
				else
				{
					core::vector3df newTarget((bbox.MaxEdge.X + bbox.MinEdge.X)/2.0f, 
						(bbox.MaxEdge.Y + bbox.MinEdge.Y)/2.0f, 
						(bbox.MaxEdge.Z + bbox.MinEdge.Z)/2.0f);

					core::vector3df newPos = newTarget + offsetNormal*distCT;
					m_oriTarget = newTarget;
					m_oriPosition = newPos;
				}

				cam->setPosition(m_oriPosition);
				cam->setTarget(m_oriTarget);

				//将初始的旋转中心设为target点
				m_rotCentre = m_oriTarget;
				cam->setUpVector(core::vector3df(0.0f, 1.0f, 0.0f));

				float distFar = m_oriTarget.getDistanceFrom(m_oriPosition) * 2;
				if (distFar > cam->getFarValue())
				{
					cam->setFarValue(distFar);
				}
			}

			CHd3DCamera* phd3DCam = dynamic_cast<CHd3DCamera*>(getCameraTool());

			//2014/12/18 初始化时根据设置相机视角类型及相机旋转中心 
			if (phd3DCam)
			{
				phd3DCam->SetCameraPosType(E_CP_TOP);

				phd3DCam->SetRotateCenter(m_oriTarget);
			}
		}

		// zoomtofull for 3ds
		void CHd3DView::ZoomToFullFor3ds()
		{
			// bool bOnlyVisibleApply = isOnlyZoomToVisiblePcds();
			bool bFirstBox = true;
			int i;
			core::array<ISceneNode*> sceneList;
			GetSceneManager()->getSceneNodesFromType(ESNT_MESH_MODEL,sceneList);

			int nSNCount = sceneList.size();
			core::aabbox3d<f32> bbox;

			//得到所有测站点云的最大外包Box
			int scanCount = 0;
			core::vector3df firstScanPos;
			for (i = 0; i<nSNCount; i++)
			{
				if (sceneList[i]->getType() == ESNT_MESH_MODEL)
				{
					// 如果标记该视图只对可见点云zoom，此处做处理
					CIMeshSceneNode* pScanSN = dynamic_cast<CIMeshSceneNode*>(sceneList[i]);

					const core::aabbox3d<f32>& boxx = sceneList[i]->getBoundingBox();
					core::aabbox3d<f32> box;
					core::aabbox3d<f32> boxRender; // 渲染model转换后的box
					boxRender.MaxEdge.set(F32_MIN,F32_MIN,F32_MIN);
					boxRender.MinEdge.set(F32_MAX,F32_MAX,F32_MAX);
					box = boxx ;

					if (bFirstBox == 0)
					{
						bbox = pScanSN->getBoundingBox();
						bFirstBox = false;
					}
					else
					{
						core::aabbox3d<irr::f32> box = pScanSN->getBoundingBox();
						bbox.addInternalBox(box);
					}

					scanCount++;
				}
			}

			// 不存在则返回
			if(scanCount == 0)
				return;

			ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
			cam->setFarValue(2000);
			cam->setFOV(core::PI / 2.5f);	// Field of view, in radians.
			core::vector3df offsetNormal(0.0f,0.0f,1.0f);

			if (!cam->isOrthogonal())
			{
				f32 fov = cam->getFOV();
				f32 nearD = cam->getNearValue();

				f32 fHeight = 2*tan(fov/2.0f)*nearD;
				f32 fWidth = fHeight*(f32)m_width/m_height;

				f32 Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*nearD/fWidth;
				f32 Dz = (bbox.MaxEdge.Z - bbox.MinEdge.Z)*nearD/fHeight;

				f32 distCT = ((Dx>Dz)?Dx:Dz) + (bbox.MaxEdge.Y - bbox.MinEdge.Y)/2.0f;

				if (scanCount == 1)
				{
					// 只有一站,设置到原点
					core::vector3df newPos = firstScanPos + offsetNormal*distCT;//core::vector3df(0.0f,-1.0f,0.0f)
					m_oriTarget = firstScanPos;
					m_oriPosition = newPos;
				}
				else
				{
					core::vector3df newTarget((bbox.MaxEdge.X + bbox.MinEdge.X)/2.0f, 
						(bbox.MaxEdge.Y + bbox.MinEdge.Y)/2.0f, 
						(bbox.MaxEdge.Z + bbox.MinEdge.Z)/2.0f);

					core::vector3df newPos = newTarget + offsetNormal*distCT;//core::vector3df(0.0f,-1.0f,0.0f)
					m_oriTarget = newTarget;
					m_oriPosition = newPos;
				}

				cam->setPosition(m_oriPosition);
				cam->setTarget(m_oriTarget);
				//将初始的旋转中心设为target点
				m_rotCentre = m_oriTarget;
				cam->setUpVector(core::vector3df(0.0f, 1.0f, 0.0f));

				float distFar = m_oriTarget.getDistanceFrom(m_oriPosition) * 2;
				if (distFar > cam->getFarValue())
				{
					cam->setFarValue(distFar);
				}
			}
			else
			{
				// 正交模式下 看到全部的对象 将外包盒子 作为视景体
				// 设置视景体宽度和高度
				f32 vWidth = cam->getWidthofViewVolume();
				f32 vHeight = cam->getHeightofViewVolume();
				f32 NearD = cam->getNearValue();
				f32 FarD = cam->getFarValue();

				f32 Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*FarD/vWidth;
				f32 Dz = (bbox.MaxEdge.Z - bbox.MinEdge.Z)*FarD/vHeight;
				f32 distCT = 0.0f;

				// 默认俯视图下，宽方向应为X，高方向应为Y而不是Z
				//if (bVertical)
				{
					Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*FarD/vWidth;
					Dz = (bbox.MaxEdge.Y - bbox.MinEdge.Y)*FarD/vHeight;

					if (bbox.MaxEdge.X - bbox.MinEdge.X > 0 && bbox.MaxEdge.Y - bbox.MinEdge.Y > 0)
					{
						// 水平方向为X，垂直方向为Y，深度为Z
						cam->setWidthofViewVolume((bbox.MaxEdge.X - bbox.MinEdge.X) * 1.2f);
						cam->setHeightofViewVolume((bbox.MaxEdge.Y - bbox.MinEdge.Y) * 1.2f);
					}

					// 俯视图下深度方向为Z
					distCT = ((Dx>Dz)?Dx:Dz) + (bbox.MaxEdge.Z - bbox.MinEdge.Z)/2.0f;
				}

				//target点设置在box中心
				if (scanCount == 1)
				{
					// 只有一站,设置到原点
					core::vector3df newPos = firstScanPos + offsetNormal*distCT;
					m_oriTarget = firstScanPos;
					m_oriPosition = newPos;
				}
				else
				{
					core::vector3df newTarget((bbox.MaxEdge.X + bbox.MinEdge.X)/2.0f, 
						(bbox.MaxEdge.Y + bbox.MinEdge.Y)/2.0f, 
						(bbox.MaxEdge.Z + bbox.MinEdge.Z)/2.0f);

					core::vector3df newPos = newTarget + offsetNormal*distCT;
					m_oriTarget = newTarget;
					m_oriPosition = newPos;
				}

				cam->setPosition(m_oriPosition);
				cam->setTarget(m_oriTarget);

				//将初始的旋转中心设为target点
				m_rotCentre = m_oriTarget;
				cam->setUpVector(core::vector3df(0.0f, 1.0f, 0.0f));

				float distFar = m_oriTarget.getDistanceFrom(m_oriPosition) * 2;
				if (distFar > cam->getFarValue())
				{
					cam->setFarValue(distFar);
				}
			}

			CHd3DCamera* phd3DCam = dynamic_cast<CHd3DCamera*>(getCameraTool());

			//2014/12/18 初始化时根据设置相机视角类型及相机旋转中心 
			if (phd3DCam)
			{
				phd3DCam->SetCameraPosType(E_CP_TOP);

				phd3DCam->SetRotateCenter(m_oriTarget);
			}
		}

		IObjectSceneNode* CHd3DView::AddDomSceneNode(const char* pcFileName)
		{
			// 判断是否已加载该文件
			bool bExist = false;

			if (bExist)
			{
				return NULL;
			}

			// 创建 DOM 节点
			CHdRasterDataset* pDataset = CHdRasterDataset::Open(pcFileName);
			if (!pDataset)
			{
				return NULL;
			}
			CHdDomSceneNode* pDomSN = new CHdDomSceneNode(GetSceneManager()->getRootSceneNode(), 
				GetSceneManager(), -1, pDataset);
			pDomSN->SetView(this);
			pDomSN->setVisible(true);
			pDomSN->drop();

			// 初始化视图的变换模型
			if (m_transModel.IsIdentity())
			{
				m_transModel = pDomSN->GetModel().getAntiModel();
			}
			pDomSN->UpdateTransModel();

			return pDomSN;
		}
		IObjectSceneNode* CHd3DView::AddDemSceneNode(const char* pcFileName)
		{
			// 判断是否已加载该文件
			bool bExist = false;

			if (bExist)
			{
				return NULL;
			}

			// 创建 DEM 节点
			CHdRasterDataset* pDataset = CHdRasterDataset::Open(pcFileName);
			if (!pDataset)
			{
				return NULL;
			}

			CHdDemSceneNode* pDemSN = new CHdDemSceneNode(GetSceneManager()->getRootSceneNode(), 
				GetSceneManager(), -1, pDataset);
			pDemSN->SetView(this);
			pDemSN->setVisible(true);
			pDemSN->drop();

			// 初始化视图的变换模型
			if (m_transModel.IsIdentity())
			{
				m_transModel = pDemSN->GetModel().getAntiModel();
			}
			pDemSN->UpdateTransModel();

			return pDemSN;
		}
	}
}
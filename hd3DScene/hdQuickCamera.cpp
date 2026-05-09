#include "StdAfx.h"
#include "hdQuickCamera.h"
#include "hdCommandDef.h"
#include "hdQuickView.h"
#include "hd3DView.h"
#include "computeNormal.h"
#include "..\hdPointCloud\hdSysSetting.h"
#include "CRoutePointSceneNode.h"
#include "CBackGroundSceneNode.h"
#include "CScanSceneNode.h"
#include "HdSeaDataSceneNode.h"
#include "..\hdFramework\userMessage.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif


namespace hd
{
	namespace fm
	{
		CHdQuickCamera::CHdQuickCamera(void)
			:CHdCamera("CHdQuickCamera", "快速浏览工具", "CHdQuickCamera", "快速浏览工具", E_HCT_Quick)
		{
			m_app = NULL;
			m_id = COMMAND_QUICK_CAMERA;
			m_meshSN = NULL;
			m_type = E_HCT_Quick ;

			m_MousePos = core::position2df(0.5f, 0.5f);
			m_RotV = 0.0f;
			m_RotH = 0.0;
			m_RotR = 0.0;
			m_bRotating = false;
			m_bViewAngleInit = false;
			m_RotateSpeed = 1.0f;
			m_fMinFov = core::PI/36.0f;
			m_fMaxFov = core::PI / 2;
			m_bMouseTrack = TRUE;

			m_fMinVerAngle = -85.0f;
			m_fMaxVerAngle = 85.0f;
		}

		CHdQuickCamera::CHdQuickCamera(const char* caption,const char* msg,const char* name,const char* tooltip, int type)
			:CHdCamera(caption, msg, name, tooltip, type)
		{
			m_id = COMMAND_QUICK_CAMERA;

			m_meshSN = NULL;
			m_MousePos = core::position2df(0.5f, 0.5f);
			m_RotV = 0.0f;
			m_RotH = 0.0;
			m_RotR = 0.0;
			m_bRotating = false;
			m_bViewAngleInit = false;
			m_fMinFov = core::PI/36.0f;
			m_fMaxFov = core::PI / 2;
			m_bMouseTrack = TRUE;
			m_fMinVerAngle = -85.0f;
			m_fMaxVerAngle = 85.0f;
				
		}

		CHdQuickCamera::~CHdQuickCamera(void)
		{
		}

		void CHdQuickCamera::OnMouseDown( int Button, int Shift, int X, int Y )
		{
			if (!GetEnable())
				return;

			if (Button == 1)//左键
			{
				m_bRotating = true;
				m_RotateStart.X = (f32)X;
				m_RotateStart.Y = (f32)Y;

				//当前正在旋转，抽稀显示点云
				ISceneView* pView = dynamic_cast<ISceneView*>(m_app->GetActiveView());
				if (pView == NULL)
				{
					return;
				}
				pView->GetSceneManager()->SetAnimateState(true);


			}	
		}

		void CHdQuickCamera::OnMouseUp( int Button, int Shift, int X, int Y )
		{
			if (!GetEnable())
				return;

			ISceneView* pView = dynamic_cast<ISceneView*>(m_app->GetActiveView());
			if (pView == NULL)
			{
				return;
			}

			if (Button == 1)
			{
				m_bRotating = false;
				//wkl 2012-7-27 13:37:16

				pView->GetSceneManager()->SetAnimateState(false);

				//// 若包含全景切片sn，刷新时更新数据纹理贴图
				//if (pView->IsIncludeSceneNode(ESNT_PANO) && pView->isMongoView() && pView->IsViewRenderAllNode())
				//{
				//	pView->SendMsgToWindowsView(WM_USER_RELOAD_TITLEPANO, (WPARAM)TRUE, NULL);
				//}

				// 更新范围后重新加载数据
				if (strcmp( pView->GetName(),"iScan3DView") != 0)
				{
					pView->ReloadData();
				}
				else
				{
					// 刷新视图
					pView->Refresh();
				}


				// ***测试加载大数据量标志不切换层chy[2015-3-31]***
				//pView->SetChangeLevl(false);

				// -------如果包含海量点云节点调用ReLoadData，重新加载数据
				/*	if (pView->IsIncludeSceneNode(ESNT_HD_SEADATA_POINT))
				{

				pView->ReloadData();
				}*/


				//在鼠标释放时，发送WM_USER_CADCAMERA_ANGLE消息，供C#模块处理  -- liangjia 2014/03/17 
				//这里保存的不是点坐标，而是用一个点的结构保存水平和竖直方向的角
				core::vector3dd pt3d(m_RotH, m_RotV, 0);
				m_bViewAngleInit = true;
				if (pView->GetHWnd())
				{
					::SendMessage(pView->GetHWnd(), WM_USER_CADCAMERA_ANGLE, (WPARAM)(&pt3d), (LPARAM)NULL);
				}

				//iScan视图下，全景查看方向与轨迹视图联动消息
				if (strcmp(pView->GetName(),"iScan3DView") == 0)
				{
					float fViex, fViey, fxFov, fyFov;
					GetCameraState(fViex, fViey, fxFov, fyFov);
					pView->SendMsgToWindowsView(WM_USER_ISCAN_PANO_ROTATE,(WPARAM)&fViex,(LPARAM)&fViey);
				}

			}


		}

		void CHdQuickCamera::OnMouseMove( int Button, int Shift, int X, int Y )
		{
			if (!GetEnable())
				return;

			ISceneView* pView = dynamic_cast<ISceneView*>(m_app->GetActiveView());
			if (pView)
			{		
				if ((Button == 1) && m_bRotating)
				{
					//得到当前的屏幕大小
					unsigned int h = pView->GetWindowHeight();
					unsigned int w = pView->GetWindowWidth();

					//记录当前点
					m_MousePos.X = (float)X;
					m_MousePos.Y = (float)Y;

					irr::scene::ISceneManager * smgr = pView->GetSceneManager();
					ICameraSceneNode* camera = smgr->getActiveCamera();

					//// 测试代码
					//hd::f32 fov = camera->getFOV();
					//hd::f32 nearD = camera->getNearValue();

					//hd::f32 fHeight = 2*tan(fov/2.0f)*nearD;
					//hd::f32 scale = fHeight / pView->GetWindowHeight();


					float fStartHori, fStartVert, fEndHori, fEndVert;

					GetAngleBySC(X,Y,fEndHori,fEndVert);
					GetAngleBySC(hd_round32(m_RotateStart.X),hd_round32(m_RotateStart.Y),fStartHori,fStartVert);

					core::vector3df pos = camera->getPosition();
					core::vector3df target = camera->getTarget();
					core::vector3df screenFar = target - pos;
					f32 length = screenFar.getLength();
					screenFar.normalize();

					//更新水平、垂直转角，因为摄像机的初始转角可能不为零;
					GetVectorAngle(screenFar,m_RotH,m_RotV);
					m_bViewAngleInit = true;
					// Rotation begin------------------------------------
					f32 nRotV = (fStartVert - fEndVert);
					f32 nRotH = (fStartHori - fEndHori);

					m_RotH += nRotH;//记录当前水平方向的角度值;
					while (true)
					{
						if (m_RotH < 0)
						{
							m_RotH += 360.0f;
						}
						else if (m_RotH >= 360.0f)
						{
							m_RotH -= 360.0f;
						}
						else
							break;
					}

					//限制垂直方向的转动，向上向下均不能超过90度;
					m_RotV += nRotV;//记录当前垂直方向的角度值;
					if (m_RotV > m_fMaxVerAngle)
					{
						m_RotV = m_fMaxVerAngle;
					}
					else if (m_RotV < m_fMinVerAngle)
					{
						m_RotV = m_fMinVerAngle;
					}
					//Rotate
					f32 nRadianV = m_RotV * core::DEGTORAD;
					f32 nRadianH = m_RotH * core::DEGTORAD;
					target.Z = pos.Z + sin(nRadianV) * length;
					target.X = pos.X + cos(nRadianV) * length * cos(nRadianH);
					target.Y = pos.Y + cos(nRadianV) * length * sin(nRadianH);
					camera->setTarget(target);

					//// 像素容差
					//float tol =32.f;

					//if (abs(m_RotateStart.X - (f32)X) >tol || abs(m_RotateStart.Y - (f32)Y)>tol)
					//{
					//	// 若包含全景切片sn，刷新时更新数据纹理贴图
					//	if (pView->IsIncludeSceneNode(ESNT_PANO) && pView->isMongoView() && pView->IsViewRenderAllNode())
					//	{
					//		pView->SendMsgToWindowsView(WM_USER_RELOAD_TITLEPANO, (WPARAM)TRUE, NULL);
					//	}
					//}

					m_RotateStart.X = (f32)X;
					m_RotateStart.Y = (f32)Y;

					// 允许对鼠标进行追踪
					if (m_bMouseTrack)
					{
						TRACKMOUSEEVENT csTME;
						csTME.cbSize = sizeof(csTME);
						csTME.dwFlags = TME_LEAVE;

						// 指定追踪的窗口
						csTME.hwndTrack = pView->GetHWnd();

						// 停留时间
						csTME.dwHoverTime = 10;

						// 开启追踪
						TrackMouseEvent(&csTME);
						m_bMouseTrack = FALSE;
					}

					// Rotation end------------------------------------				
				}

				// 像素容差
				float tol =32.f;

				if (abs(m_RotateStart.X - (f32)X) >tol || abs(m_RotateStart.Y - (f32)Y)>tol)
				{
					// 若包含全景切片sn，刷新时更新数据纹理贴图
					if (pView->IsIncludeSceneNode(ESNT_PANO) && pView->IsViewRenderAllNode())
					{
						// 获得pano sn
						CPanoSceneNode* pPanoSN = dynamic_cast<CPanoSceneNode*>(pView->GetSceneManager()->getSceneNodeFromType(ESNT_PANO));
						if (pPanoSN && pPanoSN->isTileModel())
						{
							pView->SendMsgToWindowsView(WM_USER_RELOAD_TITLEPANO, (WPARAM)TRUE, NULL);
						}
					}
				}


				pView->Refresh();
			}
			if ( Button == 0 && (pView->GetViewType() == E_HVT_QUICK  || pView->GetViewType() == E_HVT_3D) && 
				CHdSysSetting::getSysSetting()->commonSetting.showNormal == 1)
			{
				ISceneManager* sceneMng = pView->GetSceneManager();

				if (m_meshSN == NULL)
				{
					m_meshSN = new CMeshPolySceneNode(sceneMng->getRootSceneNode(),sceneMng,0);
					m_meshSN->SetView(pView);
					m_meshSN->drop();
				}

				core::vector3df scanPos;
				ISceneCollisionManager* pCln = pView->GetSceneManager()->getSceneCollisionManager();
				if(!pCln->get3DPositionFromScreenPos(X,Y,scanPos.X,scanPos.Y,scanPos.Z))
				{
					m_meshSN->setPoint(core::vector3df(0.0f,0.0f,0.0f),core::vector3df(0.0f,0.0f,0.0f));
					pView->Refresh();
					return;
				}

				float x = 0.0f,y = 0.0f,z = 0.0f;
				int pixel = 16;
				int xMin = X - pixel < 0?0:(X - pixel);
				int xMax = X + pixel > pView->GetWindowWidth() ? pView->GetWindowWidth():(X + pixel);
				int yMin = Y - pixel < 0?0:Y - pixel;
				int yMax = Y + pixel > pView->GetWindowHeight() ? pView->GetWindowHeight():(Y + pixel);
				vector<Eigen::Vector3f> pts;
				Eigen::Vector3f pt;
				for (int i = xMin;i<xMax;i++)
				{
					for (int j = yMin;j<yMax;j++)
					{
						if(pCln->get3DPositionFromScreenPos(i,j,pt[0],pt[1],pt[2]))
						{
							pts.push_back(pt);
						}
					}
				}
				float curvature;
				core::vector3df scanPtNormal;
				computePointNormal(pts,scanPtNormal.X,scanPtNormal.Y,scanPtNormal.Z,curvature);
				pt[0] = scanPos.X;
				pt[1] = scanPos.Y;
				pt[2] = scanPos.Z;
				flipNormalTowardsViewpoint(pt, 0,0,0, scanPtNormal.X,scanPtNormal.Y,scanPtNormal.Z);

				m_meshSN->setPoint(scanPos,scanPtNormal);
				m_meshSN->setVisible(true);
				pView->Refresh();
			}
			else
			{
				if (m_meshSN)
				{
					m_meshSN->setVisible(false);
				}
			}

			////iScan视图下，全景查看方向与轨迹视图联动消息
			//if (strcmp(pView->GetName(),"iScan3DView") == 0)
			//{
			//	
			//	float fViex, fViey, fxFov, fyFov;
			//	GetCameraState(fViex, fViey, fxFov, fyFov);
			//	pView->SendMsgToWindowsView(WM_USER_ISCAN_PANO_ROTATE,(WPARAM)&fViex,(LPARAM)&fViey);
			//}
		}

		void CHdQuickCamera::OnClick()
		{
			if (!GetEnable())
				return;	

			m_checked = true;

			m_RotateSpeed = 1.0f;
			ISceneView* pView = dynamic_cast<ISceneView*>(m_app->GetActiveView());
			if (pView != NULL)
			{
				irr::scene::ISceneManager * smgr = pView->GetSceneManager();
				ICameraSceneNode* camera = smgr->getActiveCamera();
				camera->setProjectionType(E_HPT_PERSPECTIVE);
				pView->SetViewProjectionType(E_HPT_PERSPECTIVE);
				camera->setUpVector(core::vector3df(0.0f,0.0f,1.0f));

			}
		}

		void CHdQuickCamera::OnCreate( CHdApp* app )
		{
			if(app == NULL)
				return;
			m_app = app;
		}

		bool CHdQuickCamera::GetEnable()
		{
			m_enabled = false;
			if (m_app && m_app->GetActiveView() != NULL)
			{
				m_enabled = (m_app->GetActiveView()->GetViewType() == E_HVT_3D ||
					m_app->GetActiveView()->GetViewType() == E_HVT_MLS3D  ||
					m_app->GetActiveView()->GetViewType() == E_HVT_QUICK  ||
                    m_app->GetActiveView()->GetViewType() == E_HVT_REG_QUICK ||
					m_app->GetActiveView()->GetViewType() == E_HVT_PANO  ||
					m_app->GetActiveView()->GetViewType() == E_HVT_ORTHO3D);
			}
			return m_enabled;
		}

		void CHdQuickCamera::Deactivate()
		{
			m_checked = false;
			m_bRotating = false;
			if (m_meshSN)
			{
				m_meshSN->remove();
				m_meshSN = NULL;
			}

			// deactivate时发消息至视图移除扇形符号
			if (m_app && m_app->GetActiveView() != NULL)
			{
				IHdView* pView = m_app->GetActiveView();
				pView->SendMsgToWindowsView(WM_USER_ISCAN_REMOVE_SECTOR,0,0);
			}

		}

		bool CHdQuickCamera::OnContextMenu( int X, int Y )
		{
			return true;
		}

		void CHdQuickCamera::OnDblClick( int Button, int Shift, int X, int Y )
		{
			//// 当前视图是IScan3D视图，才会双击退出
			//IHdView* pView = m_app->GetActiveView();
			//if (!pView)
			//{
			//	return;
			//}

			////return;

			//// 当前视图没有进行
			//if ((pView->GetViewType() == E_HVT_3D
			//	&& strcmp(pView->GetName(),"iScan3DView") == 0
			//	&& !pView->GetPanoPlayStatus()) ||
			//	(pView->GetViewType() == E_HVT_ORTHO3D && strcmp(pView->GetName(),"ClassifyOperView") == 0))
			//{
			//	FallBack3DCameraStatus();
			//}

			return;
		}

		void CHdQuickCamera::OnMouseWheel( UINT nFlags, short zDelta, int X, int Y )
		{
			if (!GetEnable())
				return;
			irr::f32 wheel = ((irr::f32)((short)zDelta)) / (irr::f32)WHEEL_DELTA;
			f32 fCurrentZoom = wheel*4.0f/100.0f;
			if (fCurrentZoom > 0.1f || fCurrentZoom < -0.1f)
			{
				int fuck = 0;
			}

			ISceneView* pView = dynamic_cast<ISceneView*>(m_app->GetActiveView());
			if (pView != NULL)
			{
				irr::scene::ISceneManager * smgr = pView->GetSceneManager();
				ICameraSceneNode* camera = smgr->getActiveCamera();

				//Zoom
				if (fCurrentZoom != 0)
				{
					//不改变相机的位置，而是通过修改平截台体的参数（变换摄像机镜头，从长焦到广角）,来实现渲染对象的缩放;
					f32 fov = camera->getFOV();
					f32 curFov = fov;
					curFov -= fCurrentZoom;
					if (curFov < m_fMinFov)
					{
						curFov = m_fMinFov;
					}
					else if (curFov > m_fMaxFov)///和条件语句一致，修正鼠标滚轮缩放到一定倍率后滚轮失效的bug，by liuzhaoliang
					{
						curFov = m_fMaxFov;
						//curFov = core::PI*2/3.0f;
					}

					camera->setFOV(curFov);

					// ***测试加载大数据量标志切换层chy[2015-3-31]***
					pView->SetChangeLevl(true);

					// -------如果包含海量点云节点调用ReLoadData，重新加载数据
					if (pView->IsIncludeSceneNode(ESNT_HD_SEADATA_POINT))
					{

						core::array<ISceneNode*> ArrNode;
						pView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,ArrNode);
						u32 size = ArrNode.size();

						for (u32 i= 0; i<size; i++)
						{
							CHdSeaDataSceneNode* pNode = dynamic_cast<CHdSeaDataSceneNode*>(ArrNode[i]); 
							if (pNode)
							{
								pNode->ReloadData();
							}

						}	
					}

					// 若包含全景切片sn，刷新时更新数据纹理贴图
					if (pView->IsIncludeSceneNode(ESNT_PANO) && pView->isMongoView())
					{
						// 获得pano sn
						CPanoSceneNode* pPanoSN = dynamic_cast<CPanoSceneNode*>(pView->GetSceneManager()->getSceneNodeFromType(ESNT_PANO));
						if (pPanoSN && pPanoSN->isTileModel())
						{
							// 更新相对位置，然后再截屏，再调用强制刷新以实时改变相机，再取消截屏
							pView->SetViewRenderAllNodeWithoutRefresh(false,true);
							pView->RefreshViewBySendMessage();
							pView->SetViewRenderAllNodeWithoutRefresh(true,false);

							pView->SendMsgToWindowsView(WM_USER_RELOAD_TITLEPANO, (WPARAM)FALSE, NULL);
						}

						

					}

				}

				pView->Refresh();
			}
		}

		void CHdQuickCamera::OnKeyDown( int keyCode, int Shift )
		{
			if (keyCode=='w'||keyCode=='W')
			{
				CameraAdjust(3);
			}
			if (keyCode=='S'||keyCode=='s')
			{
				CameraAdjust(4);
			}
			if (keyCode=='A'||keyCode=='a')
			{
				CameraAdjust(1);
			}
			if (keyCode=='D'||keyCode=='d')
			{
				CameraAdjust(2);
			}
		}

		void CHdQuickCamera::OnKeyUp( int keyCode, int Shift )
		{
			//if (keyCode==KEY_UP)
			//{
			//	CameraAdjust(3);
			//}
			//if (keyCode==KEY_DOWN)
			//{
			//	CameraAdjust(4);
			//}
			//if (keyCode==KEY_LEFT)
			//{
			//	CameraAdjust(1);
			//}
			//if (keyCode==KEY_RIGHT)
			//{
			//	CameraAdjust(2);
			//}
		}

		//! 得到相机角度
		void CHdQuickCamera::GetViewAngle(core::vector3df& angle)
		{
			//如果视图旋转角没有被计算得到过，先计算，再返回
			if (!m_bViewAngleInit)
			{
				InitViewAngle();
			}
			angle.X = m_RotH;
			angle.Y = m_RotV;
			angle.Z = m_RotR;
		}

		void CHdQuickCamera::MoveCamera(core::vector3df newPos)
		{
			CHdQuickView* pView = dynamic_cast<CHdQuickView*>(m_app->GetActiveView());
			if (pView == NULL)
			{
				return;
			}
			irr::scene::ISceneManager * smgr = pView->GetSceneManager();
			ICameraSceneNode* camera = smgr->getActiveCamera();

			//根据扫描点坐标得到球体坐标			
			core::vector3df dstTarget;
			pView->GetSpherePosByScanPos(newPos, dstTarget);

			camera->setTarget(dstTarget);

			pView->Refresh();
		}

		void CHdQuickCamera::MoveCamera( core::vector2df newPos )
		{
			CHdQuickView* pView = dynamic_cast<CHdQuickView*>(m_app->GetActiveView());
			if (pView == NULL)
			{
				return;
			}
			irr::scene::ISceneManager * smgr = pView->GetSceneManager();
			ICameraSceneNode* camera = smgr->getActiveCamera();
			core::vector3df dstTarget;
			pView->GetSpherePosByImageScale(newPos,dstTarget);

			camera->setTarget(dstTarget);
			pView->Refresh();
		}

		void CHdQuickCamera::GetVectorAngle(core::vector3df vect,float& fViewX,float& fViewY)
		{
			vect.normalize();

			fViewY = atan2(vect.Z, sqrt(vect.X * vect.X + vect.Y * vect.Y)) * core::RADTODEG;
			fViewX = atan2(vect.Y, vect.X) * core::RADTODEG;
			if (fViewX < 0)
			{
				fViewX += 360.0f;
			}
		}

		void CHdQuickCamera::GetCameraState(float& fViewX, float& fViewY, float& xFov, float& yFov)
		{
			ISceneView* pView = dynamic_cast<ISceneView*>(m_app->GetActiveView());
			if (pView == NULL)
			{
				return;
			}

			int nHeight = pView->GetWindowHeight();
			int nWidth = pView->GetWindowWidth();

			irr::scene::ISceneManager * smgr = pView->GetSceneManager();
			ICameraSceneNode* camera = smgr->getActiveCamera();

			if (camera)
			{
				core::vector3df vect = camera->getTarget() - camera->getPosition();
				GetVectorAngle(vect, fViewX, fViewY);
				yFov = camera->getFOV();
				float tana = (tan(yFov/2)*nWidth)/(float)nHeight;
				xFov = 2*atan(tana)*core::RADTODEG;
				yFov *= core::RADTODEG;
			}
		}

		void CHdQuickCamera::GetAngleBySC(int nScX,int nScY,float& fHoriAngle,float& fVertAngle)
		{
			ISceneView* pView = dynamic_cast<ISceneView*>(m_app->GetActiveView());
			if (pView == NULL)
			{
				return;
			}

			irr::scene::ISceneManager * smgr = pView->GetSceneManager();
			ICameraSceneNode* camera = smgr->getActiveCamera();

			core::line3df line = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(core::vector2di(nScX, nScY));

			core::vector3df vect = line.getVector();
			GetVectorAngle(vect, fHoriAngle, fVertAngle);

		}

		void CHdQuickCamera::FallBack3DCameraStatus()
		{
			//在快速视图下，双击鼠标，相机退出全景球
			CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
			if (!pView)
			{
				return;
			}
			irr::scene::ISceneManager* smgr = pView->GetSceneManager();
			if (!smgr)
			{
				return;
			}
			irr::scene::ICameraSceneNode* camera = smgr->getActiveCamera();
			core::vector3df newtarget;
			core::vector3df newpos;
			core::vector3df offset;
			if (pView->GetCamStatusCount() > 1)
			{
				//		//// 获取最近的相机状态
				//		SCamStatus camSts = pView->GetLatestCameraStatus();
				//		//// 设置当前的相机状态
				//		//camera->setPosition(camSts.pos);
				//		//camera->setTarget(camSts.target);
				//		//camera->setUpVector(camSts.UpVector);
				//		//camera->setAspectRatio(camSts.Aspect);
				//		//camera->setFOV(camSts.Fovy);
				//		//camera->setNearValue(camSts.ZNear);
				//		//camera->setFarValue(camSts.ZFar);
				//		//camera->setWidthofViewVolume(camSts.WidthOfViewVolume);
				//		//camera->setHeightofViewVolume(camSts.HeightOfViewVolume);

				// 蔡红云 2013/10/22 控制全景浏览之后，相机视角在结束帧附近。
				newtarget = camera->getPosition();
				newpos = newtarget;
				// 获取最近的相机状态
				SCamStatus camSts = pView->GetLatestCameraStatus();
				if (camSts.IsOrthogonal)
				{
					pView->SetViewProjectionType(E_HPT_ORTHOGONAL);
				}
				else
				{
					pView->SetViewProjectionType(E_HPT_PERSPECTIVE);
				}
				offset = camSts.pos - camSts.target; // 得到微调因子
				newpos =  newtarget + offset; // 得到最新相机相机Z位置
				camera->setPosition(newpos); // 设置相机pos
				camera->setTarget(newtarget); // 设置相机target
				camera->setUpVector(camSts.UpVector);

			}
			else
			{
				// 查看全部
				/*	pView->ZoomToFullExtent();*/

				// 蔡红云 2013/10/22 控制全景浏览之后，相机视角在结束帧附近。
				newtarget = camera->getPosition();
				newpos = newtarget;
				offset= core::vector3df(0, 0, 100); // 得到微调因子
				newpos =  newtarget + offset; // 得到最新相机相机Z位置
				camera->setPosition(newpos); // 设置相机pos
				camera->setTarget(newtarget); // 设置相机target
				camera->setUpVector(core::vector3df(0,1,0));
				camera->setHeightofViewVolume(100.0);
				camera->setWidthofViewVolume(100.0);

				pView->SetViewProjectionType(E_HPT_ORTHOGONAL);
			}

			// 设置当前工具为3D camera
			m_app->SetCurrentTool(COMMAND_3D_CAMERA);

			// 设置当前视图中的panoScenenode的注册模式为auto模式
			CPanoSceneNode* pNode = dynamic_cast<CPanoSceneNode*>(smgr->getSceneNodeFromType(ESNT_PANO));

			if (pNode)
			{
				if (pNode->GetRegisterMode())
				{
					pNode->SetRegisterMode(false);
				}
				if (pNode->GetRadius() == 10.0f)
				{
					pNode->SetRadius(2.0f);
				}
			}
			if (pView->IsBackGroundVisible())
			{
				// 显示背景scenenode
				CBackGroundSceneNode* pnode = dynamic_cast<CBackGroundSceneNode*>(smgr->getSceneNodeFromType(ESNT_HD_BACKGROUND));

				if (pnode)
				{
					pnode->setVisible(true);
				}
			}

			// 设置当前视图中的camera对象
			CHdTool* hdtool = m_app->GetCurrentTool();
			CHdCamera* cameraTool = dynamic_cast<CHdCamera*>(hdtool);
			cameraTool->SetRotateSpeed(-300);
			cameraTool->SetMoveSpeed(350);
			cameraTool->SetZoomSpeed(-600);
			pView->SetCameraTool(cameraTool);
			pView->SetAxisVisiable(true);
			/*	pView->SetComPassVisiable(true);*/
			//pView->Refresh();
			pView->RefreshViewBySendMessage();
			pView->ReloadData();
		}

		void CHdQuickCamera::CameraAdjust( int type )
		{
			if (!GetEnable())
				return;

			ISceneView* pView = dynamic_cast<ISceneView*>(m_app->GetActiveView());
			m_bRotating = true;
			if ( m_bRotating)
			{		
				if (pView != NULL)
				{
					irr::scene::ISceneManager * smgr = pView->GetSceneManager();
					ICameraSceneNode* camera = smgr->getActiveCamera();

					float fStartHori, fStartVert;

					GetAngleBySC(hd_round32(m_RotateStart.X),hd_round32(m_RotateStart.Y),fStartHori,fStartVert);

					core::vector3df pos = camera->getPosition();
					core::vector3df target = camera->getTarget();
					core::vector3df screenFar = target - pos;
					f32 length = screenFar.getLength();
					screenFar.normalize();

					//更新水平、垂直转角，因为摄像机的初始转角可能不为零;
					GetVectorAngle(screenFar,m_RotH,m_RotV);
					m_bViewAngleInit = true;
					f32 nRotV = 0.0f ;
					f32 nRotH = 0.0f;
					switch(type)
					{
					case 1:
						nRotH = 10.0f;
						break;
					case 2:
						nRotH = -10.0f;
						break;
					case 3:
						nRotV = 5.0f ;
						break;
					case 4:
						nRotV = -5.0f ;
						break;
					}


					m_RotH += nRotH;//记录当前水平方向的角度值;
					while (true)
					{
						if (m_RotH < 0)
						{
							m_RotH += 360.0f;
						}
						else if (m_RotH >= 360.0f)
						{
							m_RotH -= 360.0f;
						}
						else
							break;
					}

					//限制垂直方向的转动，向上向下均不能超过90度;
					m_RotV += nRotV;//记录当前垂直方向的角度值;
					if (m_RotV > 85.0f)
					{
						m_RotV = 85.0f;
					}
					else if (m_RotV < -85.0f)
					{
						m_RotV = -85.0f;
					}

					f32 nRadianV = m_RotV * core::DEGTORAD;
					f32 nRadianH = m_RotH * core::DEGTORAD;
					target.Z = pos.Z + sin(nRadianV) * length;
					target.X = pos.X + cos(nRadianV) * length * cos(nRadianH);
					target.Y = pos.Y + cos(nRadianV) * length * sin(nRadianH);
					camera->setTarget(target);

					//pView->Refresh();	
					// 相机视角变化后，会重新加载数据进行刷新[2014/04/15 危迟]
					pView->RefreshViewBySendMessage();
					pView->ReloadData();
				}
			}
		}


		void CHdQuickCamera::InitViewAngle()
		{
			if (!GetEnable())
				return;
			ISceneView* pView = dynamic_cast<ISceneView*>(m_app->GetActiveView());
			if (pView != NULL)
			{
				irr::scene::ISceneManager * smgr = pView->GetSceneManager();
				ICameraSceneNode* camera = smgr->getActiveCamera();

				float fStartHori, fStartVert;

				GetAngleBySC(hd_round32(m_RotateStart.X),hd_round32(m_RotateStart.Y),fStartHori,fStartVert);

				core::vector3df pos = camera->getPosition();
				core::vector3df target = camera->getTarget();
				core::vector3df screenFar = target - pos;
				f32 length = screenFar.getLength();
				screenFar.normalize();

				//更新水平、垂直转角，因为摄像机的初始转角可能不为零;
				GetVectorAngle(screenFar,m_RotH,m_RotV);
				m_bViewAngleInit = true;
			}
		}

		void CHdQuickCamera::CameraAdjustByAngle(double fAngleH,double fAngleV)
		{
			if (!GetEnable())
				return;
			ISceneView* pView = dynamic_cast<ISceneView*>(m_app->GetActiveView());
			if (pView != NULL)
			{
				irr::scene::ISceneManager * smgr = pView->GetSceneManager();
				ICameraSceneNode* camera = smgr->getActiveCamera();

				float fStartHori, fStartVert;

				GetAngleBySC(hd_round32(m_RotateStart.X),hd_round32(m_RotateStart.Y),fStartHori,fStartVert);

				core::vector3df pos = camera->getPosition();
				core::vector3df target = camera->getTarget();
				core::vector3df screenFar = target - pos;
				f32 length = screenFar.getLength();
				screenFar.normalize();

				//更新水平、垂直转角，因为摄像机的初始转角可能不为零;
				GetVectorAngle(screenFar,m_RotH,m_RotV);
				m_bViewAngleInit = true;
				// Rotation begin------------------------------------
				f32 nRotV = 0.0f ;
				f32 nRotH = 0.0f;
				nRotH = (float)fAngleH;
				nRotV = (float)fAngleV;
				nRotH = nRotH * core::RADTODEG;//弧度转角度
				nRotV = nRotV * core::RADTODEG;
				m_RotH += nRotH;//记录当前水平方向的角度值;
				while (true)
				{
					if (m_RotH < 0)
					{
						m_RotH += 360.0f;
					}
					else if (m_RotH >= 360.0f)
					{
						m_RotH -= 360.0f;
					}
					else
						break;
				}

				//限制垂直方向的转动，向上向下均不能超过90度;
				m_RotV += nRotV;//记录当前垂直方向的角度值;
				if (m_RotV > m_fMaxVerAngle)
				{
					m_RotV = m_fMaxVerAngle;
				}
				else if (m_RotV < m_fMinVerAngle)
				{
					m_RotV = m_fMinVerAngle;
				}

				//Rotate
				f32 nRadianV = m_RotV * core::DEGTORAD;
				f32 nRadianH = m_RotH * core::DEGTORAD;
				target.Z = pos.Z + sin(nRadianV) * length;
				target.X = pos.X + cos(nRadianV) * length * cos(nRadianH);
				target.Y = pos.Y + cos(nRadianV) * length * sin(nRadianH);
				camera->setTarget(target);

				// Rotation end------------------------------------
				pView->Refresh();					
			}
		}

		// 设置fov角最大最小值、传入弧度值
		void CHdQuickCamera::SetMinMaxFov( float minFov, float maxFov)
		{
			m_fMinFov = minFov;
			m_fMaxFov = maxFov;
		}

		// 获取fov角最大最小值、传入弧度值
		void CHdQuickCamera::GetMinMaxFov(float &minFov, float &maxFov)
		{
			minFov = m_fMinFov;
			maxFov = m_fMaxFov;
		}


		// 设置垂直角最大最小值、传入角度值 chy-2016-7-23
		void CHdQuickCamera::SetMinMaxVerAngle( float minAngle, float maxAngle)
		{
			m_fMinVerAngle = minAngle;
			m_fMaxVerAngle = maxAngle;
		}

		// 获取垂直角最大最小值、传入角度值
		void CHdQuickCamera::GetMinMaxVerAngle(float &minAngle, float &maxAngle)
		{
			minAngle = m_fMinVerAngle;
			maxAngle = m_fMaxVerAngle;
		}

		void CHdQuickCamera::OnMouseLeave(int Button, int Shift, int X, int Y)
		{
			if (!GetEnable())
			{
				return;
			}

			m_bMouseTrack = TRUE;
		}
	}
}


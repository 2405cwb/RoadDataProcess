#include "StdAfx.h"
#include "Hd3DCmdViewTop.h"
#include "hd3DView.h"
#include "hdCamera.h"
#include "hd3DCamera.h"
#include "..\hdFramework\hdCommandDef.h"
#include "CDomSymSceneNode.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
namespace hd
{
    namespace scene
    {
        CHd3DCmdViewTop::CHd3DCmdViewTop(void)
            :CHdCommand("viewtop","俯视图","viewtop","俯视图",E_HCT_3D | E_HVT_3DVIEWMATCH)
        {
            m_id = COMMAND_3D_VIEWTOP;
        }

        CHd3DCmdViewTop::~CHd3DCmdViewTop(void)
        {
        }

        void CHd3DCmdViewTop::OnClick()
        {
            if (!GetEnable())
                return;
            CHd3DView* hdView = (CHd3DView*)m_app->GetActiveView();
            if (!hdView)
            {
                return;
            }

            unsigned int h = hdView->GetWindowHeight();
            unsigned int w = hdView->GetWindowWidth();
            ICameraSceneNode* pCam = hdView->GetSceneManager()->getActiveCamera();				
            if (!pCam)
            {
                return;
            }

            // chy 2014/4/16
            CHd3DCamera* p3DCamera = NULL;
            CHdTool* pTool = m_app->GetTool(COMMAND_3D_CAMERA);
            if (pTool)
            {
                p3DCamera = (CHd3DCamera*)pTool;			
            }

            if (p3DCamera == NULL)
            {
                return;
            }
						

            // 2013/8/2 蔡红云 如果已经是俯视图，就不需继续执行了
            if (( p3DCamera->GetCameraPosType() == E_CP_BOTTOM &&::GetKeyState(VK_CONTROL) < 0)||
                (p3DCamera->GetCameraPosType() == E_CP_TOP && ::GetKeyState(VK_CONTROL) >= 0))
            {
                return;
            }

			/*p3DCamera->SetCameraPosType(E_CP_TOP);*/
					
			// 保持相机位置与目标点的距离不变（朱立雄 2016-11-11）
			//const core::vector3df& tar = pCam->getTarget();
			//const core::vector3df& pos = pCam->getPosition();
			//float fDisPosToTar = tar.getDistanceFrom(pos);
			//pCam->setPosition(tar + core::vector3df(0, 0, fDisPosToTar));
			
            //// 从Z最大向最小看
            core::aabbox3df viewBox;
            core::vector3df center = hdView->GetDataCenter(viewBox);
            viewBox = viewBox.getIntersectsBox(pCam->getViewFrustum()->getBoundingBox());	
            // 将当前旋转中心的平面位置赋给新的中心 确保前后一致的视觉效果 不会发生平面位置的跳变 [危迟 2014/08/13]
            center.X = p3DCamera->GetRotateCenter().X/*viewBox.getCenter()*/;
            center.Y = p3DCamera->GetRotateCenter().Y;
            center.Z = p3DCamera->GetRotateCenter().Z;
            core::vector3df newpos(center.X,center.Y,viewBox.MaxEdge.Z);
            core::vector3df newtar(center.X,center.Y,viewBox.MinEdge.Z);

            //// 计算合适的POS位置,使得视图能够全图查看
            f32 fov = pCam->getFOV();
            f32 nearD = pCam->getNearValue();

            f32 distX = (viewBox.MaxEdge.X - viewBox.MinEdge.X) / tan(fov/2.0f);
            f32 distY = (viewBox.MaxEdge.Y - viewBox.MinEdge.Y) / tan(fov/2.0f);

            f32 distCT = MAX(distX,distY);
            if (::GetKeyState(VK_CONTROL) < 0)
            {
                // 从newtar向newpos看
                core::vector3df normal = newtar - newpos;
                f32 length = normal.getLength();
                normal.normalize();
                newtar = newpos + normal * distCT;
                pCam->setPosition(newtar);
                pCam->setTarget(center);
                p3DCamera->SetCameraPosType(E_CP_BOTTOM);
            }
            else
            {
                core::vector3df normal = newpos - newtar;
                f32 length = normal.getLength();
                normal.normalize();
                newpos = newtar + normal * distCT;
                pCam->setPosition(newpos);
                pCam->setTarget(center);
                p3DCamera->SetCameraPosType(E_CP_TOP);
            }
            //hdView->SetScanChanged(true);
            if (!hdView->IsViewRenderAllNode())
            {
                //渲染点云
                hdView->SetViewRenderAllNode(true, false);
            }
            pCam->setFarValue(2000);
            pCam->setUpVector(core::vector3df(0.0f,1.0f,0.0f));
            f32 distFar = pCam->getPosition().getDistanceFrom(pCam->getTarget()) * 2;
            if (distFar > pCam->getFarValue())
            {
                pCam->setFarValue(distFar);
            }

            //pCam->setHeightofViewVolume(4.396);
            //pCam->setWidthofViewVolume(6.504);
            //pCam->setPosition(core::vector3df(144.38766,62.617937,889.36548));
            //pCam->setTarget(core::vector3df(144.38766,62.617937,-265.6733));

            hdView->RefreshViewBySendMessage();
            hdView->ReloadData();
			
            // 俯视后重新设置旋转中心
            if (p3DCamera)
            {
                int w = hdView->GetWindowWidth();
                int h = hdView->GetWindowHeight();
                if(p3DCamera->SetTargetByView(w/2,h/2))
                {
                    irr::scene::ISceneManager* smgr = hdView->GetSceneManager();
                    ICameraSceneNode* camera = smgr->getActiveCamera();
                    camera->setTarget(p3DCamera->GetRotateCenter());

                    // 为了俯视观察，重新设置pos
                    if (::GetKeyState(VK_CONTROL) < 0)
                    {
                        camera->setPosition(p3DCamera->GetRotateCenter() + core::vector3df(0, 0, -distCT));
                        p3DCamera->SetCameraPosType(E_CP_BOTTOM);
                    }
                    else
                    {
                        camera->setPosition(p3DCamera->GetRotateCenter() + core::vector3df(0, 0, distCT));
                        p3DCamera->SetCameraPosType(E_CP_TOP);
                    }

                    hdView->RefreshViewBySendMessage();
                }
            }	
			
        }

        void CHd3DCmdViewTop::OnCreate(CHdApp* app)
        {
            if(app == NULL)
                return;
            m_app = app;

        }

        bool CHd3DCmdViewTop::GetEnable()
        {
            m_enabled = false;
            if (m_app && m_app->GetActiveView() != NULL)
            {
                ENUM_HD_VIEW_TYPE type = m_app->GetActiveView()->GetViewType();
                m_enabled = (type ==  E_HVT_3D || type == E_HVT_MULTISCAN3D ||
                    type == E_HVT_MLS3D ||  type == E_HVT_FACADEEDIT || type == E_HVT_SKETCH_ISCAN3D ||
                    type == E_HVT_3DVIEWMATCH || type == E_HVT_REG || type == E_HVT_DOM_3D_VIEW
                    || type == E_HVT_REG_QUICK);
            }
            return m_enabled;
        }

    }
}


#include "StdAfx.h"
#include "Hd3DCmdViewBack.h"
#include "hd3DView.h"
#include "hdCamera.h"
#include "hd3DCamera.h"
#include "..\hdFramework\hdCommandDef.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
namespace hd
{
	namespace scene
	{
		CHd3DCmdViewBack::CHd3DCmdViewBack(void)
			:CHdCommand("viewback","后视图","viewback","后视图",E_HCT_3D | E_HVT_3DVIEWMATCH)
		{
			m_id = COMMAND_3D_VIEWBACK;
		}

		CHd3DCmdViewBack::~CHd3DCmdViewBack(void)
		{
		}

		void CHd3DCmdViewBack::OnClick()
		{
			if (!GetEnable())
				return;
			CHd3DView* hdView = (CHd3DView*)m_app->GetActiveView();
			if (hdView)
			{
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

				// 2013/9/30 蔡红云 如果已经是俯视图，就不需继续执行了
				if (( p3DCamera->GetCameraPosType() == E_CP_BACK &&::GetKeyState(VK_CONTROL) >= 0)||
					(p3DCamera->GetCameraPosType() == E_CP_FRONT && ::GetKeyState(VK_CONTROL) < 0))
				{
					return;
				}


				// 从y轴正方向往负方向看				
				core::aabbox3df viewBox;
				core::vector3df center = hdView->GetDataCenter(viewBox);
				viewBox = viewBox.getIntersectsBox(pCam->getViewFrustum()->getBoundingBox());				
				// 将当前旋转中心的平面位置赋给新的中心 确保前后一致的视觉效果 不会发生平面位置的跳变 [危迟 2014/08/13]
				center.X = p3DCamera->GetRotateCenter().X/*viewBox.getCenter()*/;
    center.Y = p3DCamera->GetRotateCenter().Y;
				center.Z = p3DCamera->GetRotateCenter().Z;
				core::vector3df newpos(center.X,viewBox.MaxEdge.Y,center.Z);
				core::vector3df newtar(center.X,viewBox.MinEdge.Y,center.Z);

				// 计算合适的POS位置,使得视图能够全图查看
				f32 fov = pCam->getFOV();
				f32 nearD = pCam->getNearValue();

				f32 distX = (viewBox.MaxEdge.X - viewBox.MinEdge.X) / tan(fov/2.0f);
				f32 distY = (viewBox.MaxEdge.Y - viewBox.MinEdge.Y) / tan(fov/2.0f);

				if (::GetKeyState(VK_CONTROL) < 0)
				{
					// 从newtar向newpos看
					f32 distCT = MAX(distX,distY);
					core::vector3df normal = newtar - newpos;
					f32 length = normal.getLength();
					normal.normalize();
					newtar = newpos + normal * distCT;
					pCam->setPosition(newtar);
					pCam->setTarget(center);
				   	p3DCamera->SetCameraPosType(E_CP_FRONT);
					//hdView->SetRotateCentre(newpos + normal * distCT * 0.5f);
				}
				else
				{
					// 从newpos向newtar看
					f32 distCT = MAX(distX,distY);
					core::vector3df normal = newpos - newtar;
					f32 length = normal.getLength();
					normal.normalize();
					newpos = newtar + normal * distCT;
					pCam->setPosition(newpos);
					pCam->setTarget(center);
					p3DCamera->SetCameraPosType(E_CP_BACK);
				}
				if (!hdView->IsViewRenderAllNode())
				{
					//渲染点云
					hdView->SetViewRenderAllNode(true, false);
				}
				pCam->setUpVector(core::vector3df(0.0f,0.0f,1.0f));
				pCam->setFarValue(2000);
				f32 distFar = pCam->getPosition().getDistanceFrom(pCam->getTarget()) * 2;
				if (distFar > pCam->getFarValue())
				{
					pCam->setFarValue(distFar);
				}

				hdView->RefreshViewBySendMessage();
				hdView->ReloadData();
			}
		}
		void CHd3DCmdViewBack::OnCreate( CHdApp* app )
		{
			if(app == NULL)
				return;
			m_app = app;
		}

		bool CHd3DCmdViewBack::GetEnable() 
		{
			m_enabled = false;
			if (m_app && m_app->GetActiveView() != NULL)
			{
				ENUM_HD_VIEW_TYPE type = m_app->GetActiveView()->GetViewType();
				m_enabled = (type ==  E_HVT_3D || type == E_HVT_MULTISCAN3D ||
					type == E_HVT_MLS3D ||  type == E_HVT_FACADEEDIT || type == E_HVT_SKETCH_ISCAN3D ||
					type == E_HVT_3DVIEWMATCH || type == E_HVT_REG || type == E_HVT_DOM_3D_VIEW ||
                    type == E_HVT_REG_QUICK);
			}
			return m_enabled;
		}
	}
}



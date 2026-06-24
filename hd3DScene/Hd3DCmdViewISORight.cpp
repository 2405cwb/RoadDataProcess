#include "StdAfx.h"
#include "Hd3DCmdViewISORight.h"
#include "..\hdCore\hdMath.h"
#include "hd3DView.h"
#include "hdCamera.h"
#include "hd3DCamera.h"
#include "..\hdFramework\hdCommandDef.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
using namespace hd::fm;

namespace hd
{
	namespace scene
	{
		CHd3DCmdViewISORight::CHd3DCmdViewISORight(void)
			:CHdCommand("viewISORight","右视图","viewISORight","右视图",E_HCT_3D | E_HVT_3DVIEWMATCH)
		{
			m_id = COMMAND_3D_ISORIGHT;
		}


		CHd3DCmdViewISORight::~CHd3DCmdViewISORight(void)
		{
		}

		void CHd3DCmdViewISORight::OnClick()
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

				// 从YZ最高X最小往对角看
				core::aabbox3df viewBox;
				core::vector3df center = hdView->GetDataCenter(viewBox);
				viewBox = viewBox.getIntersectsBox(pCam->getViewFrustum()->getBoundingBox());				
				center = viewBox.getCenter();
				core::vector3df newpos(viewBox.MinEdge.X,viewBox.MaxEdge.Y,viewBox.MaxEdge.Z);
				core::vector3df newtar(viewBox.MaxEdge.X,viewBox.MinEdge.Y,viewBox.MinEdge.Z);

				// 计算合适的POS位置,使得视图能够全图查看
				f32 fov = pCam->getFOV();
				f32 nearD = pCam->getNearValue();

				f32 distX = (viewBox.MaxEdge.X - viewBox.MinEdge.X) / tan(fov/2.0f);
				f32 distY = (viewBox.MaxEdge.Y - viewBox.MinEdge.Y) / tan(fov/2.0f);

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

				// 2013/9/30 蔡红云 如果已经是右视图，就不需继续执行了
				if (( p3DCamera->GetCameraPosType() == E_CP_ISO_INVER_RIGHT &&::GetKeyState(VK_CONTROL) < 0)||
					(p3DCamera->GetCameraPosType() == E_CP_ISO_RIGHT && ::GetKeyState(VK_CONTROL) >= 0))
				{
					return;
				}

				if (::GetKeyState(VK_CONTROL) < 0)
				{
					// 从newtar向newpos看
					f32 distCT = MAX(distX,distY);
					core::vector3df normal = newtar - newpos;
					f32 length = normal.getLength();
					normal.normalize();
					newtar = newpos + normal * distCT;

					pCam->setPosition(newtar);
					pCam->setTarget(newpos);
					p3DCamera->SetCameraPosType(E_CP_ISO_INVER_RIGHT);
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
					pCam->setTarget(newtar);
		        	p3DCamera->SetCameraPosType(E_CP_ISO_RIGHT);
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

		void CHd3DCmdViewISORight::OnCreate( CHdApp* app )
		{
			if(app == NULL)
				return;
			m_app = app;
		}

		bool CHd3DCmdViewISORight::GetEnable() 
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



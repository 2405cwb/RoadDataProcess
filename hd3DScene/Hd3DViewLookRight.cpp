#include "StdAfx.h"
#include "Hd3DViewLookRight.h"
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
		CHd3DViewLookRight::CHd3DViewLookRight(void)
			:CHdCommand("lookright","向右看","lookright","从左往右看",E_HCT_3D)
		{
			m_id = COMMAND_3D_LOOKRIGHT;
		}


		CHd3DViewLookRight::~CHd3DViewLookRight(void)
		{
		}

			void CHd3DViewLookRight::OnClick()
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

					core::aabbox3df viewBox;
					core::vector3df center = hdView->GetDataCenter(viewBox);
					viewBox = viewBox.getIntersectsBox(pCam->getViewFrustum()->getBoundingBox());				
					center = viewBox.getCenter();
					core::vector3df newpos = pCam->getPosition();
					
					if (::GetKeyState(VK_CONTROL) < 0)
					{
						// 向X负方向看
						newpos.X = newpos.X - pCam->getFarValue() / 2.0f;
						pCam->setTarget(newpos);

						//hdView->SetRotateCentre(newpos);
					}
					else
					{
						// 向X正方向看
						newpos.X = newpos.X + pCam->getFarValue() / 2.0f;
						pCam->setTarget(newpos);

						//hdView->SetRotateCentre(newpos);
					}
					pCam->setUpVector(core::vector3df(0.0f,0.0f,1.0f));
					pCam->setFarValue(2000);
					f32 distFar = newpos.getDistanceFrom(pCam->getTarget()) * 2;
					if (distFar > pCam->getFarValue())
					{
						pCam->setFarValue(distFar);
					}

					hdView->RefreshViewBySendMessage();
					if (strcmp(hdView->GetName(),"iScan3DView") != 0)
					{
						hdView->ReloadData();
					}
				}
			}

			void CHd3DViewLookRight::OnCreate(CHdApp* app)
			{
				if(app == NULL)
					return;
				m_app = app;

			}

			bool CHd3DViewLookRight::GetEnable()
			{
				m_enabled = false;
				if (m_app && m_app->GetActiveView() != NULL)
				{
					m_enabled = (m_app->GetActiveView()->GetViewType() ==  E_HVT_3D);

				}
				return m_enabled;
			}

	}
}


#include "StdAfx.h"
#include "hd3DView.h"
#include "Hd3DViewLookUp.h"
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

		CHd3DViewLookUp::CHd3DViewLookUp(void)
			:CHdCommand("lookup","向上看","lookup","从下往上看",E_HCT_3D)
		{
			m_id = COMMAND_3D_LOOKUP;
		}

		CHd3DViewLookUp::~CHd3DViewLookUp(void)
		{
		}

		void CHd3DViewLookUp::OnClick()
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
					// 向Z负方向看
					newpos.Z = newpos.Z - pCam->getFarValue() / 2.0f;
					pCam->setTarget(newpos);

					//hdView->SetRotateCentre(newpos);
				}
				else
				{
					// 向Z正方向看
					newpos.Z = newpos.Z + pCam->getFarValue() / 2.0f;
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

				// iScan视图不用重新加载数据
				if (strcmp(hdView->GetName(),"iScan3DView") != 0)
				{
					hdView->ReloadData();
				}
			}
		}

		void CHd3DViewLookUp::OnCreate(CHdApp* app)
		{
			if(app == NULL)
				return;
			m_app = app;

		}

		bool CHd3DViewLookUp::GetEnable()
		{
			m_enabled = false;
			if (m_app && m_app->GetActiveView() != NULL)
			{
				m_enabled = (m_app->GetActiveView()->GetViewType() ==  E_HVT_3D ||
							 m_app->GetActiveView()->GetViewType() == E_HVT_MULTISCAN3D ||
                             m_app->GetActiveView()->GetViewType() == E_HVT_SKETCH_ISCAN3D ||
							 m_app->GetActiveView()->GetViewType() == E_HVT_FACADEEDIT);

			}
			return m_enabled;
		}
	}
}

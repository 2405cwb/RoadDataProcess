#include "StdAfx.h"
#include "hd3DToolRotCentreSelect.h"
#include "hd3DView.h"
#include "hd3DCamera.h"
#include "hdCommandDef.h"
#include "..\hdFramework\userMessage.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	namespace fm
	{
		CHd3DToolRotCentreSelect::CHd3DToolRotCentreSelect(void)
			:CHdTool("SelectRotateCentre","选择旋转中心","SelectRotateCentre","选择旋转中心",E_HCT_3D )
		{
			m_app = NULL;
			m_id = COMMAND_3D_ROTATECENTRESEL;
			m_type = E_HCT_3D | E_HCT_CLASSIFY;
		}


		CHd3DToolRotCentreSelect::~CHd3DToolRotCentreSelect(void)
		{
		}

		void CHd3DToolRotCentreSelect::OnMouseDown( int Button, int Shift, int X, int Y )
		{
		}

		void CHd3DToolRotCentreSelect::OnMouseUp( int Button, int Shift, int X, int Y )
		{
			if (m_app->GetActiveView() == NULL)
			{
				return;
			}
			if (m_app->GetActiveView()->GetViewType() == E_HVT_3D  ||
				m_app->GetActiveView()->GetViewType() == E_HVT_MULTISCAN3D ||
                m_app->GetActiveView()->GetViewType() == E_HVT_SKETCH_ISCAN3D ||
				m_app->GetActiveView()->GetViewType() == E_HVT_MLS3D || 
				m_app->GetActiveView()->GetViewType() == E_HVT_FACADEEDIT ||
				m_app->GetActiveView()->GetViewType() == E_HVT_DOM_3D_VIEW)
			{

				CHd3DView* hd3dView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
				
				CHd3DCamera* p3DCamera = dynamic_cast<CHd3DCamera*>(m_app->GetTool(COMMAND_3D_CAMERA));
				if (p3DCamera)
				{
					if(p3DCamera->SetTargetByView(X,Y))
					{
						irr::scene::ISceneManager* smgr = hd3dView->GetSceneManager();
						ICameraSceneNode* camera = smgr->getActiveCamera();
						camera->setTarget(p3DCamera->GetRotateCenter());
						m_app->SetCurrentTool(COMMAND_3D_CAMERA);
						
						hd3dView->SetRotateCentre(p3DCamera->GetRotateCenter());

						irr::core::array<ISceneNode*> aryList;
						hd3dView->GetSceneManager()->getSceneNodeFromType(ESNT_ROTATE_CIRCLE);
						// 如果当前视图中存在旋转圈，则更新旋转圈的圆心 并将当前工具设置为测站旋转工具
						if (hd3dView->IsIncludeSceneNode(ESNT_ROTATE_CIRCLE)/* && hd3dView->GetSceneManager()
							&& hd3dView->GetSceneManager()->getSceneNodeFromType(ESNT_ROTATE_CIRCLE) && 
							hd3dView->GetSceneManager()->getSceneNodeFromType(ESNT_ROTATE_CIRCLE)->isVisible()*/)
						{
							// 发送消息至视图
							hd3dView->SendMsgToWindowsView(WM_USER_UPDATE_ROTATECENTER,0,0);
						}

						hd3dView->SendMsgToWindowsView(WM_USER_SET_COMMAND_TOOL,(int)COMMAND_3D_CAMERA,0);
						hd3dView->Refresh();
						
					}

					m_app->SetCurrentTool(COMMAND_3D_CAMERA);
				}				
			}
		}

		void CHd3DToolRotCentreSelect::OnClick()
		{
			if (!getEnable())
				return;

			/*CHdTool* pTool = m_app->GetCurrentTool();*/
			m_checked = true;
		}

		void CHd3DToolRotCentreSelect::OnCreate( CHdApp* app )
		{
			if(app == NULL)
				return;
			m_app = app;
		}

		bool CHd3DToolRotCentreSelect::getEnable()
		{
			m_enabled = false;
			if (m_app && m_app->GetActiveView() != NULL)
			{
				m_enabled = (m_app->GetActiveView()->GetViewType() == E_HVT_3D  ||
					m_app->GetActiveView()->GetViewType() == E_HVT_MULTISCAN3D ||
                    m_app->GetActiveView()->GetViewType() == E_HVT_SKETCH_ISCAN3D ||
					m_app->GetActiveView()->GetViewType() == E_HVT_MLS3D || 
					m_app->GetActiveView()->GetViewType() == E_HVT_FACADEEDIT ||
					m_app->GetActiveView()->GetViewType() == E_HVT_DOM_3D_VIEW);
			}
			return m_enabled;
		}

		void CHd3DToolRotCentreSelect::Deactivate()
		{
			m_checked = false;
		}

		bool CHd3DToolRotCentreSelect::OnContextMenu( int X, int Y )
		{
			return false;
		}

		void CHd3DToolRotCentreSelect::OnDblClick( int Button, int Shift, int X, int Y )
		{

		}

		void CHd3DToolRotCentreSelect::OnKeyDown( int keyCode, int Shift )
		{

		}

		void CHd3DToolRotCentreSelect::OnKeyUp( int keyCode, int Shift )
		{

		}

		void CHd3DToolRotCentreSelect::OnMouseMove( int Button, int Shift, int X, int Y )
		{
			
		}

		void CHd3DToolRotCentreSelect::OnMouseWheel( UINT nFlags, short zDelta, int X, int Y )
		{
			
		}



	}
}
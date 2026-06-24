#include "stdafx.h"
#include "hdApp.h"
#include "hdTool.h"
#include "hdCommandDef.h"
#include "userMessage.h"
#include "HdGdalRegister.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

using namespace hd;

#ifndef MK_ALT
#define	MK_ALT	( 0x20 )//0x0001
#endif

namespace hd
{
	namespace fm
	{

		typedef pair <int, CHdCommand*> Command_Pair;

		CHdApp::CHdApp(void)
			:m_activeView(NULL)
			,m_currentTool(NULL)
			,m_PreTool(NULL)
			,m_FurTool(NULL)
			,LoadCallback(NULL)
			,LogCallback(NULL)
			,m_hWnd(NULL)
		{
			// 注册GDAL驱动
			CHdGdalRegister::RegisterGDALDriver();
		}

		CHdApp::~CHdApp()
		{
			CHdGdalRegister::UnRegisterGDALDriver();
			//DeleteAllCommand();
		}

		/*void CHdApp::DeleteAllCommand()
		{
		std::map <int, CHdCommand*>::iterator pIter;
		for ( pIter = m_cmdList.begin( ); pIter != m_cmdList.end( ); pIter++ )
		{
		if (pIter->second != NULL)
		{
		delete pIter->second;
		pIter->second = NULL;
		}
		}
		m_cmdList.clear();
		}*/

		int CHdApp::GetPreToolID() const
		{
			return (NULL!=m_PreTool?m_PreTool->GetID():-1);
		}

		void CHdApp::SetCurrentTool( int id)
		{
			std::vector<CHdCommand*>::iterator pIter;
			for ( pIter = m_cmdList.begin( ); pIter != m_cmdList.end( ); pIter++ )
			{
				if ((*pIter) && (*pIter)->GetID() == id)
				{
					CHdTool* tool = dynamic_cast<CHdTool*>((*pIter));
					if (tool)
					{
						SetCurrentTool(tool);
					}
					break;
				}
			}
		}

		void CHdApp::ExcuteCommand(int id)
		{
			std::vector<CHdCommand*>::iterator pIter;
			for ( pIter = m_cmdList.begin( ); pIter != m_cmdList.end( ); pIter++ )
			{
				if ((*pIter) && (*pIter)->GetID() == id)
				{
					CHdCommand* cmd = dynamic_cast<CHdCommand*>((*pIter));
					if (cmd)
					{
						cmd->OnClick();
					}
				}
			}
		}

		void CHdApp::GetClientRect(int &height,int &width)
		{
			RECT rect;
			::GetClientRect(m_hWnd,&rect);
			height = rect.bottom - rect.top;
			width = rect.right - rect.left;
		}

		void CHdApp::SetCurrentTool(CHdTool* pTool)
		{
			if(pTool && m_currentTool != pTool)//m_currentTool != tool，在当前按钮上再次点击，不做任何操作
			{
				m_FurTool = pTool;

				//将原来的工具设置为非活动状态
				if (m_currentTool)
				{
					m_currentTool->Deactivate();
				}

				pTool->OnClick();
				// 在修改之前记住 [2015/07/02 luowenmin]
				m_PreTool=m_currentTool;
				//

				m_currentTool = pTool;
			}
		}

		CHdTool* CHdApp::GetCurrentTool()
		{
			return m_currentTool;
		}

		CHdTool* CHdApp::GetTool(int id)
		{
			/*if (m_cmdList[id])
			{
			CHdTool* tool = dynamic_cast<CHdTool*>(m_cmdList[id]);
			return tool;
			}*/

			std::vector<CHdCommand*>::iterator pIter;
			for ( pIter = m_cmdList.begin( ); pIter != m_cmdList.end( ); pIter++ )
			{
				if ((*pIter) && (*pIter)->GetID() == id)
				{
					CHdTool* tool = dynamic_cast<CHdTool*>(*pIter);
					return tool;
				}
			}

			return NULL;
		}

		void CHdApp::CloseView(IHdView* pView )
		{
			if (pView == NULL)
				return;			
			for (vector<IHdView*>::iterator it = m_viewList.begin() ;it != m_viewList.end();it++)
			{
				if ((*it) == pView)
				{
					if (GetCurrentTool())
					{
						GetCurrentTool()->Deactivate();
					}

					delete (*it);
					//m_viewList[i] = NULL;
					m_viewList.erase(it);

					if (m_activeView == pView)
					{
						m_activeView = NULL;
					}
					break;
				}
			}
		}

		IHdView* CHdApp::GetActiveView()
		{
			return m_activeView;
		}

		IHdView* CHdApp::GetOrthoSliceView()
		{
			for (int i = 0; i < m_viewList.size(); i++)
			{
				if (strcmp(m_viewList[i]->GetName(), "SliceView") == 0)
				{
					return m_viewList[i];
				}
			}
			return NULL;
		}

		IHdView* CHdApp::GetOrthoVerticalView()
		{
			for (int i = 0; i < m_viewList.size(); i++)
			{
				if (strcmp(m_viewList[i]->GetName(), "VerticalView") == 0)
				{
					return m_viewList[i];
				}
			}
			return NULL;
		}

		IHdView* CHdApp::GetZoomOutView()
		{
			for (int i = 0; i < m_viewList.size(); i++)
			{
				if (strcmp(m_viewList[i]->GetName(), "ZoomView") == 0)
				{
					return m_viewList[i];
				}
			}
			return NULL;
		}

		// 根据视图名称查找视图
		IHdView* CHdApp::GetViewByName(const char* name)
		{

			for (int i = 0; i < m_viewList.size(); i++)
			{
				if (strcmp(m_viewList[i]->GetName(), name) == 0)
				{
					return m_viewList[i];
				}
			}
			return NULL;

		}

		void CHdApp::SetActiveView(IHdView* pView )
		{
			if (m_activeView != pView)
			{
				m_activeView = pView;
				//在拼接或全景控制点选择时,切换视图时不切换当前工具
				if (pView->GetViewType() == E_HVT_PICTUREMATCH || pView->GetViewType() == E_HVT_PLANARMATCH  
					|| pView->GetViewType() == E_HVT_REG || pView->GetViewType() == E_HVT_REG_QUICK)
				{
					return;
				}

				if (pView->GetViewType() == E_HVT_MLS3D && (m_currentTool && m_currentTool->GetID() == COMMAND_3D_PANO_SELCTRLPT))
				{
					return;
				}

				//将当前按钮状态清除
				if (m_currentTool)
				{
					m_currentTool->Deactivate();
					m_currentTool = NULL;
				}
				//根据视图类型,自动设置相应浏览工具
				if(pView->GetViewType() == E_HVT_3D 
					|| pView->GetViewType() == E_HVT_MULTISCAN3D 
					|| pView->GetViewType() == E_HVT_SKETCH_ISCAN3D || pView->GetViewType() == E_HVT_MLS3D )
				{
					//视图切换后，将默认工具设置为漫游工具
					SetCurrentTool(COMMAND_3D_CAMERA);
				}
				else if(pView->GetViewType() == E_HVT_PLANAR || pView->GetViewType() == E_HVT_PICTURE )
				{
					//视图切换后，将默认工具设置为漫游工具
					SetCurrentTool(COMMAND_PLANAR_PAN);
				}
				else if(pView->GetViewType() == E_HVT_QUICK )
				{
					//视图切换后，将默认工具设置为漫游工具
					SetCurrentTool(COMMAND_QUICK_CAMERA);
				}
				else if (pView->GetViewType() == E_HVT_OVL3D )
				{
					//视图切换后，将默认工具设置为漫游工具
					SetCurrentTool(COMMAND_OVERVIEW_PAN);
				}
				else if (pView->GetViewType() == E_HCT_Tile )
				{
					//视图切换后，将默认工具设置为漫游工具
					SetCurrentTool(COMMAND_TILE_CAMERA);
				}
				m_activeView->Refresh();
			}
		}


		void CHdApp::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
		{

		}

		IHdView* CHdApp::ViewExist(ENUM_HD_VIEW_TYPE viewType)
		{
			//判断当前是否存在指定类型的View
			IHdView* phdView = NULL;			
			for (unsigned int i = 0;i< m_viewList.size();i++)
			{
				if(m_viewList[i]->GetViewType() == viewType)
				{
					phdView = m_viewList[i];					
					break;
				}
			}			

			return phdView;
		}

		void CHdApp::RemoveAllViews()
		{
			int viewCount = m_viewList.size();
			for (int i = 0;i < viewCount;i++)
			{
				int j = m_viewList.size();
				if (j > 0)
				{
					m_viewList[j-1]->SendMsgToWindowsView(WM_USER_CLOSEVIEW,0,LPARAM(0));//ID_FILE_CLOSE
				}
			}
		}

		void CHdApp::RemoveTypeView(ENUM_HD_VIEW_TYPE viewType)
		{
			for (unsigned int i = 0;i< m_viewList.size();i++)
			{
				if(m_viewList[i]->GetViewType() == viewType)
				{
					m_viewList[i]->SendMsgToWindowsView(WM_USER_CLOSEVIEW,0,LPARAM(0));
				}
			}	

		}
	}
}
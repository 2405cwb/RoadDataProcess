#include "hnApp.h"
#include "hnTool.h"
#include "hnCommandDef.h"
#include <windows.h>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

#ifndef MK_ALT
#define	MK_ALT	( 0x20 )//0x0001
#endif

namespace hnApp
{
	typedef pair <int, hnCommand*> Command_Pair;

	hnApp::hnApp(void)
		:m_activeView(NULL)
		, m_currentTool(NULL)
		, m_PreTool(NULL)
		, m_FurTool(NULL)
		, LoadCallback(NULL)
		, LogCallback(NULL)
		, m_hWnd(NULL)
	{

	}

	hnApp::~hnApp()
	{

	}


	int hnApp::getPreToolID() const
	{
		return (NULL != m_PreTool ? m_PreTool->getID() : -1);
	}

	void hnApp::setCurrentTool(int id)
	{
		std::vector<hnCommand*>::iterator pIter;
		for (pIter = m_cmdList.begin(); pIter != m_cmdList.end(); pIter++)
		{
			if ((*pIter) && (*pIter)->getID() == id)
			{
				hnTool* tool = dynamic_cast<hnTool*>((*pIter));
				if (tool)
				{
					setCurrentTool(tool);
				}
				break;
			}
		}
	}

	void hnApp::excuteCommand(int id)
	{
		std::vector<hnCommand*>::iterator pIter;
		for (pIter = m_cmdList.begin(); pIter != m_cmdList.end(); pIter++)
		{
			if ((*pIter) && (*pIter)->getID() == id)
			{
				hnCommand* cmd = dynamic_cast<hnCommand*>((*pIter));
				if (cmd)
				{
					cmd->onClick();
				}
			}
		}
	}

	void hnApp::getClientRect(int &height, int &width)
	{
		RECT rect;
		::GetClientRect(m_hWnd, &rect);
		height = rect.bottom - rect.top;
		width = rect.right - rect.left;
	}

	void hnApp::setCurrentTool(hnTool* pTool)
	{
		if (pTool && m_currentTool != pTool)//m_currentTool != tool，在当前按钮上再次点击，不做任何操作
		{
			m_FurTool = pTool;

			//将原来的工具设置为非活动状态
			if (m_currentTool)
			{
				m_currentTool->deactivate();
			}

			pTool->onClick();
			// 在修改之前记住 [2015/07/02 luowenmin]
			m_PreTool = m_currentTool;
			//

			m_currentTool = pTool;
		}
	}

	hnTool* hnApp::getCurrentTool()
	{
		return m_currentTool;
	}

	hnTool* hnApp::getTool(int id)
	{
		/*if (m_cmdList[id])
		{
		CHdTool* tool = dynamic_cast<CHdTool*>(m_cmdList[id]);
		return tool;
		}*/

		std::vector<hnCommand*>::iterator pIter;
		for (pIter = m_cmdList.begin(); pIter != m_cmdList.end(); pIter++)
		{
			if ((*pIter) && (*pIter)->getID() == id)
			{
				hnTool* tool = dynamic_cast<hnTool*>(*pIter);
				return tool;
			}
		}

		return NULL;
	}

	void hnApp::closeView(hnView* pView)
	{
		if (pView == NULL)
			return;
		for (vector<hnView*>::iterator it = m_viewList.begin(); it != m_viewList.end(); it++)
		{
			if ((*it) == pView)
			{
				if (getCurrentTool())
				{
					getCurrentTool()->deactivate();
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

	hnView* hnApp::getActiveView()
	{
		return m_activeView;
	}

	// 根据视图名称查找视图
	hnView* hnApp::getViewByName(const char* name)
	{

		for (int i = 0; i < m_viewList.size(); i++)
		{
			if (strcmp(m_viewList[i]->getViewName().toLocal8Bit(), name) == 0)
			{
				return m_viewList[i];
			}
		}
		return NULL;

	}

	void hnApp::setActiveView(hnView* pView)
	{
		if (m_activeView != pView)
		{
			m_activeView = pView;


			//将当前按钮状态清除
			if (m_currentTool)
			{
				m_currentTool->deactivate();
				m_currentTool = NULL;
			}

			// 设置工具

			m_activeView->refreshView();
		}
	}


	void hnApp::windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{

	}
}

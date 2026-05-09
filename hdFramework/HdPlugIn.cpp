#include "StdAfx.h"
#include "HdPlugIn.h"

namespace hd
{
	namespace fm
	{
		CHdPlugIn::CHdPlugIn(void)
			:m_loadPlugin_func(NULL),m_unloadPlugin_func(NULL),m_get_hdCommand_count_func(NULL),
			m_get_hdCommand_func(NULL),m_get_hdToolbar_count_func(NULL),m_get_hdToolbar_func(NULL),m_newpane_func(NULL),
			m_get_hdDockWinCnt_func(NULL),m_get_hdDockWin_func(NULL),m_get_hdPlnName_func(NULL),m_pretranslate_hdPlnMsg_func(NULL),
			m_pluginHistance(NULL)
		{
			strcpy(m_plugInPath,"");
			strcpy(m_plnName,"");
		}

		CHdPlugIn::~CHdPlugIn(void)
		{
			UnloadPlugIn();
		}

		// 加载插件
		BOOL CHdPlugIn::LoadPlugIn(const char* path)
		{
			// 如果先前已经加载插件,则先卸载
			if (m_pluginHistance)
			{
				UnloadPlugIn();
			}

			// 插件dll句柄
			m_pluginHistance = ::LoadLibrary(path);

			// 获取插件API函数列表
			if (m_pluginHistance)
			{
				m_loadPlugin_func = (LPDLLFUNC_LOADPLUGIN)::GetProcAddress(m_pluginHistance,"LoadPlugIn");
				m_unloadPlugin_func = (LPDLLFUNC_UNLOADPLUGIN)::GetProcAddress(m_pluginHistance,"UnloadPlugIn");
				m_get_hdCommand_count_func = (LPDLLFUNC_GET_HDCOMMANDCNT)::GetProcAddress(m_pluginHistance,"GetHdCommandCount");
				m_get_hdCommand_func = (LPDLLFUNC_GET_HDCOMMAND)::GetProcAddress(m_pluginHistance,"GetHdCommand");
				m_get_hdToolbar_count_func = (LPDLLFUNC_GET_HDTOOLBARCNT)::GetProcAddress(m_pluginHistance,"GetToolbarCount");
				m_get_hdToolbar_func = (LPDLLFUNC_GET_HDTOOLBAR)::GetProcAddress(m_pluginHistance,"GetToolBar");	
				m_newpane_func =  (LPDLLFUNC_NEWPANE)::GetProcAddress(m_pluginHistance,"NewPane");
				m_get_hdDockWinCnt_func = (LPDLLFUNC_GET_HDDOCKWINCNT)::GetProcAddress(m_pluginHistance,"GetDockWindowCount");
				m_get_hdDockWin_func    = (LPDLLFUNC_GET_HDDOCKWIN)::GetProcAddress(m_pluginHistance,"GetDockWindow");
				m_get_hdPlnName_func   = (LPDLLFUNC_GET_HDPLNNAME)::GetProcAddress(m_pluginHistance,"GetHdPlnName");
				m_pretranslate_hdPlnMsg_func = (LPDLLFUNC_PRETRANSLATE_HDPLNMSG)::GetProcAddress(m_pluginHistance,"PreTranslateHdPlnMessage");
			}
			if (m_loadPlugin_func == NULL ||
				m_unloadPlugin_func == NULL ||
				m_get_hdCommand_count_func == NULL ||
				m_get_hdCommand_func == NULL ||
				m_get_hdToolbar_count_func == NULL ||
				m_get_hdToolbar_func == NULL ||
				m_newpane_func == NULL ||
				m_get_hdDockWinCnt_func == NULL || 
				m_get_hdDockWin_func == NULL||
				m_get_hdPlnName_func == NULL||
				m_pretranslate_hdPlnMsg_func == NULL)
			{
				UnloadPlugIn();
				return FALSE;
			}

			BOOL bRet = FALSE;
			if (m_loadPlugin_func)
			{
				bRet = m_loadPlugin_func(path);
				if (bRet)
				{
					// 加载插件成功,保存插件路径
					strcpy(m_plugInPath,path);

					// 保存插件的名字，用于后续比较判断
					strcpy(m_plnName,m_get_hdPlnName_func());

					if (m_plnName == "") // 如果插件名字为空、返回失败
					{
						bRet = FALSE; 
					}
				}
			}
			return bRet;
		}

		// 卸载插件
		void CHdPlugIn::UnloadPlugIn()
		{
			// 调用插件dll释放相关资源
			if (m_unloadPlugin_func)
			{
				m_unloadPlugin_func();
			}

			m_loadPlugin_func = NULL;
			m_unloadPlugin_func = NULL;
			m_get_hdCommand_count_func = NULL;
			m_get_hdCommand_func = NULL;
			m_get_hdToolbar_count_func = NULL;
			m_get_hdToolbar_func = NULL;
			m_newpane_func = NULL;
			m_get_hdDockWinCnt_func = NULL;
			m_get_hdDockWin_func = NULL;
			m_get_hdPlnName_func = NULL;
			m_pretranslate_hdPlnMsg_func = NULL;
			if (m_pluginHistance)
			{
				BOOL freeresult =  ::FreeLibrary(m_pluginHistance);
				m_pluginHistance = NULL;
			}

			strcpy(m_plugInPath,"");
			strcpy(m_plnName,"");
		}

		// 获取命令个数
		int CHdPlugIn::GetHdCommandCount()
		{
			if(m_get_hdCommand_count_func)
			{
				return m_get_hdCommand_count_func();
			}
			else
			{
				return 0;
			}
		}

		// 获取一个命令
		CHdCommand* CHdPlugIn:: GetHdCommand(int index)
		{
			if (m_get_hdCommand_func)
			{
				return m_get_hdCommand_func(index);
			}
			else
			{
				return NULL;
			}
		}

		// 根据资源ID获取一个命令
		CHdCommand* CHdPlugIn::GetHdCommandByRcID(int rcID)
		{
			char cmdId[256] = {0};
			BOOL bFind = FALSE;
			int toolBarCount = GetToolbarCount();
			for (int i = 0;i < toolBarCount;i++)
			{
				CHdToolBarDef* pToolBarDef = GetToolBar(i);
				if (pToolBarDef == NULL)
				{
					continue;
				}
				int cmdCount = pToolBarDef->GetItemCount();
				for (int j = 0;j < cmdCount;j++)
				{
					CHdItemDef* pItemDef = pToolBarDef->GetItemInfo(j);
					if (pItemDef->GetRcID() == rcID)
					{
						bFind = TRUE;
						strcpy(cmdId,pItemDef->GetCmdID());
						break;
					}
				}
				if (bFind)
				{
					break;
				}
			}

			CHdCommand* pFindedCmd = NULL;
			int cmdCound = GetHdCommandCount();
			for (int i = 0;i < cmdCound;i++)
			{
				CHdCommand* pHdCmd = GetHdCommand(i);
				if (strcmp(pHdCmd->GetCmdID().c_str(),cmdId) == 0)
				{
					pFindedCmd = pHdCmd;
					break;
				}
			}

			return pFindedCmd;
		}

		// 根据命令ID获取一个命令
		CHdCommand* CHdPlugIn::GetHdCommandByCmdID(const char* strCmdID)
		{			
			CHdCommand* pFindedCmd = NULL;
			int cmdCound = GetHdCommandCount();
			for (int i = 0;i < cmdCound;i++)
			{
				CHdCommand* pHdCmd = GetHdCommand(i);
				if (strcmp(pHdCmd->GetCmdID().c_str(),strCmdID) == 0)
				{
					pFindedCmd = pHdCmd;
					break;
				}
			}

			return pFindedCmd;
		}

		//获取工具栏个数
		int CHdPlugIn::GetToolbarCount()
		{
			if (m_get_hdToolbar_count_func)
			{
				return m_get_hdToolbar_count_func();
			}
			else
			{
				return 0;
			}
		}

		// 获取面板个数
		int CHdPlugIn::GetPaneCount()
		{
			if (m_get_hdDockWinCnt_func)
			{
				return m_get_hdDockWinCnt_func();
			}
			else
			{
				return 0;
			}
		}

		// 获取面板
		CHdDockWinDef* CHdPlugIn::GetDockWin(int index)
		{
			if (m_get_hdToolbar_func)
			{
				return m_get_hdDockWin_func(index);
			}
			else
			{
				return NULL;
			}
		}

		// 获取工具栏
		CHdToolBarDef* CHdPlugIn::GetToolBar(int index)
		{
			if (m_get_hdToolbar_func)
			{
				return m_get_hdToolbar_func(index);
			}
			else
			{
				return NULL;
			}
		}

		BOOL CHdPlugIn::PreTranslateMessage(MSG* pMsg)
		{
			if (m_pretranslate_hdPlnMsg_func)
			{
				return m_pretranslate_hdPlnMsg_func(pMsg);
			}
			else
			{
				return FALSE;
			}
		}
	}

}


/*! HdPlugIn.h
********************************************************************************
<PRE>
模块名       : hdFramework
文件名       : HdPlugIn.h
相关文件     : 
文件实现功能 : 插件类
作者         : 蔡红云、龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/02/16   1.0      蔡红云、龚书林  
</PRE>
*******************************************************************************/

#pragma once
#include "hdCommand.h"
#include "hdTool.h"
#include "hdResourceBase.h"
#include "HdDockWinDef.h"
using namespace hd::fm;

// 加载插件函数指针
typedef BOOL (*LPDLLFUNC_LOADPLUGIN)(const char*);

// 卸载插件函数指针
typedef void (*LPDLLFUNC_UNLOADPLUGIN)();

// 获取命令个数函数指针
typedef int (*LPDLLFUNC_GET_HDCOMMANDCNT)();

// 获取一个命令函数指针
typedef CHdCommand* (*LPDLLFUNC_GET_HDCOMMAND)(int);

// 获取一个工具函数指针
typedef CHdTool* (*LPDLLFUNC_GET_HDTOOL)(int);

// 获取工具个数函数指针
typedef int (*LPDLLFUNC_GET_HDTOOLCNT)();

// 获取工具栏个数函数指针
typedef int (*LPDLLFUNC_GET_HDTOOLBARCNT)();

// 获取工具栏函数指针
typedef CHdToolBarDef* (*LPDLLFUNC_GET_HDTOOLBAR)(int);

// 获取新建一个面板
typedef CHdDockWinDef* (*LPDLLFUNC_NEWPANE)();

// 获取面板个数函数指针
typedef int (*LPDLLFUNC_GET_HDDOCKWINCNT)();

// 获取面板函数指针
typedef CHdDockWinDef* (*LPDLLFUNC_GET_HDDOCKWIN)(int);

// 获取插件的名字
typedef const char* (*LPDLLFUNC_GET_HDPLNNAME)();

// 获取消息分发的接口，解决插件中非模态对话框编辑框回车响应
typedef BOOL (*LPDLLFUNC_PRETRANSLATE_HDPLNMSG)(MSG* pMsg);


namespace hd
{
	namespace fm
	{
		class HDFRAMEWORK_API CHdPlugIn
		{
			friend class CPluginManager;
		private:
			// 加载插件函数指针
			LPDLLFUNC_LOADPLUGIN        m_loadPlugin_func;

			// 卸载插件函数指针
			LPDLLFUNC_UNLOADPLUGIN		m_unloadPlugin_func;

			// 获取命令个数函数指针
			LPDLLFUNC_GET_HDCOMMANDCNT	m_get_hdCommand_count_func;

			// 获取命令函数指针
			LPDLLFUNC_GET_HDCOMMAND     m_get_hdCommand_func;

			// 获取工具栏个数函数指针
			LPDLLFUNC_GET_HDTOOLBARCNT  m_get_hdToolbar_count_func;

			// 获取工具栏
			LPDLLFUNC_GET_HDTOOLBAR     m_get_hdToolbar_func;

			// 创建pane
			LPDLLFUNC_NEWPANE           m_newpane_func;

			// 获取面板个数函数指针
			LPDLLFUNC_GET_HDDOCKWINCNT  m_get_hdDockWinCnt_func;

			// 获取面板函数指针
			LPDLLFUNC_GET_HDDOCKWIN     m_get_hdDockWin_func;

			// 获取插件名字
			LPDLLFUNC_GET_HDPLNNAME     m_get_hdPlnName_func;

			// 消息分发 
			LPDLLFUNC_PRETRANSLATE_HDPLNMSG  m_pretranslate_hdPlnMsg_func;     
					
			// 插件文件路径
			char						m_plugInPath[256];

			// 插件名称，加载插件时的唯一区分，字符串标识，命名格式为时间+插件名称如
			// 20140901SlopMonitor
			char                      m_plnName[256];

			// 插件DLL句柄HINSTANCE,LoadLibrary后获得
			HINSTANCE					m_pluginHistance;
			

		public:

			CHdPlugIn(void);

			~CHdPlugIn(void);

			// 消息传递
			void WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

			// 获取插件路径
			char* GetPath()
			{
				return m_plugInPath;
			}

			// 加载插件,初始化插件内部命令、工具、菜单
			BOOL LoadPlugIn(const char* path);

			// 卸载插件
			void UnloadPlugIn();

			// 获取命令个数
			int GetHdCommandCount();

			// 获取一个命令,index命令序号,从0开始
			CHdCommand* GetHdCommand(int index);

			// 根据资源ID获取一个命令
			CHdCommand* GetHdCommandByRcID(int rcID);

			// 根据命令ID获取一个命令
			CHdCommand* GetHdCommandByCmdID(const char* strCmdID);

			//获取工具栏个数
			int GetToolbarCount();

			// 获取工具栏
			CHdToolBarDef* GetToolBar(int index);

			// 新建面板
			CHdDockWinDef* NewPane()
			{
				return m_newpane_func();
			}

			// 获取面板个数
			int GetPaneCount();

			// 获取面板
			CHdDockWinDef* GetDockWin(int index);

			// 消息分发
			BOOL PreTranslateMessage(MSG* pMsg);

		};
	}
}


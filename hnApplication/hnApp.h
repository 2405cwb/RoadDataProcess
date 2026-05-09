#ifndef HNAPP_H
#define HNAPP_H
#include "hnapplication_global.h"
#pragma  warning(disable:4251)
#include <vector>
#include <map>
#include "hnView.h"


using namespace std;

namespace hnApp
{
	class hnCommand;
	class hnTool;

	//! 应用程序单实例对象
	class HNAPPLICATION_EXPORT hnApp
	{
	public:
		hnApp(void);
		virtual ~hnApp();

	protected:
		//! 视图对象列表
		vector<hnView*> m_viewList;

		vector<hnCommand*> m_cmdList;

		//! 当前处于激活状态的视图
		hnView*			m_activeView;

		//!当前工程路径
		const char*			m_filepath;

		//! 当前工具
		hnTool*			m_currentTool;

		// !前一使用工具
		hnTool* m_PreTool;

		// 即将设置为当前工具的工具
		hnTool* m_FurTool;

		//! 主窗口
		HWND			m_hWnd;

		// 方法
	public:

		//! 系统加载回调函数,第一个参数0.0-1.0表示百分比,第二个参数表示提示信息
		void(*LoadCallback)(float, const char*);

		//! 系统日志回调函数,第一个参数表示错误代码
		void(*LogCallback)(int errCode);

		//! 设置主窗口
		virtual void setMainWnd(HWND hWnd) { m_hWnd = hWnd; }

		//! 获取主窗口句柄
		HWND getMainWnd() { return m_hWnd; }

		//! 注册一个命令
		virtual void registerCommand(hnCommand* cmd) {}

		//! 设置当前工具
		virtual void setCurrentTool(int id);

		//! 设置当前工具,工具由外部创建
		virtual void setCurrentTool(hnTool* pTool);

		void excuteCommand(int id);

		//! 获取当前工具
		hnTool* getCurrentTool();

		//! 获取先前使用的工具对象
		hnTool* getPreTool() const
		{
			return m_PreTool;
		}

		//获取即将设置为当前工具的工具
		hnTool* getFurTool()
		{
			return m_FurTool;
		}

		// 获取早前使用工具ID
		int getPreToolID() const;

		//! 根据工具ID获取工具
		hnTool* getTool(int id);

		//! 获取客户区的大小
		void getClientRect(int &height, int &width);

		//! 关闭视图
		virtual void closeView(hnView* pView);

		//! 设置活动视图
		virtual void setActiveView(hnView* pView);

		//! 获取当前活动状态
		virtual hnView* getActiveView();

		// 根据视图名称找到关联视图
		hnView* getViewByName(const char* name);


	protected:
		//! 注册命令
		virtual void registerBuildInCommand() {}
	private:

	public:
		//! 窗口消息处理
		virtual void windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;
	};
}

#endif

/*! hdApp.h
********************************************************************************
<PRE>
模块名       : hdFramework
文件名       : hdApp.h
相关文件     : 
文件实现功能 : 应用程序基类应用程序对象,它包含多个视图，其中有一个是活动视图，
同时包含一个工作空间对象，已经一系列工具，其中有一个当前工具
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/06/19   1.0      龚书林   
2013/02/27	 2.0	  危迟					
</PRE>
*******************************************************************************/

#ifndef HDAPP_H
#define HDAPP_H
#pragma  warning(disable:4251)
#include "stdafx.h"
#include <vector>
#include <map>
#include "hdView.h"

using namespace std;
using namespace hd::scene;

namespace hd
{
	//class PointCloud;

	namespace fm
	{
		class CHdCommand;
		class CHdTool;

		//! 应用程序单实例对象
		class HDFRAMEWORK_API CHdApp
		{
		public:
			CHdApp(void);
			virtual ~CHdApp();

			//属性
		private:
			//! 应用程序实例
			//static hdApp* m_pHdApp;

		protected:
			//! 视图对象列表
			vector<IHdView*> m_viewList;
			//! 命令列表
			//map<int,CHdCommand*> m_cmdList;
			vector<CHdCommand*> m_cmdList;
			//! 当前处于激活状态的视图
			IHdView*			m_activeView;
			//!当前工程路径
			const char*			m_filepath;
			//! 当前工具
			CHdTool*			m_currentTool;
			// !前一使用工具 [2015/07/02 luowenmin]
			CHdTool* m_PreTool;
			
			// 即将设置为当前工具的工具
			CHdTool* m_FurTool;

			//! 主窗口
			HWND			m_hWnd;

			// 方法
		public:

			//! 系统加载回调函数,第一个参数0.0-1.0表示百分比,第二个参数表示提示信息
			void (*LoadCallback)(float,const char*);

			//! 系统日志回调函数,第一个参数表示错误代码
			void (*LogCallback)(int errCode);

			//! 设置主窗口
			virtual void SetMainWnd(HWND hWnd){m_hWnd = hWnd;}

			//! 获取主窗口句柄
			HWND GetMainWnd(){return m_hWnd;}

			//! 移除指定类型视图
			virtual void RemoveTypeView(ENUM_HD_VIEW_TYPE viewType);

			//! 获取指定视图类型
			virtual IHdView* ViewExist(ENUM_HD_VIEW_TYPE viewType);

			//关闭所有的视图窗口
			virtual void RemoveAllViews();

			//! 注册一个命令
			virtual void RegisterCommand(CHdCommand* cmd){}

			//根据测站索引,获取点云数据,如果autoLoad=true内存不存在则自动加载  gsl-2012/06/30
			//bTrans = false，表示加载点云过程中，是否对坐标进行转换。只有在多测站3D视图下，才进行转换
			//virtual PointCloud* GetPointCloud(int index, //测站索引
			//	bool autoLoad = true,			 //如果内存没有,是否自动从文件加载
			//	bool bOnlyHeader = false) = 0;		 //是否仅仅加载点云文件头信息

			//! 设置当前工具
			virtual void SetCurrentTool(int id);

			//! 设置当前工具,工具由外部创建
			virtual void SetCurrentTool(CHdTool* pTool);

			void ExcuteCommand(int id);
			//! 获取当前工具
			CHdTool* GetCurrentTool();

			//! 获取先前使用的工具对象  [2015/07/02 luowenmin]
			CHdTool* GetPreTool() const
			{
				return m_PreTool;
			}

			//获取即将设置为当前工具的工具[2015-12-21 chy]
			CHdTool* GetFurTool()
			{
				return m_FurTool;
			}

			// 获取早前使用工具ID [2015/07/02 luowenmin]
			int GetPreToolID() const;

			//

			//! 根据工具ID获取工具
			CHdTool* GetTool(int id);

			//! 获取客户区的大小
			void GetClientRect(int &height,int &width);

			//! 关闭视图
			virtual void CloseView(IHdView* pView);

			//! 设置活动视图
			virtual void SetActiveView(IHdView* pView);

			//! 获取当前活动状态
			virtual IHdView* GetActiveView();

			//! 获取剖面视图 
			IHdView* GetOrthoSliceView();

			//! 获取俯视图
			IHdView* GetOrthoVerticalView();

			//! 获取放大镜视图
			IHdView* GetZoomOutView();

			// 根据视图名称找到关联视图--chy--2015-9-13
			IHdView* GetViewByName(const char* name);


		protected:
			//! 注册命令
			virtual void RegisterBuildInCommand(){}
		private:

			//! 删除所有命令
			//void DeleteAllCommand();

		public:
			//! 窗口消息处理
			virtual void WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;
		};
	}
}

#endif
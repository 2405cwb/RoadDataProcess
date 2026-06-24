/*! hdTool.h
********************************************************************************
<PRE>
模块名       : hdFramework
文件名       : hdTool.h
相关文件     : 
文件实现功能 : 鼠标交互工具接口定义 
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/06/20   1.0      龚书林    
2013/02/27	 2.0	  危迟					抽象类
</PRE>
*******************************************************************************/

#pragma once

#include "stdafx.h"
#include "hdCommand.h"
#include "hdView.h"
using namespace hd::scene;
namespace hd
{
	namespace fm
	{
		//! shift,alt,control状态
		enum ENUM_HD_PRESS_STATE
		{
			E_HPS_SHIFT			= 0x1,				// shift键按住
			E_HPS_CONTROL		= 0x2,				// control键按住
			E_HPS_ALT			= 0x4,				// alt键按住
		};

		class HDFRAMEWORK_API CHdTool: public CHdCommand
		{
		protected:
			//! 鼠标光标
			HCURSOR m_hdcursor;
			
			//! 关联视图
			IHdView* m_pRelateView;
			
			//! 是否共享工具
			bool m_bShareTool;
		public:
			CHdTool()
				:m_hdcursor(NULL),m_pRelateView(NULL),m_bShareTool(false){}
			CHdTool(const char* caption,
				const char* msg,
				const char* name,
				const char* tooltip,
				int type)
				:CHdCommand(caption,msg,name,tooltip,type)
			     ,m_pRelateView(NULL),m_bShareTool(false){}
			
			virtual ~CHdTool(){}

			//! 当前工具处于非激活状态
			virtual void Deactivate() = 0;
			//! 当前工具的右键菜单
			virtual bool OnContextMenu(int X, int Y){return false;}
			//! 鼠标双击
			virtual void OnDblClick(
				int Button, //1左键,2右键,4中键
				int Shift,  //1代表SHIFT,2代表CTRL,4代表ALT,1+4代表SHIFT+ALT,...	
				int X, int Y){}
			
			//! 键盘按键按下
			virtual void OnKeyDown(
				int keyCode,	//键盘对应的ASCII值,F1~F12为112~123
				int Shift){}	//1代表SHIFT,2代表CTRL,4代表ALT,1+4代表SHIFT+ALT,...
			//! 键盘按键弹起
			virtual void OnKeyUp(
				int keyCode,	 //键盘对应的ASCII值,F1~F12为112~123
				int Shift){}     //1代表SHIFT,2代表CTRL,4代表ALT,1+4代表SHIFT+ALT,...

			//! 鼠标按下
			virtual void OnMouseDown(
				int Button, //1左键,2右键,4中键
				int Shift,  //1代表SHIFT,2代表CTRL,4代表ALT,1+4代表SHIFT+ALT,...
				int X, int Y){}

			//! 鼠标移动
			virtual void OnMouseMove(
				int Button, //1左键,2右键,4中键
				int Shift,  //1代表SHIFT,2代表CTRL,4代表ALT,1+4代表SHIFT+ALT,...	
				int X, int Y){}
			
			//! 鼠标释放
			virtual void OnMouseUp(
				int Button, //1左键,2右键,4中建
				int Shift,	//1代表SHIFT,2代表CTRL,4代表ALT,1+4代表SHIFT+ALT,...		
				int X, int Y){}

			//! 数据中键滚轮
			virtual void OnMouseWheel(
				UINT nFlags,	
				short zDelta,	
				int X, int Y){}

			// 鼠标离开视图 chy-2016-4-14
             virtual void OnMouseLeave(
				 int Button,
				 int Shift, 
				 int X, int Y){}   

			
			//! 获取工具光标
			virtual HCURSOR GetHCursor() const{return m_hdcursor;}
		};
	}
}
/*! hdTool.h
********************************************************************************
<PRE>
模块名       : hdApplication
文件名       : hnTool.h
相关文件     : 
文件实现功能 : 鼠标交互工具接口定义 
作者         : 李夏亮
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2022/03/11   1.0      李夏亮    
</PRE>
*******************************************************************************/

#pragma once
#include "hnCommand.h"
#include "hnView.h"

namespace hnApp
{
	//! shift,alt,control状态
	enum ENUM_HN_PRESS_STATE
	{
		E_HPS_SHIFT = 0x1,				// shift键按住
		E_HPS_CONTROL = 0x2,				// control键按住
		E_HPS_ALT = 0x4,				// alt键按住
	};

	class HNAPPLICATION_EXPORT hnTool : public hnCommand
	{
	protected:
		//! 鼠标光标
		HCURSOR m_hncursor;

		//! 是否共享工具
		bool m_bShareTool;
	public:
		hnTool()
			:m_hncursor(NULL), m_bShareTool(false) {}
		hnTool(const char* caption,
			const char* msg,
			const char* name,
			const char* tooltip)
			:hnCommand(caption, msg, name, tooltip)
			, m_bShareTool(false) {}

		virtual ~hnTool() {}

		//! 当前工具处于非激活状态
		virtual void deactivate() = 0;

		//! 当前工具的右键菜单
		virtual bool onContextMenu(int X, int Y) { return false; }
		//! 鼠标双击
		virtual void onDblClick(
			int Button, //1左键,2右键,4中键
			int Shift,  //1代表SHIFT,2代表CTRL,4代表ALT,1+4代表SHIFT+ALT,...	
			int X, int Y) {}

		//! 键盘按键按下
		virtual void onKeyDown(
			int keyCode,	//键盘对应的ASCII值,F1~F12为112~123
			int Shift) {}	//1代表SHIFT,2代表CTRL,4代表ALT,1+4代表SHIFT+ALT,...
							//! 键盘按键弹起
		virtual void onKeyUp(
			int keyCode,	 //键盘对应的ASCII值,F1~F12为112~123
			int Shift) {}     //1代表SHIFT,2代表CTRL,4代表ALT,1+4代表SHIFT+ALT,...

							  //! 鼠标按下
		virtual void onMouseDown(
			int Button, //1左键,2右键,4中键
			int Shift,  //1代表SHIFT,2代表CTRL,4代表ALT,1+4代表SHIFT+ALT,...
			int X, int Y) {}

		//! 鼠标移动
		virtual void onMouseMove(
			int Button, //1左键,2右键,4中键
			int Shift,  //1代表SHIFT,2代表CTRL,4代表ALT,1+4代表SHIFT+ALT,...	
			int X, int Y) {}

		//! 鼠标释放
		virtual void onMouseUp(
			int Button, //1左键,2右键,4中建
			int Shift,	//1代表SHIFT,2代表CTRL,4代表ALT,1+4代表SHIFT+ALT,...		
			int X, int Y) {}

		//! 数据中键滚轮
		virtual void onMouseWheel(
			int nFlags,
			short zDelta,
			int X, int Y) {}

		// 鼠标离开视图 chy-2016-4-14
		virtual void onMouseLeave(
			int Button,
			int Shift,
			int X, int Y) {}


		//! 获取工具光标
		virtual HCURSOR getHCursor() const { return m_hncursor; }
	};
}
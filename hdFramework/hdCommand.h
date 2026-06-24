/*! hdCommand.h
********************************************************************************
<PRE>
模块名       : hdFramework
文件名       : hdCommand.h
相关文件     : 
文件实现功能 : 命令按钮接口定义 
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/06/20   1.0      龚书林    
2013/02/27	 2.0	  危迟					移植
2013/09/12	 2.1      危迟				添加工具类型判断接口
</PRE>
*******************************************************************************/

#pragma once
#pragma  warning(disable:4251)
#include "stdafx.h"
#include "hdApp.h"
#include <string>

using namespace std;

namespace hd
{
	namespace fm
	{
		enum ENUM_HD_COMMAND_TYPE
		{
			E_HCT_3D			= 0x0001,		// 3D视图工具
			E_HCT_Planar		= 0x0002,		// 平面视图工具
			E_HCT_Quick			= 0x0004,		// 快速视图工具
			E_HCT_Tile          = 0x0006,       // 切片视图工具
			E_HCT_OverView		= 0x0008,		// 缩略视图工具
			E_HCT_CLASSIFY		= 0x0010,		// 分类工具
		};
			//3表示3D和平面均可用,
		  //5表示3D和快速视图均可用,
		  //6表示平面视图和快速视图可用
		  //7表示任何3D\平面\快速视图可用
		
		class HDFRAMEWORK_API CHdCommand
		{
		protected:
			//! app对象指针
			CHdApp*		m_app;
			//! 命令标题
			string	m_caption;
			//! 命令是否选中状态
			bool	m_checked;
			//! 命令当前是否可用
			bool	m_enabled;
			//! 命令消息提示
			string	m_message;
			//! 命令名称
			string	m_name;
			//! 命令提示
			string	m_toolTip;
			//! 命令ID,一种命令有唯一id
			int     m_id;
			// 命令字符ID
			string  m_cmdID;
			//! 命令类型
			int m_type;
            //! 工具图标
            HBITMAP m_bitmap;

		public:
			CHdCommand():m_app(NULL),m_id(-1),m_checked(false),m_enabled(true){}
			CHdCommand(const char* caption,
				const char* msg,
				const char* name,
				const char* tooltip,
				int type)
				:m_caption(caption),m_message(msg),
				m_name(name),m_toolTip(tooltip),
				m_type(type),m_app(NULL),m_id(-1),m_checked(false),
				m_enabled(true){}
			virtual ~CHdCommand(){};

			//! 当用户点击时响应
			virtual void OnClick() = 0;

			//! 当命令创建时调用
			virtual void OnCreate(CHdApp* app) = 0;

			//! 获取命令标题
			virtual string GetCaption() const {return m_caption;}

			//! 获取是否选中状态
			virtual bool GetChecked() {return m_checked;}

			//! 获取是否可用状态
			virtual bool GetEnable() {return m_enabled;}

			//! 获取命令消息
			//virtual string GetMessage() const {return m_message;}

			//! 获取命令名称
			virtual string GetName() const {return m_name;}

			//! 获取命令提示
			virtual string GetToolTip() const {return m_toolTip;}

			//! 获取命令ID
			virtual int	   GetID() const {return m_id;}

			//! 获取命令字符ID
			virtual string GetCmdID() const{return m_cmdID;}

			//! 获取命令类型
			virtual int    GetType() const { return m_type; }

			//! 获取命令图标
			virtual HBITMAP GetHbitmap() const{return m_bitmap;}

			//! 是否分类可用
			bool IsClassifyEnableCommand() const { return (m_type & 0x0010) == 0x0010; }

			//! 是否三维视图可用工具
			bool Is3DEnanleCommand() const { return (m_type & 0x0001) == 0x0001; }

			//! 是否平面视图可用工具
			bool IsPlanarEnableCommand() const { return (m_type & 0x0002) == 0x0002; }

			//! 是否快速视图可用工具
			bool IsQucikEnableCommand() const { return (m_type & 0x0004) == 0x0004; }
		};
	}
}
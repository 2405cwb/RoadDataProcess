/*! hdCommand.h
********************************************************************************
<PRE>
模块名       : hnApp
文件名       : hnCommand.h
相关文件     : 
文件实现功能 : 命令按钮接口定义 
作者         : 李夏亮
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2020/04/01   1.0      李夏亮    
</PRE>
*******************************************************************************/

#pragma once
#pragma  warning(disable:4251)
#include "hnApp.h"
#include <string>
#include "hnapplication_global.h"

using namespace std;

namespace hnApp
{
	class HNAPPLICATION_EXPORT hnCommand
	{
	protected:
		//! app对象指针
		hnApp*		m_app;
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

		//! 工具图标
		HBITMAP m_bitmap;

	public:
		hnCommand() :m_app(NULL), m_id(-1), m_checked(false), m_enabled(true) {}
		hnCommand(const char* caption,
			const char* msg,
			const char* name,
			const char* tooltip)
			:m_caption(caption), m_message(msg),
			m_name(name), m_toolTip(tooltip),
			m_app(NULL), m_id(-1), m_checked(false),
			m_enabled(true) {}
		virtual ~hnCommand() {};

		//! 当用户点击时响应
		virtual void onClick() = 0;

		//! 当命令创建时调用
		virtual void onCreate(hnApp* app) = 0;

		//! 获取命令标题
		virtual string getCaption() const { return m_caption; }

		//! 获取是否选中状态
		virtual bool getChecked() { return m_checked; }

		//! 获取是否可用状态
		virtual bool getEnable() { return m_enabled; }

		//! 获取命令名称
		virtual string getName() const { return m_name; }

		//! 获取命令提示
		virtual string getToolTip() const { return m_toolTip; }

		//! 获取命令ID
		virtual int	   getID() const { return m_id; }

		//! 获取命令字符ID
		virtual string getCmdID() const { return m_cmdID; }

		//! 获取命令图标
		virtual HBITMAP getHbitmap() const { return m_bitmap; }
	};
}
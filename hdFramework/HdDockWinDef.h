/*! HdDockWinDef.h
********************************************************************************
<PRE>
模块名       : hdFramework
文件名       : HdDockWinDef.h
相关文件     : 
文件实现功能 : 插件面板基类
作者         : 蔡红云
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/02/16   1.0      蔡红云   
</PRE>
*******************************************************************************/
#pragma once
#include "stdafx.h"
#include <string>
namespace hd
{
	namespace fm
	{
		class CHdApp;
		class HDFRAMEWORK_API CHdDockWinDef
		{
		public:
			CHdDockWinDef(void);

			~CHdDockWinDef(void);

			// 初始化浮动窗口
			virtual void OnCreate(CHdApp* hook, HWND parent) = 0;

			// 销毁浮动窗口
			virtual void OnDestroy() = 0;

			// 获取窗口句柄
			virtual HWND GetChildHWND() = 0;

			// 获取窗口标题
			virtual char* GetCaption() = 0;

			// 获取窗口名称
			virtual char* GetName() = 0;

			// 获取窗口关联数据
			virtual char* GetUserData() = 0;
		};
	}
}


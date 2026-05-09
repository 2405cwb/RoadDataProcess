/*! PluginManager.h
********************************************************************************
<PRE>
模块名       : hdFramework
文件名       : PluginManager.h
相关文件     : 
文件实现功能 : 插件管理器实现类
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
#include "stdafx.h"
#include "hdResourceBase.h"
#include "HdPlugIn.h"
#include <vector>

namespace hd
{
	namespace fm
	{
		class  HDFRAMEWORK_API CPluginManager
		{
		public:
			CPluginManager();

			~CPluginManager();

			// 通过路径加载插件、pluginFile全路径名 ,flag 用于标识插件加载状态
            // 1,表示加载成功，0表示已加载，-1表示加载失败
			CHdPlugIn* LoadPlugin(const char* pluginFile, int& flag);

			// 获取插件个数
			int GetPlugInCount()
			{
				return m_vec_plugins.size();
			}

			// 根据序号获取插件
			CHdPlugIn* GetPlugIn(int index);

			// 卸载指定插件、pluginFile全路径名
			void UnloadPlugin(const char* pluginFile);

			// 卸载所有插件
			void UnloadAllPlugin();

			// 根据插件名称获取插件 、pluginFile全路径名
			CHdPlugIn* GetPlugin(const char* pluginFile);
			
		private:

			// 插件列表
			std::vector<CHdPlugIn*>	m_vec_plugins;
            
            // 插件路径列表
            std::vector<const char*> m_vec_pluginPath;

		};
	}
}
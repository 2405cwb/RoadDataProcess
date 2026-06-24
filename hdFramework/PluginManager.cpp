#include "StdAfx.h"
#include <io.h>
#include "PluginManager.h"

namespace hd
{
	namespace fm
	{
		// 构造函数
		CPluginManager::CPluginManager()
		{
		}

		// 析构函数
		CPluginManager::~CPluginManager()
		{
			UnloadAllPlugin();
		}

		// 加载插件、成功返回加载的插件指针、否则返回NULL
		CHdPlugIn* CPluginManager::LoadPlugin(const char* pluginFile,int& flag)
		{
			// 如果文件不存在,返回
			if(_access(pluginFile, 00) != 0)
			{
				flag = -1;
				return NULL;
			}

			// 判断是否已经加载过相同路径的插件
			for (std::vector<CHdPlugIn*>::iterator it = m_vec_plugins.begin();
				it != m_vec_plugins.end();it++)
			{
				if (strcmp((*it)->GetPath(),pluginFile) == 0)//此处判断路径,需要更加严谨
				{
					flag = 0;
					return NULL;
				}
			}

			// 加载插件
			CHdPlugIn* pPlugin = new CHdPlugIn();

			if (pPlugin->LoadPlugIn(pluginFile))
			{

				m_vec_plugins.push_back(pPlugin);
				flag = 1;
				return pPlugin;
			}
			else
			{
				delete pPlugin;
			}
			flag = -1;
			return NULL;
		}

		// 根据序号获取插件
		CHdPlugIn* CPluginManager::GetPlugIn(int index)
		{
			if (index >= 0 && index < m_vec_plugins.size())
			{
				return m_vec_plugins[index];
			}
			else
			{
				return NULL;
			}
		}

		// 根据插件名称获取插件,strName插件文件名称,或者全路径名
		CHdPlugIn* CPluginManager::GetPlugin(const char* pluginFile)
		{
			if(_access(pluginFile, 00) != 0)
			{
				return NULL;
			}

			for (std::vector<CHdPlugIn*>::iterator it = m_vec_plugins.begin();
				it != m_vec_plugins.end();it++)
			{
				if (strcmp((*it)->GetPath(),pluginFile) == 0)//此处判断路径,需要更加严谨// \\ \ /
				{
					return *it;
				}
			}
			return NULL;
		}

		// 卸载插件,pluginFile为插件路径
		void CPluginManager::UnloadPlugin(const char* pluginFile)
		{
			for (std::vector<CHdPlugIn*>::iterator it = m_vec_plugins.begin();
				it != m_vec_plugins.end();it++)
			{
				if (strcmp((*it)->GetPath(),pluginFile) == 0)//此处判断路径,需要更加严谨// \\ \ /
				{
					delete  (*it);
					m_vec_plugins.erase(it);
					break;
				}
			}
		}

		// 卸载所有插件
		void CPluginManager::UnloadAllPlugin()
		{
			for (std::vector<CHdPlugIn*>::iterator it = m_vec_plugins.begin();
				it != m_vec_plugins.end();it++)
			{
				(*it)->UnloadPlugIn();
				delete  (*it);
			}

			m_vec_plugins.clear();
		}
	}
}


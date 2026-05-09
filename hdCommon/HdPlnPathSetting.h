
/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：HdPlnPathSetting.h
文件实现功能：实现hdscene插件路径管理
作者		：蔡红云
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2014/4/29	1.0			蔡红云		创建
</PRE>
******************************************************************************************************/

#pragma once
#pragma  warning(disable:4251)
#include "hdCommon.h"
#include <string>
#include <vector>
using namespace std;
namespace hd
{
	class HDCOMMON_API CHdPlnPathSetting
	{

	public:

		CHdPlnPathSetting(void);
		~CHdPlnPathSetting(void);

	public:

		// 读取配置文件HdScenePln.xml中的插件信息
		void ReadHdScenePlnXml();

		// 写入插件路径信息至配置文件HdScenePln.xml
		void WriteHdScenePlnXml();

		// 插件路径列表
		vector<string> m_vecPlnPaths;
	};
}	
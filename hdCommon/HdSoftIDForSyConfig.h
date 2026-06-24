/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：HdSoftIDForSyConfig.h
文件实现功能：此文件是为hdsene软件系列进行内部配置文件标识
作者		：蔡红云
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2016/1/20	1.0			蔡红云		创建
</PRE>
******************************************************************************************************/

#pragma once
#pragma  warning(disable:4251)
#include "hdCommon.h"
namespace hd
{
	class HDCOMMON_API CHdSoftIDForSyConfig
	{

	public:
		CHdSoftIDForSyConfig();
		~CHdSoftIDForSyConfig();

	private:

		// 软件ID 0表示hdscene 1表示hdscene-L 用于区别各自配置文件
		static int m_iSoftID;

	public:
					
		// 获取配置
		static int getHdSoftIDForSyConfig();

		// 配置软件ID
		static void setHdSoftIDForSyConfig(int softID);
				
	};
}
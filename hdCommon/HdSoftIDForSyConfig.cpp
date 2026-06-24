/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：HdSoftIDForSyConfig.cpp
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
#include "stdafx.h"
#include"HdSoftIDForSyConfig.h"

namespace hd
{	  

	int CHdSoftIDForSyConfig::m_iSoftID = 0;
	CHdSoftIDForSyConfig::CHdSoftIDForSyConfig()
	{

	}

	CHdSoftIDForSyConfig::~CHdSoftIDForSyConfig()
	{

	}

	// 获取配置
	int CHdSoftIDForSyConfig::getHdSoftIDForSyConfig()

	{
		return m_iSoftID;
	}

	// 配置软件ID
	void CHdSoftIDForSyConfig::setHdSoftIDForSyConfig(int softID)
	{
		m_iSoftID = softID;
	}

}

/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：HdSoftTypeConfig.h
文件实现功能：此文件是为软件系列进行标识配置
作者		：蔡红云
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2016/1/19	1.0			蔡红云		创建
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
		

	class HDCOMMON_API CHdSoftTypeConfig
	{

	public:

		CHdSoftTypeConfig(void);
		~CHdSoftTypeConfig(void);

	public:

		//! 从配置文件读取，获取设置
		static CHdSoftTypeConfig* getHdSoftTypeConfig();
		
		//! 删除静态唯一对象
		static void destroyHdSoftTypeConfig();

		// !保存静态唯一对象
		static void saveHdSoftTypeConfig();

		// 软件类型 0x100 为hd系列 0x001为航空系列
		int m_iSoftType;


	private:

		//静态变量用于管理配置
		static CHdSoftTypeConfig* m_pSHdSoftTypeConfig;

	};
}	
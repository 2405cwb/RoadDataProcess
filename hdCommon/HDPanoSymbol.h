/*!@file
*******************************************************************************************************
<PRE>
模块名		：HDPanoSymbol.h
文件名		：CHDPanoSymbol.h
相关文件	: CHDPanoSymbol.cpp	HDObject.h
文件实现功能：全景标注符号对象，用来描述一个全景标注对应的符号。从CHDObject继承。
作者		：孙文
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/1/16	1.0			孙文		创建
</PRE>
******************************************************************************************************/

#pragma once
#include "HDObject.h"
#include <iostream>
using namespace std;

namespace hd
{
	class HDCOMMON_API CHDPanoSymbol : public CHDObject
	{
	public:
		// 构造函数
		CHDPanoSymbol(void);
		CHDPanoSymbol(char* strName, int nType, char* strUrl);
		virtual ~CHDPanoSymbol(void);

	public:
		virtual ENUM_HDMS_OBJECT_TYPE GetType () const { return E_HOT_PANOSYMBOL;}
		// 得到符号的名称
		const char* GetName() const ;
		// 得到符号的类型
		int GetSymbolType() const;
		// 得到符号所在路径
		const char* GetUrl() const;
		// 设置符号的名称
		void SetName(const char* strName);
		// 设置符号的类型
		void SetSymbolType(int nType);
		// 设置符号的路径
		void SetUrl(const char* strUrl);

	public:
		char m_strName[NAME_LEN];		//符号名称
		int	m_nType;					//符号类型
		char m_strUrl[MAX_PATH];		//符号所在路径

	};

}
/*!@file
*******************************************************************************************************
<PRE>
模块名		：HDEntity.h
文件名		：CHDEntity.h
相关文件	: CHDEntity.cpp	HDObject.h
文件实现功能：实体对象类，带ID的对象。
作者		：孙文
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/1/6	1.0			孙文		创建
</PRE>
******************************************************************************************************/

#pragma once
#include "HDObject.h"
#include "hdConstDef.h"

namespace hd
{

	class HDCOMMON_API CHDEntity
	{
	public:
		CHDEntity(void);
		CHDEntity(const char* pID, CHDObject* pObject);
		~CHDEntity(void);

	public:
		// 获取ID
		const char* GetID() const;
		// 设置ID
		void SetID(const char* pID);
		// 获取对象
		CHDObject* GetHDObject() const;
		// 设置对象
		void SetObject(CHDObject* pObject);

	private:
		char m_ID[OBJECT_ID_LEN_L];			// 对象ID（流水号）
		CHDObject* m_pObject;				// 对象指针
	};
}
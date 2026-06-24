// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 BASICOBJECT_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// BASICOBJECT_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#pragma once

#ifdef BASICOBJECT_EXPORTS
#define BASICOBJECT_API __declspec(dllexport)
#define BASICOBJECT_TEMPLATE
#else
#define BASICOBJECT_API __declspec(dllimport)
#endif

// 屏蔽size_t到int的警告
#pragma warning(disable:4267)

// dll导出对象
#pragma warning(disable:4251)

// 类型转换
#pragma warning(disable:4244)

// 屏蔽fopen，strcpy等安全警告
#pragma warning(disable:4996)


#include "hdBasicStruct.h"
#include "TypeDef.h"

// 定义一个基础类
// 此类是从 BasicObject.dll 导出的
class BASICOBJECT_API CHdBasicObject {
public:
	CHdBasicObject(void):
	  m_eType(E_TID_UNKNOWN)
	{}
	virtual ~CHdBasicObject() {}
	

	E_TYPE_ID GetType()const { return m_eType; }

protected:
	E_TYPE_ID		m_eType;
};


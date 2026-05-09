// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

// vc2005以上使用了更加安全的run-time library，在连接的时候会自动将旧函数替换成Security CRT functions fengjing20140730
// 但是依然会有warning C4996，需要在禁掉该警告
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1
#pragma  warning(disable: 4996)

#pragma warning(disable: 4018) // “>=”: 有符号/无符号不匹配 fengjing 20140730

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  从 Windows 头文件中排除极少使用的信息
// Windows 头文件:
#include <windows.h>

#ifdef HD3DSCENE_EXPORTS
#define HD3DSCENE_API __declspec(dllexport)
#else
#define HD3DSCENE_API __declspec(dllimport)
#endif

#pragma  warning(disable:4251)
// TODO: 在此处引用程序需要的其他头文件
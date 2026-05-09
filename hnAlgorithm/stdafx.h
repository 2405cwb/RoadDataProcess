// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
// Windows 头文件: 
#include <windows.h>

// 导出定义
#ifdef HNALGORITHM_EXPORT
#define HNALGORITHM_API __declspec(dllexport)
#else
#define HNALGORITHM_API __declspec(dllimport)
#endif // hnConnonAPI_EXPORT

// TODO:  在此处引用程序需要的其他头文件

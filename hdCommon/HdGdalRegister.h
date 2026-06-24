#ifndef HDCOMMON_HDGDALREGISTER_H
#define HDCOMMON_HDGDALREGISTER_H

#include "hdCommon.h"

namespace hd
{
	class HDCOMMON_API CHdGdalRegister
	{
	public:
		CHdGdalRegister(){}
		~CHdGdalRegister(){}

	public:

		// 注册GDAL驱动，包括栅格和矢量的驱动
		static void RegisterGDALDriver();

		// 释放GDAL驱动
		static void UnRegisterGDALDriver();
	};
}

#endif
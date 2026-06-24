#include "stdafx.h"
#include "HdGdalRegister.h"
#include "gdal.h"
#include "cpl_conv.h"

using namespace hd;

void CHdGdalRegister::RegisterGDALDriver()
{
	// 最新版本的GDAL已将栅格和矢量驱动的注册合并到一个函数中
	GDALAllRegister();

	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");
}
void CHdGdalRegister::UnRegisterGDALDriver()
{
	GDALDestroyDriverManager();
}
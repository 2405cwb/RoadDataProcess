/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdDataDriver
文件名		：HdRasterImage.h
相关文件	: GDAL,HdImageBuffer.h
文件实现功能：基于GDAL生产金字塔影像
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/4/25	1.0			马振明		创建
</PRE>
******************************************************************************************************/
#pragma once
#include "stdafx.h"
#include "gdalwarper.h"
#include "gdal_priv.h"

namespace hd
{
	namespace scene
	{
	class HD3DSCENE_API CHdPyramid
	{
	public:
		CHdPyramid(void);
		~CHdPyramid(void);

		// 生成金字塔,返回金字塔的级别,
		// 传入参数为路径，金字塔每块的标准高和宽,默认参数为64
		int CreatePyramids(const char* strFilePath,int nPyramidH=64,int nPyramidW=64);

		// 获取金字塔信息
		int GetPyramidsInfo(const char* strFilePath);

		// 重采样
		int ResampleGDAL(const char* pszSrcFile, const char* pszOutFile, float fResX = 1.0 ,float fResY=1.0,GDALResampleAlg eResample = GRA_Bilinear);

		// 裁切,传入裁剪区域的左上点坐标，以及右下点坐标,以及缩放的系数
		int ClipGDAL(const char* pszSrcFile, const char* pszOutFile,double dLX,double dUY,double dRX,double dDY,double dScale =1.0);

		// 数据裁切,传入裁剪区域的左上点坐标，以及右下点坐标,以及缩放的系数
		int ClipGDAL(GDALDataset * poDataset, const char* pszOutFile,double dLX,double dUY,double dRX,double dDY,double dScale =1.0);
	};
	}
}

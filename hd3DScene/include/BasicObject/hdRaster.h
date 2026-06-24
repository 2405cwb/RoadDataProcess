/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdBasicObject
文件名		：Raster.h
相关文件	: 
文件实现功能：定义栅格数据类
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/4/22	1.0			马振明		  移植
</PRE>
******************************************************************************************************/

#pragma once
#include "hdBasicObject.h"
#include <string>
using namespace std;

// 定义像素类型,和GDAL的类型顺序保持一致
enum E_PIXEL_TYPE
{
	E_PT_UNKNOWN= 0,
	E_PT_BYTE =1,
	E_PT_UNSHORT=2,
	E_PT_SHORT=3,
	E_PT_UNINT=4,
	E_PT_INT=5,
	E_PT_FLOAT=6,
	E_PT_DOUBLE=7
};

// 矢量对象类
class BASICOBJECT_API CHdRaster :
	public CHdBasicObject
{
public:
	CHdRaster(void);
	virtual ~CHdRaster(void);

	// 创建
	virtual bool Creat(void* pBuffer, int nRows, int nCols, int nBand, int nBPP, E_PIXEL_TYPE ePixelType);

	// 获取行列
	void GetRowsCols(int& nRows, int& nCols) const;

	// 获取波段数据
	int GetBandNum() const;

	// 获取每个像素字节大小
	int GetBPP() const;

	// 获取每个波段像素字节大小
	int GetBPB() const;

	// 获取像素类型
	E_PIXEL_TYPE GetPixelType() const;

	// 获取名称
	char* GetName() const
	{
		return m_strName;
	}

	// 设置名称
	void SetName(const char* strName);

	// 数据缓存的排序类型
	bool IsBufferBand() const
	{
		return m_bBufferBand;
	}

	// 设置数据缓存的排序类型
	void SetBufferBand(bool bBand) 
	{
		m_bBufferBand = bBand;
	}
public:
	void*			m_pBuffer;									// 缓存
protected:
	int				m_nRows;									// 行
	int				m_nCols;									// 列
	int				m_nBand;									// 波段数
	int				m_nBPP;										// 每个像素字节大小
	bool			m_bBufferBand;								// 像素排列顺序，
																// 默认为true，即按波段排列									
	E_PIXEL_TYPE	m_ePixelType;								// 像素类型
	char*			m_strName;									// 影像名称
};


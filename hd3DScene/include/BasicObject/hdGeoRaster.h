/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdBasicObject
文件名		：GeoRaster.h
相关文件	: 
文件实现功能：定义带地理坐标的栅格数据类
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
#include "hdRaster.h"
#include "BaseRect.h"
#pragma warning(disable:4251)

class BASICOBJECT_API CHdGeoRaster :
	public CHdRaster
{
public:
	CHdGeoRaster(void);
	virtual ~CHdGeoRaster(void);

	// 设置左上点坐标
	void SetLT(Point2dd ptLTX);

	// 设置左下点的坐标
	void SetLT(double dLTx,double dLTy);

	// 设置X,Y方向上的间距
	void SetCellSize(float fCellSizeX,float fCellSizeY);

	// 获取左下点坐标
	Point2dd GetLBX() const;

	// 获取左上点坐标
	Point2dd GetLUX() const;


	// 获取间隔大小
	void GetCellSize(float& fCellSizeX,float& fCellSizeY) const;

	// 更新包围盒
	void UpdateBoundingBox();

	// 获取包围盒
	Chd2DBoundingBoxd GetBoundingBox() const
	{
		return m_BoundingBox;
	}

public:
	float				m_fMax;										// 波段对应的最大值
	float				m_fMin;										// 波段对应的最小值
protected:
	Point2dd			m_ptLT;										// 左下角点坐标
	float				m_fCellSizeX;								// X方向间距大小
	float				m_fCellSizeY;								// Y方向间距大小
	Chd2DBoundingBoxd	m_BoundingBox;								// 包围盒
};


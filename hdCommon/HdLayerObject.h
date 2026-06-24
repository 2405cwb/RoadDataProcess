/*! CHdLayerObject.h
********************************************************************************
<PRE>
模块名       : hdApplication
文件名       : CHdLayerObject.h
相关文件     : CHdLayerObject.cpp
文件实现功能 : ORG图层数据对象
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/06/14   1.0      龚书林    				
</PRE>
*******************************************************************************/

#pragma once
#include "stdafx.h"
#include "hdobject.h"
#include "ogrsf_frmts.h"
#include "ogr_api.h"
namespace hd
{
class HDCOMMON_API CHdLayerObject :
	public CHDObject
{
public:
	CHdLayerObject(void);
	~CHdLayerObject(void);
	// 打开图层数据
	BOOL Open(const char* path,const char* lyrName = NULL);
	// 获取图层对象
	OGRLayer* GetLayer() const {return m_pOgrLayer;}
	// 获取类型
	virtual ENUM_HDMS_OBJECT_TYPE GetType() const {return ESDT_OBJECT_LAYER;}
	
	//获取文件名
	std::string GetFileName() const;

	// 获取文件扩展
	std::string GetFileExt() const;

	// 获取文件路径
	std::string GetFilePath() const;

    // 设置文件路径
    void SetFilePath(std::string strFilePath);
private:
	// OGR图层
	OGRLayer* m_pOgrLayer;
	// OGR数据集
	GDALDataset* m_poDS;
	std::string m_strPath; // 文件路径
};

//! OGR矢量点对象
class CHdOgrPoint:
	public CHDObject
{
public:
	CHdOgrPoint(OGRPoint* pOgrPoint)
	{
		m_pOgrPoint = pOgrPoint;
	}
	virtual ~CHdOgrPoint()
	{
		if (m_pOgrPoint)
		{
			OGR_G_DestroyGeometry((OGRGeometryH)m_pOgrPoint);
			m_pOgrPoint = NULL;
		}
	}
	OGRPoint* GetPoint() const {return m_pOgrPoint;}
	
	// 获取类型
	virtual ENUM_HDMS_OBJECT_TYPE GetType() const {return ESDT_OBJECT_OGRPOINT;}

private:
	OGRPoint* m_pOgrPoint;
};
//! OGR矢量线对象
class CHdOgrPolyline:
	public CHDObject
{
public:
	CHdOgrPolyline(OGRLineString* pOgrPolyline)
	{
		m_pOgrPolyline = pOgrPolyline;
	}
	virtual ~CHdOgrPolyline()
	{
		if (m_pOgrPolyline)
		{
			OGR_G_DestroyGeometry((OGRGeometryH)m_pOgrPolyline);
			m_pOgrPolyline = NULL;
		}
	}
	OGRLineString* GetPolyline() const{return m_pOgrPolyline;}
	// 获取类型
	virtual ENUM_HDMS_OBJECT_TYPE GetType() const {return ESDT_OBJECT_OGRPOLYLINE;}

private:
	OGRLineString* m_pOgrPolyline;
};
//! OGR矢量面对象
class CHdOgrPolygon:
	public CHDObject
{
public:
	CHdOgrPolygon(OGRPolygon* pOgrPolygon)
	{
		m_pOgrPolygon = pOgrPolygon;
	}
	virtual ~CHdOgrPolygon()
	{
		if (m_pOgrPolygon)
		{
			OGR_G_DestroyGeometry((OGRGeometryH)m_pOgrPolygon);
			m_pOgrPolygon = NULL;
		}
	}
	OGRPolygon* GetPolygon() const{return m_pOgrPolygon;}
	// 获取类型
	virtual ENUM_HDMS_OBJECT_TYPE GetType() const {return ESDT_OBJECT_OGRPOLYGON;}

private:
	OGRPolygon* m_pOgrPolygon;
};
}
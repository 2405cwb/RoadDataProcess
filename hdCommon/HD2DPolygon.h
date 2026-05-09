/*!@file
*******************************************************************************************************
<PRE>
模块名		：HD2DPolygon.h
文件名		：CHD2DPolygon.h
相关文件	: CHD2DPolygon.cpp	HD2DObject.h	HDBaseStruct.h
文件实现功能：二维多边形对象，从CHD2DObject继承，二维多边形对象是首位两个节点闭合的对象，
在绘制时，需要将此对象的首尾节点连接起来。
作者		：任高强
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/1/6	1.0			任高强		创建
</PRE>
******************************************************************************************************/

#pragma once
#pragma  warning(disable:4251)
#include "HD2DObject.h"
#include "HDBaseStruct.h"

#include <vector>
using namespace std;

namespace hd
{

	class HDCOMMON_API CHD2DPolygon:public CHD2DObject
	{
	public:
		CHD2DPolygon(void);
		virtual ~CHD2DPolygon(void);

	protected:
		vector<HD_2DPOINT>   m_vecVertexs;	// 多边形节点
		CHD2DBoundingBox      m_BBox;		// 多边形最小外扩包围盒

	public:
		virtual CHD2DBoundingBox GetBoundingBox() const { return m_BBox;}
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_2DPOLYGON;}
		// 赋值运算
		CHD2DPolygon& operator=(const CHD2DPolygon& other);
		// 得到多边形结点个数
		unsigned int GetVertexCount() const;
		// 得到指定索引位置的节点
		HD_2DPOINT GetVertex(unsigned int nIndex) const;
		// 在多线段尾部添加一个节点
		void AddVertex(const HD_2DPOINT& pt);
		// 在多线段尾部添加一个节点
		void AddVertex(double dX, double dY);
		// 更改指定索引位置的节点
		bool SetVertex(unsigned nIndex, const HD_2DPOINT& pt);
		// 在指定位置插入一个节点
		bool InsertVertex(const HD_2DPOINT& pt, unsigned int nIndex);
		// 删除指定位置的节点
		bool DeleteVertex(unsigned int nIndex);
		// 清空所有节点
		void ClearALL();
		// 判断二维点是否在多边形内，如果在返回true，不在返回false
		bool IsPointIn(const HD_2DPOINT& pt) const;
		bool IsPointIn(double dX, double dY) const;

		// 重新计算当前多边形的最小外包围盒
		void ReCalcBoundingBox();
		
	};

}
/*!@file
*******************************************************************************************************
<PRE>
模块名		：HD2DPolyline.h
文件名		：CHD2DPolyline.h
相关文件	: CHD2DPolyline.cpp	HD2DObject.h	HDBaseStruct.h
文件实现功能：二维多线段对象，从CHD2DObject继承。
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

	class HDCOMMON_API CHD2DPolyline : public CHD2DObject
	{
	public:
		CHD2DPolyline(void);
		virtual ~CHD2DPolyline(void);

	public:
		// 得到多线段上节点个数
		unsigned int GetVertexCount() const;

		// 得到指定索引位置的节点
		HD_2DPOINT GetVertex(unsigned int nIndex) const;

		// 在多线段尾部添加一个节点
		void AddVertex(const HD_2DPOINT& pt);

		// 在多线段尾部添加一个节点
		void AddVertex(double dX, double dY);

		// 更改指定索引位置的值
		bool SetVertex(unsigned int nIndex, const HD_2DPOINT& pt);

		// 更改指定索引位置的值
		bool SetVertex(unsigned int nIndex, double dX, double dY);

		// 在指定位置，插入一个节点
		bool InsertVertex(unsigned int nIndex, const HD_2DPOINT& pt);

		// 删除指定位置的节点
		bool DeleteVertex(unsigned int nIndex);

		// 清空所有节点
		void ClearALL();

		// 赋值运算
		CHD2DPolyline& operator=(const CHD2DPolyline& other);

		virtual CHD2DBoundingBox GetBoundingBox() const { return m_BBox;}
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_2DPOLYLINE;}

		//得到点到多线段之间的最短距离
		double GetClosestPoint(const HD_2DPOINT* pPoint, HD_2DPOINT& closestPt) const;

		//判断点到多线段之间的最短距离是否小于fRadius
		bool IsInRange(const HD_2DPOINT* pPoint, float fRadius) const;

		// 判断点是否在线上
		bool IsOnLine(double dX,double dY,double dError =0.001f);

		// 判断是否相交,并求出交点[add by mzm 2014.11.26]
		bool IsIntersect(const CHD2DPolyline* pPolyLine,HD_2DPOINT& pCrossPoint);

		// 判断4个点组成的2个线段是否相交，前2个点构成一个线段，后2个点构成一个线段,并求出交点 [add by mzm 2014.11.26]
		bool IsLineIntersectLine(const HD_2DPOINT& pFirst1,const HD_2DPOINT& pFirst2,const HD_2DPOINT& pSecond1,const HD_2DPOINT& pSecond2, HD_2DPOINT& pCrossPoint);

		// 通过距离获得多段线上某一点 [add by mzm 2015.2.7]
		bool GetPointByDis(double dis,HD_2DPOINT& point);

		// 获取长度
		double GetLength();

	private:
		// 重新计算当前多线段的最小外扩包围盒 
		void ReCalcBoundingBox();
		HD_2DPOINT GetClosestPoint(const HD_2DPOINT* pStartPt, const HD_2DPOINT* pEndPt, const HD_2DPOINT* pPoint) const;

	protected:
		vector<HD_2DPOINT> m_vecVertexs;		// 多线段上的节点
		CHD2DBoundingBox m_BBox;				// 多线段的最小外扩包围盒

	};

}
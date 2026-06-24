/*!@file
*******************************************************************************************************
<PRE>
模块名		：HD3DPolyline.h
文件名		：CHD3DPolyline.h
相关文件	: CHD3DPolyline.cpp	HD3DObject.h	HDBaseStruct.h
文件实现功能：三维多线段对象，从CHD3DObject继承。
作者		：孙文
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/1/6	1.0			孙文		创建
</PRE>
******************************************************************************************************/

#pragma once
#pragma  warning(disable:4251)
#include "HD3DObject.h"
#include "HDBaseStruct.h"
#include <vector>
using namespace std;

namespace hd
{
	class HDCOMMON_API CHD3DPolyline : public CHD3DObject
	{
	public:
		CHD3DPolyline(void);
		virtual ~CHD3DPolyline(void);

	public:
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const {return E_HOT_3DPOLYLINE;}
		// 得到多线段节点个数
		unsigned int GetVertexCount() const;
		// 得到指定索引位置的节点
		HD_3DPOINT GetVertex(unsigned int nIndex) const;
		// 在多线段尾部添加一个节点
		void AddVertex(const HD_3DPOINT& pt);
		void AddVertex(double dX, double dY, double dZ);
		// 更改指定索引位置的节点的值
		bool SetVertex(unsigned int nIndex, const HD_3DPOINT& pt);
		// 更改指定索引位置的节点的值
		bool SetVertex(unsigned int nIndex, double dX, double dY, double dZ);
		// 在指定位置，插入一个节点
		bool InsertVertex(unsigned int nIndex, const HD_3DPOINT& pt);
		// 删除指定位置的节点
		bool DeleteVertex(unsigned int nIndex);
		// 清空所有点
		void ClearAll();
		// 获取Box
		virtual CHD3DBoundingBox GetBoundingBox() const;
		// 赋值运算
		CHD3DPolyline& operator=(const CHD3DPolyline& other);
		// 判断线段是否在这个球内
		virtual bool IsInSphere(double dX, double dY, double dZ, double dR) const;
		// 计算点到多线段的最短距离，并返回最短距离的点
		double GetClosestPoint(const HD_3DPOINT* pPoint, HD_3DPOINT& closestPt) const;

		// 判断一个点是否在多边形的边上，如果在，返回边的点索引
		void PointIsOnEdge(double dX,double dY,double dZ, int& nStartIndex, int& nEndIndex,double fError = 3.0);
	private:
		// 重新计算当前多线段的最小外包围盒
		void RecalcBoundingBox();
		HD_3DPOINT GetClosestPoint(const HD_3DPOINT* pStartPt, const HD_3DPOINT* pEndPt, const HD_3DPOINT* pPoint) const;
		// 判断是否在多段线的包围盒内
		bool IsInPolyLineBox(double dX, double dY, double dZ,double dError = 0.5);
	private:
		vector<HD_3DPOINT> m_vecVertexs;	// 多线段上的节点
		CHD3DBoundingBox m_bBox;			// 多线段的最小外扩包围盒
	};

}
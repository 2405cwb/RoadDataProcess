/*!@file
*******************************************************************************************************
<PRE>
模块名		：HD3DCube.h
文件名		：CHD3DCube.h
相关文件	: CHD3DCube.cpp	HD3DObject.h	HDBaseStruct.h
文件实现功能：三维多线段对象，从CHD3DObject继承。
作者		：刘俊
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2013/11/6	1.0			刘俊		创建
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
	class HDCOMMON_API CHD3DCube : public CHD3DObject
	{
	public:
		CHD3DCube(void);
		virtual ~CHD3DCube(void);

	public:
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const {return E_HOT_CUBE;}
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
		CHD3DCube& operator=(const CHD3DCube& other);
		// 等号运算符
		bool IsSame(const CHD3DCube* other);
		// 判断线段是否在这个球内
		virtual bool IsInSphere(double dX, double dY, double dZ, double dR) const;
		// 计算点到多线段的最短距离，并返回最短距离的点
		double GetClosestPoint(const HD_3DPOINT* pPoint, HD_3DPOINT& closestPt) const;
		// 更新点
		void UpdateVertexs(int index , double dx, double dy, double dz);
		// 更新面，index为面索引，0,1为上下面，移动Z值，2,3为前后面，移动X，4,5为左右面，移动Y
		void UpdatePlane(int index, double dx, double dy, double dz);
	private:
		// 重新计算当前多线段的最小外包围盒
		void RecalcBoundingBox();
		HD_3DPOINT GetClosestPoint(const HD_3DPOINT* pStartPt, const HD_3DPOINT* pEndPt, const HD_3DPOINT* pPoint) const;
	private:
		vector<HD_3DPOINT> m_vecVertexs;	// 多线段上的节点
		CHD3DBoundingBox m_bBox;			// 多线段的最小外扩包围盒
	};

}
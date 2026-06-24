/*!@file
*******************************************************************************************************
<PRE>
模块名		：HDMarkerFacade.h
文件名		：CHDMarkerFacade.h
相关文件	: CHDMarkerFacade.cpp 	HDFace.h	HD3DPolyline.h
文件实现功能：立面对象，从CHDFace继承，用来描述建筑物或目标物的立面。
作者		：孙文
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2013/1/6	1.0			孙文		创建
2013/3/13	1.0			孙文		添加获取片面类型接口
</PRE>
******************************************************************************************************/

#pragma once
#include "HDFace.h"
#include "HD3DPolyline.h"
#include "HD3DPoint.h"

namespace hd
{	
	class HDCOMMON_API CHDMarkerFacade : public CHDFace
	{
		friend class CHD3DPolyline;
	public:
		CHDMarkerFacade(void);
		virtual ~CHDMarkerFacade(void);

	public:
		virtual ENUM_HDMS_OBJECT_TYPE GetType () const { return E_HOT_MARKER_FACADE;}
		virtual CHD3DBoundingBox GetBoundingBox() const;
		// 判断对象是否在球体内
		virtual bool IsInSphere(double dX, double dY, double dZ, double dR) const;
		// 判断对象是否在立方体里
		bool IsInBox(CHD3DBoundingBox& bBox);
		// 得到面的法向量
		virtual HD_3DPOINT GetNormal();
		// 根据一串点，创建一个立面，其中pVertexs中点以XYZ的形式排列。此函数中进行数据合法性检查，
		// 检查pVertexs中的点，是否在一个立面上，如果不在，则创建失败，返回false。
		bool CreateFacade(int nVertexCount, double* pVertexs);
		// 根据底边和高创建立面，其中pVertexs中的点以XY的形式排列
		bool CreateFacade(int nVertexCount, double* pVertexs, double dZMin, double dZMax);
		// 获取当前面的中心点
		bool GetCenterPt(HD_2DPOINT& pt) const;
		// 获取面片类型
		int GetFacadeType();
		// 得到多线段节点个数
		unsigned int GetVertexCount() const;
		// 得到指定索引位置的节点
		HD_3DPOINT GetVertex(unsigned int nIndex) const;
		// 在多线段尾部添加一个节点
		void AddVertex(const HD_3DPOINT& pt);
		void AddVertex(double dX, double dY, double dZ);
		//更改立面的节点坐标
		bool UpdateVertex(int nIndex, double dX, double dY, double dZ);

	private:
		// 点与立方体的碰撞检测
		bool BoxCollision(CHD3DBoundingBox bBox, double dX, double dY, double dZ);

	private:
		CHD3DPolyline m_Polyline;		// 立面的节点
	};

}
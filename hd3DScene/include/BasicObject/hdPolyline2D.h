/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdBasicObject
文件名		：Polyline2D.h
相关文件	: BasicStruct.h
文件实现功能：定义一个2D点多段线类
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
#ifndef HDPOLYLINE2D_HPP_
#define  HDPOLYLINE2D_HPP_
#include "hdBasicObject.h"
#include "BaseRect.h"
#include <vector>
using namespace std;

template<typename T> class
 CPolyline2D :public CHdBasicObject
{
public:
	CPolyline2D(void)
	{
		m_eType = E_TID_POLYLINE_2D;

		m_pPolyline = new std::vector<POINT2D<T>>;
	}

	virtual ~CPolyline2D(void)
	{
		if (m_pPolyline != NULL)
		{
			delete m_pPolyline;
		}
	}

public:

	// 获取外接区域
	CBaseRect<T> GetRange() const
	{
		return m_BoundingBox;
	}

	// 添加点
	void AddVertex(const POINT2D<T>& Pt)
	{
		if (m_pPolyline)
		{
			m_pPolyline->push_back(Pt);
		}
	}

	// 添加点
	void AddVertex(T x,T y)
	{
		if (m_pPolyline)
		{
			POINT2D<T> pt;
			pt.x = x;
			pt.y = y;
			m_pPolyline->push_back(pt);
		}
	}

	// 删除点
	bool DeleteVertex(int nIndex)
	{
		if (m_pPolyline)
		{
			m_pPolyline->erase(m_pPolyline->begin()+nIndex);
			return true;
		}

		return false;
	}

	// 获取线段
	vector<POINT2D<T>>* GetPolyLine() const
	{
		return m_pPolyline;
	}

	// 获取点的个数
	int GetVertexCount() const
	{
		// 如果有效
		if (m_pPolyline)
		{
			return static_cast<int>(m_pPolyline->size());
		}
		else
		{
			return 0;
		}
	}

	// 重载赋值操作符
	CPolyline2D<T>& operator=(const CPolyline2D<T>& otherPolyLine)
	{
		// 防止本值赋值
		if (this == &otherPolyLine)
		{
			return *this;
		}
		
		if (otherPolyLine.GetPolyLine())
		{
			if (m_pPolyline != NULL)
			{
				delete m_pPolyline;
			}
		
			m_pPolyline = new vector<POINT2D<T>>;
			int nSize = otherPolyLine.GetPolyLine()->size();
			for (int i=0;i<nSize;i++)
			{
				m_pPolyline->push_back(otherPolyLine.GetPolyLine()->at(i));
			}
			m_BoundingBox = otherPolyLine.GetRange();
		}
	
		return *this;
	}
protected:
	vector<POINT2D<T>>*					m_pPolyline;					// 点集
	CBaseRect<T>						m_BoundingBox;					// 包围盒	
};

typedef CPolyline2D<double> BASICOBJECT_API CPolyline2Dd;							// 定义double类型的多段线
#endif

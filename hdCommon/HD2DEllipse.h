/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：CHD2DEllipse.h
相关文件	: CHD2DEllipse.cpp	HD2DObject.h	HDBaseStruct.h
文件实现功能：二维椭圆对象
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2013/12/27	1.0			马振明		  创建
</PRE>
******************************************************************************************************/
#pragma once
#include "hd2dobject.h"
#include "HD2DPoint.h"
#include <math.h>
#include <vector>
#include <math.h>
using namespace std;

namespace hd
{
	class HDCOMMON_API CHD2DEllipse
		:public CHD2DObject
	{
	public:
		CHD2DEllipse(void);
		~CHD2DEllipse(void);

		// 得到外接矩形
		virtual CHD2DBoundingBox GetBoundingBox() const;

		// 得到类型
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_2DELLIPSE;}

		// 赋值运算
		CHD2DEllipse& operator=(const CHD2DEllipse& other);

		// pt是否在椭圆中
		bool IsPointInEllipse(const CHD2DPoint& pt) const;

		// 计算椭圆上的顶点，按角度迭代,默认按5度迭代
		void GenerateVertexes(int angle = 5);
	public:
		CHD2DPoint			m_ptLB;				// 左下角点
		CHD2DPoint			m_ptRT;				// 右上角点
		vector<CHD2DPoint*>	m_vectPoints;		// 椭圆上的点
		CHD2DPoint			m_CenterPoint;		// 中心点
		double				m_da;				// 椭圆的a值
		double				m_db;				// 椭圆的b值
	};
}



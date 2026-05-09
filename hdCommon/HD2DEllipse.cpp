/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：CHD2DEllipse.cpp
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
#include "StdAfx.h"
#include "HD2DEllipse.h"

#define PI 3.14
namespace hd
{
	// 构造函数
	CHD2DEllipse::CHD2DEllipse(void)
	{
		m_vectPoints.reserve(200);
		m_vectPoints.clear();
	}

	// 析构，清除指针
	CHD2DEllipse::~CHD2DEllipse(void)
	{
		// 清除椭圆边界上的指针
		for (unsigned int i=0; i<m_vectPoints.size(); i++)
		{
			if (m_vectPoints.at(i))
			{
				delete m_vectPoints.at(i);
				m_vectPoints.at(i)=NULL;
			}
		}
		// 清除数组
		m_vectPoints.clear();
	}

	// 获取包围盒
	CHD2DBoundingBox CHD2DEllipse::GetBoundingBox() const
	{
		CHD2DBoundingBox box;
		box.m_dMinX = m_ptLB.m_x;
		box.m_dMinY = m_ptLB.m_y;
		box.m_dMaxX = m_ptRT.m_x;
		box.m_dMaxY = m_ptRT.m_y;

		return box;
	}

	// 判断点是否在椭圆中
	bool CHD2DEllipse::IsPointInEllipse(const CHD2DPoint& pt) const
	{
		// 用椭圆公式计算 
		// 椭圆公式为（x-x0）*（x-x0）/a*a + （y-y0）*（y-y0）/b*b = 1;
		double dResult = ((pt.m_x - m_CenterPoint.m_x)*(pt.m_x - m_CenterPoint.m_x))/(m_da*m_da)
						+ ((pt.m_y - m_CenterPoint.m_y)*(pt.m_y - m_CenterPoint.m_y))/(m_db*m_db);
	
		// 判断是否在椭圆内
		if (dResult<1)
		{
			return true;
		}

		return false;
	}

	// 计算椭圆上的点,按角度迭代
	// 原理是椭圆的参数方程
	// 公式为：x=x1 + a*cosA; y=y1+ b*sinA; 
	// 其中x1,y1为椭圆中心点，A为迭代角度，a为长轴，b为短轴
	void CHD2DEllipse::GenerateVertexes(int angle)
	{
		if ((m_ptLB.m_x == m_ptRT.m_x) && (m_ptLB.m_y == m_ptRT.m_y) )
		{
			return;
		}
		// 椭圆中心点
		m_CenterPoint = CHD2DPoint((m_ptLB.m_x+m_ptRT.m_x)/2,(m_ptLB.m_y+m_ptRT.m_y)/2);

		// 椭圆长短半轴
		m_da = fabs(m_ptRT.m_x-m_ptLB.m_x)/2;
		m_db = fabs(m_ptRT.m_y-m_ptLB.m_y)/2;

		double dx =0;
		double dy =0;
		int radian =0;
		// 清除椭圆边界上的指针
		for (unsigned int i=0; i<m_vectPoints.size(); i++)
		{
			if (m_vectPoints.at(i))
			{
				delete m_vectPoints.at(i);
				m_vectPoints.at(i)=NULL;
			}
		}
		// 清除数组
		m_vectPoints.clear();

		// 迭代
		while(radian<360)
		{
			if (radian==0)
			{
				m_vectPoints.push_back(new CHD2DPoint(m_CenterPoint.m_x+m_da,m_CenterPoint.m_y));
				radian+=angle;
				continue;
			}
			else if (radian==90)
			{
				m_vectPoints.push_back(new CHD2DPoint(m_CenterPoint.m_x,m_CenterPoint.m_y+m_db));
				radian+=angle;
				continue;
			}
			else if (radian==180)
			{
				m_vectPoints.push_back(new CHD2DPoint(m_CenterPoint.m_x-m_da,m_CenterPoint.m_y));
				radian+=angle;
				continue;
			}
			else if (radian==270)
			{
				m_vectPoints.push_back(new CHD2DPoint(m_CenterPoint.m_x,m_CenterPoint.m_y-m_db));
				radian+=angle;
				continue;	
			}
			else
			{
				dx = m_CenterPoint.m_x + m_da*cos(radian*PI/180);
				dy = m_CenterPoint.m_y + m_db*sin(radian*PI/180);
				m_vectPoints.push_back(new CHD2DPoint(dx,dy));
				radian+=angle;
			}
		}
	}
	
	// 赋值函数
	CHD2DEllipse& CHD2DEllipse::operator=(const CHD2DEllipse& other)
	{
		m_ptLB.m_x = other.m_ptLB.m_x;
		m_ptLB.m_y = other.m_ptLB.m_y;
		m_ptRT.m_x = other.m_ptRT.m_x;
		m_ptRT.m_y = other.m_ptRT.m_y;
		m_da = other.m_da;
		m_db = other.m_db;
		m_CenterPoint.m_x = other.m_CenterPoint.m_x;
		m_CenterPoint.m_y = other.m_CenterPoint.m_y;
		
		// 循环赋值椭圆边界上的点 
		for (vector<CHD2DPoint*>::const_iterator it=other.m_vectPoints.begin();it!=other.m_vectPoints.end();it++)
		{
			m_vectPoints.push_back(new CHD2DPoint((*it)->m_x,(*it)->m_y));
		}
		return *this;
	}
}

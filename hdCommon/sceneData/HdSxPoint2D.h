/*! HdSxPoint2D.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HdPoint.h
相关文件     :

文件实现功能 : 定义点对象类
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/03/04   1.0      龚书林
</PRE>
*******************************************************************************/
#pragma once
#include "..\hdobject.h"
#include "..\HD2DPoint.h"
#include <string>
using namespace std;
namespace hd
{
	class CHdSxPoint2D:
		public CHD2DPoint
	{
	public:
		CHdSxPoint2D(void)
			:CHD2DPoint(0.0,0.0),m_fRow(-1.0), m_fCol(-1.0)
		{
		}
		CHdSxPoint2D(double x, double y)
			:CHD2DPoint(x,y),m_fRow(-1.0), m_fCol(-1.0)
		{
		}
		CHdSxPoint2D(double x,double y,double row,double col)
			:CHD2DPoint(x,y),m_fRow(row), m_fCol(col)
		{
		}

		virtual ~CHdSxPoint2D(void){}

	public:

		double m_fRow;	//对应的点云的行比例
		double m_fCol;	//对应的点云的列比例

	public:
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_POINT2D;}

		//! 判断点是否为平面或快速视图获取到的点
		bool isPixelPoint() const
		{
			return (m_fRow >= 0.0f && m_fRow <= 1.0 &&
				m_fCol >= 0.0f && m_fCol <= 1.0);
		}
		void setRowColRatio(double fRow, double fCol)
		{
			m_fRow = fRow;
			m_fCol = fCol;
		}
		
	};
}
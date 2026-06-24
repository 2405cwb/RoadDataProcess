/*! HdSxPolyline2D.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HdSxPolyline2D.h
相关文件     :

文件实现功能 : 定义点对象类
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/03/07   1.0      龚书林    
</PRE>
*******************************************************************************/
#pragma once
#pragma  warning(disable:4251)
#include "..\hdobject.h"
#include "HdSxPoint2D.h"
#include <vector>

using namespace std;

namespace hd
{
	class HDCOMMON_API CHdSxPolyline2D:
		public CHDObject
	{
	public:
		CHdSxPolyline2D(void);
		virtual ~CHdSxPolyline2D(void);

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const {return ESDT_OBJECT_POLYLINE2D;}

		//多线段编辑接口
		void AddPoint(const CHdSxPoint2D& point);

		void AddPoint(double x, double y);

		void DeletePoint(unsigned int nIndex);

		void InsertAfter(unsigned int nIndex, const CHdSxPoint2D& point);
		
		bool SetLastPoint(double x, double y);

		bool SetLastPoint(float colScale,float rowScale,double x, double y);

		void SetPoint(unsigned int nIndex,const CHdSxPoint2D& pPoint, bool bReCalcBox = false);
		
		void DeleteLastPoint();
		
		void Close();//将二维多段线闭合

		void Clear()
		{
			m_Polyline2D.clear();
			m_MinPt = m_MaxPt = CHdSxPoint2D(0.0, 0.0);
		}
		void ClosePlanar(); 

		unsigned int GetPointCount() const;

		const CHdSxPoint2D& GetPoint(unsigned int nIndex) const;

		const CHdSxPoint2D& GetBoxMinPt() const { return m_MinPt; }
		const CHdSxPoint2D& GetBoxMaxPt() const { return m_MaxPt; }


	private:
		void UpdateBox(const CHdSxPoint2D& point);
		void RecalclateBox();
			

	private:
		//定义多线段，由多个点构成;
		vector<CHdSxPoint2D>	m_Polyline2D;
		//定义线段的范围
		CHdSxPoint2D			m_MinPt;
		CHdSxPoint2D			m_MaxPt;
	};
}


/*! HdRoutePoint.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HdRoutePoint.h
相关文件     : HDObject.h 

文件实现功能 : 定义轨迹点类
作者         : 危迟
版本         : 1.0

</PRE>
*******************************************************************************/
#pragma once
#include "hdobject.h"
#include "HD3DPoint.h"

namespace hd
{
	class CHdRoutePoint :
		public CHDObject
	{
	public:
		CHdRoutePoint(void):nIndex(0) {}
		CHdRoutePoint(int idx):nIndex(idx) {}
		virtual ~CHdRoutePoint(void) {}

	public:
		CHD3DPoint m_Position;
		int nIndex;

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_ROUTEPOINT; }

	};
}
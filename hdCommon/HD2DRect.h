/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：CHD2DRect.h
相关文件	: CHD2DRect.cpp	HD2DObject.h	HDBaseStruct.h
文件实现功能：二维矩形对象。
作者		：姚立
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2013/06/13	1.0			任高强		创建
</PRE>
******************************************************************************************************/

#pragma once
#pragma  warning(disable:4251)
#include "hd2dobject.h"
#include "HD2DPoint.h"

namespace hd
{
	class HDCOMMON_API CHD2DRect :
		public CHD2DObject
	{
	public:
		CHD2DRect(void);
		virtual ~CHD2DRect(void);

		//得到外接矩形
		virtual CHD2DBoundingBox GetBoundingBox() const;
		//得到类型
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_2DRECT;}
		// 赋值运算
		CHD2DRect& operator=(const CHD2DRect& other);
		// pt是否在rect中
		bool IsPointInRect(const CHD2DPoint& pt) const;

	public:
		CHD2DPoint			m_ptLB;				// 左下角点
		CHD2DPoint			m_ptRT;				// 右上角点
		bool				m_bIsRect;			// 是否是矩形
	};
}
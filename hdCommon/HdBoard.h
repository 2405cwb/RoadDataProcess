/*!@file
*******************************************************************************************************
<PRE>
模块名		：HDFacade.h
文件名		：CHDFacade.h
相关文件	: CHDFacade.cpp 	HDFace.h	HD3DPolyline.h
文件实现功能：广告牌，从CHDFace继承，用来描述建筑物或目标物的立面。
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2013/4/23	1.0			马振明		创建
</PRE>
******************************************************************************************************/
#pragma once
#include "HDFacade.h"
#include "HD3DPolyline.h"
#include "HD3DPoint.h"

namespace hd
{
	class HDCOMMON_API CHdBoard: public CHDFacade
	{
	public:
		CHdBoard(void);
		~CHdBoard(void);
	public:
		virtual ENUM_HDMS_OBJECT_TYPE GetType () const { return E_HOT_BOARD;}
		
		string GetMarkerID()
		{
			return m_strMarkerID;
		}
		
	public:
		string m_strMarkerID;
	};
}



/*!@file
*******************************************************************************************************
<PRE>
模块名		：HD2DObject.h
文件名		：CHD2DObject.h
相关文件	: HDObject.h	HD2DBoundingBox.h
文件实现功能：抽象类，从CHDObject继承，二维对象的基类。
作者		：任高强
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/1/6	1.0			任高强		创建
</PRE>
******************************************************************************************************/

#pragma once
#include "HDObject.h"
#include "HD2DBoundingBox.h"

namespace hd
{

	class HDCOMMON_API CHD2DObject:public CHDObject
	{
	public:
		CHD2DObject(void){};
		virtual ~CHD2DObject(void){};

	public:
		virtual CHD2DBoundingBox GetBoundingBox() const = 0;
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_2DOBJECT;}
	};

}
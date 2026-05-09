/*!@file
*******************************************************************************************************
<PRE>
模块名		：HD3DObject.h
文件名		：CHD3DObject.h
相关文件	: HDObject.h	HD3DBoundingBox.h
文件实现功能：抽象类，从CHDObject继承，三维对象的基类。
作者		：孙文
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/1/6	1.0			孙文		创建
</PRE>
******************************************************************************************************/

#pragma once
#include "HDObject.h"
#include "HD3DBoundingBox.h"

namespace hd
{
	class HDCOMMON_API CHD3DObject : public CHDObject
	{
	public:
		CHD3DObject(void){};
		virtual ~CHD3DObject(void){};

	public:
		virtual CHD3DBoundingBox GetBoundingBox() const = 0;
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_3DOBJECT;}
		virtual bool IsInSphere(double dX, double dY, double dZ, double dR) const = 0;
	};

}
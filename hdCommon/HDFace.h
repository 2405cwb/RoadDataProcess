/*!@file
*******************************************************************************************************
<PRE>
模块名		：HDFace.h
文件名		：CHDFace.h
相关文件	: HD3DObject.h	HDBaseStruct.h
文件实现功能：抽象类，从CHD3DObject继承，是所有面对象的基类。
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
#include "HD3DObject.h"
#include "HDBaseStruct.h"

namespace hd
{
	class HDCOMMON_API CHDFace : public CHD3DObject
	{
	public:
		CHDFace(void){};
		virtual ~CHDFace(void){};

	public:
		virtual ENUM_HDMS_OBJECT_TYPE GetType () const { return E_HOT_FACE;}
		virtual CHD3DBoundingBox GetBoundingBox() const = 0;
		// 判断对象是否在球体内
		virtual bool IsInSphere(double dX, double dY, double dZ, double dR) const = 0;
		// 得到面的法向量
		virtual HD_3DPOINT GetNormal() = 0;
	};

}
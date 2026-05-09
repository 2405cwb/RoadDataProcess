/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：CHDCrossLine.h
相关文件	: CHDCrossLine.cpp	HD2DObject.h	HDBaseStruct.h
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
#include "HD3DPoint.h"

namespace hd
{
	class HDCOMMON_API CHDCrossLine :
		public CHD3DObject
	{
	public:
		CHDCrossLine(void);
		virtual ~CHDCrossLine(void);

		//得到外接矩形
		virtual CHD3DBoundingBox GetBoundingBox() const;
		//得到类型
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_CROSSLINE;}
		// 赋值运算
		CHDCrossLine& operator=(const CHDCrossLine& other);

	public:
		/*      +
		 */
		CHD3DPoint          m_ptUp;            // 上面点     
		CHD3DPoint          m_ptDown;          // 下面点
		CHD3DPoint          m_ptLeft;          // 左边点
		CHD3DPoint          m_ptRight;         // 右边点
		CHD3DPoint          m_ptCenter;        // 中间点
	};
}
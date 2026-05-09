/*!@file
*******************************************************************************************************
<PRE>
模块名		：HDPanoMarker.h
文件名		：CHDPanoMarker.h	
相关文件	: CHDPanoMarker.cpp		HD3DObject.h	HDBaseStruct.h
文件实现功能：全景标注对象，用来描述一个全景标注。从CHD3DObject继承。
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
#include "HD3DNormalPoint.h"

namespace hd
{
	class HDCOMMON_API CHDPanoMarker : public CHD3DObject
	{
	public:
		CHDPanoMarker(void);
		CHDPanoMarker(const char* strName, const char* m_strSymbolID, CHD3DNormalPoint& normalPt, double dScaleX = 1.0, double dScaleY = 1.0, double dScaleZ = 1.0);
		virtual ~CHDPanoMarker(void);

	public:
		virtual ENUM_HDMS_OBJECT_TYPE GetType () const { return E_HOT_PANOMARKER;}
		virtual CHD3DBoundingBox GetBoundingBox() const;
		// 判断对象是否在球体内
		virtual bool IsInSphere(double dX, double dY, double dZ, double dR) const;
		// 得到标注点的位置
		CHD3DNormalPoint GetPosition() const;
		// 设置标注点的位置
		void SetPosition(CHD3DNormalPoint& normalPt);
		// 得到标注的名称
		const char* GetName() const;
		// 设置标注的名称
		void SetName(const char* strName);
		// 得到符号ID
		const char* GetSymbolID() const;
		// 设置符号ID
		void SetSymbolID(const char* strSymbolID);
		// 得到缩放比例
		void GetScale(double& dScaleX, double& dScaleY, double& dScaleZ) const;
		// 设置缩放比例
		void SetScale(double dScaleX, double dScaleY, double dScaleZ);

	private:
		char				m_strName[NAME_LEN];			//标注名称
		char				m_strSymbolID[OBJECT_ID_LEN];	//符号的ID
		CHD3DNormalPoint	m_Pos;							//全景标注点的位置及法向量
		double				m_dScaleX;						//X方向的缩放比例，默认为1.0
		double				m_dScaleY;						//Y方向的缩放比例，默认为1.0
		double				m_dScaleZ;						//Z方向的缩放比例，默认为1.0

	};

}
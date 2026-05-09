/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdBasicObject
文件名		：CameraIntParam.h
相关文件	: 
文件实现功能：相机外参类
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/4/28	1.0			马振明		  移植
</PRE>
******************************************************************************************************/

#pragma once
#include "hdBasicObject.h"
#include "BaseVector2d.h"
#include "Camera.h"

class BASICOBJECT_API CCameraExtParam :
	public CHdBasicObject
{
public:
	CCameraExtParam(void);
	virtual ~CCameraExtParam(void);
	
	public:
	// 获取旋转矩阵
	void GetRoation(double* dRoation) const;

	// 更新角度
	void UpdateAngle(double* dRoation);

	// 姿态
	double m_dPhi;
	double m_dOmg;
	double m_dKap;

	// 位置
	double m_dXs;
	double m_dYs;
	double m_dZs;
	CAM_ROATION_TYPE m_CamRoationType;				// 外参的旋转系统
};


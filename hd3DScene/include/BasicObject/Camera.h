#pragma once
#include "hdBasicObject.h"

// 定义相机的旋转系统
enum CAM_ROATION_TYPE
{
	E_RT_ZXY =0,			// Z,X,Y，即yaw,pitch，raw的方式
	E_RT_ZYX =1,			
};

class BASICOBJECT_API CCamera :
	public CHdBasicObject
{
public:
	CCamera(void);
	virtual ~CCamera(void);
};


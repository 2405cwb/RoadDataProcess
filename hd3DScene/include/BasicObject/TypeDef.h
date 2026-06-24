/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdBasicObject
文件名		：TypeDef.h
相关文件	: 
文件实现功能：定义类型枚举变量
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/4/22	1.0			马振明		  移植
</PRE>
******************************************************************************************************/

#pragma once

enum E_TYPE_ID
{
	//未知类型
	E_TID_UNKNOWN	= 0x00000000,

	//矢量类型ID定义
	E_TID_VECTOR	= 0x10000000,

	E_TID_POINT		= 0x11000000,
	E_TID_POINT_2D	= 0x11000002,
	E_TID_POINT_3D	= 0x11000003,

	E_TID_POLYLINE		= 0x12000000,
	E_TID_POLYLINE_2D	= 0x12000002,
	E_TID_POLYLINE_3D	= 0x12000003,

	E_TID_POLYGON		= 0x13000000,

	E_TID_RECT			= 0x13000001,

	//栅格类型ID定义
	E_TID_RASTER		= 0x20000000,
	E_TID_GEO_RASTER	= 0x21000000,
	E_TID_GEO_IMAGE		= 0x21000001,
	E_TID_DEM_RASTER	= 0x21000002,

	//文本类型ID定义
	E_TID_TEXT		= 0x30000000,
	E_TID_LABEL		= 0x30000001,

	//相机相关
	E_TID_CAMERA				= 0x40000000,
	E_TID_CAMERA_PARM			= 0x41000000,
	E_TID_CAMERA_PARM_INT		= 0x41000001,
	E_TID_CAMERA_PARM_EXT		= 0x41000002,
	E_TID_CAMERA_MODEL			= 0x42000000,
	E_TID_CAMERA_MODEL_NORMAL	= 0x42000001,
	E_TID_CAMERA_MODEL_FISHEYE	= 0x42000002,

	//变换模型
	E_TID_TRANS					= 0x50000000,
	E_TID_TRANS_LINEAR			= 0x50000001,
	E_TID_TRANS_AFFINE			= 0x50000002,
	E_TID_TRANS_PROJECT			= 0x50000003
};
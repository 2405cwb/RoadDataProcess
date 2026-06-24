#pragma once
#include "hdCommon.h"
#include "hdConstDef.h"
#include "..\hdCore\tinyxml.h"

namespace hd
{
	enum ENUM_HDMS_OBJECT_TYPE
	{
		E_HOT_UNKNOWN			= 0x00000000,		//未知类型
		E_HOT_2DOBJECT			= 0x00000001,		//二维对象
		E_HOT_2DPOINT			= 0x00000003,		//二维点对象
		E_HOT_NODE				= 0x00000013,		//邻接点对象，属于二维点
		E_HOT_2DPOLYLINE		= 0x00000005,		//二维多线段
		E_HOT_2DPOLYGON			= 0x00000009,		//二维多边形
		E_HOT_2DCIRCLE			= 0x00000019,		//二维圆
		E_HOT_2DELLIPSE			= 0x00000020,		//二维椭圆
		E_HOT_2DRECT			= 0x00000029,		//二维矩形
		E_HOT_3DOBJECT			= 0x00001000,		//三维对象
		E_HOT_3DPOINT			= 0x00003000,		//三维点对象
		E_HOT_3DNORMALPOINT		= 0x00013000,		//三维点对象——带法向量
		E_HOT_3DPOLYLINE		= 0x00005000,		//三维多线段
		E_HOT_FACE				= 0x00007000,		//三维面
		E_HOT_FACADE			= 0x00017000,		//三维立面
		E_HOT_BOARD				= 0x00017001,		//广告牌
		E_HOT_PALNE				= 0x00027000, 		//三维平面
		E_HOT_CUBE				= 0x00027001, 		//三维六面体 室内全景六面体标注专用 [11/5/2013 liujun]
		E_HOT_MARKER_FACADE		= 0x00027002,		//三维立面标注 室内全景六面体标注专用 [11/5/2013 liujun]
		E_HOT_PANOMARKER		= 0x01000000,		//全景标注点
		E_HOT_PANOSYMBOL		= 0x02000000,		//全景符号
		E_HOT_SHPMARKERSYMBOL   = 0x06000000,       //全景shp标注符号
		E_HOT_ROUTE				= 0x03000000,		//轨迹
		E_HOT_ROUTEFRAGMENT		= 0x04000000,		//轨迹片断
		E_HOT_DBROUTE			= 0x05000000,		//轨迹数据类
		E_HOT_SVLINK			= 0x06000000,		//连接关系
		E_HOT_PANOFILE			= 0x01200000,		//全景文件
		E_HOT_PANODATA			= 0x01300000,		//全景内存流
		E_HOT_POINTCLOUD		= 0x01400000,		//点云对象
		E_HOT_CROSSLINE			= 0x01500000,		//三维十字
		E_HOT_SEADATA		    = 0x01600000,		//海量点云
		E_HOT_IMAGECTRLPT		= 0x01700000,		//控制点
		E_HOT_2DIMAGEPT			= 0x01800000,		//二维影像点
		E_HOT_CTRLPTPAIR		= 0x01900000,		//控制点对
        E_HOT_SKETCHTRIANGLE    = 0x02000000,       // 测站中心三角形


		//----------------------------HD Scenen数据类型------------------------------------------------
		ESDT_UNKNOWN			= 0x00000000,
		ESDT_FILE				= 0x00000110,			//此类型表示对象为文件类
		ESDT_FILE_HLS			= 0x00000130,
		ESDT_FILE_LAS			= 0x00000140,
		ESDT_FILE_COLOR_PIC		= 0x00000150,
		ESDT_FILE_GREY_PIC		= 0x00000170,
		ESDT_FILE_PANO_PIC		= 0x00000190,
		ESDT_FILE_SCANFILES		= 0x0000FFFF,			//定义为所有文件类的类型
		ESDT_OBJECT				= 0x00010000,			//此类型表示对象为内存对象，无对应的磁盘文件
		ESDT_OBJECT_POINT		= 0x00030000,
		ESDT_OBJECT_POINT2D		= 0x00031000,
		ESDT_OBJECT_CTRLPT		= 0x00032000,
		ESDT_IMAGE_CTRLPT		= 0x00033000,			// 影像配准控制点
		ESDT_OBJECT_LABEL		= 0x00034000,
		ESDT_OBJECT_SPHERE		= 0x00035000,
		ESDT_OBJECT_GPS_CTRLPT  = 0x00036000,
		ESDT_OBJECT_SCANCTRLPT  = 0x00037000,
		ESDT_OBJECT_KEYBOARD	= 0x00038000,			//
		ESDT_OBJECT_ORIENTPOINT = 0x00039000,			// 定向点
		ESDT_OBJECT_FEATURE_POINT = 0x00040000,			// 特征点
		ESDT_OBJECT_CONTROLPOINTSET  = 0x00040001,       // 外部导入的控制点集
		ESDT_OBJECT_HS_TARGET = 0x00040002,			   // hs系列标靶
		ESDT_OBJECT_PLANE		= 0x00041000,			//面对象;
		ESDT_OBJECT_POLYLINE	= 0x00050000,
		ESDT_OBJECT_POLYLINE2D	= 0x00051000,
		ESDT_OBJECT_POLYLINE_ADJACENT  = 0x00052000,    //草图视图下邻接关系线
		ESDT_OBJECT_FIT			= 0x00060000,
		ESDT_OBJECT_MCAM_CTRLPT = 0x00061000,			// M-Cam相机标定控制点
		ESDT_OBJECT_ROUTEPOINT  = 0x00081000,
		ESDT_OBJECT_ROUTEPOINTS  = 0x00080000,	
		ESDT_OBJECT_FACE		= 0x00090000,
		ESDT_SCENE_SCAN			= 0xFFFFFFFF,			//包含所有对象，包括文件对象和内存对象
		ESDT_OBJECT_CADPOLYLINE	= 0x00100000,           //CAD线对象

		/*-------------------iScan工程对象对象-------------------------*/
		ESDT_SCENE_ISCAN        = 0x00090000,

		/*-------------------OGR图层、几何对象对象-------------------------*/
		ESDT_OBJECT_LAYER		    = 0x00070000,
		ESDT_OBJECT_OGRPOINT        = 0x00070001,
		ESDT_OBJECT_OGRPOLYLINE     = 0x00070002,
        ESDT_OBJECT_OGRPOLYGON      = 0x00070003,
        ESDT_OBJECT_GRID            = 0x00070004,
	

		/*-------------拼接工程下拼接组/拼接对/拼接测站/控制点对象-------------*/
		ESDT_OBJECT_REGPROJ_GROUP = 0x00000301,
		ESDT_OBJECT_REGPROJ_PAIR  = 0x00000302,
		ESDT_OBJECT_REGPROJ_SCAN  = 0x00000303,
		ESDT_OBJECT_REGPROJ_CTRL  = 0x00000304

	};


	class HDCOMMON_API CHDObject
	{
	public:
		CHDObject(void) {};
		virtual ~CHDObject(void) {};

		// 得到对象的类型
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const = 0;
	
	};
}
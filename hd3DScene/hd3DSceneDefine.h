/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : hd3DSceneDefine.h
相关文件     : 
文件实现功能 : 定义三维视图渲染
作者         : 软件部，危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/07/05	1.0        危迟					创建
2013/09/12  1.1		   危迟				 声明分类相关场景结点全局ID
2013/09/30  1.2        蔡红云            增加视图枚举类型
2013/12/10  1.3        蔡红云            增加点云渲染尺寸枚举
2014/01/18  1.4        危迟				 增加模型渲染类型枚举
</PRE>
*******************************************************************************/
#pragma once

namespace hd
{
	typedef enum ENUM_RENDERSTYLE {
		RENDER_BY_Z = 0,				//根据Z值渲染
		RENDER_BY_INTENSITY,			//根据反射强度渲染
		RENDER_BY_DEFAULT,				//默认的用default显示
		RENDER_BY_RGB,				    //根据点云文件中的RGB显示
		RENDER_BY_CLASS,				//按分类渲染
		RENDER_BY_X,					//根据X值渲染
		RENDER_BY_Y,					//根据Y值渲染
		RENDER_BY_DISTANCE,				//根据距离渲染
		RENDER_BY_COL,                  //根据选择列范围渲染
		RENDER_BY_CYCLERAMP,			//按照循环色带渲染
		RENDER_BY_FEATURE_BIN,			//按照分类实时渲染
		RENDER_AREA_BY_Z,               //选择区域按高程Z值渲染
		RENDER_AREA_BY_Y,               //选择区域按Y值渲染
		RENDER_AREA_BY_X,               //选择区域按X值渲染
		RENDER_AREA_BY_DEFAULT,         //选择区域按默认（蓝色）渲染
		RENDER_AREA_BY_INTENSITY,       //选择区域按反射强度渲染
		RENDER_AREA_BY_CLASSIFY,        //选择区域按分类渲染
		RENDER_BY_CLASS_AND_Z,			//按照类别和高程渲染
	} ENUM_RENDERSTYLE;

	// 模型渲染方式 fengjing
	typedef enum ENUM_DEM_RENDERSTYLE {
		RENDER_DEM_BY_WIREFRAME_ACOLOR = 0,		//根据线框渲染
		RENDER_DEM_BY_WIREFRAME_Z,				
		RENDER_DEM_BY_WIREFRAME_CYCLERAMP,
		RENDER_DEM_BY_ENTITY_ACOLOR,			//根据实体渲染
		RENDER_DEM_BY_ENTITY_Z,
		RENDER_DEM_BY_ENTITY_CYCLERAMP,
		RENDER_DEM_BY_POINTCLOUD_ACOLOR,		//根据点云渲染
		RENDER_DEM_BY_POINTCLOUD_Z,
		RENDER_DEM_BY_POINTCLOUD_CYCLERAMP,
	} ENUM_DEM_RENDERSTYLE;

	typedef enum ENUM_RENDERSIZE
	{
		E_RS_SIZE1 = 1,                   // 小尺寸点
		E_RS_SIZE2,                       // 中尺寸点    
		E_RS_SIZE3,                       // 大尺寸点
	}ENUM_RENDERSIZE;

	enum ENUM_HD_3D_PROJECTION_TYPE
	{
		E_HPT_PERSPECTIVE		= 0,			 // 透视投影
		E_HPT_ORTHOGONAL		= 1,			 // 正交投影
	};

	typedef enum ENUM_SHOWSTYLE {
		SHOW_ALL = 0,					//显示所有
		SHOW_SELECT,					//显示选择
		SHOW_UNSELECT,					//显示未选择
	} ENUM_SHOWSTYLE;

	typedef enum ENUM_HD_ADJUST_MODE
	{
		E_HVM_OFFSET = 0,				// 平移
		E_HVM_ROTATE = 1,				// 缩放
	}ENUM_HD_ADJUST_MODE;

	typedef enum ENUM_HD_VIEW_ANGLE
	{
		E_HVA_XOY    = 0,				// 俯视图 XOY平面
		E_HVA_XOZ    = 1,				// 前视图 XOZ平面
		E_HVA_YOZ    = 2,				// 侧视图 YOZ平面
		E_HVA_ISO    = 3,				// 任意视角 
		E_HVA_SLICE	 = 4,				// 剖视图
		E_HVA_SLOPE_SLICE = 5,          // 滑坡剖面视图（视角自定义，垂直于剖面）
	}ENUM_HD_VIEW_ANGLE;

	typedef enum ENUM_HD_ORTHOVIEW_ANGLE
	{
		E_HOA_NEAR = 0,					// 近平面
		E_HOA_RIGHT = 1,				// 右平面
		E_HOA_TOP = 2,					// 顶平面
	}ENUM_HD_ORTHOVIEW_ANGLE;

	typedef enum ENUM_HD_BOX_OPERATION
	{
		E_HBO_OFFSET = 0,				// 平移box
		E_HBO_ROTATE = 1,				// 旋转box
		E_HBO_RESIZE = 2,				// 拉伸box
		E_HBO_BOTH	 = 3,				// 全部
		E_HBO_NONE	 = 4,				// 无操作
	}ENUM_HD_BOX_OPERATION;
	typedef enum ENUM_CAMERA_POSITION
	{
		E_CP_USER_POSITION=0,   // 默认视角
		E_CP_FRONT,             // 前视图
		E_CP_BACK,              // 后视图 
		E_CP_TOP,               // 顶视图
		E_CP_BOTTOM,            // 底视图
		E_CP_LEFT,              // 左视图
		E_CP_RIGHT,             // 右视图
		E_CP_ISO_LEFT,          // ISO左视图
		E_CP_ISO_INVER_LEFT,     // ISO左视图 + CTRL 反转  
		E_CP_ISO_RIGHT,          // ISO右视图
		E_CP_ISO_INVER_RIGHT,    // ISO右视图 + CTRL 反转
	}ENUM_CAMERA_POSITION;

	typedef enum ENUM_HD_MODEL_RENDERSTYLE
	{
		E_HMR_WIREFRAME = 0,	// 线框风格
		E_HMR_ENTITY	= 1,	// 实体风格
		E_HMR_POINTCLOUD = 2,	// 点云风格
	}ENUM_HD_MODEL_RENDERSTYLE;

	typedef enum ENUM_HD_MODEL_TYPE
	{
		E_HMT_DEM = 0,			// 规则DEM
		E_HMT_TIN = 1,			// 不规则三角网
		R_HMT_MESH = 2,			// 三维Mesh
		E_HMT_CUTFILL = 3,      // 挖填
	}ENUM_HD_MODEL_TYPE;

	// 蔡红云 2013/9/3 控制球操作结构体
	typedef enum ENUM_3DBOX_SHPERE
	{
		E_3S_XU = 0,				// 控制X正方向拉伸
		E_3S_XD = 1,				// 控制X负方向拉伸
		E_3S_YU = 2,				// 控制Y正方向拉伸
		E_3S_YD = 3,				// 控制Y负方向拉伸
		E_3S_ZU = 4,				// 控制Z正方向拉伸
		E_3S_ZD = 5,				// 控制Z负方向拉伸
		E_3S_R  = 6,                // 控制旋转
		E_3S_T  = 7,                // 控制平移
		E_3S_O  = 8,                // 默认类型不操作
	}ENUM_3DBOX_SHPERE;


	enum ENUM_HD_SCANROUTE_MOVE_TYPE
	{
		E_HDRT_MOVE_FIRST 		= 1,            // 移动到第一帧
		E_HDRT_MOVE_LAST		= 2,			// 移动到最后一帧 
		E_HDRT_MOVE_NEST		= 3,			// 移动到下一帧
		E_HDRT_MOVE_PRE		    = 4,			// 移动到前一帧
		E_HDRT_PLAY_NEXT        = 5,            // 连续向前播放
		E_HDRT_PLAY_PRE         = 6,            // 连续向后播放
		E_HDRT_MOVE_TO_FRAME    = 7,            // 跳转至指定帧
		E_HDRT_STOP_PLAY        = 8,
		E_HDRT_UNKNOWN			= 99,			// 未知类型
	};
	
}

// 场景结点ID号全局声明 [2013-09-12 危迟]
// 1.声明后，场景结点ID号被独占，其它场景结点ID在创建时，不应使用已经声明的ID号
// 2.场景结点ID号HDScene软件中占用范围1000-5000 
// 3.使用ID声明的目的是便于管理，在某些场景下使用更加高效
#define CLASSIFY_VERTICAL_RECT_SN	898		// HdScene分类界面俯视图窗口选择
#define CLASSIFY_3DWND_RECT_SN		899		// HdScene分类界面三维选择盒在俯视图下显示结点
// Copyright (C) 2002-2010 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __E_SCENE_NODE_TYPES_H_INCLUDED__
#define __E_SCENE_NODE_TYPES_H_INCLUDED__

#include "irrTypes.h"

namespace irr
{
namespace scene
{

	//! An enumeration for all types of built-in scene nodes
	/** A scene node type is represented by a four character code
	such as 'cube' or 'mesh' instead of simple numbers, to avoid
	name clashes with external scene nodes.*/
	enum ESCENE_NODE_TYPE
	{
		//! simple cube scene node
		ESNT_CUBE           = MAKE_IRR_ID('c','u','b','e'),

		//! Sphere scene node
		ESNT_SPHERE         = MAKE_IRR_ID('s','p','h','r'),

		//! Text Scene Node
		ESNT_TEXT           = MAKE_IRR_ID('t','e','x','t'),

		//! Water Surface Scene Node
		ESNT_WATER_SURFACE  = MAKE_IRR_ID('w','a','t','r'),

		//! Terrain Scene Node
		ESNT_TERRAIN        = MAKE_IRR_ID('t','e','r','r'),

		//! Sky Box Scene Node
		ESNT_SKY_BOX        = MAKE_IRR_ID('s','k','y','_'),

		//! Sky Dome Scene Node
		ESNT_SKY_DOME       = MAKE_IRR_ID('s','k','y','d'),


		//! Shadow Volume Scene Node
		ESNT_SHADOW_VOLUME  = MAKE_IRR_ID('s','h','d','w'),

		//! Octree Scene Node
		ESNT_OCTREE         = MAKE_IRR_ID('o','c','t','r'),

		//! Mesh Scene Node
		ESNT_MESH           = MAKE_IRR_ID('m','e','s','h'),

		//! Light Scene Node
		ESNT_LIGHT          = MAKE_IRR_ID('l','g','h','t'),

		//! Empty Scene Node
		ESNT_EMPTY          = MAKE_IRR_ID('e','m','t','y'),

		//! Dummy Transformation Scene Node
		ESNT_DUMMY_TRANSFORMATION = MAKE_IRR_ID('d','m','m','y'),

		//! Camera Scene Node
		ESNT_CAMERA         = MAKE_IRR_ID('c','a','m','_'),

		//! Billboard Scene Node
		ESNT_BILLBOARD      = MAKE_IRR_ID('b','i','l','l'),

		//! Animated Mesh Scene Node
		ESNT_ANIMATED_MESH  = MAKE_IRR_ID('a','m','s','h'),

		//! Particle System Scene Node
		ESNT_PARTICLE_SYSTEM = MAKE_IRR_ID('p','t','c','l'),

		//! Quake3 Shader Scene Node
		ESNT_Q3SHADER_SCENE_NODE  = MAKE_IRR_ID('q','3','s','h'),

		//! Quake3 Model Scene Node ( has tag to link to )
		ESNT_MD3_SCENE_NODE  = MAKE_IRR_ID('m','d','3','_'),

		//! Volume Light Scene Node
		ESNT_VOLUME_LIGHT  = MAKE_IRR_ID('v','o','l','l'),

		//! Maya Camera Scene Node
		/** Legacy, for loading version <= 1.4.x .irr files */
		ESNT_CAMERA_MAYA    = MAKE_IRR_ID('c','a','m','M'),

		//! First Person Shooter Camera
		/** Legacy, for loading version <= 1.4.x .irr files */
		ESNT_CAMERA_FPS     = MAKE_IRR_ID('c','a','m','F'),

		//******************************HDSY 自定义SceneNode类型注册********************************//
		//! 二维/三维多线段
		ESNT_IPOLYLINE		= MAKE_IRR_ID('p','y','l','e'),

		//! 自定义的camera;
		ESNT_CAMERA_CUS		= MAKE_IRR_ID('c','a','m','C'),

        //! 等边三角形Scene Node
        ESNT_TRIANGLE       = MAKE_IRR_ID('t', 'a', 'n', 'g'),

		//! HD坐标轴对象
		ESNT_HD_AXIS		= MAKE_IRR_ID('h','a','x','s'),

		//! HD指北针对象
		ESNT_HD_CPSS		= MAKE_IRR_ID('h','c','p','s'),

		//! HD帧数对象
		ESNT_HD_FPS			= MAKE_IRR_ID('h','f','p','s'),

		//! 体积相关结果显示
		ESNT_HD_VOLUME_RESULT = MAKE_IRR_ID('h','v','r','s'),

		//! Pano Scene Node
		ESNT_PANO			= MAKE_IRR_ID('p','a','n','o'),

		ESNT_PANO_POINT     = MAKE_IRR_ID('p','a','p','o'),

		//! Symbol Scene Node
		ESNT_PANO_SYMBOLE    = MAKE_IRR_ID('p','a','s','y'),

		//! 吉奥SHPScene Node
		ESNT_PANO_SHPSYMBOL  = MAKE_IRR_ID('s','h','s','y'),

		//! Planar Scene Node
		ESNT_PLANAR			= MAKE_IRR_ID('p','l','a','n'),

		//! DOM Scene node
		//ESNT_DOM_SCENE_NODE = MAKE_IRR_ID('d','m','s','n'),

		//! Polyline Scene Node
		ESNT_POLYLINE		= MAKE_IRR_ID('p','l','y','l'),

		//！ Adjacent PolyLine Scene Node
		ESNT_ADJACENT_POLYLINE = MAKE_IRR_ID('p','a','l','n'),

		//! CADPolyline Scene Node
		ESNT_CADPOLYLINE		= MAKE_IRR_ID('p','l','y','d'),

		//! MeshPoly Scene Node,探面的Scene Node
		ESNT_MESHPOLY		= MAKE_IRR_ID('m','s','p','l'),

		//! Point Scene Node
		ESNT_POINT			= MAKE_IRR_ID('p','o','i','t'),

		// 边坡监测特征点
		ESNT_SLOPPOINT = MAKE_IRR_ID('s','l','p','t'),

		//! Label Scene Node
		ESNT_LABEL			= MAKE_IRR_ID('l','a','b','l'),		

		//! ScanPoint Scene Node
		ESNT_SCAN_POINT			= MAKE_IRR_ID('s','n','p','t'),

		//! Part ScanPoint Scene Node
		ESNT_PART_SCAN_POINT		= MAKE_IRR_ID('s','n','p','a'),

		//! Test ScanPoint Scene Node
		ESNT_TESTSCAN_POINT			= MAKE_IRR_ID('t','n','p','t'),

		//! Point Scene Node
		ESNT_CTRL_POINT			= MAKE_IRR_ID('c','t','p','t'),

		//! OgrlayerScene Node
		ESNT_OGRL_SCENE_NODE    = MAKE_IRR_ID('o','r','s','n'),

		//! Image Ctrl Point
		ESNT_IMAGECTRL_POINT	= MAKE_IRR_ID('i','m','p','t'),

		//! HD chessboard 
		ESNT_HD_CHESSBOARD		= MAKE_IRR_ID('h','c','h','e'),

		//! HDOrientPoint 
		ESNT_HD_ORIENTPOINT		= MAKE_IRR_ID('h','o','t','p'),
		
		//! HDPolySelect 
		ESNT_HD_POLYSELECT		= MAKE_IRR_ID('h','p','l','s'),
		//! HDPolySelect 
		ESNT_HD_INCOMMON_POLYSELECT		= MAKE_IRR_ID('h','c','l','s'),

		//! measure_erea
		ESNT_HD_POLY_MEASURE_EREA		= MAKE_IRR_ID('h','p','m','e'),

		//!HD Sphere 
		ESNT_HD_SPHERE			= MAKE_IRR_ID('h','s','p','h'),

		//!HD Plane
		ESNT_HD_PLANE				= MAKE_IRR_ID('h','p','l','n'),

		//! HD RoutePoint
		ESNT_HD_ROUTEPOINT		= MAKE_IRR_ID('h','r','t','p'),

		//! mz routePoint
		ESNT_HD_MZ_ROUTEPOINT	= MAKE_IRR_ID('h','m','t','p'),

		//! HD RoutePoints
		ESNT_HD_ROUTEPOINTS		= MAKE_IRR_ID('h','r','p','s'),

		//! 剖面选择
		ESNT_HD_SECTION_SELECT  = MAKE_IRR_ID('h','p','s','s'),

		//!HD Mls对象
		//ESNT_HD_MLS				= MAKE_IRR_ID('h','m','l','s'),

		//! HD Ovl对象
		//ESNT_HD_OVL				= MAKE_IRR_ID('h','o','v','l'),

		//! 背景scenenode
		ESNT_HD_BACKGROUND      = MAKE_IRR_ID('h','b','k','g'),

		//! 截屏
		ESNT_HD_SCREENSHOT		= MAKE_IRR_ID('h','s','c','t'),

		//! 碰撞检测射线
		ESNT_HD_COLLISIONRAY	= MAKE_IRR_ID('h','d','c','r'),

		//! 平面选择
		ESNT_HD_PLANARSELECT    = MAKE_IRR_ID('h','d','p','s'),

		//! 可移动圆用于选择圆心控制点
		ESNT_HD_POINTPICK_CIRCLE = MAKE_IRR_ID('h','d','p','c'),

		//! 色彩图例
		ESNT_COLOR_LEGEND        = MAKE_IRR_ID('h','c','l','d'),

		//! 比例尺图例
		ESNT_RADIO_LEGEND        = MAKE_IRR_ID('h','r','l','d'),

		//! 测站调整旋转圆圈
		ESNT_ROTATE_CIRCLE       = MAKE_IRR_ID('h','r','t','c'),

        //! 草图视图下测站绕Z轴旋转的圆
        ESNT_Z_ROTATE_CIRCLE     = MAKE_IRR_ID('h', 'z', 'r', 'c'),

		//! 网格模型
		ESNT_MESH_MODEL			 = MAKE_IRR_ID('h','d','m','m'),

		//! 分类功能点云场景结点
		ESNT_CLASSIFY_PTD	= MAKE_IRR_ID('h','c','p','c'),

		//! 三维选择盒
		ESNT_SELECT_BOX		= MAKE_IRR_ID('h','s','b','x'),

		//! 显示视图对应名称
		ESNT_VIEW_PCD_NAME  = MAKE_IRR_ID('h','v','p','n'),

		//! 俯视图 剖面选择
		ESNT_VERTICAL_SECTION = MAKE_IRR_ID('h','v','s','n'),

		//! 画刷分类工具
		ESNT_CLASSIFY_BRUSH = MAKE_IRR_ID('h','c','b','h'),

		//! Facade Scene Node
		ESNT_FACADE		= MAKE_IRR_ID('f','a','d','e'),

		// 标注软件专用face Secen Node
		ESNT_MARKER_FACE  = MAKE_IRR_ID('m','k','f','a'),

		// 标注软件专用CUBE Secen Node
		ESNT_MARKER_CUBE  = MAKE_IRR_ID('m','k','c','b'),

		//! 绘制至CAD三维点
		ESNT_IPOINT		= MAKE_IRR_ID('i','p','n','t'),

		//! ScanPoint Scene Node
		ESNT_SCAN_POINT_LOD	= MAKE_IRR_ID('s','n','p','l'),

		//! DEM 场景结点
		ESNT_HD_DEM =  MAKE_IRR_ID('h','d','e','m'),

		//! TIN
		ESNT_HD_TIN		= MAKE_IRR_ID('h','t','i','n'),

		//! CUTFILL挖填方显示SN
		ESNT_HD_CUTFILL = MAKE_IRR_ID('h','c','t','l'),

		//! DOM
		ESNT_HD_DOM		= MAKE_IRR_ID('h','d','o','m'),

		// ! 浮动线
		ESNT_FLOAT_LINE = MAKE_IRR_ID('f','l','i','n'),

		//! 切面
		ESNT_SLICE_PLANE = MAKE_IRR_ID('h','s','c','p'),

		//! 等高线
		ESNT_CONTOUR_LINES = MAKE_IRR_ID('h','c','t','l'),

		//! 海量数据点云节点
		ESNT_HD_SEADATA_POINT = MAKE_IRR_ID('h','s','d','t'),

		//! 控制点集
		ESNT_HD_CONTROLPOINT_SET = MAKE_IRR_ID('h','c','p','s'),

		//! 草图点云
		ESNT_HD_SKETCH_PCD	= MAKE_IRR_ID('h','s','t','p'),
		//***********************************************************************************************************//
		//! Unknown scene node
		ESNT_UNKNOWN        = MAKE_IRR_ID('u','n','k','n'),

		//! Will match with any scene node when checking types
		ESNT_ANY            = MAKE_IRR_ID('a','n','y','_'),

        // 标注软件像素类线面 lhq on 2015/12/30
        ESNT_MARKER_LINEPANE = MAKE_IRR_ID('m','k','l','p'),

		//! 3DPoint Scene Node
		ESNT_3DPOINT			= MAKE_IRR_ID('d','p','o','t'),

		//! SLAM轨迹软件 轨迹姿态节点
		ESNT_HD_IP_ATTITUTE = MAKE_IRR_ID('h','I','P','A'),

		// hdPtVectorGIS 要素反投全景标注节点  chy 2016 - 7-29 
		ESNT_MARKET_POINT = MAKE_IRR_ID('m','r','p','t'),

		// 总览视图图例节点
		ESNT_OVERALLVIEW_LEGEND = MAKE_IRR_ID('o','r','l','d'),

		// 总览视图测站三角形节点
        ESNT_OVERALLVIEW_TRIANGLE = MAKE_IRR_ID('o', 'r', 't', 'g'),

        // 总揽视图邻接关系节点
        ESNT_OVERALLVIEW_ADJACENCY = MAKE_IRR_ID('o', 'r', 'a', 'c'),

		// 全景调查多段线
		ESNT_PANO_SURVEY_POLYLINE = MAKE_IRR_ID('p', 's', 'p', 'l'),

		// 全景调查标志物节点
		ESNT_PANO_SURVEY_POINT = MAKE_IRR_ID('p', 's', 'p', 't'),

		// 全景调查闭合圈节点
		ESNT_PANO_SURVEY_POLYGON = MAKE_IRR_ID('p', 's', 'p', 'g')
	};
		
} // end namespace scene
} // end namespace irr


#endif


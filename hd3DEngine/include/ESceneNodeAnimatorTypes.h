// Copyright (C) 2002-2010 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __E_SCENE_NODE_ANIMATOR_TYPES_H_INCLUDED__
#define __E_SCENE_NODE_ANIMATOR_TYPES_H_INCLUDED__

namespace irr
{
namespace scene
{

	//! An enumeration for all types of built-in scene node animators
	enum ESCENE_NODE_ANIMATOR_TYPE
	{
		//! Fly circle scene node animator
		ESNAT_FLY_CIRCLE = 0,

		//! Fly straight scene node animator
		ESNAT_FLY_STRAIGHT,

		//! Follow spline scene node animator
		ESNAT_FOLLOW_SPLINE,

		//! Rotation scene node animator
		ESNAT_ROTATION,

		//! Texture scene node animator
		ESNAT_TEXTURE,

		//! Deletion scene node animator
		ESNAT_DELETION,

		//! Collision respose scene node animator
		ESNAT_COLLISION_RESPONSE,

		//! FPS camera animator
		ESNAT_CAMERA_FPS,

		//! Maya camera animator
		ESNAT_CAMERA_MAYA,

		//! Customized animator //Added by yaoli on 2012/5/21 for customized animator
		ESNAT_CAMERA_CUS,

		//! Amount of built-in scene node animators
		ESNAT_COUNT,

		//! Unknown scene node animator
		ESNAT_UNKNOWN,

		//! This enum is never used, it only forces the compiler to compile this enumeration to 32 bit.
		ESNAT_FORCE_32_BIT = 0x7fffffff
	};

	enum ESCENE_NODE_CUSANIMATOR_STYLE
	{
		//! 摄像机模式，旋转时摄像机转动，观察对象不动;
		ESNCS_CAMERA = 0,

		//! 对象模式，旋转时观察对象转动，摄像机不动;
		ESNCS_OBJECT,

		//! 平面模式，摄像机不可旋转，通过左、右、上、下平移，来实现在图片的移动，通过在远近方向上移动摄像机，来实现缩放;
		ESNCS_PLANE,

		//! amount of built-in styles
		ESNCS_COUNT
	};
} // end namespace scene
} // end namespace irr


#endif


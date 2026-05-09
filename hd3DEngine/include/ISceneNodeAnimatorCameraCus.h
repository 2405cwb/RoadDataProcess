/*! @file
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : CSceneNodeAnimatorCameraCus.h
相关文件     : CSceneNodeAnimatorCameraCus.cpp, ISceneNodeAnimatorCameraCus.h
文件实现功能 : 定义自定义的摄像机，定义其接口
作者         : 软件部，姚立
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/05/21   1.0      姚立                新增加文件
</PRE>
*******************************************************************************/

#ifndef __I_SCENE_NODE_ANIMATOR_CAMERA_CUS_H_INCLUDED__
#define __I_SCENE_NODE_ANIMATOR_CAMERA_CUS_H_INCLUDED__

#include "ISceneNodeAnimator.h"

namespace irr
{

namespace scene
{

	//! Special scene node animator for customize-style cameras
	/** This scene node animator can be attached to a camera to make it act like a 3d
	modelling tool.
	The camera is moving relative to the target with the mouse, by pressing either
	of the three buttons.
	*/
	class ISceneNodeAnimatorCameraCus : public ISceneNodeAnimator
	{
	public:

		//! Returns the speed of movement
		virtual f32 getMoveSpeed() const = 0;

		//! Sets the speed of movement
		virtual void setMoveSpeed(f32 moveSpeed) = 0;

		//! Returns the rotation speed
		virtual f32 getRotateSpeed() const = 0;

		//! Set the rotation speed
		virtual void setRotateSpeed(f32 rotateSpeed) = 0;

		//! Returns the zoom speed
		virtual f32 getZoomSpeed() const = 0;

		//! Set the zoom speed
		virtual void setZoomSpeed(f32 zoomSpeed) = 0;

		//! Returns the style of this animator, added by yaoli 2012/05/24;
		virtual ESCENE_NODE_CUSANIMATOR_STYLE GetStyle() = 0;

		//! Set the style of this animator, added by yaoli 2012/05/24;
		virtual void setStyle(ESCENE_NODE_CUSANIMATOR_STYLE style) = 0;

		//! Returns the view angle of this animator, by yaoli 2012/05/25;
		virtual void GetViewAngle(core::vector3df& angle) = 0;
	};

} // end namespace scene
} // end namespace irr

#endif


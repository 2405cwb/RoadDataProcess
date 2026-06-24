/*! @file
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : CSceneNodeAnimatorCameraCus.h
相关文件     : CSceneNodeAnimatorCameraCus.cpp, ISceneNodeAnimatorCameraCus.h
文件实现功能 : 实现自定义的摄像机，对鼠标、键盘消息响应后，调整摄像机的方向、位置等。
作者         : 软件部，姚立
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/05/21   1.0      姚立                新增加内容
</PRE>
*******************************************************************************/


#ifndef __C_SCENE_NODE_ANIMATOR_CAMERA_CUS_H_INCLUDED__
#define __C_SCENE_NODE_ANIMATOR_CAMERA_CUS_H_INCLUDED__

#include "ISceneNodeAnimatorCameraCus.h"
#include "ICameraSceneNode.h"
#include "vector2d.h"

namespace irr
{

namespace gui
{
	class ICursorControl;
}

namespace scene
{

	//! Special scene node animator for FPS cameras
	/** This scene node animator can be attached to a camera to make it act
	like a 3d modelling tool camera
	*/
	class CSceneNodeAnimatorCameraCus : public ISceneNodeAnimatorCameraCus
	{
	public:
		//! Constructor
		CSceneNodeAnimatorCameraCus(gui::ICursorControl* cursor, f32 rotateSpeed = -1500.0f, 
			f32 zoomSpeed = 200.0f, f32 translationSpeed = 1500.0f,
			scene::ESCENE_NODE_CUSANIMATOR_STYLE style = ESNCS_CAMERA);

		//! Destructor
		virtual ~CSceneNodeAnimatorCameraCus();

		//! 摄像机模式。在此模式下，通过摄像机镜头旋转来观察对象;
		void animateNodeCameraStyle(ISceneNode *node);

		//! 对象模式。在此模式下，对象绕其中心旋转;
		void animateNodeObjectStyle(ISceneNode *node);
		//! 玛雅模式 yf add 2012.7.13
		void animateMayaStyle(ISceneNode *node);

		//! 平面模式。在此模式下，摄像机不可旋转，只能在XYZ三个方向上移动，来实现图片的平移、缩放;
		void animateNodePlaneStyle(ISceneNode *node);

		//! Animates the scene node, currently only works on cameras
		virtual void animateNode(ISceneNode* node, u32 timeMs);

		//! Event receiver
		virtual bool OnEvent(const SEvent& event);

		//! Returns the speed of movement in units per millisecond
		virtual f32 getMoveSpeed() const;

		//! Sets the speed of movement in units per millisecond
		virtual void setMoveSpeed(f32 moveSpeed);

		//! Returns the rotation speed
		virtual f32 getRotateSpeed() const;

		//! Set the rotation speed
		virtual void setRotateSpeed(f32 rotateSpeed);

		//! Returns the zoom speed
		virtual f32 getZoomSpeed() const;

		//! Set the zoom speed
		virtual void setZoomSpeed(f32 zoomSpeed);

		//! This animator will receive events when attached to the active camera
		virtual bool isEventReceiverEnabled() const
		{
			return true;
		}

		//! Returns type of the scene node
		virtual ESCENE_NODE_ANIMATOR_TYPE getType() const 
		{
			return ESNAT_CAMERA_CUS;
		}

		//! Returns the style of this animator, added by yaoli 2012/05/24;
		virtual ESCENE_NODE_CUSANIMATOR_STYLE GetStyle();

		//! Set the style of this animator, added by yaoli 2012/05/24;
		virtual void setStyle(ESCENE_NODE_CUSANIMATOR_STYLE style);

		//! Creates a clone of this animator.
		/** Please note that you will have to drop
		(IReferenceCounted::drop()) the returned pointer after calling
		this. */
		virtual ISceneNodeAnimator* createClone(ISceneNode* node, ISceneManager* newManager=0);

		//! Returns the view angle of this animator, by yaoli 2012/05/25;
		virtual void GetViewAngle(core::vector3df& angle);

	private:

		void allKeysUp();
		void animate();
		bool isMouseKeyDown(s32 key) const;

		bool MouseKeys[4];//添加鼠标中键滚动事件

		gui::ICursorControl *CursorControl;
		core::position2df RotateStart;
		core::position2df TranslateStart;
		core::position2df MousePos;
		f32 ZoomSpeed;
		f32 RotateSpeed;
		f32 TranslateSpeed;
		f32 CurrentZoom;
		f32 RotV, RotH, RotR;		//记录当前垂直、水平方向的角度值;
		bool Rotating;
		bool Moving;
		bool Translating;
		ESCENE_NODE_CUSANIMATOR_STYLE Style;	//记录当前的运动样式，分为对象模式和摄像机模式;
		core::position2df Center;	//当运动模式为对象模式时，摄像机绕此中心转动，得到的渲染效果就是对象绕此中心旋转;

		//--maya mode yf add 2012.7.13
		scene::ICameraSceneNode* OldCamera;
		core::vector3df OldTarget;
		core::vector3df LastCameraTarget;	// to find out if the camera target was moved outside this animator
		f32 RotX, RotY;
		bool Zooming;
		core::position2df ZoomStart;
		f32 MouseWheel;
		//-------------------
	};

} // end namespace scene
} // end namespace irr

#endif


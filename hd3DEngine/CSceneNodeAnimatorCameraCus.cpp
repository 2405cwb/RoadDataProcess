/*! @file
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : CSceneNodeAnimatorCameraCus.cpp
相关文件     : CSceneNodeAnimatorCameraCus.h, ISceneNodeAnimatorCameraCus.h
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

#include "CSceneNodeAnimatorCameraCus.h"
#include "ICursorControl.h"
#include "ICameraSceneNode.h"
#include "SViewFrustum.h"
#include "ISceneManager.h"

namespace irr
{
namespace scene
{

//! constructor
CSceneNodeAnimatorCameraCus::CSceneNodeAnimatorCameraCus(gui::ICursorControl* cursor, f32 rotate, f32 zoom, f32 translate,
	scene::ESCENE_NODE_CUSANIMATOR_STYLE style)
	: CursorControl(cursor), OldCamera(0), MousePos(0.5f, 0.5f), MouseWheel(0.0),
	ZoomSpeed(zoom), RotateSpeed(rotate), TranslateSpeed(translate),
	CurrentZoom(70.0f), RotV(0.0f),RotH(0.0),RotR(0.0), RotX(0.0f), RotY(0.0f),
	Rotating(false), Moving(false), Translating(false),Zooming(false),
	Style(style),Center(0.0f,0.0f)
{
	#ifdef _DEBUG
	setDebugName("CSceneNodeAnimatorCameraCus");
	#endif

	if (CursorControl)
	{
		CursorControl->grab();
		MousePos = CursorControl->getRelativePosition();
	}

	allKeysUp();
}


//! destructor
CSceneNodeAnimatorCameraCus::~CSceneNodeAnimatorCameraCus()
{
	if (CursorControl)
		CursorControl->drop();
}

//! Returns the style of this animator, added by yaoli 2012/05/24;
ESCENE_NODE_CUSANIMATOR_STYLE CSceneNodeAnimatorCameraCus::GetStyle()
{
	return Style;
}

//! Set the style of this animator, added by yaoli 2012/05/24;
void CSceneNodeAnimatorCameraCus::setStyle(ESCENE_NODE_CUSANIMATOR_STYLE style)
{
	Style = style;
}


bool CSceneNodeAnimatorCameraCus::OnEvent(const SEvent& event)
{
	if (event.EventType != EET_MOUSE_INPUT_EVENT)
		return false;

	switch(event.MouseInput.Event)
	{
	case EMIE_LMOUSE_PRESSED_DOWN:
		MouseKeys[0] = true;
		MouseKeys[3] = false;
		break;
	case EMIE_RMOUSE_PRESSED_DOWN:
		MouseKeys[2] = true;
		MouseKeys[3] = false;
		break;
	case EMIE_MMOUSE_PRESSED_DOWN:
		MouseKeys[1] = true;
		MouseKeys[3] = false;
		break;
	case EMIE_LMOUSE_LEFT_UP:
		MouseKeys[0] = false;
		MouseKeys[3] = false;
		break;
	case EMIE_RMOUSE_LEFT_UP:
		MouseKeys[2] = false;
		MouseKeys[3] = false;
		break;
	case EMIE_MMOUSE_LEFT_UP:
		MouseKeys[1] = false;
		MouseKeys[3] = false;
		break;
	case EMIE_MOUSE_MOVED:
		MousePos = CursorControl->getRelativePosition();
		break;
	case EMIE_MOUSE_WHEEL:
		CurrentZoom += event.MouseInput.Wheel*ZoomSpeed/1000.0f;
		break;
		//MouseKeys[3] = true;
		//MouseWheel = event.MouseInput.Wheel / 100.0f;
		//break;
	case EMIE_LMOUSE_DOUBLE_CLICK:
	case EMIE_RMOUSE_DOUBLE_CLICK:
	case EMIE_MMOUSE_DOUBLE_CLICK:
	case EMIE_LMOUSE_TRIPLE_CLICK:
	case EMIE_RMOUSE_TRIPLE_CLICK:
	case EMIE_MMOUSE_TRIPLE_CLICK:
	case EMIE_COUNT:
		return false;
	}
	return true;
}

void CSceneNodeAnimatorCameraCus::animateNodeCameraStyle(ISceneNode *node)
{
	if (!node || node->getType() != ESNT_CAMERA)
		return;

	ICameraSceneNode* camera = static_cast<ICameraSceneNode*>(node);

	// If the camera isn't the active camera, and receiving input, then don't process it.
	if (!camera->isInputReceiverEnabled())
		return;

	scene::ISceneManager * smgr = camera->getSceneManager();
	if (smgr && smgr->getActiveCamera() != camera)
		return;

	//设置摄像机当前状态，如果正在移动、旋转、缩放，则设置为真值;
	smgr->SetAnimateState(Translating || Rotating);

	core::vector3df pos = camera->getPosition();
	core::vector3df upVector(camera->getUpVector());
	core::vector3df target = camera->getTarget();
	core::vector3df screenFar = target - pos;
	f32 length = screenFar.getLength();
	screenFar.normalize();

	//更新水平、垂直转角，因为摄像机的初始转角可能不为零;
	RotV = asin(screenFar.Z);
	RotH = asin(screenFar.X/cos(RotV));
	RotV *= core::RADTODEG;
	RotH *= core::RADTODEG;
	if (screenFar.Y >= 0.0f)
	{
		if (RotH < 0)
		{
			RotH += 360.0f;
		}
	}
	else if (screenFar.Y < 0)
	{
		if (RotH > 0)
		{
			RotH = 180.0f - RotH;
		}
		else
		{
			RotH = 180.0f - RotH;
		}
	}

	//Translate
	if (false/*isMouseKeyDown(1)*/)//鼠标中键按下后，开始平移操作;---摄像机模式下，不允许平移操作;
	{
		if (!Translating)
		{
			TranslateStart = MousePos;
			Translating = true;
		}
		else
		{
			//得到当前屏幕中心向右的矢量screenRight;
			core::vector3df screenRight = screenFar.crossProduct(upVector);
			screenRight.normalize();
			//得到当前屏幕中心向上的矢量screenTop;
			core::vector3df screenTop = screenRight.crossProduct(screenFar);
			screenTop.normalize();

			//平移时，始终沿screenRight和screenTop方向平移;
			pos +=  screenRight * (MousePos.X - TranslateStart.X)*TranslateSpeed +
				screenTop * (MousePos.Y - TranslateStart.Y)*TranslateSpeed;
			target = screenFar*length + pos;
			camera->setTarget(target);
			camera->setPosition(pos);
			TranslateStart = MousePos;
		}

		CurrentZoom = 0;//将平移过程中累积的zoom量归零;
		return;//平移、旋转和缩放，同一时间只能有一种操作;
	}
	else if (Translating)
	{
		Translating = false;
		CurrentZoom = 0;//将平移过程中累积的zoom量归零;
		return;//平移、旋转和缩放，同一时间只能有一种操作;
	}

	// Rotation ------------------------------------
	if (isMouseKeyDown(0))//鼠标左键按下后，开始旋转操作;
	{
		if (!Rotating)
		{
			RotateStart = MousePos;
			Rotating = true;
		}
		else
		{
			f32 nRotV = 0.0f;
			f32 nRotH = 0.0f;

			nRotH = (RotateStart.X - MousePos.X) * RotateSpeed;
			nRotV = (RotateStart.Y - MousePos.Y) * RotateSpeed;

			RotH += nRotH;//记录当前水平方向的角度值;
			if (RotH < 0)
			{
				RotH += 360.0f;
			}
			else if (RotH > 360.0f)
			{
				RotH -= 360.0f;
			}

			//限制垂直方向的转动，向上向下均不能超过90度;
			RotV += nRotV;//记录当前垂直方向的角度值;
			if (RotV > 85.0f)
			{
				RotV = 85.0f;
			}
			else if (RotV < -85.0f)
			{
				RotV = -85.0f;
			}

			//Rotate
			f32 nRadianV = RotV*core::DEGTORAD;
			f32 nRadianH = RotH*core::DEGTORAD;
			target.Z = pos.Z + sin(nRadianV)*length;
			target.X = pos.X + cos(nRadianV)*length*sin(nRadianH);
			target.Y = pos.Y + cos(nRadianV)*length*cos(nRadianH);
			camera->setTarget(target);

			RotateStart = MousePos;
		}

		CurrentZoom = 0;//将旋转过程中累积的zoom量归零;
		return;//平移、旋转和缩放，同一时间只能有一种操作;
	}
	else if (Rotating)
	{
		Rotating = false;
		CurrentZoom = 0;//将旋转过程中累积的zoom量归零;
		return;//平移、旋转和缩放，同一时间只能有一种操作;
	}

	//Zoom
	if (CurrentZoom != 0)
	{
		//不改变相机的位置，而是通过修改平截台体的参数（变换摄像机镜头，从长焦到广角）,来实现渲染对象的缩放;
		f32 fov = camera->getFOV();
		f32 curFov = fov;
		curFov -= CurrentZoom;
		if (curFov < core::PI/36.0f)
		{
			curFov = core::PI/36.0f;
		}
		else if (curFov > core::PI*5/6.0f)
		{
			curFov = core::PI*5/6.0f;
		}

		//当放大、缩小后，更改旋转的速度
		RotateSpeed += -1000.0f * (curFov - fov)/core::PI;
		camera->setFOV(curFov);
		CurrentZoom = 0;
	}
}

void CSceneNodeAnimatorCameraCus::animateNodeObjectStyle(ISceneNode *node)
{
	if (!node || node->getType() != ESNT_CAMERA)
		return;

	ICameraSceneNode* camera = static_cast<ICameraSceneNode*>(node);

	// If the camera isn't the active camera, and receiving input, then don't process it.
	if (!camera->isInputReceiverEnabled())
		return;

	scene::ISceneManager * smgr = camera->getSceneManager();
	if (smgr && smgr->getActiveCamera() != camera)
		return;

	//设置摄像机当前状态，如果正在移动、旋转、缩放，则设置为真值;
	smgr->SetAnimateState(Translating || Rotating);

	core::vector3df pos = camera->getPosition();
	core::vector3df upVector(camera->getUpVector());
	core::vector3df target = camera->getTarget();
	core::vector3df screenFar = target - pos;
	f32 length = screenFar.getLength();
	screenFar.normalize();

	//更新水平、垂直转角，因为摄像机的初始转角可能不为零;
	RotV = asin(screenFar.Z);
	RotH = asin(screenFar.X/cos(RotV));
	RotV *= core::RADTODEG;
	RotH *= core::RADTODEG;
	if (screenFar.Y >= 0.0f)
	{
		if (RotH < 0)
		{
			RotH += 360.0f;
		}
	}
	else if (screenFar.Y < 0)
	{
		if (RotH > 0)
		{
			RotH = 180.0f - RotH;
		}
		else
		{
			RotH = 180.0f - RotH;
		}
	}
/*
	f32 upLength = upVector.getLength();
	RotR = atan(upVector.X/upVector.Y);
	RotR *= core::RADTODEG;
	if (upVector.Y >= 0.0f)
	{
		if (RotR < 0)
		{
			RotR += 360.0f;
		}
	}
	else if (upVector.Y < 0)
	{
		if (RotR > 0)
		{
			RotR = 180.0f - RotR;
		}
		else
		{
			RotR = 180.0f - RotR;
		}
	}*/

	//Translate
	if (isMouseKeyDown(1))//鼠标中键按下后，开始平移操作;
	{
		if (!Translating)
		{
			TranslateStart = MousePos;
			Translating = true;
		}
		else
		{
			//得到当前屏幕中心向右的矢量screenRight;
			core::vector3df screenRight = screenFar.crossProduct(upVector);
			screenRight.normalize();
			//得到当前屏幕中心向上的矢量screenTop;
			core::vector3df screenTop = screenRight.crossProduct(screenFar);
			screenTop.normalize();

			//平移时，始终沿screenRight和screenTop方向平移;
			pos +=  screenRight * (MousePos.X - TranslateStart.X)*TranslateSpeed +
				screenTop * (MousePos.Y - TranslateStart.Y)*TranslateSpeed;
			target = screenFar*length + pos;
			camera->setTarget(target);
			camera->setPosition(pos);
			TranslateStart = MousePos;
		}

		CurrentZoom = 0;//将平移过程中累积的zoom量归零;
		return;//平移、旋转和缩放，同一时间只能有一种操作;
	}
	else if (Translating)
	{
		Translating = false;
		CurrentZoom = 0;//将平移过程中累积的zoom量归零;
		return;//平移、旋转和缩放，同一时间只能有一种操作;
	}

	// Rotation ------------------------------------
	if (isMouseKeyDown(0))//鼠标左键按下后，开始旋转操作;
	{
		if (!Rotating)
		{
			RotateStart = MousePos;
			Rotating = true;
		}
		else
		{
			f32 nRotV = 0.0f;
			f32 nRotH = 0.0f;
			nRotH = (RotateStart.X - MousePos.X) * RotateSpeed;
			nRotV = (RotateStart.Y - MousePos.Y) * RotateSpeed;

			//得到当前屏幕中心向右的矢量screenRight;
			core::vector3df screenRight = screenFar.crossProduct(upVector);
			screenRight.normalize();
			//得到当前屏幕中心向上的矢量screenTop;
			core::vector3df screenTop = screenRight.crossProduct(screenFar);
			screenTop.normalize();

			//将相机位置与旋转中心(target）连线的矢量绕矢量screenTop旋转-nRotH度;
			core::vector3df tmp = target - pos;
			tmp.rotateByVector(-nRotH,screenTop);

			//上一步旋转后，屏幕screenRight的值发生了变化，重新计算临时的screenRight;
			core::vector3df tmpR = tmp.crossProduct(upVector);

			//将相机位置与旋转中心(target）连线的矢量绕当前的screenRight旋转nRotV度;
			tmp.rotateByVector(nRotV,tmpR);

			//旋转后，upVector的值发生了变化，重新计算upVector的值;
			upVector = tmpR.crossProduct(tmp);
			upVector.normalize();

			//得到相机位置，设置相机位置以及upVector;
			pos = target - tmp;
			camera->setPosition(pos);
			camera->setUpVector(upVector);

			RotateStart = MousePos;
		}

		CurrentZoom = 0;//将平移过程中累积的zoom量归零;
		return;//平移、旋转和缩放，同一时间只能有一种操作;
	}
	else if (Rotating)
	{
		Rotating = false;
		CurrentZoom = 0;//将旋转过程中累积的zoom量归零;
		return;//平移、旋转和缩放，同一时间只能有一种操作;
	}
	
	//Zoom
	if (CurrentZoom != 0)
	{
		pos += screenFar*CurrentZoom;
		camera->setPosition(pos);
		CurrentZoom = 0;
	}
}

void CSceneNodeAnimatorCameraCus::animateMayaStyle(ISceneNode *node)
{
	//Alt + LM = Rotate around camera pivot
	//Alt + LM + MM = Dolly forth/back in view direction (speed % distance camera pivot - max distance to pivot)
	//Alt + MM = Move on camera plane (Screen center is about the mouse pointer, depending on move speed)

	if (!node || node->getType() != ESNT_CAMERA)
		return;

	ICameraSceneNode* camera = static_cast<ICameraSceneNode*>(node);

	// If the camera isn't the active camera, and receiving input, then don't process it.
	if (!camera->isInputReceiverEnabled())
		return;

	scene::ISceneManager * smgr = camera->getSceneManager();
	if (smgr && smgr->getActiveCamera() != camera)
		return;

	//设置摄像机当前状态，如果正在移动、旋转、缩放，则设置为真值 2012/7/11
	smgr->SetAnimateState(Translating || Rotating || Zooming);

	if (OldCamera != camera)
	{
		LastCameraTarget = OldTarget = camera->getTarget();
		OldCamera = camera;
	}
	else
	{
		OldTarget += camera->getTarget() - LastCameraTarget;
	}

	f32 nRotX = RotX;
	f32 nRotY = RotY;
	f32 nZoom = CurrentZoom;

	//if ( (isMouseKeyDown(0) && isMouseKeyDown(2)) || isMouseKeyDown(1) )
	//鼠标中键滚动缩放
	if ( isMouseKeyDown(3) )
	{
		if (!Zooming)
		{
			ZoomStart = MousePos;
			Zooming = true;
		}
		else
		{
			const f32 targetMinDistance = 0.1f;
			//nZoom += (ZoomStart.X - MousePos.X) * ZoomSpeed;
			nZoom += MouseWheel * ZoomSpeed;

			if (nZoom < targetMinDistance) // jox: fixed bug: bounce back when zooming to close
				nZoom = targetMinDistance;
		}
	}
	else if (Zooming)
	{
		const f32 old = CurrentZoom;
		//CurrentZoom = CurrentZoom + (ZoomStart.X - MousePos.X ) * ZoomSpeed;
		CurrentZoom = CurrentZoom + MouseWheel * ZoomSpeed;
		nZoom = CurrentZoom;

		if (nZoom < 0)
			nZoom = CurrentZoom = old;
		Zooming = false;
	}

	// Translation ---------------------------------

	core::vector3df translate(OldTarget);
	const core::vector3df upVector(camera->getUpVector());
	const core::vector3df target = camera->getTarget();

	core::vector3df pos = camera->getPosition();
	core::vector3df tvectX = pos - target;
	tvectX = tvectX.crossProduct(upVector);
	tvectX.normalize();

	const SViewFrustum* const va = camera->getViewFrustum();
	core::vector3df tvectY = (va->getFarLeftDown() - va->getFarRightDown());
	//tvectY = tvectY.crossProduct(upVector.Y > 0 ? pos - target : target - pos);
	//改变y方向的平移方向
	//tvectY = tvectY.crossProduct(upVector.Y > 0 ? target - pos : pos - target);
	tvectY = tvectY.crossProduct(target - pos);

	tvectY.normalize();
	//鼠标中键平移
	if (isMouseKeyDown(1) && !Zooming)
	{
		if (!Translating)
		{
			TranslateStart = MousePos;
			Translating = true;
		}
		else
		{
			translate +=  tvectX * (TranslateStart.X - MousePos.X)*TranslateSpeed +
			              tvectY * (MousePos.Y - TranslateStart.Y)*TranslateSpeed;
		}
	}
	else if (Translating)
	{
		translate += tvectX * (TranslateStart.X - MousePos.X)*TranslateSpeed +
		             tvectY * (MousePos.Y - TranslateStart.Y)*TranslateSpeed;
		OldTarget = translate;
		Translating = false;
	}

	// Rotation ------------------------------------
	//鼠标左键旋转
	if (isMouseKeyDown(0) && !Zooming)
	{
		if (!Rotating)
		{
			RotateStart = MousePos;
			Rotating = true;
			nRotX = RotX;
			nRotY = RotY;
		}
		else
		{
			nRotX += (RotateStart.X - MousePos.X) * RotateSpeed;
			nRotY += (RotateStart.Y - MousePos.Y) * RotateSpeed;
		}
	}
	else if (Rotating)
	{
		RotX += (RotateStart.X - MousePos.X) * RotateSpeed;
		RotY += (RotateStart.Y - MousePos.Y) * RotateSpeed;
		nRotX = RotX;
		nRotY = RotY;
		Rotating = false;
	}

	// Set pos ------------------------------------	
	pos = translate;
	pos.X += nZoom;

	//pos.rotateXYBy(nRotY, translate);
	//pos.rotateXZBy(-nRotX, translate);
	pos.rotateXZBy(-nRotY, translate);
	pos.rotateXYBy(nRotX, translate);

	camera->setPosition(pos);
	camera->setTarget(translate);

	// Rotation Error ----------------------------

	// jox: fixed bug: jitter when rotating to the top and bottom of y
	pos.set(0,0,1);//orign is pos.set(0,1,0) yf 2012.7.12
	/*pos.rotateXYBy(-nRotY);
	pos.rotateXZBy(-nRotX+180.f);*/
	pos.rotateXZBy(nRotY);
	pos.rotateXYBy(nRotX-180.f);
	camera->setUpVector(pos);
	LastCameraTarget = camera->getTarget();
}

void CSceneNodeAnimatorCameraCus::animateNodePlaneStyle(ISceneNode *node)
{
	if (!node || node->getType() != ESNT_CAMERA)
		return;

	ICameraSceneNode* camera = static_cast<ICameraSceneNode*>(node);

	// If the camera isn't the active camera, and receiving input, then don't process it.
	if (!camera->isInputReceiverEnabled())
		return;

	scene::ISceneManager * smgr = camera->getSceneManager();
	if (smgr && smgr->getActiveCamera() != camera)
		return;

	core::vector3df pos = camera->getPosition();
	core::vector3df target = camera->getTarget();
	core::vector3df screenFar = target - pos;
	f32 length = screenFar.getLength();
	screenFar.normalize();

	//Translating
	if (isMouseKeyDown(0))
	{
		if (!Translating)
		{
			TranslateStart = MousePos;
			Translating = true;
		}
		else
		{
			//当前屏幕中心向右的矢量screenRight;
			core::vector3df screenRight(-1.0f,0.0f,0.0f);
			//当前屏幕中心向上的矢量screenTop;
			core::vector3df screenTop(0.0f,0.0f,1.0f);

			//平移时，始终沿screenRight和screenTop方向平移;
			pos +=  screenRight * (TranslateStart.X - MousePos.X)*TranslateSpeed +
				screenTop * (MousePos.Y - TranslateStart.Y)*TranslateSpeed;
			target = screenFar*length + pos;
			camera->setTarget(target);
			camera->setPosition(pos);
			TranslateStart = MousePos;
		}

		CurrentZoom = 0;//将平移过程中累积的zoom量归零;
		return;//平移、旋转和缩放，同一时间只能有一种操作;
	}
	else if (Translating)
	{
		Translating = false;
		CurrentZoom = 0;//将平移过程中累积的zoom量归零;
		return;//平移、旋转和缩放，同一时间只能有一种操作;
	}

	//Zoom
	if (CurrentZoom != 0)
	{
		//不改变相机的位置，而是通过修改平截台体的参数（变换摄像机镜头，从长焦到广角）,来实现渲染对象的缩放;
		f32 fov = camera->getFOV();
		f32 curFov = fov;
		curFov -= CurrentZoom;
		if (curFov < core::PI/36.0f)
		{
			curFov = core::PI/36.0f;
		}
		else if (curFov > core::PI*5/6.0f)
		{
			curFov = core::PI*5/6.0f;
		}

		//当放大、缩小后，更改旋转的速度
		TranslateSpeed += 100.0f * (curFov - fov)/core::PI;
		camera->setFOV(curFov);
		CurrentZoom = 0;
	}
}

//! OnAnimate() is called just before rendering the whole scene.
void CSceneNodeAnimatorCameraCus::animateNode(ISceneNode *node, u32 timeMs)
{
	switch (Style)
	{
	case ESNCS_CAMERA:
		animateNodeCameraStyle(node);
		break;
	case ESNCS_OBJECT:
		//animateNodeObjectStyle(node);
		animateMayaStyle(node);
		break;
	case  ESNCS_PLANE:
		animateNodePlaneStyle(node);
		break;
	default:
		break;
	}
}


bool CSceneNodeAnimatorCameraCus::isMouseKeyDown(s32 key) const
{
	return MouseKeys[key];
}


void CSceneNodeAnimatorCameraCus::allKeysUp()
{
	for (s32 i=0; i<4; ++i)
		MouseKeys[i] = false;
}


//! Sets the rotation speed
void CSceneNodeAnimatorCameraCus::setRotateSpeed(f32 speed)
{
	RotateSpeed = speed;
}


//! Sets the movement speed
void CSceneNodeAnimatorCameraCus::setMoveSpeed(f32 speed)
{
	TranslateSpeed = speed;
}


//! Sets the zoom speed
void CSceneNodeAnimatorCameraCus::setZoomSpeed(f32 speed)
{
	ZoomSpeed = speed;
}


//! Gets the rotation speed
f32 CSceneNodeAnimatorCameraCus::getRotateSpeed() const
{
	return RotateSpeed;
}


// Gets the movement speed
f32 CSceneNodeAnimatorCameraCus::getMoveSpeed() const
{
	return TranslateSpeed;
}


//! Gets the zoom speed
f32 CSceneNodeAnimatorCameraCus::getZoomSpeed() const
{
	return ZoomSpeed;
}


ISceneNodeAnimator* CSceneNodeAnimatorCameraCus::createClone(ISceneNode* node, ISceneManager* newManager)
{
	CSceneNodeAnimatorCameraCus * newAnimator =
		new CSceneNodeAnimatorCameraCus(CursorControl, RotateSpeed, ZoomSpeed, TranslateSpeed);
	return newAnimator;
}

void CSceneNodeAnimatorCameraCus::GetViewAngle(core::vector3df& angle)
{
	angle.X = RotH;
	angle.Y = RotV;
	angle.Z = RotR;
}

} // end namespace
} // end namespace


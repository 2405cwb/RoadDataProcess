/*! hdCamera.h
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : hdCamera.h
相关文件     : 
文件实现功能 : 摄像机工具接口定义 
作者         : 杨峰
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/07/14   1.0      杨峰    
</PRE>
*******************************************************************************/

#pragma once
#include "stdafx.h"
#include "..\hdFramework\hdTool.h"
#include "..\hd3DEngine\include\irrlicht.h"

using namespace irr;
using namespace hd;
using namespace hd::fm;

namespace hd
{
	namespace fm
	{
		class HD3DSCENE_API CHdCamera :
			public CHdTool
		{
		protected:
			//! 缩放速度
			f32 m_ZoomSpeed;
			//! 旋转速度
			f32 m_RotateSpeed;
			//! 移动速度
			f32 m_TranslateSpeed;
		public:
			CHdCamera(void):m_ZoomSpeed(1200.0f),m_RotateSpeed(1500.0f),m_TranslateSpeed(1500.0f){};
			CHdCamera(const char* caption,
				const char* msg,
				const char* name,
				const char* tooltip,
				int type)
				:CHdTool(caption,msg,name,tooltip,type),m_ZoomSpeed(1200.0f),m_RotateSpeed(1500.0f),m_TranslateSpeed(1500.0f){}
			virtual ~CHdCamera(void){}


			//! 得到移动速度
			virtual f32 GetMoveSpeed() const { return m_TranslateSpeed; }
			//! 设置移动速度
			virtual void SetMoveSpeed(f32 moveSpeed) { m_TranslateSpeed = moveSpeed; }
			//! 得到旋转速度
			virtual f32 GetRotateSpeed() const { return m_RotateSpeed; }
			//! 设置旋转速度
			virtual void SetRotateSpeed(f32 rotateSpeed) { m_RotateSpeed = rotateSpeed; }
			//! 得到缩放速度
			virtual f32 GetZoomSpeed() { return m_ZoomSpeed; }
			//! 设置缩放速度
			virtual void SetZoomSpeed(f32 zoomSpeed) { m_ZoomSpeed = zoomSpeed; }
			//! 得到相机角度
			virtual void GetViewAngle(core::vector3df& angle){}
			//! 移动相机
			virtual void MoveCamera(core::vector3df newPos){}
			//! 移动相机,通过像素比例设置新目标
			void MoveCamera(core::vector2df newPos){}
		};
	}
}



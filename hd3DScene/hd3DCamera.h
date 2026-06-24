#pragma once

#include "stdafx.h"
#include "hdCamera.h"
#include "CMeshPolySceneNode.h"
#include "CCollisionRaySceneNode.h"

using namespace irr;
using namespace hd::scene;
using namespace hd::fm;

namespace hd
{
	namespace fm
	{


		// 键盘控制平移枚举类型 [2013/8/12 蔡红云] 
		enum ENUM_KEYCTROL_TYPE
		{
			E_KCTL_LEFT 			= 0,	// 左移
			E_KCTL_RIGHT			= 1,	// 右移
			E_KCTL_UP			    = 2,	// 上移
			E_KCTL_DOWN			    = 3		// 下移

		};
		// !线程函数
		DWORD WINAPI ZoomOnTimer(void* Param);

		// 设置scenenode可被碰撞或者高亮属性
		enum
		{
			ID_IsNotPickable = 0,

			IDFlag_IsPickable = 1 << 0,

			IDFlag_IsHighlightable = 1 << 1
		};

		class HD3DSCENE_API CHd3DCamera :
			public CHdCamera
		{
		protected:

			ENUM_KEYCTROL_TYPE keyType;
			core::position2df m_TranslateStart;		//移动起点
			bool m_bRotating;
			bool m_bTranslating;
			bool m_bOperateBox;
			core::position2di m_MousePos;			//鼠标位置
			f32 m_RotX, m_RotY;						//旋转角度
			core::position2di m_lastMPos;			//上次鼠标位置
			core::vector3df m_rotCenter;
			CMeshPolySceneNode* m_meshSN;			//探面结果
			CCollisionRaySceneNode* m_CollisonSN;	//碰撞检测射线
			bool m_bAnimator;						//是否启用双击过度动画
			bool m_bScenePan;						// 是否为HDScene浏览模式即按住鼠标右键进行平移 [危迟 2014/08/16]
		public:

			CHd3DCamera(void);

			virtual ~CHd3DCamera(void);

			// 单击
			virtual void OnClick();

			// 创建 
			virtual void OnCreate( CHdApp* app );

			// 是否可用
			virtual bool GetEnable();

			// 鼠标按下
			virtual void OnMouseDown( int Button, int Shift, int X, int Y );

			// 鼠标弹起
			virtual void OnMouseUp( int Button, int Shift, int X, int Y );

			// 双击
			virtual void OnDblClick( int Button, int Shift, int X, int Y );

			// 鼠标移动
			virtual void OnMouseMove( int Button, int Shift, int X, int Y );

			// 鼠标中键
			virtual void OnMouseWheel( UINT nFlags, short zDelta, int X, int Y );

			// 非活动
			virtual void Deactivate();

			// 键盘按下
			virtual void OnKeyDown( int keyCode, int Shift );

			// 键盘弹起
			virtual void OnKeyUp( int keyCode, int Shift );

			virtual bool OnContextMenu( int X, int Y );

			//! 设置为右键平移模式 HDScene新的三维浏览方式  [危迟 2014/08/16]
			void SetHDScenePanMode(bool bHDScene) { m_bScenePan = bHDScene; }

			//! 设置x旋转角度
			void SetViewAngleX(f32 rotX);

			//! 设置y旋转角度
			void SetViewAngleY(f32 rotY);

			// 蔡红云 键盘控制平移 key_Type:键盘按下类型 step:每次平移步长
			void MoveKeyCtrol(ENUM_KEYCTROL_TYPE key_Type, int step);

			//! 得到相机角度
			virtual void GetViewAngle(core::vector3df& angle);

			//! 设置相机的平移
			virtual void MoveCamera(core::vector3df newPos);

			//! 设置相机的平移-重载
			void MoveCamera(core::vector3df newPos,core::vector3df newTar);

			//! 获取旋转状态
			bool GetBRotating() { return m_bRotating; }

			//! 获取平移状态
			bool GetBTranslating() { return m_bTranslating; }

			//! 根据当前视图,重新设置相机目标点,从屏幕srcX,srcY位置检测点云
			bool SetTargetByView(int srcX,int srcY);

			//! 获取旋转中心点
			const core::vector3df& GetRotateCenter()const {return m_rotCenter;}

			//! 设置旋转中心点 [2014/6/25]
			void SetRotateCenter(core::vector3df rotCenter)
			{
				m_rotCenter = rotCenter;

			}

			//! 旋转相机 [2013/8/23 蔡红云] 
			void RotateCamera(core::vector2di sdvig);

			//  设置操作球状态 [2013/9/12 蔡红云]
			void Set3DboxOpState(bool flag ) { m_bOperateBox = flag;}

			// !设置相机观察视角类型 [2013/9/30 蔡红云]  
			void SetCameraPosType(ENUM_CAMERA_POSITION cmppostype){ m_ecmr_pos_type = cmppostype; }

			// !获取相机观察视角类型 [2013/9/30 蔡红云] 
			ENUM_CAMERA_POSITION  GetCameraPosType(){  return m_ecmr_pos_type;}

			//! 设置是否响应按键浏览 [2013/11/20 危迟]
			void SetPanViewByKey(bool bEnable) { m_bPanViewByKey = bEnable; }

			//! 设置是否启用动画过度
			void SetAnimator(bool bAnimator){m_bAnimator = bAnimator;}

		public:
			bool m_bMouseDown;

		private:	

			// 移动场景结点
			void MoveSceneNode(ISceneNode* node,bool bBack = false);

			// 位置改变计数
			int m_nPosChangeCount;

			// 场景结点的原始位置
			core::vector3df m_oriPos;

			CHdSxPolyline3D*	m_pPolyline;
			
			// !旋转相机时，鼠标移动速度 [2013/8/23 蔡红云]
			float m_fMouseRotateMult;
			
			// !旋转相机时鼠标位置，从按下左键开始记录 [2013/8/23 蔡红云] 
			core::vector2di m_btnlftdwn;
			
			// ! 相机观察视角类型 [2013/9/30 蔡红云]
			ENUM_CAMERA_POSITION m_ecmr_pos_type;

			//! 是否响应ASWD按键浏览 [2013/11/20 危迟]
			bool m_bPanViewByKey;

			int m_nCount;

		};

	}
}


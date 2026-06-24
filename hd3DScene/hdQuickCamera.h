#pragma once
#include "hdCamera.h"
#include "CMeshPolySceneNode.h"

using namespace irr;

namespace hd
{
	namespace fm
	{
		class HD3DSCENE_API CHdQuickCamera :
			public CHdCamera
		{


		private:

			core::position2df m_RotateStart;
			core::position2df m_MousePos;

			//记录当前垂直、水平方向的角度值;
			f32 m_RotV, m_RotH, m_RotR;	

			//开始旋转操作
			bool m_bRotating;

			//判断旋转角是否被计算过,如果没有计算
			bool m_bViewAngleInit;       

			//鼠标探面结果对象
			CMeshPolySceneNode* m_meshSN;

			// 缩放时fov角最小值--chy -- 2016-4-14
			float m_fMinFov;
			
			// 缩放时fov角最大值
			float m_fMaxFov;
			 
			// 鼠标追踪 chy --chy -- 2016-4-14
			BOOL m_bMouseTrack;

			// 浏览时垂直角最小值 -chy -- 2016-7-23
			float m_fMinVerAngle;

			// 浏览时垂直角最大值
			float m_fMaxVerAngle;
			

		public:
			CHdQuickCamera(void);
			CHdQuickCamera(const char* caption,const char* msg,	const char* name,const char* tooltip,int type);
			virtual ~CHdQuickCamera(void);

			core::position2df GetRotateStartPos(){ return m_RotateStart;}
			core::position2df GetMousePos(){ return m_MousePos;}

			virtual void OnClick();

			virtual void OnCreate( CHdApp* app );

			virtual bool GetEnable();

			virtual void OnMouseDown( int Button, int Shift, int X, int Y );

			virtual void OnMouseUp( int Button, int Shift, int X, int Y );

			virtual void Deactivate();

			virtual void OnKeyDown( int keyCode, int Shift );

			virtual void OnKeyUp( int keyCode, int Shift );

			virtual bool OnContextMenu( int X, int Y );

			virtual void OnDblClick( int Button, int Shift, int X, int Y );

			virtual void OnMouseMove( int Button, int Shift, int X, int Y );

			virtual void OnMouseWheel( UINT nFlags, short zDelta, int X, int Y );

			virtual void OnMouseLeave(int Button, int Shift, int X, int Y);

			//! 得到相机角度
			virtual void GetViewAngle(core::vector3df& angle);

			//! 设置相机的平移
			virtual void MoveCamera(core::vector3df newPos);

			//! 移动相机,通过像素比例设置新目标
			virtual void MoveCamera(core::vector2df newPos);

			//! 获取旋转状态
			bool getBRotating() { return m_bRotating; }

			void GetVectorAngle(core::vector3df vect,float& fViewX,float& fViewY);

			void GetAngleBySC(int nScX,int nScY,float& fHoriAngle,float& fVertAngle);

			// 恢复相机状态
			void FallBack3DCameraStatus();

			//根据WASD键输入调整相机视角
			void CameraAdjust(int type);//type(1：上调 2：右调 3：左调 4：下调)

			//初始化相机角度，通过计算得到
			void InitViewAngle();

			//给定旋转角来调整相机视角 liangjia
			void CameraAdjustByAngle(double fAngleH,double fAngleV);//Angle单位为弧度

			//得到当前相机状态,fViewX表示水平方向与中轴线的夹角。
			void GetCameraState(float& fViewX, float& fViewY, float& xFov, float& yFov);

			// 设置fov角最大最小值、传入弧度值 chy-2016-4-13
			void SetMinMaxFov( float minFov, float maxFov);

			// 获取fov角最大最小值、传入弧度值
			void GetMinMaxFov(float &minFov, float &maxFov);
			
			// 设置c垂直角最大最小值、传入角度值 chy-2016-7-23
			void SetMinMaxVerAngle( float minAngle, float maxAngle);

			// 获取垂直角最大最小值、传入角度值
			void GetMinMaxVerAngle(float &minAngle, float &maxAngle);

			// 获取鼠标捕捉状态
			BOOL GetMouseTrack(){return m_bMouseTrack;}
		};

	}
}



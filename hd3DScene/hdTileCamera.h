/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdTileCamera
文件名		：hdTileCamera.h
相关文件	: hdTileCamera.cpp
文件实现功能：提供切片浏览交互方法.
作者		：
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
创建记录：
日期		版本		创建人		创建内容
2016/4/21	1.0			程鹏		提供切片浏览交互方法
</PRE>
******************************************************************************************************/
#pragma once
#include "hdCamera.h"
#include "CMeshPolySceneNode.h"

using namespace irr;

namespace hd
{
	namespace fm
	{
		class HD3DSCENE_API CHdTileCamera :
			public CHdCamera
		{

		private:
			core::position2df m_RotateStart;
			core::position2df m_MousePos;
			f32 m_RotV, m_RotH, m_RotR;		//记录当前垂直、水平方向的角度值;
			bool m_bRotating;				        //开始旋转操作
			bool m_bViewAngleInit;       //判断旋转角是否被计算过,如果没有计算，
			//在返回旋转角前，先计算，再返回  liangjia 20140904
			CMeshPolySceneNode* m_meshSN;	//鼠标探面结果对象
		public:
			CHdTileCamera(void);
			CHdTileCamera(const char* caption,const char* msg,	const char* name,const char* tooltip,int type);
			virtual ~CHdTileCamera(void);

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
		};

	}
}



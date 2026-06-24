/*! @file
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : HdPointPock3D.h
相关文件     : HdPointPock3D.cpp,Hd3DPointSceneNode.h,Hd3DPointSceneNode.cpp. 
文件实现功能 : 移植点选功能到公共模块，只用于三维视图 
作者         : 冯晶
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2015/01/07   1.0     冯 晶                 创建
</PRE>
*******************************************************************************/
#pragma once
#include "..\hdFramework\hdTool.h"
#include "Hd3DPointSceneNode.h"
#include "hd3DCamera.h"

struct PointPos
{
	PointXYZIPRGBA pcd;	// 相对坐标
	hd::f64 gx;			// 全局x坐标 
	hd::f64 gy;			// 全局y坐标
	hd::f64 gz;			// 全局z坐标
	int X;				// 屏幕x
	int Y;				// 屏幕y
};

namespace hd
{
	namespace fm
	{
		class HD3DSCENE_API CHdPointPock3D :
			public CHdTool
		{
		public:
			CHdPointPock3D(void);
			~CHdPointPock3D(void);

			// 属性
		private:
			// 绘制区域的场景结点
			CHdSxPoint3D* m_p3DPoint;

			// 标记鼠标点击位置
			CHdPointSceneNode3D* m_p3DPointSN;

			// 鼠标点击三维坐标
			PointPos m_PCDPOS;

		protected:
			// 三维视图下浏览工具
			CHd3DCamera* m_p3DCamera;

			//鼠标按下位置
			core::vector2di m_ptMouseDown;

			//删除以及绘制的场景结点
			void DeletePoint();

		public:
			// 获取鼠标点击的三维坐标
			PointPos GetBtnPos() { return m_PCDPOS; }

			// 方法
		public:

			virtual void OnClick();

			virtual void OnCreate( CHdApp* app );

			virtual bool GetEnable();

			virtual void OnMouseDown( int Button, int Shift, int X, int Y );

			virtual void OnMouseUp( int Button, int Shift, int X, int Y );

			virtual void Deactivate();

			virtual bool OnContextMenu( int X, int Y );

			virtual void OnMouseMove( int Button, int Shift, int X, int Y );

			virtual void OnMouseWheel( UINT nFlags, short zDelta, int X, int Y );
		};
	}
}


/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : HdPolySelect3D.h
相关文件     : HdPolySelect3D.cpp
文件实现功能 : 实现三维视图下矩形选择（公共模块使用）
作者         : 软件部，朱旭波
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/10/18   1.00     朱旭波              
</PRE>
*******************************************************************************/

#pragma once
#include "..\hdFramework\hdTool.h"
#include "hd3DCamera.h"
#include "..\hdCommon\sceneData\HdSxPolyline2D.h"
#include "HdPolySelectIn3DSceneNode.h"

namespace hd
{
	namespace fm
	{
		class HD3DSCENE_API CHdPolySelect3D :
			public CHdTool
		{
		public:
			CHdPolySelect3D(void);
			~CHdPolySelect3D(void);

		protected:
			// 鼠标按下位置
			core::vector2di m_ptMouseDown;
			//ISceneView* m_pView;				//记录添加了多边形的试图视图

			CHdSxPolyline2D*	m_pPolyline2D;
			HRGN m_rgnPoly;
			CHdPolySelectIn3DSceneNode* m_pPolySelectSN;
			CHd3DCamera*		m_p3DCamera;
			bool				m_bStart;

			bool m_bPolySuccess;//绘制成功

			//移除多边形选择区
			void DeletePolygon();
			
			//由多边形创建区域
			bool CreateRegion();

			//! 选择点云
			bool SelectPoints();
			
			// 移除选择点
			void DeselectAll();

			// 获得选择点云的XYZ最大最小值（点云坐标）
			bool GetSelectPcdBounding(double& fMinX,double& fMinY,double& fMinZ,
				double& fMaxX,double& fMaxY,double& fMaxZ);

		public:
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
		};
	}
}



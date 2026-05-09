/*! @file
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : HdPointPock3D.cpp
相关文件     : HdPointPock3D.h,Hd3DPointSceneNode.h,Hd3DPointSceneNode.cpp.
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
#include "StdAfx.h"
#include "HdPointPock3D.h"
#include "CScanSceneNode.h"
#include "hd3DView.h"
#include "..\hdFramework\hdCommandDef.h"
#include "..\hdPointCloud\hdPointCloud.h"
#include <WindowsX.h>
#include "..\hdCommon\point_types.h"

using namespace hd;

namespace hd
{
	namespace fm
	{

		CHdPointPock3D::CHdPointPock3D(void)
		{
			m_app = NULL;
			m_caption = "hdToolPickPoint";
			m_message = "选择点";
			m_name = "hdToolPickPoint";
			m_toolTip = "点选工具";
			m_type = E_HVT_3D;
			m_id = COMMAND_PICK_POINT_ONLY_3D;

			m_p3DPointSN = NULL;
			m_p3DPoint = NULL;
		}


		CHdPointPock3D::~CHdPointPock3D(void)
		{
			DeletePoint();

			if (m_p3DCamera)
			{
				m_p3DCamera = NULL;
			}
		}

		void CHdPointPock3D::OnClick()
		{
			m_checked = true;
			m_p3DCamera = (CHd3DCamera*)(m_app->GetTool(COMMAND_3D_CAMERA));
			// 点选工具不抽稀显示点云 [2013/12/9 危迟]
			if (m_app && m_app->GetActiveView() != NULL)
			{
				IHdView* pView = m_app->GetActiveView();

				ISceneView* pSView = dynamic_cast<ISceneView*>(pView);

				if (pSView)
				{
					pSView->SetPointCloudSimpleRender(false);
				}
			}
		}

		void CHdPointPock3D::OnCreate( CHdApp* app )
		{
			if(app == NULL)
				return;
			m_app = app;
		}

		bool CHdPointPock3D::GetEnable()
		{
			m_enabled = false;
			if (m_app && m_app->GetActiveView() != NULL)
			{
				ENUM_HD_VIEW_TYPE type = m_app->GetActiveView()->GetViewType();
				m_enabled = ((type == E_HVT_3D));
			}

			return m_enabled;
		}

		void CHdPointPock3D::DeletePoint()
		{
			if (m_p3DPoint)
			{
				delete m_p3DPoint;
				m_p3DPoint = NULL;
			}

			if (m_p3DPointSN)
			{
				//删除视中对应的SceneNode对象	
				CHd3DView* p3DView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
				if (!p3DView)
				{
					return;
				}
				m_p3DPointSN->remove();
				m_p3DPointSN = NULL;

				p3DView->Refresh();
			}
		}

		void CHdPointPock3D::OnMouseDown( int Button, int Shift, int X, int Y )
		{
			//EnterCriticalSection(&m_cs);
			CHd3DView* view = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
			if (view == NULL)
			{
				return;
			}

			if (view->GetViewType() != E_HVT_3D)
			{
				return;
			}

			m_p3DCamera->OnMouseDown(Button,Shift,X,Y);		
			m_ptMouseDown.set(X,Y);
		}

		void CHdPointPock3D::OnMouseUp( int Button, int Shift, int X, int Y )
		{
			IHdView* pView = m_app->GetActiveView();
			if (pView == NULL)
			{
				return;
			}
			if (pView->GetViewType() == E_HVT_3D)
			{	
				// 支持选旋转平移
				m_p3DCamera->OnMouseUp(Button,Shift,X,Y);
				//鼠标未拖动情况下 添加选点
				if (m_ptMouseDown.X != X || m_ptMouseDown.Y != Y)
				{
					return;
				}
			}

			// 单选有右键取消功能，多选不需要，若需要删除，可在对话框中删除即可,方便管理
			//右键取消工具
			if (Button == 2)
			{
				DeletePoint();
				return;
			}

			if (Button == 1)//左键
			{				
				if (pView == NULL)
				{
					return;
				}
				ENUM_HD_VIEW_TYPE type = pView->GetViewType();

				// 根据屏幕坐标获取点云坐标
				hd::f64 gx = 0.0;  // 全局x坐标 
				hd::f64 gy = 0.0;  // 全局y坐标
				hd::f64 gz = 0.0;  // 全局z坐标
				PointXYZIPRGBA pcd;// 相对坐标

				// 获取当前视图
				CHd3DView* p3DView = dynamic_cast<CHd3DView*>(pView);

				// 是否获取到点云
				bool bRet = false;

				// 通过视图类型进行判断
				if (type == E_HVT_3D)
				{
					// 获取相对坐标和绝对坐标
					bRet= p3DView->Get2PosFromScrPos(pcd, gx, gy, gz, ESNT_SCAN_POINT, X, Y, 5);
				}

				//点击没有点云区域不显示Dlg
				if (!bRet)
				{
					return;
				}

				// 获取反射强度 占用第13、14位标记分类选中后 真实强度值只有12位 0-4096
				pcd.intensity = pcd.getIntensity();

				// 显示坐标
				double gxtmp;
				double gytmp;
				double gztmp;

				// 通过视图，从绝对坐标到显示坐标
				CBursaWolfModel* model = p3DView->GetTransModel();
				gxtmp = gx;
				gytmp = gy;
				gztmp = gz;	
				model->Translate(gxtmp, gytmp, gztmp);

				//  点选点节点 采用更新坐标方式刷新，而不是实时生成
				if (m_p3DPoint == NULL)
				{
					// 如果点指针为空，则New一个
					m_p3DPoint = new CHdSxPoint3D(gxtmp, gytmp, gztmp);
				}
				else
				{
					// 如果存在则进行数据更新
					m_p3DPoint->m_x = gxtmp;
					m_p3DPoint->m_y = gytmp;
					m_p3DPoint->m_z = gztmp;
				}

				// 单选添加
				if (m_p3DPointSN == NULL)
				{
					m_p3DPointSN = new CHdPointSceneNode3D(m_p3DPoint, EPS_CROSS,
						p3DView->GetSceneManager()->getRootSceneNode(), p3DView->GetSceneManager(), -1);
					m_p3DPointSN->SetView(p3DView);
					m_p3DPointSN->drop();
				}
				else
				{
					m_p3DPointSN->UpdatePoint(m_p3DPoint);
					m_p3DPointSN->UpdateBoundingBox();
					m_p3DPointSN->SetColor(video::SColor(255,255,255,0));
				}
				
				// 将相对坐标传出去
				m_PCDPOS.pcd = pcd;
				m_PCDPOS.gx = gx;
				m_PCDPOS.gy = gy;
				m_PCDPOS.gz = gz;
				m_PCDPOS.X = X;
				m_PCDPOS.Y = Y;

				// 刷新视图
				pView->Refresh();	
			}
		}

		void CHdPointPock3D::Deactivate()
		{
			m_checked = false;	
			DeletePoint();

			//将点选查询dialog隐藏
			if (m_app && m_app->GetActiveView())
			{
				// 点选工具不抽稀显示点云 [2013/12/9 危迟]
				IHdView* pView = m_app->GetActiveView();

				ISceneView* pSView = dynamic_cast<ISceneView*>(pView);

				if (pSView)
				{
					pSView->SetPointCloudSimpleRender(true);
				}
				pSView->Refresh();
			}
		}

		bool CHdPointPock3D::OnContextMenu( int X, int Y )
		{
			return false;
		}

		void CHdPointPock3D::OnMouseMove( int Button, int Shift, int X, int Y )
		{
			if(!m_enabled)
				return;

			IHdView* pView = m_app->GetActiveView();
			if (pView == NULL)
			{
				return;
			}
			if (pView->GetViewType() == E_HVT_3D)
			{
				CHd3DView* view = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
				if (!view->IsViewRenderAllNode())
				{
					view->GetSceneManager()->SetAnimateState(false);
					//渲染点云
					view->SetViewRenderAllNode(true, false);
				}	
				// 支持选旋转平移
				m_p3DCamera->OnMouseMove(Button,Shift,X,Y);
			}
	
			pView->Refresh();
		}

		void CHdPointPock3D::OnMouseWheel( UINT nFlags, short zDelta, int X, int Y )
		{
			if (m_app->GetActiveView() == NULL)
			{
				return;
			}
			if (m_app->GetActiveView()->GetViewType() == E_HVT_3D)
			{
				CHd3DView* view = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
				if (!view->IsViewRenderAllNode())
				{
					view->GetSceneManager()->SetAnimateState(false);
					//渲染点云
					view->SetViewRenderAllNode(true, false);
				}		
				m_p3DCamera->OnMouseWheel(nFlags,zDelta,X,Y);
			}
		}
	}
}

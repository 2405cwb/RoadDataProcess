/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : HdPolySelect3D.cpp
相关文件     : HdPolySelect3D.h
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

#include "StdAfx.h"
#include "HdPolySelect3D.h"
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
		CHdPolySelect3D::CHdPolySelect3D(void)
		{
			m_caption = "CHd3DToolSelectPoly";
			m_message = "矩形三维选择";
			m_name = "CHd3DToolRectSelect";
			m_toolTip = "矩形三维选择";
			m_type = E_HVT_3D;
			m_app = NULL;
			m_id = COMMAND_ONLY_3D_SELECT_POLYLINE;

			m_p3DCamera = NULL;
			m_pPolyline2D = NULL;
			m_pPolySelectSN = NULL;
		}


		CHdPolySelect3D::~CHdPolySelect3D(void)
		{
			if (m_rgnPoly)
			{
				DeleteRgn(m_rgnPoly);
				m_rgnPoly = NULL;
			}
			DeletePolygon();
		}

		void CHdPolySelect3D::OnMouseDown( int Button, int Shift, int X, int Y )
		{
			if(!m_enabled)
				return;

			// 视图存在
			if (!m_app->GetActiveView())
			{
				return;
			}

			if (m_app->GetActiveView()->GetViewType() == E_HVT_3D )
			{
				ISceneView* view = dynamic_cast<ISceneView*>(m_app->GetActiveView());
				if (view)
				{
					if (Button == 4)	// 中键放大缩小平移
					{
						if (view)
						{
							if (!view->IsViewRenderAllNode())
							{
								////渲染点云
								view->SetViewRenderAllNode(true, false);
							}
						}
						m_p3DCamera->OnMouseDown(Button,Shift,X,Y);
					}
				}
			}

			if (Button != 1 || (m_ptMouseDown.X == X && m_ptMouseDown.Y == Y))
				return;
			m_ptMouseDown.set(X,Y);

			ISceneView* pView = dynamic_cast<ISceneView*>(m_app->GetActiveView());

			if (pView)
			{
				if (pView->IsViewRenderAllNode())
				{
					//为了下一次的选择，需要重新截取屏幕
					pView->SetViewRenderAllNode(false);
				}
			}

			if (m_pPolyline2D == NULL)
			{				
				m_pPolyline2D = new CHdSxPolyline2D;					

				CHdSxPoint2D ptRet(X, Y);


				if (pView->GetViewType() == E_HVT_3D )
				{
					CHd3DView* p3DView = dynamic_cast<CHd3DView*>(pView);
					m_pPolyline2D->AddPoint(X, Y);
					m_pPolyline2D->AddPoint(X, Y);
					m_pPolyline2D->AddPoint(X, Y);
					m_pPolyline2D->AddPoint(X, Y);
					p3DView->GetSceneManager()->SetAnimateState(true);
					
					//在视图中添加线对象对应的SceneNode
					// 添加SN
					const CHdSxPolyline2D* pPolyline2D = m_pPolyline2D;
					CHdPolySelectIn3DSceneNode* pPolySelectSN = new CHdPolySelectIn3DSceneNode(pPolyline2D, 
						p3DView->GetSceneManager()->getRootSceneNode(), p3DView->GetSceneManager(),-1);
					pPolySelectSN->SetView(p3DView);
					pPolySelectSN->drop();

					m_pPolySelectSN = dynamic_cast<CHdPolySelectIn3DSceneNode*>(pPolySelectSN);
				}

				//更新Polyline SceneNode的范围
				if (m_pPolySelectSN)
				{
					m_pPolySelectSN->UpdateBoundingBox();
				}

				pView->Refresh();
			}
			else		//if (m_bFinish)
			{
				if (m_pPolySelectSN)
				{
					if (m_pPolySelectSN->m_bPolySuccess)
					{	
						//闭合
						if (pView->GetViewType() == E_HVT_3D )
						{
							m_pPolyline2D->Close();
							if (CreateRegion())
							{
								SelectPoints();
								DeletePolygon();
							}
							CHd3DView* p3DView = dynamic_cast<CHd3DView*>(pView);
							p3DView->GetSceneManager()->SetAnimateState(false);
							if (!p3DView->IsViewRenderAllNode())
							{
								p3DView->SetViewRenderAllNode(true);	
							}
						}
					}
					else//绘制失败则删除多段线
					{
						DeletePolygon();
					}
					pView->Refresh();
				}
			}
		}

		void CHdPolySelect3D::OnMouseUp( int Button, int Shift, int X, int Y )
		{
			IHdView* pView = m_app->GetActiveView();
			if (pView == NULL)
			{
				return;
			}

			if (m_app->GetActiveView()->GetViewType() == E_HVT_3D)
			{
				if (Button == 4)	// 中键放大缩小平移
				{
					ISceneView* view = dynamic_cast<ISceneView*>(m_app->GetActiveView());
					if (view)
					{
						if (!view->IsViewRenderAllNode())
						{
							////渲染点云
							view->SetViewRenderAllNode(true, true);
						}
					}
					m_p3DCamera->OnMouseUp(Button,Shift,X,Y);
				}
			}

			// 在绘制多边形的过程中，右键取消绘制，同时向主界面发送消息，不弹出右键菜单 [危迟]
			if (Button == 2)
			{
				bool bIsDrawing = false;
				if (m_pPolySelectSN)
				{
					bIsDrawing = true;
				}
				DeletePolygon();
				pView->Refresh();

				return;
			}

		}

		void CHdPolySelect3D::OnClick()
		{
			if (!GetEnable())
				return;	
			m_checked = true;
			m_p3DCamera = (CHd3DCamera*)(m_app->GetTool(COMMAND_3D_CAMERA));
		}

		void CHdPolySelect3D::OnCreate( CHdApp* app )
		{
			if(app == NULL)
				return;
			m_app = app;
		}

		bool CHdPolySelect3D::GetEnable()
		{
			m_enabled = false;
			if (m_app && m_app->GetActiveView() != NULL
				&& (m_app->GetActiveView()->GetViewType() == E_HVT_3D))
			{
				m_enabled = true;
			}
			return m_enabled;
		}

		void CHdPolySelect3D::Deactivate()
		{
			IHdView* pView = m_app->GetActiveView();
			if (!pView )
			{
				return;
			}

			m_checked = false;
			DeletePolygon();

			if (m_rgnPoly)
			{
				DeleteRgn(m_rgnPoly);
				m_rgnPoly = NULL;
			}

			CHd3DView* p3DView = dynamic_cast<CHd3DView*>(pView);
			if (p3DView && !p3DView->IsViewRenderAllNode())
			{
				////渲染点云
				p3DView->SetViewRenderAllNode(true, false);
			}
		}

		bool CHdPolySelect3D::OnContextMenu( int X, int Y )
		{
			return true;
		}

		void CHdPolySelect3D::OnDblClick( int Button, int Shift, int X, int Y )
		{
			return;

			if(!m_enabled)
				return;
			IHdView* pView = m_app->GetActiveView();
			if (pView == NULL)
			{
				return;
			}

			if (m_pPolyline2D == NULL || m_pPolyline2D->GetPointCount() < 3)
			{
				return;
			}
			if (m_app->GetActiveView() == NULL)
			{
				return;
			}
			ENUM_HD_VIEW_TYPE type = pView->GetViewType();
			if (type == E_HVT_3D )
			{
				m_pPolyline2D->Close();
				if (CreateRegion())
				{
					//	//根据生成的区域 选择点
					SelectPoints();
					//	//之后删除多边形
					DeletePolygon();
				}
				CHd3DView* p3DView = dynamic_cast<CHd3DView*>(pView);
				p3DView->GetSceneManager()->SetAnimateState(false);
				if (!p3DView->IsViewRenderAllNode())
				{	
					//双击完成后，选中的渲染点云
					p3DView->SetViewRenderAllNode(true);
					//int scanIndex = p3DView->GetScanIndex();
					//if (scanIndex >= 0)
					//{
					//	CHdApplication::getAppInstance()->UpdateRenderStyleByScanIndex(scanIndex);
					//}
					//else
					//{
					//	p3DView->SetScanAreaRenderStyle(p3DView->GetScanAreaRenderStyle());
					//}
				}
				p3DView->Refresh();

			}

			m_bStart = false;
		}

		void CHdPolySelect3D::OnMouseMove( int Button, int Shift, int X, int Y )
		{
			if(!m_enabled)
				return;

			// 在任意影像与点云配准中使用选择工具时，禁止图片视图使用
			IHdView* pView = m_app->GetActiveView();
			if (pView == NULL)
			{
				return;
			}

			if (m_app->GetActiveView()->GetViewType() == E_HVT_3D )
			{
				CHd3DView* p3DView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
				if (Button == 4)	// 中键放大缩小平移
				{
					p3DView->GetSceneManager()->SetAnimateState(false);
					if (!p3DView->IsViewRenderAllNode())
					{
						//渲染点云
						p3DView->SetViewRenderAllNode(true, false);
					}
					m_p3DCamera->OnMouseMove(Button,Shift,X,Y);
				}
			}

			if (m_pPolyline2D )
			{
				ENUM_HD_VIEW_TYPE type = pView->GetViewType();

				// 得到折线点个数
				unsigned int nPtCount = m_pPolyline2D->GetPointCount();

				// 鼠标移动到视图外时，可能一个点都没有，此时应直接返回
				if (nPtCount <= 0)
				{
					return;
				}

				//更新其中三个点
				for (int i = 0; i < nPtCount - 1;i++)
				{
					m_pPolyline2D->DeleteLastPoint();
				}

				if (type == E_HVT_3D)
				{
					m_pPolyline2D->AddPoint(m_pPolyline2D->GetPoint(0).m_x,Y);
					m_pPolyline2D->AddPoint(X,Y);
					m_pPolyline2D->AddPoint(X,m_pPolyline2D->GetPoint(0).m_y);
				}

				m_pPolySelectSN->UpdateBoundingBox();//更新Polyline SceneNode的范围
				pView->Refresh();
			}
		}

		void CHdPolySelect3D::OnMouseWheel( UINT nFlags, short zDelta, int X, int Y )
		{
			if(!m_enabled)
				return;

			// 在任意影像与点云配准中使用选择工具时，禁止图片视图使用
			if (m_app->GetActiveView() == NULL)
			{
				return;
			}

			if (m_app->GetActiveView()->GetViewType() == E_HVT_3D)
			{
				CHd3DView* p3DView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
				p3DView->GetSceneManager()->SetAnimateState(false);
				if (!p3DView->IsViewRenderAllNode())
				{
					p3DView->SetViewRenderAllNode(true);
				}
				m_p3DCamera->OnMouseWheel(nFlags,zDelta,X,Y);
			}		
		}

		void CHdPolySelect3D::OnKeyDown( int keyCode, int Shift )
		{

		}

		void CHdPolySelect3D::OnKeyUp( int keyCode, int Shift )
		{
			return;

			IHdView* pView = m_app->GetActiveView();
			if (m_pPolyline2D == NULL ||
				pView == NULL)
			{
				return;
			}

			// U键或ctrl+z撤销上一个点,u=85 z=90,Shift==2表示ctrl键按下
			if (keyCode == 85 || (keyCode == 90 && Shift == 2))
			{
				m_pPolyline2D->DeleteLastPoint();
				m_pPolySelectSN->UpdateBoundingBox();

				pView->Refresh();
			}
			// Esc键停止绘制 
			else if (keyCode == 27)
			{
				bool bIsDrawing = false;
				if (m_pPolySelectSN)
				{
					bIsDrawing = true;
				}
				DeletePolygon();
				pView->Refresh();

				////不显示菜单
				//if (bIsDrawing)
				//{
				//	pView->SendMsgToWindowsView(WM_USER_DONOTSHOWMENU,WPARAM(false),LPARAM(0));
				//}

				//Deactivate();
			}
		}

		bool CHdPolySelect3D::CreateRegion()
		{
			bool ret = false;
			if (m_pPolyline2D)
			{	
				IHdView* pView = m_app->GetActiveView();
				if (pView == NULL)
				{
					return false;
				}
				ENUM_HD_VIEW_TYPE type = pView->GetViewType();

				int nPtCount = m_pPolyline2D->GetPointCount()-1;
				POINT* ptVertex = new POINT[nPtCount];
				if (type == E_HVT_3D )
				{
					for (int i = 0;i<nPtCount;i++)
					{
						ptVertex[i].x = hd_round32(m_pPolyline2D->GetPoint(i).m_x);
						ptVertex[i].y = hd_round32(m_pPolyline2D->GetPoint(i).m_y);
					}
				}

				if (m_rgnPoly)
				{
					DeleteRgn(m_rgnPoly);
					m_rgnPoly = NULL;
				}
				m_rgnPoly = CreatePolygonRgn(ptVertex,nPtCount,1);
				if (m_rgnPoly)
				{
					ret = true;
				}
				delete[] ptVertex;
				ptVertex = NULL;
			}
			return ret;
		}

		bool CHdPolySelect3D::SelectPoints()
		{
			//获取当前活动视图
			IHdView* pView = m_app->GetActiveView();
			if(pView == NULL || pView->GetViewType() != E_HVT_3D)
				return false;

			CHd3DView* p3DView = NULL;
			ISceneManager* sceneMng = NULL;
			CBursaWolfModel absTrans;	// 转换视图坐标系
			p3DView = dynamic_cast<CHd3DView*>(pView);
			sceneMng = p3DView->GetSceneManager();			

			// 范围判断相关变量
			core::aabbox3d<f32> cube;
			bool bIsCubeIn = false;
			bool bLoopIntersect = false;
			float xmin,ymin,zmin,xmax,ymax,zmax;
			double xminD,yminD,zminD,xmaxD,ymaxD,zmaxD;
			const irr::scene::SViewFrustum* pViewFrustum = sceneMng->getActiveCamera()->getViewFrustum();
			irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
			RECT srcRect;
			GetRgnBox(m_rgnPoly,&srcRect);
			irr::core::recti irrRect;
			irrRect.LowerRightCorner.set(srcRect.right,srcRect.bottom);
			irrRect.UpperLeftCorner.set(srcRect.left,srcRect.top);
			sceneMng->GetViewFrustum(irrRect,&rgnFrustum);

			//遍历点云坐标
			core::vector3df coord;
			core::position2di screenPos;
			core::vector2df imgScale;
			double x,y,z;

			//// 当前为显示选择点的部分 且  只对显示的点进行操作   fengjing 20140910
			//if (p3DView->GetShowStyle() == SHOW_SELECT && CHdSysSetting::getSysSetting()->measureSetting.nSelPtsMode == 1)
			//{
			//	// 遍历选择的点云
			//	for (std::vector<PointCloud*>::iterator it = m_aryPcd.begin();
			//		it != m_aryPcd.end();it++)
			//	{
			//		PointCloud* pcd = (*it);
			//		if (!pcd)
			//		{
			//			return;
			//		}

			//		CScanSceneNode* pScanSN = GetSceneNodeByPcd(p3DView,pcd);

			//		if (!pScanSN)
			//		{
			//			continue;
			//		}

			//		// 新相对坐标、绝对坐标、显示坐标转化方法 [2014/03/20 危迟]
			//		// 获取场景节点中的点云在显示时是否对相对坐标进行了转换
			//		bool bTrans = pScanSN->IsTrans();

			//		// 获取相对坐标转绝对坐标的模型
			//		CBursaWolfModel absModel = pScanSN->GetModel();

			//		// 获取相对坐标转显示坐标的模型
			//		CBursaWolfModel renderModel = pScanSN->GetRenderModel();

			//		u32 loopCount = pcd->getLoopCount();

			//		unsigned int i = 0;
			//		// 获取点云
			//		hdBlkArray<PointXYZIPRGBA>& pts = pcd->getPoints();
			//		//#pragma omp parallel for private(n)
			//		for (i = 0; i < pcd->getSelectCount();i++)
			//		{
			//			// 获取选择的点对应于内存中点的索引
			//			u32 Index = pcd->getIndexBySelectionID(i);

			//			PointXYZIPRGBA& pt = pts[Index];
			//			if (!pt.isValid())
			//			{
			//				continue;
			//			}

			//			BOOL scrInside = FALSE;

			//			if (bTrans)
			//			{
			//				x = pt.x;
			//				y = pt.y;
			//				z = pt.z;
			//				renderModel.Translate(x,y,z);
			//				coord.set((float)x, (float)y, (float)z);
			//			}
			//			else
			//			{
			//				coord.set((float)pt.x, (float)pt.y, (float)pt.z);
			//			}

			//			screenPos = sceneMng->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);

			//			if (screenPos.X <= m_pActiveView->GetWindowWidth() ||
			//				screenPos.Y <= m_pActiveView->GetWindowHeight() ||
			//				screenPos.X >= 0 || screenPos.Y >= 0)
			//			{
			//				scrInside = ::PtInRegion(rgn,screenPos.X,screenPos.Y);
			//			}

			//			if((!scrInside && bInside) ||
			//				(scrInside && !bInside))
			//			{
			//				if (selMode == SELECT_ADD ||
			//					selMode == SELECT_NEW)
			//				{
			//					// 首先将选中的点设为不选中
			//					pt.setUnSelected();
			//				}
			//				continue;
			//			}

			//			if (selMode == SELECT_ADD ||
			//				selMode == SELECT_NEW)
			//			{
			//				pt.setSelected();
			//			}
			//			else if (selMode == SELECT_MIUS)
			//			{
			//				// 设置不选中
			//				pt.setUnSelected();
			//			}
			//			else if (selMode == SELECT_UNSEL)
			//			{
			//				// 设置反选
			//				pt.setXorSelected();
			//			}
			//		}

			//		// 统计选中点个数,选择失败则跳出循环
			//		if (!pcd->setSelectCount())
			//		{
			//			break;
			//		}

			//		// 更新对应灰度图
			//		int nViewIndex = ((CHdSx3DView*)m_pActiveView)->GetScanIndex();
			//		CHdApplication::getAppInstance()->UpdataImageByScanIndex(nViewIndex);
			//	}
			//}
			//else
			{
				// 如果是新建选择,则先清理选择
				//if (selMode == SELECT_NEW)
				{
					DeselectAll();
				}

				core::array<ISceneNode*> arrSn;
				sceneMng->getSceneNodesFromType(ESNT_SCAN_POINT,arrSn);
				u32 selectCount = 0;	

				for (unsigned int m = 0;m < arrSn.size();m++)
				{
					CScanSceneNode* pScanSN = (CScanSceneNode*)(arrSn[m]);

					if (!pScanSN)
					{
						continue;
					}

					PointCloud* pcd = pScanSN->GetPointCloud();

					// 新相对坐标、绝对坐标、显示坐标转化方法 [2014/03/20 危迟]
					// 获取场景节点中的点云在显示时是否对相对坐标进行了转换
					bool bTrans = pScanSN->IsTrans();

					// 获取相对坐标转绝对坐标的模型
					CBursaWolfModel absModel = pScanSN->GetModel();

					// 获取相对坐标转显示坐标的模型
					CBursaWolfModel renderModel = pScanSN->GetRenderModel();

					u32 loopCount = pcd->getLoopCount();

					int n = 0;
					for (n = 0;n < loopCount;n++)
					{
						hdVector<PointXYZIPRGBA>& pts = pcd->getLoop(n);
						core::aabbox3d<f32>loopExtentTmp;
						loopExtentTmp.MaxEdge.set(F32_MIN,F32_MIN,F32_MIN);
						loopExtentTmp.MinEdge.set(F32_MAX,F32_MAX,F32_MAX);

						// 判断当前圈是否和rgh相交
						//if (bInside)
						{
							pcd->getLoopExtent(n,xmin,ymin,zmin,xmax,ymax,zmax);
							if (bTrans)
							{
								xminD = xmin;
								yminD = ymin;
								zminD = zmin;

								xmaxD = xmax;
								ymaxD = ymax;
								zmaxD = zmax;

								cube.MinEdge.set(xmin,ymin,zmin);
								cube.MaxEdge.set(xmax,ymax,zmax);

								// 重新计算box，因为查看全图视锥体初始化话时，box更新，这边计算方式需保持一致[2014/9/24 蔡红云]
								core::vector3df edges[8];
								cube.getEdges(edges);
								for (int i =0 ; i!=8;i++)
								{
									renderModel.Translate(edges[i].X,edges[i].Y,edges[i].Z);
									loopExtentTmp.addInternalPoint(edges[i]);
								}
								cube = loopExtentTmp;

							}
							else
							{
								cube.MinEdge.set(xmin,ymin,zmin);
								cube.MaxEdge.set(xmax,ymax,zmax);
							}	

							// 判断圈是否和设置区域相交
							//bIsCubeIn = rgnFrustum.isCubeIn(cube);

							// 圈与设置区域相交并不完全准确，存在部分真实应相交的被忽略导致选中不完整，此处建议使用视锥相交--add by zhubo 2014.07.31
							bIsCubeIn = pViewFrustum->isCubeIn(cube);
							if(!bIsCubeIn)
								continue;
						}

						for (unsigned int i = 0;i < pts.size();i++)
						{
							PointXYZIPRGBA& pt = *(pts._Myfirst + i);
							if (!pt.isValid())
							{
								continue;
							}

							BOOL scrInside = FALSE;

							if (bTrans)
							{
								x = pt.x;
								y = pt.y;
								z = pt.z;
								renderModel.Translate(x,y,z);
								coord.set((float)x, (float)y, (float)z);
							}
							else
							{
								coord.set((float)pt.x, (float)pt.y, (float)pt.z);
							}

							screenPos = sceneMng->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);

							if (screenPos.X <= pView->GetWindowWidth() ||
								screenPos.Y <= pView->GetWindowHeight() ||
								screenPos.X >= 0 || screenPos.Y >= 0)
							{
								scrInside = ::PtInRegion(m_rgnPoly,screenPos.X,screenPos.Y);
							}

							if (!scrInside)
							{
								continue;
							}

							//if((!scrInside && bInside) ||
							//	(scrInside && !bInside))
							//{
							//	continue;
							//}

							//if (selMode == SELECT_ADD ||
							//	selMode == SELECT_NEW)
							{
								pt.setSelected();
							}
							//else if (selMode == SELECT_MIUS)
							//{
							//	// 设置不选中
							//	pt.setUnSelected();
							//}
							//else if (selMode == SELECT_UNSEL)
							//{
							//	// 设置反选
							//	pt.setXorSelected();
							//}
						}
					}

					// 统计选中点个数,选择失败则跳出循环
					if (!pcd->setSelectCount())
					{
						break;
					}
				}
			}

			//double fMinX,fMinY,fMinZ,fMaxX,fMaxY,fMaxZ;
			//GetSelectPcdBounding(fMinX,fMinY,fMinZ,fMaxX,fMaxY,fMaxZ);

			//// 输出
			//char strMsg[512];
			//sprintf(strMsg,"%sX:\t%lf\n%sY:\t%lf\n%sZ:\t%lf\n%sX:\t%lf\n%sY:\t%lf\n%sZ:\t%lf",
			//	"min",fMinX,"min",fMinY,
			//	"min",fMinZ,"max",fMaxX,"max", fMaxY,"max",fMaxZ);
			//::MessageBox(m_app->GetMainWnd(),strMsg,"测试",MB_OK);

			return true;
		}

		void CHdPolySelect3D::DeletePolygon()
		{
			//删除正在绘制的线段
			if (m_pPolyline2D)
			{
				delete m_pPolyline2D;
				m_pPolyline2D = NULL;
			}
			m_bPolySuccess = false;

			//删除场景节点和多段线
			if (m_pPolySelectSN)
			{
				if (m_pPolySelectSN)
				{
					m_pPolySelectSN->remove();
					m_pPolySelectSN = NULL;
				}

			}

		}

		// 移除所有选择点
		void CHdPolySelect3D::DeselectAll()
		{
			IHdView* pView = m_app->GetActiveView();
			if(pView == NULL || pView->GetViewType() != E_HVT_3D)
				return;

			core::array<ISceneNode*> arrSn;
			CHd3DView* p3dView = (CHd3DView*)pView;
			if (!p3dView)
			{
				return;
			}
			
			p3dView->GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,arrSn);

			for (unsigned int n = 0; n < arrSn.size();n++)
			{
				CScanSceneNode* pScanSn = (CScanSceneNode*)(arrSn[n]);
				if (!pScanSn)
				{
					continue;
				}

				PointCloud* pcd = pScanSn->GetPointCloud();
				if (!pcd)
				{
					continue;
				}

				u32 selCount = pcd->getSelectCount();
				for (u32 i = 0;i < selCount;i++)
				{
					PointXYZIPRGBA& pt = pcd->getSelectionPoint(i);
					pt.setUnSelected();
				}
				
				// 统计选中点个数,选择失败则跳出循环
				if (!pcd->setSelectCount())
				{
					break;
				}
			}
		}

		bool CHdPolySelect3D::GetSelectPcdBounding( double& fMinX,double& fMinY,double& fMinZ, double& fMaxX,double& fMaxY,double& fMaxZ )
		{
			IHdView* pView = m_app->GetActiveView();
			if (!pView || pView->GetViewType() != E_HVT_3D)
			{
				return false;
			}

			// 中间定义
			core::array<ISceneNode*> arrSn;
			CHd3DView* p3dView = (CHd3DView*)(pView);
			if (!p3dView)
			{
				return false;
			}

			// 获得视图中所有点云SN
			p3dView->GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,arrSn);
			if (arrSn.size() <= 0)
			{
				return false;
			}

			// 初值
			double tmpX,tmpY,tmpZ;
			fMinX = fMinY = fMinZ = F32_MAX;
			fMaxX = fMaxY = fMaxZ = F32_MIN;

			// 遍历统计内存中所有选中点的范围值
			for (unsigned int i = 0;i < arrSn.size();i++)
			{
				// 获得SN
				CScanSceneNode* pScanSn = (CScanSceneNode*)(arrSn[i]);
				if (!pScanSn)
				{
					continue;
				}

				// 由SN获得点云
				PointCloud* pcd = pScanSn->GetPointCloud();
				if (!pcd)
				{
					continue;
				}

				CBursaWolfModel model = pcd->GetModel();

				u32 selCount = pcd->getSelectCount();
				for (u32 i = 0;i < selCount;i++)
				{
					PointXYZIPRGBA& pt = pcd->getSelectionPoint(i);
					tmpX = pt.x;
					tmpY = pt.y;
					tmpZ = pt.z;

					// 转换至绝对坐标
					model.Translate(tmpX,tmpY,tmpZ);

					// 比较更新最大最小值
					fMinX = min(tmpX,fMinX);
					fMinY = min(tmpY,fMinY);
					fMinZ = min(tmpZ,fMinZ);

					fMaxX = max(tmpX,fMaxX);
					fMaxY = max(tmpY,fMaxY);
					fMaxZ = max(tmpZ,fMaxZ);
				}
			}

			return true;
		}

	}
}

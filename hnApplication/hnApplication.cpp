#include "hnApplication.h"
#include "..\hd3DScene\Hd3DView.h"
#include "..\hd3DScene\hd3DCamera.h"
#include "..\hd3DScene\hdQuickCamera.h"
#include "..\hd3DScene\CScanSceneNode.h"
#include "..\hd3DScene\HdSeaDataSceneNode.h"
#include "..\hd3DScene\CPanoSceneNode.h"
#include "..\hd3DScene\ISceneView.h"
#include "..\hdFramework\hdCommandDef.h"
#include "..\hdFramework\userMessage.h"
#include "..\hdPointCloud\PointCloudCache.h"
#include "..\hdCommon\sceneData\HdSceneScan.h"
#include "..\hdCommon\HdSvTileInfoBuffer.h"
#include "..\hd3DScene\hd3DToolRotCentreSelect.h"
#include "..\hd3DScene\Hd3DCmdViewTop.h"
#include "..\hd3DScene\CScanPartPointsSceneNode.h"
#include "hnSysSetting.h"
#include "..\hnCommon\hnCloudDef.h"
#include "hn2DCameraView.h"
#include "hn3DCameraView.h"
#include "hnStreetCameraView.h"
#include "hn2dPixWidget.h"	//路面破损图片显示窗口


namespace hnApp
{
	hnApplication* hnApplication::m_pHnApp = NULL;

	hnApplication::hnApplication() :CHdApp()
	{
		// 注册命令
		RegisterCommand();
	}

	hnApplication::~hnApplication()
	{
		// 删除命令
		DeleteAllCommand();
	}

	// 获取单实例
	hnApplication* hnApplication::getApp()
	{
		if (!m_pHnApp)
		{
			m_pHnApp = new hnApplication();
		}

		return m_pHnApp;
	}

	// 析构单实例
	void hnApplication::destroyApp()
	{
		if (m_pHnApp)
		{
			delete m_pHnApp;
			m_pHnApp = NULL;
		}
	}

	hn2dPixScrollWidget * hnApplication::newRaodDamageContinousBrowserPixWidget()
	{
		hn2dPixScrollWidget *widget = new hn2dPixScrollWidget;
		widget->getPixWidget()->setObjectName("2d");
		return widget;
	}

	hn3dPixScrollWidget * hnApplication::new3DImageViewWidget()
	{
		hn3dPixScrollWidget *widget = new hn3dPixScrollWidget;
		widget->getPixWidget()->setObjectName("3d");
		return widget;
	}

	void hnApplication::closeDocument()
	{
		for (unsigned int i = 0; i < m_viewList.size(); i++)
		{
			if (m_viewList[i] != NULL)
			{
				delete m_viewList[i];
				m_viewList[i] = NULL;
			}
		}
		m_viewList.clear();
	}

	void hnApplication::RegisterCommand()
	{
		// 注册3d浏览工具
		CHd3DCamera* hd3DCamera = new CHd3DCamera();
		hd3DCamera->SetAnimator(false);
		hd3DCamera->OnCreate(this);
		m_cmdList1.push_back(hd3DCamera);

		// 注册快速浏览工具
		CHdQuickCamera* hdQuickCamera = new CHdQuickCamera();
		hdQuickCamera->OnCreate(this);
		m_cmdList1.push_back(hdQuickCamera);

		// 注册旋转中心选择工具
		CHd3DToolRotCentreSelect* hnRotCenterSel = new CHd3DToolRotCentreSelect();
		hnRotCenterSel->OnCreate(this);
		m_cmdList1.push_back(hnRotCenterSel);

		// 注册俯视图工具
		CHd3DCmdViewTop* hn3dCmdViewTop = new CHd3DCmdViewTop();
		hn3dCmdViewTop->OnCreate(this);
		m_cmdList1.push_back(hn3dCmdViewTop);

		m_cmdList = m_cmdList1;
	}

	void hnApplication::DeleteAllCommand()
	{
		std::vector<CHdCommand*>::iterator pIter;
		for (pIter = m_cmdList1.begin(); pIter != m_cmdList1.end(); pIter++)
		{
			if ((*pIter) != NULL)
			{
				delete *pIter;
			}
		}

		m_cmdList1.clear();
	}

	void hnApplication::SetCurrentTool(int id)
	{
		CHdApp::SetCurrentTool(id);
	}

	void hnApplication::SetCurrentTool(CHdTool* pTool)
	{
		if (pTool && m_currentTool != pTool)//m_currentTool != tool，在当前按钮上再次点击，不做任何操作
		{
			//将原来的工具设置为非活动状态
			if (m_currentTool)
			{
				m_currentTool->Deactivate();
			}

			pTool->OnClick();
			m_currentTool = pTool;
		}
	}

	IHdView* hnApplication::new3DView(HWND hwnd)
	{
		if (hwnd == NULL)
			return NULL;

		//将当前按钮状态清除
		if (m_currentTool)
		{
			m_currentTool->Deactivate();
			m_currentTool = NULL;
		}

		CHd3DView* hdView = new CHd3DView();
		hdView->InitialView(hwnd);
		hdView->Set3DCameraTool((CHd3DCamera*)GetTool(COMMAND_3D_CAMERA));

		// 设置相机工具
		hdView->SetCameraTool((CHdCamera*)GetTool(COMMAND_3D_CAMERA));

		hdView->SetName("iScan3DView");
		if (m_activeView != hdView)
		{
			m_activeView = hdView;
			if (m_activeView)
			{
				m_activeView->Refresh();
			}
		}

		hdView->SetFPSVisiable(false);
		hdView->SetAxisVisiable(true);

		// 每次加载点云时，视图统一重置为正射投影
		hdView->SetViewProjectionType(E_HPT_ORTHOGONAL);
		//hdView->SetViewProjectionType(E_HPT_PERSPECTIVE);

		// 设置当前工具
		SetCurrentTool(COMMAND_3D_CAMERA);
		m_viewList.push_back(hdView);

		return hdView;
	}

	// 关闭视图
	void hnApplication::CloseView(IHdView* pView)
	{
		if (pView == NULL)
			return;
		for (vector<IHdView*>::iterator it = m_viewList.begin(); it != m_viewList.end(); it++)
		{
			if ((*it) == pView)
			{
				int CToolID;
				if (GetCurrentTool())
				{
					CToolID = GetCurrentTool()->GetID();
					GetCurrentTool()->Deactivate();
				}

				delete (*it);
				m_viewList.erase(it);

				if (m_activeView == pView)
				{
					m_activeView = NULL;
				}
				break;
			}
		}
	}

	void hnApplication::SetActiveViewOnly(IHdView* pView)
	{
		if (!pView)
		{
			return;
		}

		if (m_activeView != pView)
		{
			m_activeView = pView;

			if (m_activeView)
			{
				m_activeView->Refresh();
			}
		}

	}

	//! 三维点云窗口消息处理
	void hnApplication::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (hWnd == NULL)
			return;
		if ((message == WM_PAINT ||
			message == WM_SIZE))
		{
			for (unsigned int i = 0; i < m_viewList.size(); i++)
			{
				if (m_viewList[i]->GetHWnd() == hWnd)
				{
					m_viewList[i]->WindowProc(message, wParam, lParam);
					break;
				}
			}
		}

		// 视口变化时，更新纹理图片chy-2016-4-15
		//if (message == WM_SIZE)
		//{
		//	m_bFirstPanoView = true;
		//}

		// 点云卸载,自动关闭对应视图
		if (message == WM_USER_ClOSE_SEAPCD && wParam)
		{
			SEAPCD* pcd = (SEAPCD*)wParam;
			CSeaPointCloud* pPcd = pcd->pSEAPCD;
			bool bFind = false;
			for (unsigned int i = 0; i < m_viewList.size(); i++)
			{
				ISceneView* pSceneView = dynamic_cast<ISceneView*>(m_viewList[i]);
				if (!pSceneView)
					continue;

				// 卸载点云
				core::array<ISceneNode*> sceneList;
				pSceneView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT, sceneList);
				int nSNCount = sceneList.size();
				if (nSNCount > 0)
				{
					for (int j = 0; j < nSNCount; j++)
					{
						CHdSeaDataSceneNode* pScanSn = dynamic_cast<CHdSeaDataSceneNode*>(sceneList[j]);
						if (pScanSn && pScanSn->GetPointCloud() == pPcd)
						{
							pScanSn->remove();
							bFind = true;
						}
					}
				}

				if (bFind)
				{
					pSceneView->Refresh();
				}

				// 新建时，重置视图Model,
				if (m_bIsResetModel == true)
				{
					pSceneView->ResetTransModel();

					// 隐藏坐标轴
					pSceneView->SetAxisVisiable(false);
				}
			}
		}

		// 传递消息到当前工具
		if (m_currentTool == NULL)
		{
			return;
		}

		struct messageMap
		{
			irr::s32 group;
			UINT winMessage;
			irr::s32 irrMessage;
		};

		static messageMap mouseMap[] =
		{
			{ 0, WM_LBUTTONDOWN, irr::EMIE_LMOUSE_PRESSED_DOWN },
			{ 1, WM_LBUTTONUP,   irr::EMIE_LMOUSE_LEFT_UP },
			{ 0, WM_RBUTTONDOWN, irr::EMIE_RMOUSE_PRESSED_DOWN },
			{ 1, WM_RBUTTONUP,   irr::EMIE_RMOUSE_LEFT_UP },
			{ 0, WM_MBUTTONDOWN, irr::EMIE_MMOUSE_PRESSED_DOWN },
			{ 1, WM_MBUTTONUP,   irr::EMIE_MMOUSE_LEFT_UP },
			{ 2, WM_MOUSEMOVE,   irr::EMIE_MOUSE_MOVED },
			{ 3, WM_MOUSEWHEEL,  irr::EMIE_MOUSE_WHEEL },
			{ -1, 0, 0 }
		};

		// handle grouped events
		messageMap * m = mouseMap;
		while (m->group >= 0 && m->winMessage != message)
			m += 1;

		DWORD shiftState = GetKeyState(VK_LSHIFT) & 0xff00;
		DWORD ctrlState = GetKeyState(VK_LCONTROL) & 0xff00;
		//bool alt = ((LOWORD(wParam) & MK_ALT) != 0);
		int shiftkey = (shiftState == 0xff00 ? E_HPS_SHIFT : 0) | (ctrlState == 0xff00 ? E_HPS_CONTROL : 0);//| (alt ? altPress:0)
		RECT rect;
		POINT pt;
		switch (message)
		{
		case WM_LBUTTONDBLCLK:
		{
			m_currentTool->OnDblClick(1, shiftkey, (short)LOWORD(lParam), (short)HIWORD(lParam));
			break;
		}
		case WM_MOUSEWHEEL:
		{
			short X = (short)LOWORD(lParam);
			short Y = (short)HIWORD(lParam);
			POINT p; // fixed by jox
			p.x = 0; p.y = 0;
			ClientToScreen(hWnd, &p);
			X -= (short)p.x;
			Y -= (short)p.y;

			m_currentTool->OnMouseWheel(1, HIWORD(wParam), X, Y);

			break;
		}
		case WM_LBUTTONDOWN:
		{
			m_currentTool->OnMouseDown(1, shiftkey, (short)LOWORD(lParam), (short)HIWORD(lParam));
			break;
		}
		case WM_LBUTTONUP:
		{
			m_currentTool->OnMouseUp(1, shiftkey, (short)LOWORD(lParam), (short)HIWORD(lParam));
			break;
		}
		case WM_RBUTTONDOWN:
		{
			m_currentTool->OnMouseDown(2, shiftkey, (short)LOWORD(lParam), (short)HIWORD(lParam));
			break;
		}
		case WM_RBUTTONUP:
		{
			m_currentTool->OnMouseUp(2, shiftkey, (short)LOWORD(lParam), (short)HIWORD(lParam));
			break;
		}
		case WM_MBUTTONDOWN:
		{
			m_currentTool->OnMouseDown(4, shiftkey, (short)LOWORD(lParam), (short)HIWORD(lParam));
			break;
		}
		case WM_MBUTTONUP:
		{
			m_currentTool->OnMouseUp(4, shiftkey, (short)LOWORD(lParam), (short)HIWORD(lParam));
			break;
		}
		case WM_MOUSEMOVE:
		{
			hd::u32 ButtonStates = /*(u32)wParam*/wParam & (MK_LBUTTON | MK_RBUTTON);
			if (wParam & MK_MBUTTON)
				ButtonStates |= irr::EMBSM_MIDDLE;
			pt.x = (short)LOWORD(lParam);
			pt.y = (short)HIWORD(lParam);
			::GetClientRect(hWnd, &rect);
			if (PtInRect(&rect, pt))
			{
				m_currentTool->OnMouseMove(ButtonStates, shiftkey, (short)LOWORD(lParam), (short)HIWORD(lParam));
			}
			break;
		}

		case WM_MOUSELEAVE:
		{
			hd::u32 ButtonStates = /*(u32)wParam*/wParam & (MK_LBUTTON | MK_RBUTTON);
			if (wParam & MK_MBUTTON)
				ButtonStates |= irr::EMBSM_MIDDLE;

			if (m_currentTool->GetID() == COMMAND_QUICK_CAMERA)
			{
				CHdQuickCamera* pQuickCamera = dynamic_cast<CHdQuickCamera*>(m_currentTool);
				if (pQuickCamera && pQuickCamera->GetMouseTrack() == FALSE)
				{
					//OnReloadTitlePanoDataNew();
				}

				IHdView* pHdView = GetViewByName("iScan3DView");

				if (pHdView)
				{
					pHdView->Refresh();
				}
			}

			m_currentTool->OnMouseLeave(ButtonStates, shiftkey, (short)LOWORD(lParam), (short)HIWORD(lParam));

			break;
		}

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			if (message == WM_KEYDOWN)
			{
				m_currentTool->OnKeyDown(wParam, shiftkey);
			}
			else if (message == WM_KEYUP)
			{
				m_currentTool->OnKeyUp(wParam, shiftkey);
			}
			break;
		}
		default:
			break;
		}
	}


	// 在测量工具和全景工具进行切换
	void hnApplication::ChangeMeasureTool(bool bChange)
	{

		CHdTool * pTool = GetCurrentTool();

		if (pTool&&pTool->GetID() == COMMAND_QUICK_PCD_SERV_MEASURE_FOR_SZ&&bChange)
		{
			SetCurrentTool(COMMAND_QUICK_CAMERA);
		}

	}

	// 添加点云
	void hnApplication::addPtCloud(const char* strPcd)
	{
		// 视图中是否存在点云
		bool bExistPcd = isExistPcdInView(HN3D_ABSOLUTE_VIEW);

		// 获得视图
		ISceneView* p3dView = dynamic_cast<ISceneView*>(GetViewByName(HN3D_ABSOLUTE_VIEW));
		if (!p3dView)
		{
			return;
		}

		// 获得路径
		CSeaPointCloud* pSeaPcd = CPointCloudCache::GetCacheInstance()->CacheSeaPcd(strPcd);

		if (!pSeaPcd)
		{
			return;
		}

		// 查找点云
		core::array<ISceneNode*> scnlsit;
		p3dView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT, scnlsit);

		// 判断是否已加载该点云
		for (int i = 0; i < scnlsit.size(); i++)
		{
			CHdSeaDataSceneNode* pSelPcdSN = dynamic_cast<CHdSeaDataSceneNode*>(scnlsit[i]);
			if (pSelPcdSN->GetPointCloud() == pSeaPcd)
			{
				// 控制点云的显隐状态
				if (pSelPcdSN->isVisible())
				{
					pSelPcdSN->setVisible(false);
				}
				else
				{
					pSelPcdSN->setVisible(true);
				}

				// 即时刷新
				p3dView->RefreshViewBySendMessage();
				return;
			}
		}

		// 添加对象sn
		CHdSeaPcdObject pcdObj(pSeaPcd);
		CHdSeaDataSceneNode* pPcdSN = dynamic_cast<CHdSeaDataSceneNode*>(p3dView->AddObject(&pcdObj));

		// 如果指针为空返回
		if (!pPcdSN)
		{
			return;
		}

		// 获取参数
		hnPcdShowInfo pcdShowInfo;
		pcdShowInfo = hnSysSetting::getSetting()->getPcdShowInfo();

		pPcdSN->SetRenderStyle(pcdShowInfo.m_eRenderType);
		pPcdSN->SetPointSize((int)(pcdShowInfo.m_eRenderSize));

		int nUpdateStByPos = 1;

		// 显示距离
		float dist = pcdShowInfo.m_dPcdShowDist;

		// 3D相机浏览阈值
		int i3DCameraThred = pcdShowInfo.m_n3DShowCnt;

		// 快速相机浏览阈值
		int iQuickCameraThred = pcdShowInfo.m_nPanoShowCnt;

		pPcdSN->SetThredIn3DCamera(i3DCameraThred); // 3D相机浏览阈值
		pPcdSN->SetShowDistInQucikCamera(dist);// 快速相机浏览时距离阈值
		pPcdSN->SetThredInQuickCamera(iQuickCameraThred); // 快速相机浏览阈值

														  // 设置是否要自动更新
		pPcdSN->SetAutoFilter(nUpdateStByPos);

		pPcdSN->setVisible(true);

		// 获取当前视图相机
		CHd3DView* v3dView = (CHd3DView*)p3dView;

		// 是否需要截屏
		if (!v3dView->IsViewRenderAllNode())
		{
			v3dView->SetViewRenderAllNode(true);
		}

		if (!bExistPcd)
		{
			// 缩放至全部
			v3dView->ZoomToFullExtent();
		}

		// 即时刷新
		v3dView->RefreshViewBySendMessage();
		v3dView->ReloadData();

		return;
	}

	// 更新点云参数
	void hnApplication::updatePcdInfo(double dDist, int n3DViewCnt, int nPanoViewCnt)
	{
		// 视图中是否存在点云
		bool bExistPcd = isExistPcdInView(HN3D_ABSOLUTE_VIEW);

		// 获得视图
		ISceneView* p3dView = dynamic_cast<ISceneView*>(GetViewByName(HN3D_ABSOLUTE_VIEW));
		if (!p3dView)
		{
			return;
		}

		// 查找点云
		core::array<ISceneNode*> scnlsit;
		p3dView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT, scnlsit);

		// 判断是否已加载该点云
		for (int i = 0; i < scnlsit.size(); i++)
		{
			CHdSeaDataSceneNode* pSelPcdSN = dynamic_cast<CHdSeaDataSceneNode*>(scnlsit[i]);

			if (!pSelPcdSN)
			{
				continue;
			}

			pSelPcdSN->SetThredIn3DCamera(n3DViewCnt); // 3D相机浏览阈值
			pSelPcdSN->SetShowDistInQucikCamera(dDist);// 快速相机浏览时距离阈值
			pSelPcdSN->SetThredInQuickCamera(nPanoViewCnt); // 快速相机浏览阈值
		}

		// 获取当前视图相机
		CHd3DView* v3dView = (CHd3DView*)p3dView;

		// 即时刷新
		v3dView->RefreshViewBySendMessage();
		v3dView->ReloadData();

		return;
	}

	IHdView* hnApplication::getViewByHwnd(const HWND& hwnd)
	{
		IHdView* pFindView = NULL;
		for (vector<IHdView*>::iterator it = m_viewList.begin(); it != m_viewList.end(); it++)
		{
			IHdView* pView = (*it);

			// 比较句柄;
			if (pView->GetHWnd() == hwnd)
			{
				pFindView = pView;
				break;
			}
		}

		return pFindView;
	}

	bool hnApplication::updatePartPcdInView(vector<hnPointXYZIF>& pPartPts, const char* pViewName)
	{
		// 根据传入视图名称获取视图;
		CHd3DView* p3dView = dynamic_cast<CHd3DView*>(GetViewByName(pViewName));
		if (!p3dView || pPartPts.size() <= 0)
		{
			return false;
		}

		// 查找特定点云节点对象;
		core::array<ISceneNode*> scnlsit;
		p3dView->GetSceneManager()->getSceneNodesFromType(ESNT_PART_SCAN_POINT, scnlsit);

		// 检查点云个数;
		bool bExist = false;
		if (scnlsit.size() > 0)
		{
			bExist = true;
		}

		// 点云节点对象存在，则直接更新数据;
		CScanPartPointsSceneNode* pPartSceneNode = NULL;
		if (!bExist)
		{
			// 不存在则创建;
			pPartSceneNode = new hd::scene::CScanPartPointsSceneNode(p3dView->GetSceneManager()->getRootSceneNode(),
				p3dView->GetSceneManager(), -1);
			pPartSceneNode->SetView(p3dView);
			pPartSceneNode->SetDropData(true);
			pPartSceneNode->drop();

			//CBursaWolfModel* viewTransform = p3dView->GetTransModel();
			//if (viewTransform->IsIdentity())
			//{
			//	viewTransform->SetOffset(0.0 - pPartPts[0].x, 0.0 - pPartPts[0].y, 0.0 - pPartPts[0].z);
			//}
		}
		else
		{
			// 遍历查找;
			for (unsigned int n = 0; n < scnlsit.size(); n++)
			{
				CScanPartPointsSceneNode* pPartSN = dynamic_cast<CScanPartPointsSceneNode*>(scnlsit[n]);
				if (!pPartSN)
				{
					continue;
				}

				pPartSceneNode = pPartSN;
				break;
			}
		}

		// 添加数据;
		pPartSceneNode->SetPartPoints(pPartPts, false);

		// 俯视图查看全部;
		p3dView->ZoomToFullExtent();

		p3dView->Refresh();
	}

	// 视图中是否有点云
	bool hnApplication::isExistPcdInView(const char* pViewName)
	{
		// 获得视图
		ISceneView* p3dView = dynamic_cast<ISceneView*>(GetViewByName(pViewName));
		if (!p3dView)
		{
			return false;
		}

		// 查找点云
		core::array<ISceneNode*> scnlsit;
		p3dView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT, scnlsit);

		// 检查点云个数
		if (scnlsit.size() <= 0)
		{
			return false;
		}

		return true;
	}

	// 删除所有节点
	void hnApplication::clearAllNode(const char* pViewName)
	{
		// 获得视图
		CHd3DView* p3dView = dynamic_cast<CHd3DView*>(GetViewByName(pViewName));
		if (!p3dView)
		{
			return;
		}

		// 删除点云
		p3dView->RemoveSceneNodeFromType(ESNT_IPOLYLINE);
		p3dView->RemoveSceneNodeFromType(ESNT_PANO);
		p3dView->RemoveSceneNodeFromType(ESNT_PANO_POINT);
		p3dView->RemoveSceneNodeFromType(ESNT_HD_ROUTEPOINT);
		p3dView->RemoveSceneNodeFromType(ESNT_POLYLINE);
		p3dView->RemoveSceneNodeFromType(ESNT_HD_SEADATA_POINT);
		p3dView->RemoveSceneNodeFromType(ESNT_POINT);

		// 刷新视图
		p3dView->Refresh();
	}

	// 创建影像视图
	hnView* hnApplication::newImageView(QWidget *parent, QString strViewName, VIEW_TYPE nViewType)
	{
		hnView* curView = NULL;

		switch (nViewType)
		{
		case hnCommon::VIEW_2D_CAMERA_TYPE:
		{
			curView = new hn2DCameraView(parent);
			curView->setViewName(strViewName);
			curView->setViewType(nViewType);

			m_vecImageView.push_back(curView);
			break;
		}
		case hnCommon::VIEW_3D_CAMERA_TYPE:
		{
			curView = new hn3DCameraView(parent);
			curView->setViewName(strViewName);
			curView->setViewType(nViewType);

			m_vecImageView.push_back(curView);
			break;
		}
		case hnCommon::VIEW_STREET_CAMERA_TYPE:
		{
			curView = new hnStreetCameraView(parent);
			//curView = new hnShowPixWidget(parent);
			curView->setViewName(strViewName);
			curView->setViewType(nViewType);

			m_vecImageView.push_back(curView);
			break;
		}
		default:
			break;
		}

		return curView;
	}

	//! 根据类型获取影像视图
	hnView* hnApplication::getViewByType(VIEW_TYPE nViewType)
	{
		for (int i = 0; i < m_vecImageView.size(); i++)
		{
			if (m_vecImageView[i]->getViewType() != nViewType)
			{
				continue;
			}

			return m_vecImageView[i];
		}

		return NULL;
	}
}

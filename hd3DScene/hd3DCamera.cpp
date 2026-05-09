#include "StdAfx.h"
#include "hd3DCamera.h"
#include "hdCommandDef.h"
#include <process.h>
#include "hd3DView.h"
#include <vector>
#include <math.h>
#include "..\hdCommon\point_types.h"
#include "..\hdPointCloud\computeNormal.h"
#include "..\hdPointCloud\hdSysSetting.h"
#include "CRoutePointSceneNode.h"
//#include "CIScanSceneNode.h"
#include "HdSeaDataSceneNode.h"
#include "CPanoSceneNode.h"
#include "CScanSceneNode.h"
#include "CDEMSceneNode.h"
#include "CDomSymSceneNode.h"
#include "HdTinSceneNode.h"
#include "CBackGroundSceneNode.h"
#include "..\hdFramework\userMessage.h"
#include "CAnimatorFlyStraight.h"
#include "CAnimatorFlyCamera.h"
#include <time.h>
#include "CIMeshSceneNode.h"
#include "HdDomSceneNode.h"
#include "HdDemSceneNode.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
using namespace irr;
using namespace std;

namespace hd
{
    namespace fm
    {
        //! 记录zoom状态
        bool g_bZooming = false;
        //! 记录mousewhel时的tickCount
        DWORD g_tickCount;
        //! 记录translate状态
        bool g_bTranslating;
        //! 记录Rotate状态
        bool g_bRotating;

        HANDLE g_pZoomThread = NULL;

        CHd3DCamera::CHd3DCamera(void)
            :CHdCamera("CHd3DCamera", "3D浏览工具", "CHd3DCamera", "3D浏览工具", E_HCT_3D)
        {
            m_app = NULL;
            m_id = COMMAND_3D_CAMERA;

            m_meshSN = NULL;
            m_CollisonSN = NULL;
            //m_MousePos = core::position2df(0, 0.5f);
            m_RotX = 0.0f;
            m_RotY = 0.0f;
            m_bRotating = false;
            m_bTranslating = false;
            m_bOperateBox = false;
            m_bAnimator = true;
            m_fMouseRotateMult = 0.35f;
            m_lastMPos.set(-1, -1);
            m_ecmr_pos_type = E_CP_USER_POSITION;
            m_bPanViewByKey = true;
            m_bScenePan = false;
			m_bMouseDown = false;
			m_nCount = 0;
        }

        CHd3DCamera::~CHd3DCamera(void)
        {	
            try
            {
                if (g_pZoomThread)
                {
                    ::TerminateThread(g_pZoomThread,0);
                    g_pZoomThread = NULL;
                }
            }
            catch (...)
            {
            }
        }

        void CHd3DCamera::OnClick()
        {
            if (!GetEnable())
                return;	

            m_checked = true;

            IHdView* pView = m_app->GetActiveView();
            if (!pView)
            {
                return;
            }

            ISceneView* pSceneView = dynamic_cast<ISceneView*>(pView);

            ICameraSceneNode* cam = pSceneView->GetSceneManager()->getActiveCamera();
            if (pSceneView && cam)
            {
                ENUM_HD_3D_PROJECTION_TYPE type = pSceneView->GetViewProjectionType();
                //ENUM_HD_3D_PROJECTION_TYPE type = pSceneView->GetProjectionType();
                cam->setProjectionType(type);
            }
        }

        void CHd3DCamera::OnCreate( CHdApp* app )
        {
            if(app == NULL)
                return;
            m_app = app;

            // 创建缩放后自动恢复抽稀显示线程
            if (g_pZoomThread == NULL)
            {
                g_pZoomThread = ::CreateThread(
                    NULL,0,
                    ZoomOnTimer,app,0,NULL);
            }
        }

        bool CHd3DCamera::GetEnable()
        {
            m_enabled = false;
            IHdView* pAv = m_app->GetActiveView();
            if (m_app && pAv != NULL
                && (pAv->GetViewType() == E_HVT_3D 
                || pAv->GetViewType() == E_HVT_MULTISCAN3D
                || pAv->GetViewType() == E_HVT_SKETCH_ISCAN3D
                || pAv->GetViewType() == E_HVT_MLS3D 
                || pAv->GetViewType() == E_HVT_FACADEEDIT
                || pAv->GetViewType() == E_HVT_ORTHO3D
                || pAv->GetViewType() == E_HVT_3DVIEWMATCH
                || pAv->GetViewType() == E_HVT_REG
                || pAv->GetViewType() == E_HVT_REG_QUICK
                || pAv->GetViewType() == E_HVT_DOM_3D_VIEW
                || pAv->GetViewType() == E_HVT_OVERALL_VIEW))
            {
                m_enabled = true;		
            }
            return m_enabled;
        }

        //! 根据当前视图,重新设置相机目标点,从视图中心往外开始检测点云
        bool CHd3DCamera::SetTargetByView(int srcX,int srcY)
        {
            CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
            if (pView != NULL)
            {
                //// 对于车载多个扫描头与多测站做相同处理 fengjing
                //core::array<ISceneNode*> sceneList;
                //if (strcmp(pView->GetName(), "iScan3DView") == 0)
                //{
                //	ISceneManager* sceneMng = pView->GetSceneManager();
                //	if (sceneMng)
                //	{
                //		sceneMng->getSceneNodesFromType(ESNT_SCAN_POINT, sceneList);
                //	}
                //}

                core::array<ISceneNode*> sceneList;
                f32 tmpX,tmpY,tmpZ;
                f32 x = 0.f,y = 0.f,z = 0.f;
                int tol = 3;
                u64 count = 0;

                int width,heighth;
                width = pView->GetWindowWidth();
                heighth = pView->GetWindowHeight();
                int sumMin = 2 * (heighth + width);

                pView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_DEM, sceneList);
                if (sceneList.size() == 0)
                {
                    pView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_CUTFILL, sceneList);
                }

                if (sceneList.size() > 0)
                {
                    for (u32 i = 0; i < sceneList.size(); i++)
                    {
                        CDEMSceneNode* pDemSn = dynamic_cast<CDEMSceneNode*>(sceneList[i]);						
                        if(pDemSn == NULL)
                        {
                            continue;
                        }

                        ISceneCollisionManager* pCln = pDemSn->getSceneManager()->getSceneCollisionManager();
                        if (!pCln)
                        {
                            return false;
                        }

                        for (int i = -tol; i<= tol;i++)
                        {
                            for (int j = -tol; j<= tol;j++)
                            {
                                if(srcX + i >=0 && srcX + i < width && srcY + j >= 0 && srcY + j < heighth
                                    && pCln->get3DPositionFromScreenPos(srcX + i,srcY + j,tmpX,tmpY,tmpZ))
                                {
                                    // 将opengl捕捉到的三维坐标进行对比，取离屏幕中心最近的点作为返回值
                                    if (abs(i) + abs(j) < sumMin)
                                    {
                                        x = tmpX;
                                        y = tmpY;
                                        z = tmpZ;
                                        sumMin = abs(i) + abs(j);
                                    }
                                    count++;
                                }
                            }
                        }
                    }

                    if (count > 0)
                    {
                        m_rotCenter.X = x;
                        m_rotCenter.Y = y;
                        m_rotCenter.Z = z;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }

                // 获得TINsn
                pView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_TIN, sceneList);
                if (sceneList.size() > 0)
                {
                    for (u32 i = 0; i < sceneList.size(); i++)
                    {
                        CHdTinSceneNode* pTinSn = dynamic_cast<CHdTinSceneNode*>(sceneList[i]);						
                        if(pTinSn == NULL)
                        {
                            continue;
                        }

                        ISceneCollisionManager* pCln = pTinSn->getSceneManager()->getSceneCollisionManager();
                        if (!pCln)
                        {
                            return false;
                        }

                        for (int i = -tol; i<= tol;i++)
                        {
                            for (int j = -tol; j<= tol;j++)
                            {
                                if(srcX + i >=0 && srcX + i < width && srcY + j >= 0 && srcY + j < heighth
                                    && pCln->get3DPositionFromScreenPos(srcX + i,srcY + j,tmpX,tmpY,tmpZ))
                                {
                                    // 将opengl捕捉到的三维坐标进行对比，取离屏幕中心最近的点作为返回值
                                    if (abs(i) + abs(j) < sumMin)
                                    {
                                        x = tmpX;
                                        y = tmpY;
                                        z = tmpZ;
                                        sumMin = abs(i) + abs(j);
                                    }
                                    count++;
                                }
                            }
                        }
                    }

                    if (count > 0)
                    {
                        m_rotCenter.X = x;
                        m_rotCenter.Y = y;
                        m_rotCenter.Z = z;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }

                // 获取海量点云数据节点
                pView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT, sceneList);
                if (sceneList.size() > 0)
                {
                    for (u32 i = 0; i < sceneList.size(); i++)
                    {
                        CHdSeaDataSceneNode* pHdSdSn = dynamic_cast<CHdSeaDataSceneNode*>(sceneList[i]);						
                        if(pHdSdSn == NULL)
                        {
                            continue;
                        }

                        ISceneCollisionManager* pCln = pHdSdSn->getSceneManager()->getSceneCollisionManager();
                        if (!pCln)
                        {
                            return false;
                        }

                        for (int i = -tol; i<= tol;i++)
                        {
                            for (int j = -tol; j<= tol;j++)
                            {
                                if(srcX + i >=0 && srcX + i < width && srcY + j >= 0 && srcY + j < heighth
                                    && pCln->get3DPositionFromScreenPos(srcX + i,srcY + j,tmpX,tmpY,tmpZ))
                                {
                                    // 将opengl捕捉到的三维坐标进行对比，取离屏幕中心最近的点作为返回值
                                    if (abs(i) + abs(j) < sumMin)
                                    {
                                        x = tmpX;
                                        y = tmpY;
                                        z = tmpZ;
                                        sumMin = abs(i) + abs(j);
                                    }
                                    count++;
                                }
                            }
                        }
                    }

                    if (count > 0)
                    {
                        m_rotCenter.X = x;
                        m_rotCenter.Y = y;
                        m_rotCenter.Z = z;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }

                // 获得3ds sn模型
                pView->GetSceneManager()->getSceneNodesFromType(ESNT_MESH_MODEL, sceneList);
                if (sceneList.size() > 0)
                {
                    for (u32 i = 0; i < sceneList.size(); i++)
                    {
                        CIMeshSceneNode* p3dsSn = dynamic_cast<CIMeshSceneNode*>(sceneList[i]);						
                        if(p3dsSn == NULL)
                        {
                            continue;
                        }

                        ISceneCollisionManager* pCln = p3dsSn->getSceneManager()->getSceneCollisionManager();
                        if (!pCln)
                        {
                            return false;
                        }

                        for (int i = -tol; i<= tol;i++)
                        {
                            for (int j = -tol; j<= tol;j++)
                            {
                                if(srcX + i >=0 && srcX + i < width && srcY + j >= 0 && srcY + j < heighth
                                    && pCln->get3DPositionFromScreenPos(srcX + i,srcY + j,tmpX,tmpY,tmpZ))
                                {
                                    // 将opengl捕捉到的三维坐标进行对比，取离屏幕中心最近的点作为返回值
                                    if (abs(i) + abs(j) < sumMin)
                                    {
                                        x = tmpX;
                                        y = tmpY;
                                        z = tmpZ;
                                        sumMin = abs(i) + abs(j);
                                    }
                                    count++;
                                }
                            }
                        }
                    }

                    if (count > 0)
                    {
                        m_rotCenter.X = x;
                        m_rotCenter.Y = y;
                        m_rotCenter.Z = z;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }

                // 通过opengl捕捉方式获取旋转中心坐标
                if(pView->Get3DRenderPosFromScrPos(x,y,z,srcX,srcY))
                {
                    m_rotCenter.X = x;
                    m_rotCenter.Y = y;
                    m_rotCenter.Z = z;

                    return true;
                }

                return false;
            }

            return false;
        }

        void CHd3DCamera::OnMouseDown( int Button, int Shift, int X, int Y )
        {
			m_bMouseDown = true;

            if (!GetEnable())
                return;

            CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
            if (pView == NULL)
            {
                return;
            }

            if (strcmp(pView->GetName(),"SlopeMonitorView") == 0 && Button != 4)
            {
                return;
            }

            irr::scene::ISceneManager* smgr = pView->GetSceneManager();
            if (!smgr)
            {
                return;
            }

            POINT point;

            // 获取光标所在位置
            GetCursorPos(&point); 

            static POINT pointLast;
            pointLast.x = 9999;
            pointLast.y = 9999;

            // 把屏幕坐标转为客户坐标
            ScreenToClient(pView->GetHWnd(), &point );

            RECT rect;

            // 获取视图的客户坐标
            GetClientRect(pView->GetHWnd(), &rect);

            // 如果当前鼠标不在视图中或者和上次鼠标位置一样、则不处理
            if (!PtInRect(&rect,point) )
            {
                return;
            }

            m_btnlftdwn.set(X, Y);
            // 双击时获取当前相机的位置
            irr::scene::ICameraSceneNode* camera = smgr->getActiveCamera();
            core::position2di mousepos(X,Y);
            if (Button == 1)
            {
                m_bRotating = true;
                g_bRotating = m_bRotating;

                unsigned int h = pView->GetWindowHeight();
                unsigned int w = pView->GetWindowWidth();

                //当前正在旋转，抽稀显示点云
                pView->GetSceneManager()->SetAnimateState(true);
            }
            else if (m_bScenePan ? Button == 2 : Button == 4)   // 改中键平移为右键平移 [危迟 2014/08/16]
            {
                m_bTranslating = true;
                g_bTranslating = m_bTranslating;

                unsigned int h = pView->GetWindowHeight();
                unsigned int w = pView->GetWindowWidth();

                m_TranslateStart.X = X / (f32)w;
                m_TranslateStart.Y = Y / (f32)h;

                m_lastMPos.set(X, Y);

                //当前正在平移，抽稀显示点云
                pView->GetSceneManager()->SetAnimateState(true);
            }
        }

        void CHd3DCamera::OnMouseUp( int Button, int Shift, int X, int Y )
        {
			m_bMouseDown = false;

            if (!GetEnable())
                return;
            CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
            //core::array<ISceneNode*> sceneList;
            //int nSNCount = 0;

            if (pView != NULL)
            {
                if (strcmp(pView->GetName(),"SlopeMonitorView") == 0 && Button != 4)
                {
                    return;
                }

                POINT point;

                // 获取光标所在位置
                GetCursorPos(&point); 

                static POINT pointLast;
                pointLast.x = 9999;
                pointLast.y = 9999;

                // 把屏幕坐标转为客户坐标
                ScreenToClient(pView->GetHWnd(), &point );

                RECT rect;

                // 获取视图的客户坐标
                GetClientRect(pView->GetHWnd(), &rect);

                // 如果当前鼠标不在视图中或者和上次鼠标位置一样、则不处理
                if (!PtInRect(&rect,point) )
                {
                    return;
                }

                m_btnlftdwn.set(0,0);
                if (Button == 1)
                {
                    m_bRotating = false;
                    g_bRotating = m_bRotating;
					
                    //旋转结束，返回原来的显示采样
                    pView->GetSceneManager()->SetAnimateState(false);
                    pView->RefreshViewBySendMessage();

                    // 更新范围后重新加载数据
                    // ***测试加载大数据量标志不切换层chy[2015-3-31]***
                    pView->SetChangeLevl(false);
                    pView->ReloadData();
                }
                else if (m_bScenePan ? Button == 2 : Button == 4)  // 改中键平移为右键平移 [危迟 2014/08/16]
                {
                    m_bTranslating = false;
                    g_bTranslating = m_bTranslating;

                    m_lastMPos.set(-1, -1);

                    //平移结束，返回原来的显示采样
                    pView->GetSceneManager()->SetAnimateState(false);
                    pView->RefreshViewBySendMessage();

                    // ***测试加载大数据量标志不切换层chy[2015-3-31]***
                    pView->SetChangeLevl(false);
                    pView->ReloadData();

                    // 动态设置相机目标点
                    int w = pView->GetWindowWidth();
                    int h = pView->GetWindowHeight();
                    // 平移浏览后重新设置旋转中心  gsl-2013/9/13
                    irr::scene::ISceneManager* smgr = pView->GetSceneManager();
                    ICameraSceneNode* camera = smgr->getActiveCamera();

                    if(SetTargetByView(w/2,h/2))
                    {
                        //camera->m_rotCenter = GetRotateCenter();
                        core::vector2di srcCenter(w/2,h/2);
                        core::vector2di rotCenter = smgr->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(GetRotateCenter());
                        if (srcCenter.getDistanceFrom(rotCenter) < 16)
                        {
                            core::vector3df newTarget = GetRotateCenter();
                            core::vector3df offSet = newTarget - camera->getTarget();
                            camera->setTarget(GetRotateCenter());
                            // 透视视图下，改变相机位置，会产生缩放效果 
                            // 限制该操作在正交视图下进行 【2013-10-14 危迟】
                            if (camera->isOrthogonal())
                            {
                                camera->setPosition(camera->getPosition() + offSet);
                            }
                            pView->RefreshViewBySendMessage();
                        }
                    }
                }
            }
        }

        void CHd3DCamera::OnMouseMove( int Button, int Shift, int X, int Y )
        {
            if (!GetEnable())
                return;

            CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
            unsigned int h = pView->GetWindowHeight();
            unsigned int w = pView->GetWindowWidth();

            POINT point;

            // 获取光标所在位置
            GetCursorPos(&point); 

            static POINT pointLast;
            pointLast.x = 9999;
            pointLast.y = 9999;

            // 把屏幕坐标转为客户坐标
            ScreenToClient(pView->GetHWnd(), &point );

            RECT rect;

            // 获取视图的客户坐标
            GetClientRect(pView->GetHWnd(), &rect);

            // 如果当前鼠标不在视图中或者和上次鼠标位置一样、则不处理
            if (!PtInRect(&rect,point) )
            {
                return;
            }

            if (Button == 1 && m_bRotating)
            {	
                irr::scene::ISceneManager* smgr = pView->GetSceneManager();
                ICameraSceneNode* camera = smgr->getActiveCamera();	
                f32 nRotX = m_RotX;
                f32 nRotY = m_RotY;
                // 蔡红云 2013/8/23 新增的鼠标旋转场景的方式
                core::vector2di deltaPos;
                core::vector2di point;
                point.set(X,Y);
                deltaPos = m_btnlftdwn- point;
                RotateCamera(deltaPos);
                m_btnlftdwn.set(X, Y);
                pView->Refresh();
                m_ecmr_pos_type = E_CP_USER_POSITION;

            }
            // 平移模式 中键按住平移 
            // 改中键平移为右键平移 [危迟 2014/08/16]
            else if ((m_bScenePan ? Button == 2 : Button == 4) && m_bTranslating)
            {
                if (abs(m_lastMPos.X - X) + abs(m_lastMPos.Y - Y) < 5)
                {
                    return;
                }
                irr::scene::ISceneManager* smgr = pView->GetSceneManager();
                ICameraSceneNode* camera = smgr->getActiveCamera();

                // 鼠标和POS的射线与近平面的交点
                core::plane3df nearPlane = camera->getViewFrustum()->planes[camera->getViewFrustum()->VF_NEAR_PLANE];
                core::vector3df lastNearIntersection, NearIntersection;
                core::line3df lineNear1 = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(core::vector2di(m_lastMPos.X, m_lastMPos.Y));
                nearPlane.getIntersectionWithLine(lineNear1.start,lineNear1.getVector().normalize(),lastNearIntersection);
                core::line3df lineNear2 = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(core::vector2di(X, Y));
                nearPlane.getIntersectionWithLine(lineNear2.start,lineNear2.getVector().normalize(),NearIntersection);

                core::vector3df vectNear = lastNearIntersection - NearIntersection;
                float lenCameraRay = (camera->getTarget() - camera->getPosition()).getLength();
                core::vector3df newTarget,newPos;

                if (!camera->isOrthogonal())
                {
                    newTarget = camera->getTarget() + (vectNear * lenCameraRay);
                    newPos = camera->getPosition() + (vectNear * lenCameraRay);
                }
                else
                {
                    newTarget = camera->getTarget() + (vectNear);
                    newPos = camera->getPosition() + (vectNear);
                }

                camera->setTarget(newTarget);
                camera->setPosition(newPos);
                m_lastMPos.set(X, Y);

                pView->Refresh();

                m_TranslateStart.X = X / (f32)w;
                m_TranslateStart.Y = Y / (f32)h;
            }

            string strName = pView->GetName();//放大镜视图不要探面效果
            if ( Button == 0 && pView->GetViewType() == E_HVT_3D && 
                CHdSysSetting::getSysSetting()->commonSetting.showNormal == 1
                && strName!="ZoomView")
            {
                ISceneManager* sceneMng = pView->GetSceneManager();

                if (m_meshSN == NULL)
                {
                    m_meshSN = new CMeshPolySceneNode(sceneMng->getRootSceneNode(),sceneMng,0);
                    m_meshSN->drop();
                }
                m_meshSN->SetView(pView);

                core::vector3df scanPos;
                ISceneCollisionManager* pCln = pView->GetSceneManager()->getSceneCollisionManager();
                if(!pCln->get3DPositionFromScreenPos(X,Y,scanPos.X,scanPos.Y,scanPos.Z))
                {
                    m_meshSN->setPoint(core::vector3df(0.0f,0.0f,0.0f),core::vector3df(0.0f,0.0f,0.0f));
                    pView->Refresh();
                    return;
                }

                float x = 0.0f,y = 0.0f,z = 0.0f;
                int pixel = 8;
                int xMin = X - pixel < 0?0:(X - pixel);
                int xMax = X + pixel > pView->GetWindowWidth() ? pView->GetWindowWidth():(X + pixel);
                int yMin = Y - pixel < 0?0:Y - pixel;
                int yMax = Y + pixel > pView->GetWindowHeight() ? pView->GetWindowHeight():(Y + pixel);
                vector<Eigen::Vector3f> pts;
                Eigen::Vector3f pt;
                for (int i = xMin;i<xMax;i++)
                {
                    for (int j = yMin;j<yMax;j++)
                    {
                        if(pCln->get3DPositionFromScreenPos(i,j,pt[0],pt[1],pt[2]))
                        {
                            pts.push_back(pt);
                        }
                    }
                }

                float curvature;
                core::vector3df scanPtNormal;
                computePointNormal(pts,scanPtNormal.X,scanPtNormal.Y,scanPtNormal.Z,curvature);
                pt[0] = scanPos.X;
                pt[1] = scanPos.Y;
                pt[2] = scanPos.Z;
                flipNormalTowardsViewpoint (pt, 0,0,0,
                    scanPtNormal.X,scanPtNormal.Y,scanPtNormal.Z);

                m_meshSN->setPoint(scanPos,scanPtNormal);
                m_meshSN->setVisible(true);
                pView->Refresh();

            }
            else
            {
                if (m_meshSN)
                {
                    m_meshSN->setVisible(false);
                }
            }
        }

        void CHd3DCamera::SetViewAngleX(f32 rotX)
        {
            m_RotX = rotX + 180;

            CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
            if (pView == NULL)
            {
                return;
            }
            ICameraSceneNode* camera = pView->GetSceneManager()->getActiveCamera();
            core::vector3df pos = camera->getPosition();
            core::vector3df target = camera->getTarget();	
            f32 length = (pos - target).getLength();

            // Set pos ------------------------------------
            pos = target;
            pos.X += length;

            pos.rotateXZBy(-m_RotY, target);
            pos.rotateXYBy(m_RotX, target);

            camera->setPosition(pos);

            // Rotation Error ----------------------------
            pos.set(0,0,1);
            pos.rotateXZBy(m_RotY);
            pos.rotateXYBy(m_RotX-180.f);
            camera->setUpVector(pos);

            //pView->SendMsgToWindowsView(WM_PAINT,0,LPARAM(0));
        }

        void CHd3DCamera::SetViewAngleY(f32 rotY)
        {
            m_RotY = rotY;

            CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
            if (pView == NULL)
            {
                return;
            }
            ICameraSceneNode* camera = pView->GetSceneManager()->getActiveCamera();
            core::vector3df pos = camera->getPosition();
            core::vector3df target = camera->getTarget();	
            f32 length = (pos - target).getLength();

            // Set pos ------------------------------------
            pos = target;
            pos.X += length;

            pos.rotateXZBy(-m_RotY, target);
            pos.rotateXYBy(m_RotX, target);

            camera->setPosition(pos);

            // Rotation Error ----------------------------
            pos.set(0,0,1);
            pos.rotateXZBy(m_RotY);
            pos.rotateXYBy(m_RotX-180.f);
            camera->setUpVector(pos);

            // pView->SendMsgToWindowsView(WM_PAINT,0,LPARAM(0));
        }

        void CHd3DCamera::Deactivate()
        {
            m_checked = false;
            m_bRotating = false;
            m_bTranslating = false;	
            // 将相机的视角设为默认视角 fengjing
            m_ecmr_pos_type = E_CP_USER_POSITION;
            if (m_meshSN)
            {
                m_meshSN->remove();
                m_meshSN = NULL;
            }
            if (m_CollisonSN)
            {
                m_CollisonSN->remove();
                m_CollisonSN = NULL;
            }
        }

        // 将键盘相应事件从OnKeyDown移到OnKeyUp 张恒
        void CHd3DCamera::OnKeyDown( int keyCode, int Shift )
        {
			int step =32;
			if (!m_bPanViewByKey)
			{
				return;
			}
			switch(keyCode)
			{
				// A or a left
			case KEY_KEY_A:
			case KEY_LEFT:
				{
					keyType = E_KCTL_RIGHT;
					MoveKeyCtrol(keyType, step);
					break;
				}
				// D or d right
			case KEY_KEY_D:
			case KEY_RIGHT:
				{
					keyType = E_KCTL_LEFT;
					MoveKeyCtrol(keyType, step);
					break;
				}
				// S or s down
			case KEY_KEY_S:
			case KEY_DOWN:
				{
					keyType = E_KCTL_UP;
					MoveKeyCtrol(keyType, step);
					break;
				}
				// W or w up
			case KEY_KEY_W:
			case KEY_UP:
				{
					keyType = E_KCTL_DOWN;
					MoveKeyCtrol(keyType, step);
					break;
				}
			default:
				break;
			}
        }

        void CHd3DCamera::OnKeyUp( int keyCode, int Shift )
        {
            
        }

        bool CHd3DCamera::OnContextMenu( int X, int Y )
        {
            return false;
        }

        void CHd3DCamera::OnDblClick( int Button, int Shift, int X, int Y )
        {
            if(!m_bAnimator)
                return;
            CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
            if (!pView)
            {
                return;
            }
            irr::scene::ISceneManager* smgr = pView->GetSceneManager();
            if (!smgr)
            {
                return;
            }

            // 双击时获取当前相机的位置
            irr::scene::ICameraSceneNode* camera = smgr->getActiveCamera();

            SCamStatus camSts(camera);
            // 将当前的状态添加到view中
            pView->AddCamStatus(camSts);

            // 获取当前双击的鼠标位置与相机的位置的射线
            m_MousePos.X = X;
            m_MousePos.Y = Y;
            core::line3df line = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(m_MousePos);

            core::array</*scene::*/ESCENE_NODE_TYPE> typeList;
            typeList.push_back(ESNT_PANO);
            typeList.push_back(ESNT_HD_ROUTEPOINT);
            typeList.push_back(ESNT_HD_DOM);

            scene::ISceneNode * selectedSceneNode =
                smgr->getSceneCollisionManager()->getSceneNodeFromRayBB(line,typeList,false,smgr->getRootSceneNode());


            // 判断获取到的scenenode的类型
            if (selectedSceneNode)
            {
                // 
                if (selectedSceneNode->getType() == ESNT_PANO && selectedSceneNode->isVisible()/*&& !camera->isOrthogonal()*/)
                {
                    // 获取当前选中的PanoSceneNode的位置
                    core::vector3df newCamPos = selectedSceneNode->getPosition();

                    // 设置panoSceneNode的注册模式为skybox模式
                    CPanoSceneNode* node = dynamic_cast<CPanoSceneNode*>(selectedSceneNode);

                    if (!node->GetRegisterMode())
                    {
                        node->SetRegisterMode(true);
                    }
                    if (node->GetRadius() == 2.0f)
                    {
                        node->SetRadius(10.0f);
                    }

                    // 将当前相机移动到全景球的中心
                    if (!camera->isOrthogonal())
                    {
                        CAnimatorFlyStraight* anm = new CAnimatorFlyStraight(camera,pView,camera->getPosition(),newCamPos,100);
                        //CAnimatorFlyCamera* anm = new CAnimatorFlyCamera(pView,camera->getPosition(),newCamPos,5000);
                        if (anm)
                        {
                            anm->Run();	
                            delete anm;
                            anm = 0;
                        }
                    }

                    if (pView->IsBackGroundVisible())
                    {
                        // 隐藏backgroundscenenode
                        CBackGroundSceneNode* pnode = dynamic_cast<CBackGroundSceneNode*>(smgr->getSceneNodeFromType(ESNT_HD_BACKGROUND));

                        if (pnode)
                        {
                            pnode->setVisible(false);
                        }
                    }

                    // 设置当前工具为快速视图camera
                    m_app->SetCurrentTool(COMMAND_QUICK_CAMERA);

                    // 设置当前视图中的camera对象
                    CHdTool* hdtool = m_app->GetCurrentTool();;
                    CHdCamera* cameraTool = dynamic_cast<CHdCamera*>(hdtool);

                    pView->SetCameraTool(cameraTool);

                    // [2014/6/28 蔡红云 双击进入全景球后，坐标轴隐藏]
                    pView->SetAxisVisiable(false);

                    // 发送消息，按照距离及时间信息渲染点云
                    pView->SendMsgToWindowsView(WM_USER_ISCAN_SCAN_BY_DIS,WPARAM(0),LPARAM(0));

                }
                // 如果当前点击的位置存在轨迹点的scenenode，则将全景球移动到该点，同时替换全景图片
                else if (selectedSceneNode->getType() == ESNT_HD_ROUTEPOINT)
                {
                    // 如果点击到的是轨迹点，则发出消息，将选中scenenode的轨迹点的索引发送
                    CRoutePointSceneNode* node = dynamic_cast<CRoutePointSceneNode*>(selectedSceneNode);

                    if (node)
                    {
                        bool bVaRoute = node->isVaRoute();
                        int nRoutIdx = node->GetIndex();
                        if (!bVaRoute)
                        {
                            //更新全景点
                            pView->SendMsgToWindowsView(WM_USER_REFRESHROUTEPANO,WPARAM(nRoutIdx),LPARAM(0));
                        }
                        else // 如果是va工程，则更新显示dom影像并设置相机等
                        {
                            hd::s32 domScale = node->GetDomScale();

                            pView->SendMsgToWindowsView(WM_USER_ISCAN_REFRESH_ROUTE_DOM,WPARAM(nRoutIdx),LPARAM(domScale));
                        }
                    }
                }
                else if (selectedSceneNode->getType() == ESNT_HD_DOM)
                {
                    pView->SendMsgToWindowsView(WM_USER_ISCAN_DOM_BIGSIZE,WPARAM(selectedSceneNode),LPARAM(0));
                }

            }

            pView->Refresh();
        }

        void CHd3DCamera::OnMouseWheel( UINT nFlags, short zDelta, int X, int Y )
        {
			m_nCount ++;

            IHdView* pIHdView = m_app->GetActiveView();
            if (!pIHdView)
            {
                return;
            }

            POINT point;

            // 获取光标所在位置
            GetCursorPos(&point); 

            static POINT pointLast;
            pointLast.x = 9999;
            pointLast.y = 9999;

            // 把屏幕坐标转为客户坐标
            ScreenToClient(pIHdView->GetHWnd(), &point );

            RECT rect;

            // 获取视图的客户坐标
            GetClientRect(pIHdView->GetHWnd(), &rect);

            // 如果当前鼠标不在视图中或者和上次鼠标位置一样、则不处理
            if (!PtInRect(&rect,point) )
            {
                return;
            }

            irr::f32 wheel = ((irr::f32)(zDelta)) / (irr::f32)WHEEL_DELTA;

            CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());

            if (pView != NULL)
            {
                core::array<ISceneNode*> sceneList;
                pView->GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);
                int nSNCount = sceneList.size();

                // 判断是否含有海量点云
                if (nSNCount == 0)
                {
                    pView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneList);
                    nSNCount = sceneList.size();
                }

                if (nSNCount > 0)
                {
                    //!防止不断缩小，视图飞掉 更改 fengjing 2013-7-18
                    unsigned int height = 0, width = 0;
                    pView->GetScreenRange(height, width);
                    if (height < 64 && width < 64 && wheel < 0)
                    {
                        return;
                    }
                }
				
                //开启定时器
				g_tickCount = ::GetTickCount();
				if (!g_bZooming)
				{
					g_bZooming = true;
					m_MousePos.X = X;
					m_MousePos.Y = Y;

					pView->GetSceneManager()->SetAnimateState(true);
				}	
				
                irr::scene::ISceneManager* smgr = pView->GetSceneManager();
                ICameraSceneNode* camera = smgr->getActiveCamera();

                core::vector3df mIntersection;		// 鼠标和POS的射线与近平面的交点
                core::vector3df tIntersection;		// target和POS的射线与近平面的交点
                core::line3df line = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(m_MousePos);
                camera->getViewFrustum()->planes[camera->getViewFrustum()->VF_NEAR_PLANE].getIntersectionWithLine(line.start, line.getVector().normalize(), mIntersection);

                core::vector3df pos = camera->getPosition();
                core::vector3df target = camera->getTarget();

                camera->getViewFrustum()->planes[camera->getViewFrustum()->VF_NEAR_PLANE].getIntersectionWithLine(pos, (target-pos).normalize(), tIntersection);

                // 缩放比例 = (target 离 pos的length) / (target和POS的射线与近平面的交点 离 pos的length)
                f32 zoomRatio = (tIntersection - pos).getLength() / (target - pos).getLength();
                core::vector3df mtVect,offsetT,mpVect,offsetP,newTarget,newPos;

                if (!camera->isOrthogonal())
                {
                    mtVect = mIntersection - tIntersection;
                    offsetT = mtVect/(wheel>0? 3*zoomRatio:-15*zoomRatio);//translate的偏移量 0.15

                    mpVect = mIntersection - pos;
                    offsetP = mpVect/(wheel>0? 3*zoomRatio:-15*zoomRatio);//pos的偏移量 0.15

                    newTarget = camera->getTarget() + offsetT;
                    newPos = camera->getPosition() + offsetP;

                    const f32 MinDistance = camera->getNearValue();//如同camera->nearValue
                    const f32 MaxDistance = camera->getFarValue();//如同camera->farValue
                    core::vector3df newVect = newPos - newTarget;
                    f32 nlen = newVect.getLength();

                    if (((pos - target).normalize().dotProduct(newVect.normalize()) < 0) ||
                        (nlen < MinDistance) || (nlen > MaxDistance))
                    {
                        return;
                    }

                    // 设置new pos和target
                    camera->setTarget(newTarget);
                    camera->setPosition(newPos);
                }
                else  //相机的前后平移对缩放没有效果，需要改变视景体的宽、高来显示
                {
                    unsigned int h = pView->GetWindowHeight();
                    unsigned int w = pView->GetWindowWidth();	

                    float hRatio = (float)X/h;
                    float wRatio = (float)Y/w;

                    // 获取缩放后的视景体宽、高
                    float ratio = zDelta<0?1.25f:0.75f;

                    float afterWidth = ratio*camera->getWidthofViewVolume();
                    float afterHeight = ratio*camera->getHeightofViewVolume();

                    bool bChange = false;

                    core::vector3df dPosRec = mIntersection;

                    // 加上限制条件，使视景体的宽、高不能太小
                    if (afterWidth > 0.01f && afterHeight >0.01f)
                    {
                        camera->setWidthofViewVolume(afterWidth);
                        camera->setHeightofViewVolume(afterHeight);

                        camera->render();

                        // 相机改变后的位置
                        line = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(m_MousePos);
                        camera->getViewFrustum()->planes[camera->getViewFrustum()->VF_NEAR_PLANE].getIntersectionWithLine(line.start, line.getVector().normalize(), mIntersection);

                        offsetT = mIntersection-dPosRec;

                        core::vector3df newPos = camera->getPosition() -offsetT;
                        core::vector3df newTarget = camera->getTarget()-offsetT;

                        camera->setPosition(newPos);
                        camera->setTarget(newTarget);

                        bChange = true;
                    }

                    //同时移动相机pos位置，解决切换成透视投影时视图出现不一致的问题 lixialiang 2015/04/24
                    //if (bChange)
                    //{
                    //	mpVect = target - pos;
                    //	offsetP = mpVect*(wheel>0? 0.4:-0.2);//pos的偏移量 0.48

                    //	newPos = camera->getPosition() + offsetP;

                    //	const f32 MinDistance = camera->getNearValue();//如同camera->nearValue
                    //	const f32 MaxDistance = camera->getFarValue();//如同camera->farValue
                    //	core::vector3df newVect = newPos - newTarget;
                    //	f32 nlen = newVect.getLength();

                    //	if (((pos - target).normalize().dotProduct(newVect.normalize()) < 0) ||
                    //		(nlen < MinDistance) || (nlen > MaxDistance))
                    //	{
                    //		return;
                    //	}

                    //	// 设置new pos
                    //	camera->setPosition(newPos);
                    //}
                }

				pView->Refresh();

				// 重新加载栅格数据，不采用延迟加载方式（朱立雄 2016-11-9）
				sceneList.clear();
				pView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_DOM, sceneList);
				if (sceneList.size() > 0)
				{
					for (unsigned int i = 0; i < sceneList.size(); i ++)
					{
						CHdDomSceneNode* pDomSN = dynamic_cast<CHdDomSceneNode*>(sceneList[i]);
						if (pDomSN && pDomSN->isVisible())
						{
							pDomSN->ReloadData();
						}
					}
				}

				sceneList.clear();
				pView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_DEM, sceneList);
				if (sceneList.size() > 0)
				{
					for (unsigned int i = 0; i < sceneList.size(); i ++)
					{
						CHdDemSceneNode* pDemSN = dynamic_cast<CHdDemSceneNode*>(sceneList[i]);
						if (pDemSN && pDemSN->isVisible())
						{
							pDemSN->ReloadData();
						}
					}
				}
            }
        }

        //! 得到相机角度
        void CHd3DCamera::GetViewAngle(core::vector3df& angle)
        {
            CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
            if (pView == NULL)
            {
                return;
            }
            ICameraSceneNode* camera = pView->GetSceneManager()->getActiveCamera();
            core::vector3df pos = camera->getPosition();
            core::vector3df target = camera->getTarget();
            core::vector3df viewVector = target - pos;

            //////////////////////////////////////////////////////////////////////////
            //wkl 2012-7-27 10:49:17
            double x,y;
            GetPointAngle(viewVector.X, viewVector.Y, viewVector.Z, x, y);
            angle.X = (f32)x;
            angle.Y = (f32)y;
        }

        void CHd3DCamera::MoveCamera(core::vector3df newPos)
        {
            CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
            if (pView == NULL)
            {
                return;
            }
            irr::core::vector3df nPos(newPos.X,newPos.Y,newPos.Z);
            ICameraSceneNode* camera = pView->GetSceneManager()->getActiveCamera();
            core::vector3df pos = camera->getPosition();
            core::vector3df target = camera->getTarget();

            f32 x = target.X - pos.X + newPos.X;
            f32 y = target.Y - pos.Y + newPos.Y;
            f32 z = target.Z - pos.Z + newPos.Z;
            core::vector3df newtarget(x, y, z);

            camera->setPosition(nPos);
            camera->setTarget(newtarget);

            pView->RefreshViewBySendMessage();
            pView->ReloadData();
        }

        // 2013/8/12 蔡红云键盘控制平移
        void CHd3DCamera::MoveKeyCtrol(ENUM_KEYCTROL_TYPE key_Type, int step)
        {
            CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
            if (pView == NULL)
            {
                return;
            }

            // 获取窗口的大小
            unsigned int h = pView->GetWindowHeight();
            unsigned int w = pView->GetWindowWidth();

            // 获取场景管理器
            irr::scene::ISceneManager* smgr = pView->GetSceneManager();

            // 获取活动相机
            ICameraSceneNode* camera = smgr->getActiveCamera();	
            core::vector3df newTarget;
            core::vector3df newPos;
            core::vector3df vectstep;// 偏移量

            // 屏幕中心点像素位置
            core::vector2di ScereenCenter(w/2, h/2);
            core::vector2di MoveStep(0,0);
            switch(key_Type)
            {
            case E_KCTL_LEFT:// 向左
                {
                    MoveStep.set(w/2-step, h/2);
                    break;
                }
            case E_KCTL_RIGHT:// 向右
                {
                    MoveStep.set(w/2+step, h/2);
                    break;
                }
            case E_KCTL_DOWN: // 向下
                {
                    MoveStep.set(w/2, h/2+step);
                    break;
                }
            case E_KCTL_UP:  // 向上
                {
                    MoveStep.set(w/2, h/2-step);
                    break;
                }
            default:
                break;

            }

            // 构造射线与近平面的交点
            core::plane3df nearPlane = camera->getViewFrustum()->planes[camera->getViewFrustum()->VF_NEAR_PLANE];
            core::vector3df lastNearIntersection, NearIntersection;
            core::line3df lineNear1 = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(ScereenCenter);
            nearPlane.getIntersectionWithLine(lineNear1.start,lineNear1.getVector().normalize(),lastNearIntersection);
            core::line3df lineNear2 = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(MoveStep);
            nearPlane.getIntersectionWithLine(lineNear2.start,lineNear2.getVector().normalize(),NearIntersection);
            core::vector3df vectNear = lastNearIntersection - NearIntersection;
            float lenCameraRay = (camera->getTarget() - camera->getPosition()).getLength();

            // 只操作正射投影
            if (camera->isOrthogonal())
            {
                newTarget = camera->getTarget() + (vectNear);
                newPos = camera->getPosition() + (vectNear);

                // 重新设置相机pos和target
                camera->setTarget(newTarget);
                camera->setPosition(newPos);

                // 刷新视图
                pView->RefreshViewBySendMessage();
                pView->ReloadData();
            } 



        }

        void CHd3DCamera::MoveCamera(core::vector3df newPos,core::vector3df newTar)
        {
            CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
            if (pView == NULL)
            {
                return;
            }
            ICameraSceneNode* camera = pView->GetSceneManager()->getActiveCamera();
            //core::vector3df pos = camera->getPosition();
            //core::vector3df target = camera->getTarget();
            //irr::core::vector3df nPos(newPos.X,newPos.Y,newPos.Z);
            //irr::core::vector3df nTar(newTar.X,newTar.Y,newTar.Z);
            pView->RefreshViewBySendMessage();
            camera->setPosition(newPos);
            camera->setTarget(newTar);
            pView->ReloadData();
        }

        void CHd3DCamera::MoveSceneNode(ISceneNode* node,bool bBack)
        {
            if (!m_pPolyline || m_pPolyline->GetPointCount() < 2) 
            {
                return;
            }
            const CHdSxPoint3D& ptStart = m_pPolyline->GetPoint(0);
            const CHdSxPoint3D& ptEnd = m_pPolyline->GetPoint(1);
            //
            double dx = 0,dy = 0,dz = 0;

            dx = ptEnd.m_x - ptStart.m_x;
            dy = ptEnd.m_y - ptStart.m_y;
            dz = ptEnd.m_z - ptStart.m_z;

            if(fabs(dx) == 0.0f && fabs(dy)==0.0f  && fabs(dz) == 0.0f)
            {
                return;
            }
            IHdView* pView = m_app->GetActiveView();

            if (pView)
            {
                // 第一次移动场景结点时，记下初始位置
                if (m_nPosChangeCount == 0)
                {
                    irr::core::vector3df pos = node->getPosition();
                    m_oriPos = pos;
                }

                if (!bBack)
                {
                    core::vector3df curPos = m_oriPos;
                    curPos.X += (f32)dx;
                    curPos.Y += (f32)dy;
                    curPos.Z += (f32)dz;

                    node->setPosition(curPos);
                    m_nPosChangeCount++;
                }
                else
                {
                    node->setPosition(m_oriPos);
                }
            }

        }

        //! 蔡红云 2013/8/23 旋转相机 
        void CHd3DCamera::RotateCamera(core::vector2di sdvig)
        {
            // 定义相关旋转变量
            f32 rx, ry, rz;
            f32 x, y, z;

            rx = ry = rz = 0;
            // 获取当前活动视图
            CHd3DView* pView = dynamic_cast<CHd3DView*>(m_app->GetActiveView());
            // 为空返回
            if (pView == NULL)
            {
                return;
            }
            // 获取相机节点
            ICameraSceneNode* camera = pView->GetSceneManager()->getActiveCamera();
            // 获取pos关于target之间的X，Y，Z方向的角度
            camera->GetRotationAboutTarget(x, y, z);
            // 计算最新的pos关于target之间的X，Y，Z方向的角度
            rz = z - sdvig.X*m_fMouseRotateMult;
            rx = x + sdvig.Y*m_fMouseRotateMult;
            ry = y;
            // 设置pos关于target之间的X，Y，Z方向的角度
            camera->SetRotationAboutTarget(rx, ry, rz);
        }

        DWORD WINAPI ZoomOnTimer(void* Param)
        {
            while (true)
            {
                if (g_bZooming)
                {
                    //得到当前的view
                    CHdApp* app = (CHdApp*)Param;
                    CHd3DView* p3dView = dynamic_cast<CHd3DView*>(app->GetActiveView());

                    if (p3dView &&(p3dView->GetViewType() == E_HVT_3D || 
                        p3dView->GetViewType() == E_HVT_MULTISCAN3D || 
                        p3dView->GetViewType() == E_HVT_SKETCH_ISCAN3D ||
                        p3dView->GetViewType() == E_HVT_SKETCH_ISCAN3D ||
                        p3dView->GetViewType() == E_HVT_FACADEEDIT ||
                        p3dView->GetViewType() == E_HVT_3DVIEWMATCH ||
                        p3dView->GetViewType() == E_HVT_ORTHO3D))
                    {
                        DWORD curTick = ::GetTickCount();
                        if (curTick - g_tickCount > 20)
                        {
                            //重置zooming为false
                            g_bZooming = false;
                            p3dView->GetSceneManager()->SetAnimateState(false);
                            p3dView->RefreshViewBySendMessage();

                            // ***测试加载大数据量标志切换层chy[2015-3-31]***
                            p3dView->SetChangeLevl(true);
                            
							// 只对点云数据延迟加载（朱立雄 2016-11-9）
							core::array<ISceneNode*> sceneList;
							p3dView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT, sceneList);
							if (sceneList.size() > 0)
							{
								for (unsigned int i = 0; i < sceneList.size(); i ++)
								{
									CHdSeaDataSceneNode* pSeaDataSN = dynamic_cast<CHdSeaDataSceneNode*>(sceneList[i]);
									if (pSeaDataSN && pSeaDataSN->isVisible())
									{
										pSeaDataSN->ReloadData();
									}
								}
							}

							sceneList.clear();
							p3dView->GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT, sceneList);
							if (sceneList.size() > 0)
							{
								for (unsigned int i = 0; i < sceneList.size(); i ++)
								{
									CScanSceneNode* pPointSN = dynamic_cast<CScanSceneNode*>(sceneList[i]);
									if (pPointSN && pPointSN->isVisible())
									{
										pPointSN->ReloadData();
									}
								}
							}
							p3dView->Refresh();
                        }
                    }
                }	
                ::Sleep(20);
            }
        }

    }
}
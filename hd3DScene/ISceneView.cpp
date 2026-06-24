#include "StdAfx.h"
#include "ISceneView.h"
#include "..\hd3DEngine\CIrrDeviceWin32.h"
#include "..\hd3DEngine\CWriteFile.h"
#include "..\hd3DEngine\CImageWriterJPG.h"
#include "..\hdframework\hdview.h"
#include "..\hdCommon\sceneData\HdFileData.h"
#include "..\hdCommon\sceneData\HdSceneScan.h"
#include "..\hdCommon\HDRoutePoint.h"
#include "..\hd3DEngine\include\IReadFile.h"
#include "..\hdPointCloud\hdSysSetting.h"

#include "HdRadioLegendSceneNode.h"
#include "CDEMSceneNode.h"
#include "HdTinSceneNode.h"
#include "CAxisSceneNode.h"
#include "CComPassSceneNode.h"
#include "CFPSSceneNode.h"
#include "CColorLegendSceneNode.h"
#include "CBackGroundSceneNode.h"
#include "CScreenShotSceneNode.h"
#include "IObjectSceneNode.h"
#include "CPanoSceneNode.h"
#include "CScanSceneNode.h"
#include "HdSeaDataSceneNode.h"
//#include "Hd3DPointSceneNode.h"
//#include "CIScanSceneNode.h"
#include "CPlanarSceneNode.h"
#include "hdCamera.h"
#include "CIMeshSceneNode.h"
#include "..\..\hdCommon\hdSceneStr.h"
#include "..\hdHlslib\HdParcelBase.h"
#include "..\hdHlslib\HlzDefs.h"
#include "..\hdCommon\point_types2.h"
#include "CScanPartPointsSceneNode.h"
#include "..\hdFramework\userMessage.h"
#include "CDomSymSceneNode.h"
//#include "CTriangleSceneNode.h"
#include "HdDomSceneNode.h"
#include "HdDemSceneNode.h"
#include "..\hdCore\hdMatrix4.h"
#include "..\hd3DEngine\include\dimension2d.h"
#include "..\hdPointCloud\hdFilterSelectPoly.h"
#include "..\hdPointCloud\hdFilterSelectCircle.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

//ScanNode重新加载线程池线程个数在1-20个之间     袁亮   20160820
/*#define MIN_THREAD_NUM  1
#define MAX_THREAD_NUM  20

//Windwos线程池回调环境     袁亮    20160820
TP_CALLBACK_ENVIRON g_pcbe;
//记录已经reload的ScanNode数目    20160820
long    g_loadCnt = 0;*/

using namespace std;
using namespace irr;
using namespace irr::io;
using namespace irr::scene;
using namespace hd::fm;
using namespace hd::ptcloud;

namespace hd
{
    namespace scene
    {
        ISceneView::ISceneView(void)
            :IHdView(),m_pointSize(2.0f),m_cyclstep(10.0f),m_cyclaxis(0), m_cyclcur(4),m_camera(NULL),m_p3DCamera(NULL),m_bShowAxis(true),m_bShowComPass(true),m_bScanChanged(false),
            m_bShowFPS(false),m_bShowLOD(false),m_bShowCL(false),m_bShowBackGround(false),m_bRenderAll(true),m_bShowCollision(false),
            m_bShowScanModel(false),m_bShowOutBox(false),m_bSimpleRender(true),m_fMinIntenStre(0.15f),m_fMaxIntenStre(0.05f)
        {
            m_bMongoData = false;
            m_lastRefresh = ESCENE_REFRESH_TYPE::HDVIEW_ALL;
            m_eProjType = E_HPT_PERSPECTIVE;
            m_eShowStyle = SHOW_ALL;
            m_reloadEvent = NULL;
            m_reloadThread = NULL;
            /*m_reloadPool = NULL;
            ::InitializeThreadpoolEnvironment(&g_pcbe);*/

            for (int i = 0; i< 256;i++)
            {
                m_bRenderClassIndex[i] = true;
            }
            m_bIsRenderSetting = false;
            //::InitializeCriticalSectionAndSpinCount( &m_cs, 0x80000400 );

            m_bNeedScreenShot = true;

            m_bShowRadioLegend = false;

            m_bZoomToVisiblePcd = false;

            m_nLegend = 0;
            m_bCalReLoad = false;
        }


        ISceneView::~ISceneView(void)
        {
            //RemoveAllSceneNode();//非公共的SceneNode在此处无法delete
            //DeleteCriticalSection(&m_cs);
            if (m_irrDevice != NULL)
            {
                m_irrDevice->drop();
                m_irrDevice = NULL;
            }

            //关闭线程池回调环境，释放线程池资源    袁亮   20150820
            /* ::DestroyThreadpoolEnvironment(&g_pcbe);
            if(m_reloadPool != NULL)
            {
            ::CloseThreadpool(m_reloadPool);
            m_reloadPool = NULL;
            }*/

            if (m_reloadEvent)
            {
                ::CloseHandle(m_reloadEvent);
                m_reloadEvent = NULL;
            }
            BOOL b = FALSE;
            if (m_reloadThread)
            {
                b = ::TerminateThread(m_reloadThread,0);
                ::CloseHandle(m_reloadThread);
                m_reloadThread = NULL;
            }
        }

        void ISceneView::ClearSelection(ISceneNode* node)
        {
            if (node == NULL)
            {
                node = GetSceneManager()->getRootSceneNode();
            }

            if (node->getType() != ESNT_UNKNOWN && node->getType() != ESNT_CAMERA && node->getType() != ESNT_LIGHT)
            {
                IObjectSceneNode* pObjNode = dynamic_cast<IObjectSceneNode*>(node);

                if (pObjNode)
                {
                    pObjNode->SetSelected(false);
                }
            }

            const ISceneNodeList& list = node->getChildren();
            ISceneNodeList::ConstIterator it = list.begin();
            for (; it!=list.end(); ++it)
            {
                ClearSelection(*it);
            }
        }

        void ISceneView::ClearAllSelection()
        {

            ISceneNode* pStart = GetSceneManager()->getRootSceneNode();

            ClearSelection(pStart);

        }

        void ISceneView::InitialView(HWND hwnd)
        {
            IHdView::InitialView(hwnd);

            // 将工程属性C++异常该为SEH异步异常，可以捕捉内存访问错误异常fengjing
            m_irrDevice = InitialIrrDevice(hwnd);
            if (!m_irrDevice)
            {
                return;
            }

            m_irrDevice->setResizable(true);

            //showAxis(m_bShowAxis);

            SetAxisVisiable(m_bShowAxis);
            /*	SetComPassVisiable(m_bShowComPass);*/
            //SetBackGroundVisible(m_bShowBackGround);

            SetFPSVisiable(m_bShowFPS);

            SetLODVisiable(m_bShowLOD);
            //SetColorLegendVisible(m_bShowCL);

            SetRadioLegendVisiable(m_bShowRadioLegend);

            SetCamera();	

            SetViewRenderAllNode(m_bRenderAll);

            // 初始化比例尺
            InitLend();

            if (m_reloadEvent)
            {
                ::CloseHandle(m_reloadEvent);
                m_reloadEvent = NULL;
            }

            if (m_reloadThread)
            {
                ::TerminateThread(m_reloadThread,0);
                ::CloseHandle(m_reloadThread);
                m_reloadThread = NULL;
            }

            /*if (m_reloadPool != NULL)
            {
            ::CloseThreadpool(m_reloadPool);
            m_reloadPool = NULL;
            }*/

            // 创建多个事件不能指定事件名称，如果指定事件名称，
            // 会自动寻找该名称已存在的事件对象，并继承该对象返回新的句柄，
            // 导致一个视图设置事件有信号时，多个视图均会接受到信号而执行，
            // 对于同一份点云文件会造成访问冲突，导致崩溃--add by zhubo 2014.07.23

            m_reloadEvent = ::CreateEvent( 
                NULL,               // default security attributes
                TRUE,               // manual-reset event
                FALSE,              // initial state is nonsignaled
                NULL/*TEXT("ReloadEvent")*/  // object name
                );
            DWORD threadID;

            //ReloadThreadProc pProc = (ReloadThreadProc)&ISceneView::ReloadDataThread;
            m_reloadThread = ::CreateThread(
                NULL,							// default security
                0,								// default stack size
                //*(LPTHREAD_START_ROUTINE*)&pProc,   // name of the thread function
                ISceneView::ReloadDataThread,
                (LPVOID)this,					// no thread parameters
                0,								// default startup flags
                &threadID); 

            //创建线程池，并初始化回调环境    袁亮  20160820
            /*m_reloadPool = ::CreateThreadpool(NULL);
            ::SetThreadpoolThreadMinimum(m_reloadPool, MIN_THREAD_NUM);
            ::SetThreadpoolThreadMaximum(m_reloadPool, MAX_THREAD_NUM);
            ::InitializeThreadpoolEnvironment(&g_pcbe);
            ::SetThreadpoolCallbackPool(&g_pcbe, m_reloadPool);*/
        }

        void ISceneView::InitLend()
        {
            // 由设备DPI获得一英寸对应像素值
            HDC hDc = ::GetDC(m_hWnd);
            int nLen = ::GetDeviceCaps(hDc,LOGPIXELSX); // 屏幕横向方向
            m_nLegend = (int)(nLen / 2.54); // 一英寸对应25.4mm

            // 释放dc
            ReleaseDC(m_hWnd,hDc);

        }

        void ISceneView::SetCamera()
        {
            // 判空
            if (!m_irrDevice)
            {
                return;
            }

            scene::ICameraSceneNode* cam = m_irrDevice->getSceneManager()->addCameraSceneNode();

            //cam->setNearValue(0.1f);
            cam->setFarValue(2000);
            cam->setProjectionType(E_HPT_ORTHOGONAL);
            //m_eProjType = E_HPT_PERSPECTIVE;
            m_eProjType = E_HPT_ORTHOGONAL;
            // 初始记录camera的状态
            if (m_CamVec.size() == 0)
            {
                SCamStatus camSts(cam);

                m_CamVec.push_back(camSts);
            }

            m_lastCameraStatus.SetCamera(cam);
        }

        //! 根据参考点云对象,设置显示坐标转换模型
        void ISceneView::SetTransModel(PointCloud* pPcd)
        {
            m_transModel = pPcd->GetModel();
            __raise OnTransModelChanged(&m_transModel);
        }

        ISceneManager* ISceneView::GetSceneManager()
        {
            if(m_irrDevice == NULL)
                return NULL;
            return m_irrDevice->getSceneManager();
        }



        void ISceneView::SetProjectionType(ENUM_HD_3D_PROJECTION_TYPE projType)
        {
            if(GetProjectionType() == projType)
                return;
            ISceneManager* sceneMng = GetSceneManager();
            if (sceneMng)
            {
                ICameraSceneNode *cam = sceneMng->getActiveCamera();
                if(cam)
                {
                    cam->setProjectionType((u32)projType);
                    //ZoomToFullExtent();
                }
            }
        }


        ENUM_HD_3D_PROJECTION_TYPE ISceneView::GetProjectionType()
        {
            ENUM_HD_3D_PROJECTION_TYPE type = E_HPT_PERSPECTIVE;
            ISceneManager* sceneMng = GetSceneManager();
            if (sceneMng)
            {
                if(sceneMng->getActiveCamera())
                {
                    if (sceneMng->getActiveCamera()->isOrthogonal())
                    {
                        type = E_HPT_ORTHOGONAL;
                    }
                }
            }
            return type;
        }

        IObjectSceneNode* ISceneView::AddObject(const CHDObject* pHdObject)
        {
            if (!pHdObject || !m_irrDevice)
                return NULL;
            IObjectSceneNode* pObjSN = NULL;
            switch(pHdObject->GetType())
            {
            case E_HOT_PANODATA:// 添加内存流全景
                {
                    // 根据条件判断是从本地获取还是从服务器获取的pano sn
                    const CHdPanoData* pPanoData = dynamic_cast<CHdPanoData*>(const_cast<CHDObject*>(pHdObject));
                    if (pPanoData->m_panoData->nSize == 0) // 长度为0表示为切片全景
                    {
                        CPanoSceneNode* panoSceneNode = new CPanoSceneNode(NULL,
                            (f32)pPanoData->m_fColStartAngle, (f32)pPanoData->m_fColEndAngle,
                            (f32)pPanoData->m_fRowStartAngle,  (f32)pPanoData->m_fRowEndAngle,
                            (f32)10.0f, GetSceneManager()->getRootSceneNode(), GetSceneManager(), 99);
                        panoSceneNode->SetView(this);
                        panoSceneNode->SetTileModel(true);
                        panoSceneNode->SetPanoID(pPanoData->m_panoData->strImageID);
                        panoSceneNode->SetDataFrmMogo();
                        panoSceneNode->drop();

                        pObjSN = panoSceneNode;
                    }
                    else
                    {
                        
                        CPanoSceneNode* panoSceneNode = new CPanoSceneNode(
                        (f32)pPanoData->m_fColStartAngle, (f32)pPanoData->m_fColEndAngle,
                        (f32)pPanoData->m_fRowStartAngle,  (f32)pPanoData->m_fRowEndAngle,
                        (f32)10.0f, false, GetSceneManager()->getRootSceneNode(), GetSceneManager(), 99);
                        panoSceneNode->SetView(this);
                        panoSceneNode->SetPanoID(pPanoData->m_panoData->strImageID);
                        panoSceneNode->UpdateTextureScene(pPanoData);
                        panoSceneNode->drop();

                        pObjSN = panoSceneNode;
                        /*
                        video::IVideoDriver* driver = m_irrDevice->getVideoDriver();
                        irr::io::IReadFile* pReadFile = irr::io::createMemoryReadFile(pPanoData->m_panoData->pImageData,
                            pPanoData->m_panoData->nSize,pPanoData->m_panoData->strImageID,false);
                        video::ITexture* pPanoTexture = driver->getTexture(pReadFile);
                        if (pPanoTexture == NULL)
                        {
                            ::MessageBox(GetHWnd(),HDSCENE_IDS_PANO_LOADFAILED_OUTMEMORY,HDSCENE_IDS_ERROR,MB_OK);
                            return NULL;
                        }
                        pReadFile->drop();
                        if (pPanoTexture)
                        {
                            CPanoSceneNode* panoSceneNode = new CPanoSceneNode(pPanoTexture,
                                (f32)pPanoData->m_fColStartAngle, (f32)pPanoData->m_fColEndAngle,
                                (f32)pPanoData->m_fRowStartAngle,  (f32)pPanoData->m_fRowEndAngle,
                                (f32)10.0f, GetSceneManager()->getRootSceneNode(), GetSceneManager(), 99);
                            panoSceneNode->SetView(this);
                            panoSceneNode->SetTileModel(false);
                            panoSceneNode->SetPanoID(pPanoData->m_panoData->strImageID);
                            panoSceneNode->drop();

                            pObjSN = panoSceneNode;
                        }
						*/
                    }
                    break;
                }
            case E_HOT_POINTCLOUD:
                {
                    const CHdPcdObject* pPcdData = dynamic_cast<CHdPcdObject*>(const_cast<CHDObject*>(pHdObject));
                    //if (m_transModel.IsIdentity())
                    // 如果判断测站个数为0,则设置视图模型,gsl-2014/1/8
                    irr::core::array<ISceneNode*> aryList;
                    GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,aryList);
                    if (aryList.size() == 0 && pPcdData && pPcdData->m_pPcd && m_transModel.IsIdentity())
                    {
                        //设置视图的模型参数为第一个点云模型的偏移量 [2014/03/20 危迟]
                        //m_transModel = pPcdData->m_pPcd->GetModel();
                        m_transModel.m_fOffset[0] = -pPcdData->m_pPcd->GetModel().m_fOffset[0];
                        m_transModel.m_fOffset[1] = -pPcdData->m_pPcd->GetModel().m_fOffset[1];
                        m_transModel.m_fOffset[2] = -pPcdData->m_pPcd->GetModel().m_fOffset[2];

                        // 重新计算矩阵
                        m_transModel.Parameter2matrix();
						
						//m_transModel = pPcdData->m_pPcd->GetModel().getAntiModel();
                    }

                    hd::scene::CScanSceneNode* sn = new hd::scene::CScanSceneNode(GetSceneManager()->getRootSceneNode()
                        ,GetSceneManager(),11);
                    sn->SetView(this);
                    if(!sn->SetPointCloud(pPcdData->m_pPcd))
                    {
                        ::MessageBox(GetHWnd(),HDSCENE_IDS_LOADPOINTFAILED_OUTMEMORY,HDSCENE_IDS_ERROR,MB_OK);
                        sn->drop();
                        sn->remove();
                        return NULL;
                    }
                    sn->SetSimpleCount(CHdSysSetting::getSysSetting()->commonSetting.renderSimple);
                    sn->drop();
                    sn->SetPointSize(2.0);

                    pObjSN = sn;

                    m_bScanChanged = true;


                    // 生成相应的扫描仪节点【2014/6/26 蔡红云 注释掉的原因是多线程加载下载后，生成mesh时，内存申请不到】
                    //AddLsModel(pPcdData->m_pPcd);

                    break;
                }

            //case E_HOT_SKETCHTRIANGLE:
            //    {
            //        const CHDTriangleObject* pTriangle = dynamic_cast<CHDTriangleObject*>(const_cast<CHDObject*>(pHdObject));
            //        //if (m_transModel.IsIdentity())
            //        // 如果判断测站个数为0,则设置视图模型,gsl-2014/1/8
            //        /* irr::core::array<ISceneNode*> aryList;
            //        GetSceneManager()->getSceneNodesFromType(ESNT_TRIANGLE, aryList);*/

            //        //if (pTriangle && pTriangle->m_pPcd && m_transModel.IsIdentity())
            //        //{
            //        //    //设置视图的模型参数为第一个点云模型的偏移量 [2014/03/20 危迟]
            //        //    //m_transModel = pTriangle->m_pPcd->GetModel();
            //        //    m_transModel.m_fOffset[0] = -pTriangle->m_pPcd->GetModel().m_fOffset[0];
            //        //    m_transModel.m_fOffset[1] = -pTriangle->m_pPcd->GetModel().m_fOffset[1];
            //        //    m_transModel.m_fOffset[2] = -pTriangle->m_pPcd->GetModel().m_fOffset[2];

            //        //    // 重新计算矩阵
            //        //    m_transModel.Parameter2matrix();
            //        //}

            //        hd::scene::CTriangleSceneNode* sn = new hd::scene::CTriangleSceneNode(GetSceneManager()->getRootSceneNode(),
            //            GetSceneManager(), 11);
            //        sn->SetView(this);
            //        //core::vector3df pt(pTriangle->m_pPcd->GetModel().m_fOffset[0], pTriangle->m_pPcd->GetModel().m_fOffset[1], pTriangle->m_pPcd->GetModel().m_fOffset[2]);
            //        //pTriangle->m_pPcd->GetModel().Translate(pt.X, pt.Y, pt.Z);
            //        //sn->SetTriangleCenterPoint(pt.X, pt.Y, pt.Z);
            //        //sn->SetTriangleCenterPoint(pTriangle->m_pPcd->m_header.offsetX, pTriangle->m_pPcd->m_header.offsetY,
            //        //    pTriangle->m_pPcd->m_header.offsetZ);
            //        //sn->SetOriginalDirection(core::vector3df(pTriangle->m_pPcd->m_header.rotateX,
            //        //    pTriangle->m_pPcd->m_header.rotateY, pTriangle->m_pPcd->m_header.rotateZ));
            //        sn->SetTriangleCenterPoint(
            //            CHdApplication::getAppInstance()->getDocument()->m_pHdSceneScans[pTriangle.m_ScanIndex]->transModel.m_fOffset[0],
            //            CHdApplication::getAppInstance()->getDocument()->m_pHdSceneScans[pTriangle.m_ScanIndex]->transModel.m_fOffset[1],
            //            CHdApplication::getAppInstance()->getDocument()->m_pHdSceneScans[pTriangle.m_ScanIndex]->transModel.m_fOffset[2]);
            //        sn->SetOriginalDirection(core::vector3df(
            //            CHdApplication::getAppInstance()->getDocument()->m_pHdSceneScans[pTriangle.m_ScanIndex]->transModel.m_fAngle[0],
            //            CHdApplication::getAppInstance()->getDocument()->m_pHdSceneScans[pTriangle.m_ScanIndex]->transModel.m_fAngle[1],
            //            CHdApplication::getAppInstance()->getDocument()->m_pHdSceneScans[pTriangle.m_ScanIndex]->transModel.m_fAngle[2]));
            //        sn->SetSize(15);
            //        sn->drop();

            //        pObjSN = sn;

            //        break;
            //    }

            case E_HOT_SEADATA:
                {
                    const CHdSeaPcdObject* pSeaPcdData = dynamic_cast<CHdSeaPcdObject*>(const_cast<CHDObject*>(pHdObject));
                    //if (m_transModel.IsIdentity())
                    // 如果判断测站个数为0,则设置视图模型,gsl-2014/1/8
                    irr::core::array<ISceneNode*> aryList;
                    GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,aryList);

                    // CScanSceneNode 节点个数
                    int sscnt = aryList.size();

                    GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,aryList);

                    // CHdSeaDataSceneNode 节点个数
                    int seacnt = aryList.size();

                    if (sscnt == 0 && seacnt == 0 && pSeaPcdData && pSeaPcdData->m_pSeaPcd&&m_transModel.IsIdentity())
                    {
                        //设置视图的模型参数为第一个点云模型的偏移量 [2014/03/20 危迟]
                        m_transModel.m_fOffset[0] = -pSeaPcdData->m_pSeaPcd->GetModel().m_fOffset[0];
                        m_transModel.m_fOffset[1] = -pSeaPcdData->m_pSeaPcd->GetModel().m_fOffset[1];
                        m_transModel.m_fOffset[2] = -pSeaPcdData->m_pSeaPcd->GetModel().m_fOffset[2];

                        // 重新计算矩阵
                        m_transModel.Parameter2matrix();
						
						//m_transModel = pSeaPcdData->m_pSeaPcd->GetModel().getAntiModel();
                    }

                    hd::scene::CHdSeaDataSceneNode* sn = new hd::scene::CHdSeaDataSceneNode(GetSceneManager()->getRootSceneNode()
                        ,GetSceneManager(),11);
                    sn->SetView(this);
                    if(!sn->SetPointCloud(pSeaPcdData->m_pSeaPcd))
                    {
                        ::MessageBox(GetHWnd(),HDSCENE_IDS_LOADPOINTFAILED_OUTMEMORY,HDSCENE_IDS_ERROR,MB_OK);
                        sn->drop();
                        sn->remove();

                        return NULL;
                    }

                    // Add TOp Data 
                    sn->LoadTopData();

                    sn->drop();
                    sn->SetPointSize(2.0);

                    // 重新统计坐标范围 
                    statAllSeaSndeStatCoord();

                    pObjSN = sn;

                    m_bScanChanged = true;

                    break;

                }
            case ESDT_FILE_COLOR_PIC:
                {
                    const CHdColorPicFileData* pColorPic = dynamic_cast<CHdColorPicFileData*>(const_cast<CHDObject*>(pHdObject));
                    break;
                }
            case ESDT_FILE_GREY_PIC:
                {
                    const CHdGreyPicFileData* pGreyPic = dynamic_cast<CHdGreyPicFileData*>(const_cast<CHDObject*>(pHdObject));
                    video::IVideoDriver* driver = m_irrDevice->getVideoDriver();
                    video::ITexture* pGreyTexture = driver->getTexture(pGreyPic->m_strFile.data());
                    if (pGreyTexture == NULL)
                    {
                        ::MessageBox(GetHWnd(),HDSCENE_IDS_LOADPOINTFAILED_OUTMEMORY,HDSCENE_IDS_ERROR,MB_OK);
                        return NULL;
                    }
                    if (pGreyTexture)
                    {
                        CPanoSceneNode* panoSceneNode = new CPanoSceneNode(pGreyTexture,
                            pGreyPic->m_fColStartAngle, pGreyPic->m_fColEndAngle,
                            pGreyPic->m_fRowStartAngle,  pGreyPic->m_fRowEndAngle,
                            10.0f, GetSceneManager()->getRootSceneNode(), GetSceneManager(), 99);
                        panoSceneNode->SetView(this);
                        panoSceneNode->drop();

                        if (m_camera)
                        {
                            m_camera->SetZoomSpeed(100.0f);
                            m_camera->SetRotateSpeed(-300.0f);
                            m_camera->SetMoveSpeed(50.0f);
                        }
                        pObjSN = panoSceneNode;
                    }
                }
                break;
            case ESDT_FILE_PANO_PIC:
                {
                    const CHdPanoPicFileData* pPanoPic = dynamic_cast<CHdPanoPicFileData*>(const_cast<CHDObject*>(pHdObject));
                    video::IVideoDriver* driver = m_irrDevice->getVideoDriver();
                    video::ITexture* pPanoTexture = driver->getTexture(pPanoPic->m_strFile.data());
                    if (pPanoTexture == NULL)
                    {
                        ::MessageBox(GetHWnd(),HDSCENE_IDS_LOADPOINTFAILED_OUTMEMORY,HDSCENE_IDS_ERROR,MB_OK);
                        return NULL;
                    }
                    if (pPanoTexture)
                    {			
                        CPanoSceneNode* panoSceneNode = new CPanoSceneNode(pPanoTexture,
                            (f32)pPanoPic->m_fColStartAngle, (f32)pPanoPic->m_fColEndAngle,
                            (f32)pPanoPic->m_fRowStartAngle,  (f32)pPanoPic->m_fRowEndAngle,
                            10.0f, GetSceneManager()->getRootSceneNode(), GetSceneManager(), 99);
                        panoSceneNode->SetView(this);
                        panoSceneNode->drop();

                        if (m_camera)
                        {
                            m_camera->SetZoomSpeed(100.0f);
                            m_camera->SetRotateSpeed(-300.0f);
                            m_camera->SetMoveSpeed(50.0f);
                        }
                        pObjSN = panoSceneNode;
                    }
                }
                break;	
                //case ESDT_OBJECT_POINT: // hdPtVectorArcGIS三维窗口点场景结点显示
                //	{
                //		const CHdSxPoint3D* pPoint = dynamic_cast<CHdSxPoint3D*>(const_cast<CHDObject*>(pHdObject));
                //		CHd3DPointSceneNode* pPtSN = new CHd3DPointSceneNode(pPoint, EPS_CROSS,
                //			GetSceneManager()->getRootSceneNode(), GetSceneManager(), -1);
                //		pPtSN->SetView(this);

                //		pPtSN->drop();
                //		return pPtSN;
                //	}
                //	break;
            default:
                break;
            }

            // 触发添加事件
            __raise OnAddedSceneNode(pObjSN);

            return pObjSN;
        }

        void ISceneView::AddObject(const std::vector<CHDObject*> dataList)
        {
            for (unsigned int i = 0;i< dataList.size();i++)
            {
                AddObject(dataList[i]);
            }
        }

        void ISceneView::AddCamStatus(SCamStatus camSts)
        {
            m_CamVec.push_back(camSts);
        }

        SCamStatus ISceneView::GetLatestCameraStatus()
        {
            unsigned int count = m_CamVec.size();
            SCamStatus sts;
            if (count > 0)
            {
                sts = m_CamVec[count - 1];
            }
            return sts;
        }

        irr::f32 ISceneView::GetScanPointSize()
        {
            core::array<ISceneNode*> sceneList;
            core::array<ISceneNode*> sceneListSea; // 海量点云节点
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneListSea);

            if (sceneList.empty())
            {
                // chy 剖面视图点云粒子大小设置
                GetSceneManager()->getSceneNodesFromType(ESNT_CLASSIFY_PTD, sceneList);
                if (sceneList.empty())
                {
                    if (sceneListSea.empty())
                    {
                        return 0.0;
                    }
                }

            }
            for(unsigned int i = 0;i< sceneList.size();i++)
            {
                CScanSceneNode* scanNode = dynamic_cast<CScanSceneNode*>(sceneList[i]);
                if(scanNode)
                {
                    // 以第一个点点云大小为准
                    m_pointSize = scanNode->GetPointSize();
                    break;
                }
            }

            for(unsigned int i = 0;i< sceneListSea.size();i++)
            {
                CHdSeaDataSceneNode* scanNode = dynamic_cast<CHdSeaDataSceneNode*>(sceneListSea[i]);
                if(scanNode)
                {
                    // 以第一个点点云大小为准
                    m_pointSize = scanNode->GetPointSize();
                    break;
                }
            }
            return m_pointSize;
        }

        void ISceneView::SetScanPointSize(float fSize)
        {
            core::array<ISceneNode*> sceneList;
            core::array<ISceneNode*> sceneListSea; // 海量点云节点
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneListSea);

            if (sceneList.empty())
            {
                GetSceneManager()->getSceneNodesFromType(ESNT_CLASSIFY_PTD, sceneList);
                if (sceneList.empty())
                {
                    if (sceneListSea.empty())
                    {
                        return;
                    }
                }


            }
            for(unsigned int i = 0;i< sceneListSea.size();i++)
            {
                CHdSeaDataSceneNode* scanNode = dynamic_cast<CHdSeaDataSceneNode*>(sceneListSea[i]);
                if(scanNode)
                {
                    // 2013/11/12 蔡红云 为和工具栏控制点尺寸保持一致
                    // 修改尺寸大小和相对应按钮ID
                    scanNode->SetPointSize(fSize);
                    if (fSize == 1.0)
                    {
                        scanNode->SetPointSizeBtn(1);
                    }
                    else if(fSize == 2.0)
                    {
                        scanNode->SetPointSizeBtn(2);
                    }
                    else if (fSize == 3.0)
                    {
                        scanNode->SetPointSizeBtn(3);
                    }
                    // 自设尺寸，4.0及以上
                    else
                        scanNode->SetPointSizeBtn(4);
                }
            }

            for(unsigned int i = 0;i< sceneList.size();i++)
            {
                CScanSceneNode* scanNode = dynamic_cast<CScanSceneNode*>(sceneList[i]);
                if(scanNode)
                {
                    // 2013/11/12 蔡红云 为和工具栏控制点尺寸保持一致
                    // 修改尺寸大小和相对应按钮ID
                    scanNode->SetPointSize(fSize);
                    if (fSize == 1.0)
                    {
                        scanNode->SetPointSizeBtn(1);
                    }
                    else if(fSize == 2.0)
                    {
                        scanNode->SetPointSizeBtn(2);
                    }
                    else if (fSize == 3.0)
                    {
                        scanNode->SetPointSizeBtn(3);
                    }
                    // 自设尺寸，4.0及以上
                    else
                        scanNode->SetPointSizeBtn(4);
                }
            }


            m_pointSize = fSize;
        }

        void ISceneView::SetScanRenderStyle(ENUM_RENDERSTYLE style)
        {
            core::array<ISceneNode*> sceneList;
            core::array<ISceneNode*> sceneListSea;// 海量点云节点

            // 3D视图或者多测站3D视图 
            if (m_viewType == E_HVT_3D || m_viewType == E_HVT_MULTISCAN3D || m_viewType == E_HVT_SKETCH_ISCAN3D ||
                m_viewType == E_HVT_QUICK || m_viewType == E_HVT_3DVIEWMATCH || 
                m_viewType == E_HVT_ORTHO3D || m_viewType == E_HVT_REG || m_viewType == E_HVT_DOM_3D_VIEW || m_viewType == E_HVT_REG_QUICK)
            {
                GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);
                GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneListSea);

                if (sceneList.empty())
                {
                    GetSceneManager()->getSceneNodesFromType(ESNT_CLASSIFY_PTD,sceneList);

                    if (sceneList.empty())
                    {
                        GetSceneManager()->getSceneNodesFromType(ESNT_HD_SKETCH_PCD,sceneList);

                        if (sceneList.empty())
                        {
                            if (sceneListSea.empty())
                            {
                                return;
                            }
                        }
                    }

                }

                for(unsigned int i = 0;i< sceneList.size();i++)
                {
                    CScanSceneNode* scanNode = dynamic_cast<CScanSceneNode*>(sceneList[i]);
                    if(scanNode /*&& scanNode->isVisible()*/)
                    {
                        scanNode->SetRenderStyle(style);

                        //// 如果显示选择点云时，设置区域渲染同步【蔡红云 2014/7/24】 
                        //if (m_eShowStyle == SHOW_SELECT)
                        //{

                        //	if (style== RENDER_BY_Z  )
                        //	{
                        //		scanNode->SetAreaRenderStyle(RENDER_AREA_BY_Z);
                        //	}
                        //	if (style ==  RENDER_BY_Y)
                        //	{
                        //		scanNode->SetAreaRenderStyle(RENDER_AREA_BY_Y);
                        //	}
                        //	if (style ==  RENDER_BY_X)
                        //	{
                        //		scanNode->SetAreaRenderStyle(RENDER_AREA_BY_X);
                        //	}
                        //		
                        //}

                    }
                }

                // 设置海量点云的渲染方式
                for(unsigned int i = 0;i< sceneListSea.size();i++)
                {
                    CHdSeaDataSceneNode* pScanNode = dynamic_cast<CHdSeaDataSceneNode*>(sceneListSea[i]);
                    if(pScanNode /*&& scanNode->isVisible()*/)
                    {
                        pScanNode->SetRenderStyle(style);

                    }
                }

            }


        }

        ENUM_RENDERSTYLE ISceneView::GetScanRenderStyle()
        {
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

            unsigned int i = 0;
            unsigned int nSNCount = sceneList.size();
            if(nSNCount == 0)
            {
                GetSceneManager()->getSceneNodesFromType(ESNT_CLASSIFY_PTD,sceneList);
                nSNCount = sceneList.size();
            }
            for (i = 0; i<nSNCount; i++)
            {
                CScanSceneNode* pScanSn = (CScanSceneNode*)sceneList[i];
                if (pScanSn/* && pScanSn->isVisible()*/)
                {
                    return pScanSn->GetRenderStyle();
                }
            }

            if (nSNCount == 0)
            {
                GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneList);

                nSNCount = sceneList.size();

                for (i = 0; i<nSNCount; i++)
                {
                    CHdSeaDataSceneNode* pScanSn = (CHdSeaDataSceneNode*)sceneList[i];
                    if (pScanSn/* && pScanSn->isVisible()*/)
                    {
                        return pScanSn->GetRenderStyle();
                    }
                }
            }

            return RENDER_BY_DEFAULT;
        }

        void ISceneView::SetShowStyle( ENUM_SHOWSTYLE style )
        {	
            core::array<ISceneNode*> sceneList;

            // 常规点云节点
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

            for(unsigned int i = 0;i< sceneList.size();i++)
            {
                CScanSceneNode* scanNode = dynamic_cast<CScanSceneNode*>(sceneList[i]);
                if(scanNode)
                {
                    scanNode->SetShowStyle(style);
                }
            }

            // 海量点云节点
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneList);

            for(unsigned int i = 0;i< sceneList.size();i++)
            {
                CHdSeaDataSceneNode* scanNode = dynamic_cast<CHdSeaDataSceneNode*>(sceneList[i]);
                if(scanNode)
                {
                    scanNode->SetShowStyle(style);
                }
            }

        }

        ENUM_SHOWSTYLE ISceneView::GetShowStyle()
        {
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

            unsigned int i = 0;
            unsigned int nSNCount = sceneList.size();
            for (i = 0; i<nSNCount; i++)
            {
                if (sceneList[i]->getType() == ESNT_SCAN_POINT)
                {
                    return ((CScanSceneNode*)(sceneList[i]))->GetShowStyle();
                }
            }

            // 海量点云节点
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneList);

            nSNCount = sceneList.size();
            for (i = 0; i<nSNCount; i++)
            {
                if (sceneList[i]->getType() == ESNT_HD_SEADATA_POINT)
                {
                    return ((CHdSeaDataSceneNode*)(sceneList[i]))->GetShowStyle();
                }
            }

            return SHOW_ALL;
        }

        void ISceneView::SetPointCloudDefaultColor(SColorf color)
        {
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

            for(unsigned int i = 0;i< sceneList.size();i++)
            {
                CScanSceneNode* scanNode = dynamic_cast<CScanSceneNode*>(sceneList[i]);
                if(scanNode)
                {
                    scanNode->SetDefaultColor(color);
                }
            }
        }

        SColorf ISceneView::GetPointCloudDefaultColor()
        {
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

            for(unsigned int i = 0;i< sceneList.size();i++)
            {
                CScanSceneNode* scanNode = dynamic_cast<CScanSceneNode*>(sceneList[i]);
                if(scanNode)
                {
                    return scanNode->GetDefaultColor();
                }
            }

            return SColor(0,0,0,0);
        }

        IObjectSceneNode* ISceneView::GetObjSceneNode(const CHDObject* pHdData,ISceneNode* pStart)
        {
            if (pStart == NULL)
            {
                pStart = GetSceneManager()->getRootSceneNode();
            }
            if (pStart->getType() != ESNT_UNKNOWN && pStart->getType() != ESNT_CAMERA && pStart->getType() != ESNT_LIGHT)
            {
                IObjectSceneNode* pObjNode = dynamic_cast<IObjectSceneNode*>(pStart);

                if (pObjNode)
                {
                    if (pObjNode->GetHdData() == pHdData)
                        return pObjNode;
                }
            }

            IObjectSceneNode* node = 0;

            const ISceneNodeList& list = pStart->getChildren();
            ISceneNodeList::ConstIterator it = list.begin();
            for (; it!=list.end(); ++it)
            {
                node = GetObjSceneNode(pHdData, *it);
                if (node)
                    return node;
            }

            return NULL;
        }

        IObjectSceneNode* ISceneView::GetObjSceneNode(const CHDObject* pHdData)
        {
            return GetObjSceneNode(pHdData,GetSceneManager()->getRootSceneNode());
        }

        //ScanNode回调加载函数，将被Windows线程池中线程调用      袁亮   20160820
        /*VOID CALLBACK ISceneView::ReloadSNCallback(PTP_CALLBACK_INSTANCE pInstance, PVOID pContext)
        {
        if(pContext != NULL)
        {
        irr::scene::ISceneNode* pSN = (irr::scene::ISceneNode*)pContext;
        if (pSN->getType() == ESNT_HD_DEM || pSN->getType() == ESNT_HD_CUTFILL)
        {
        CDEMSceneNode* pDemSn = dynamic_cast<CDEMSceneNode*>(pSN);
        if (pDemSn != NULL)
        {
        pDemSn->ReloadData();
        }
        }
        else if (pSN->getType() == ESNT_HD_TIN)
        {
        CHdTinSceneNode* pTinSn = dynamic_cast<CHdTinSceneNode*>(pSN);
        if (pTinSn != NULL)
        {
        pTinSn->ReloadData();
        }
        }

        else if (pSN->getType() == ESNT_HD_SEADATA_POINT)
        {
        CHdSeaDataSceneNode* pHdSdSn = dynamic_cast<CHdSeaDataSceneNode*>(pSN);
        if (pHdSdSn != NULL && pHdSdSn->isVisible())
        {
        pHdSdSn->ReloadData();
        }
        }
        else
        {
        CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(pSN);
        if(pScanSn != NULL && pScanSn->GetPointCloud() != NULL && pScanSn->GetPointCloud()->count() > 0)
        {
        pScanSn->ReloadData();
        }
        }
        }
        //完成数+1
        ::InterlockedIncrement(&g_loadCnt);
        }*/

        //! 重新加载点云数据
        void ISceneView::ReloadData()
        {
			core::array<ISceneNode*> sceneList;
			GetSceneManager()->getSceneNodesFromType(ESNT_HD_DOM, sceneList);
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
			GetSceneManager()->getSceneNodesFromType(ESNT_HD_DEM, sceneList);
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

			sceneList.clear();
			GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT, sceneList);
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
			GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT, sceneList);
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
			
			Refresh();

            //EnterCriticalSection(&m_cs);
            //::SetEvent(m_reloadEvent);
            //LeaveCriticalSection(&m_cs);

            /*core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);
            int nSNCount = sceneList.size();
            if (nSNCount > 0)
            {
            for (int i = 0; i<nSNCount; i++)
            {
            CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);
            if(pScanSn)
            pScanSn->ReloadData();
            }
            }
            Refresh();*/
        }

        // !部分刷新截屏
        void ISceneView::CreateScreenShot()
        {
            //if (type != ESCENE_REFRESH_TYPE::HDVIEW_ALL)
            {
                video::IVideoDriver* driver = m_irrDevice->getVideoDriver();
                video::IImage* image = NULL;
                CScreenShotSceneNode* pscsNode = dynamic_cast<CScreenShotSceneNode*>(GetSceneManager()->getSceneNodeFromType(ESNT_HD_SCREENSHOT));
                SViewFrustum curFrustum = *(GetSceneManager()->getActiveCamera()->getViewFrustum());

                //if (pscsNode == NULL )//|| (m_lastRefresh != type)
                {
                    //|| !(curFrustum.cameraPosition == pscsNode->GetViewFrustum().cameraPosition &&
                    //	curFrustum.boundingBox == pscsNode->GetViewFrustum().boundingBox)
                    // 范围改变或未截屏情况下,创建截屏
                    //GetSceneManager()->SetRefreshType((ESCENE_REFRESH_TYPE)~type);
                    //::SendMessage(m_hWnd,WM_PAINT,0,0);
                    //image = driver->createScreenShot();

                    /*IImageWriter* imgWrite = new CImageWriterJPG();
                    IWriteFile* imgFile = createWriteFile("d:\\createScreenShot.jpg",false);
                    imgWrite->writeImage(imgFile,image);
                    imgFile->drop();
                    imgWrite->drop();*/

                    //GetSceneManager()->SetRefreshType(type);
                }

                if (pscsNode == NULL && image)
                {						
                    video::ITexture* pTexture = driver->addTexture("CurrentScreenShot",image);
                    // 添加纹理失败，造成截屏SN失败返回
                    if (pTexture)
                    {	
                        pscsNode = new CScreenShotSceneNode(m_irrDevice->getSceneManager()->getRootSceneNode(),m_irrDevice->getSceneManager(),201,pTexture);
                        pscsNode->SetView(this);
                        pscsNode->drop();
                    }
                }
                else if(pscsNode != NULL && image)
                {
                    video::ITexture* pTexture = driver->addTexture("CurrentScreenShot",image);
                    pscsNode->UpdateTexture(pTexture);
                }

                if (image)
                {
                    image->drop();
                }
            }
        }

        void ISceneView::Refresh()
        {
            if (m_hWnd)
            {	
                GetSceneManager()->SetRefreshType(ESCENE_REFRESH_TYPE::HDVIEW_ALL);
                // 检查相机是否变化,如果变化就要激发事件
                scene::ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
                SCamStatus camSts(cam);
                if (camSts != m_lastCameraStatus)
                {
                    __raise OnCameraChanged(cam);
                    m_lastCameraStatus = camSts;
                }

                // InvalidateRect会产生WM_PAINT消息，排在消息队列中
                // 等到调用刷新函数的函数返回时，才进入消息循环，不满足即时刷新的要求 [2013/04/15 危迟] 
                ::InvalidateRect(m_hWnd,NULL,FALSE);// 第三个参数设为FALSE，不进行背景擦除，可以避免C#程序中全景控件的闪烁问题 wkl 2013-4-18

                //::PostMessage(m_hWnd, WM_PAINT, 0, 0);
                //SendMessage会导致关闭平面/快速后其他平面/快速视图变白 
                //wkl 2012-8-1 11:01:40

                // 触发视图异步刷新事件
                __raise IHdView::OnRefreshEvent();
            }
        }

        //! 部分刷新
        void ISceneView::RefreshPartial(ESCENE_REFRESH_TYPE type)
        {
            if (m_hWnd)
            {	
                //if (type != HDVIEW_ALL && m_lastRefresh != type)
                //{
                //	GetSceneManager()->SetRefreshType((ESCENE_REFRESH_TYPE)~type);//
                //	// 第三个参数设为FALSE，不进行背景擦除，可以避免C#程序中全景控件的闪烁问题 wkl 2013-4-18
                //	::InvalidateRect(m_hWnd,NULL,FALSE);
                //}
                GetSceneManager()->SetRefreshType(type);
                //CreateScreenShot(type);

                // InvalidateRect会产生WM_PAINT消息，排在消息队列中
                // 等到调用刷新函数的函数返回时，才进入消息循环，不满足即时刷新的要求 [2013/04/15 危迟] 

                ::InvalidateRect(m_hWnd,NULL,FALSE);

                // 检查相机是否变化,如果变化就要激发事件
                scene::ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
                SCamStatus camSts(cam);
                if (camSts != m_lastCameraStatus)
                {
                    __raise OnCameraChanged(cam);
                    m_lastCameraStatus = camSts;
                }
                // 触发视图异步刷新事件
                __raise IHdView::OnRefreshEvent();
            }
        }

        void ISceneView::RefreshViewBySendMessage()
        {
            // 即时刷新
            if (m_hWnd)
            {
                GetSceneManager()->SetRefreshType(ESCENE_REFRESH_TYPE::HDVIEW_ALL);
                // 检查相机是否变化,如果变化就要激发事件
                scene::ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
                SCamStatus camSts(cam);
                if (camSts != m_lastCameraStatus)
                {
                    __raise OnCameraChanged(cam);
                    m_lastCameraStatus = camSts;
                }

                ::SendMessage(m_hWnd,WM_PAINT,0,0);
                //::PostMessage(m_hWnd, WM_PAINT,0,0);

                // 触发视图异步刷新事件
                __raise IHdView::OnRefreshSendEvent();
            }
        }

        void ISceneView::OnSize(int cx, int cy)
        {
            IHdView::OnSize(cx,cy);
        }

        void ISceneView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
        {

            if(m_irrDevice == NULL)
                return ;
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

            irr::CIrrDeviceWin32* dev = (irr::CIrrDeviceWin32*)m_irrDevice;
            irr::SEvent event;

            static irr::s32 ClickCount=0;
            if (GetCapture() != m_hWnd && ClickCount > 0)
                ClickCount = 0;


            struct messageMap
            {
                irr::s32 group;
                UINT winMessage;
                irr::s32 irrMessage;
            };

            static messageMap mouseMap[] =
            {
                {0, WM_LBUTTONDOWN, irr::EMIE_LMOUSE_PRESSED_DOWN},
                {1, WM_LBUTTONUP,   irr::EMIE_LMOUSE_LEFT_UP},
                {0, WM_RBUTTONDOWN, irr::EMIE_RMOUSE_PRESSED_DOWN},
                {1, WM_RBUTTONUP,   irr::EMIE_RMOUSE_LEFT_UP},
                {0, WM_MBUTTONDOWN, irr::EMIE_MMOUSE_PRESSED_DOWN},
                {1, WM_MBUTTONUP,   irr::EMIE_MMOUSE_LEFT_UP},
                {2, WM_MOUSEMOVE,   irr::EMIE_MOUSE_MOVED},
                {3, WM_MOUSEWHEEL,  irr::EMIE_MOUSE_WHEEL},
                {-1, 0, 0}
            };

            // handle grouped events
            messageMap * m = mouseMap;
            while ( m->group >=0 && m->winMessage != message )
                m += 1;

            switch (message)
            {
            case WM_PAINT:
                {
                    /*CMainFrame* pMainWnd = (CMainFrame*)(AfxGetMainWnd());
                    if (pMainWnd)
                    {

                    }*/
                    //if (m_irrDevice != NULL && m_bSizeChanged)
                    {
                        const core::dimension2d<u32> screensize = core::dimension2d<u32>(m_width,m_height);
                        m_irrDevice->getVideoDriver()->OnResize(screensize);
                        if (m_width != 0)// && m_irrDevice->getSceneManager()->getActiveCamera()->gp
                        {
                            if (GetProjectionType() == E_HPT_ORTHOGONAL)
                            {
                                // 告诉正交投影长宽
                                float fRatio = (f32)(m_width)/m_height;
                                float fCurVVWidth = m_irrDevice->getSceneManager()->getActiveCamera()->getWidthofViewVolume();
                                float fCurVVHeight = m_irrDevice->getSceneManager()->getActiveCamera()->getHeightofViewVolume();
                                float fVVHeight = fCurVVWidth/fRatio;
                                float fVVWidth = fCurVVHeight*fRatio;
                                if (fVVHeight >= fCurVVHeight)
                                {
                                    fCurVVHeight = fVVHeight;
                                }
                                else if (fVVWidth >= fCurVVWidth)
                                {
                                    fCurVVWidth = fVVWidth;
                                }
                                m_irrDevice->getSceneManager()->getActiveCamera()->setWidthofViewVolume(fCurVVWidth);
                                m_irrDevice->getSceneManager()->getActiveCamera()->setHeightofViewVolume(fCurVVHeight);
                            }
                            else if (GetProjectionType() == E_HPT_PERSPECTIVE)
                            {
                                // 告诉透视投影长宽比例
                                f32 fWidRadius = (f32)m_width/(m_height==0?1:m_height);
                                //if (fWidRadius > 4.0)
                                //{
                                //	fWidRadius = 4.0f;
                                //}
                                m_irrDevice->getSceneManager()->getActiveCamera()->setAspectRatio(fWidRadius);
                            }
                        }
                        m_bSizeChanged = false;
                    }
                    SColor bkColor(m_bkColor.getAlpha(),m_bkColor.getRed(),m_bkColor.getGreen(),m_bkColor.getBlue());
                    m_irrDevice->getVideoDriver()->beginScene(true, true, bkColor,m_irrDevice->getVideoDriver()->getExposedVideoData());	
                    if (m_width != 0 && m_height != 0)
                    {
                        core::rect<s32> sourceRect;
                        sourceRect.LowerRightCorner.set(m_width,m_height);
                        //m_irrDevice->getVideoDriver()->setGradientBackground(SColor(0,0,0,255));
                        m_irrDevice->getVideoDriver()->setViewPort(sourceRect);
                        m_irrDevice->getSceneManager()->drawAll();	
                        //m_irrDevice->getVideoDriver()->drawStencilShadow(true,SColor(255,0,255,0),SColor(255,0,255,0),SColor(255,0,0,255),SColor(255,255,0,0));

                    }				
                    m_irrDevice->getVideoDriver()->endScene();	

                    // 控制截屏
                    //if (m_bNeedScreenShot)
                    //{
                    //	CreateScreenShot();
                    //	m_bNeedScreenShot = false;
                    //}
                }
                break ;

            case WM_ERASEBKGND:
                break ;

            case WM_SIZE:
                {
                    int cx = LOWORD(lParam);
                    int cy = HIWORD(lParam);

                    OnSize(cx,cy);

                    //const core::dimension2d<u32> screensize = core::dimension2d<u32>(m_width,m_height);
                    //m_irrDevice->getVideoDriver()->OnResize(screensize);
                    //if (dev)
                    //	dev->OnResized();
                }
                break ;

            case WM_DESTROY:
                //PostQuitMessage(0);
                break ;

            case WM_SYSCOMMAND:
                // prevent screensaver or monitor powersave mode from starting
                if ((wParam & 0xFFF0) == SC_SCREENSAVE ||
                    (wParam & 0xFFF0) == SC_MONITORPOWER)
                    break ;
                break;

            case WM_ACTIVATE:
                // we need to take care for screen changes, e.g. Alt-Tab
                if (dev)
                {
                    if ((wParam&0xFF)==WA_INACTIVE)
                        dev->switchToFullScreen(true);
                    else
                        dev->switchToFullScreen();
                }
                break;

            case WM_USER:
                event.EventType = irr::EET_USER_EVENT;
                event.UserEvent.UserData1 = (irr::s32)wParam;
                event.UserEvent.UserData2 = (irr::s32)lParam;

                if (dev)
                    dev->postEventFromUser(event);

                break;

            case WM_SETCURSOR:
                // because Windows forgot about that in the meantime

                if (dev)
                    dev->getCursorControl()->setVisible( dev->getCursorControl()->isVisible() );
                break;

            }
            DefWindowProc(m_hWnd, message, wParam, lParam);
        }

        void ISceneView::SetFPSVisiable( bool bShow )
        {
            if (!m_irrDevice)
            {
                return;
            }

            CFPSSceneNode* fpsNode = dynamic_cast<CFPSSceneNode*>(GetSceneManager()->getSceneNodeFromId(999));
            if (fpsNode == NULL)
            {
                fpsNode = new CFPSSceneNode(m_irrDevice->getSceneManager()->getRootSceneNode(),m_irrDevice->getSceneManager(),999);
                fpsNode->SetView(this);
                fpsNode->drop();
            }
            fpsNode->SetFpsVisable(bShow);
            m_bShowFPS = bShow;
        }

        void ISceneView::SetLODVisiable(bool bshow)
        {
            if (!m_irrDevice)
            {
                return;
            }
            CFPSSceneNode* fpsNode = dynamic_cast<CFPSSceneNode*>(GetSceneManager()->getSceneNodeFromId(999));
            if (fpsNode == NULL)
            {
                fpsNode = new CFPSSceneNode(m_irrDevice->getSceneManager()->getRootSceneNode(),m_irrDevice->getSceneManager(),999);
                fpsNode->SetView(this);
                fpsNode->drop();
            }
            fpsNode->SetLodVisable(bshow);
            m_bShowLOD = bshow;
        }

        void ISceneView::SetAxisVisiable( bool bShow )
        {
            if (!m_irrDevice)
            {
                return;
            }

            if (m_viewType == E_HVT_3D || m_viewType == E_HVT_MULTISCAN3D || m_viewType == E_HVT_MLS3D || m_viewType == E_HVT_FACADEEDIT
                || m_viewType == E_HVT_ORTHO3D || m_viewType == E_HVT_SKETCH_ISCAN3D || m_viewType == E_HVT_DOM_3D_VIEW)
            {
                CAxisSceneNode* pAxisNode = dynamic_cast<CAxisSceneNode*>(GetSceneManager()->getSceneNodeFromId(1000));
                if (pAxisNode == NULL)
                {
                    pAxisNode = new CAxisSceneNode(m_irrDevice->getSceneManager()->getRootSceneNode(),m_irrDevice->getSceneManager(),1000);
                    pAxisNode->SetView(this);
                    pAxisNode->drop();
                }
                pAxisNode->setVisible(bShow);
                m_bShowAxis = bShow;
            }
        }

        void ISceneView::SetComPassVisiable(bool bShow)
        {
            if (!m_irrDevice)
            {
                return;
            }

            if (m_viewType == E_HVT_3D || m_viewType == E_HVT_MULTISCAN3D || m_viewType == E_HVT_SKETCH_ISCAN3D || m_viewType == E_HVT_MLS3D || m_viewType == E_HVT_FACADEEDIT
                || m_viewType == E_HVT_ORTHO3D)
            {
                CComPassSceneNode* pComPassNode = dynamic_cast<CComPassSceneNode*>(GetSceneManager()->getSceneNodeFromId(1100));
                if (pComPassNode == NULL)
                {
                    pComPassNode = new CComPassSceneNode(m_irrDevice->getSceneManager()->getRootSceneNode(),m_irrDevice->getSceneManager(),1100);
                    pComPassNode->SetView(this);
                    pComPassNode->drop();
                }
                pComPassNode->setVisible(bShow);
                m_bShowComPass = bShow;
            }

        }

        void ISceneView::SetColorLegendVisible(bool bShow)
        {
            if (!m_irrDevice)
            {
                return;
            }

            if (m_viewType == E_HVT_3D || m_viewType == E_HVT_MULTISCAN3D || m_viewType == E_HVT_SKETCH_ISCAN3D || m_viewType == E_HVT_MLS3D || m_viewType == E_HVT_FACADEEDIT)
            {
                CColorLegendSceneNode* pCLNode = dynamic_cast<CColorLegendSceneNode*>(GetSceneManager()->getSceneNodeFromType(ESNT_COLOR_LEGEND));
                if (pCLNode == NULL)
                {	
                    pCLNode = new CColorLegendSceneNode(m_irrDevice->getSceneManager()->getRootSceneNode(),m_irrDevice->getSceneManager(),2013);
                    pCLNode->SetView(this);
                    pCLNode->drop();
                }

                core::array<ISceneNode*> sceneList;
                GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);
                float minValue = 0.0f;
                float maxValue = 0.0f;
                for (unsigned int i = 0;i< sceneList.size();i++)
                {
                    CScanSceneNode* pScanScene = (CScanSceneNode*)sceneList[i];
                    pCLNode->SetColorRamp(pScanScene->GetColorRampZ());
                    pScanScene->GetMinMaxValue(maxValue,minValue);
                    pCLNode->SetMaxMinValue(maxValue,minValue);
                    break;
                }
                pCLNode->setVisible(bShow);
                m_bShowCL = bShow;
            }
        }

        void ISceneView::SetBackGroundVisible( bool bShow)
        {
            if (!m_irrDevice)
            {
                return;
            }

            if (m_viewType == E_HVT_3D || m_viewType == E_HVT_MULTISCAN3D || m_viewType == E_HVT_SKETCH_ISCAN3D || m_viewType == E_HVT_MLS3D)
            {
                if (strcmp(GetName(),"3DClassifyView") == 0)
                {
                    return;
                }
                CBackGroundSceneNode* pbkdNode = dynamic_cast<CBackGroundSceneNode*>(GetSceneManager()->getSceneNodeFromType(ESNT_HD_BACKGROUND));
                if (pbkdNode == NULL)
                {
                    video::IVideoDriver* driver = m_irrDevice->getVideoDriver();
                    string filePath = getCurrentDir();
                    filePath += "skydome.jpg";
                    video::ITexture* pBkTexture = driver->getTexture(filePath.data());
                    if (!pBkTexture)
                    {
                        return;
                    }

                    pbkdNode = new CBackGroundSceneNode(m_irrDevice->getSceneManager()->getRootSceneNode(),m_irrDevice->getSceneManager(),105,pBkTexture);
                    pbkdNode->SetView(this);
                    pbkdNode->drop();
                }

                pbkdNode->setVisible(bShow);
                m_bShowBackGround = bShow;

                CAxisSceneNode* pAxisSceneColor = dynamic_cast<CAxisSceneNode*>(GetSceneManager()->getSceneNodeFromType(ESNT_HD_AXIS));

                if (pAxisSceneColor)
                {
                    if (m_bShowBackGround)
                    {
                        pAxisSceneColor->SetAxisColor(1, SColor(255,255,255,255));
                    }
                    else
                    {

                    }		pAxisSceneColor->SetAxisColor(1, SColor(255,0,255,0));

                }


            }
        }

        void ISceneView::SetViewRenderAllNode(bool bAll, bool bCreateScreenShot)	// bSwitch重命名
        {
            if (!m_irrDevice)
            {
                return;
            }

            if (m_viewType == E_HVT_3D || m_viewType == E_HVT_MULTISCAN3D || m_viewType == E_HVT_SKETCH_ISCAN3D || m_viewType == E_HVT_PANO || m_viewType == E_HVT_DOM_3D_VIEW ||
                m_viewType == E_HVT_MLS3D || m_viewType == E_HVT_ORTHO3D || m_viewType == E_HVT_FACADEEDIT)
            {
                // 需要先刷新下,再截图,否则如果有其他视图打开情况下,截图不正确.gsl-2013/7/19
                RefreshViewBySendMessage();

                CScreenShotSceneNode* pscsNode = dynamic_cast<CScreenShotSceneNode*>(GetSceneManager()->getSceneNodeFromType(ESNT_HD_SCREENSHOT));
                if (pscsNode == NULL)
                {
                    //初始化时新建空白纹理
                    video::IVideoDriver* driver = m_irrDevice->getVideoDriver();
                    const core::dimension2d<u32> screensize = core::dimension2d<u32>(m_width,m_height);
                    video::ITexture* pTexture = driver->addTexture(screensize,"CurrentScreenShot",ECF_A8R8G8B8);

                    // 添加纹理失败，造成截屏SN失败返回
                    if (!pTexture)
                    {
                        m_bRenderAll = bAll;
                        m_bNeedScreenShot = false;
                        RefreshViewBySendMessage();
                        return;
                    }
                    pscsNode = new CScreenShotSceneNode(m_irrDevice->getSceneManager()->getRootSceneNode(),m_irrDevice->getSceneManager(),201,pTexture);
                    pscsNode->SetView(this);
                    pscsNode->drop();
                }
                else 
                {
                    if (bCreateScreenShot)		//控制绘制多段线时，不生成图像
                    {
                        video::IVideoDriver* driver = m_irrDevice->getVideoDriver();
                        video::IImage* image = driver->createScreenShot();
                        if (!image)
                        {
                            m_bRenderAll = bAll;
                            m_bNeedScreenShot = false;
                            RefreshViewBySendMessage();
                            ReloadData();
                            return;
                        }

                        video::ITexture* pTexture = driver->addTexture("CurrentScreenShot",image);
                        if (!pTexture)
                        {
                            if (image)
                            {
                                image->drop();
                            }

                            m_bRenderAll = bAll;
                            m_bNeedScreenShot = false;
                            RefreshViewBySendMessage();
                            ReloadData();
                            return;
                        }

                        pscsNode->UpdateTexture(pTexture);

                        // createScreenShot实际是new了一个新的image,需要drop
                        if (image)
                        {
                            image->drop();
                        }
                    }
                }
                m_bRenderAll = bAll;
            }
        }

        bool ISceneView::SetCameraTool( CHdCamera* camera )
        {
            if (!camera)
            {
                return false;
            }

            m_camera = camera;

            return true;
        }

        //!计算场景中缩小以及旋转的最小阈值height * width fengjing 2013-7-17
        int ISceneView::GetScreenRange(unsigned int &height, unsigned int &width)
        {
            core::aabbox3df bbox;
            core::vector3d<f32> edges[8]; 
            core::position2di screenPos[8];
            core::position2di min,max;

            // 获取数据包围盒
            core::vector3df center;

            // 对于包围盒
            if (IsIncludeSceneNode(ESNT_HD_DEM) || IsIncludeSceneNode((ESNT_HD_TIN)))
            {
                center = GetDataCenter(bbox);
            }
            else
            {
                center = GetCenter(bbox);
            }

            bbox.getEdges(edges);

            int i = 0;
            for (i=0; i<8; i++)
            {
                screenPos[i] = GetSceneManager()->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(edges[i]);
            }

            min.X = screenPos[0].X;
            min.Y = screenPos[0].Y;

            max.X = screenPos[0].X;
            max.Y = screenPos[0].Y;

            for (i=0; i<8; i++)
            {
                VecUpdateMinMax2dv(min, max, screenPos[i]);
            }

            height = max.X - min.X;
            width = max.Y - min.Y;

            return 1;	
        }

        //! 获取视图中心,如果是地面单测站就是测站原点,其他情况是立方盒中心
        core::vector3df ISceneView::GetCenter(core::aabbox3df& bbox)
        {
            // 目前策略，ESNT_SCAN_POINT 和ESNT_HD_SEADATA_POINT 2类节点不可共存于同一个视图中
            bbox.reset(0.0f,0.0f,0.0f);

            bool bFirstBox = true;
            int i = 0;
            core::array<ISceneNode*> sceneList; // 常规点云

            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

            int nSNCount = sceneList.size();

            //得到所有测站点云的最大外包Box
            int scanCount = 0;
            core::vector3df firstScanPos;
            for (i = 0; i<nSNCount; i++)
            {
                const core::aabbox3d<f32>& box = sceneList[i]->getBoundingBox();
                if (bFirstBox)
                {
                    CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                    if(pScanSn && pScanSn->GetPointCloud() && pScanSn->isVisible())
                    {
                        PointCloud* pcd = pScanSn->GetPointCloud();
                        if (pcd->m_header.number_of_col * pcd->m_header.number_of_row == pcd->m_header.number_of_point_records)
                        {
                            // 规则点云用测站原点
                            firstScanPos.X = 0.0f;
                            firstScanPos.Y = 0.0f;
                            firstScanPos.Z = 0.0f;
                        }
                        else
                        {
                            firstScanPos.X = box.getCenter().X;
                            firstScanPos.Y = box.getCenter().Y;
                            firstScanPos.Z = box.getCenter().Z;
                        }
                    }
                    else
                    {
                        firstScanPos.X = box.getCenter().X;
                        firstScanPos.Y = box.getCenter().Y;
                        firstScanPos.Z = box.getCenter().Z;
                    }
                    bbox = box;
                    bFirstBox = false;
                }
                else
                {
                    bbox.addInternalBox(box);
                }
            }

            if(nSNCount == 1)
            {
                return firstScanPos;
            }

            core::array<ISceneNode*> SeaScnList; // 海量点云节点

            GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,SeaScnList);

            int nSeaScCount = SeaScnList.size();

            if (nSeaScCount >0 )
            {
                // 海量点云节点
                for (i = 0; i < nSeaScCount; i++)
                {
                    const core::aabbox3d<f32>& box = SeaScnList[i]->getBoundingBox();

                    bbox.addInternalBox(box);
                }
                return bbox.getCenter();
            }

            return core::vector3df();
        }

        //! 获取视图旋转中心点
        bool ISceneView::GetRotateCenter(core::vector3df& rotCenter,int srcX,int srcY)
        {
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

            int nSNCount = sceneList.size();
            if(nSNCount == 0)
            {
                // 为获取测站点云的情况下，尝试从视图中获取分类点云
                GetSceneManager()->getSceneNodesFromType(ESNT_CLASSIFY_PTD, sceneList);

                nSNCount = sceneList.size();

                if (nSNCount == 0)
                {
                    return false;
                }
            }

            f32 minDist = F32_MAX;
            f32 cx = 0.0f;
            f32 cy = 0.0f;
            f32 cz = 0.0f;
            for (int i = 0; i<nSNCount; i++)
            {
                CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL || pScanSn->GetPointCloud() == NULL
                    || !pScanSn->isVisible())
                    continue;
                f32 dist = pScanSn->GetRotCenter(cx,cy,cz,srcX,srcY);
                if (dist < minDist)
                {
                    rotCenter.X = cx;
                    rotCenter.Y = cy;
                    rotCenter.Z = cz;
                    minDist = dist;
                }
            }
            return minDist < 50.0f;
        }

        core::vector3df ISceneView::GetDataCenter( core::aabbox3df&  bbox /*= core::aabbox3df()*/ )
        {
            bool bFirstBox = true;
            int i;
            core::array<ISceneNode*> ScansceneList; // ScanSceneNode节点列表
            core::array<ISceneNode*> SeaDatasceneList; // SeaScanSceneNode 节点列表

            // 包含ScanNode
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,ScansceneList);

            // ScanSceneNode 节点个数
            int nSNCount = ScansceneList.size();

            // 包含海量点云节点
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,SeaDatasceneList);

            // 海量点云节点个数
            int nSeaDataCount = SeaDatasceneList.size();

            // 节点不存在
            if(nSNCount == 0 && nSeaDataCount == 0)
            {
                return core::vector3df();
            }

            //得到所有测站点云的最大外包Box

            double dx = 0.0;
            double dy = 0.0;
            double dz = 0.0;

            //参与计算质心的点云个数
            int scanCount = 0;
            //计算质心的圈数抽样
            int colSimple = 4;
            //计算质心的行数抽样
            int rowSimple = 4;
            int n = 0;

            for (i = 0; i < nSNCount; i++)
            {
                // Scan Node
                if (ScansceneList[i]->getType() == ESNT_SCAN_POINT)
                {

                    // 如果标记该视图只对可见点云zoom，此处做处理
                    CScanSceneNode* pScanSN = dynamic_cast<CScanSceneNode*>(ScansceneList[i]);
                    if (m_bZoomToVisiblePcd && (pScanSN->isVisible() == false))
                    {
                        continue;
                    }

                    const core::aabbox3d<f32>& boxx = ScansceneList[i]->getBoundingBox();
                    core::aabbox3d<f32> box;
                    box = boxx ;

                    if (pScanSN && pScanSN->IsTrans())
                    {
                        irr::core::vector3df center = box.getCenter();
                        irr::core::vector3df extent = box.getExtent();

                        CBursaWolfModel renderModel = pScanSN->GetRenderModel();
                        renderModel.Translate(center.X,center.Y,center.Z);

                        box.MinEdge.X = center.X - extent.X/2.f;
                        box.MaxEdge.X = center.X + extent.X/2.f;
                        box.MinEdge.Y = center.Y - extent.Y/2.f;
                        box.MaxEdge.Y = center.Y + extent.Y/2.f;
                        box.MinEdge.Z = center.Z - extent.Z/2.f;
                        box.MaxEdge.Z = center.Z + extent.Z/2.f;
                    }

                    if (bFirstBox)
                    {
                        bbox = box;
                        bFirstBox = false;
                    }
                    else
                    {
                        bbox.addInternalBox(box);
                    }

                    // 一圈一圈的统计在视椎范围内的点云外包围盒
                    // 当通过PointCloud获取点云的圈范围时，对于经过平移旋转的点云圈的范围尚未更新的情况，这种方式失效
                    //CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                    //if(pScanSn == NULL || pScanSn->GetPointCloud() == NULL ||
                    //	!pScanSn->isVisible())
                    //	continue;

                    //PointCloud* pcd = pScanSn->GetPointCloud();
                    //// 新相对坐标、绝对坐标、显示坐标转化方法 [2014/03/20 危迟]
                    //// 获取场景节点中的点云在显示时是否对相对坐标进行了转换
                    //bool bTrans = pScanSn->IsTrans();

                    //// 获取相对坐标转绝对坐标的模型
                    //CBursaWolfModel absModel = pScanSn->GetModel();

                    //// 获取相对坐标转显示坐标的模型
                    //CBursaWolfModel renderModel = pScanSn->GetRenderModel();

                    //u32 loopCount = pcd->getLoopCount();
                    //for (u32 i = 0;i<loopCount;i++)
                    //{
                    //	pcd->getLoopExtent(i,xmin,ymin,zmin,xmax,ymax,zmax);
                    //	if (bTrans)
                    //	{
                    //		xminD = xmin;
                    //		yminD = ymin;
                    //		zminD = zmin;
                    //		xmaxD = xmax;
                    //		ymaxD = ymax;
                    //		zmaxD = zmax;
                    //		renderModel.TranslateExtent(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);

                    //		loopBox.MinEdge.set((f32)xminD,(f32)yminD,(f32)zminD);
                    //		loopBox.MaxEdge.set((f32)xmaxD,(f32)ymaxD,(f32)zmaxD);
                    //	}
                    //	else
                    //	{
                    //		loopBox.MinEdge.set(xmin,ymin,zmin);
                    //		loopBox.MaxEdge.set(xmax,ymax,zmax);
                    //	}
                    //	if (!pViewFrustum->isCubeIn(loopBox))
                    //	{
                    //		continue;
                    //	}
                    //	if (bFirstBox)
                    //	{
                    //		bbox = loopBox;
                    //	}
                    //	else
                    //		bbox.addInternalBox(loopBox);

                    //	bFirstBox = false;
                    //}

                    // 计算当前视图ScanSceneNode的点云质心位置
                    float cx = 0.0,cy = 0.0,cz = 0.0;
                    if (pScanSN->GetViewCenter(cx,cy,cz))
                    {
                        dx += cx;
                        dy += cy;
                        dz += cz;
                        scanCount++;
                    }
                }
            }


            // 如果视图中存在海量点云节点，那么直接统计点云包围盒的
            for (i = 0; i < nSeaDataCount; i++)
            {
                // SeaData Node
                if (SeaDatasceneList[i]->getType() == ESNT_HD_SEADATA_POINT)
                {

                    // 如果标记该视图只对可见点云zoom，此处做处理
                    CHdSeaDataSceneNode* pSeaDtsn = dynamic_cast<CHdSeaDataSceneNode*>(SeaDatasceneList[i]);
                    if (m_bZoomToVisiblePcd && (pSeaDtsn->isVisible() == false))
                    {
                        continue;
                    }

                    const core::aabbox3d<f32>& boxx = SeaDatasceneList[i]->getBoundingBox();
                    core::aabbox3d<f32> box;
                    box = boxx ;

                    if (pSeaDtsn && pSeaDtsn->IsTrans())
                    {
                        irr::core::vector3df center = box.getCenter();
                        irr::core::vector3df extent = box.getExtent();

                        CBursaWolfModel renderModel = pSeaDtsn->GetRenderModel();
                        renderModel.Translate(center.X,center.Y,center.Z);

                        box.MinEdge.X = center.X - extent.X/2.f;
                        box.MaxEdge.X = center.X + extent.X/2.f;
                        box.MinEdge.Y = center.Y - extent.Y/2.f;
                        box.MaxEdge.Y = center.Y + extent.Y/2.f;
                        box.MinEdge.Z = center.Z - extent.Z/2.f;
                        box.MaxEdge.Z = center.Z + extent.Z/2.f;
                    }

                    if (bFirstBox)
                    {
                        bbox = box;
                        bFirstBox = false;
                    }
                    else
                    {
                        bbox.addInternalBox(box);
                    }

                    // 计算当前视图ScanSceneNode的点云质心位置
                    float cx = 0.0,cy = 0.0,cz = 0.0;
                    if (pSeaDtsn->GetViewCenter(cx,cy,cz))
                    {
                        dx += cx;
                        dy += cy;
                        dz += cz;
                        scanCount++;
                    }
                }
            }


            if (scanCount > 0)
            {
                return core::vector3df((f32)(dx / scanCount),(f32)(dy / scanCount),(f32)(dz / scanCount));//
            }
            else
            {
                return bbox.getCenter();
            }
        }

        // 3D视图通过屏幕坐标获得点所在的圈号 
        PointXYZIPRGBA ISceneView::getImageScaleFrScrPos(ESCENE_NODE_TYPE type, float& rowscal,float& colscal , int srcX, int srcY, int tol)
        {
            if (tol <= 0)
            {
                tol = 5;
            }
            PointXYZIPRGBA pointRet;

            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return pointRet;
            }
            irr::core::recti irrRect;
            irrRect.LowerRightCorner.set(srcX + tol,srcY + tol);
            irrRect.UpperLeftCorner.set(srcX - tol,srcY - tol);
            const irr::scene::SViewFrustum* pViewFrustum = sceneMng->getActiveCamera()->getViewFrustum();
            irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
            sceneMng->GetViewFrustum(irrRect,&rgnFrustum);
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(type,sceneList);

            //分类视图中的三维视图的场景结点为ESNT_CLASSIFY_PTD
            if (!sceneList.size())
            {
                GetSceneManager()->getSceneNodesFromType(ESNT_CLASSIFY_PTD, sceneList);
            }

            core::vector3df coord;
            core::position2di screenPos;

            irr::core::aabbox3df loopBox;
            F32 xmin,ymin,zmin,xmax,ymax,zmax;
            F64 xminD,yminD,zminD,xmaxD,ymaxD,zmaxD;
            u32 srcMinDist = U32_MAX,srcDist;
            u32 srcDx,srcDy;
            F64 ptX,ptY,ptZ;

            for (u32 i = 0; i<sceneList.size(); i++)
            {
                CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL || pScanSn->GetPointCloud() == NULL ||
                    !pScanSn->isVisible())
                    continue;

                PointCloud* pcd = pScanSn->GetPointCloud();

                // 新相对坐标、绝对坐标、显示坐标转化方法 [2014/03/20 危迟]
                // 获取场景节点中的点云在显示时是否对相对坐标进行了转换
                bool bTrans = pScanSn->IsTrans();

                // 获取相对坐标转绝对坐标的模型
                CBursaWolfModel absModel = pScanSn->GetModel();

                // 获取相对坐标转显示坐标的模型
                CBursaWolfModel renderModel = pScanSn->GetRenderModel();

                u32 loopCount = pcd->getLoopCount();
                for (u32 i = 0;i<loopCount;i++)
                {
                    pcd->getLoopExtent(i,xmin,ymin,zmin,xmax,ymax,zmax);
                    if (bTrans)
                    {
                        xminD = xmin;
                        yminD = ymin;
                        zminD = zmin;
                        xmaxD = xmax;
                        ymaxD = ymax;
                        zmaxD = zmax;
                        renderModel.TranslateExtent(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);

                        loopBox.MinEdge.set((f32)xminD,(f32)yminD,(f32)zminD);
                        loopBox.MaxEdge.set((f32)xmaxD,(f32)ymaxD,(f32)zmaxD);
                    }
                    else
                    {
                        loopBox.MinEdge.set(xmin,ymin,zmin);
                        loopBox.MaxEdge.set(xmax,ymax,zmax);
                    }
                    if (!rgnFrustum.isCubeIn(loopBox))
                    {
                        continue;
                    }

                    hdVector<PointXYZIPRGBA>& pts = pcd->getLoop(i);
                    for (u32 n = 0;n < pts.size();n++)
                    {
                        PointXYZIPRGBA& pt = *(pts._Myfirst + n);
                        if (!pt.isValid())
                        {
                            continue;
                        }

                        // 点云不可见时 不捕捉
                        if (!IsPointVisible(pt))
                        {
                            continue;
                        }

                        if (bTrans)
                        {			
                            ptX = pt.x;
                            ptY = pt.y;
                            ptZ = pt.z;
                            renderModel.Translate(ptX,ptY,ptZ);
                            coord.set((float)ptX,(float)ptY,(float)ptZ);
                        }
                        else
                        {
                            coord.set(pt.x,pt.y,pt.z);
                        }

                        screenPos = sceneMng->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
                        if (!irrRect.isPointInside(screenPos))
                            continue;

                        srcDx = screenPos.X - srcX;
                        srcDy = screenPos.Y - srcY;
                        srcDist = srcDx * srcDx + srcDy * srcDy;
                        if (srcDist < srcMinDist)
                        {
                            srcMinDist = srcDist;
                            colscal =(float)i/loopCount;
                            // 对于浏览模式下的点云，由于每一圈加载时，采用ReadLoop接口，去掉了无效点，从而丢失规则行列信息
                            // 那么就无法获取正确的行列比例值 [2014/07/23 危迟]
                            rowscal =(float)n/pts.size();			
                            pointRet = pt;
                        }

                    }//for (u32 n = 0;n < pts.size();n++)
                }//for (u32 i = 0;i<loopCount;i++)
            }//for (u32 i = 0; i<sceneList.size(); i++)

            return pointRet;
        }

        // 3D视图通过屏幕坐标获得点强度 
        bool ISceneView::getIntensityFrScrPos(int& intensity, int srcX, int srcY, int tol)
        {
            if (tol < 0)
            {
                return false;
            }

            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            irr::core::recti irrRect;
            irrRect.LowerRightCorner.set(srcX + tol,srcY + tol);
            irrRect.UpperLeftCorner.set(srcX - tol,srcY - tol);
            const irr::scene::SViewFrustum* pViewFrustum = sceneMng->getActiveCamera()->getViewFrustum();
            irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
            sceneMng->GetViewFrustum(irrRect,&rgnFrustum);
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

            //分类视图中的三维视图的场景结点为ESNT_CLASSIFY_PTD
            if (sceneList.size() < 1)
            {
                GetSceneManager()->getSceneNodesFromType(ESNT_CLASSIFY_PTD, sceneList);
            }

            core::vector3df coord;
            core::position2di screenPos;

            irr::core::aabbox3df loopBox;
            F32 xmin,ymin,zmin,xmax,ymax,zmax;
            F64 xminD,yminD,zminD,xmaxD,ymaxD,zmaxD;
            u32 srcMinDist = U32_MAX,srcDist;
            u32 srcDx,srcDy;
            F64 ptX,ptY,ptZ;
            for (u32 i = 0; i<sceneList.size(); i++)
            {
                CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL || pScanSn->GetPointCloud() == NULL ||
                    !pScanSn->isVisible())
                    continue;

                PointCloud* pcd = pScanSn->GetPointCloud();

                // 新相对坐标、绝对坐标、显示坐标转化方法 [2014/03/20 危迟]
                // 获取场景节点中的点云在显示时是否对相对坐标进行了转换
                bool bTrans = pScanSn->IsTrans();

                // 获取相对坐标转绝对坐标的模型
                CBursaWolfModel absModel = pScanSn->GetModel();

                // 获取相对坐标转显示坐标的模型
                CBursaWolfModel renderModel = pScanSn->GetRenderModel();

                u32 loopCount = pcd->getLoopCount();
                for (u32 i = 0;i<loopCount;i++)
                {
                    pcd->getLoopExtent(i,xmin,ymin,zmin,xmax,ymax,zmax);
                    if (bTrans)
                    {
                        xminD = xmin;
                        yminD = ymin;
                        zminD = zmin;
                        xmaxD = xmax;
                        ymaxD = ymax;
                        zmaxD = zmax;
                        renderModel.TranslateExtent(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);

                        loopBox.MinEdge.set((f32)xminD,(f32)yminD,(f32)zminD);
                        loopBox.MaxEdge.set((f32)xmaxD,(f32)ymaxD,(f32)zmaxD);
                    }
                    else
                    {
                        loopBox.MinEdge.set(xmin,ymin,zmin);
                        loopBox.MaxEdge.set(xmax,ymax,zmax);
                    }
                    if (!rgnFrustum.isCubeIn(loopBox))
                    {
                        continue;
                    }

                    hdVector<PointXYZIPRGBA>& pts = pcd->getLoop(i);
                    for (u32 n = 0;n < pts.size();n++)
                    {
                        PointXYZIPRGBA& pt = *(pts._Myfirst + n);
                        if (!pt.isValid())
                        {
                            continue;
                        }

                        // 点云不可见时 不捕捉
                        if (!IsPointVisible(pt))
                        {
                            continue;
                        }

                        if (bTrans)
                        {			
                            ptX = pt.x;
                            ptY = pt.y;
                            ptZ = pt.z;
                            renderModel.Translate(ptX,ptY,ptZ);
                            coord.set((float)ptX,(float)ptY,(float)ptZ);
                        }
                        else
                        {
                            coord.set((float)pt.x,(float)pt.y,(float)pt.z);
                        }

                        screenPos = sceneMng->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
                        if (!irrRect.isPointInside(screenPos))
                            continue;

                        srcDx = screenPos.X - srcX;
                        srcDy = screenPos.Y - srcY;
                        srcDist = srcDx * srcDx + srcDy * srcDy;
                        if (srcDist < srcMinDist)
                        {
                            srcMinDist = srcDist;
                            intensity = pt.intensity;
                        }

                    }//for (u32 n = 0;n < pts.size();n++)
                }//for (u32 i = 0;i<loopCount;i++)
            }//for (u32 i = 0; i<sceneList.size(); i++)

            return srcMinDist < U32_MAX;
        }

        //! 根据矩形框获取点云 zhangfei
        bool ISceneView::QueryPtsInRect(ESCENE_NODE_TYPE type, HRGN Rgn, std::vector<PointXYZIPRGBA*>& rectInPts)
        {
            //vector<PointXYZIPRGBA> rectInPts;
            if (rectInPts.size() > 0)
            {
                rectInPts.clear();
            }
            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            const irr::scene::SViewFrustum* pViewFrustum = sceneMng->getActiveCamera()->getViewFrustum();
            irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
            // 将Rgn转换为irrRect
            RECT srcRect;
            GetRgnBox(Rgn,&srcRect);
            irr::core::recti irrRect;
            irrRect.LowerRightCorner.set(srcRect.right,srcRect.bottom);
            irrRect.UpperLeftCorner.set(srcRect.left,srcRect.top);
            sceneMng->GetViewFrustum(irrRect,&rgnFrustum);
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(type, sceneList);

            core::vector3df coord;
            core::position2di screenPos;

            irr::core::aabbox3df loopBox;
            F32 xmin, ymin, zmin, xmax, ymax, zmax;
            F64 xminD, yminD, zminD, xmaxD, ymaxD, zmaxD;
            u32 srcMinDist = U32_MAX;

            PointXYZIPRGBA ptPoint;
            for (u32 i = 0; i < sceneList.size(); i++)
            {
                CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL || pScanSn->GetPointCloud() == NULL ||
                    !pScanSn->isVisible())
                    continue;

                PointCloud* pcd = pScanSn->GetPointCloud();

                // 新相对坐标、绝对坐标、显示坐标转化方法 [2014/03/20 危迟]
                // 获取场景节点中的点云在显示时是否对相对坐标进行了转换
                bool bTrans = pScanSn->IsTrans();

                // 获取相对坐标转绝对坐标的模型
                CBursaWolfModel absModel = pScanSn->GetModel();

                // 获取相对坐标转显示坐标的模型
                CBursaWolfModel renderModel = pScanSn->GetRenderModel();

                u32 loopCount = pcd->getLoopCount();
                for (u32 i = 0;i<loopCount;i++)
                {
                    pcd->getLoopExtent(i,xmin,ymin,zmin,xmax,ymax,zmax);
                    if (bTrans)
                    {
                        xminD = xmin;
                        yminD = ymin;
                        zminD = zmin;
                        xmaxD = xmax;
                        ymaxD = ymax;
                        zmaxD = zmax;
                        renderModel.TranslateExtent(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);

                        loopBox.MinEdge.set((f32)xminD,(f32)yminD,(f32)zminD);
                        loopBox.MaxEdge.set((f32)xmaxD,(f32)ymaxD,(f32)zmaxD);
                    }
                    else
                    {
                        loopBox.MinEdge.set(xmin,ymin,zmin);
                        loopBox.MaxEdge.set(xmax,ymax,zmax);
                    }
                    if (!rgnFrustum.isCubeIn(loopBox))
                    {
                        continue;
                    }

                    hdVector<PointXYZIPRGBA>& pts = pcd->getLoop(i);
                    for (u32 n = 0;n < pts.size();n++)
                    {
                        PointXYZIPRGBA& pt = *(pts._Myfirst + n);
                        if (!pt.isValid())
                        {
                            continue;
                        }

                        // 点云不可见时 不捕捉
                        if (!IsPointVisible(pt))
                        {
                            continue;
                        }

                        if (bTrans)
                        {			
                            ptPoint.x = pt.x;
                            ptPoint.y = pt.y;
                            ptPoint.z = pt.z;
                            renderModel.Translate(ptPoint.x, ptPoint.y, ptPoint.z);
                            coord.set(ptPoint.x, ptPoint.y, ptPoint.z);
                        }
                        else
                        {
                            coord.set(pt.x,pt.y,pt.z);
                        }
                        screenPos = sceneMng->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
                        if (::PtInRegion(Rgn,screenPos.X,screenPos.Y))
                        {
                            rectInPts.push_back(&pt);
                        }

                    }//for (u32 n = 0;n < pts.size();n++)
                }//for (u32 i = 0;i<loopCount;i++)
            }//for (u32 i = 0; i<sceneList.size(); i++)

            if (rectInPts.size() < 1)
            {
                return false;
            }
            return true;
        }

        //！ 根据屏幕位置获取点云，返回点云坐标为显示坐标 fengjing
        bool ISceneView::Get3DPosFromScrPos(PointXYZIPRGBA& ptPoint, ESCENE_NODE_TYPE type, int srcX, int srcY, int tol)
        {
            if (tol <= 0)
            {
                tol = 5;
            }
            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(type, sceneList);

            f64 globalX = 0,globalY = 0,globalZ = 0;
            u32 srcMinDist = U32_MAX, srcDist;

            for (u32 i = 0; i < sceneList.size(); i++)
            {
                CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL)
                    continue;

                srcDist = pScanSn->Get3DPosFromScrPos(ptPoint,globalX,globalY,globalZ,srcX,srcY,tol,false);

                if (srcDist < srcMinDist)
                {
                    srcMinDist = srcDist;

                    // 根据绝对坐标计算显示坐标
                    m_transModel.Translate(globalX,globalY,globalZ);

                    ptPoint.x = (float)globalX;
                    ptPoint.y = (float)globalY;
                    ptPoint.z = (float)globalZ;
                }

            }
            return srcMinDist < U32_MAX;
        }

        bool ISceneView::Get3DPosAndModelFromScrPos(PointXYZIPRGBA& ptPoint, CBursaWolfModel& transModel,ESCENE_NODE_TYPE type, int srcX, int srcY, int tol)
        {

            if (tol <= 0)
            {
                tol = 5;
            }
            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(type, sceneList);

            f64 globalX = 0,globalY = 0,globalZ = 0;
            u32 srcMinDist = U32_MAX, srcDist;

            for (u32 i = 0; i < sceneList.size(); i++)
            {
                PointXYZIPRGBA tmpPoint;

                CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL)
                {
                    continue;
                }

                srcDist = pScanSn->Get3DPosFromScrPos(tmpPoint,globalX,globalY,globalZ,srcX,srcY,tol,false);

                if (srcDist < srcMinDist)
                {
                    srcMinDist = srcDist;

                    ptPoint = tmpPoint;
                    transModel = pScanSn->GetModel();
                }

            }
            return srcMinDist < U32_MAX;
        }

        bool ISceneView::Get3DPosAndModelFromScrPos(PointXYZIPRGBA& ptPoint, CBursaWolfModel& transModel, int srcX, int srcY, int tol)
        {

            if (tol <= 0)
            {
                tol = 5;
            }
            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            ESCENE_NODE_TYPE sent_type = ESNT_SCAN_POINT;
            if (IsIncludeSceneNode(ESNT_HD_SEADATA_POINT))
            {
                sent_type = ESNT_HD_SEADATA_POINT;
            }

            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(sent_type, sceneList);

            f64 globalX = 0,globalY = 0,globalZ = 0;
            u32 srcMinDist = U32_MAX, srcDist;

            for (u32 i = 0; i < sceneList.size(); i++)
            {
                PointXYZIPRGBA tmpPoint;
                if (sent_type == ESNT_SCAN_POINT)
                {
                    CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                    if(pScanSn == NULL)
                    {
                        continue;
                    }

                    srcDist = pScanSn->Get3DPosFromScrPos(tmpPoint,globalX,globalY,globalZ,srcX,srcY,tol,false);

                    if (srcDist < srcMinDist)
                    {
                        srcMinDist = srcDist;

                        ptPoint = tmpPoint;
                        transModel = pScanSn->GetModel();
                    }
                }
                else if (sent_type == ESNT_HD_SEADATA_POINT)
                {
                    CHdSeaDataSceneNode* pScanSn = dynamic_cast<CHdSeaDataSceneNode*>(sceneList[i]);						
                    if(pScanSn == NULL)
                    {
                        continue;
                    }

                    srcDist = pScanSn->Get3DPosFromScrPos(tmpPoint,globalX,globalY,globalZ,srcX,srcY,tol,false);

                    if (srcDist < srcMinDist)
                    {
                        srcMinDist = srcDist;

                        ptPoint = tmpPoint;
                        transModel = pScanSn->GetModel();
                    }
                }
            }
            return srcMinDist < U32_MAX;
        }


        //! 海量点云根据屏幕坐标获取相对坐标和从相对到转换模型 -----chy------
        bool ISceneView::Get3DPosAndModelFromSeaScrPos(
            PointXYZIPRGBA& ptPoint,
            CBursaWolfModel& transModel,
            ESCENE_NODE_TYPE type,  // 场景结点类型   
            int srcX, int srcY,		// 屏幕坐标
            int tol
            )
        {

            if (tol <= 0)
            {
                tol = 5;
            }

            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(type, sceneList);

            f64 globalX = 0,globalY = 0,globalZ = 0;
            u32 srcMinDist = U32_MAX, srcDist;

            for (u32 i = 0; i < sceneList.size(); i++)
            {
                CHdSeaDataSceneNode* pScanSn = dynamic_cast<CHdSeaDataSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL)
                    continue;

                PointXYZIPRGBA tmpPoint;
                srcDist = pScanSn->Get3DPosFromScrPos(tmpPoint,globalX,globalY,globalZ,srcX,srcY,tol,false);

                if (srcDist < srcMinDist)
                {
                    srcMinDist = srcDist;

                    ptPoint = tmpPoint;
                    transModel = pScanSn->GetModel();
                }
            }
            return srcMinDist < U32_MAX;
        }

        bool ISceneView::Get3DSelPosAndModelFromScrPos(PointXYZIPRGBA& ptPoint, CBursaWolfModel& transModel,ESCENE_NODE_TYPE type, int srcX, int srcY, int tol, bool bSelect)
        {
            if (tol <= 0)
            {
                tol = 5;
            }
            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(type, sceneList);

            f64 globalX = 0,globalY = 0,globalZ = 0;
            u32 srcMinDist = U32_MAX, srcDist;

            for (u32 i = 0; i < sceneList.size(); i++)
            {
                CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL)
                    continue;

                PointXYZIPRGBA tmpPoint;
                srcDist = pScanSn->Get3DPosFromScrPos(tmpPoint,globalX,globalY,globalZ,srcX,srcY,tol,false,bSelect);

                if (srcDist < srcMinDist)
                {
                    srcMinDist = srcDist;

                    ptPoint = tmpPoint;
                    transModel = pScanSn->GetModel();
                }
            }
            return srcMinDist < U32_MAX;
        }


        // 根据屏幕位置获得点云、其行列比例和相对到绝对的转换模型
        bool ISceneView::GetPosScaleAndModelFromScrPos(PointXYZIPRGBA& ptPoint,	CBursaWolfModel& transModel, ESCENE_NODE_TYPE type,	int srcX, int srcY,	float& scaleX, float& scaleY, int tol)
        {
            if (tol <= 0)
            {
                tol = 5;
            }

            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            irr::core::recti irrRect;
            irrRect.LowerRightCorner.set(srcX + tol,srcY + tol);
            irrRect.UpperLeftCorner.set(srcX - tol,srcY - tol);
            const irr::scene::SViewFrustum* pViewFrustum = sceneMng->getActiveCamera()->getViewFrustum();
            irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
            sceneMng->GetViewFrustum(irrRect,&rgnFrustum);
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(type,sceneList);

            if (sceneList.size() <= 0)
            {
                return false;
            }

            core::vector3df coord;
            core::position2di screenPos;

            irr::core::aabbox3df loopBox;
            F32 xmin,ymin,zmin,xmax,ymax,zmax;
            F64 xminD,yminD,zminD,xmaxD,ymaxD,zmaxD;
            u32 srcMinDist = U32_MAX,srcDist;
            u32 srcDx,srcDy;
            F64 ptX,ptY,ptZ;
            for (u32 i = 0; i<sceneList.size(); i++)
            {
                CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL || pScanSn->GetPointCloud() == NULL || !pScanSn->isVisible())
                    continue;

                PointCloud* pcd = pScanSn->GetPointCloud();

                // 获取场景节点中的点云在显示时是否对相对坐标进行了转换
                bool bTrans = pScanSn->IsTrans();

                // 获取相对坐标转绝对坐标的模型
                CBursaWolfModel absModel = pScanSn->GetModel();

                // 获取相对坐标转显示坐标的模型
                CBursaWolfModel renderModel = pScanSn->GetRenderModel();

                u32 loopCount = pcd->getLoopCount();
                for (u32 i = 0;i<loopCount;i++)
                {
                    pcd->getLoopExtent(i,xmin,ymin,zmin,xmax,ymax,zmax);
                    if (bTrans)
                    {
                        xminD = xmin;
                        yminD = ymin;
                        zminD = zmin;
                        xmaxD = xmax;
                        ymaxD = ymax;
                        zmaxD = zmax;
                        renderModel.TranslateExtent(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);

                        loopBox.MinEdge.set((f32)xminD,(f32)yminD,(f32)zminD);
                        loopBox.MaxEdge.set((f32)xmaxD,(f32)ymaxD,(f32)zmaxD);
                    }
                    else
                    {
                        loopBox.MinEdge.set(xmin,ymin,zmin);
                        loopBox.MaxEdge.set(xmax,ymax,zmax);
                    }
                    if (!rgnFrustum.isCubeIn(loopBox))
                    {
                        continue;
                    }

                    hdVector<PointXYZIPRGBA>& pts = pcd->getLoop(i);
                    for (u32 n = 0;n < pts.size();n++)
                    {
                        PointXYZIPRGBA& pt = *(pts._Myfirst + n);
                        if (!pt.isValid())
                        {
                            continue;
                        }

                        // 点云不可见时 不捕捉
                        if (!IsPointVisible(pt))
                        {
                            continue;
                        }

                        if (bTrans)
                        {			
                            ptX = pt.x;
                            ptY = pt.y;
                            ptZ = pt.z;
                            renderModel.Translate(ptX,ptY,ptZ);
                            coord.set((float)ptX,(float)ptY,(float)ptZ);
                        }
                        else
                        {
                            coord.set(pt.x,pt.y,pt.z);
                        }

                        screenPos = sceneMng->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
                        if (!irrRect.isPointInside(screenPos))
                            continue;

                        srcDx = screenPos.X - srcX;
                        srcDy = screenPos.Y - srcY;
                        srcDist = srcDx * srcDx + srcDy * srcDy;
                        if (srcDist < srcMinDist)
                        {
                            srcMinDist = srcDist;

                            scaleX = (float)i/loopCount;
                            scaleY = (float)n/pts.size();			
                            ptPoint = pt;

                            transModel = pcd->GetModel();
                        }

                    }//for (u32 n = 0;n < pts.size();n++)
                }//for (u32 i = 0;i<loopCount;i++)
            }//for (u32 i = 0; i<sceneList.size(); i++)

            return srcMinDist < U32_MAX;
        }

        // 根据屏幕坐标返回点相对坐标和绝对坐标
        bool ISceneView::Get2PosFromScrPos(PointXYZIPRGBA& ptPoint, f64& x,f64& y,f64& z,ESCENE_NODE_TYPE type, int srcX, int srcY, int tol)
        {
            if (tol <= 0)
            {
                tol = 5;
            }
            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(type, sceneList);

            f64 globalX = 0,globalY = 0,globalZ = 0;
            u32 srcMinDist = U32_MAX, srcDist;
            PointXYZIPRGBA point;
            for (u32 i = 0; i < sceneList.size(); i++)
            {
                CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL)
                    continue;

                srcDist = pScanSn->Get3DPosFromScrPos(point,globalX,globalY,globalZ,srcX,srcY,tol,true);

                // 如果距离为0并且全局坐标均为0，则执行下次
                if (srcDist == 0 && globalX == 0&& globalY == 0 && globalZ == 0)
                {
                    continue;
                }

                if (srcDist < srcMinDist)
                {
                    srcMinDist = srcDist;
                    ptPoint = point;
                    x = globalX;
                    y = globalY;
                    z = globalZ;
                }

            }
            return srcMinDist < U32_MAX;

        }
        //! 根据屏幕坐标获取点云绝对坐标,gsl-2014/1/8修改注释
        bool ISceneView::Get3DPosFromScrPos(
            f64& x,f64& y,f64& z,	// 返回点云绝对坐标坐标值
            int srcX,int srcY,		// 屏幕坐标
            int tol)				// 屏幕容差,默认值5
        {
            if (tol < 0)
            {
                return false;
            }

            x = 0.0;
            y = 0.0;
            z = 0.0;

            PointXYZIPRGBA ptPoint;
            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }


            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

            //分类视图中的三维视图的场景结点为ESNT_CLASSIFY_PTD
            if (!sceneList.size())
            {
                GetSceneManager()->getSceneNodesFromType(ESNT_CLASSIFY_PTD, sceneList);
            }

			if (!sceneList.size())
			{
				PointXYZIPRGBA outPt;
				return Get3DPosFromTmpPcd(outPt,x,y,z,srcX,srcY,tol);
			}

            u32 srcMinDist = U32_MAX,srcDist;
            F64 ptX = 0,ptY = 0,ptZ = 0;
            for (u32 i = 0; i<sceneList.size(); i++)
            {
                CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL)
                    continue;

                srcDist = pScanSn->Get3DPosFromScrPos(ptPoint,ptX,ptY,ptZ,srcX,srcY,tol,false);

                if (srcDist < srcMinDist)
                {
                    srcMinDist = srcDist;
                    x = ptX;
                    y = ptY;
                    z = ptZ;
                }
            }
            return srcMinDist < U32_MAX;
        }

        bool ISceneView::IsPointVisible(PointXYZIPRGBA& pt)
        {
            //通过当前视图中显示模式以及点的状态来判断当期点是否显示
            bool bVisiable = true;

            if ((GetViewShowStyle() == SHOW_SELECT && !pt.isSelected()) ||
                (GetViewShowStyle() == SHOW_UNSELECT && pt.isSelected()) ||
                (GetbRenderSetting() && !(GetIsRenderClass(pt.prop))))
            {
                bVisiable = false;
            }

            return bVisiable;
        }
        //! 加载数据线程函数
        DWORD WINAPI ISceneView::ReloadDataThread(LPVOID param)
        {
            ISceneView* pView = (ISceneView*)param;
            while(true)
            {
                if(::WaitForSingleObject(pView->m_reloadEvent,INFINITE) == WAIT_OBJECT_0)
                {
                    //static bool bRun = false;
                    try
                    {
                        //if (bRun)
                        //{
                        //	::ResetEvent(pView->m_reloadEvent);
                        //	continue;
                        //}
                        //bRun = true;

                        //已加载SceneNode个数重置为0   袁亮    20160820
                        //::InterlockedExchange(&g_loadCnt, 0);

                        core::array<ISceneNode*> sceneList;
                        pView->GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);


                        if (sceneList.size() == 0)
                        {
                            pView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_CUTFILL,sceneList);
                        }

                        // 判断是否含有TIN
                        if (sceneList.size() == 0)
                        {
                            pView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_TIN,sceneList);
                        }

                        // 判断是否含有分类节点
                        if (sceneList.size() == 0)
                        {
                            pView->GetSceneManager()->getSceneNodesFromType(ESNT_CLASSIFY_PTD,sceneList);
                        }

                        // 判断是否含有海量点云节点
                        if (sceneList.size() == 0)
                        {
                            pView->GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneList);
                        }

                        int nSNCount = sceneList.size();
                        if (nSNCount > 0)
                        {
                            //SceneNode重加载工作提交给Windows线程池完成   袁亮   20160820
                            for (int i = 0; i<nSNCount; i++)
                            {
                                if (sceneList[i]->getType() == ESNT_HD_SEADATA_POINT)
                                {
                                    CHdSeaDataSceneNode* pHdSdSn = dynamic_cast<CHdSeaDataSceneNode*>(sceneList[i]);
                                    if (pHdSdSn&& (pHdSdSn->isVisible() || pView->GetCalReLoad()))
                                    {
                                        pHdSdSn->ReloadData();
                                    }
                                }
                                else
                                {
                                    CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);
                                    if(pScanSn && pScanSn->GetPointCloud() && pScanSn->GetPointCloud()->count() > 0)
                                    {
                                        pScanSn->ReloadData();
                                    }
                                }
                                //::TrySubmitThreadpoolCallback(ReloadSNCallback, sceneList[i], &g_pcbe);
                            }
                        }
                        //等待所有SceneNode加载完成，然后刷新整个视图   袁亮   20160820
                        /* while(nSNCount != g_loadCnt)
                        {
                        ::Sleep(100);
                        }*/
                        pView->Refresh();
                        ::ResetEvent(pView->m_reloadEvent);
                        //bRun = false;
                    }
                    catch (...)
                    {

                    }
                }
                //::Sleep(20);
            }
        }
		//! 根据类型设置场景节点可见性
		void ISceneView::SetSceneNodeVisible(ESCENE_NODE_TYPE type,bool bVisible)
		{
			core::array<ISceneNode*> sceneList;

			GetSceneManager()->getSceneNodesFromType(type,sceneList);
			size_t i;
			for ( i = 0; i< sceneList.size();i++)
			{
				sceneList[i]->setVisible(bVisible);
			}
		}

        bool ISceneView::IsSceneNodeVisible(ESCENE_NODE_TYPE type)
        {
            bool bVisible = false;

            if (!IsIncludeSceneNode(type))
            {
                return bVisible;
            }

            core::array<ISceneNode*> sceneList;

            GetSceneManager()->getSceneNodesFromType(type,sceneList);
            size_t i;
            for ( i = 0; i< sceneList.size();i++)
            {
                if (sceneList[i]->isVisible())
                {
                    bVisible = true;
                    break;
                }
                continue;
            }

            return bVisible;
        }

        bool ISceneView::IsIncludeSceneNode( ESCENE_NODE_TYPE type )
        {
            bool bInclude = false;

            core::array<ISceneNode*> sceneList;

            GetSceneManager()->getSceneNodesFromType(type,sceneList);

            if (sceneList.size() > 0)
            {
                bInclude = true;
            }
            return bInclude;
        }

        bool ISceneView::HasIScanSceneNode()
        {
            irr::core::array<ISceneNode*> pScanNodes;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,pScanNodes);
            bool bIScan = false;
            for (unsigned int i = 0;i<pScanNodes.size();i++)
            {
                CScanSceneNode* pScanNd = dynamic_cast<CScanSceneNode*>(pScanNodes[i]);
                if(pScanNd && pScanNd->GetPointCloud())
                {
                    PointCloud* pcd = pScanNd->GetPointCloud();
                    if (pcd->m_header.offsetX > 100000.0 || pcd->m_header.offsetY > 100000.0)
                    {
                        bIScan = true;
                        break;
                    }
                }
            }
            return bIScan;
        }

        //！获取视图旋转中心点新
        bool ISceneView::GetRotateCenterNew(core::vector3df& rotCenter,int srcX,int srcY)
        {
            return true;

        }

        //！设置视图旋转中心点新
        bool ISceneView::SetRotateCenterNew(core::vector3df& rotCenter,int srcX,int srcY)
        {
            return true;
        }

        bool ISceneView::GetAverageIntensityFrom3DPos(ESCENE_NODE_TYPE type, f64& x,f64& y,f64& z, float areaTol /*= 0.1*/ )
        {
            if (areaTol < 0)
            {
                return false;
            }

            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            const irr::scene::SViewFrustum* pViewFrustum = sceneMng->getActiveCamera()->getViewFrustum();
            irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(type,sceneList);

            //分类视图中的三维视图的场景结点为ESNT_CLASSIFY_PTD
            if (!sceneList.size())
            {
                GetSceneManager()->getSceneNodesFromType(ESNT_CLASSIFY_PTD, sceneList);
            }
            f64 centerPosX = x;
            f64 centerPosY = y;
            f64 centerPosZ = z;
            f64 ptX,ptY,ptZ;
            x = y = z = 0.0;
            core::vector3df coord;
            irr::core::aabbox3df loopBox;
            irr::core::aabbox3df selBox;
            selBox.MinEdge.set((f32)(centerPosX - areaTol) ,(f32)(centerPosY - areaTol),(f32)(centerPosZ - areaTol));
            selBox.MaxEdge.set((f32)(centerPosX + areaTol) ,(f32)(centerPosY + areaTol),(f32)(centerPosZ + areaTol));

            int nCount = 0;

            for (int i =0;i < (int)sceneList.size();i++)
            {
                CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL || pScanSn->GetPointCloud() == NULL ||
                    !pScanSn->isVisible())
                    continue;

                PointCloud* pcd = pScanSn->GetPointCloud();

                // 新相对坐标、绝对坐标、显示坐标转化方法 [2014/03/20 危迟]
                // 获取场景节点中的点云在显示时是否对相对坐标进行了转换
                bool bTrans = pScanSn->IsTrans();

                // 获取相对坐标转绝对坐标的模型
                CBursaWolfModel absModel = pScanSn->GetModel();

                // 获取相对坐标转显示坐标的模型
                CBursaWolfModel renderModel = pScanSn->GetRenderModel();

                u32 loopCount = pcd->getLoopCount();
                for (u32 i = 0;i<loopCount;i++)
                {
                    bool bRet = pScanSn->isLoopInView(i);
                    if (!bRet)
                    {
                        continue;
                    }

                    hdVector<PointXYZIPRGBA>& pts = pcd->getLoop(i);
                    for (u32 n = 0;n < pts.size();n++)
                    {
                        PointXYZIPRGBA& pt = *(pts._Myfirst + n);
                        if (!pt.isValid())
                        {
                            continue;
                        }
                        ptX = pt.x;
                        ptY = pt.y;
                        ptZ = pt.z;

                        // 转换至点云全局坐标系
                        absModel.Translate(ptX,ptY,ptZ);

                        coord.set((float)ptX,(float)ptY,(float)ptZ);

                        if (selBox.isPointInside(coord))
                        {
                            x += ptX;
                            y += ptY;
                            z += ptZ;
                            nCount++;
                        }
                    }//for (u32 n = 0;n < pts.size();n++)
                }//for (u32 i = 0;i<loopCount;i++)
            }

            if (nCount > 0)
            {
                x /= nCount;
                y /= nCount;
                z /= nCount;
            }

            return true;
        }


        bool ISceneView::GetAveragePosFrom3DPos( f64& x,f64& y,f64& z, float areaTol /*= 0.1*/ ,bool bVisiable/*= true*/)
        {
            if (areaTol < 0)
            {	
                return false;
            }
            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            const irr::scene::SViewFrustum* pViewFrustum = sceneMng->getActiveCamera()->getViewFrustum();
            irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);
            f64 centerPosX = x;
            f64 centerPosY = y;
            f64 centerPosZ = z;
            f64 ptX,ptY,ptZ;
            x = y = z = 0.0;
            core::vector3df coord;
            irr::core::aabbox3df loopBox;
            irr::core::aabbox3df selBox;
            //selBox.MinEdge.set((f32)(centerPosX - areaTol) ,(f32)(centerPosY - areaTol),(f32)(centerPosZ - areaTol));
            //selBox.MaxEdge.set((f32)(centerPosX + areaTol) ,(f32)(centerPosY + areaTol),(f32)(centerPosZ + areaTol));

            // 定义selbox中心显示坐标的中间变量
            f64 fShowCenterX,fShowCenterY,fShowCenterZ;
            fShowCenterX = fShowCenterY = fShowCenterZ = 0.0f;

            int nCount = 0;

            for (int i =0;i < (int)sceneList.size();i++)
            {
                CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL || pScanSn->GetPointCloud() == NULL)
                    continue;
                if (bVisiable && !pScanSn->isVisible())
                {
                    continue;
                }
                PointCloud* pcd = pScanSn->GetPointCloud();
                // 新相对坐标、绝对坐标、显示坐标转化方法 [2014/03/20 危迟]
                // 获取场景节点中的点云在显示时是否对相对坐标进行了转换
                bool bTrans = pScanSn->IsTrans();

                // 获取相对坐标转绝对坐标的模型
                CBursaWolfModel absModel = pScanSn->GetModel();

                // 获取相对坐标转显示坐标的模型
                CBursaWolfModel renderModel = pScanSn->GetRenderModel();

                fShowCenterX = centerPosX;
                fShowCenterY = centerPosY;
                fShowCenterZ = centerPosZ;

                // 绝对坐标转换相对坐标
                absModel.AntiTranslate(fShowCenterX,fShowCenterY,fShowCenterZ);

                // 相对坐标转换显示坐标
                if (bTrans)
                {
                    renderModel.Translate(fShowCenterX,fShowCenterY,fShowCenterZ);
                }

                // 显示坐标设置box
                selBox.MinEdge.set((f32)(fShowCenterX - areaTol) ,(f32)(fShowCenterY - areaTol),(f32)(fShowCenterZ - areaTol));
                selBox.MaxEdge.set((f32)(fShowCenterX + areaTol) ,(f32)(fShowCenterY + areaTol),(f32)(fShowCenterZ + areaTol));

                u32 loopCount = pcd->getLoopCount();
                for (u32 i = 0;i<loopCount;i++)
                {
                    bool bRet = pScanSn->isLoopInView(i);
                    if (!bRet)
                    {
                        continue;
                    }

                    hdVector<PointXYZIPRGBA>& pts = pcd->getLoop(i);
                    for (u32 n = 0;n < pts.size();n++)
                    {
                        PointXYZIPRGBA& pt = *(pts._Myfirst + n);
                        if (!pt.isValid())
                        {
                            continue;
                        }

                        // 显示坐标下进行判断
                        ptX = pt.x;
                        ptY = pt.y;
                        ptZ = pt.z;

                        if (bTrans)
                        {
                            renderModel.Translate(ptX,ptY,ptZ);
                        }

                        // 转换至点云全局坐标系
                        //absModel.Translate(ptX,ptY,ptZ);
                        coord.set((float)ptX,(float)ptY,(float)ptZ);

                        if (selBox.isPointInside(coord))
                        {
                            // 在box范围内，先将相对坐标转换绝对坐标再累加
                            ptX = pt.x;
                            ptY = pt.y;
                            ptZ = pt.z;
                            absModel.Translate(ptX,ptY,ptZ);

                            x += ptX;
                            y += ptY;
                            z += ptZ;
                            nCount++;
                        }
                    }//for (u32 n = 0;n < pts.size();n++)
                }//for (u32 i = 0;i<loopCount;i++)
            }

            if (nCount > 0)
            {
                x /= nCount;
                y /= nCount;
                z /= nCount;
            }

            return true;
        }

        //! 根据传入的坐标值获取以该点为中心一定范围内所有点海量点云得平均值作返回
        bool ISceneView::GetAveragePosFromSea3DPos( f64& x,f64& y,f64& z, float areaTol ,bool bVisiable)
        {

            // 平均值为0 返回
            if (areaTol < 0)
            {	
                return false;
            }

            // 场景管理器不存时返回
            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            // 获取海量点云列表
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneList);

            // 统计X、Y、Z累计值
            f64 totalX = 0.0;
            f64 totalY = 0.0;
            f64 totalZ = 0.0;

            // 点个数
            int nCount = 0;

            for (int i =0;i < (int)sceneList.size();i++)
            {
                CHdSeaDataSceneNode* pScanSn = dynamic_cast<CHdSeaDataSceneNode*>(sceneList[i]);							

                if(pScanSn == NULL || pScanSn->GetPointCloud() == NULL)// 无效点云过滤
                {
                    continue;
                }

                if (bVisiable && !pScanSn->isVisible()) // 不可见点云过滤
                {
                    continue;
                }

                f64 inX = x;
                f64 inY = y;
                f64 inZ = z;

                // 调用点云内部接口
                if (pScanSn->GetAveragePosFromSea3DPos(inX,inY,inZ,areaTol,bVisiable))
                {
                    totalX += inX;
                    totalY += inY;
                    totalZ += inZ;
                    nCount++;

                }
            }

            // 有效则计算
            if (nCount >0)
            {
                x = totalX/nCount;
                y = totalY/nCount;
                z = totalZ/nCount;

                return true;
            }

            return false;
        }

        void ISceneView::SetScanAreaRenderStyle(ENUM_RENDERSTYLE style)
        {
            core::array<ISceneNode*> sceneList;
            // 3D视图或者多测站3D视图 
            if (m_viewType == E_HVT_3D || m_viewType == E_HVT_MULTISCAN3D || m_viewType == E_HVT_SKETCH_ISCAN3D ||
                m_viewType == E_HVT_QUICK || m_viewType==E_HVT_FACADEEDIT)
            {
                GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

                if (sceneList.empty())
                {
                    return;
                }

                for(unsigned int i = 0;i< sceneList.size();i++)
                {
                    CScanSceneNode* scanNode = dynamic_cast<CScanSceneNode*>(sceneList[i]);
                    if(scanNode)
                    {
                        scanNode->SetAreaRenderStyle(style);
                    }
                }
            }
        }
        //! 2013/8/26 蔡红云 加载扫描仪模型
        void ISceneView::SetScanModelVisible( bool bShow )
        {
            // 支持3D视图，多测站视图
            if (m_viewType == E_HVT_3D || m_viewType == E_HVT_SKETCH_ISCAN3D || m_viewType == E_HVT_MULTISCAN3D)
            {
                m_bShowScanModel = bShow;
                // if the data is Iscan  return 
                if (HasIScanSceneNode())
                {
                    return;
                }
                // 获取场景中的点云
                core::array<ISceneNode*> snList;
                GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT, snList);
                if (snList.size() == 0)
                {	
                    return;

                }
                // 获取场景中的扫描仪模型
                core::array<ISceneNode*> msnList;
                GetSceneManager()->getSceneNodesFromType(ESNT_MESH_MODEL, msnList);
                if (msnList.size() == 0)
                {	
                    return;

                }
                int snCount = snList.size();// 场景中的点云数目
                int msnCount = msnList.size(); // 场景中的扫描仪模型数目
                for (int i=0; i<snCount; i++)
                {
                    CScanSceneNode* scanSceneNode = dynamic_cast<CScanSceneNode*>(snList[i]);
                    if (!scanSceneNode)
                    {
                        return;
                    }

                    PointCloud* pcd = scanSceneNode->GetPointCloud();
                    for (int j=0; j< msnCount; j++)
                    {
                        CIMeshSceneNode* pMeshSN = dynamic_cast<CIMeshSceneNode*>(msnList[j]);
                        string strMesh = pMeshSN->getPcdFilePath();
                        string strPcd = pcd->GetPointCloudPath();
                        std::transform(strPcd.begin(), strPcd.end(), strPcd.begin(),tolower);
                        std::transform(strMesh.begin(), strMesh.end(), strMesh.begin(),tolower);
                        if (strMesh == strPcd)
                        {
                            pMeshSN->setVisible(m_bShowScanModel);
                        }
                    }
                }

            }
        }

        hd::ENUM_RENDERSTYLE ISceneView::GetScanAreaRenderStyle()
        {
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

            unsigned int i = 0;
            unsigned int nSNCount = sceneList.size();
            for (i = 0; i<nSNCount; i++)
            {
                return ((CScanSceneNode*)(sceneList[i]))->GetAreaRenderStyle();
            }

            return RENDER_AREA_BY_DEFAULT;
        }

        bool ISceneView::Get3DPosFromScrPosForCatch( f64& x,f64& y,f64& z, int srcX,int srcY, int tol /*= 5*/ ,bool bVisible /*= true*/, bool  bselect)
        {
            if (tol < 0)
            {
                return false;
            }

            x = 0.0;
            y = 0.0;
            z = 0.0;
            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            irr::core::recti irrRect;
            irrRect.LowerRightCorner.set(srcX + tol,srcY + tol);
            irrRect.UpperLeftCorner.set(srcX - tol,srcY - tol);
            const irr::scene::SViewFrustum* pViewFrustum = sceneMng->getActiveCamera()->getViewFrustum();
            irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
            sceneMng->GetViewFrustum(irrRect,&rgnFrustum);
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

            core::vector3df coord;
            core::position2di screenPos;

            irr::core::aabbox3df loopBox;
            u32 srcMinDist = U32_MAX,srcDist;
            u32 srcDx,srcDy;
            F64 ptX,ptY,ptZ;
            for (u32 i = 0; i<sceneList.size(); i++)
            {
                CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL || pScanSn->GetPointCloud() == NULL)
                    continue;
                if (bVisible && !pScanSn->isVisible())
                {
                    continue;
                }
                PointCloud* pcd = pScanSn->GetPointCloud();
                // 新相对坐标、绝对坐标、显示坐标转化方法 [2014/03/20 危迟]
                // 获取场景节点中的点云在显示时是否对相对坐标进行了转换
                bool bTrans = pScanSn->IsTrans();

                // 获取相对坐标转绝对坐标的模型
                CBursaWolfModel absModel = pScanSn->GetModel();

                // 获取相对坐标转显示坐标的模型
                CBursaWolfModel renderModel = pScanSn->GetRenderModel();

                u32 loopCount = pcd->getLoopCount();
                for (u32 i = 0;i<loopCount;i++)
                {
                    bool bRet = pScanSn->isLoopInView(i);
                    if (!bRet)
                    {
                        continue;
                    }

                    hdVector<PointXYZIPRGBA>& pts = pcd->getLoop(i);
                    for (u32 n = 0;n < pts.size();n++)
                    {
                        PointXYZIPRGBA& pt = *(pts._Myfirst + n);
                        if (!pt.isValid())
                        {
                            continue;
                        }

                        // 此处根据该节点显示模式进行判断
                        if (((pScanSn->GetShowStyle() == SHOW_UNSELECT) && pt.isSelected())||
                            ((pScanSn->GetShowStyle()  == SHOW_SELECT) && !pt.isSelected()))
                        {
                            continue;
                        }

                        /*if (bselect&&pt.isSelected())
                        {
                        continue;	
                        }*/

                        if (bTrans)
                        {			
                            ptX = pt.x;
                            ptY = pt.y;
                            ptZ = pt.z;
                            renderModel.Translate(ptX,ptY,ptZ);
                            coord.set((float)ptX,(float)ptY,(float)ptZ);
                        }
                        else
                        {
                            coord.set(pt.x,pt.y,pt.z);
                        }

                        screenPos = sceneMng->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
                        if (!irrRect.isPointInside(screenPos))
                            continue;

                        srcDx = screenPos.X - srcX;
                        srcDy = screenPos.Y - srcY;
                        srcDist = srcDx * srcDx + srcDy * srcDy;
                        if (srcDist < srcMinDist)
                        {
                            srcMinDist = srcDist;
                            ptX = pt.x;
                            ptY = pt.y;
                            ptZ = pt.z;
                            absModel.Translate(ptX,ptY,ptZ);
                            x = ptX;
                            y = ptY;
                            z = ptZ;
                        }

                    }//for (u32 n = 0;n < pts.size();n++)
                }//for (u32 i = 0;i<loopCount;i++)
            }//for (u32 i = 0; i<sceneList.size(); i++)

            return srcMinDist < U32_MAX;
        }

		// 根据平面位置获取点云中的点及其绝对坐标（朱立雄 2017-3-10）
		bool ISceneView::Get3DPosFromPcd(PointXYZIPRGBA& pt, f64& x, f64& y, f64& z, 
			int srcX, int srcY, int tol, bool bVisible, bool bSelected)
		{
			// 场景管理器不存在时退出
			ISceneManager* sceneMng = GetSceneManager();
			if (!sceneMng)
			{
				return false;
			}

			// 获取点云列表
			core::array<ISceneNode*> sceneList;
			GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

			u32 srcMinDist = U32_MAX;
			u32 srcDist  = U32_MAX;
			for (u32 i = 0; i<sceneList.size(); i++)
			{
				CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(sceneList[i]);						
				if(pScanSn == NULL || pScanSn->GetPointCloud() == NULL)
				{
					continue;
				}

				if (bVisible && !pScanSn->isVisible())
				{
					continue;
				}

				PointXYZIPRGBA ptTmp;// 临时点

				// 临时坐标
				f64 xTmp = 0.0; 
				f64 yTmp = 0.0;
				f64 zTmp = 0.0;	

				// 调用点云内部接口
				srcDist = pScanSn->Get3DPosFromScrPos(ptTmp, xTmp, yTmp, zTmp, srcX, srcY, tol, bVisible, bSelected);

				// 更新坐标
				if (srcDist < srcMinDist)
				{
					srcMinDist = srcDist;
					pt = ptTmp;
					x = xTmp;
					y = yTmp;
					z = zTmp;

				}
			}//	for (u32 i = 0; i<sceneList.size(); i++)

			return srcMinDist < U32_MAX;
		}

        // 根据平面位置获取海量点云坐标-- -- chy
        bool ISceneView::Get3DPosFromSeaPcd(
            PointXYZIPRGBA&pt,
            f64& x,f64& y,f64& z,	// 返回点云坐标值
            int srcX,int srcY,		// 屏幕坐标
            int tol ,bool bVisible , bool bSelected
            )
        {

            // 容差非法、返回
            if (tol < 0)
            {
                return false;
            }

            x = 0.0;
            y = 0.0;
            z = 0.0;

            // 场景管理器不存在时退出
            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            // 获取海量点云列表
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneList);

            u32 srcMinDist = U32_MAX;
            u32 srcDist  = U32_MAX;
            for (u32 i = 0; i<sceneList.size(); i++)
            {
                CHdSeaDataSceneNode* pScanSn = dynamic_cast<CHdSeaDataSceneNode*>(sceneList[i]);						
                if(pScanSn == NULL || pScanSn->GetPointCloud() == NULL)
                {
                    continue;
                }

                if (bVisible && !pScanSn->isVisible())
                {
                    continue;
                }

                PointXYZIPRGBA ptTmp;// 临时点

                // 临时坐标
                f64 xTmp = 0.0; 
                f64 yTmp = 0.0;
                f64 zTmp = 0.0;	

                // 调用点云内部接口
                srcDist = pScanSn->Get3DPosFromScrPos(ptTmp,xTmp,yTmp,zTmp,srcX,srcY,tol,bVisible,bSelected);

				//PointXYZIPRGBA ptTmp1;// 临时点

				//// 临时坐标
				//f64 xTmp1 = 0.0; 
				//f64 yTmp1 = 0.0;
				//f64 zTmp1 = 0.0;	
				//srcDist = pScanSn->Get3DPosFromScrPos1(ptTmp1,xTmp1,yTmp1,zTmp1,srcX,srcY,tol,bVisible,bSelected);

				//double dxx,dyy,dzz;
				//dxx = xTmp - xTmp1;
				//dyy = yTmp - yTmp1;
				//dzz = zTmp - zTmp1;
				//int test = 0;

                // 更新坐标
                if (srcDist < srcMinDist)
                {
                    srcMinDist = srcDist;
                    pt = ptTmp;
                    x = xTmp;
                    y = yTmp;
                    z = zTmp;

                }
            }//	for (u32 i = 0; i<sceneList.size(); i++)

            return srcMinDist < U32_MAX;

        }


        // 根据平面位置获取临时点云坐标-----chy
        bool ISceneView::Get3DPosFromTmpPcd(
            PointXYZIPRGBA&outPt,       // 相对点
            f64& x,f64& y,f64& z,	// 返回点云坐标值
            int srcX,int srcY,		// 屏幕坐标
            int tol 	// 屏幕容差
            )
        {

            // 屏幕容差不合法、返回
            if (tol < 0)
            {
                return false;
            }

            // 场景管理器不存在、返回
            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            // 构造缓冲区
            irr::core::recti irrRect;
            irrRect.LowerRightCorner.set(srcX + tol,srcY + tol);
            irrRect.UpperLeftCorner.set(srcX - tol,srcY - tol);

            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_PART_SCAN_POINT,sceneList);

            core::vector3df coord;
            core::position2di screenPos;

            irr::core::aabbox3df loopBox;
            u32 srcMinDist = U32_MAX;
            u32 srcDx = 0;
            u32 srcDy = 0;
            u32 srcDist = 0;

            for (u32 i = 0; i<sceneList.size(); i++)
            {
                CScanPartPointsSceneNode* pPartSN = dynamic_cast<CScanPartPointsSceneNode*>(sceneList[i]);						

                if(pPartSN == NULL)
                {
                    continue;
                }

                // 数据不存在返回
                const vector<hnCommon::hnPointXYZIF> pPoints = pPartSN->GetPartPoints();

				int nPtsCount = pPoints.size();

                if (nPtsCount <= 0)
                {
                    continue;
                }

                for (int i=0; i<nPtsCount; i++)
                {
					hnCommon::hnPointXYZIF pt = pPoints.at(i);
                    coord.set(pt.x,pt.y,pt.z);

                    screenPos = sceneMng->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
                    if (!irrRect.isPointInside(screenPos))
                    {
                        continue;
                    }

                    srcDx = screenPos.X - srcX;
                    srcDy = screenPos.Y - srcY;
                    srcDist = srcDx * srcDx + srcDy * srcDy;
                    if (srcDist < srcMinDist)
                    {
                        srcMinDist = srcDist;
						outPt.x = pt.x;
						outPt.y = pt.y;
						outPt.z = pt.z;
                       // outPt = pt;
                        x = outPt.x;
                        y = outPt.y;
                        z = outPt.z;

                        // 转为绝对坐标
                        m_transModel.AntiTranslate(x,y,z);

                    }
                }
            }//for (u32 i = 0; i<sceneList.size(); i++)

            return srcMinDist < U32_MAX;
        }

        bool ISceneView::Get3DPosFromTmpPcd(
            PointXYZIPRGBA&outPt,       // 相对点
            int srcX,int srcY,		// 屏幕坐标
            int tol	/* 屏幕容差*/)
        {
            // 屏幕容差不合法、返回
            if (tol < 0)
            {
                return false;
            }

            // 场景管理器不存在、返回
            ISceneManager* sceneMng = GetSceneManager();
            if (!sceneMng)
            {
                return false;
            }

            // 构造缓冲区
            irr::core::recti irrRect;
            irrRect.LowerRightCorner.set(srcX + tol,srcY + tol);
            irrRect.UpperLeftCorner.set(srcX - tol,srcY - tol);

            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_PART_SCAN_POINT,sceneList);

            core::vector3df coord;
            core::position2di screenPos;

            irr::core::aabbox3df loopBox;
            u32 srcMinDist = U32_MAX;
            u32 srcDx = 0;
            u32 srcDy = 0;
            u32 srcDist = 0;

            for (u32 i = 0; i<sceneList.size(); i++)
            {
                CScanPartPointsSceneNode* pPartSN = dynamic_cast<CScanPartPointsSceneNode*>(sceneList[i]);						

                if(pPartSN == NULL)
                {
                    continue;
                }

                // 数据不存在返回
                const vector<hnCommon::hnPointXYZIF> pPoints = pPartSN->GetPartPoints();

				int nPtsCount = pPoints.size();

				if (nPtsCount <= 0)
				{
					continue;
				}

                for (int i=0; i<nPtsCount; i++)
                {
					hnCommon::hnPointXYZIF pt = pPoints.at(i);
                    coord.set(pt.x,pt.y,pt.z);

                    screenPos = sceneMng->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
                    if (!irrRect.isPointInside(screenPos))
                    {
                        continue;
                    }

                    srcDx = screenPos.X - srcX;
                    srcDy = screenPos.Y - srcY;
                    srcDist = srcDx * srcDx + srcDy * srcDy;
                    if (srcDist < srcMinDist)
                    {
                        srcMinDist = srcDist;
						outPt.x = pt.x;
						outPt.y = pt.y;
						outPt.z = pt.z;
                        //outPt = pt;
                    }
                }
            }//for (u32 i = 0; i<sceneList.size(); i++)

            return srcMinDist < U32_MAX;
        }

        //! 2013/9/30 蔡红云 针对点云生成扫描仪模型节点
        IObjectSceneNode* ISceneView::AddLsModel( PointCloud* pcd)
        {

            // 相应的地面站点云增加扫描仪模型
            if (pcd->m_header.offsetX < 100000.0 && pcd->m_header.offsetY < 100000.0)
            {
                string pcdFilepath = pcd->GetPointCloudPath();
                string filePath = getCurrentDir();
                // model path
                filePath += "scan.3ds";

                //// 测试代码，打开房子模型
                //string filePath = "E:\\Test4.3DS";

                // 如果文件不存在返回
                if (_access(filePath.c_str(), 04) != 0)
                    return NULL;
                CIMeshSceneNode* pMeshSN = NULL;
                core::array<ISceneNode*> msnList;
                // 获取当前场景中的扫描仪模型节点
                GetSceneManager()->getSceneNodesFromType(ESNT_MESH_MODEL, msnList);
                // 如果 为0， 则创建
                if (msnList.size() == 0)
                {
                    if (!pMeshSN)
                    {
                        IMesh* mesh = GetSceneManager()->getMesh(filePath.c_str());
                        if (!mesh)
                        {
                            return NULL;
                        }
                        pMeshSN = new CIMeshSceneNode(
                            GetIrrDevice()->getSceneManager()->getRootSceneNode(), 
                            GetIrrDevice()->getSceneManager(),108, mesh);

                        pMeshSN->setMaterialFlag(video::EMF_LIGHTING, true);
                        pMeshSN->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true);
                        pMeshSN->setMaterialFlag(video::EMF_GOURAUD_SHADING,true);
                        // hide the back face  ,show solid effect 
                        pMeshSN->setMaterialFlag(video::EMF_BACK_FACE_CULLING,false);
                        pMeshSN->setDebugDataVisible(irr::scene::EDS_OFF);
                        ITriangleSelector* triangleSelector = GetSceneManager()->createTriangleSelector(mesh, pMeshSN);
                        pMeshSN->setTriangleSelector(triangleSelector);
                        //pMeshSN->setMaterialTexture(0,driver->getTexture(imagePath.c_str()));
                        pMeshSN->SetView(this);
                        // 进行坐标转换模型设置，使扫描仪模型处于适当位置
                        CBursaWolfModel* pBursaModel = GetTransModel();
                        CBursaWolfModel ivtModel = pBursaModel->getAntiModel();
                        CBursaWolfModel MeshabsModel = pMeshSN->GetModel()*ivtModel;
                        pMeshSN->SetModel(MeshabsModel);
                        pMeshSN->drop();
                        triangleSelector->drop();
                    }
                }
                // 进行遍历，以点云路径为唯一标识判断每个点云是否对应一个扫描仪节点
                for (unsigned int j=0; j< msnList.size(); j++)
                {
                    CIMeshSceneNode* pMeshSN1 = dynamic_cast<CIMeshSceneNode*>(msnList[j]);
                    string strPcd = pcd->GetPointCloudPath();
                    string strMesh = pMeshSN1->getPcdFilePath();
                    std::transform(strPcd.begin(), strPcd.end(), strPcd.begin(),tolower);
                    std::transform(strMesh.begin(), strMesh.end(), strMesh.begin(),tolower);
                    // 如果不存在就重新创建。。
                    if ( strMesh != strPcd)
                    {
                        if (!pMeshSN)
                        {
                            IMesh* mesh = GetSceneManager()->getMesh(filePath.c_str());
                            if (!mesh)
                            {
                                return NULL;
                            }
                            pMeshSN = new CIMeshSceneNode(
                                GetIrrDevice()->getSceneManager()->getRootSceneNode(), 
                                GetIrrDevice()->getSceneManager(),108, mesh);

                            pMeshSN->setMaterialFlag(video::EMF_LIGHTING, true);
                            pMeshSN->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true);
                            pMeshSN->setMaterialFlag(video::EMF_GOURAUD_SHADING,true);
                            // hide the back face  ,show solid effect 
                            pMeshSN->setMaterialFlag(video::EMF_BACK_FACE_CULLING,false);
                            pMeshSN->setDebugDataVisible(irr::scene::EDS_OFF);
                            ITriangleSelector* triangleSelector = GetSceneManager()->createTriangleSelector(mesh, pMeshSN);
                            pMeshSN->setTriangleSelector(triangleSelector);
                            //pMeshSN->setMaterialTexture(0,driver->getTexture(imagePath.c_str()));
                            pMeshSN->SetView(this);
                            // 进行坐标转换，使扫描仪模型处于适当位置
                            CBursaWolfModel* pBursaModel = GetTransModel();
                            CBursaWolfModel ivtModel = pBursaModel->getAntiModel();
                            CBursaWolfModel MeshabsModel = pMeshSN->GetModel()*ivtModel;
                            pMeshSN->SetModel(MeshabsModel);
                            pMeshSN->drop();
                            triangleSelector->drop();
                        }
                    }
                }

                // 加个光源，防止模型太暗
                core::vector3df lightPos;

                //  Move the model to right position
                f64 offx = pcd->m_header.offsetX;
                f64 offy = pcd->m_header.offsetY;
                f64 offz = pcd->m_header.offsetZ;

                f64 lightx = offx + 100;
                f64 lighty = offy + 100;
                f64 lightz = offz + 100;
                if (pMeshSN == NULL)
                {
                    return NULL;
                }
                CBursaWolfModel transModel = pMeshSN->GetModel();
                transModel.Translate(lightx, lighty, lightz);
                transModel.Translate(offx, offy, offz);
                irr::core::vector3df newPos((float)offx, (float)offy, (float)offz);
                lightPos.set((float)lightx, (float)lighty, (float)lightz);
                pMeshSN->setPosition(newPos);

                // 可适当旋转角度
                irr::core::vector3df newRotation;
                newRotation.X = 0.0f;
                newRotation.Y = 0.0f;
                newRotation.Z = 0.0f;
                pMeshSN->setRotation(newRotation);

                // 设置文件路径..进行节点标识
                pMeshSN->setPcdFilePath(pcdFilepath);

                // 默认不显示扫描仪
                pMeshSN->setVisible(m_bShowScanModel);
                // add light 
                GetSceneManager()->addLightSceneNode(0, lightPos,
                    video::SColorf(0.5f, 0.5f, 180.f), 300);
                GetSceneManager()->setAmbientLight(video::SColorf(0.3f,0.3f,180.f));

                GetSceneManager()->setShadowColor(video::SColor(255, 0, 0, 0));
                return pMeshSN;

            }

            return NULL;
        }

        void ISceneView::ShowIntensityRender(BOOL bShow)
        {


            core::array<ISceneNode*> sceneList;
            core::array<ISceneNode*> seaSceneList;// 海量点云节点

            GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,seaSceneList);

            for(unsigned int i = 0;i< seaSceneList.size();i++)
            {
                CHdSeaDataSceneNode* scanNode = dynamic_cast<CHdSeaDataSceneNode*>(seaSceneList[i]);
                if(scanNode)
                {
                    scanNode->ShowIntensityRender(bShow);
                }
            }

            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);
            if (sceneList.empty())
            {
                GetSceneManager()->getSceneNodesFromType(ESNT_CLASSIFY_PTD, sceneList);
                if (sceneList.empty())
                    return;
            }

            for(unsigned int i = 0;i< sceneList.size();i++)
            {
                CScanSceneNode* scanNode = dynamic_cast<CScanSceneNode*>(sceneList[i]);
                if(scanNode)
                {
                    scanNode->ShowIntensityRender(bShow);
                }
            }
        }

        // 获取场景中显示的扫描仪模型数
        int ISceneView::getScnshowcnt()
        { 	
            // 获取场景中的扫描仪模型
            core::array<ISceneNode*> msnList;
            GetSceneManager()->getSceneNodesFromType(ESNT_MESH_MODEL, msnList);
            if ( msnList.size() <= 0)
            {
                return 0;
            }
            int cnt = 0;
            for (int j = 0; j!=msnList.size();j++)
            {
                ISceneNode* pnode = msnList[j];
                if (pnode->isVisible())
                {
                    cnt++;
                }
            }
            return cnt;
        }
        PointCloud *ptCloud = NULL;
        // 获取场景中点云的偏移量的最小值 蔡红云 2013/11/17
        void ISceneView::getAllScanSnodeExt(f32& offsetx, f32& offsety, f32& offsetz)
        {

            // 获取场景中的点云节点
            core::array<ISceneNode*> scnList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT, scnList);
            if ( scnList.size() <= 0)
            {
                return ;
            }

            for (int j = 0; j!=scnList.size();j++)
            {
                CScanSceneNode* pNode =(CScanSceneNode*)scnList[j];

                if (pNode)
                {
                    // 获取点云
                    ptCloud = pNode->GetPointCloud();
                    // 第一次初始化offsetx、offsety、offsetz
                    if (j == 0)
                    {
                        offsetx = (float)ptCloud->m_header.offsetX;
                        offsety = (float)ptCloud->m_header.offsetY;
                        offsetz = (float)ptCloud->m_header.offsetZ;
                    }
                    // 然后 获取offsetx、offsety、offsetz最小值
                    else
                    {
                        offsetx = min(offsetx, (float)ptCloud->m_header.offsetX);
                        offsetx = min(offsety, (float)ptCloud->m_header.offsetY);
                        offsetx = min(offsetz, (float)ptCloud->m_header.offsetZ);
                    }

                }

            }

        }

        // 统计场景中所有点云的按XYZ渲染坐标范围 蔡红云 2013/11/17
        void ISceneView::statAllScanSndeStatCoord()
        {
            //! 按Z渲染的最大最小值
            float fMinHeight = 0.f;
            float fMaxHeight = 0.f;

            //! 按X渲染的最大最小值
            float fMinCorX = 0.f;
            float fMaxCorX = 0.f;

            //! 按Y渲染的最大最小值
            float fMinCorY = 0.f;
            float fMaxCorY = 0.f;

            // 获取场景中的点云节点
            core::array<ISceneNode*> scnList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT, scnList);
            if ( scnList.size() <= 0)
            {
                return ;
            }
            for (int j = 0; j!=scnList.size();j++)
            {
                CScanSceneNode* pNode =dynamic_cast<CScanSceneNode*>(scnList[j]);

                // 添加对点云数据的有效性判断 yangfeng 2014/2/28
                if (pNode != NULL && pNode->GetPointCloud() != NULL)
                {
                    // 单个点云统计坐标
                    pNode->StatCoord();

                    // 最大、最小值临时变量
                    f32 xmintmp = 0.0;
                    f32 xmaxtmp = 0.0;
                    f32 ymintmp = 0.0;
                    f32 ymaxtmp = 0.0;
                    f32 zmintmp = 0.0;
                    f32 zmaxtmp = 0.0;
                    pNode->GetZmaxmin(zmaxtmp, zmintmp);
                    pNode->GetYmaxmin(ymaxtmp, ymintmp);
                    pNode->GetXmaxmin(xmaxtmp, xmintmp);

                    // 获取点云所对应的变换模型
                    CBursaWolfModel mode = pNode->GetModel();
                    CBursaWolfModel* pBursaModel = GetTransModel();
                    // 获取点云
                    PointCloud* pcd = pNode->GetPointCloud();

                    // 第一次赋初值
                    if (j == 0 )
                    {
                        fMinHeight = zmintmp;
                        fMaxHeight = zmaxtmp;
                        fMinCorX = xmintmp;
                        fMaxCorX = xmaxtmp;
                        fMinCorY = ymintmp;
                        fMaxCorY = ymaxtmp;
                    }
                    /*	只采取第一个点云的范围作为高程范围
                    更新最大值、最小值*/
                    else
                    {
                        fMinHeight = min(fMinHeight, zmintmp);
                        fMaxHeight = max(fMaxHeight, zmaxtmp);
                        fMinCorX = min(fMinCorX, xmintmp );
                        fMaxCorX = max(fMaxCorX, xmaxtmp);
                        fMinCorY = min(fMinCorY, ymintmp);
                        fMaxCorY = max(fMaxCorY, ymaxtmp);
                    }

                }

            }

            // 最后对视图中的所有CScanSceneNode重置渲染XYZ最大值最小值
            for (int i = 0; i!=scnList.size();i++)
            {
                CScanSceneNode* pNode =(CScanSceneNode*)scnList[i];
                if (pNode)
                {
                    pNode->SetXmaxmin(fMaxCorX, fMinCorX);
                    pNode->SetYmaxmin(fMaxCorY, fMinCorY);
                    pNode->SetZmaxmin(fMaxHeight,fMinHeight);

                }
            }
        }

        // 统计海量点云按XYZ渲染坐标范围 蔡红云 2015/11/7
        void ISceneView::statAllSeaSndeStatCoord()
        {
            //按Z渲染的最大最小值
            float fMinHeight = 0.f;
            float fMaxHeight = 0.f;

            //按X渲染的最大最小值
            float fMinCorX = 0.f;
            float fMaxCorX = 0.f;

            //按Y渲染的最大最小值
            float fMinCorY = 0.f;
            float fMaxCorY = 0.f;

            // 获取场景中的点云节点
            core::array<ISceneNode*> scnList;
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT, scnList);
            if ( scnList.size() <= 0)
            {
                return ;
            }
            for (int j = 0; j!=scnList.size();j++)
            {
                CHdSeaDataSceneNode* pNode = dynamic_cast<CHdSeaDataSceneNode*>(scnList[j]);

                if (pNode != NULL && pNode->GetPointCloud() != NULL)
                {

                    // 最大、最小值临时变量
                    f32 xmintmp = 0.0;
                    f32 xmaxtmp = 0.0;
                    f32 ymintmp = 0.0;
                    f32 ymaxtmp = 0.0;
                    f32 zmintmp = 0.0;
                    f32 zmaxtmp = 0.0;
                    pNode->GetZmaxmin(zmaxtmp, zmintmp);
                    pNode->GetYmaxmin(ymaxtmp, ymintmp);
                    pNode->GetXmaxmin(xmaxtmp, xmintmp);


                    // 第一次赋初值
                    if (j == 0 )
                    {
                        fMinHeight = zmintmp;
                        fMaxHeight = zmaxtmp;
                        fMinCorX = xmintmp;
                        fMaxCorX = xmaxtmp;
                        fMinCorY = ymintmp;
                        fMaxCorY = ymaxtmp;
                    }
                    /*	只采取第一个点云的范围作为高程范围
                    更新最大值、最小值*/
                    else
                    {
                        fMinHeight = min(fMinHeight, zmintmp);
                        fMaxHeight = max(fMaxHeight, zmaxtmp);
                        fMinCorX = min(fMinCorX, xmintmp );
                        fMaxCorX = max(fMaxCorX, xmaxtmp);
                        fMinCorY = min(fMinCorY, ymintmp);
                        fMaxCorY = max(fMaxCorY, ymaxtmp);
                    }

                }

            }

            // 最后对视图中的所有CHdSeaDataSceneNode重置渲染XYZ最大值最小值
            for (int i = 0; i!=scnList.size();i++)
            {
                CHdSeaDataSceneNode* pNode =dynamic_cast<CHdSeaDataSceneNode*>(scnList[i]);
                if (pNode)
                {
                    pNode->SetXmaxmin(fMaxCorX, fMinCorX);
                    pNode->SetYmaxmin(fMaxCorY, fMinCorY);
                    pNode->SetZmaxmin(fMaxHeight,fMinHeight);
                    pNode->SetRenderStyle(pNode->GetRenderStyle());// 此处仅更新下数据

                }
            }

        }

        // 统计视图中所有点云外包盒的范围xyz最小值  蔡红云 2013/11/13
        void ISceneView:: StateAllBoxMin(f32& xmin, f32& ymin, f32& zmin)
        {

            //! 按Z渲染的最大最小值
            float fMinHeight;
            float fMaxHeight;
            //! 按X渲染的最大最小值
            float fMinCorX;
            float fMaxCorX;
            //! 按Y渲染的最大最小值
            float fMinCorY;
            float fMaxCorY;

            // 获取场景中的点云节点
            core::array<ISceneNode*> scnList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT, scnList);
            if ( scnList.size() <= 0)
            {
                return ;
            }
            for (int j = 0; j!=scnList.size();j++)
            {
                CScanSceneNode* pNode =(CScanSceneNode*)scnList[j];
                if (pNode )
                {

                    // 最大、最小值临时变量
                    f32 xmintmp = 0.0;
                    f32 xmaxtmp = 0.0;
                    f32 ymintmp = 0.0;
                    f32 ymaxtmp = 0.0;
                    f32 zmintmp = 0.0;
                    f32 zmaxtmp = 0.0;


                    // 获取点云
                    PointCloud* pcd = pNode->GetPointCloud();

                    core::aabbox3d<f32> box = pNode->getBoundingBox();
                    // 新相对坐标、绝对坐标、显示坐标转化方法 [2014/03/20 危迟]
                    // 获取场景节点中的点云在显示时是否对相对坐标进行了转换
                    bool bTrans = pNode->IsTrans();

                    // 获取相对坐标转绝对坐标的模型
                    CBursaWolfModel absModel = pNode->GetModel();

                    // 获取相对坐标转显示坐标的模型
                    CBursaWolfModel renderModel = pNode->GetRenderModel();

                    // 获取box的中心
                    irr::core::vector3df center = box.getCenter();
                    irr::core::vector3df extent = box.getExtent();

                    if (bTrans && scnList.size() > 1)
                    {
                        renderModel.Translate(center.X,center.Y,center.Z);
                        box.MinEdge.X = center.X - extent.X/2.f;
                        box.MaxEdge.X = center.X + extent.X/2.f;
                        box.MinEdge.Y = center.Y - extent.Y/2.f;
                        box.MaxEdge.Y = center.Y + extent.Y/2.f;
                        box.MinEdge.Z = center.Z - extent.Z/2.f;
                        box.MaxEdge.Z = center.Z + extent.Z/2.f;

                    } 

                    zmintmp = box.MinEdge.Z;
                    zmaxtmp = box.MaxEdge.Z;
                    xmintmp = box.MinEdge.X;
                    xmaxtmp = box.MaxEdge.X;
                    ymintmp = box.MinEdge.Y;
                    ymaxtmp = box.MaxEdge.Y;

                    // 第一次赋初值
                    if (j == 0 )
                    {
                        fMinHeight = zmintmp;
                        fMaxHeight = zmaxtmp;
                        fMinCorX = xmintmp;
                        fMaxCorX = xmaxtmp;
                        fMinCorY = ymintmp;
                        fMaxCorY = ymaxtmp;
                    }
                    // 更新最大值、最小值
                    else
                    {
                        fMinHeight = min(fMinHeight, zmintmp );
                        fMaxHeight = max(fMaxHeight, zmaxtmp);
                        fMinCorX = min(fMinCorX, xmintmp);
                        fMaxCorX = max(fMaxCorX, xmaxtmp);
                        fMinCorY = min(fMinCorY, ymintmp);
                        fMaxCorY = max(fMaxCorY, ymaxtmp);
                    }

                }

            }
            // 更新x\y\z最小值
            xmin = fMinCorX;
            ymin = fMinCorY;
            zmin = fMinHeight;

        }

        //! 根据平面位置获取显示坐标 基于openGL拾取坐标原理快速获取显示坐标 
        bool ISceneView::Get3DRenderPosFromScrPos( f32& x,f32& y,f32& z, int srcX,int srcY, int tol /*= 3*/ )
        {
            ESCENE_NODE_TYPE sent_type = ESNT_SCAN_POINT;
            if (IsIncludeSceneNode(ESNT_HD_SEADATA_POINT))
            {
                sent_type = ESNT_HD_SEADATA_POINT;
            }

			if (IsIncludeSceneNode(ESNT_PART_SCAN_POINT))
			{
				sent_type = ESNT_PART_SCAN_POINT;
			}

            core::array<ISceneNode*> sceneList;

            GetSceneManager()->getSceneNodesFromType(sent_type,sceneList);

            //分类视图中的三维视图的场景结点为ESNT_CLASSIFY_PTD
            if (!sceneList.size())
            {
                GetSceneManager()->getSceneNodesFromType(ESNT_CLASSIFY_PTD, sceneList);
            }

			// 获取部分点云视图展示;
			if (!sceneList.size())
			{
				GetSceneManager()->getSceneNodesFromType(ESNT_PART_SCAN_POINT, sceneList);
			}

            int count = 0;
            int sumMin = 2*(m_height + m_width);
            f32 tmpX,tmpY,tmpZ;

            for(unsigned int i = 0;i< sceneList.size();i++)
            {
                ISceneCollisionManager* pCln = NULL;
                if (sent_type == ESNT_SCAN_POINT)
                {
                    CScanSceneNode* scanNode = dynamic_cast<CScanSceneNode*>(sceneList[i]);
                    if(!scanNode)
                    {
                        continue;
                    }

                    pCln = scanNode->getSceneManager()->getSceneCollisionManager();
                }
                else if (sent_type == ESNT_HD_SEADATA_POINT)
                {
                    CHdSeaDataSceneNode* scanNode = dynamic_cast<CHdSeaDataSceneNode*>(sceneList[i]);
                    if(!scanNode)
                    {
                        continue;
                    }

                    pCln = scanNode->getSceneManager()->getSceneCollisionManager();
                }
				else if (sent_type == ESNT_PART_SCAN_POINT)
				{
					CScanPartPointsSceneNode* scanNode = dynamic_cast<CScanPartPointsSceneNode*>(sceneList[i]);
					if (!scanNode)
					{
						continue;
					}

					pCln = scanNode->getSceneManager()->getSceneCollisionManager();
				}

                if (!pCln)
                {
                    return false;
                }

                for (int i = -tol; i<= tol;i++)
                {
                    for (int j = -tol; j<= tol;j++)
                    {
                        if(srcX + i >=0 && srcX + i < m_width && srcY + j >= 0 && srcY + j < m_height
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
                return true;
            }
            else
            {
                return false;
            }
        }

        //! 根据平面位置获取显示坐标 基于openGL拾取坐标原理快速获取显示坐标(hlz点云数据)
        bool ISceneView::Get3DRenderPosFromScrPosSea(f32& x,f32& y,f32& z,int srcX,int srcY,int tol)
        {
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneList);

            //分类视图中的三维视图的场景结点为ESNT_CLASSIFY_PTD
            if (!sceneList.size())
            {
                GetSceneManager()->getSceneNodesFromType(ESNT_CLASSIFY_PTD, sceneList);
            }


            int count = 0;
            int sumMin = 2*(m_height + m_width);
            float tmpX,tmpY,tmpZ;

            for(unsigned int i = 0;i< sceneList.size();i++)
            {
                CHdSeaDataSceneNode* scanNode = dynamic_cast<CHdSeaDataSceneNode*>(sceneList[i]);
                if(!scanNode)
                {
                    continue;
                }

                ISceneCollisionManager* pCln = scanNode->getSceneManager()->getSceneCollisionManager();
                if (!pCln)
                {
                    return false;
                }

                for (int i = -tol; i<= tol;i++)
                {
                    for (int j = -tol; j<= tol;j++)
                    {
                        if(srcX + i >=0 && srcX + i < m_width && srcY + j >= 0 && srcY + j < m_height
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
                return true;
            }
            else
            {
                return false;
            }
        }

        IrrlichtDevice* InitialIrrDevice(HWND hwnd)
        {
            video::SExposedVideoData videodata(0);
            SIrrlichtCreationParameters param;

            param.DeviceType = EIDT_WIN32;
            param.DriverType = video::EDT_OPENGL;
            param.WindowId = reinterpret_cast<void*>(hwnd);

            IrrlichtDevice* irrDevice = NULL;

            // createDeviceEx函数内部存在new，可能会创建失败，尝试try catch -- add by zhubo 2014.07.07
            try
            {
                irrDevice = irr::createDeviceEx(param);
            }
            catch (...)
            {
                return NULL;
            }

            return irrDevice;
        }

        int ISceneView::GetPcdRenderState()
        {
            // 暂不考虑多测站
            CScanSceneNode* pPcdSN = dynamic_cast<CScanSceneNode*>(GetSceneManager()->getSceneNodeFromType(ESNT_SCAN_POINT));
            if (pPcdSN)
            {
                int nSimpleLev = pPcdSN->GetRenderSimpleLevel();
                if (nSimpleLev >= 0)
                {
                    return nSimpleLev;
                }
            }

            // 没有点云对象
            return -1;
        }

        void ISceneView::UpdateIntensityRender()
        {
            core::array<ISceneNode*> sceneList;
            GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);

            if (!sceneList.size())
            {
                return;
            }

            for(unsigned int i = 0;i< sceneList.size();i++)
            {
                CScanSceneNode* scanNode = dynamic_cast<CScanSceneNode*>(sceneList[i]);

                if(!scanNode)
                {
                    continue;
                }

                scanNode->ReStatIntensity();
            }
        }

        void ISceneView::SetRadioLegendVisiable( bool bShow )
        {
            if (!m_irrDevice)
            {
                return;
            }
            CHdRadioLegendSceneNode* radioLegNode = dynamic_cast<CHdRadioLegendSceneNode*>(GetSceneManager()->getSceneNodeFromType(ESNT_RADIO_LEGEND));
            if (radioLegNode == NULL)
            {
                //radioLegNode = new CHdRadioLegendSceneNode(m_irrDevice->getSceneManager()->getRootSceneNode(),m_irrDevice->getSceneManager(),1000);
                radioLegNode = new CHdRadioLegendSceneNode(m_irrDevice->getSceneManager()->getRootSceneNode(),m_irrDevice->getSceneManager(),1000);
                radioLegNode->SetView(this);
                radioLegNode->drop();
            }
            radioLegNode->setVisible(bShow);
            m_bShowRadioLegend = bShow;
        }

        float ISceneView::GetDisplayScale()
        {

            video::IVideoDriver* driver = GetSceneManager()->getVideoDriver();

            if (!driver)
            {		
                return 0.0;
            }

            irr::scene::ICameraSceneNode* camera = GetSceneManager()->getActiveCamera();

            if (!camera)
            {
                return 0.0;
            }

            // 获得屏幕大小(像素值)
            core::dimension2di size = (core::dimension2di)driver->getCurrentRenderTargetSize();

            // 由相机pos及target构成的射线与近平面相交点
            core::plane3df nearPlane = camera->getViewFrustum()->planes[camera->getViewFrustum()->VF_NEAR_PLANE];
            core::vector3df NearDown;
            core::vector3df NearUp;


            // 获得相机与指定像素处屏幕交线
            core::line3df lineNear = GetSceneManager()->getSceneCollisionManager()->getRayFromScreenCoordinates(core::vector2di(size.Width / 2, size.Height / 2));
            core::line3df lineNearTarg = GetSceneManager()->getSceneCollisionManager()->getRayFromScreenCoordinates(core::vector2di(size.Width / 2 + m_nLegend, size.Height / 2));

            // 求得交点
            nearPlane.getIntersectionWithLine(lineNear.start,lineNear.getVector().normalize(),NearDown);
            nearPlane.getIntersectionWithLine(lineNearTarg.start,lineNearTarg.getVector().normalize(),NearUp);

            // 由三维坐标距离值计算1cm对应屏幕坐标像素差
            float dDist = sqrtf(pow(NearDown.X - NearUp.X,2) + 
                pow(NearDown.Y - NearUp.Y,2) + pow(NearDown.Z - NearUp.Z,2));

            return dDist*100;

        }

        // ****地理距离到屏幕像素距离转换****
        int ISceneView::GetScreenDist(f32 geoDist)
        {
            f32 dispScale = GetDisplayScale();
            return hd::hd_round32( geoDist * 100 * m_nLegend / dispScale);
        }

        void ISceneView::SetChangeLevl(bool flag)
        {
            core::array<ISceneNode*> arrSeaNode;

            GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,arrSeaNode);

            for (int i = 0; i<arrSeaNode.size();i++)
            {
                CHdSeaDataSceneNode* pSeaNode = dynamic_cast<CHdSeaDataSceneNode*>(arrSeaNode[i]);

                if (pSeaNode)
                {
                    pSeaNode->SetChangeLvl(flag);
                }
            }
        }

        void ISceneView::OnloadTitlePanoData()
        {
            SendMsgToWindowsView(WM_USER_LOAD_TITLEPANO, NULL, NULL);
        }

        //! 设置视图刷新全部，屏蔽调用强制刷新
        void ISceneView::SetViewRenderAllNodeWithoutRefresh( bool bAll, bool bCreateScreenShot /*= true*/ )
        {
            if (!m_irrDevice)
            {
                return;
            }

            if (m_viewType == E_HVT_3D || m_viewType == E_HVT_MULTISCAN3D || m_viewType == E_HVT_SKETCH_ISCAN3D || m_viewType == E_HVT_PANO ||
                m_viewType == E_HVT_MLS3D || m_viewType == E_HVT_ORTHO3D || m_viewType == E_HVT_FACADEEDIT)
            {
                CScreenShotSceneNode* pscsNode = dynamic_cast<CScreenShotSceneNode*>(GetSceneManager()->getSceneNodeFromType(ESNT_HD_SCREENSHOT));
                if (pscsNode == NULL)
                {
                    //初始化时新建空白纹理
                    video::IVideoDriver* driver = m_irrDevice->getVideoDriver();
                    const core::dimension2d<u32> screensize = core::dimension2d<u32>(m_width,m_height);
                    video::ITexture* pTexture = driver->addTexture(screensize,"CurrentScreenShot",ECF_A8R8G8B8);

                    // 添加纹理失败，造成截屏SN失败返回
                    if (!pTexture)
                    {
                        m_bRenderAll = bAll;
                        m_bNeedScreenShot = false;
                        RefreshViewBySendMessage();
                        return;
                    }
                    pscsNode = new CScreenShotSceneNode(m_irrDevice->getSceneManager()->getRootSceneNode(),m_irrDevice->getSceneManager(),201,pTexture);
                    pscsNode->SetView(this);
                    pscsNode->drop();
                }
                else 
                {
                    if (bCreateScreenShot)		//控制绘制多段线时，不生成图像
                    {
                        video::IVideoDriver* driver = m_irrDevice->getVideoDriver();
                        video::IImage* image = driver->createScreenShot();
                        if (!image)
                        {
                            m_bRenderAll = bAll;
                            m_bNeedScreenShot = false;
                            RefreshViewBySendMessage();
                            ReloadData();
                            return;
                        }

                        video::ITexture* pTexture = driver->addTexture("CurrentScreenShot",image);
                        if (!pTexture)
                        {
                            if (image)
                            {
                                image->drop();
                            }

                            m_bRenderAll = bAll;
                            m_bNeedScreenShot = false;
                            RefreshViewBySendMessage();
                            ReloadData();
                            return;
                        }

                        pscsNode->UpdateTexture(pTexture);

                        // createScreenShot实际是new了一个新的image,需要drop
                        if (image)
                        {
                            image->drop();
                        }
                    }
                }
                m_bRenderAll = bAll;
            }
        }
		
		//! 添加一个多边形过滤器到所有点云结点
		void ISceneView::AddPolygonFilter(E_Select_Mode select_type, irr::core::vector2df* screen_pts, int pt_count)
		{
			const irr::core::rect<s32>& view_rect = GetSceneManager()->getVideoDriver()->getViewPort();
			irr::core::dimension2du view_size(view_rect.getWidth(), view_rect.getHeight());
			
			ICameraSceneNode* camera = GetSceneManager()->getActiveCamera();
/*			const irr::core::matrix4& prj_matrix = camera->getProjectionMatrix();
			const irr::core::matrix4& view_matrix = camera->getViewMatrix();		*/	
			irr::core::matrix4 prj_matrix = camera->getProjectionMatrix();
			irr::core::matrix4 view_matrix = camera->getViewMatrix();

			// 为视图中的每个点云添加一个过滤器
			irr::core::array< ISceneNode* > snArray;
			GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT, snArray);
			unsigned int nPointCloudCount = snArray.size();
			for (unsigned int i = 0; i < nPointCloudCount; i ++)
			{
				CScanSceneNode* pScanSN = dynamic_cast< CScanSceneNode* >(snArray[i]);
				if (pScanSN && pScanSN->isVisible() && pScanSN->GetPointCloud())
				{
					// 点云局部坐标到视图坐标的变换矩阵，注意要对 model 的矩阵进行转置
					CBursaWolfModel& renderModel = pScanSN->GetRenderModel();
					irr::core::matrix4 renderMatrix;
					for (int iRow = 0; iRow < 4; iRow ++)
					{
						for (int iCol = 0; iCol < 4; iCol ++)
						{
							renderMatrix(iRow, iCol) = (float)renderModel.m_matrix[iCol][iRow];
						}
					}

					// 合并三个矩阵，得到点云局部坐标到投影坐标的变换参数
					irr::core::matrix4 unionMatrix = prj_matrix * view_matrix * renderMatrix;

					// 创建过滤器
					hdFilterSelectPoly *filter = new hdFilterSelectPoly(select_type, 
						screen_pts, pt_count, view_size, unionMatrix.pointer());

					pScanSN->GetPointCloud()->GetFilterManager()->addFilter(filter);
				}
			}

			// 为所有海量点云添加过滤器
			snArray.clear();
			GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT, snArray);
			nPointCloudCount = snArray.size();
			for (unsigned int i = 0; i < nPointCloudCount; i ++)
			{
				CHdSeaDataSceneNode* pScanSN = dynamic_cast< CHdSeaDataSceneNode* >(snArray[i]);
				if (pScanSN && pScanSN->isVisible() && pScanSN->GetPointCloud())
				{
					// 点云局部坐标到视图坐标的变换矩阵，注意要对 model 的矩阵进行转置
					CBursaWolfModel& renderModel = pScanSN->GetRenderModel();
					irr::core::matrix4 renderMatrix;
					for (int iRow = 0; iRow < 4; iRow ++)
					{
						for (int iCol = 0; iCol < 4; iCol ++)
						{
							renderMatrix(iRow, iCol) = (float)renderModel.m_matrix[iCol][iRow];
						}
					}

					// 合并三个矩阵，得到点云局部坐标到投影坐标的变换参数
					irr::core::matrix4 unionMatrix = prj_matrix * view_matrix * renderMatrix;

					// 创建过滤器
					hdFilterSelectPoly *filter = new hdFilterSelectPoly(select_type, 
						screen_pts, pt_count, view_size, unionMatrix.pointer());

					pScanSN->GetPointCloud()->GetFilterManager()->addFilter(filter);
				}
			}
		}

		//! 添加一个圆形过滤器到所有点云结点
		void ISceneView::AddCircleFilter(hd::ptcloud::E_Select_Mode select_type, irr::core::vector2df center, f32 radius)
		{
			const irr::core::rect<s32>& view_rect = GetSceneManager()->getVideoDriver()->getViewPort();
			irr::core::dimension2du view_size(view_rect.getWidth(), view_rect.getHeight());

			ICameraSceneNode* camera = GetSceneManager()->getActiveCamera();
/*			const irr::core::matrix4& prj_matrix = camera->getProjectionMatrix();
			const irr::core::matrix4& view_matrix = camera->getViewMatrix();	*/	
			irr::core::matrix4 prj_matrix = camera->getProjectionMatrix();
			irr::core::matrix4 view_matrix = camera->getViewMatrix();

			// 为视图中的每个点云添加一个过滤器
			irr::core::array< ISceneNode* > snArray;
			GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT, snArray);
			unsigned int nPointCloudCount = snArray.size();
			for (unsigned int i = 0; i < nPointCloudCount; i ++)
			{
				CScanSceneNode* pScanSN = dynamic_cast< CScanSceneNode* >(snArray[i]);
				if (pScanSN && pScanSN->isVisible() && pScanSN->GetPointCloud())
				{
					// 点云局部坐标到视图坐标的变换矩阵，注意要对 model 的矩阵进行转置
					CBursaWolfModel& renderModel = pScanSN->GetRenderModel();
					irr::core::matrix4 renderMatrix;
					for (int iRow = 0; iRow < 4; iRow ++)
					{
						for (int iCol = 0; iCol < 4; iCol ++)
						{
							renderMatrix(iRow, iCol) = (float)renderModel.m_matrix[iCol][iRow];
						}
					}

					// 合并三个矩阵，得到点云局部坐标到投影坐标的变换参数
					irr::core::matrix4 unionMatrix = prj_matrix * view_matrix * renderMatrix;

					// 创建过滤器
					hdFilterSelectCircle *filter = new hdFilterSelectCircle(select_type, 
						center, radius, view_size, unionMatrix.pointer());

					pScanSN->GetPointCloud()->GetFilterManager()->addFilter(filter);
				}
			}

			snArray.clear();
			GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT, snArray);
			nPointCloudCount = snArray.size();
			for (unsigned int i = 0; i < nPointCloudCount; i ++)
			{
				CHdSeaDataSceneNode* pScanSN = dynamic_cast< CHdSeaDataSceneNode* >(snArray[i]);
				if (pScanSN && pScanSN->isVisible() && pScanSN->GetPointCloud())
				{
					// 点云局部坐标到视图坐标的变换矩阵，注意要对 model 的矩阵进行转置
					CBursaWolfModel& renderModel = pScanSN->GetRenderModel();
					irr::core::matrix4 renderMatrix;
					for (int iRow = 0; iRow < 4; iRow ++)
					{
						for (int iCol = 0; iCol < 4; iCol ++)
						{
							renderMatrix(iRow, iCol) = (float)renderModel.m_matrix[iCol][iRow];
						}
					}

					// 合并三个矩阵，得到点云局部坐标到投影坐标的变换参数
					irr::core::matrix4 unionMatrix = prj_matrix * view_matrix * renderMatrix;

					// 创建过滤器
					hdFilterSelectCircle *filter = new hdFilterSelectCircle(select_type, 
						center, radius, view_size, unionMatrix.pointer());

					pScanSN->GetPointCloud()->GetFilterManager()->addFilter(filter);
				}
			}
		}

		//! 移除所有点云结点的所有过滤器
		void ISceneView::ClearFilters()
		{
			irr::core::array< ISceneNode* > snArray;
			GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT, snArray);
			unsigned int nPointCloudCount = snArray.size();
			for (unsigned int i = 0; i < nPointCloudCount; i ++)
			{
				CScanSceneNode* pScanSN = dynamic_cast< CScanSceneNode* >(snArray[i]);
				if (pScanSN && /*pScanSN->isVisible() &&*/ pScanSN->GetPointCloud())
				{
					pScanSN->GetPointCloud()->GetFilterManager()->removeFilterAll();
				}
			}

			snArray.clear();
			GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT, snArray);
			nPointCloudCount = snArray.size();
			for (unsigned int i = 0; i < nPointCloudCount; i ++)
			{
				CHdSeaDataSceneNode* pScanSN = dynamic_cast< CHdSeaDataSceneNode* >(snArray[i]);
				if (pScanSN && /*pScanSN->isVisible() &&*/ pScanSN->GetPointCloud())
				{
					pScanSN->GetPointCloud()->GetFilterManager()->removeFilterAll();
				}
			}
		}

		//! 刷新显示
		hd::u32 ISceneView::updateForFilter()
		{
			// 总选中点数
			hd::u32 nSelectedCount = 0;

			core::array<ISceneNode*> sceneList;
			GetSceneManager()->getSceneNodesFromType(ESNT_SCAN_POINT,sceneList);
			if (!sceneList.empty())
			{
				for(u32 i = 0; i< sceneList.size(); i++)
				{
					CScanSceneNode* pcd_scene_node = static_cast<CScanSceneNode*>(sceneList[i]);
					if(pcd_scene_node)
					{				
						nSelectedCount += pcd_scene_node->updateForFilter();
					}
				}
			}

			// 海量点云
			core::array<ISceneNode*> sceneListSea;
			GetSceneManager()->getSceneNodesFromType(ESNT_HD_SEADATA_POINT,sceneListSea);
			if (sceneListSea.size() > 0)
			{
				for (unsigned int i = 0;i< sceneListSea.size();i++)
				{
					CHdSeaDataSceneNode* pSeaDataSN = (CHdSeaDataSceneNode*)sceneListSea[i];
					if (pSeaDataSN)
					{
						nSelectedCount += pSeaDataSN->updateForFilter();
					}
				}
			}

			return nSelectedCount;
		}
    }
}

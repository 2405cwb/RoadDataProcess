/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : CTriangleSceneNode.cpp
相关文件     : CTriangleSceneNode.h
文件实现功能 : 草图视图测站中心三角形
作者         : 张阳
版本         : 软件部，张阳
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2016/03/12   1.0       张阳                 创建
</PRE>
*******************************************************************************/

#include "StdAfx.h"
#include "CTriangleSceneNode.h"
#include "COpenGLExtensionHandler.h"
#include "..\hdPointCloud\hdSysSetting.h"
#include "hd3DCamera.h"
#include <math.h>
#include "..\hdFramework\hdCommandDef.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

//#define SAMPLE_COUNT (500000.0)
using namespace hd::fm;


namespace hd
{
    namespace scene
    {
#pragma region 256彩色值映射表
        static COLORREF triangleColorList[] = 
        {
            RGB( 0xFF, 0xFF, 0xFF ),
            RGB( 0xFF, 0xFF, 0xFF ),
            RGB( 0xFF, 0xFF, 0xFF ),
            RGB( 0xFF, 0xFF, 0xFF ),
            RGB( 0xFF, 0xFF, 0xFF ),
            RGB( 0xFF, 0xFF, 0xFF ),
            RGB( 0xFF, 0xBF, 0xFF ),
            RGB( 0xFF, 0xFF, 0xFF ),
			//RGB( 0xFF, 0x00, 0x00 ),
			//RGB( 0x00, 0x64, 0x00 ),
			//RGB( 0xFF, 0xD7, 0x00 ),
			//RGB( 0x94, 0x00, 0xD3 ),
			//RGB( 0x7F, 0xFF, 0x00 ),
			//RGB( 0x2F, 0x4F, 0x4F ),
			//RGB( 0x00, 0xBF, 0xFF ),
			//RGB( 0xD2, 0x69, 0x1E ),
        };
#pragma  endregion
        CTriangleSceneNode::CTriangleSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id)
            : IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id)
        {
            m_ColorIndex = -1;
            m_Size = 0;
            m_OriginalDirection = core::vector3df(0, 0, 0);
            m_RotateDirection = core::vector3df(0, 0, 0);
            m_StrScanName = "";
            m_Material.Wireframe = false;
            m_Material.Lighting = false;
            m_SketchScanEditType = SKETCH_SCAN_NULL;
            m_bIsRender = true;
            for (int i = 0; i < 3; i ++)
            {
                m_TriangleList[i].X = 0;
                m_TriangleList[i].Y = 0;
            }

            setAutomaticCulling(irr::scene::EAC_OFF);
        }


        CTriangleSceneNode::~CTriangleSceneNode(void)
        {
        }

        // 注册SceneNode
        void CTriangleSceneNode::OnRegisterSceneNode()
        {
            if (IsVisible)
            {
                SceneManager->registerNodeForRendering(this);
            }

            ISceneNode::OnRegisterSceneNode();
        }

        // 设置三角形各个顶点的坐标
        void CTriangleSceneNode::SetPoint(core::vector2d<s32> centerPoint, core::vector3df dOriDir, core::vector3df dRotDir, int tSize)
        {
            if (tSize > 0)
            {
                switch (GetCameraPos())
                {
                case E_CP_TOP:
                    // 设置坐标值
                    for (int i = 0; i < 3; i++)
                    {
                        m_TriangleList[i].X = (cos(dOriDir.Z + dRotDir.Z + 5 * core::PI64 / 6 + core::PI64 * i * 2 / 3)) * tSize + centerPoint.X;
                        m_TriangleList[i].Y = (sin(dOriDir.Z + dRotDir.Z + 5 * core::PI64 / 6 + core::PI64 * i * 2 / 3)) * tSize + centerPoint.Y;
                    }
                    break;
                case E_CP_RIGHT:
                    // 设置坐标值
                    for (int i = 0; i < 3; i++)
                    {
                        m_TriangleList[i].X = (cos(dOriDir.X + dRotDir.X + 5 * core::PI64 / 6 + core::PI64 * i * 2 / 3)) * tSize + centerPoint.X;
                        m_TriangleList[i].Y = (sin(dOriDir.X + dRotDir.X + 5 * core::PI64 / 6 + core::PI64 * i * 2 / 3)) * tSize + centerPoint.Y;
                    }
                    break;
                case E_CP_BACK:
                    // 设置坐标值
                    for (int i = 0; i < 3; i++)
                    {
                        m_TriangleList[i].X = (cos(dOriDir.Y + dRotDir.Y + 5 * core::PI64 / 6 + core::PI64 * i * 2 / 3)) * tSize + centerPoint.X;
                        m_TriangleList[i].Y = (sin(dOriDir.Y + dRotDir.Y + 5 * core::PI64 / 6 + core::PI64 * i * 2 / 3)) * tSize + centerPoint.Y;
                    }
                    break;
                default:
                    return;
                    break;
                }

                // 设置角度
                m_OriginalDirection = dOriDir;
                m_RotateDirection = dRotDir;
            }
        }

        // 设置三角形名字(测站名)
        void CTriangleSceneNode::SetName(string strName)
        {
            m_StrScanName = strName;
        }

        // 得到三角形名字(测站名)
        string CTriangleSceneNode::GetName()
        {
            return m_StrScanName;
        }

        // 设置颜色索引
        void CTriangleSceneNode::SetColorIndex(int tIndex)
        {
			int arr_size = sizeof(triangleColorList)/4;

            m_ColorIndex = tIndex % arr_size;
        }

        // 渲染
        void CTriangleSceneNode::render()
        {
            video::IVideoDriver* driver = SceneManager->getVideoDriver();

            irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();
            ISceneView* pView = (ISceneView*)(m_pView);

            if ((!pView) || (m_pView->IsViewRenderAllNode() != m_bIsRender))
            {
                return;
            }

            // 
            if (!camera || !driver ||! m_pView || !pView)
            {
                return;
            }


            // 仅在草图视图下显示
            if (/*m_pView->GetViewType() != E_HVT_3D 
                && m_pView->GetViewType() != E_HVT_MULTISCAN3D
                && */m_pView->GetViewType() != E_HVT_SKETCH_ISCAN3D
                /*&& m_pView->GetViewType() != E_HVT_ORTHO3D*/)
            {
                return;
            }

            glEnable(GL_LINE_SMOOTH); //启用线抗锯齿，边缘会降低其alpha值
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
            glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

            driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
            ////m_Material.Thickness = 5.f;
            driver->setMaterial(m_Material);
            core::vector3df pt(m_TriangleCenterPoint.X, m_TriangleCenterPoint.Y, m_TriangleCenterPoint.Z);
            CBursaWolfModel* pViewModel = pView->GetTransModel();

            pViewModel->Translate(pt.X, pt.Y, pt.Z);

            //////m_renderModel.Translate(tempX, tempY, tempZ);
            ////glVertex3d(tempX, tempY, tempZ);

            core::position2di scrPos = pView->GetSceneManager()->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(pt);

            core::vector2d<s32> centerPoint(scrPos.X, scrPos.Y);
            COLORREF color;
            SetPoint(centerPoint, m_OriginalDirection, m_RotateDirection, m_Size);
            double originalDirection, rotateDirection;

            // 测站被选中
            if (m_SketchScanEditType == SKETCH_SCAN_TRANSLATION || m_SketchScanEditType == SKETCH_SCAN_ROTATE)
            {
                color = RGB( 0x00,0x00,0xFF);
            }
            // 测站未被选中
            else
            {
                color = triangleColorList[m_ColorIndex];
            }

            m_Color = video::SColor(155, GetRValue(color), GetGValue(color), GetBValue(color));

            // 绘制三角形
            driver->draw2DTriangle(m_TriangleList, 3, m_Color);
            core::position2d<s32> posBegin;

            switch (GetCameraPos())
            {
            case E_CP_TOP:
                originalDirection = m_OriginalDirection.Z;
                rotateDirection = m_RotateDirection.Z;
                break;
            case E_CP_RIGHT:
                originalDirection = m_OriginalDirection.X;
                rotateDirection = m_RotateDirection.X;
                break;
            case E_CP_BACK:
                originalDirection = m_OriginalDirection.Y;
                rotateDirection = m_RotateDirection.Y;
                break;
            default:
                return;
                break;
            }

            posBegin.X = cos(originalDirection + core::PI64 / 2) * 100 + scrPos.X;
            posBegin.Y = sin(originalDirection + core::PI64 / 2) * 100 + scrPos.Y;
            m_Material.Thickness = 1;
            driver->setMaterial(m_Material);

            if (m_SketchScanEditType == SKETCH_SCAN_ROTATE)
            {
                // 绘制方向箭头
                driver->draw2DLine(scrPos, posBegin, video::SColor(155, 255, 255, 255));

                // 绘制旋转外圆和旋转角
                SColor zClor(155, 0, 0, 255);
                driver->draw2DCircle(centerPoint, 100, zClor);
                core::position2d<s32> posEnd;
                posEnd.X = cos(originalDirection + rotateDirection + core::PI64 / 2) * 100 + scrPos.X;
                posEnd.Y = sin(originalDirection + rotateDirection + core::PI64 / 2) * 100 + scrPos.Y;

                driver->draw2DLine(scrPos, posEnd, video::SColor(155, 255, 255, 255));
                driver->draw2DEllipse(posEnd, 6, 6, video::SColor(150, 116, 0, 0)); // 实心圆

                // 绘制角度弧
                double beginAngle = -originalDirection - core::PI64 / 2;
                double endAngle = -originalDirection - rotateDirection - core::PI64 / 2;
                double drawBeginAngle;
                double drawEndAngle;

                // 转化到0 ~ 2PI
                if (beginAngle > 0)
                {
                    while (beginAngle >= core::PI * 2)
                    {
                        beginAngle -= core::PI * 2;
                    }
                }
                else
                {
                    while (beginAngle < 0)
                    {
                        beginAngle += core::PI * 2;
                    }
                }

                if (endAngle > 0)
                {
                    while (endAngle >= core::PI * 2)
                    {
                        endAngle -= core::PI * 2;
                    }
                }
                else
                {
                    while (endAngle < 0)
                    {
                        endAngle += core::PI * 2;
                    }
                }

                // 绘制角度弧形
                if (endAngle > beginAngle)
                {
                    if (endAngle - beginAngle > core::PI)
                    {
                        drawBeginAngle = endAngle;
                        drawEndAngle = beginAngle + core::PI * 2;
                    }
                    else
                    {
                        drawBeginAngle = beginAngle;
                        drawEndAngle = endAngle;
                    }
                }
                else if (endAngle < beginAngle)
                {
                    if (beginAngle - endAngle > core::PI)
                    {
                        drawBeginAngle = beginAngle;
                        drawEndAngle = endAngle + core::PI * 2;
                    }
                    else
                    {
                        drawBeginAngle = endAngle;
                        drawEndAngle = beginAngle;
                    }
                }
                else
                {
                    return;
                }

                driver->draw2DArc(centerPoint, drawBeginAngle, drawEndAngle, 20, rotateDirection * 180 / core::PI);

                // 角度值位置
                double angle = (drawBeginAngle + drawEndAngle) / 2;
                core::vector2di anglePos(centerPoint.X + cos(angle) * 50,
                    centerPoint.Y - sin(angle) * 50);

                // 绘制角度值
                // 获得中文字体
                gui::IGUIFont* pGUIFont = getSceneManager()->GetChineseFont();

                core::rect<s32> rect(anglePos, core::dimension2d<s32>(40, 40));

                // 转换宽字节
                char strDist[32];
                if (rotateDirection > 0)
                {
                    angle = 360 - rotateDirection * 180 / core::PI;
                }
                else
                {
                    angle = 0;
                }

                if (angle > 180)
                {
                    angle = 360 - angle;
                }

                sprintf(strDist, "%0.2f°", angle);
                wchar_t* wStrLabel = pGUIFont->CharToWchar(strDist);

                // 绘制
                pGUIFont->draw(wStrLabel, rect, video::SColor(155, 255, 255, 255), true, true);
            }
            else
            {
                driver->draw2DLine(scrPos, posBegin, video::SColor(155, 255, 255, 255));
                driver->draw2DEllipse(posBegin, 6, 6, m_Color); // 实心圆
            }

            // 绘制测站名
            // 获得中文字体
            gui::IGUIFont* pGUIFont = getSceneManager()->GetChineseFont();

            core::rect<s32> rect(scrPos + core::position2di(5, -2), core::dimension2d<s32>(40, 40));

            // 转换宽字节
            char strDist[32];
            sprintf(strDist, m_StrScanName.data());
            wchar_t* wStrLabel = pGUIFont->CharToWchar(strDist);

            // 绘制
            pGUIFont->draw(wStrLabel, rect, video::SColor(155, 255, 255, 255), true, true);

            //绘制完关闭抗锯齿
            glDisable(GL_BLEND); 
            glDisable(GL_LINE_SMOOTH); 
        }

        // 
        video::SMaterial& CTriangleSceneNode::getMaterial(u32 i)
        {
            return m_Material;
        }

        // 
        u32 CTriangleSceneNode::getMaterialCount() const
        {
            return 1;
        }

        // 
        void CTriangleSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
        {
        }

        // 
        void CTriangleSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
        {
        }

        // 
        const core::aabbox3d<f32>& CTriangleSceneNode::getBoundingBox() const
        {
            return m_box;
        }

        // 设置三角形中心点
        void CTriangleSceneNode::SetTriangleCenterPoint(f32 x, f32 y, f32 z)
        {
            m_TriangleCenterPoint.X = x;
            m_TriangleCenterPoint.Y = y;
            m_TriangleCenterPoint.Z = z;
        }

        // 设置三角形中心点
        void CTriangleSceneNode::SetTriangleCenterPoint(core::vector3df triangleCenter)
        {
            m_TriangleCenterPoint = triangleCenter;
        }

        // 设置三角形外接圆半径
        void CTriangleSceneNode::SetSize(int tSize/* = 12*/)
        {
            m_Size = tSize;
        }

        // 得到旋转小圆的中心
        core::vector2di CTriangleSceneNode::GetOperBallScreenPos()
        {
            ISceneView* pView = (ISceneView*)(m_pView);
            core::vector2di posEnd;

            if (!pView)
            {
                return posEnd;
            }

            core::vector3df pt(m_TriangleCenterPoint.X, m_TriangleCenterPoint.Y, m_TriangleCenterPoint.Z);
            CBursaWolfModel* pViewModel = pView->GetTransModel();

            pViewModel->Translate(pt.X, pt.Y, pt.Z);

            //////m_renderModel.Translate(tempX, tempY, tempZ);
            ////glVertex3d(tempX, tempY, tempZ);

            core::position2di scrPos = pView->GetSceneManager()->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(pt);

            switch (GetCameraPos())
            {
            case E_CP_TOP:
                posEnd.X = cos(m_OriginalDirection.Z + core::PI64 / 2) * 100 + scrPos.X;
                posEnd.Y = sin(m_OriginalDirection.Z + core::PI64 / 2) * 100 + scrPos.Y;
            	break;
            case E_CP_RIGHT:
                posEnd.X = cos(m_OriginalDirection.X + core::PI64 / 2) * 100 + scrPos.X;
                posEnd.Y = sin(m_OriginalDirection.X + core::PI64 / 2) * 100 + scrPos.Y;
                break;
            case E_CP_BACK:
                posEnd.X = cos(m_OriginalDirection.Y + core::PI64 / 2) * 100 + scrPos.X;
                posEnd.Y = sin(m_OriginalDirection.Y + core::PI64 / 2) * 100 + scrPos.Y;
                break;
            default:
                break;
            }
            return posEnd;
        }

        // 得到旋转中心的屏幕坐标
        core::vector2di CTriangleSceneNode::GetScanCenterScreenPos()
        {
            ISceneView* pView = (ISceneView*)(m_pView);

            if (!pView)
            {
                return core::vector2di(-1, -1);
            }

            core::vector3df pt(m_TriangleCenterPoint.X, m_TriangleCenterPoint.Y, m_TriangleCenterPoint.Z);
            CBursaWolfModel* pViewModel = pView->GetTransModel();

            pViewModel->Translate(pt.X, pt.Y, pt.Z);

            //////m_renderModel.Translate(tempX, tempY, tempZ);
            ////glVertex3d(tempX, tempY, tempZ);

            core::position2di scrPos = pView->GetSceneManager()->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(pt);

            return scrPos;
        }

        ENUM_CAMERA_POSITION CTriangleSceneNode::GetCameraPos()
        {
            if (m_pView)
            {
                CHd3DCamera* p3DCamera = m_pView->Get3DCameraTool();

                if (p3DCamera == NULL)
                {
                    return E_CP_USER_POSITION;
                }

                ENUM_CAMERA_POSITION cameraType = p3DCamera->GetCameraPosType();

                // 右视图
                if (cameraType == E_CP_LEFT || cameraType == E_CP_RIGHT)
                {
                    return E_CP_RIGHT;
                }
                else if (cameraType == E_CP_FRONT || cameraType == E_CP_BACK)
                {
                    return E_CP_BACK;
                }
                else/* if (cameraType == E_CP_TOP || cameraType == E_CP_BOTTOM )*/
                {
                    // 默认俯视图
                    return E_CP_TOP;
                }
            }

            return E_CP_USER_POSITION;
        }
    }
}
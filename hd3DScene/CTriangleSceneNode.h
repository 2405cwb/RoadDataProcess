/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : CTriangleSceneNode.h
相关文件     : CTriangleSceneNode.cpp
文件实现功能 : 草图视图测站中心三角形
作者         : 张阳
版本         : 软件部，张阳
--------------------------------------------------------------------------------
备注         : 等边三角形
            /↑\
           / | \
          /  |  \
         /___|___\
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2016/03/12   1.0       张阳                 创建
</PRE>
*******************************************************************************/

#pragma once
#include "..\hd3DEngine\include\irrlicht.h"
#include "IObjectSceneNode.h"
#include "..\hdCommon\HD2DPoint.h"

using namespace irr;
using namespace hd;

namespace hd
{
    namespace scene
    {
        //点显示的类型
        enum ENUM_SKETCH_SCAN_EDIT_TYPE
        {
            SKETCH_SCAN_NULL,            // 无任何操作
            SKETCH_SCAN_TRANSLATION,     // 平移
            SKETCH_SCAN_ROTATE           // 旋转
        };

        class HD3DSCENE_API CTriangleSceneNode :
            public IObjectSceneNode
        {
        public:
            // 构造函数
            CTriangleSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id);

            // 虚析构函数
            virtual ~CTriangleSceneNode(void);

            //*************************ISceneNode 基类接口*******************************//

            // 注册草图视图等边三角形SceneNode
            virtual void OnRegisterSceneNode();

            // 渲染等边三角形
            virtual void render();

            //virtual const core::aabbox3d<f32>& getBoundingBox() const;

            // 获取渲染材质
            virtual video::SMaterial& getMaterial(u32 i);

            // 得到材质总数
            virtual u32 getMaterialCount() const;

            // 序列化属性
            virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options = 0) const;

            // 反序列化属性
            virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options = 0);

            // 得到SceneNode类型, 自定义类型
            virtual ESCENE_NODE_TYPE getType() const { return ESNT_TRIANGLE; }

            // 获取包围盒
            virtual const core::aabbox3d<f32>& getBoundingBox() const;

            //virtual ISceneNode* clone(ISceneNode* newParent = 0, ISceneManager* newManager = 0);

            //*****************************自身对象接口***************************************//
            // 设置三角形各个顶点的坐标(默认12像素)
            void SetPoint(core::vector2d<s32> centerPoint, core::vector3df dOriDir, core::vector3df dRotDir, int tSize);

            // 设置三角形中心点
            void SetTriangleCenterPoint(f32 x, f32 y, f32 z);

            // 设置三角形中心点
            void SetTriangleCenterPoint(core::vector3df triangleCenter);

            // 得到三角形中心点
            core::vector3df GetTriangleCenterPoint() { return m_TriangleCenterPoint; }

            // 设置三角形名字(测站名)
            void SetName(string strName);

            // 得到三角形名字(测站名)
            string GetName();

            // 设置颜色索引
            void SetColorIndex(int tIndex);

            // 设置点云路径
            void SetScanPath(string strScanPath) { m_strScanPath = strScanPath; }

            // 得到点云路径
            string GetScanPath() { return m_strScanPath; }

            void SetOriginalDirection(core::vector3df originalDirection) { m_OriginalDirection = originalDirection; }

            core::vector3df GetOriginalDirection() { return m_OriginalDirection; }

            void SetRotateDirection(core::vector3df rotateDirection) { m_RotateDirection = rotateDirection; }

            core::vector3df GetRotateDirection() { return m_RotateDirection; }

            // 设置三角形外接圆半径
            void SetSize(int tSize = 20);

            // 操作状态
            ENUM_SKETCH_SCAN_EDIT_TYPE GetSketchEditType() { return m_SketchScanEditType; }

            // 设置操作状态
            void SetSketchEditType(ENUM_SKETCH_SCAN_EDIT_TYPE bSketchEditType) { m_SketchScanEditType = bSketchEditType; }

            // 设置是否在截屏时刷新
            void SetRender(bool isRender) { m_bIsRender = isRender; }

            // 得到旋转小圆的中心
            core::vector2di GetOperBallScreenPos();

            // 得到旋转中心的屏幕坐标
            core::vector2di GetScanCenterScreenPos();

        protected:
            // 获取三维视图相机位置
            ENUM_CAMERA_POSITION GetCameraPos();

        private:
            // 颜色索引信息
            int                     m_ColorIndex;

            // 材质
            video::SMaterial        m_Material;

            // 等边三角形大小(像素信息, 外接圆半径)
            int                     m_Size;

            // 等边三角形方向(旋转方向, 仅绕Z轴旋转)
            core::vector3df         m_RotateDirection;

            // 初始方向
            core::vector3df         m_OriginalDirection;

            // 等边三角形颜色
            //COLORREF                m_Color;

            // 测站名
            string                  m_StrScanName;

            // 三角形顶点
            core::vector2d<f32>     m_TriangleList[3];

            // 三角形中心点(绝对坐标)
            core::vector3df         m_TriangleCenterPoint;

            // 包围盒
            core::aabbox3df         m_box;

            // 点云路径
            string                  m_strScanPath;

            // 操作状态
            ENUM_SKETCH_SCAN_EDIT_TYPE    m_SketchScanEditType;

            // 旋转平移是否刷新三角形[张阳 2016/04/13]
            bool m_bIsRender;
        };
    }
}


/*! hd3DView.h
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : hd3DView.h
相关文件     : 
文件实现功能 : 实现三维视图封装 
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/06/20   1.0      龚书林    
2013/02/27   2.0      危迟				   移植重构
2013/07/05   2.1      危迟                 新增视角属性
2013/07/11   2.2      危迟                 删除视角属性，添加获取视角方法
2013/09/04   2.3      危迟				   增加严格获取视角方法
2015/12/25   2.4      张阳                 合并HdSxDomPcd3DView.h(hdApplication), 用于点云显示正射影像
</PRE>
*******************************************************************************/
#pragma once
//#ifndef _HD3DVIEW_H_
//#define _HD3DVIEW_H_

#include "stdafx.h"
#include "ISceneView.h"
#include "IObjectSceneNode.h"
// #include "HdSx3DView.h"
#include "include\BasicObject\hdGeoRaster.h"

// 引用命名空间
using namespace std;
using namespace irr;
using namespace irr::scene;
using namespace hd;
using namespace hd::fm;

// 加载影像的方式
enum E_LOAD_IMAGE_TYPE
{
    E_LIT_OVERVIEW =0,					// 用金字塔的形式显示策略，适合较大的影像数据
    E_LIT_IMAGE	=1						// 用普通图像的方式显示，即先将图像读到内存，再绘制，适合较小的影像数据
};

namespace hd
{
	namespace scene
	{
		class COverViewDataManger;

		class HD3DSCENE_API CHd3DView :
			public ISceneView
		{
        private:
            COverViewDataManger* m_OverViewDataManger;  // 金字塔数据管理
            Chd2DBoundingBoxd m_viewBoundingBox;        // 包围盒
            double m_RWCellSize;                        // 地理坐标分辨率,默认x,y方向上一致
		protected:
			//! 是否显示target点
			bool m_bShowTarget;
			//! 旋转中心点
			core::vector3df m_rotCentre;
			// ！蔡红云 2013/8/2 相机位置
			core::vector3df m_camPos;

			ENUM_HD_BOX_OPERATION m_eBoxOperate;  
			
		//public:
			//! 相机目标点
			core::vector3df m_oriTarget;
			//! 相机源点
			core::vector3df m_oriPosition;
			
			//! 相机视角
			// ENUM_HD_VIEW_ANGLE m_eAngle;
        protected:
            // 通过路径添加影像，并制定影像添加的方式,默认采用金字塔
            bool AddImagePic(const char* strImage,E_LOAD_IMAGE_TYPE eLoadImageType =E_LIT_OVERVIEW);

            //// 判断是否含有金字塔数据
            //bool IsOverView()
            //{
            //	return m_OverViewDataManger->IsOverView();
            //}

            // 通过ID,删除影像
            bool DeleteImage(const char* strImageID);

            // 通过ID,删除节点
            bool DeleteSN(const char* strID);

		public:
			CHd3DView(void);
			virtual ~CHd3DView(void);

            // 根据视锥获得当前视图显示范围以更新视图包围盒m_viewBoundingBox
            void UpdateBoxByView();

            // 添加图片
            bool AddImagePic(const CHdGeoRaster* pImageBuffer, const char* strID);

            // 得到当前视图的百分比，如10%，获取的结果为0.01
            double GetCurPecent();

			//! 设置相机目标点
			void SetOriTarget(core::vector3df oriTarget){ m_oriTarget = oriTarget; }

			//! 设置相机源点
			void SetOriPosition(core::vector3df oriPosition){ m_oriPosition = oriPosition; }
						
			//! 设置最近显示距离
			void SetNearValue(f32 zn);

			//! 设置最远显示距离
			void SetFarValue(f32 zf);

			//! 获取最近显示距离
			f32 GetNearValue();

			//! 获取最远显示距离
			f32 GetFarValue();

            //! 获得包围盒
            Chd2DBoundingBoxd GetBoundingBox() { return m_viewBoundingBox; }

            //! 获得OverViewDataManager
            COverViewDataManger* GetOverViewDataManager() { return m_OverViewDataManger; }

			//! 获取旋转中心
			core::vector3df GetRotateCentre() {return m_rotCentre;}

			//! 设置旋转中心
			void SetRotateCentre(core::vector3df rotCentre);

			//! 2013/8/2 蔡红云 设置相机目标
			void SetCamTarPos(core::vector3df tar);
		
			//! 设置旋转中心点到视图数据中心
			void SetRotate2DataCenter();

			//! 缩放至当前场景
			void ZoomToCurrentScene(float extent);

			// 缩放到指定范围
			void ZoomToSpcExtent(const core::aabbox3df& extent);

			//! 设置相机的位置的目标，使相机能够看到视图中的所有对象 
			// 默认情况下为俯视图视角查看全部 也可传递参数不以俯视角 [2013/11/20 危迟]
			virtual void ZoomToFullExtent(bool bVertical = true);

			//  测试加载海量点云节点
			virtual void ZoomToFullExtent1(bool bVertical = true);
			
			//! 获取视图数据范围
			core::aabbox3df GetViewExtent();

			// 获取视角 容差值1度
			ENUM_HD_VIEW_ANGLE GetViewAngle();

			// 获取视角 容差值0度 严格角度
			ENUM_HD_VIEW_ANGLE GetViewAngleStrict();

			// 获取box操作类型
			ENUM_HD_BOX_OPERATION GetBoxOperationType() { return m_eBoxOperate;}

			// 设置box操作类型
			void SetBoxOperationType(ENUM_HD_BOX_OPERATION type) { m_eBoxOperate = type; }

			//! 移除指定的SceneNode
			bool RemoveSceneNode(IObjectSceneNode* pSceneNode);

			//! 移除指定类型的SceneNode
			void RemoveSceneNodeFromType(ESCENE_NODE_TYPE type);

			// 若三维视图中不存在点云，则zoom to hdi范围，若存在点云则不做处理
			void ZoomToHdiIfNeed(bool bVertical = true);
	
            // 根据domID（路径名）获取已添加的sn，不存在则为空
            IObjectSceneNode* GetDomSceneNode(const char* strDomPath);

            // 添加dom数据，以金字塔纹理贴图方式显示在视图中
            IObjectSceneNode* AddDomData(const char* strDomPath);

            //// 添加点云sn
            //IObjectSceneNode* AddPcdData(const char* strPcdPath);

            // 刷新前调用显示金字塔
            bool RefreshOverView(bool isAddDom);

            // 根据dom范围缩放至全部查看
            void ZoomToFull();

            // 测试代码，提供截图
			void screenShot();

			// zoomtofull for 3ds
			void ZoomToFullFor3ds();

			// 添加 DOM 场景节点（朱立雄 2016-10-24）
			IObjectSceneNode* AddDomSceneNode(const char* pcFileName);

			// 添加 DEM 场景节点（朱立雄 2016-11-2）
			IObjectSceneNode* AddDemSceneNode(const char* pcFileName);

		};
	}
}

//#endif	
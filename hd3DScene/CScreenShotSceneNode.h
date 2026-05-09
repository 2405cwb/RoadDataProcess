/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : CScreenShotSceneNode.h
相关文件     : CScreenShotSceneNode.cpp
文件实现功能 : 截屏场景
			   只在当前视图的状态为选择状态时，进行渲染
			   提高刷新效率
作者         : 软件部，危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
04/26		1.0        危迟					创建
</PRE>
*******************************************************************************/
#pragma once
#include "IObjectSceneNode.h"
#include "ITexture.h"
using namespace irr;
using namespace irr::video;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CScreenShotSceneNode : public IObjectSceneNode
		{
		public:
			CScreenShotSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id, video::ITexture* text);
			~CScreenShotSceneNode(void);
			//*************************ISceneNode 基类接口*******************************//
		public:

			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual const core::aabbox3d<f32>& getBoundingBox()const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual u32 getMaterialCount() const;

			virtual ESCENE_NODE_TYPE getType() const { return ESNT_HD_SCREENSHOT; }

			//*************************自身对象接口*******************************//
			void UpdateTexture(ITexture* text);

			ITexture* GetTexture();

			// 获取截屏对应的视口
			SViewFrustum GetViewFrustum (){return m_curViewFrustum;}
						
		private:
			video::SMaterial m_material;
			core::aabbox3d<f32> m_box;
			SViewFrustum m_curViewFrustum;
		};
	}
}


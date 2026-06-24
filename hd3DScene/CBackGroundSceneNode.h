/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : CBackGroundSceneNode.h
相关文件     : CBackGroundSceneNode.cpp
文件实现功能 : 设置背景
作者         : 软件部，危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
04/12		1.0        危迟					创建
</PRE>
*******************************************************************************/
#pragma once
#include "IObjectSceneNode.h"

using namespace irr;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CBackGroundSceneNode :
			public IObjectSceneNode
		{
		public:
			CBackGroundSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id, video::ITexture* text);
			~CBackGroundSceneNode(void);

			//*************************ISceneNode 基类接口*******************************//
		public:

			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual const core::aabbox3d<f32>& getBoundingBox()const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual u32 getMaterialCount() const;

			virtual ESCENE_NODE_TYPE getType() const { return ESNT_HD_BACKGROUND; }

		private:
			video::SMaterial Material;
			core::aabbox3d<f32> Box;
			
		};
	}
}



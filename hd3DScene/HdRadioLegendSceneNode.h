/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : HdRadioLegendSceneNode.h
相关文件     : HdRadioLegendSceneNode.cpp
文件实现功能 : 比例尺图例
作者         : 软件部，朱旭波
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
10/31		1.0       朱旭波				创建
</PRE>
*******************************************************************************/

#pragma once
#include "IObjectSceneNode.h"

using namespace irr;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CHdRadioLegendSceneNode :
			public IObjectSceneNode
		{
		public:
			CHdRadioLegendSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id);
			virtual ~CHdRadioLegendSceneNode(void);

			//*************************ISceneNode 基类接口*******************************//
		public:

			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual const core::aabbox3d<f32>& getBoundingBox()const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual u32 getMaterialCount() const;

			virtual ESCENE_NODE_TYPE getType() const { return ESNT_RADIO_LEGEND; }

		private:
			// 材质
			video::SMaterial Material;

			// 包围盒
			core::aabbox3d<f32> Box;
		};
	}
}




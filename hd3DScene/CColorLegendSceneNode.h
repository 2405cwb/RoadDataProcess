/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : CColorLegendSceneNode.h
相关文件     : CColorLegendSceneNode.cpp
文件实现功能 : 色彩图例
作者         : 软件部，危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
06/27		1.0        危迟					创建
09/27       1.1        朱旭波               显示图例范围及外部控制显示等
</PRE>
*******************************************************************************/
#pragma once
#include "IObjectSceneNode.h"

using namespace irr;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CColorLegendSceneNode :
			public IObjectSceneNode
		{
		public:
			CColorLegendSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id);
			virtual ~CColorLegendSceneNode(void);

			//*************************ISceneNode 基类接口*******************************//
		public:

			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual const core::aabbox3d<f32>& getBoundingBox()const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual u32 getMaterialCount() const;

			virtual ESCENE_NODE_TYPE getType() const { return ESNT_COLOR_LEGEND; }

			//************************* 自身接口*******************************//
			const CHdColorRamp& GetColorRamp() { return m_colorRamp; }

			void SetColorRamp(CHdColorRamp ramp) { m_colorRamp = ramp; }

			void SetMaxMinValue(float maxValue,float minValue);

		private:
			video::SMaterial Material;
			core::aabbox3d<f32> Box;
			// 渲染颜色带
			CHdColorRamp   m_colorRamp;
			// 显示的色带高程值
			float m_fMinValue;
			float m_fMaxValue;


		};
	}
}
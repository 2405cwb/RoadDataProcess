/*! @file
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : ComPass.h
相关文件     : ComPass.cpp
文件实现功能 : 在视图上显示指北针
作者         : 蔡红云 
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/11/15   1.0      蔡红云              创建

</PRE>
*******************************************************************************/
#pragma once

#pragma once
#include <irrlicht.h>
#include "driverChoice.h"
#include "IObjectSceneNode.h"
using namespace irr;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CComPassSceneNode : public IObjectSceneNode
		{
		private:
			//! 材质,每个SceneNode必须包含此对象
			video::SMaterial	m_material;
			//! 外包范围
			core::aabbox3d<f32> m_box;
			//! X轴颜色
			video::SColor m_Xcolor;
			//! Y轴颜色
			video::SColor m_Ycolor;
			//! Z轴颜色
			video::SColor m_Zcolor;


		public:
			CComPassSceneNode(ISceneNode* parent,ISceneManager* mgr,s32 id);
			~CComPassSceneNode(void);

			/************************************************************************/
			/*                   ISceneNode接口实现                                 */
			/************************************************************************/
		public:
			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual const core::aabbox3d<f32>& getBoundingBox() const;

			virtual u32 getMaterialCount() const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual ESCENE_NODE_TYPE getType() const { return ESNT_HD_CPSS; }
			// axisNo=0,1,2;0 表示设置X轴颜色，1 表示设置Y轴颜色， 2 表示设置Z轴颜色
			virtual void SetAxisColor(int axisNo,video::SColor color)
			{
				if (0 == axisNo)	m_Xcolor = color;
				if (1 == axisNo)	m_Ycolor = color;
				if (2 == axisNo)	m_Zcolor = color;
			}
		};

	}// end of namespace scene
} // end of namespace scene

//#endif
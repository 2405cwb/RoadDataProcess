#pragma once
//#ifndef _C_AXIS_SCENENODE_H_
//#define _C_AXIS_SCENENODE_H_

#include <irrlicht.h>
#include "driverChoice.h"
#include "IObjectSceneNode.h"
using namespace irr;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CAxisSceneNode : public IObjectSceneNode
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
			CAxisSceneNode(ISceneNode* parent,ISceneManager* mgr,s32 id);
			~CAxisSceneNode(void);

			/************************************************************************/
			/*                   ISceneNode接口实现                                 */
			/************************************************************************/
		public:
			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual const core::aabbox3d<f32>& getBoundingBox() const;

			virtual u32 getMaterialCount() const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual ESCENE_NODE_TYPE getType() const { return ESNT_HD_AXIS; }
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
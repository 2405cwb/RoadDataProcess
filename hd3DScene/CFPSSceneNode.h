/*! @file
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : CFPSSceneNode.h
相关文件     : CFPSSceneNode.cpp
文件实现功能 : 在视图上显示FPS（每秒帧数）
作者         : 软件部，危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
</PRE>
*******************************************************************************/
#pragma once
//#ifndef _C_FPS_SCENENODE_H_
//#define _C_FPS_SCENENODE_H_

#include <irrlicht.h>
#include "driverChoice.h"
#include "IObjectSceneNode.h"

using namespace irr;
namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CFPSSceneNode : public IObjectSceneNode
		{
		private:
			//! 材质,每个SceneNode必须包含此对象
			video::SMaterial	m_material;

			core::aabbox3d<f32> m_box;

			// 三角形数量 fengjing
			u32 m_triangleCount;

			// 点数 fengjing
			u32 m_ptsSize;

			// 模型视图显示三角形数与点数
			bool m_IsModelview;

			// 是否显示LOD
			bool m_bShowLod;

			// 是否显示FPS
			bool m_bShowFps;
		public:
			int lastFPS;
		public:
			CFPSSceneNode(ISceneNode* parent,ISceneManager* mgr,s32 id);
			~CFPSSceneNode(void);

			/************************************************************************/
			/*                   ISceneNode接口实现                                 */
			/************************************************************************/
		public:
			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual u32 getMaterialCount() const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual const core::aabbox3d<f32>& getBoundingBox() const{return m_box;}

			//! Returns type of the scene node
			virtual ESCENE_NODE_TYPE getType() const { return ESNT_HD_FPS; }

			// 设置三角形数与点数
			void SetTrianCounts(u32 triangleCount, /*u32 ptsSize,*/ bool IsModelview) { 
				m_triangleCount = triangleCount;
			   /* m_ptsSize = ptsSize;*/
			    m_IsModelview = IsModelview;}
			// 设置是否显示FPS
			void SetFpsVisable(bool bShow){m_bShowFps = bShow;}

			// 设置是否显示LOD
			void SetLodVisable(bool bShow){m_bShowLod = bShow;}
		};

	}
}

//#endif
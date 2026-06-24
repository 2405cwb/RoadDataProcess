/*! @file
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : HdPolySelectIn3DSceneNode.h
相关文件     : HdPolySelectIn3DSceneNode.cpp
文件实现功能 : 实现多边形的显示(公共模块使用)
作者         : 软件部，朱旭波
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/10/18   1.00     朱旭波              
</PRE>
*******************************************************************************/
#pragma once

#include "..\..\hd3DEngine\include\irrlicht.h"
#include "..\..\hdCommon\sceneData\HdSxPolyline2D.h"
#include "..\..\hd3DScene\iobjectscenenode.h"

using namespace irr;
using namespace hd::scene;

namespace hd
{
	namespace fm
	{
		class HD3DSCENE_API CHdPolySelectIn3DSceneNode : public IObjectSceneNode
		{
		public:
			//! constructor
			CHdPolySelectIn3DSceneNode(const CHdSxPolyline2D* polyline2D, ISceneNode* parent, ISceneManager* mgr, s32 id);
			~CHdPolySelectIn3DSceneNode(void);

			//! pre render event
			virtual void OnRegisterSceneNode();

			//! render
			virtual void render();

			//! returns the axis aligned bounding box of this node
			virtual const core::aabbox3d<f32>& getBoundingBox() const;

			//! 由于CPolyline对象会在外部被更新，所以需要有接口去更新SceneNode的范围
			void UpdateBoundingBox();

			virtual video::SMaterial& getMaterial(u32 i);

			//! returns amount of materials used by this scene node.
			virtual u32 getMaterialCount() const;			

			//! Writes attributes of the scene node.
			virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const;

			//! Reads attributes of the scene node.
			virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0);

			//! Returns type of the scene node
			virtual ESCENE_NODE_TYPE getType() const { return ESNT_HD_INCOMMON_POLYSELECT; }

			//! Creates a clone of this scene node and its children.
			virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);

		private:			
			const CHdSxPolyline2D*	m_pPolyline2D;
			video::SMaterial		m_Material;
			core::aabbox3d<f32>		m_BBox;
			video::SColor           m_polySelColor;
		public:
			bool m_bPolySuccess;//标记多边形是否绘制成功
		};

	} // end namespace ddd
} // end namespace hd



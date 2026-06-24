/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : CCollisionRaySceneNode.h
相关文件     : CCollisionRaySceneNode.cpp, 
文件实现功能 : 实现碰撞检测射线绘制
作者         : 危迟
版本         : 软件部，危迟
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/06/20	 1.0	  危迟					创建
</PRE>
*******************************************************************************/
#pragma once

#include "..\hd3DEngine\include\irrlicht.h"
#include "..\hdCommon\sceneData\HdSxPolyline3D.h"
#include "IObjectSceneNode.h"

using namespace irr;
using namespace hd::scene;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CCollisionRaySceneNode : public IObjectSceneNode
		{
		public:
			CCollisionRaySceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id);
			virtual ~CCollisionRaySceneNode();
			//*************************ISceneNode 基类接口*******************************//

			virtual void OnRegisterSceneNode();

			virtual void render();
		
			virtual const core::aabbox3d<f32>& getBoundingBox() const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual u32 getMaterialCount() const;

			virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const;

			virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0);

			virtual ESCENE_NODE_TYPE getType() const { return ESNT_HD_COLLISIONRAY; }

			virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);

			//*****************************自身对象接口***************************************//
			void UpdateBoundingBox();

			// 设置直线
			void setPolyline(core::vector3df StartPt,core::vector3df EndPt)
			{
				m_pRay.start = StartPt;
				m_pRay.end = EndPt;

				UpdateBoundingBox();
			}

		private:
			core::line3df		m_pRay;
			video::SMaterial		m_Material;
			core::aabbox3d<f32>		m_BBox;
		};
	}
}



#pragma once
#include "IObjectSceneNode.h"

#include "ESceneNodeTypes.h"

using namespace irr;
using namespace irr::scene;
using namespace hd;
using namespace hd::scene;
namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CBoxSceneNode:public IObjectSceneNode
		{


		public:
			CBoxSceneNode(core::aabbox3df box,ISceneNode* parent, ISceneManager* mgr, s32 id);
			virtual ~CBoxSceneNode(void);

		public:
			//*************************ISceneNode 基类接口*******************************//
			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual const core::aabbox3d<f32>& getBoundingBox()const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual u32 getMaterialCount() const;

			virtual ESCENE_NODE_TYPE getType() const { return ESNT_SELECT_BOX; }
			

			//*************************CBoxSceneNode 基类接口*******************************//
			// 更新范围
			void UpdateBoundingBox(core::aabbox3d<f32> box);
		
			// 重新生成mesh
			void GenerateMesh();
			
			void getBoxSize(core::vector3df &size){}

		public:
		
			// mesh对象
			SMeshBuffer*			     m_Buffer;
		
			// box
			core::aabbox3df                m_Box;
			
			// 是否需要进行坐标转换
			bool m_bTrans;
		
		};
	}
}



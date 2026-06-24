/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : CIMeshSceneNode.h
相关文件     : CIMeshSceneNode.cpp
文件实现功能 : 网格场景结点，渲染模型
作者         : 软件部，危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
08/05		 1.0      危迟				    创建
2013/9/29    1.1      蔡红云                增加点云文件路径用来关联对应点云
</PRE>
*******************************************************************************/
#pragma once
#include "IObjectSceneNode.h"
#include "IMesh.h"

using namespace irr;
using namespace irr::scene;

namespace hd
{	
	namespace scene
	{
		class HD3DSCENE_API CIMeshSceneNode :
			public IObjectSceneNode
		{
		public:
			CIMeshSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,IMesh* mesh);
			virtual ~CIMeshSceneNode(void);

			//*************************ISceneNode 基类接口*******************************//
		public:

			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual const core::aabbox3d<f32>& getBoundingBox()const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual u32 getMaterialCount() const;

			virtual ESCENE_NODE_TYPE getType() const { return ESNT_MESH_MODEL; }

			//************************* 自身接口*******************************//
		public:
			// 获取网格
			IMesh* GetMesh() { return m_mesh;}

			// 设置网格
			virtual void SetMesh(IMesh* mesh);

			// 获取点云文件路径
			string getPcdFilePath() { return m_strpcdFileName;}

			// 设置点云文件路径
			void setPcdFilePath(string strpcdFileName)
			{
				m_strpcdFileName = strpcdFileName;
			}

		private:
			void CopyMaterials();
			core::array<video::SMaterial> m_Materials;
			video::SMaterial m_ReadOnlyMaterial;
			core::aabbox3d<f32> m_Box;
			IMesh* m_mesh;
			s32 m_nPassCount;
			bool m_bReadOnlyMaterials;
			// 点云路径
			string m_strpcdFileName;
		};
	}
}
#include "StdAfx.h"
#include "CIMeshSceneNode.h"
#include "CScanSceneNode.h"
namespace hd
{	
	namespace scene
	{
		CIMeshSceneNode::CIMeshSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id ,IMesh* mesh)
			:IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id),m_bReadOnlyMaterials(false)
		{
			m_mesh = NULL;
			SetMesh(mesh);
			m_strpcdFileName = "";
		}


		CIMeshSceneNode::~CIMeshSceneNode(void)
		{
			if (m_mesh)
				m_mesh->drop();
		}

		void CIMeshSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
			{
				video::IVideoDriver* driver = SceneManager->getVideoDriver();

				m_nPassCount = 0;
				int transparentCount = 0;
				int solidCount = 0;

				if (m_bReadOnlyMaterials && m_mesh)
				{

					for (u32 i=0; i<m_mesh->getMeshBufferCount(); ++i)
					{
						scene::IMeshBuffer* mb = m_mesh->getMeshBuffer(i);
						video::IMaterialRenderer* rnd = mb ? driver->getMaterialRenderer(mb->getMaterial().MaterialType) : 0;

						if (rnd && rnd->isTransparent())
							++transparentCount;
						else
							++solidCount;

						if (solidCount && transparentCount)
							break;
					}
				}
				else
				{
					for (u32 i=0; i<m_Materials.size(); ++i)
					{
						video::IMaterialRenderer* rnd =
							driver->getMaterialRenderer(m_Materials[i].MaterialType);

						if (rnd && rnd->isTransparent())
							++transparentCount;
						else
							++solidCount;

						if (solidCount && transparentCount)
							break;
					}
				}

				if (solidCount)
					SceneManager->registerNodeForRendering(this, scene::ESNRP_SOLID);

				if (transparentCount)
					SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);

				ISceneNode::OnRegisterSceneNode();
			}
		}

		void CIMeshSceneNode::render()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();

			if (!m_mesh || !driver)
				return;

			bool isTransparentPass =
				SceneManager->getSceneNodeRenderPass() == scene::ESNRP_TRANSPARENT;

			++m_nPassCount;

			driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
			m_Box = m_mesh->getBoundingBox();
			//// 蔡红云 2013/10/30 
			//// 解决多测站视图点云卸载后、仍然在多测站有扫描仪模型的问题
			//core::array<ISceneNode*> arraySn;
			//SceneManager->getSceneNodesFromType(ESNT_SCAN_POINT,arraySn);
			//// 如果场景中没有点云，扫描仪模型不进行渲染、并且设置为隐藏
			//if (arraySn.size()<=0)
			//{
			//	IsVisible = false;
			//	return;
			//}
			//PointCloud *ptCloud = NULL;
			//bool flag = false;
			//CScanSceneNode* pScanSnShow;// 视图中存在的和扫描仪模型保持关联的点云
			//for (unsigned int i = 0;i < arraySn.size();i++)
			//{
			//	CScanSceneNode* pScanSn = dynamic_cast<CScanSceneNode*>(arraySn[i]);
			//	if (pScanSn)
			//	{
			//		ptCloud = pScanSn->GetPointCloud();
			//		string filepath = ptCloud->GetPointCloudPath();
			//		// 如果点云路径和扫描仪模型关联点云路径相同
			//		// 对flag进行标记
			//		if (m_strpcdFileName == filepath)
			//		{
			//			flag = true;
			//			pScanSnShow = pScanSn;
			//		}

			//	}
			//}
			//// 如果没有关联的点云，扫描仪模型不进行渲染、并且设置为隐藏
			//if (!flag)
			//{
			//	IsVisible = false;
			//	return;
			//}
			//// 如果有关联的点云，但是点云为隐藏
			//// 那么扫描仪模型不进行渲染、并且设置为隐藏
			//if (pScanSnShow->isVisible() == false)
			//{
			//	IsVisible = false;
		 //   	return;
			//}

			for (u32 i=0; i<m_mesh->getMeshBufferCount(); ++i)
			{
				scene::IMeshBuffer* mb = m_mesh->getMeshBuffer(i);
				if (mb)
				{
					const video::SMaterial& material = m_bReadOnlyMaterials ? mb->getMaterial() : m_Materials[i];

					video::IMaterialRenderer* rnd = driver->getMaterialRenderer(material.MaterialType);
					bool transparent = (rnd && rnd->isTransparent());

					if (transparent == isTransparentPass)
					{
						driver->setMaterial(material);
						driver->drawMeshBuffer(mb);
					}
				}
			}
		}

		const core::aabbox3d<f32>& CIMeshSceneNode::getBoundingBox()const
		{
			return m_mesh ? m_mesh->getBoundingBox() : m_Box;
		}

		video::SMaterial& CIMeshSceneNode::getMaterial(u32 i)
		{
			if (m_mesh && m_bReadOnlyMaterials && i<m_mesh->getMeshBufferCount())
			{
				m_ReadOnlyMaterial = m_mesh->getMeshBuffer(i)->getMaterial();
				return m_ReadOnlyMaterial;
			}

			if (i >= m_Materials.size())
				return ISceneNode::getMaterial(i);

			return m_Materials[i];
		}

		u32 CIMeshSceneNode::getMaterialCount() const
		{
			return m_Materials.size();
		}

		void CIMeshSceneNode::SetMesh(IMesh* mesh)
		{
			if (mesh)
			{
				mesh->grab();
				if (m_mesh)
					m_mesh->drop();

				m_mesh = mesh;
				CopyMaterials();
			}
		}

		void CIMeshSceneNode::CopyMaterials()
		{
			m_Materials.clear();

			if (m_mesh)
			{
				video::SMaterial mat;

				for (u32 i=0; i<m_mesh->getMeshBufferCount(); ++i)
				{
					IMeshBuffer* mb = m_mesh->getMeshBuffer(i);
					if (mb)
						mat = mb->getMaterial();

					m_Materials.push_back(mat);
				}

				m_Box = m_mesh->getBoundingBox();
			}
		}
	}
}
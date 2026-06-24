#include "StdAfx.h"
#include "CBoxSceneNode.h"
#include "CScanSceneNode.h"
#include "..\hd3DEngine\COpenGLExtensionHandler.h"
namespace hd
{
	namespace scene
	{

		CBoxSceneNode::CBoxSceneNode(core::aabbox3df box,ISceneNode* parent, ISceneManager* mgr, s32 id)
			: IObjectSceneNode(NULL,video:: SColor(100, 100, 180, 180),g_selColor,parent, mgr, id)
		{
			IVideoDriver* driver = SceneManager->getVideoDriver();
			m_Box = box;
			
			if (driver)
			{
				m_Buffer = new SMeshBuffer();
				m_Buffer->Material.Lighting = false;
				m_Buffer->Material.ZBuffer = false;
				m_Buffer->Material.ZWriteEnable = false;
				m_Buffer->Material.AntiAliasing = video::EAAM_OFF;
				m_Buffer->Material.Wireframe = false;
				m_Buffer->Material.BackfaceCulling = false;
				m_Buffer->Material.FrontfaceCulling = true;
				//ITexture* text = driver->addTexture()
				m_Buffer->BoundingBox = box;
	
				GenerateMesh();
			}
		}


		CBoxSceneNode::~CBoxSceneNode(void)
		{
			if (m_Buffer)
			{
				getSceneManager()->getVideoDriver()->removeHardwareBuffer(m_Buffer);
				m_Buffer->drop();
				m_Buffer = NULL;
			}
		}

		void CBoxSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
				SceneManager->registerNodeForRendering(this);

			ISceneNode::OnRegisterSceneNode();
		}

	

		// 渲染
		void CBoxSceneNode::render()
		{

			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

			if (!camera || !driver || !m_pView)
				return;

			if (!m_pView->IsViewRenderAllNode())
			{
				return;
			}

			driver->setMaterial(m_Buffer->Material);
			driver->setRenderStates3DMode();
			ENUM_HD_VIEW_TYPE type = m_pView->GetViewType();
			
			// 只在3D视图下，进行绘制3DBOX
			if (type == E_HVT_3D)
			{
				
    			core::matrix4 mat(AbsoluteTransformation);
	
				// 设置变换矩阵
				mat.setTranslation(m_Box.getCenter() +  getPosition() );

				// 进行变换
				driver->setTransform(video::ETS_WORLD, mat);
								
				driver->setMaterial(m_Buffer->Material);
				
				// 开启混合
				glEnable(GL_BLEND);
			
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 
				
				driver->drawMeshBuffer(m_Buffer);
	
				video::SColor lineColor(200, 100, 180, 180);

				// 画12条边
				driver->draw3DLine(m_Buffer->Vertices[0].Pos ,m_Buffer->Vertices[1].Pos ,lineColor);
				driver->draw3DLine(m_Buffer->Vertices[0].Pos ,m_Buffer->Vertices[3].Pos ,lineColor);
				driver->draw3DLine(m_Buffer->Vertices[0].Pos ,m_Buffer->Vertices[7].Pos ,lineColor);
				driver->draw3DLine(m_Buffer->Vertices[1].Pos ,m_Buffer->Vertices[4].Pos ,lineColor);
				driver->draw3DLine(m_Buffer->Vertices[1].Pos ,m_Buffer->Vertices[2].Pos ,lineColor);
				driver->draw3DLine(m_Buffer->Vertices[4].Pos ,m_Buffer->Vertices[5].Pos ,lineColor);
				driver->draw3DLine(m_Buffer->Vertices[2].Pos ,m_Buffer->Vertices[3].Pos ,lineColor);
				driver->draw3DLine(m_Buffer->Vertices[2].Pos ,m_Buffer->Vertices[5].Pos ,lineColor);
				driver->draw3DLine(m_Buffer->Vertices[4].Pos ,m_Buffer->Vertices[7].Pos ,lineColor);
				driver->draw3DLine(m_Buffer->Vertices[5].Pos ,m_Buffer->Vertices[6].Pos ,lineColor);
				driver->draw3DLine(m_Buffer->Vertices[6].Pos ,m_Buffer->Vertices[7].Pos ,lineColor);
				driver->draw3DLine(m_Buffer->Vertices[3].Pos ,m_Buffer->Vertices[6].Pos ,lineColor);
				driver->draw3DLine(m_Buffer->Vertices[6].Pos ,m_Buffer->Vertices[7].Pos ,lineColor);

				glDisable(GL_BLEND);
			}

		}

		const core::aabbox3d<f32>& CBoxSceneNode::getBoundingBox() const
		{
			return m_Box;
		}

		video::SMaterial& CBoxSceneNode::getMaterial(u32 i)
		{
			return m_Buffer->Material;
		}

		u32 CBoxSceneNode::getMaterialCount() const
		{
			return 1;
		}

		// 更新3DBOX，用于第一次盒子的初始化
		void CBoxSceneNode::UpdateBoundingBox(core::aabbox3d<f32> box)
		{

			m_Box = box;
			// 生成mesh
			GenerateMesh();
		}

	
		// 生成mesh
		void CBoxSceneNode::GenerateMesh()
		{
			// 清空数据
			m_Buffer->Vertices.clear();
			m_Buffer->Indices.clear();

			// 构建三角形，共12个顶点，每3个顶点构成一个三角形
			const u16 u[36] = {   0,2,1,   0,3,2,   1,5,4,   1,2,5,   4,6,7,   4,5,6, 
				7,3,0,   7,6,3,   9,5,2,   9,8,5,   0,11,10,   0,10,7};
			m_Buffer->Indices.set_used(36);
			for (u32 i=0; i<36; ++i)
				m_Buffer->Indices[i] = u[i];
			// Create vertices
			// 默认绘制浅绿色透明包围盒
			/*	video::SColor clr(150, 100, 101, 140);*/
			//video::SColor clr(100, 100, 180, 180);
			m_Buffer->Vertices.reallocate(12);
			// 后面的0，1，2... 是顶点序号
			m_Buffer->Vertices.push_back(video::S3DVertex(0,0,0, -1,-1,-1, m_Color, 0, 1));// 0
			m_Buffer->Vertices.push_back(video::S3DVertex(1,0,0,  1,-1,-1, m_Color, 1, 1));// 1
			m_Buffer->Vertices.push_back(video::S3DVertex(1,1,0,  1, 1,-1, m_Color, 1, 0));// 2
			m_Buffer->Vertices.push_back(video::S3DVertex(0,1,0, -1, 1,-1, m_Color, 0, 0));// 3
			m_Buffer->Vertices.push_back(video::S3DVertex(1,0,1,  1,-1, 1, m_Color, 0, 1));// 4
			m_Buffer->Vertices.push_back(video::S3DVertex(1,1,1,  1, 1, 1, m_Color, 0, 0));// 5
			m_Buffer->Vertices.push_back(video::S3DVertex(0,1,1, -1, 1, 1, m_Color, 1, 0));// 6
			m_Buffer->Vertices.push_back(video::S3DVertex(0,0,1, -1,-1, 1, m_Color, 1, 1));// 7
			m_Buffer->Vertices.push_back(video::S3DVertex(0,1,1, -1, 1, 1, m_Color, 0, 1));// 8 (6)
			m_Buffer->Vertices.push_back(video::S3DVertex(0,1,0, -1, 1,-1, m_Color, 1, 1)); // 9（3）
			m_Buffer->Vertices.push_back(video::S3DVertex(1,0,1,  1,-1, 1, m_Color, 1, 0)); //10(4)
			m_Buffer->Vertices.push_back(video::S3DVertex(1,0,0,  1,-1,-1, m_Color, 0, 0)); // 11(1)

			// Recalculate bounding box
			m_Buffer->BoundingBox.reset(0,0,0);

			core::vector3df size = m_Box.getExtent();
			for (u32 i=0; i<12; ++i)
			{
				m_Buffer->Vertices[i].Pos -= core::vector3df(0.5f, 0.5f, 0.5f);// 默认盒子的中心
				m_Buffer->Vertices[i].Pos *= size;  // 根据 m_Box 的范围扩展盒子
				core::vector3df pos = m_Buffer->Vertices[i].Pos;
				m_Buffer->BoundingBox.addInternalPoint(m_Buffer->Vertices[i].Pos);
			}
			// 设置硬件映射模式为dynamic 
			m_Buffer->setHardwareMappingHint(irr::scene::EHM_DYNAMIC);
			// 标记buffer的顶点及顶点索引已经改变，更新硬件中的buffer 
			m_Buffer->setDirty(EBT_VERTEX_AND_INDEX);
			// 重新计算m_Buffer的外包围盒
			m_Buffer->recalculateBoundingBox();

     	}

	}
}


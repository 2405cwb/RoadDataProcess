#include "StdAfx.h"
#include "CBackGroundSceneNode.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	namespace scene
	{

		CBackGroundSceneNode::CBackGroundSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
			 video::ITexture* text)
			: IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id)
		{
#ifdef _DEBUG
			setDebugName("CBackGroundSceneNode");
#endif
			setAutomaticCulling(EAC_OFF);

			Material.Lighting = false;
			Material.ZBuffer = video::ECFN_NEVER;
			Material.ZWriteEnable = false;
			Material.AntiAliasing = video::EAAM_OFF;
			Material.setTexture(0, text);
			Box.MaxEdge.set(0,0,0);
			Box.MinEdge.set(0,0,0);

			/*Buffer = new SMeshBuffer();
			Buffer->Material.Lighting = false;*/
			//// 设定Zbuffer为关闭 保证在任何时候均显示
			//Buffer->Material.ZBuffer = video::ECFN_NEVER;
			//Buffer->Material.ZWriteEnable = false;
			//Buffer->Material.AntiAliasing = video::EAAM_OFF;
			//Buffer->Material.setTexture(0, text);
			//Buffer->BoundingBox.MaxEdge.set(0,0,0);
			//Buffer->BoundingBox.MinEdge.set(0,0,0);

			//ICameraSceneNode* camera = SceneManager->getActiveCamera();
			//core::vector3df LU = camera->getViewFrustum()->getFarLeftUp();
			//core::vector3df RU = camera->getViewFrustum()->getFarRightUp();
			//core::vector3df LD = camera->getViewFrustum()->getFarLeftDown();
			//core::vector3df RD = camera->getViewFrustum()->getFarRightDown();

			//video::S3DVertex vtx;
			//vtx.Color.set(255,255,255,255);
			//vtx.Normal.set(0.0f,0.0f,1.0f);

			//vtx.Pos = LD;
			//vtx.TCoords.set(0.0f, 1.0f);
			//Buffer->Vertices.push_back(vtx);

			//vtx.Pos = RD;
			//vtx.TCoords.set(0.0f, 0.0f);
			//Buffer->Vertices.push_back(vtx);

			//vtx.Pos = RU;
			//vtx.TCoords.set(1.0f, 0.0f);
			//Buffer->Vertices.push_back(vtx);

			//vtx.Pos = LU;
			//vtx.TCoords.set(1.0f, 1.0f);
			//Buffer->Vertices.push_back(vtx);

			//Buffer->Indices.push_back(0);
			//Buffer->Indices.push_back(1);
			//Buffer->Indices.push_back(2);

			//Buffer->Indices.push_back(0);
			//Buffer->Indices.push_back(2);
			//Buffer->Indices.push_back(3);
			//
		}

		CBackGroundSceneNode::~CBackGroundSceneNode(void)
		{
			//if (Buffer)
			//{
			//	getSceneManager()->getVideoDriver()->removeTexture(Buffer->Material.getTexture(0));
			//	getSceneManager()->getVideoDriver()->removeHardwareBuffer(Buffer);
			//	Buffer->drop();
			//	Buffer = NULL;
			//}
			if (Material.getTexture(0))
			{
				getSceneManager()->getVideoDriver()->removeTexture(Material.getTexture(0));
			}
			
		}

		void CBackGroundSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
				SceneManager->registerNodeForRendering(this, ESNRP_SKY_BOX);

			ISceneNode::OnRegisterSceneNode();
		}

		void CBackGroundSceneNode::render()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

			if (!camera || !driver || !m_pView)
			{		
				return;
			}
			if (!m_pView->IsViewRenderAllNode())
			{
				return;
			}
			{
				video::ITexture* tex = Material.getTexture(0);
				core::rect<s32> rctDest(core::position2d<s32>(0,0),
					core::dimension2di(driver->getCurrentRenderTargetSize()));
				core::rect<s32> rctSrc(core::position2d<s32>(0,0),
					core::dimension2di(tex->getSize()));

				driver->draw2DImage(tex, rctDest, rctSrc);

			}
		}

		const core::aabbox3d<f32>& CBackGroundSceneNode::getBoundingBox() const
		{
			return Box;
		}

		video::SMaterial& CBackGroundSceneNode::getMaterial(u32 i)
		{
			return Material;
		}

		u32 CBackGroundSceneNode::getMaterialCount() const
		{
			return 1;
		}
	}
}


#include "StdAfx.h"
#include "CScreenShotSceneNode.h"
#include "ISceneView.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	namespace scene
	{
		CScreenShotSceneNode::CScreenShotSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
			video::ITexture* text)
			: IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id)
		{
#ifdef _DEBUG
			setDebugName("CScreenShotSceneNode");
#endif
			setAutomaticCulling(EAC_OFF);

			m_curViewFrustum = *(mgr->getActiveCamera()->getViewFrustum());
			m_material.Lighting = false;
			m_material.ZBuffer = video::ECFN_NEVER;
			m_material.ZWriteEnable = false;
			m_material.AntiAliasing = video::EAAM_OFF;
			m_material.setTexture(0, text);
			m_box.MaxEdge.set(0,0,0);
			m_box.MinEdge.set(0,0,0);
		}


		CScreenShotSceneNode::~CScreenShotSceneNode(void)
		{
		}

		void CScreenShotSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
				SceneManager->registerNodeForRendering(this, ESNRP_SKY_BOX);

			ISceneNode::OnRegisterSceneNode();
		}

		ITexture* CScreenShotSceneNode::GetTexture()
		{
			return m_material.getTexture(0);
		}
		void CScreenShotSceneNode::render()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();
			
			video::ITexture* tex = m_material.getTexture(0);
			if (!camera || !driver || !m_pView || tex == NULL)
			{		
				return;
			}
			// 渲染所有场景结点时，不渲染截屏结点
			// 渲染截屏结点时，不渲染所有结点
			if (m_pView->IsViewRenderAllNode())
			//if(SceneManager->GetRefreshType() == ESCENE_REFRESH_TYPE::HDVIEW_ALL ||
			//	SceneManager->GetRefreshType() < 0)
			{
				return;
			}
		
			core::rect<s32> rctDest(core::position2d<s32>(0,0),
				core::dimension2di(driver->getCurrentRenderTargetSize()));
			core::rect<s32> rctSrc(core::position2d<s32>(0,0),
				core::dimension2di(tex->getSize()));

			driver->draw2DImage(tex, rctDest, rctSrc);
		}

		const core::aabbox3d<f32>& CScreenShotSceneNode::getBoundingBox() const
		{
			return m_box;
		}

		video::SMaterial& CScreenShotSceneNode::getMaterial(u32 i)
		{
			return m_material;
		}

		u32 CScreenShotSceneNode::getMaterialCount() const
		{
			return 1;
		}

		void CScreenShotSceneNode::UpdateTexture(ITexture* text)
		{
			if (text)
			{
				ITexture* pText0 = m_material.getTexture(0);				
				if(pText0)
				{
					video::IVideoDriver* driver = SceneManager->getVideoDriver();
					driver->removeTexture(pText0);
				}
				m_material.setTexture(0,text);

				m_curViewFrustum = *(SceneManager->getActiveCamera()->getViewFrustum());
			}
		}
	}

}


#include "StdAfx.h"
#include "CCollisionRaySceneNode.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	namespace scene
	{
		CCollisionRaySceneNode::CCollisionRaySceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id)
			:IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id)
		{
			m_Material.Wireframe = false;
			m_Material.Lighting = false;
			setAutomaticCulling(irr::scene::EAC_OFF);
		}


		CCollisionRaySceneNode::~CCollisionRaySceneNode(void)
		{
		}

		void CCollisionRaySceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
				SceneManager->registerNodeForRendering(this);

			ISceneNode::OnRegisterSceneNode();
		}

		void CCollisionRaySceneNode::render()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

			if (!camera || !driver ||! m_pView)
				return;
			if (m_pView->GetViewType() != E_HVT_QUICK &&
				m_pView->GetViewType() != E_HVT_3D)
				return;

			if (m_pRay.getLength() == 0.f)
			{
				return;
			}

			driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
			driver->setMaterial(m_Material);

			//使用蓝线绘制射线
			m_Color.set(0,0,0,255);
			driver->draw3DLine(m_pRay.start,m_pRay.end,m_Color);

		}

		const core::aabbox3d<f32>& CCollisionRaySceneNode::getBoundingBox() const
		{
			return m_BBox;
		}

		void CCollisionRaySceneNode::UpdateBoundingBox()
		{
			if (!m_pView)
			{
				return;
			}
		}
		video::SMaterial& CCollisionRaySceneNode::getMaterial(u32 i)
		{
			return m_Material;
		}

		u32 CCollisionRaySceneNode::getMaterialCount() const
		{
			return 1;
		}

		void CCollisionRaySceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
		{
		}

		void CCollisionRaySceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
		{
		}

		ISceneNode* CCollisionRaySceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
		{
			if (!newParent)
				newParent = Parent;
			if (!newManager)
				newManager = SceneManager;

			CCollisionRaySceneNode* nb = new CCollisionRaySceneNode(newParent, newManager, ID);

			nb->cloneMembers(this, newManager);

			if ( newParent )
				nb->drop();
			return nb;
		}
	}
}


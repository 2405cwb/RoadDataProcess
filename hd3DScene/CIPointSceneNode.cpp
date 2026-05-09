#include "StdAfx.h"
#include "CIPointSceneNode.h"
#include "..\..\hdPointCloud\hdSysSetting.h"

namespace hd
{
	namespace scene
	{
		CIPointSceneNode::CIPointSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id)
			:IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id)
		{
			m_Material.Wireframe = false;
			m_Material.Lighting = false;
			setAutomaticCulling(irr::scene::EAC_OFF);
		}


		CIPointSceneNode::~CIPointSceneNode(void)
		{
		}

		void CIPointSceneNode::render()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

			if (!camera || !driver ||! m_pView)
				return;
			if (m_pView->GetViewType() != E_HVT_3D 
				&& m_pView->GetViewType() != E_HVT_MULTISCAN3D
                && m_pView->GetViewType() != E_HVT_SKETCH_ISCAN3D
				&& m_pView->GetViewType() != E_HVT_ORTHO3D)
				return;

			if (m_vecCoords.size() == 0)
			{
				return;
			}

			driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
			driver->setMaterial(m_Material);

			// 根据系统设置的CAD绘制点颜色进行设置
			COLORREF colorTemp = CHdSysSetting::getSysSetting()->measureSetting.cadPointColor;
			video::SColor curColor = video::SColor(255,GetRValue(colorTemp),GetGValue(colorTemp),GetBValue(colorTemp));

			video::SColor color = m_bSelected?m_SelectedColor:curColor;
			int nCount = GetVertexCount();
			core::position2di screenPos;
			s32 nPtSize = 5;//画点时，点的大小，又像素为单位计算

			for (int i = 0;i < nCount;i++)
			{
				core::vector3df start = m_vecCoords[i];
				screenPos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(start);
				driver->draw2DLine(core::vector2di(screenPos.X - nPtSize, screenPos.Y),
					core::vector2di(screenPos.X + nPtSize, screenPos.Y),color);
				driver->draw2DLine(core::vector2di(screenPos.X, screenPos.Y - nPtSize),
					core::vector2di(screenPos.X, screenPos.Y + nPtSize),color);
			}

		}

		void CIPointSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
				SceneManager->registerNodeForRendering(this);

			ISceneNode::OnRegisterSceneNode();
		}

		const core::aabbox3d<f32>& CIPointSceneNode::getBoundingBox() const
		{
			return m_BBox;
		}

		void CIPointSceneNode::UpdateBoundingBox()
		{
			if (!m_pView)
			{
				return;
			}

			m_BBox.MinEdge.X = F32_MAX;
			m_BBox.MinEdge.Y = F32_MAX;
			m_BBox.MinEdge.Z = F32_MAX;
			m_BBox.MaxEdge.X = F32_MIN;
			m_BBox.MaxEdge.Y = F32_MIN;
			m_BBox.MaxEdge.Z = F32_MIN;

			int count = GetVertexCount();

			for (int i = 0; i< count;i++)
			{
				core::vector3df pt = m_vecCoords[i];

				if (pt.X < m_BBox.MinEdge.X )
				{
					m_BBox.MinEdge.X  = pt.X;
				}
				if (pt.X > m_BBox.MaxEdge.X)
				{
					m_BBox.MaxEdge.X = pt.X;
				}
				if (pt.Y < m_BBox.MinEdge.Y)
				{
					m_BBox.MinEdge.Y = pt.Y;
				}
				if (pt.Y > m_BBox.MaxEdge.Y)
				{
					m_BBox.MaxEdge.Y = pt.Y;
				}
				if (pt.Z < m_BBox.MinEdge.Z)
				{
					m_BBox.MinEdge.Z = pt.Z;
				}
				if (pt.Z > m_BBox.MaxEdge.Z)
				{
					m_BBox.MaxEdge.Z = pt.Z;
				}
			}
		}

		void CIPointSceneNode::AddCoordinate(core::vector3df coord)
		{
			m_vecCoords.push_back(coord);
		}

		bool CIPointSceneNode::GetVertexCoord(int i,core::vector3df& coord)
		{
			if (i < 0 || i >= (int)m_vecCoords.size())
			{
				return false;
			}

			coord = m_vecCoords[i];

			return true;
		}

		void CIPointSceneNode::ClearAllVertex()
		{
			m_vecCoords.clear();
		}

		video::SMaterial& CIPointSceneNode::getMaterial(u32 i)
		{
			return m_Material;
		}

		u32 CIPointSceneNode::getMaterialCount() const
		{
			return 1;
		}

		void CIPointSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
		{
		}

		void CIPointSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
		{
		}

		ISceneNode* CIPointSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
		{
			if (!newParent)
				newParent = Parent;
			if (!newManager)
				newManager = SceneManager;

			CIPointSceneNode* nb = new CIPointSceneNode(newParent, newManager, ID);

			nb->cloneMembers(this, newManager);

			if ( newParent )
				nb->drop();
			return nb;
		}

	}
}


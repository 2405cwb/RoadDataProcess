#include "StdAfx.h"
#include "CIPolylineSceneNode.h"
#include "..\hdPointCloud\hdSysSetting.h"

namespace hd
{
	namespace scene
	{
		CIPolylineSceneNode::CIPolylineSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id)
			: IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id)
		{
			m_bClosure = false;
			m_b3D = true;
			m_bTrans = false;
			m_bSysColor = true;
			m_Material.Wireframe = false;
			m_Material.Lighting = false;
			//m_Material.ZBuffer = false;
			setAutomaticCulling(irr::scene::EAC_OFF);
		}


		CIPolylineSceneNode::~CIPolylineSceneNode(void)
		{
		}

		void CIPolylineSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
				SceneManager->registerNodeForRendering(this);

			ISceneNode::OnRegisterSceneNode();
		}

		void CIPolylineSceneNode::render()
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
			//m_Material.Thickness = 5.f;
			driver->setMaterial(m_Material);

			// 根据系统设置中的选择线颜色进行设置
			if (m_bSysColor)
			{
				COLORREF colorTemp = CHdSysSetting::getSysSetting()->measureSetting.measureLineColor;
				m_Color = video::SColor(255,GetRValue(colorTemp),GetGValue(colorTemp),GetBValue(colorTemp));
			}
			
			int count = GetVertexCount();
			
			// 绘制三维线
			if (m_b3D)
			{
				for (int i = 0; i< count;i++)
				{
					if (i < count - 1)
					{
						core::vector3df start = m_vecCoords[i];
						core::vector3df end = m_vecCoords[i + 1];
						if (m_bTrans)
						{
							double tmpX = start.X;
							double tmpY = start.Y;
							double tmpZ = start.Z;
							m_absModel.Translate(tmpX,tmpY,tmpZ);
							start.X = (float)tmpX;
							start.Y = (float)tmpY;
							start.Z = (float)tmpZ;

							tmpX = end.X;
							tmpY = end.Y;
							tmpZ = end.Z;

							m_absModel.Translate(tmpX,tmpY,tmpZ);
							end.X = (float)tmpX;
							end.Y = (float)tmpY;
							end.Z = (float)tmpZ;
						}
						driver->draw3DLine(start,end,m_Color);
					}
					else 
					{
						// 闭合线段
						if (m_bClosure)
						{
							core::vector3df start = m_vecCoords[count - 1];
							core::vector3df end = m_vecCoords[0];
							if (m_bTrans)
							{
								double tmpX = start.X;
								double tmpY = start.Y;
								double tmpZ = start.Z;
								m_absModel.Translate(tmpX,tmpY,tmpZ);
								start.X = tmpX;
								start.Y = tmpY;
								start.Z = tmpZ;

								tmpX = end.X;
								tmpY = end.Y;
								tmpZ = end.Z;

								m_absModel.Translate(tmpX,tmpY,tmpZ);
								end.X = tmpX;
								end.Y = tmpY;
								end.Z = tmpZ;
							}
							driver->draw3DLine(start,end,m_Color);
						}
					}
				}
			}
			// 绘制二维线 屏幕坐标
			else
			{
				for (int i = 0; i< count;i++)
				{
					if (i < count - 1)
					{
						core::vector3df start = m_vecCoords[i];
						core::vector3df end = m_vecCoords[i + 1];
			
						core::vector2di sStart(start.X,start.Y);
						core::vector2di sEnd(end.X,end.Y);
					
						driver->draw2DLine(sStart,sEnd,m_Color);
					}
					else 
					{
						// 闭合线段
						if (m_bClosure)
						{
							core::vector3df start = m_vecCoords[count - 1];
							core::vector3df end = m_vecCoords[0];
							core::vector2di sStart(start.X,start.Y);
							core::vector2di sEnd(end.X,end.Y);

							driver->draw2DLine(sStart,sEnd,m_Color);
						}
					}
				}
			}
		}

		const core::aabbox3d<f32>& CIPolylineSceneNode::getBoundingBox() const
		{
			return m_BBox;
		}

		void CIPolylineSceneNode::UpdateBoundingBox()
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

			// 未进行转换时，才设置变换模型 变换模型设置后，不能再设置
		/*	if (!m_bTrans)
			{
				CBursaWolfModel* pBursaModel = m_pView->GetTransModel();

				if (m_absModel == *pBursaModel)
				{
					m_bTrans = false;
				}
				else
				{
					CBursaWolfModel ivtModel = pBursaModel->getAntiModel();
					m_absModel = ivtModel*m_absModel;
					m_bTrans = true;
				}
			}*/

			for (int i = 0; i< count;i++)
			{
				core::vector3df pt = m_vecCoords[i];

				if (m_bTrans)
				{
					double tmpX = pt.X;
					double tmpY = pt.Y;
					double tmpZ = pt.Z;
					m_absModel.Translate(tmpX,tmpY,tmpZ);
					pt.X = tmpX;
					pt.Y = tmpY;
					pt.Z = tmpZ;
				}
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

		void CIPolylineSceneNode::AddCoordinate(core::vector3df coord)
		{
			m_vecCoords.push_back(coord);
		}

		bool CIPolylineSceneNode::GetVertexCoord(int i,core::vector3df& coord)
		{
			if (i < 0 || i >= m_vecCoords.size())
			{
				return false;
			}

			coord = m_vecCoords[i];

			return true;
		}

		void CIPolylineSceneNode::ClearAllVertex()
		{
			m_vecCoords.clear();
		}

		video::SMaterial& CIPolylineSceneNode::getMaterial(u32 i)
		{
			return m_Material;
		}

		u32 CIPolylineSceneNode::getMaterialCount() const
		{
			return 1;
		}

		void CIPolylineSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
		{
		}

		void CIPolylineSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
		{
		}

		ISceneNode* CIPolylineSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
		{
			if (!newParent)
				newParent = Parent;
			if (!newManager)
				newManager = SceneManager;

			CIPolylineSceneNode* nb = new CIPolylineSceneNode(newParent, newManager, ID);

			nb->cloneMembers(this, newManager);

			if ( newParent )
				nb->drop();
			return nb;
		}
	}
}


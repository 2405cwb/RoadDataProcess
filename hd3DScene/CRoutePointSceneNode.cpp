#include "StdAfx.h"
#include "..\hd3DEngine\include\IVideoDriver.h"
#include "..\hd3DEngine\include\ITexture.h"
#include "..\hdFramework\hdCommandDef.h"
#include "CRoutePointSceneNode.h"
#include "Hd3DView.h"

using namespace irr;
using namespace irr::video;

namespace hd
{
	namespace scene
	{
		CRoutePointSceneNode::CRoutePointSceneNode(CHdSxPoint3D point, int index,ISceneNode* parent, ISceneManager* mgr, s32 id,bool bVaRoute,float fRadius)
			: IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id)
		{
			m_bVaRoute = bVaRoute;
			m_nIndex = index;
			m_point = point;

			m_bDrawID = false;

			IVideoDriver* driver = SceneManager->getVideoDriver();

			if (driver)
			{
				string filePath = getCurrentDir();
				filePath += "routepoint.jpg";
				//ITexture* rtext = driver->getTexture(filePath.data());

				m_Mesh = SceneManager->getGeometryCreator()->createSphereMesh(fRadius,32,32);
				//m_Mesh->getMeshBuffer(0)->getMaterial().setTexture(0,rtext);
				m_Mesh->getMeshBuffer(0)->getMaterial().setFlag(video::EMF_LIGHTING, false);
				m_Mesh->getMeshBuffer(0)->getMaterial().setFlag(video::EMF_ZBUFFER, true);

			}
			setAutomaticCulling(irr::scene::EAC_OFF);
			//RecalculateBBox();
		}


		CRoutePointSceneNode::~CRoutePointSceneNode(void)
		{
			if (m_Mesh)
			{
				//getSceneManager()->getVideoDriver()->removeTexture(GetTexture());
				getSceneManager()->getVideoDriver()->removeHardwareBuffer(m_Mesh->getMeshBuffer(0));
				if (m_Mesh->drop())
				{
					m_Mesh = NULL;
				}
			}
		}

		void CRoutePointSceneNode::OnRegisterSceneNode()
		{
			RecalculateBBox();

			if (IsVisible)
				SceneManager->registerNodeForRendering(this);

			ISceneNode::OnRegisterSceneNode();
		}


		void CRoutePointSceneNode::render()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

			if (!camera || !driver ||!m_pView)
				return;

			if (!m_pView->IsViewRenderAllNode())
			{
				return;
			}

			ENUM_HD_VIEW_TYPE type = m_pView->GetViewType();
			
			if (type == E_HVT_ORTHO3D ||  type == E_HVT_3D || type == E_HVT_FACADEEDIT || type == E_HVT_DOM_3D_VIEW)
			{
				CHd3DView* p3dView = dynamic_cast<CHd3DView*>(m_pView);

				if (!p3dView)
				{
					return;
				}

				if (p3dView->getCameraTool()->GetID() == COMMAND_3D_ORTHO_CAMERA 
					|| p3dView->getCameraTool()->GetID() == COMMAND_3D_CAMERA 
					|| p3dView->getCameraTool()->GetID() == COMMAND_3D_FPSCAMERA
					|| p3dView->getCameraTool()->GetID() == COMMAND_3D_DOM_CAMERA)
				{
					core::matrix4 mat(AbsoluteTransformation);
					mat.setTranslation(getPosition());

					driver->setTransform(video::ETS_WORLD, mat);
					driver->setMaterial(m_Mesh->getMeshBuffer(0)->getMaterial());
					driver->drawMeshBuffer(m_Mesh->getMeshBuffer(0));

					core::vector3df pos(m_point.m_x, m_point.m_y, m_point.m_z);

                    if (m_bDrawID)
					{
						// 获取包围盒的中心点
						//core::vector3df pos = getPosition();

						// 根据轨迹球心坐标获取屏幕坐标
						core::position2di ScreenPos = getSceneManager()->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(pos);

						// 绘制索引号
						core::stringw tmp;
						tmp += m_nIndex;

						// 构建字体
						gui::IGUIFont* pGUIFont = getSceneManager()->GetBuiltInFont();

						//绘制标签名
						core::rect<s32> rect(ScreenPos, ScreenPos + core::position2di(40,40));
						pGUIFont->draw(tmp, rect, video::SColor(255,255,0,0), true, true);
						//pGUIFont->draw()
					}
					else
					{
						driver->drawSphere(pos, 0.5f, video::SColor(255,255,0,0));
					}

					//driver->draw3DBox(m_Mesh->getMeshBuffer(0)->getBoundingBox(), video::SColor(255,0,255,0));
				}
			}
		}

		const core::aabbox3d<f32>& CRoutePointSceneNode::getBoundingBox() const
		{
			return m_Mesh->getMeshBuffer(0)->getBoundingBox();
		}

		video::SMaterial& CRoutePointSceneNode::getMaterial(u32 i)
		{
			return m_Mesh->getMeshBuffer(0)->getMaterial();
		}

	
		u32 CRoutePointSceneNode::getMaterialCount() const
		{
			return 1;
		}


		void CRoutePointSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
		{
		}



		void CRoutePointSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
		{
		}


		ISceneNode* CRoutePointSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
		{
			if (!newParent)
				newParent = Parent;
			if (!newManager)
				newManager = SceneManager;

			CRoutePointSceneNode* nb = new CRoutePointSceneNode(m_point, m_nIndex,newParent, newManager, ID);

			nb->cloneMembers(this, newManager);
			nb->getMaterial(0) = m_Mesh->getMeshBuffer(0)->getMaterial();
			if ( newParent )
				nb->drop();
			return nb;
		}

		void CRoutePointSceneNode::RecalculateBBox()
		{
		/*	double X = 0,Y = 0,Z = 0;
			X = m_point.m_x;
			Y = m_point.m_y;
			Z = m_point.m_z;*/

		/*	m_absModel.AntiTranslate(X,Y,Z);
			float centerX = (float)X;
			float centerY = (float)Y;
			float centerZ = (float)Z;*/

			m_BBox.MinEdge.set(m_point.m_x - 0.5f,m_point.m_y - 0.5f,m_point.m_z - 0.5f);
			m_BBox.MaxEdge.set(m_point.m_x + 0.5f,m_point.m_y + 0.5f,m_point.m_z + 0.5f);

			//core::vector3df pos = getPosition();
			//m_BBox.MinEdge.set(pos.X - 0.5f,pos.Y - 0.5f,pos.Z - 0.5f);
			//m_BBox.MaxEdge.set(pos.X + 0.5f,pos.Y + 0.5f,pos.Z + 0.5f);

			//m_Mesh->getMeshBuffer(0)->recalculateBoundingBox();
		}
	}
}


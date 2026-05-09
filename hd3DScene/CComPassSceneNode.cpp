
#include "StdAfx.h"
#include "CComPassSceneNode.h"
#include "COpenGLExtensionHandler.h"
#include <gl/GLU.h>
#include "hd3DView.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	namespace scene
	{
		
		CComPassSceneNode::CComPassSceneNode(ISceneNode* parent,ISceneManager* mgr,s32 id )
			:IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent,mgr,id)
		{
			m_material.Wireframe = true;
			m_material.Lighting = false;
			m_material.Thickness = 2;
			m_box.MinEdge.X = 0;
			m_box.MinEdge.Y = 0;
			m_box.MinEdge.Z = 0;

			m_box.MaxEdge.X = 1;
			m_box.MaxEdge.Y = 1;
			m_box.MaxEdge.Z = 1;

			m_Xcolor = video::SColor(255,255,0,0);
			m_Ycolor = video::SColor(255,0,255,0);
			m_Zcolor = video::SColor(255,0,0,255);

			//setAutomaticCulling(irr::scene::EAC_OFF);
		}

		CComPassSceneNode::~CComPassSceneNode(void)
		{

		}

		void CComPassSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
			{
				SceneManager->registerNodeForRendering(this,irr::scene::ESNRP_AXIS);
			}
			ISceneNode::OnRegisterSceneNode();
		}

		void CComPassSceneNode::render()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

			if (!camera || !driver || !m_pView)
				return;

			if (!m_pView->IsViewRenderAllNode())
			{
				return;
			}
			driver->setMaterial(m_material);
			driver->setTransform(video::ETS_WORLD,AbsoluteTransformation);//core::matrix4()
			driver->setRenderStates3DMode();
			//yf 2012/7/31 在视图的左下角绘制坐标轴
			//得到坐标轴的屏幕坐标
			core::vector3df camPos = camera->getPosition();

			//得到相机的target
			f32 fov = camera->getFOV();

			core::vector3df tPos = camera->getTarget();
			core::vector3df vect = (tPos - camPos).normalize();
			tPos = camPos + 10.0f*vect;

			// 在正交投影下，距离是透视投影的50倍，yf 2013-05-24
			float length(0.0f);
			if (!camera->isOrthogonal() )
			{
				length = tan(fov/2)/(tan(core::PI / 5.0f));
			}
			else
			{
				length = 50*tan(fov/2)/(tan(core::PI / 5.0f));
			}

			//float length = tan(fov/2)/(tan(core::PI / 5.0f));

			core::vector3df xPos(tPos.X + length, tPos.Y, tPos.Z);
			core::vector3df yPos(tPos.X, tPos.Y + length, tPos.Z);
			core::vector3df zPos(tPos.X, tPos.Y, tPos.Z + length);

			// 需要根据scenenode中的模型参数来绘制 坐标轴与点云scenenode的模型参数应该保持一致
			m_absModel.Translate(tPos.X,tPos.Y,tPos.Z);
			m_absModel.Translate(xPos.X,xPos.Y,xPos.Z);
			m_absModel.Translate(yPos.X,yPos.Y,yPos.Z);
			m_absModel.Translate(zPos.X,zPos.Y,zPos.Z);

			ISceneCollisionManager* scColn = getSceneManager()->getSceneCollisionManager();
			core::vector2di tScPos = scColn->getScreenCoordinatesFrom3DPosition(tPos);
			core::vector2di xSCPos = scColn->getScreenCoordinatesFrom3DPosition(xPos);
			core::vector2di ySCPos = scColn->getScreenCoordinatesFrom3DPosition(yPos);
			core::vector2di zSCPos = scColn->getScreenCoordinatesFrom3DPosition(zPos);

			//得到坐标轴平移后原点的屏幕坐标
			core::vector2di centerPt;

			CHd3DView* p3Dview = dynamic_cast<CHd3DView*>(m_pView);
			p3Dview->GetComPassPoint(centerPt.X,centerPt.Y);

			//平稳坐标轴
			xSCPos += centerPt - tScPos;
			ySCPos += centerPt - tScPos;
			zSCPos += centerPt - tScPos;

			// 正交投影下，以center为中心，画固定长度的坐标轴 yf 2013-5-24 
			if (camera->isOrthogonal() )
			{
				// 鬼火的vector2di有精度损失，因为normalize返回的是浮点数，yf 2013-05-24
				// 只能先得到浮点数，再强转成int
				core::vector2df deltaX = core::vector2df((f32)(xSCPos.X-centerPt.X), (f32)(xSCPos.Y-centerPt.Y)).normalize();
				core::vector2df deltaY = core::vector2df((f32)(ySCPos.X-centerPt.X), (f32)(ySCPos.Y-centerPt.Y)).normalize();
				core::vector2df deltaZ = core::vector2df((f32)(zSCPos.X-centerPt.X), (f32)(zSCPos.Y-centerPt.Y)).normalize();

				core::vector2df xSCPosf = (core::vector2df((f32)(centerPt.X), (f32)(centerPt.Y)) + (deltaX*50));
				core::vector2df ySCPosf = (core::vector2df((f32)(centerPt.X), (f32)(centerPt.Y)) + (deltaY*50));
				core::vector2df zSCPosf = (core::vector2df((f32)(centerPt.X), (f32)(centerPt.Y)) + (deltaZ*50));

				xSCPos = core::vector2di((int)(xSCPosf.X+0.5), (int)(xSCPosf.Y+0.5));
				ySCPos = core::vector2di((int)(ySCPosf.X+0.5), (int)(ySCPosf.Y+0.5));
				zSCPos = core::vector2di((int)(zSCPosf.X+0.5), (int)(zSCPosf.Y+0.5));
			}
		
			// 蔡红云 2013/8/17 画圆柱坐标轴
			// 把获得的屏幕坐标转换成3d坐标，然后再画坐标轴
			const SViewFrustum* pViewFrustum = camera->getViewFrustum();
			core::plane3d<f32> nearPlane = pViewFrustum->planes[1];
			core::plane3d<f32> farPlane = pViewFrustum->planes[0];
			core::plane3d<f32> midPlane;
			midPlane.D= (nearPlane.D+farPlane.D)/2;

			midPlane.Normal = nearPlane.Normal ;

			// 算出x y z 3个方向的距离长度, 确保圆柱的长度不为0
			core::vector3df lastNearIntersection, NearIntersection;

			// 和近平面求交得到3D坐标，能保证透视投影下看到坐标轴
			// x方向
			core::line3df lineNear1X = scColn->getRayFromScreenCoordinates(core::vector2di(centerPt.X, centerPt.Y));
			nearPlane.getIntersectionWithLine(lineNear1X.start,lineNear1X.getVector().normalize(),lastNearIntersection);
			core::line3df lineNear2X = scColn->getRayFromScreenCoordinates(core::vector2di(xSCPos.X, xSCPos.Y));
			nearPlane.getIntersectionWithLine(lineNear2X.start,lineNear2X.getVector().normalize(),NearIntersection);
			core::vector3df vectNearX = lastNearIntersection - NearIntersection;
			double	dAxisLengthX = (vectNearX).getLength();

			// y方向
			core::line3df lineNear1Y = scColn->getRayFromScreenCoordinates(core::vector2di(centerPt.X, centerPt.Y));
			nearPlane.getIntersectionWithLine(lineNear1Y.start,lineNear1Y.getVector().normalize(),lastNearIntersection);
			core::line3df lineNear2Y = scColn->getRayFromScreenCoordinates(core::vector2di(ySCPos.X, ySCPos.Y));
			nearPlane.getIntersectionWithLine(lineNear2Y.start,lineNear2Y.getVector().normalize(),NearIntersection);
			core::vector3df vectNearY = lastNearIntersection - NearIntersection;
			double	dAxisLengthY = (vectNearY).getLength();

			// z方向
			core::line3df lineNear1Z = scColn->getRayFromScreenCoordinates(core::vector2di(centerPt.X, centerPt.Y));
			nearPlane.getIntersectionWithLine(lineNear1Z.start,lineNear1Z.getVector().normalize(),lastNearIntersection);
			core::line3df lineNear2Z = scColn->getRayFromScreenCoordinates(core::vector2di(zSCPos.X, zSCPos.Y));
			nearPlane.getIntersectionWithLine(lineNear2Z.start,lineNear2Z.getVector().normalize(),NearIntersection);
			core::vector3df vectNearZ = lastNearIntersection - NearIntersection;
			double	dAxisLengthZ = (vectNearZ).getLength();

			double	dAxisLength = dAxisLengthX;

			if (dAxisLengthX == 0 && dAxisLengthY != 0)
			{
				dAxisLength = dAxisLengthY;
			}

			if (dAxisLengthX == 0 && dAxisLengthY == 0 && dAxisLengthZ != 0)
			{
				dAxisLength = dAxisLengthZ;
			}
			double	dAxisRadius = dAxisLength/25;
			double	dArrowLength = dAxisLength/6;
			double	dArrowRadius = dAxisLength/12;
			int		iSlices = 10;
			int		iStacks = 2;
			driver->DrawCompass(lastNearIntersection, dAxisLength, dAxisRadius, dArrowLength,
				dArrowRadius, iSlices, iStacks, NULL, NULL ,NULL ,true);

			// 2013/8/17 蔡红云重新根据三维坐标点来画X，Y，Z字体
			double dArrowPosn = dAxisLength - (dArrowLength/2);
			core::vector3df tmp;
			tmp.set(0,0,dArrowPosn + dArrowRadius*3);

			// 获取屏幕坐标z
			core::vector3df z3d = lastNearIntersection+tmp;
			core::vector2di znewSCPos = scColn->getScreenCoordinatesFrom3DPosition(z3d);

			// 获取屏幕坐标x
			core::vector3df x3d = lastNearIntersection;
			x3d.rotateXZBy(90,lastNearIntersection);
			tmp.set(dArrowPosn + dArrowRadius*3, 0, 0);
			x3d = x3d + tmp;
			core::vector2di xnewSCPos = scColn->getScreenCoordinatesFrom3DPosition(x3d);

			// 获取屏幕坐标y
			core::vector3df y3d = lastNearIntersection;
			y3d.rotateYZBy(-90,lastNearIntersection);
			tmp.set( 0, dArrowPosn + dArrowRadius*3,0);
			y3d = y3d + tmp;
			core::vector2di ynewSCPos = scColn->getScreenCoordinatesFrom3DPosition(y3d);
			gui::IGUIFont* pGUIFont = getSceneManager()->GetBuiltInFont();
			core::dimension2d<s32> dim(10,10);
			core::recti rtX1(xSCPos, dim);
			core::recti rtX(xnewSCPos, dim);
			pGUIFont->draw("E", rtX, m_Xcolor, true, true);
			core::recti rtY(ynewSCPos, dim);
			pGUIFont->draw("N", rtY, m_Ycolor, true, true);
			core::recti rtZ(znewSCPos, dim);
		/*	pGUIFont->draw("Z", rtZ, m_Zcolor, true, true);*/
	
		}

		const core::aabbox3d<f32>& CComPassSceneNode::getBoundingBox() const
		{
			return m_box;
		}

		irr::u32 CComPassSceneNode::getMaterialCount() const
		{
			return 1;
		}

		video::SMaterial& CComPassSceneNode::getMaterial( u32 i )
		{
			return m_material;
		}

	}// end of namespace scene
}// end of namespace hd

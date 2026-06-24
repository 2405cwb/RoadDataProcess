#include "StdAfx.h"
#include "HdRadioLegendSceneNode.h"

namespace hd
{
	namespace scene
	{
		CHdRadioLegendSceneNode::CHdRadioLegendSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id)
			:IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id)
		{
			setAutomaticCulling(EAC_OFF);

			Material.Lighting = false;
			Material.ZBuffer = video::ECFN_NEVER;
			Material.ZWriteEnable = false;
			Material.AntiAliasing = video::EAAM_OFF;
			Box.MaxEdge.set(0,0,0);
			Box.MinEdge.set(0,0,0);
		}

		//! 析构
		CHdRadioLegendSceneNode::~CHdRadioLegendSceneNode(void)
		{

		}

		//! 注册
		void CHdRadioLegendSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
				SceneManager->registerNodeForRendering(this,ESNRP_AXIS);

			ISceneNode::OnRegisterSceneNode();
		}

		void CHdRadioLegendSceneNode::render()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();
			gui::IGUIFont* pGUIFont = getSceneManager()->GetBuiltInFont();

			if (!camera || !driver || !m_pView)
			{		
				return;
			}
			if (!m_pView->IsViewRenderAllNode())
			{
				return;
			}

			// 获得屏幕大小(像素值)
			core::dimension2di size = (core::dimension2di)driver->getCurrentRenderTargetSize();

			// 由相机pos及target构成的射线与近平面相交点
			core::plane3df nearPlane = camera->getViewFrustum()->planes[camera->getViewFrustum()->VF_NEAR_PLANE];
			core::vector3df NearDown;
			core::vector3df NearUp;

			// 由设备DPI获得一英寸对应像素值
			HDC hDc = ::GetDC(m_pView->GetHWnd());
			int nLen = ::GetDeviceCaps(hDc,LOGPIXELSX); // 屏幕横向方向
			int nLegend = (int)(nLen / 2.54); // 一英寸对应25.4mm
			ReleaseDC(m_pView->GetHWnd(), hDc); 

			// 获得相机与指定像素处屏幕交线
			core::line3df lineNear = SceneManager->getSceneCollisionManager()->getRayFromScreenCoordinates(core::vector2di(size.Width / 2, size.Height / 2));
			core::line3df lineNearTarg = SceneManager->getSceneCollisionManager()->getRayFromScreenCoordinates(core::vector2di(size.Width / 2 + nLegend, size.Height / 2));

			// 求得交点
			nearPlane.getIntersectionWithLine(lineNear.start,lineNear.getVector().normalize(),NearDown);
			nearPlane.getIntersectionWithLine(lineNearTarg.start,lineNearTarg.getVector().normalize(),NearUp);

			// 由三维坐标距离值计算1m对应屏幕坐标像素差
			double dDist = sqrtf(pow(NearDown.X - NearUp.X,2) + 
				pow(NearDown.Y - NearUp.Y,2) + pow(NearDown.Z - NearUp.Z,2));

			// 计算1m相差像素值
			if (dDist > 0.0)
			{
				// 由此像素值在屏幕上绘制
				int height = size.Height - 30;

				core::position2d<s32> start;
				start.X = size.Width - 30 - nLegend;
				start.Y = height;

				core::position2d<s32> startTmp;
				startTmp.X = size.Width - 30 - nLegend;
				startTmp.Y = height - 5;

				// 绘制比例凸点
				driver->draw2DLine(start,startTmp,SColor(255,255,255,0));

				core::position2d<s32> end;
				end.X = start.X + nLegend;
				end.Y = start.Y;

				core::position2d<s32> endTmp;
				endTmp.X = start.X + nLegend;
				endTmp.Y = start.Y - 5;

				// 绘制比例凸点
				driver->draw2DLine(end,endTmp,SColor(255,255,255,0));

				// 绘制比例连线
				driver->draw2DLine(start,end,SColor(255,255,255,0));

				// 显示1cm对应三维距离字样
				core::position2d<s32> start2d;
				start2d.X = (start.X + end.X) / 2 - 10;
				start2d.Y = start.Y - 20;
				core::rect<s32> rect(start2d, core::dimension2d<s32>(20,10));

				char strDist[32];
				sprintf(strDist, "%.2lf%s",dDist,"m");
				pGUIFont->draw(strDist,rect,SColor(255,255,255,0),true,true);
			}
		}

		//! 包围盒
		const core::aabbox3d<f32>& CHdRadioLegendSceneNode::getBoundingBox() const
		{
			return Box;
		}

		//! 获得材质
		video::SMaterial& CHdRadioLegendSceneNode::getMaterial(u32 i)
		{
			return Material;
		}

		//! 材质
		u32 CHdRadioLegendSceneNode::getMaterialCount() const
		{
			return 1;
		}
	}
}


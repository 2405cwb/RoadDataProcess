#include "StdAfx.h"
#include "CColorLegendSceneNode.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

using namespace irr::video;

namespace hd
{
	namespace scene
	{
		CColorLegendSceneNode::CColorLegendSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id)
			: IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id),
			m_colorRamp(COLORARGB(255,0,0,180),COLORARGB(255,180,0,0),10)
		{
			setAutomaticCulling(EAC_OFF);

			Material.Lighting = false;
			Material.ZBuffer = video::ECFN_NEVER;
			Material.ZWriteEnable = false;
			Material.AntiAliasing = video::EAAM_OFF;
			Box.MaxEdge.set(0,0,0);
			Box.MinEdge.set(0,0,0);
		}

		CColorLegendSceneNode::~CColorLegendSceneNode(void)
		{
		}

		void CColorLegendSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
				SceneManager->registerNodeForRendering(this,ESNRP_SKY_BOX);

			ISceneNode::OnRegisterSceneNode();
		}

		void CColorLegendSceneNode::render()
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

			core::dimension2di size = (core::dimension2di)driver->getCurrentRenderTargetSize();

			// 计算颜色带的宽度与高度 以像素为单位
			// 渐变色带有10个渐变颜色
			// 采用绘制彩色直线的方式
			int height = size.Height - 30; 
			u32 a = 0,r,g,b;

			for (int i = 0; i< 10;i++)
			{
				for (int j = 0; j< 20;j++)
				{
					SColor color;
	
					m_colorRamp.GetColor4i((float)j/20.f,a,r,g,b,i + 1);
					color.set(a,r,g,b);

					core::position2d<s32> start;
					start.X = size.Width - 50;
					start.Y = height - (i*20 + j);

					core::position2d<s32> end;
					end.X = start.X - 20;
					end.Y = start.Y;

					driver->draw2DLine(start,end,color);
				}
			}

			// 绘制文字--起始颜色
			SColor colorStart;
			m_colorRamp.GetColor4i(0.0f,a,r,g,b,1);
			colorStart.set(a,r,g,b);
			core::position2d<s32> start;
			start.X = size.Width -50;
			start.Y = height - 10;
			core::rect<s32> rect(start/* + core::position2di(5,0)*/, core::dimension2d<s32>(50,20));
			char strDist[32];
			sprintf(strDist, "- %0.1f", m_fMinValue);
			pGUIFont->draw(strDist,rect,colorStart,false,true);

			// 中间值颜色
			SColor colorMid;
			m_colorRamp.GetColor4i(1.0f,a,r,g,b,5);
			colorMid.set(a,r,g,b);
			core::position2d<s32> mid;
			mid.X = size.Width -50;
			mid.Y = height - 100 - 10;
			core::rect<s32> rectMid(mid/* + core::position2di(5,0)*/, core::dimension2d<s32>(50,20));
			float fMidValue = (m_fMaxValue + m_fMinValue)/ 2.0f;
			sprintf(strDist, "- %0.1f", fMidValue);
			pGUIFont->draw(strDist,rectMid,colorMid,false,true);

			// 终止颜色
			SColor colorEnd;
			m_colorRamp.GetColor4i(1.0f,a,r,g,b,10);
			colorEnd.set(a,r,g,b);
			core::position2d<s32> end;
			end.X = size.Width - 50;
			end.Y = height - 200 - 10;
			core::rect<s32> rectEnd(end/* + core::position2di(5,0)*/, core::dimension2d<s32>(50,20));
			sprintf(strDist, "- %0.1f", m_fMaxValue);
			pGUIFont->draw(strDist,rectEnd,colorEnd,false,true);
		}

		const core::aabbox3d<f32>& CColorLegendSceneNode::getBoundingBox() const
		{
			return Box;
		}

		video::SMaterial& CColorLegendSceneNode::getMaterial(u32 i)
		{
			return Material;
		}

		u32 CColorLegendSceneNode::getMaterialCount() const
		{
			return 1;
		}

		void CColorLegendSceneNode::SetMaxMinValue( float maxValue,float minValue )
		{
			m_fMaxValue = maxValue;
			m_fMinValue = minValue;
		}

	}
}
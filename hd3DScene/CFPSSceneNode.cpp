/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : CFPSSceneNode.cpp
相关文件     : CFPSSceneNode.h
文件实现功能 : 实现FPS的显示
作者         : 软件部，危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容 
2012/07/18   1.0      危迟              新增加内容
</PRE>
*******************************************************************************/
#include "stdafx.h"
#include "CFPSSceneNode.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	namespace scene
	{
		CFPSSceneNode::CFPSSceneNode( ISceneNode* parent,ISceneManager* mgr,s32 id )
			:IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent,mgr,id)
		{
			m_material.Wireframe = false;
			m_material.Lighting = false;
			m_material.ZBuffer = false;			// 让该结点置于上层

			m_ptsSize = 0;
			m_triangleCount = 0;

			m_IsModelview = false;

			setAutomaticCulling(irr::scene::EAC_OFF);

			lastFPS = -1;

			m_bShowLod = false;

			m_bShowFps = true;
		}

		CFPSSceneNode::~CFPSSceneNode(void)
		{

		}

		void CFPSSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
			{
				SceneManager->registerNodeForRendering(this);
			}
			ISceneNode::OnRegisterSceneNode();
		}

		void CFPSSceneNode::render()
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

			int fps = driver->getFPS();

			//将三维坐标转换成为屏幕坐标
			//core::position2di screenPos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(centerPos);
		
			core::stringw strFPS("FPS:");

			if (m_bShowFps)
			{
				strFPS += fps;
				lastFPS = fps;
			}
			else 
			{
				strFPS = "";
			}

			// 获取显示点云的LOD级别[zhangfei 2014/6/5]
			char strRenderState[100];
			int nRenderLev = m_pView->GetPcdRenderState();
			if(nRenderLev >= 1 && m_bShowLod)
			{
				char strTmp[50];
				sprintf_s(strTmp,"LOD = %d", nRenderLev);
				strcpy(strRenderState, strTmp);
			}
			else 
			{
				char* strTmp = "";
				strcpy(strRenderState, strTmp);
			}

			// 得到西文字体
			gui::IGUIFont* pGUIFont = getSceneManager()->GetBuiltDefaultFont();
			core::stringw strwLOD = pGUIFont->CharToWchar(strRenderState);
			//strFPS += strRenderState;

			// 绘制FPS
			core::rect<s32> rectFPS(core::position2di(10,5), core::position2di(50,20));
			pGUIFont->draw(strFPS, rectFPS, video::SColor(255,255,0,0), false, true);

			// 绘制点云LOD
			bool bHasLod = (strwLOD.size() > 0) ? true : false;
			if (bHasLod)
			{
				core::rect<s32> rectLOD(core::position2di(10,20), core::position2di(50,40));
				pGUIFont->draw(strwLOD, rectLOD, video::SColor(255,255,0,0), false, true);
			}

			// 绘制模型视图下显示三角形数与点数 fengjing
			if (m_IsModelview)
			{
				core::stringw strwModel("Triangle Counts:");
				strwModel += m_triangleCount;

				// 如果有LOD，则三角形数显示在FPS下面的第三层位置，反之则在第二层位置
				if (bHasLod)
				{
					core::rect<s32> rectModel(core::position2di(10,40), core::position2di(50,60));
					pGUIFont->draw(strwModel, rectModel, video::SColor(255,255,0,0), false, true);
				}
				else
				{
					core::rect<s32> rectModel(core::position2di(10,20), core::position2di(50,40));					
					pGUIFont->draw(strwModel, rectModel, video::SColor(255,255,0,0), false, true);
				}
				
			}
		}

		irr::u32 CFPSSceneNode::getMaterialCount() const
		{
			return 1;
		}

		video::SMaterial& CFPSSceneNode::getMaterial( u32 i )
		{
			return m_material;
		}

	}// end of namespace scene
}// end of namespcae hd
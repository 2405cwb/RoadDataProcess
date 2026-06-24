#include "StdAfx.h"
#include "hdQuickView.h"
#include <io.h>
#include "hdCommandDef.h"
#include "CImageWriterBMP.h"
#include "CImageWriterPNG.h"
#include "CImageWriterJPG.h"
#include "irrlicht.h"
#include "CImage.h"
#include "..\hdCommon\sceneData\HdFileData.h"
#include "..\hdCommon\point_types.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

using namespace irr::video;
using namespace irr::io;

namespace hd
{
	namespace scene
	{
		CHdQuickView::CHdQuickView(void)
			:ISceneView()
		{
			m_viewType = E_HVT_QUICK ;
			m_pPanoSN = NULL;
		}

		CHdQuickView::~CHdQuickView(void)
		{

		}
				
		void CHdQuickView::SetCamera()
		{
			irr::scene::ICameraSceneNode* cam = m_irrDevice->getSceneManager()->addCameraSceneNode();
			cam->setUpVector(core::vector3df(0.0f,0.0f,1.0f));
			cam->setPosition(core::vector3df(0.0f,0.0f,0.0f));
			cam->setTarget(core::vector3df(0.0f,10.0f,0.0f));
			cam->setFarValue(2000);
		}
		
		//! 添加全景图片文件
		CPanoSceneNode* CHdQuickView::AddPanoObject(const CHdPanoPicFileData* pPanoFile)
		{
			if (pPanoFile == NULL || _access(pPanoFile->m_strFile.data(), 04) != 0)
			{
				return NULL;
			}
			if (m_pPanoSN)
			{
				GetSceneManager()->getRootSceneNode()->removeChild(m_pPanoSN);
				m_pPanoSN = NULL;
			}

			ISceneNode* pSN = NULL;
			if (_access(pPanoFile->m_strFile.data(), 04) == 0)
			{				
				pSN = AddObject(pPanoFile);
			}
			m_pPanoSN = dynamic_cast<CPanoSceneNode*>(pSN);
			return m_pPanoSN;
		}

		//! 添加全景图片内存对象
		CPanoSceneNode* CHdQuickView::AddPanoObject(const CHdPanoData* pPanoData)
		{
			if (pPanoData == NULL || pPanoData->m_panoData->pImageData == NULL || 
				pPanoData->m_panoData->nSize == 0)
			{
				return NULL;
			}
			if (m_pPanoSN)
			{
				GetSceneManager()->getRootSceneNode()->removeChild(m_pPanoSN);
				m_pPanoSN = NULL;
			}

			ISceneNode* pSN = NULL;
			pSN = AddObject(pPanoData);
			m_pPanoSN = dynamic_cast<CPanoSceneNode*>(pSN);
			return m_pPanoSN;
		}


		void CHdQuickView::GetSpherePosByImageScale(const core::vector2df& imageScale, core::vector3df& spherePos )
		{
			if(m_pPanoSN)
			{
				return m_pPanoSN->GetSpherePosByImageScale(imageScale,spherePos);
			}
		}

		/*void CHdQuickView::GetScreenPosByImageScale(const core::vector2df imgScale, core::vector2di& ptSC)
		{
			if (m_pPanoSN)
			{
				m_pPanoSN->GetScreenPosByImageScale(imgScale,ptSC);
			}
		}

		void CHdQuickView::GetImageScaleByScreenPos(const core::vector2di& ptSC, core::vector2df& imgScale)
		{
			if (m_pPanoSN)
			{
				m_pPanoSN->GetImageScaleByScreenPos(ptSC,imgScale);
			}
		}*/

		void CHdQuickView::GetAngleByScanPos(const core::vector3df& scanPos, core::vector2df& angle)
		{
			//////////////////////////////////////////
		
			double x = 0,y = 0;
		
			GetPointAngle(scanPos.X , scanPos.Y ,scanPos.Z , x, y);
			angle.X = (f32)x;
			angle.Y = (f32)y;
		}
		
		void CHdQuickView::GetSpherePosByScanPos(const core::vector3df& scanPos, core::vector3df& spherePos)
		{
			core::vector2df angle;
			GetAngleByScanPos(scanPos, angle);

			if (m_pPanoSN)
			{
				m_pPanoSN->GetSpherePos(angle, spherePos);
				
			}
		}
	}//end of namespace scene
}//end of namespace hd

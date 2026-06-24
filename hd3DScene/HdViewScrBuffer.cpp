#include "StdAfx.h"
#include "HdViewScrBuffer.h"
#include "ISceneView.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"

namespace hd
{
	namespace scene
	{

CHdViewScrBuffer::CHdViewScrBuffer(void)
	:m_viewType(0),m_srcPtCount(0),m_scrWidth(1),m_scrHeight(1),m_refView(NULL)
{

}

CHdViewScrBuffer::~CHdViewScrBuffer(void)
{
	ReleaseBuffer();
}

// 设置屏幕点,ptColor带渲染颜色,点坐标是视图相对坐标
int CHdViewScrBuffer::SetScrPoint(const PointXYZIPRGBA& ptColor,int srcX,int srcY)
{
	int pixIndex = srcY * m_scrWidth + srcX;
	if (pixIndex > m_scrWidth * m_scrHeight - 1 || pixIndex < 0)
	{
		return 0;
	}
	if (m_viewType == 0 && m_refView == NULL)
		return 0;

	m_srcPtCount++;
	ScrNearAndFarPoint*& srcPix = *(m_pts.data() + pixIndex);
	if(srcPix == NULL)
		srcPix = new ScrNearAndFarPoint();

	if (m_srcPtCount == 1)
	{
		srcPix->nearPt = ptColor;
		srcPix->farPt = ptColor;

		if (m_viewType == 0) // 3D视图
		{
			irr::scene::ICameraSceneNode* pCamera = m_refView->GetSceneManager()->getActiveCamera();
			const core::vector3df vec = pCamera->getPosition();
			core::vector3df vecFarPt(srcPix->farPt.x,srcPix->farPt.y,srcPix->farPt.z);
			core::vector3df vecNearPt(srcPix->farPt.x,srcPix->farPt.y,srcPix->farPt.z);
			srcPix->farDist = vec.getDistanceFrom(vecFarPt);
			srcPix->nearDist = vec.getDistanceFrom(vecNearPt);
		}
	}
	else
	{
		if (m_viewType == 0) // 3D视图
		{
			// 计算当前点离视点距离
			irr::scene::ICameraSceneNode* pCamera = m_refView->GetSceneManager()->getActiveCamera();
			const core::vector3df vec = pCamera->getPosition();			
			core::vector3df vecPt(ptColor.x,ptColor.y,ptColor.z);
			float dist = vec.getDistanceFrom(vecPt);
			// 更新像素点
			if (dist > srcPix->farDist)
			{
				srcPix->farPt = ptColor;
				srcPix->farDist = dist;
			}

			if (dist < srcPix->nearDist)
			{
				srcPix->nearPt = ptColor;
				srcPix->nearDist = dist;
			}
		}
		else if (m_viewType == 1)// 2D视图
		{
			// 更新低点,2D视图低点是远点
			if (ptColor.z < srcPix->farPt.z)
			{
				srcPix->farPt = ptColor;
			}

			// 更新低点,2D视图低点是远点
			if(ptColor.z > srcPix->nearPt.z)
			{
				srcPix->nearPt = ptColor;
			}
		}
	}

	return 1;
}

// 设置像素点及颜色,pt点坐标是视图相对坐标
int CHdViewScrBuffer::SetScrPoint(const PointXYZIPRGBA& pt,int srcX,int srcY,hd::u8 r,hd::u8 g,hd::u8 b,hd::u8 a)
{
	int pixIndex = srcY * m_scrWidth + srcX;
	if (pixIndex > m_scrWidth * m_scrHeight - 1 || pixIndex < 0)
	{
		return 0;
	}
	if (m_viewType == 0 && m_refView == NULL)
		return 0;

	m_srcPtCount++;
	ScrNearAndFarPoint*& srcPix = *(m_pts.data() + pixIndex);
	if(srcPix == NULL)
		srcPix = new ScrNearAndFarPoint();

	if (m_srcPtCount == 1)
	{
		srcPix->nearPt = pt;
		srcPix->nearPt.r = r;
		srcPix->nearPt.g = g;
		srcPix->nearPt.b = b;
		srcPix->nearPt.prop = a;

		srcPix->farPt = pt;
		srcPix->farPt.r = r;
		srcPix->farPt.g = g;
		srcPix->farPt.b = b;
		srcPix->farPt.prop = a;

		if (m_viewType == 0) // 3D视图
		{
			irr::scene::ICameraSceneNode* pCamera = m_refView->GetSceneManager()->getActiveCamera();
			const core::vector3df vec = pCamera->getPosition();
			core::vector3df vecFarPt(srcPix->farPt.x,srcPix->farPt.y,srcPix->farPt.z);
			core::vector3df vecNearPt(srcPix->farPt.x,srcPix->farPt.y,srcPix->farPt.z);
			srcPix->farDist = vec.getDistanceFrom(vecFarPt);
			srcPix->nearDist = vec.getDistanceFrom(vecNearPt);
		}
	}
	else
	{
		if (m_viewType == 0) // 3D视图
		{
			// 计算当前点离视点距离
			irr::scene::ICameraSceneNode* pCamera = m_refView->GetSceneManager()->getActiveCamera();
			const core::vector3df vec = pCamera->getPosition();			
			core::vector3df vecPt(pt.x,pt.y,pt.z);
			float dist = vec.getDistanceFrom(vecPt);
			// 更新像素点
			if (dist > srcPix->farDist)
			{
				srcPix->farPt = pt;
				srcPix->farPt.r = r;
				srcPix->farPt.g = g;
				srcPix->farPt.b = b;
				srcPix->farPt.prop = a;

				srcPix->farDist = dist;
			}

			if (dist < srcPix->nearDist)
			{
				srcPix->nearPt = pt;
				srcPix->nearPt.r = r;
				srcPix->nearPt.g = g;
				srcPix->nearPt.b = b;
				srcPix->nearPt.prop = a;

				srcPix->nearDist = dist;
			}
		}
		else if (m_viewType == 1)// 2D视图
		{
			// 更新低点,2D视图低点是远点
			if (pt.z < srcPix->farPt.z)
			{
				srcPix->farPt = pt;
				srcPix->farPt.r = r;
				srcPix->farPt.g = g;
				srcPix->farPt.b = b;
				srcPix->farPt.prop = a;
			}

			// 更新低点,2D视图低点是远点
			if(pt.z > srcPix->nearPt.z)
			{
				srcPix->nearPt = pt;
				srcPix->nearPt.r = r;
				srcPix->nearPt.g = g;
				srcPix->nearPt.b = b;
				srcPix->nearPt.prop = a;
			}
		}
	}

	return 1;
}

	}
}
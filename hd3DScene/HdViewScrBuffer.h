/*! HdViewScrBuffer.h
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : HdViewScrBuffer.h
相关文件     : HdViewScrBuffer.cpp
文件实现功能 : 实现点云屏幕缓存及平面碰撞检测
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2016/6/7     2.4      龚书林              创建
</PRE>
*******************************************************************************/

#pragma once
#include "stdafx.h"
#include "..\hdCommon\point_types.h"
namespace hd
{
	namespace scene
	{
struct ScrNearAndFarPoint
{
	ScrNearAndFarPoint()
		:nearDist(0.0f),farDist(0.0f){}
	// 像素位置近距离深度点,其中rgba为渲染颜色
	PointXYZIPRGBA nearPt;
	// 相机离nearPt点距离
	float          nearDist;
	// 像素位置近距离深度点,其中rgba为渲染颜色
	PointXYZIPRGBA farPt;
	// 相机离farPt点距离
	float          farDist;
};

class ISceneView;

class HD3DSCENE_API CHdViewScrBuffer
{
private:
	// 视图类型0代表3D,1代表2D
	int m_viewType;
	// 当前屏幕点数
	int m_srcPtCount;
	// 屏幕宽度
	int m_scrWidth;
	// 屏幕高度
	int m_scrHeight;
	// 屏幕缓存点
	std::vector<ScrNearAndFarPoint*> m_pts;
	// 三维视图
	ISceneView* m_refView;
public:
	CHdViewScrBuffer(void);
	~CHdViewScrBuffer(void);

	// 设置视图类型0代表3D,1代表2D
	void SetViewType(int viewType)
	{
		if (viewType == 0 || viewType == 1)
		{
			m_viewType = viewType;
		}
	}
	// 设置三维视图3D
	void Set3DView(ISceneView* pView)
	{
		m_refView = pView;
	}
	// 设置屏幕大小
	int SetScrSize(int width,int height)
	{
		if ((m_scrWidth == width && m_scrHeight == height) 
			|| m_scrWidth <= 0 || m_scrHeight <= 0
			|| m_scrWidth > 5000 || m_scrHeight > 5000)
		{
			return 0;
		}
		m_pts.clear();
		m_pts.resize(m_scrWidth * m_scrHeight);
		m_srcPtCount = 0;
	}
	
	// 释放内存
	void ReleaseBuffer()
	{
		ResetBuffer();
		m_pts.clear();
	}
	// 清空像素点
	void ResetBuffer()
	{
		for (std::vector<ScrNearAndFarPoint*>::iterator it = m_pts.begin();
			it != m_pts.end();it++)
		{
			if ((*it) != NULL)
			{
				delete *it;
				*it = NULL;
			}
		}
		m_srcPtCount = 0;
	}
	// 设置屏幕点,ptColor带渲染颜色,点坐标是视图相对坐标
	int SetScrPoint(const PointXYZIPRGBA& ptColor,int srcX,int srcY);
	
	// 设置像素点及颜色,pt点坐标是视图相对坐标
	int SetScrPoint(const PointXYZIPRGBA& pt,int srcX,int srcY,hd::u8 r,hd::u8 g,hd::u8 b,hd::u8 a);
			
	// 获取像素点颜色,默认获取近点,俯视图获取z值高点
	inline int GetPixelColor(int srcX,int srcY,hd::u8& r,hd::u8& g,hd::u8& b,hd::u8& a)
	{
		int pixIndex = srcY * m_scrWidth + srcX;
		if (pixIndex > m_scrWidth * m_scrHeight - 1 || pixIndex < 0)
		{
			return 0;
		}
		ScrNearAndFarPoint*& srcPix = *(m_pts.data() + pixIndex);
		if (srcPix == NULL)
		{
			return 0;
		}
		// 判断是否有效点
		if (fabs(srcPix->nearDist) < 0.00001 && fabs(srcPix->nearPt.z) < 0.00001)
		{
			return 0;
		}
		r = srcPix->nearPt.r;
		g = srcPix->nearPt.g;
		b = srcPix->nearPt.b;
		a = srcPix->nearPt.prop;
		return 1;
	}

	// 获取像素点坐标,默认获取近点,俯视图获取z值高点
	inline int GetPixelPoint(int srcX,int srcY,PointXYZIPRGBA& outPt)
	{
		int pixIndex = srcY * m_scrWidth + srcX;
		if (pixIndex > m_scrWidth * m_scrHeight - 1 || pixIndex < 0)
		{
			return 0;
		}
		ScrNearAndFarPoint*& srcPix = *(m_pts.data() + pixIndex);
		if (srcPix == NULL)
		{
			return 0;
		}
		// 判断是否有效点
		if (fabs(srcPix->nearDist) < 0.00001 && fabs(srcPix->nearPt.z) < 0.00001)
		{
			return 0;
		}
		outPt = srcPix->nearPt;
		return 1;
	}
};

	}
}
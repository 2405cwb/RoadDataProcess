#include "StdAfx.h"
#include "CIScanSceneNode.h"
#include "COpenGLExtensionHandler.h"
#include <time.h>
#include <algorithm>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{	
	namespace scene
	{
		//ISceneCollisionManager* g_sceneCollision = NULL;
		ISceneManager*			g_pSceneManage = NULL;
		core::matrix4			g_transMat;			// 三维坐标转换屏幕坐标矩阵
		core::dimension2d<u32>  g_dim;
		void SetTransMat()
		{
			if(g_pSceneManage)
			{
				ICameraSceneNode* pCamera = g_pSceneManage->getActiveCamera();
				IVideoDriver* pDriver = g_pSceneManage->getVideoDriver();
				const core::rect<s32>& viewPort = pDriver->getViewPort();
				g_dim.set(viewPort.getWidth() / 2,viewPort.getHeight() / 2);

				g_transMat = pCamera->getProjectionMatrix();
				g_transMat *= pCamera->getViewMatrix();
			}
		}

		BOOL ViewTrans(const f32& x,const f32& y,const f32& z,s32& srcX,s32& srcY)
		{
			f32 transformedPos[4] = { x, y, z, 1.0f };

			g_transMat.multiplyWith1x4Matrix(transformedPos);

			if (transformedPos[3] < 0)
			{
				srcX = -1;
				srcY = -1;
				return FALSE;
			}
			
			const f32 zDiv = transformedPos[3] == 0.0f ? 1.0f :1.0f/transformedPos[3];
			
			srcX = s32(g_dim.Width * transformedPos[0] * zDiv + 0.5f) + g_dim.Width;
			srcY = g_dim.Height - s32(g_dim.Height * (transformedPos[1] * zDiv) + 0.5);	
			return !(srcX < 0 || srcX > g_dim.Width * 2 || srcY < 0 || srcY > g_dim.Height * 2);
		}

CIScanSceneNode::CIScanSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id)
	:IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id),
	m_loadLimit(2000000)
{
	m_material.Wireframe = false;
	m_material.Lighting = false;
	m_material.Thickness = 2;
	m_material.ZWriteEnable = true;
	m_defaultClr.set(0.5,0.5,0.5);
	setAutomaticCulling(irr::scene::EAC_OFF);

	//g_sceneCollision = SceneManager->getSceneCollisionManager();
	g_pSceneManage = SceneManager;

	//m_ptBuf.resize(m_loadLimit);
}


CIScanSceneNode::~CIScanSceneNode(void)
{
}

void CIScanSceneNode::SetPath(const char* path)
{
	m_ptCloud.Open(path);
	m_ptCloud.LoadHlsFile();
	//m_ptCloud.QueryByExtent()
	m_box.MinEdge.set(
		m_ptCloud.m_hlsReader.m_header.min_x,
		m_ptCloud.m_hlsReader.m_header.min_y,
		m_ptCloud.m_hlsReader.m_header.min_z);

	m_box.MaxEdge.set(
		m_ptCloud.m_hlsReader.m_header.max_x,
		m_ptCloud.m_hlsReader.m_header.max_y,
		m_ptCloud.m_hlsReader.m_header.max_z);
}

void CIScanSceneNode::OnRegisterSceneNode()
{
	if (IsVisible)
	{
		SceneManager->registerNodeForRendering(this);
	}
	ISceneNode::OnRegisterSceneNode();
}

void CIScanSceneNode::render()
{
	m_pSceneView = dynamic_cast<ISceneView*>(m_pView);
	if(m_pSceneView == NULL)
		return;
	core::matrix4 mat(AbsoluteTransformation);
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	driver->setTransform(video::ETS_WORLD,mat);
	f32 lastSize = m_material.Thickness;
	
	bool bAni = SceneManager->IsAnimating();
	driver->setMaterial(m_material);

	driver->setRenderStates3DMode();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glRasterPos3f(10,10,0);

	glBegin(GL_POINTS);

	glColor4f(m_defaultClr.r, m_defaultClr.g, m_defaultClr.b,0.5);
	
	/*HLS2_LOOPINDEX* pLoopIdx = m_ptCloud.GetLoopIndex();
	u32 count = 0;
	for (u32 i = 0;i < m_ptCloud.m_hlsReader.m_header.number_of_col;i++)
	{
		const PointXYZI* vecPts = *(m_ptCloud.m_mapData._Myfirst + i);
		if (vecPts)
		{
			const HLS2_LOOPINDEX& loopIdx = pLoopIdx[i];
			count = loopIdx.size / sizeof(PointXYZI);
			for(u32 iPt = 0;iPt < count;)
			{
				const PointXYZI& pt = vecPts[iPt];
				glVertex3f(pt.x,pt.y,pt.z);
				if (bAni)
				{
					iPt += m_ptCloud.GetSimpleLevel();
				}
				else
					iPt++;
			}
		}
	}*/
	
	PointXYZIPRGBA* pPtBuf = NULL;
	m_ptCloud.ResetRead();
	u32 ptcount = 0;
	
	core::rect<s32> rect = driver->getViewPort();
	s32 width = rect.getWidth();
	s32 height = rect.getHeight();

	HRGN hRgn = CreateRectRgn(width / 2 - 20,height / 2 - 20,width / 2 + 20,height / 2 + 20);

#ifdef _DEBUG
	LARGE_INTEGER tFreq;
	QueryPerformanceFrequency(&tFreq);
	LARGE_INTEGER tStartQ;
	QueryPerformanceCounter(&tStartQ);
#endif

	ISceneCollisionManager* pSceneCol = SceneManager->getSceneCollisionManager();
	core::line3df line = pSceneCol->getRayFromScreenCoordinates(core::vector2di(width / 2,height / 2));
	core::vector3df pos;
	core::aabbox3df box;
	u32 inBox = 0;
	u64 inviewCount = m_ptCloud.count();

	u32 simpleLevel = inviewCount / m_loadLimit + 1;//m_ptCloud.getLoadSimple()
	simpleLevel = bAni?simpleLevel * 5:simpleLevel;
	u32 renderCount = 0;
	while(m_ptCloud.ReadNext(pPtBuf,ptcount))
	{
		if (pPtBuf)
		{
			/*m_ptCloud.GetExtent(box.MinEdge.X,box.MinEdge.Y,box.MinEdge.Z,box.MaxEdge.X,box.MaxEdge.Y,box.MaxEdge.Z);
			if(box.intersectsWithLine(line))
			{
				inBox++;
			}*/
			
			for (u32 i = 0;i<ptcount;)
			{
				const PointXYZIPRGBA& pt = pPtBuf[i];
				/*PointXYZ& ptxyz = *(m_ptBuf._Myfirst + renderCount);
				ptxyz.x = pt.x;
				ptxyz.y = pt.y;
				ptxyz.z = pt.z;*/
				glVertex3f(pt.x,pt.y,pt.z);
				renderCount++;
				//if (bAni)
				{
					i += simpleLevel;
				}
				//else
				//	i++;
			}
		}
	}
#ifdef _DEBUG
	LARGE_INTEGER tEndR;
	QueryPerformanceCounter(&tEndR);
#endif
	/*for (u32 i = 0;i<renderCount;i++)
	{
		PointXYZ& pt = *(m_ptBuf._Myfirst + i);
		glVertex3f(pt.x,pt.y,pt.z);
	}*/
	//driver->draw3DPointList(m_ptBuf._Myfirst,renderCount,NULL,E_VERTEX_TYPE::EVT_XYZFLOAT);
	m_ptCloud.EndRead();

#ifdef _DEBUG
	LARGE_INTEGER tEndR1;
	QueryPerformanceCounter(&tEndR1);
	char strDbg[256] = {0};
	sprintf(strDbg,"read:%lf,render:%lf,count:%d,%d\n",
		(double)(tEndR.QuadPart - tStartQ.QuadPart)/tFreq.QuadPart,
		(double)(tEndR1.QuadPart - tEndR.QuadPart)/tFreq.QuadPart,
		renderCount,simpleLevel);
	OutputDebugString(strDbg);
#endif

	//m_ptCloud.SelectPoint(hRgn,width,height,ViewTrans);
	/*u64 count = m_ptCloud.GetCount();
	for (u64 i = 0;i<count;)
	{
		const PointXYZI& pt = m_ptCloud[i];
		glVertex3f(pt.x,pt.y,pt.z);
		if (bAni)
		{
			i += m_ptCloud.GetSimpleLevel();
		}
		else
			i++;
	}*/
	DeleteObject(hRgn);

	glEnd();
	glFlush();
}

BOOL CIScanSceneNode::ReloadData()
{
	if(m_pView == NULL)
		return FALSE;
	int srcWidth = m_pView->GetWindowWidth();
	int srcHeight = m_pView->GetWindowHeight();
	SetTransMat();
	const core::aabbox3df& box = SceneManager->getActiveCamera()->getViewFrustum()->getBoundingBox();
	return m_ptCloud.LoadByViewPort(ViewTrans,srcWidth,srcHeight,
		CHdBox3df(box.MinEdge.X,box.MinEdge.Y,box.MinEdge.Z,
				  box.MaxEdge.X,box.MaxEdge.Y,box.MaxEdge.Z),SceneManager->getActiveCamera()->isOrthogonal());
	////return m_ptCloud.LoadByExtent(box.MinEdge.X,box.MinEdge.Y,box.MinEdge.Z,
	////	box.MaxEdge.X,box.MaxEdge.Y,box.MaxEdge.Z);
	////return m_ptCloud.LoadCurExtent<ISceneCollisionManager>(pCollision);

	//HLS2_LOOPINDEX* pLoopIdx = m_ptCloud.GetLoopIndex();
	//core::vector3df pt1,pt2,pt3,pt4;
	//core::vector3df pt5,pt6,pt7,pt8;
	//core::vector3df centroid;

	//core::position2di src1,src2,src3,src4;
	//core::position2di src5,src6,src7,src8;
	//core::position2di srcCentorid;
	
	//
	//vector<char> vecIntersect;
	//vecIntersect.resize(m_ptCloud.m_hlsReader.m_header.number_of_col);
	//u64 inViewCount = 0;
	//for (unsigned int i = 0;i < m_ptCloud.m_hlsReader.m_header.number_of_col;i++)
	//{
	//	const HLS2_LOOPINDEX& loopIdx = pLoopIdx[i];
	//	/*
	//	// 底平面4个点
	//	F32 x1 = loopIdx.xmin,y1 = loopIdx.ymin,z1 = loopIdx.zmin;
	//	F32 x2 = loopIdx.xmax,y2 = loopIdx.ymin,z2 = loopIdx.zmin;
	//	F32 x3 = loopIdx.xmax,y3 = loopIdx.ymax,z3 = loopIdx.zmin;
	//	F32 x4 = loopIdx.xmin,y4 = loopIdx.ymax,z4 = loopIdx.zmin;
	//	// 顶平面4个点
	//	F32 x5 = loopIdx.xmin,y5 = loopIdx.ymin,z5 = loopIdx.zmax;
	//	F32 x6 = loopIdx.xmax,y6 = loopIdx.ymin,z6 = loopIdx.zmax;
	//	F32 x7 = loopIdx.xmax,y7 = loopIdx.ymax,z7 = loopIdx.zmax;
	//	F32 x8 = loopIdx.xmin,y8 = loopIdx.ymax,z8 = loopIdx.zmax;

	//	pt1.set(x1,y1,z1);
	//	pt2.set(x2,y2,z2);
	//	pt3.set(x3,y3,z3);
	//	pt4.set(x4,y4,z4);
	//	pt5.set(x5,y5,z5);
	//	pt6.set(x6,y6,z6);
	//	pt7.set(x7,y7,z7);
	//	pt8.set(x8,y8,z8);
	//	centroid.set(loopIdx.centroidX,loopIdx.centroidY,loopIdx.centroidZ);

	//	src1 = pCollision->getScreenCoordinatesFrom3DPosition(pt1);
	//	src2 = pCollision->getScreenCoordinatesFrom3DPosition(pt2);
	//	src3 = pCollision->getScreenCoordinatesFrom3DPosition(pt3);
	//	src4 = pCollision->getScreenCoordinatesFrom3DPosition(pt4);
	//	src5 = pCollision->getScreenCoordinatesFrom3DPosition(pt5);
	//	src6 = pCollision->getScreenCoordinatesFrom3DPosition(pt6);
	//	src7 = pCollision->getScreenCoordinatesFrom3DPosition(pt7);
	//	src8 = pCollision->getScreenCoordinatesFrom3DPosition(pt8);
	//	srcCentorid = pCollision->getScreenCoordinatesFrom3DPosition(centroid);

	//	core::aabbox3df tmpBox(loopIdx.xmin,loopIdx.ymin,loopIdx.zmin,loopIdx.xmax,loopIdx.ymax,loopIdx.zmax);
	//	if ((!(src1.X < 0 || src1.X > srcWidth || src1.Y < 0 || src1.Y > srcHeight) ||
	//		!(src2.X < 0 || src2.X > srcWidth || src2.Y < 0 || src2.Y > srcHeight) ||
	//		!(src3.X < 0 || src3.X > srcWidth || src3.Y < 0 || src3.Y > srcHeight) ||
	//		!(src4.X < 0 || src4.X > srcWidth || src4.Y < 0 || src4.Y > srcHeight) ||
	//		!(src5.X < 0 || src5.X > srcWidth || src5.Y < 0 || src5.Y > srcHeight) ||
	//		!(src6.X < 0 || src6.X > srcWidth || src6.Y < 0 || src6.Y > srcHeight) ||
	//		!(src7.X < 0 || src7.X > srcWidth || src7.Y < 0 || src7.Y > srcHeight) ||
	//		!(src8.X < 0 || src8.X > srcWidth || src8.Y < 0 || src8.Y > srcHeight) ||
	//		!(srcCentorid.X < 0 || srcCentorid.X > srcWidth || srcCentorid.Y < 0 || srcCentorid.Y > srcHeight)))
	//	{
	//		inViewCount += (loopIdx.size / sizeof(PointXYZI));
	//		*(vecIntersect._Myfirst + i) = 1;
	//	}
	//	*/

	//	/*pt1.set(loopIdx.firstX,loopIdx.firstY,loopIdx.firstZ);
	//	pt2.set(loopIdx.middleX,loopIdx.middleY,loopIdx.middleZ);
	//	pt3.set(loopIdx.lastX,loopIdx.lastY,loopIdx.lastZ);
	//	src1 = pCollision->getScreenCoordinatesFrom3DPosition(pt1);
	//	src2 = pCollision->getScreenCoordinatesFrom3DPosition(pt2);
	//	src3 = pCollision->getScreenCoordinatesFrom3DPosition(pt3);
	//	if (!(src1.X < 0 || src1.X > srcWidth || src1.Y < 0 || src1.Y > srcHeight) ||
	//	!(src2.X < 0 || src2.X > srcWidth || src2.Y < 0 || src2.Y > srcHeight) ||
	//	!(src3.X < 0 || src3.X > srcWidth || src3.Y < 0 || src3.Y > srcHeight))
	//	{
	//	inViewCount += (loopIdx.size / sizeof(PointXYZI));
	//	*(vecIntersect._Myfirst + i) = 1;
	//	}*/

	//	/*core::aabbox3df tmpBox(loopIdx.xmin,loopIdx.ymin,loopIdx.zmin,loopIdx.xmax,loopIdx.ymax,loopIdx.zmax);
	//	if (box.intersectsWithBox(tmpBox) && 
	//	camPos.getDistanceFrom(core::vector3df(loopIdx.centroidX,loopIdx.centroidY,loopIdx.centroidZ)) < 100.0f)
	//	{
	//	inViewCount += (loopIdx.size / sizeof(PointXYZI));
	//	*(vecIntersect._Myfirst + i) = 1;
	//	}*/
	//}

	//u32 levelCount = 1;
	//if (inViewCount <= m_loadLimit)
	//{
	//	levelCount = 1;
	//}
	//else
	//{
	//	u64 ptcount = inViewCount;
	//	levelCount = 1;
	//	ptcount = (ptcount >> 1);
	//	while(ptcount > m_loadLimit)
	//	{
	//		levelCount = (levelCount<<1);
	//		ptcount = (ptcount >> 1);
	//	}
	//}

	//u64 inMemoCount = 0;
	//u32 colPtCount = 0;
	//BOOL bChange = FALSE;
	//for (u32 i = 0;i< m_ptCloud.m_hlsReader.m_header.number_of_col;i++)
	//{
	//	PointXYZI*& vecPts = *(m_ptCloud.m_mapData._Myfirst + i);
	//	const HLS2_LOOPINDEX& loopIdx = pLoopIdx[i];
	//	colPtCount = loopIdx.size / sizeof(PointXYZI);
	//	if (*(vecIntersect._Myfirst + i) == 1 && (i % levelCount) == 0)
	//	{
	//		if (vecPts == NULL)
	//		{
	//			//m_ptCloud.LoadLoop(i);
	//			bChange = TRUE;
	//		}			
	//	}
	//	else
	//	{
	//		if(vecPts)
	//		{
	//			delete[] vecPts;
	//			vecPts = NULL;
	//			bChange = TRUE;
	//		}
	//	}	

	//	if (vecPts)
	//	{
	//		inMemoCount += colPtCount;
	//	}
	//}
	//return bChange;
}

const core::aabbox3d<f32>& CIScanSceneNode::getBoundingBox() const
{
	return m_box;
}

irr::u32 CIScanSceneNode::GetMaterialCount() const
{
	return 1;
}

video::SMaterial& CIScanSceneNode::GetMaterial( u32 i )
{
	return m_material;
}

	}
}
/*! @file
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : HdPolySelectIn3DSceneNode.cpp
相关文件     : HdPolySelectIn3DSceneNode.h
文件实现功能 : 实现多边形的显示。
作者         : 软件部，朱旭波
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/10/18   1.0      朱旭波              新增加内容
</PRE>
*******************************************************************************/
#include "StdAfx.h"
#include "HdPolySelectIn3DSceneNode.h"
#include "..\..\hd3DEngine\include\IVideoDriver.h"
#include "..\..\hd3DEngine\include\ISceneManager.h"
#include "..\..\hd3DEngine\include\ICameraSceneNode.h"
#include "..\..\hd3DEngine\os.h"
#include "..\..\hdPointCloud\hdSysSetting.h"
//#include "hdApplication.h"
#include "..\..\hdFramework\hdView.h"
#include "Hd3DView.h"
#include <math.h>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	class CHdSxPolyline2D;

	namespace fm
	{

		CHdPolySelectIn3DSceneNode::CHdPolySelectIn3DSceneNode(const CHdSxPolyline2D* polyline2D, ISceneNode* parent, ISceneManager* mgr,	s32 id)
			: IObjectSceneNode(polyline2D,video::SColor(100,255,255,0),g_selColor,parent, mgr, id)
		{
			m_Material.Wireframe = false;
			m_Material.Lighting = false;

			m_pPolyline2D = polyline2D;
			m_bPolySuccess = true;
			AutomaticCullingState = EAC_OFF;//标记不需要判断范围
			m_polySelColor = video::SColor(120,255,255,0);
		}


		CHdPolySelectIn3DSceneNode::~CHdPolySelectIn3DSceneNode(void)
		{
		}

		//! pre render event
		void CHdPolySelectIn3DSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
				SceneManager->registerNodeForRendering(this);

			ISceneNode::OnRegisterSceneNode();
		}


		//! render
		void CHdPolySelectIn3DSceneNode::render()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

			if (!camera || !driver || !m_pPolyline2D ||!m_pView)
				return;

			{	
				// 根据系统设置中的选择线颜色进行设置
				COLORREF colorTemp = CHdSysSetting::getSysSetting()->measureSetting.selPLineColor;
				m_polySelColor = video::SColor(100,GetRValue(colorTemp),GetGValue(colorTemp),GetBValue(colorTemp));

				driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
				driver->setMaterial(m_Material);				

				ISceneView* pView = dynamic_cast<ISceneView*>(m_pView);
				ENUM_HD_VIEW_TYPE type = pView->GetViewType();

				unsigned int nPtCount = m_pPolyline2D->GetPointCount();		

				video::SColor polyColor = m_bSelected?m_SelectedColor:m_Color;

				//画二维多段线
				for (unsigned int i = 1; i<nPtCount; i++)
				{
					const CHdSxPoint2D& ptStart = m_pPolyline2D->GetPoint(i-1);
					const CHdSxPoint2D& ptEnd = m_pPolyline2D->GetPoint(i);					

					if (type == E_HVT_3D)
					{
						core::position2d<s32> Start,End;
						Start.X = (int)ptStart.m_x;
						Start.Y = (int)ptStart.m_y;
						End.X = (int)ptEnd.m_x;
						End.Y = (int)ptEnd.m_y;

						driver->draw2DLine(Start, End, m_polySelColor);

					}
				}

				if (nPtCount>2)
				{
					core::position2d<f32>* points2d = new core::position2d<f32>[nPtCount];

					//判断多边形是顺时针还是逆时针
					double crossProduct = 0.0;
					for (unsigned int i = 1; i<nPtCount-1; i++)
					{
						const CHdSxPoint2D& pt1 = m_pPolyline2D->GetPoint(i);
						const CHdSxPoint2D& pt0 = m_pPolyline2D->GetPoint(i-1);
						const CHdSxPoint2D& pt2 = m_pPolyline2D->GetPoint(i+1);
						crossProduct += (pt1.m_x - pt0.m_x)*(pt2.m_y - pt1.m_y) - (pt1.m_y - pt0.m_y)*(pt2.m_x - pt1.m_x);
					}
					if (crossProduct > 0)//顺时针
					{
						core::position2di scrPoint;

						if (type == E_HVT_3D )
						{
							for (unsigned int i = 0; i<nPtCount; i++)
							{
								const CHdSxPoint2D& pt = m_pPolyline2D->GetPoint(i);
								points2d[i].X = (float)pt.m_x;
								points2d[i].Y = (float)pt.m_y;
							}
						}
					}
					core::position2di scrPoint;
					if (crossProduct<0)//逆时针
					{
						if (type == E_HVT_3D )
						{
							for (unsigned int i = 0; i<nPtCount; i++)
							{
								const CHdSxPoint2D& pt = m_pPolyline2D->GetPoint(i);
								points2d[nPtCount - i -1].X = (float)pt.m_x;
								points2d[nPtCount - i -1].Y = (float)pt.m_y;
							}
						}
					}

					if (nPtCount == 3)
					{
						//画二维多边形					
						driver->draw2DTriangle( 
							points2d,
							nPtCount,
							m_polySelColor 
							);
					}
					else if (nPtCount >= 4)
					{
						core::position2d<f32>* Validpoints2d = new core::position2d<f32>[nPtCount - 1];
						core::position2d<f32>* tripoints2d = new core::position2d<f32>[nPtCount - 1];
						// 顺时针
						if (crossProduct > 0)
						{
							for (size_t i = 0; i< nPtCount - 1;i++)
							{
								Validpoints2d[i].X = points2d[i].X;
								Validpoints2d[i].Y = points2d[i].Y;
							}
						}
						// 逆时针
						else if (crossProduct < 0)
						{
							for (u32 i = 1; i< nPtCount;i++)
							{
								Validpoints2d[i - 1].X = points2d[i].X;
								Validpoints2d[i - 1].Y = points2d[i].Y;
							}
						}
						tripoints2d[0].X = Validpoints2d[0].X;
						tripoints2d[0].Y = Validpoints2d[0].Y;
						tripoints2d[1].X = Validpoints2d[nPtCount - 2].X;
						tripoints2d[1].Y = Validpoints2d[nPtCount - 2].Y;
						const CHdSxPoint2D& pt = m_pPolyline2D->GetPoint(nPtCount - 1);
						if (pView->GetViewType() == E_HVT_3D)
						{
							tripoints2d[2].X = (float)pt.m_x;
							tripoints2d[2].Y = (float)pt.m_y;
						}

						//画二维多边形					
						driver->draw2DPoly( 
							m_bPolySuccess,
							Validpoints2d,
							nPtCount - 1,
							m_polySelColor 
							);		
						//画三角形 		
						driver->draw2DTriangle(
							tripoints2d,
							3,
							m_polySelColor
							);
						delete[] Validpoints2d;
						delete[] tripoints2d;
					}
					delete[] points2d;
				}
			}
		}

		//! 因为CPolyline中的点在外部可能会被修改，所以BoudingBox需要在调用时实时计算
		const core::aabbox3d<f32>& CHdPolySelectIn3DSceneNode::getBoundingBox() const
		{
			return m_BBox;
		}

		void CHdPolySelectIn3DSceneNode::UpdateBoundingBox()
		{
			if (!m_pPolyline2D)
			{
				return;
			}

			const CHdSxPoint2D& minPt = m_pPolyline2D->GetBoxMinPt();
			const CHdSxPoint2D& maxPt = m_pPolyline2D->GetBoxMaxPt();
			m_BBox.MinEdge.X = (float)minPt.m_x;
			m_BBox.MinEdge.Y = (float)minPt.m_y;
			m_BBox.MinEdge.Z = 0.0f;
			m_BBox.MaxEdge.X = (float)maxPt.m_x;
			m_BBox.MaxEdge.Y = (float)maxPt.m_y;
			m_BBox.MaxEdge.Z = 0.0f;
		}

		video::SMaterial& CHdPolySelectIn3DSceneNode::getMaterial(u32 i)
		{
			return m_Material;
		}


		//! returns amount of materials used by this scene node.
		u32 CHdPolySelectIn3DSceneNode::getMaterialCount() const
		{
			return 1;
		}


		//! Writes attributes of the scene node.
		void CHdPolySelectIn3DSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
		{
		}


		//! Reads attributes of the scene node.
		void CHdPolySelectIn3DSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
		{
		}

		//! Creates a clone of this scene node and its children.
		ISceneNode* CHdPolySelectIn3DSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
		{
			if (!newParent)
				newParent = Parent;
			if (!newManager)
				newManager = SceneManager;

			CHdPolySelectIn3DSceneNode* nb = new CHdPolySelectIn3DSceneNode(m_pPolyline2D, newParent, newManager, ID);

			nb->cloneMembers(this, newManager);

			if ( newParent )
				nb->drop();
			return nb;
		}
	}//end of namespcace scene
}//end of namespace hd

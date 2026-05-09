/*! @file
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : CDomSymSceneNode.cpp
相关文件     : CDomSymSceneNode.h
文件实现功能 : 实现图片的显示。在此类中，构造一个平板，然后将图片又纹理贴到此平板上;
作者         : 软件部
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2015/07/15   1.0      朱旭波              新增，基于planarsn改
</PRE>
*******************************************************************************/
#include "stdafx.h"
#include "CDomSymSceneNode.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"
#include "..\..\hd3DEngine\os.h"
#include "..\..\hd3DScene\hd3dview.h"
#include "..\hdFramework\hdCommandDef.h"
#include "HdRaterCache.h"
#include "OverViewDataManger.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	namespace scene
	{
		//! constructor
		CDomSymbSceneNode::CDomSymbSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
					const core::vector3df& position, f32 radius)
			: IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id)
		{
			m_strDomPath = "";
			centerPos = position;
			Radius = radius;

			Buffer = new SMeshBuffer();
			Buffer->Material.Lighting = false;
			Buffer->Material.ZBuffer = video::ECFN_NEVER;
			Buffer->Material.ZWriteEnable = false;
			Buffer->Material.AntiAliasing = video::EAAM_OFF;
			Buffer->BoundingBox.MaxEdge.set(0,0,0);
			Buffer->BoundingBox.MinEdge.set(0,0,0);

			// 记录构造时显示的位置（显示坐标系）
			m_fCentX = position.X;
			m_fCentY = position.Y;
			m_fCentZ = position.Z;

			Buffer->Material.GouraudShading = false;
		}

		CDomSymbSceneNode::~CDomSymbSceneNode()
		{
			if (Buffer)
			{
				getSceneManager()->getVideoDriver()->removeTexture(getTexture());
				getSceneManager()->getVideoDriver()->removeHardwareBuffer(Buffer);
				Buffer->drop();
				Buffer = NULL;
			}

            if (m_pHdData)
            {
                delete m_pHdData;
                m_pHdData = NULL;
            }
		}


		//! pre render event
		void CDomSymbSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
				SceneManager->registerNodeForRendering(this, ESNRP_SKY_BOX);

			ISceneNode::OnRegisterSceneNode();
		}


		//! render
		void CDomSymbSceneNode::render()
		{
            //ReloadData();

            // View
            CHd3DView* p3DView = dynamic_cast<CHd3DView*>(m_pView);
            // 判断是否含有金字塔数据
            if (p3DView->GetOverViewDataManager()->IsOverView())
            {
                // 更新包围盒
                p3DView->UpdateBoxByView();

                // 获得金字塔数据
                vector<CHdRasterBufferPtr> vectRasters;
                Chd2DBoundingBoxd showBox = p3DView->GetBoundingBox();

                {
                    p3DView->GetOverViewDataManager()->GetRasters(showBox, p3DView->GetCurPecent(), vectRasters);
                }

                // 将数据更新
                if (!vectRasters.empty())
                {
                    // 遍历添加图像
                    int nSize = vectRasters.size();
                    for (int i = 0; i < nSize; i++)
                    {
                        // 获得更新后的金字塔数据，用于更新sn的纹理信息
                        CHdRasterBufferPtr pBuffer =  vectRasters.at(i);
                        CHdGeoRaster* pRaster = pBuffer->GetRater();
                        if (pRaster && pRaster->m_pBuffer)
                        {
                            p3DView->AddImagePic(pRaster, pBuffer->GetStrImagePath().c_str());
                        }
                    }
                }
            }

			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

			if (!camera || !driver || !m_pView || !IsVisible)
			{		
				return;
			}

			if (!m_pView->IsViewRenderAllNode())
			{
				return;
			}

			//if (camera->getPosition().X)
			{
				//core::matrix4 mat(AbsoluteTransformation);
				//mat.setTranslation(core::vector3df(0.0f, 0.0f, 0.0f));
				driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);

				driver->setMaterial(Buffer->Material);
				driver->drawMeshBuffer(Buffer);
			}
		}


		//! returns the axis aligned bounding box of this node
		const core::aabbox3d<f32>& CDomSymbSceneNode::getBoundingBox() const
		{
			return Buffer->BoundingBox;
		}


		//! sets the size of the plane
		void CDomSymbSceneNode::setSize(const core::dimension2d<f32>& size)
		{
			Size = size;

			if (Size.Width == 0.0f)
				Size.Width = 1.0f;

			if (Size.Height == 0.0f )
				Size.Height = 1.0f;

			f32 halfW = Size.Width/2;
			f32 halfH = Size.Height/2;

			// 更新包围盒
			Buffer->BoundingBox.MinEdge.set(centerPos.X - halfW, centerPos.Y - halfH, centerPos.Z - 0.01f);
			Buffer->BoundingBox.MaxEdge.set(centerPos.X + halfW ,centerPos.Y + halfH, centerPos.Z + 0.01f);

			// 重新设置节点
			Buffer->Vertices.clear();
			Buffer->Indices.clear();
			Buffer->Vertices.reallocate(4);
			Buffer->Indices.reallocate(6);

			// 节点坐标、纹理坐标更新
			video::S3DVertex vtx;
			vtx.Color.set(255,255,255,255);
			vtx.Normal.set(0.0f,0.0f,1.0f);

			// 角点1
			vtx.Pos.set(centerPos.X - halfW, centerPos.Y - halfH, centerPos.Z);
			vtx.TCoords.set(0.0f, 1.0f);
			Buffer->Vertices.push_back(vtx);

			// 角点2
			vtx.Pos.set(centerPos.X - halfW,centerPos.Y + halfH, centerPos.Z);
			vtx.TCoords.set(0.0f, 0.0f);
			Buffer->Vertices.push_back(vtx);

			// 角点3
			vtx.Pos.set(centerPos.X + halfW, centerPos.Y + halfH, centerPos.Z);
			vtx.TCoords.set(1.0f, 0.0f);
			Buffer->Vertices.push_back(vtx);

			// 角点4
			vtx.Pos.set(centerPos.X + halfW, centerPos.Y - halfH, centerPos.Z);
			vtx.TCoords.set(1.0f, 1.0f);
			Buffer->Vertices.push_back(vtx);

			// 角点更新至buf
			Buffer->Indices.push_back(0);
			Buffer->Indices.push_back(1);
			Buffer->Indices.push_back(2);

			Buffer->Indices.push_back(0);
			Buffer->Indices.push_back(2);
			Buffer->Indices.push_back(3);
		}


		video::SMaterial& CDomSymbSceneNode::getMaterial(u32 i)
		{
			return Buffer->Material;
		}


		//! returns amount of materials used by this scene node.
		u32 CDomSymbSceneNode::getMaterialCount() const
		{
			return 1;
		}


		//! gets the size of the plane
		const core::dimension2d<f32>& CDomSymbSceneNode::getSize() const
		{
			return Size;
		}


		//! Writes attributes of the scene node.
		void CDomSymbSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
		{
			ISceneNode::serializeAttributes(out, options);

			out->addFloat("Width", Size.Width);
			out->addFloat("Height", Size.Height);
		}


		//! Reads attributes of the scene node.
		void CDomSymbSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
		{
			ISceneNode::deserializeAttributes(in, options);

			Size.Width = in->getAttributeAsFloat("Width");
			Size.Height = in->getAttributeAsFloat("Height");

			setSize(Size);
		}


		//! Set the color of all vertices of the plane
		//! \param overallColor: the color to set
		void CDomSymbSceneNode::setColor(const video::SColor & overallColor)
		{
		}


		//! Set the color of the top and bottom vertices of the plane
		//! \param topColor: the color to set the top vertices
		//! \param bottomColor: the color to set the bottom vertices
		void CDomSymbSceneNode::setColor(const video::SColor & topColor, const video::SColor & bottomColor)
		{
		}


		//! Gets the color of the top and bottom vertices of the plane
		//! \param[out] topColor: stores the color of the top vertices
		//! \param[out] bottomColor: stores the color of the bottom vertices
		void CDomSymbSceneNode::getColor(video::SColor & topColor, video::SColor & bottomColor) const
		{
		}


		//! Creates a clone of this scene node and its children.
		ISceneNode* CDomSymbSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
		{
			if (!newParent)
				newParent = Parent;
			if (!newManager)
				newManager = SceneManager;

			CDomSymbSceneNode* nb = new CDomSymbSceneNode(newParent, newManager, ID, RelativeTranslation,
									Radius);

			nb->cloneMembers(this, newManager);

			if ( newParent )
				nb->drop();
			return nb;
		}

		//! 根据屏幕上指定的点与相机位置，构造直线，计算此直线与平面相交点，换算成纹理图像的行、列坐标后返回;
		void CDomSymbSceneNode::getImageScaleByScreenPos(core::position2di pos, core::position2df& imgScale)
		{
			core::line3df line = SceneManager->getSceneCollisionManager()->getRayFromScreenCoordinates(pos,NULL);
			core::plane3df plane(centerPos, core::vector3df(0.0f,-1.0f,0.0f));

			core::vector3df intersection;
			if (plane.getIntersectionWithLine(line.start, line.getVector(), intersection))
			{
				imgScale.X = (intersection.X - Buffer->Vertices[1].Pos.X)/Size.Width;
				imgScale.Y = -(intersection.Z - Buffer->Vertices[1].Pos.Z)/Size.Height;
		   	}
		}

		//! 像素坐标得到屏幕坐标,gsl-2012/7/17
		void CDomSymbSceneNode::getScreenPosByImageScale(core::position2df imgScale, core::position2di& srcpos)
		{
			//f32 xScale = 0.5 - imgPt.X / (f32)m_textureSize.Width;
			//f32 yScale = 0.5 - imgPt.Y / (f32)m_textureSize.Height;

			core::vector3df planePos;
			planePos.X = (f32)(Size.Width * (imgScale.X - 0.5));//xScale;
			planePos.Z = (f32)(Size.Height * (0.5 - imgScale.Y));//yScale;
			planePos.Y = centerPos.Y;

			srcpos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(planePos);
		}

		void CDomSymbSceneNode::getPlanePosByScreenPos( core::position2di pos, core::vector3df& planePos )
		{
			core::line3df line = SceneManager->getSceneCollisionManager()->getRayFromScreenCoordinates(pos,NULL);
			core::plane3df plane(centerPos, core::vector3df(0.0f,-1.0f,0.0f));

			core::vector3df intersection;
			plane.getIntersectionWithLine(line.start, line.getVector(), planePos);
		}

		ITexture* CDomSymbSceneNode::getTexture()
		{
			if (Buffer)
			{
				return Buffer->Material.getTexture(0);		
			}
			return NULL;
		}

		// 获得dom纹理文件路径
		std::string CDomSymbSceneNode::GetDomTexturePah()
		{
			return m_strDomPath;
		}

		// 设置dom纹理文件路径
		void CDomSymbSceneNode::SetDomTexturePath( std::string strPath )
		{
			m_strDomPath = strPath;
		}

		// 更新纹理
		void CDomSymbSceneNode::UpdateTexture( ITexture* pTexture ,Chd2DBoundingBoxd box)
		{
			// 传入二维包围盒为dom的实际坐标（绝对坐标），需转换显示坐标来更新meshbuffer
			m_BoundingBox = box;
			irr::core::vector3df minPos,maxPos;
			CBursaWolfModel* pViewModel = m_pView->GetTransModel();
			double fTmpX,fTmpY,fTmpZ;
			fTmpX = m_BoundingBox.GetMinX();
			fTmpY = m_BoundingBox.GetMinY();
			fTmpZ = 0.0f/* - pViewModel->m_fOffset[2]*/;
			pViewModel->Translate(fTmpX,fTmpY,fTmpZ);
			minPos.set(fTmpX,fTmpY,fTmpZ);

			// 最大值
			fTmpX = m_BoundingBox.GetMaxX();
			fTmpY = m_BoundingBox.GetMaxY();
			fTmpZ = 0.0f/* - pViewModel->m_fOffset[2]*/;
			pViewModel->Translate(fTmpX,fTmpY,fTmpZ);
			maxPos.set(fTmpX,fTmpY,fTmpZ);

			// 更新中心坐标位置
			centerPos.X = (minPos.X + maxPos.X) * 0.5f;
			centerPos.Y = (minPos.Y + maxPos.Y) * 0.5f;
			centerPos.Z = 0.0f/* - pViewModel->m_fOffset[2]*/; // 应以点云最低点作为纹理高程值

			getSceneManager()->getVideoDriver()->removeTexture(getTexture());

			// 更新设置纹理并更新设置
			Buffer->Material.setTexture(0, pTexture);
			const core::dimension2d<u32>& size = pTexture->getSize();
			irr::core::dimension2d<f32> fSize;

			// 根据半径判断是否是缩略
			//if (Radius == 2.0f)
			//{
			//	fSize.Height = 2.0f;
			//	fSize.Width = 2.0f;

			//	centerPos.X = m_fCentX;
			//	centerPos.Y = m_fCentY;
			//	centerPos.Z = m_fCentZ;
			//}
			//else
			{
				fSize.Height = maxPos.Y - minPos.Y;
				fSize.Width = maxPos.X - minPos.X;
			}

			setSize(fSize);
		}

		// 设置半径
		void CDomSymbSceneNode::SetRadius( f32 radius )
		{
			Radius = radius;
		}

		//// 设置缩略XY坐标
		//void CDomSymbSceneNode::SetCentXY( double dx,double dy,double dz )
		//{
		//	m_fCentX = dx;
		//	m_fCentY = dy;
		//	m_fCentZ = dz;
		//}

        //! 根据视口重新加载数据
        BOOL CDomSymbSceneNode::ReloadData()
        {
            getSceneManager()->getActiveCamera();

            // 判断是否合法 !m_pView->IsViewRenderAllNode()||
            if(m_pView == NULL || (!IsVisible&&!m_pView->GetCalReLoad()))
            {
                return FALSE;
            }

            bool quickcam = false;

            // view
            ISceneView* pScneView = dynamic_cast<ISceneView*>(m_pView);
            CHd3DView* p3DView = dynamic_cast<CHd3DView*>(m_pView);

            if (pScneView && pScneView->getCameraTool() && pScneView->getCameraTool()->GetID
                () == COMMAND_QUICK_CAMERA)
            {
                quickcam = true;
            }
            else
            {
                quickcam = false;
            }


            // 锁定代码块
            // EnterCriticalSection(&m_cs);

            // 读取金字塔数据
            // 判断是否含有金字塔数据
            if (!p3DView->GetOverViewDataManager()->IsOverView())
            {
                return false;
            }

            // 更新包围盒
            p3DView->UpdateBoxByView();

            // 获得金字塔数据
            vector<CHdRasterBufferPtr> vectRasters;
            Chd2DBoundingBoxd showBox = p3DView->GetBoundingBox();

            //if (isAddDom)
            //{
            //    m_OverViewDataManger->GetRasters(GetCurPecent(),vectRasters);
            //}
            //else
            {
                p3DView->GetOverViewDataManager()->GetRasters(showBox, p3DView->GetCurPecent(), vectRasters);
            }

            // 将数据更新
            if (vectRasters.empty())
            {
                return false;
            }
            else
            {
                // 遍历添加图像
                int nSize = vectRasters.size();
                for (int i=0;i<nSize;i++)
                {
                    // 获得更新后的金字塔数据，用于更新sn的纹理信息
                    CHdRasterBufferPtr pBuffer =  vectRasters.at(i);
                    CHdGeoRaster* pRaster = pBuffer->GetRater();
                    if (pRaster && pRaster->m_pBuffer)
                    {
                        p3DView->AddImagePic(pRaster, pBuffer->GetStrImagePath().c_str());
                    }
                }
            }

            return TRUE;
        }

	} // end namespace scene
} // end namespace irr


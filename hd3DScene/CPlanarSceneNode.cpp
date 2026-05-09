/*! @file
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : CPlanarSceneNode.cpp
相关文件     : CPlanarSceneNode.h, IPlanarSceneNode.h
文件实现功能 : 实现图片的显示。在此类中，构造一个平板，然后将图片又纹理贴到此平板上;
作者         : 软件部，姚立
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/05/21   1.0      姚立                新增加内容
</PRE>
*******************************************************************************/
#include "stdafx.h"
#include "CPlanarSceneNode.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"
#include "..\hd3DEngine\os.h"
#include "..\hd3DScene\hd3dview.h"
#include "gdal_priv.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	namespace scene
	{
		//! constructor
		CPlanarSceneNode::CPlanarSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
					const core::vector3df& position, const core::dimension2d<f32>& size, video::ITexture* text)
			: IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id)
		{
			#ifdef _DEBUG
			setDebugName("CPlanarSceneNode");
			#endif

			centerPos = position;

			Buffer = new SMeshBuffer();
			Buffer->Material.Lighting = false;
			Buffer->Material.ZBuffer = video::ECFN_NEVER;
			Buffer->Material.ZWriteEnable = false;
			Buffer->Material.AntiAliasing = video::EAAM_OFF;
			Buffer->Material.setTexture(0, text);
			Buffer->BoundingBox.MaxEdge.set(0,0,0);
			Buffer->BoundingBox.MinEdge.set(0,0,0);
	
			setSize(size);
		}

		CPlanarSceneNode::CPlanarSceneNode(ISceneNode* parent,ISceneManager* mgr,s32 id,
			const core::vector3df& position, const io::path &filename)
			: IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id)
		{
		#ifdef _DEBUG
			setDebugName("CPlanarSceneNode");
		#endif
		
			centerPos = position;
		
			ITexture* pTexture = CreateTexture(filename.c_str());
		
			Buffer = new SMeshBuffer();
			Buffer->Material.Lighting = false;
			Buffer->Material.ZBuffer = video::ECFN_NEVER;
			Buffer->Material.ZWriteEnable = false;
			Buffer->Material.AntiAliasing = video::EAAM_OFF;
			Buffer->Material.setTexture(0,pTexture);
			Buffer->BoundingBox.MaxEdge.set(0,0,0);
			Buffer->BoundingBox.MinEdge.set(0,0,0);
		
			//setSize(size);
		}

		CPlanarSceneNode::~CPlanarSceneNode()
		{
			if (Buffer)
			{
				getSceneManager()->getVideoDriver()->removeTexture(getTexture());
				getSceneManager()->getVideoDriver()->removeHardwareBuffer(Buffer);
				Buffer->drop();
				Buffer = NULL;
			}
		}

		inline core::dimension2d<f32> CPlanarSceneNode::getTextureSize()
		{
			core::dimension2d<f32> textSize(0, 0);

			if (Buffer->Material.getTexture(0))
			{
				textSize = Buffer->Material.getTexture(0)->getSize();
			}
			return textSize;
		}

		//! pre render event
		void CPlanarSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
				SceneManager->registerNodeForRendering(this, ESNRP_SKY_BOX);

			ISceneNode::OnRegisterSceneNode();
		}


		//! render
		void CPlanarSceneNode::render()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

			if (!camera || !driver || !m_pView)
			{		
				return;
			}

			if ( !camera->isOrthogonal() )
			{
				//core::matrix4 mat(AbsoluteTransformation);
				//mat.setTranslation(core::vector3df(0.0f, 0.0f, 0.0f));
				driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);

				driver->setMaterial(Buffer->Material);
				driver->drawMeshBuffer(Buffer);
			}	
		}


		//! returns the axis aligned bounding box of this node
		const core::aabbox3d<f32>& CPlanarSceneNode::getBoundingBox() const
		{
			return Buffer->BoundingBox;
		}


		//! sets the size of the plane
		void CPlanarSceneNode::setSize(const core::dimension2d<f32>& size)
		{
			Size = size;

			if (Size.Width == 0.0f)
				Size.Width = 1.0f;

			if (Size.Height == 0.0f )
				Size.Height = 1.0f;

			f32 halfW = Size.Width/2;
			f32 halfH = Size.Height/2;

			Buffer->BoundingBox.MinEdge.set(-halfW, centerPos.Y - 0.01f, -halfH);
			Buffer->BoundingBox.MaxEdge.set(halfW, centerPos.Y + 0.01f ,halfH);

			Buffer->Vertices.clear();
			Buffer->Indices.clear();
			Buffer->Vertices.reallocate(4);
			Buffer->Indices.reallocate(6);

			video::S3DVertex vtx;
			vtx.Color.set(255,255,255,255);
			vtx.Normal.set(0.0f,-1.0f,0.0f);

			vtx.Pos.set(-halfW, centerPos.Y, -halfH);
			vtx.TCoords.set(0.0f, 1.0f);
			Buffer->Vertices.push_back(vtx);

			vtx.Pos.set(-halfW, centerPos.Y, halfH);
			vtx.TCoords.set(0.0f, 0.0f);
			Buffer->Vertices.push_back(vtx);

			vtx.Pos.set(halfW, centerPos.Y, halfH);
			vtx.TCoords.set(1.0f, 0.0f);
			Buffer->Vertices.push_back(vtx);

			vtx.Pos.set(halfW, centerPos.Y, -halfH);
			vtx.TCoords.set(1.0f, 1.0f);
			Buffer->Vertices.push_back(vtx);

			Buffer->Indices.push_back(0);
			Buffer->Indices.push_back(1);
			Buffer->Indices.push_back(2);

			Buffer->Indices.push_back(0);
			Buffer->Indices.push_back(2);
			Buffer->Indices.push_back(3);
		}


		video::SMaterial& CPlanarSceneNode::getMaterial(u32 i)
		{
			return Buffer->Material;
		}


		//! returns amount of materials used by this scene node.
		u32 CPlanarSceneNode::getMaterialCount() const
		{
			return 1;
		}


		//! gets the size of the plane
		const core::dimension2d<f32>& CPlanarSceneNode::getSize() const
		{
			return Size;
		}


		//! Writes attributes of the scene node.
		void CPlanarSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
		{
			ISceneNode::serializeAttributes(out, options);

			out->addFloat("Width", Size.Width);
			out->addFloat("Height", Size.Height);
		}


		//! Reads attributes of the scene node.
		void CPlanarSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
		{
			ISceneNode::deserializeAttributes(in, options);

			Size.Width = in->getAttributeAsFloat("Width");
			Size.Height = in->getAttributeAsFloat("Height");

			setSize(Size);
		}


		//! Set the color of all vertices of the plane
		//! \param overallColor: the color to set
		void CPlanarSceneNode::setColor(const video::SColor & overallColor)
		{
		}


		//! Set the color of the top and bottom vertices of the plane
		//! \param topColor: the color to set the top vertices
		//! \param bottomColor: the color to set the bottom vertices
		void CPlanarSceneNode::setColor(const video::SColor & topColor, const video::SColor & bottomColor)
		{
		}


		//! Gets the color of the top and bottom vertices of the plane
		//! \param[out] topColor: stores the color of the top vertices
		//! \param[out] bottomColor: stores the color of the bottom vertices
		void CPlanarSceneNode::getColor(video::SColor & topColor, video::SColor & bottomColor) const
		{
		}


		//! Creates a clone of this scene node and its children.
		ISceneNode* CPlanarSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
		{
			if (!newParent)
				newParent = Parent;
			if (!newManager)
				newManager = SceneManager;

			CPlanarSceneNode* nb = new CPlanarSceneNode(newParent, newManager, ID, RelativeTranslation,
									Size,Buffer->Material.TextureLayer[0].Texture);

			nb->cloneMembers(this, newManager);

			if ( newParent )
				nb->drop();
			return nb;
		}

		//! 根据屏幕上指定的点与相机位置，构造直线，计算此直线与平面相交点，换算成纹理图像的行、列坐标后返回;
		void CPlanarSceneNode::getImageScaleByScreenPos(core::position2di pos, core::position2df& imgScale)
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
		void CPlanarSceneNode::getScreenPosByImageScale(core::position2df imgScale, core::position2di& srcpos)
		{
			//f32 xScale = 0.5 - imgPt.X / (f32)m_textureSize.Width;
			//f32 yScale = 0.5 - imgPt.Y / (f32)m_textureSize.Height;

			core::vector3df planePos;
			planePos.X = (f32)(Size.Width * (imgScale.X - 0.5));//xScale;
			planePos.Z = (f32)(Size.Height * (0.5 - imgScale.Y));//yScale;
			planePos.Y = centerPos.Y;

			srcpos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(planePos);
		}

		void CPlanarSceneNode::getPlanePosByScreenPos( core::position2di pos, core::vector3df& planePos )
		{
			core::line3df line = SceneManager->getSceneCollisionManager()->getRayFromScreenCoordinates(pos,NULL);
			core::plane3df plane(centerPos, core::vector3df(0.0f,-1.0f,0.0f));

			core::vector3df intersection;
			plane.getIntersectionWithLine(line.start, line.getVector(), planePos);
		}

		ITexture* CPlanarSceneNode::getTexture()
		{
			if (Buffer)
			{
				return Buffer->Material.getTexture(0);		
			}
			return NULL;
		}

		ITexture* CPlanarSceneNode::CreateTexture(const char* pcFileName)
		{
			// 屏蔽GDAL缓存
			int nCacheSize = GDALGetCacheMax();
			GDALSetCacheMax(0);

			// 打开图片
			GDALDataset* pDataset = (GDALDataset*)GDALOpen(pcFileName, GA_ReadOnly);
			if (!pDataset)
			{
				return NULL;
			}

			// 获取图片大小
			int nImageWidth = pDataset->GetRasterXSize();
			int nImageHeight = pDataset->GetRasterYSize();

			// 创建一个纹理
			IVideoDriver* driver = getSceneManager()->getVideoDriver();

			unsigned int nWidth = (nImageWidth > 4096) ? 4096 : nImageWidth;
			unsigned int nHeight = (nImageHeight > 2048) ? 2048 : nImageHeight;
			ITexture* pTexture = driver->addTexture(irr::core::dimension2du(nWidth, nHeight), 
				irr::io::path(pcFileName), irr::video::ECF_A8R8G8B8);

			if (pTexture)
			{
				// 获取纹理实际大小
				const irr::core::dimension2du& textureSize = pTexture->getSize();
				unsigned int nTextureWidth = textureSize.Width;
				unsigned int nTextureHeight = textureSize.Height;
				int nPixelBytes = 4;

				// 解压图片数据到纹理中
				unsigned char* pTextureData = (unsigned char*)pTexture->lock(false);
				memset(pTextureData, 255, nPixelBytes * nTextureWidth * nTextureHeight);

				int nBandCount = 3;
				int pBandIndex[3] = {3, 2, 1};  // 依次为 R, G, B
				int nPixelSpace = nPixelBytes;
				int nLineSpace = nPixelBytes * nTextureWidth;
				int nBandSpace = 1;
				pDataset->RasterIO(GF_Read, 0, 0, nImageWidth, nImageHeight, pTextureData, nTextureWidth, nTextureHeight,
					GDT_Byte, nBandCount, pBandIndex, nPixelSpace, nLineSpace, nBandSpace);

				pTexture->unlock();

				// 记录图片的实际大小
				m_imageSize.Width = nImageWidth;
				m_imageSize.Height = nImageHeight;
			}

			GDALClose(pDataset);

			GDALSetCacheMax(nCacheSize);

			return pTexture;
		}

	} // end namespace scene
} // end namespace irr


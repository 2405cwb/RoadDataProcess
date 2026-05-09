/*! @file
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : CPanoSceneNode.h
相关文件     : CPanoSceneNode.cpp, IPanoSceneNode.h
文件实现功能 : 实现全景视图的显示，根据角度范围，构造一个球体，将全景图片贴到这个球体内表面;
作者         : 软件部，姚立
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/05/21   1.0      姚立                新增加内容
2015/06/11   1.1      朱旭波			  针对地面扫描仪区域扫描得到的点云快速视图
										  显示不正确，更改全景mesh构建方式：由于点
										  云计算水平、垂直角时Y方向取负，因此通过角
										  度反算全景mesh.pos位置时Y应取负，对应纹理
										  坐标水平方向不应反转，碰撞全景sn检测计算水
										  平、垂直角时也应更改Y为负。panosn对应计算接
										  口均已修改，相关软件应测试验证。
</PRE>
*******************************************************************************/
#include "stdafx.h"
#include "CPanoSceneNode.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"
#include "CTriangleSelector.h"
#include "SMesh.h"
#include "IAnimatedMesh.h"
#include <os.h>
#include <math.h>
#include <time.h>
#include "COpenGLExtensionHandler.h"

#include "gdal_priv.h"
#include "gdal.h"

#include "..\hd3DEngine\CImage.h"
using namespace irr;

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	namespace scene
	{

	/* horiRes and vertRes:
		Controls the number of faces along the horizontal axis (30 is a good value)
		and the number of faces along the vertical axis (8 is a good value).

		texturePercentage:
		Only the top texturePercentage of the image is used, e.g. 0.8 uses the top 80% of the image,
		1.0 uses the entire image. This is useful as some landscape images have a small banner
		at the bottom that you don't want.

		spherePercentage:
		This controls how far around the sphere the sky dome goes. For value 1.0 you get exactly the upper
		hemisphere, for 1.1 you get slightly more, and for 2.0 you get a full sphere. It is sometimes useful
		to use a value slightly bigger than 1 to avoid a gap between some ground place and the sky. This
		parameters stretches the image to fit the chosen "sphere-size". */

		CPanoSceneNode::CPanoSceneNode(video::ITexture* texture,
			f32 horiStart, f32 horiEnd, f32 vertStart, f32 vertEnd, 
			f32 radius,	ISceneNode* parent, ISceneManager* mgr, s32 id)
		: IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id), Buffer(0), Radius(radius)
		{
			#ifdef _DEBUG
			setDebugName("CPanoSceneNode");
			#endif

			if (horiStart > horiEnd)
			{
				//转换成弧度;
				HoriStartAngle = core::PI * horiEnd/180.0f;
				HoriEndAngle = core::PI * HoriStartAngle/180.0f;
				HorizontalResolution = (u32)floor((horiStart - horiEnd)/5.0f);
			}
			else
			{
				//转换成弧度;
				HoriStartAngle = core::PI * horiStart/180.0f;
				HoriEndAngle = core::PI * horiEnd/180.0f;
				HorizontalResolution = (u32)floor((horiEnd - horiStart)/5.0f);
			}

			if (vertStart < vertEnd)
			{
				VertStartAngle = core::PI * vertEnd/180.0f;
				VertEndAngle = core::PI * vertStart/180.0f;
				VerticalResolution = (u32)(floor((vertEnd - vertStart)/5.0f));
			}
			else
			{
				VertStartAngle = core::PI * vertStart/180.0f;
				VertEndAngle = core::PI * vertEnd/180.0f;
				VerticalResolution = (u32)(floor((vertStart - vertEnd)/5.0f));
			}

			if (VerticalResolution == 0)
			{
				VerticalResolution = 1;
			}
			if (HorizontalResolution == 0)
			{
				HorizontalResolution = 1;
			}

			// new一个buffer
            m_bTile = false;
			Buffer = new SMeshBuffer();
			Buffer->Material.Lighting = false;
			Buffer->Material.ZBuffer = video::ECFN_NEVER; 
			Buffer->Material.ZWriteEnable = false;
			Buffer->Material.AntiAliasing = video::EAAM_OFF;
			Buffer->Material.setTexture(0, texture);
			Buffer->BoundingBox.MaxEdge.set(radius,radius,radius);
			Buffer->BoundingBox.MinEdge.set(-radius,-radius,-radius);

            m_pPanoTileHelper = NULL;

			
			// new一个Buffer_Zero
			Buffer_Zero = new SMeshBuffer();
			Buffer_Zero->Material.Lighting = false;
			Buffer_Zero->Material.ZBuffer = video::ECFN_NEVER; 
			Buffer_Zero->Material.ZWriteEnable = false;
			Buffer_Zero->Material.AntiAliasing = video::EAAM_OFF;
			Buffer_Zero->Material.setTexture(0, texture);
			Buffer_Zero->BoundingBox.MaxEdge.set(radius,radius,radius);
			Buffer_Zero->BoundingBox.MinEdge.set(-radius,-radius,-radius);

			//setAutomaticCulling(scene::EAC_OFF);
			if (texture)
			{
				// regenerate the mesh
				GenerateMesh();
			}

			IsRenderLikeSkyBox = true;
			//TriangleSelector = new CTriangleSelector(Buffer,this);	
		}

		CPanoSceneNode::CPanoSceneNode(f32 horiStart, f32 horiEnd, 
			f32 vertStart, f32 vertEnd, f32 radius, bool bIsTile, ISceneNode* parent, ISceneManager* mgr, s32 id)
			: IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id), Buffer(0), Radius(radius)
		{
			Buffer = NULL;
			Buffer_Zero = NULL;
			m_pPanoTileHelper = NULL;
			m_bTile = bIsTile;

			if (horiStart > horiEnd)
			{
				//转换成弧度;
				HoriStartAngle = core::PI * horiEnd/180.0f;
				HoriEndAngle = core::PI * HoriStartAngle/180.0f;
				HorizontalResolution = (u32)floor((horiStart - horiEnd)/5.0f);
			}
			else
			{
				//转换成弧度;
				HoriStartAngle = core::PI * horiStart/180.0f;
				HoriEndAngle = core::PI * horiEnd/180.0f;
				HorizontalResolution = (u32)floor((horiEnd - horiStart)/5.0f);
			}

			if (vertStart < vertEnd)
			{
				VertStartAngle = core::PI * vertEnd/180.0f;
				VertEndAngle = core::PI * vertStart/180.0f;
				VerticalResolution = (u32)(floor((vertEnd - vertStart)/5.0f));
			}
			else
			{
				VertStartAngle = core::PI * vertStart/180.0f;
				VertEndAngle = core::PI * vertEnd/180.0f;
				VerticalResolution = (u32)(floor((vertStart - vertEnd)/5.0f));
			}

			if (VerticalResolution == 0)
			{
				VerticalResolution = 1;
			}
			if (HorizontalResolution == 0)
			{
				HorizontalResolution = 1;
			}

			if (!m_bTile)
			{
				Buffer = new SMeshBuffer();
				Buffer->Material.Lighting = false;
				Buffer->Material.ZBuffer = video::ECFN_NEVER; 
				Buffer->Material.ZWriteEnable = false;
				Buffer->Material.AntiAliasing = video::EAAM_OFF;
				Buffer->BoundingBox.MaxEdge.set(radius,radius,radius);
				Buffer->BoundingBox.MinEdge.set(-radius,-radius,-radius);

				GenerateMesh();
			}
			else
			{
				Buffer_Zero = new SMeshBuffer();
				Buffer_Zero->Material.Lighting = false;
				Buffer_Zero->Material.ZBuffer = video::ECFN_NEVER; 
				Buffer_Zero->Material.ZWriteEnable = false;
				Buffer_Zero->Material.AntiAliasing = video::EAAM_OFF;
				Buffer_Zero->BoundingBox.MaxEdge.set(radius,radius,radius);
				Buffer_Zero->BoundingBox.MinEdge.set(-radius,-radius,-radius);
			}

			IsRenderLikeSkyBox = true;
		}

		CPanoSceneNode::~CPanoSceneNode()
		{
			// 主纹理buf
			if (Buffer)
			{
				getSceneManager()->getVideoDriver()->removeTexture(Buffer->Material.getTexture(0));
				getSceneManager()->getVideoDriver()->removeHardwareBuffer(Buffer);
				Buffer->drop();
				Buffer = NULL;
			}

			// 半径为2的切片级纹理buf
			if (Buffer_Zero)
			{
				getSceneManager()->getVideoDriver()->removeTexture(Buffer_Zero->Material.getTexture(0));
				getSceneManager()->getVideoDriver()->removeHardwareBuffer(Buffer_Zero);
				Buffer_Zero->drop();
				Buffer_Zero = NULL;
			}

			// 球体管理器内存释放
			if (m_pPanoTileHelper)
			{
				delete m_pPanoTileHelper;
				m_pPanoTileHelper = NULL;
			}
		}


		void CPanoSceneNode::GenerateMesh()
		{
			f32 azimuth;
			u32 k;

			Buffer->Vertices.clear();
			Buffer->Indices.clear();

			// 设置buffer材质 半径较小时，当前相机为3D camera，设置Zbuffer 打开
			//				  半径较大时，当前相机为quick camera，设置Zbuffer 关闭 [2013/08/05 危迟] 
			//if (Radius == 2.f)
			if (Radius < 3.0)
			{
				Buffer->Material.setFlag(video::EMF_ZBUFFER, true);
			}
			else
			{
				Buffer->Material.setFlag(video::EMF_ZBUFFER, false);
			}

			const f32 azimuth_step = (HoriEndAngle - HoriStartAngle) / HorizontalResolution;
			const f32 elevation_step = (VertStartAngle - VertEndAngle)/ (f32)VerticalResolution;

			Buffer->Vertices.reallocate( (HorizontalResolution + 1) * (VerticalResolution + 1) );
			Buffer->Indices.reallocate(3 * (2*VerticalResolution - 1) * HorizontalResolution);

			video::S3DVertex vtx;
			vtx.Color.set(255,255,255,255);
			vtx.Normal.set(0.0f,-1.f,0.0f);

			const f32 tcV = 1.0f / VerticalResolution;
			for (k = 0, azimuth = HoriStartAngle; k <= HorizontalResolution; ++k)
			{
				f32 elevation = VertStartAngle;
				const f32 tcU = 1.0f - (f32)k / (f32)HorizontalResolution;
				const f32 sinA = sinf(azimuth);
				const f32 cosA = cosf(azimuth);
				for (u32 j = 0; j <= VerticalResolution; ++j)
				{
					const f32 cosEr = Radius * cosf(elevation);
					//vtx.Pos.set(cosEr*sinA, Radius*sinf(elevation), cosEr*cosA);
					vtx.Pos.set(cosEr*cosA, -cosEr*sinA ,Radius*sinf(elevation)); // 由于点云计算角度时Y坐标取负值，此处需要统一坐标系
					vtx.TCoords.set(1 - tcU, j*tcV); // 纹理坐标也做更改

					vtx.Normal = -vtx.Pos;
					vtx.Normal.normalize();

					Buffer->Vertices.push_back(vtx);
					elevation -= elevation_step;
				}
				azimuth += azimuth_step;
			}

			//for (k = 0; k < HorizontalResolution; ++k)
			//{
			//	Buffer->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k);
			//	Buffer->Indices.push_back(0 + (VerticalResolution + 1)*k);
			//	Buffer->Indices.push_back(1 + (VerticalResolution + 1)*k);

			//	for (u32 j = 1; j < VerticalResolution; ++j)
			//	{
			//		Buffer->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k + j);
			//		Buffer->Indices.push_back(0 + (VerticalResolution + 1)*k + j);
			//		Buffer->Indices.push_back(1 + (VerticalResolution + 1)*k + j);

			//		Buffer->Indices.push_back(VerticalResolution + 1 + (VerticalResolution + 1)*k + j);
			//		Buffer->Indices.push_back(0 + (VerticalResolution + 1)*k + j);
			//		Buffer->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k + j);
			//	}
			//}

			for (k = 0; k < HorizontalResolution; ++k)
			{
				// 设置三角索引时，根据坐标系方向，右手系时设置为顺时针方向纹理显示正确
				Buffer->Indices.push_back(1 + (VerticalResolution + 1)*k);
				Buffer->Indices.push_back(0 + (VerticalResolution + 1)*k);
				Buffer->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k);

				for (u32 j = 1; j < VerticalResolution; ++j)
				{
					Buffer->Indices.push_back(1 + (VerticalResolution + 1)*k + j);
					Buffer->Indices.push_back(0 + (VerticalResolution + 1)*k + j);
					Buffer->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k + j);

					Buffer->Indices.push_back(0 + (VerticalResolution + 1)*k + j);
					Buffer->Indices.push_back(VerticalResolution + 1 + (VerticalResolution + 1)*k + j);
					Buffer->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k + j);
				}
			}

			//Buffer->setHardwareMappingHint(EHM_NEVER,EBT_VERTEX_AND_INDEX);
			// 设置硬件映射模式为dynamic [2013/07/31 危迟]
			Buffer->setHardwareMappingHint(irr::scene::EHM_DYNAMIC);
			// 标记buffer的顶点及顶点索引已经改变，更新硬件中的buffer [2013/07/31 危迟] 
			Buffer->setDirty(EBT_VERTEX_AND_INDEX);
			//重新计算boundingBox
			Buffer->recalculateBoundingBox();
		}

		//! renders the node.
		void CPanoSceneNode::render()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();
			
			ITexture* pTexture = GetTexture();
			if (!camera || !driver || !m_pView || pTexture == NULL)
				return;

			if (!m_pView->IsViewRenderAllNode())
			{
				return;
			}

			core::matrix4 mat(AbsoluteTransformation);
			mat.setTranslation(getPosition());//camera->getAbsolutePosition()

			driver->setTransform(video::ETS_WORLD, mat);

			// 根据是切片模式还是完整模式进行渲染
			if (m_bTile)
			{
				// 根据半径选择渲染
				if (Radius == 2.0f)
				{
					if (Buffer_Zero)
					{
						driver->setMaterial(Buffer_Zero->Material);
						driver->drawMeshBuffer(Buffer_Zero);
					}
				}
				else
				{
					if (m_pPanoTileHelper)
					{
						m_pPanoTileHelper->Render();
					}
				}
			}
			else
			{
				if (Buffer)
				{
					driver->setMaterial(Buffer->Material);
					driver->drawMeshBuffer(Buffer);
				}
			}
		}


		//! returns the axis aligned bounding box of this node
		const core::aabbox3d<f32>& CPanoSceneNode::getBoundingBox() const
		{
			// 根据当前sn是切片还是完整分别处理
			if (!m_bTile)
			{
				if (Buffer)
				{
					return Buffer->BoundingBox;
				}
			}
			else 
			{
				if (m_pPanoTileHelper)
				{
					m_pPanoTileHelper->getBoundingBox();
				}
			}

		}

		void CPanoSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
			{
				if (IsRenderLikeSkyBox)
				{
					SceneManager->registerNodeForRendering(this, ESNRP_SKY_BOX);
				}
				else
				{
					SceneManager->registerNodeForRendering(this, ESNRP_AUTOMATIC);
				}
			}

			ISceneNode::OnRegisterSceneNode();
		}


		//! returns the material based on the zero based index i. To get the amount
		//! of materials used by this scene node, use getMaterialCount().
		//! This function is needed for inserting the node into the scene hirachy on a
		//! optimal position for minimizing renderstate changes, but can also be used
		//! to directly modify the material of a scene node.
		video::SMaterial& CPanoSceneNode::getMaterial(u32 i)
		{
			SMeshBuffer* pMeshBuf = Buffer;
			if (pMeshBuf)
			{
				return pMeshBuf->Material;
			}
		}


		//! returns amount of materials used by this scene node.
		u32 CPanoSceneNode::getMaterialCount() const
		{
			return 1;
		}


		//! Writes attributes of the scene node.
		void CPanoSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
		{
			ISceneNode::serializeAttributes(out, options);

			out->addInt  ("HorizontalResolution", HorizontalResolution);
			out->addInt  ("VerticalResolution",   VerticalResolution);
			out->addFloat("Radius",               Radius);
		}


		//! Reads attributes of the scene node.
		void CPanoSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
		{
			HorizontalResolution = in->getAttributeAsInt  ("HorizontalResolution");
			VerticalResolution   = in->getAttributeAsInt  ("VerticalResolution");
			Radius               = in->getAttributeAsFloat("Radius");

			ISceneNode::deserializeAttributes(in, options);

			// regenerate the mesh
			if (!m_bTile)
			{
				GenerateMesh();
			}
		}

		//! Creates a clone of this scene node and its children.
		ISceneNode* CPanoSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
		{
			if (!newParent)
				newParent = Parent;
			if (!newManager)
				newManager = SceneManager;

			SMeshBuffer* pMeshBuf = Buffer;
			CPanoSceneNode* nb = new CPanoSceneNode(pMeshBuf->Material.TextureLayer[0].Texture, 
				HoriStartAngle,HoriEndAngle,VertStartAngle,VertEndAngle, Radius, newParent, newManager, ID);

			nb->cloneMembers(this, newManager);

			if ( newParent )
				nb->drop();
			return nb;
		}

		void CPanoSceneNode::GetSpherePos(const core::vector2df angle, core::vector3df& spherePos)
		{
			f32 cosR = Radius*cosf(angle.Y*core::DEGTORAD);
			spherePos.X = cosR*cosf(angle.X*core::DEGTORAD);
			spherePos.Y = cosR*sinf(angle.X*core::DEGTORAD);
			spherePos.Y = -spherePos.Y;
			spherePos.Z = Radius*sinf(angle.Y*core::DEGTORAD);
		}

		void CPanoSceneNode::GetSpherePosByImageScale( const core::vector2df& imageScale, core::vector3df& spherePos )
		{
			// 此处计算忽略位置和旋转,暂不考虑
			core::vector2df angle;
			angle.X = (/*1 - */imageScale.X) * fabs(HoriEndAngle - HoriStartAngle) + HoriStartAngle;
			angle.Y = VertStartAngle - imageScale.Y * fabs(VertStartAngle - VertEndAngle);

			angle.X *= core::RADTODEG;
			angle.Y *= core::RADTODEG;
			GetSpherePos(angle, spherePos);
		}

		void CPanoSceneNode::GetScreenPosByImageScale( core::position2df imgScale, core::position2di& screenPos )
		{
			// 此处计算忽略位置和旋转,暂不考虑
			core::vector2df angle;
			angle.X = (/*1 - */imgScale.X) * fabs(HoriEndAngle - HoriStartAngle) + HoriStartAngle;
			angle.Y = VertStartAngle - imgScale.Y * fabs(VertStartAngle - VertEndAngle);
	
			angle.X *= core::RADTODEG;
			angle.Y *= core::RADTODEG;
			core::vector3df spherePos;
			GetSpherePos(angle, spherePos);

			screenPos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(spherePos);
		}

		void CPanoSceneNode::GetImageScaleByScreenPos( core::position2di pos, core::position2df& imgScale )
		{
			core::line3df line = SceneManager->getSceneCollisionManager()->getRayFromScreenCoordinates(pos);
			core::vector3df vect = line.getVector();
			vect.normalize();

			core::vector2df angle;
			angle.Y = asin(vect.Z);
			angle.X = atan2(-vect.Y, vect.X);
			if (angle.X < 0)
			{
				angle.X += (2 * core::PI);
			}

			imgScale.X = (f32)(/*1.0 - */(angle.X - HoriStartAngle)/(HoriEndAngle - HoriStartAngle));//core::PI * 2 - 
			imgScale.Y = (VertStartAngle - angle.Y)/(VertStartAngle - VertEndAngle);
			clamp(imgScale.X,0.0f,1.0f);
			clamp(imgScale.Y,0.0f,1.0f);
		}

		//! 获得屏幕上的像素坐标
		void CPanoSceneNode::GetScreenPostion(core::position2di pos, core::position2df& imgScale)
		{
			core::line3df line = SceneManager->getSceneCollisionManager()->getRayFromScreenCoordinates(pos);
			core::vector3df vect = line.getVector();

			core::matrix4 m = ISceneNode::getRelativeTransformation();

			vect.normalize();

			double x = vect.X * m(0,0) + vect.Y * m(0,1) + vect.Z * m(0,2);
			double y = vect.X * m(1,0) + vect.Y * m(1,1) + vect.Z * m(1,2);
			double z = vect.X * m(2,0) + vect.Y * m(2,1) + vect.Z * m(2,2);

			vect.X = x;
			vect.Y = y;
			vect.Z = z;

			core::vector2df angle;
			angle.Y = asin(vect.Z);
			angle.X = atan2(-vect.Y, vect.X);
			if (angle.X < 0)
			{
				angle.X += (2 * core::PI);
			}

			imgScale.X = (f32)(/*1.0 - */(angle.X - HoriStartAngle)/(HoriEndAngle - HoriStartAngle));//core::PI * 2 - 
			imgScale.Y = (VertStartAngle - angle.Y)/(VertStartAngle - VertEndAngle);
			clamp(imgScale.X,0.0f,1.0f);
			clamp(imgScale.Y,0.0f,1.0f);
		}
		ITexture* CPanoSceneNode::GetTexture()
		{
			// 分别进行处理返回
			if (!m_bTile)
			{
				if (Buffer)
				{
					return Buffer->Material.getTexture(0);		
				}
			}
			else
			{
				if (m_pPanoTileHelper)
				{
					return m_pPanoTileHelper->GetZeroPanoTexture();
				}
			}

			return NULL;
		}

		// 仅完整全景调用，非切片接口调用
		void CPanoSceneNode::Update()
		{
			SMeshBuffer* pMeshBuf = Buffer;
			if (pMeshBuf)
			{
				ITexture* pTexture = pMeshBuf->Material.getTexture(0);
				if (pTexture)
				{
					pTexture->update();
				}
				GenerateMesh();
			
			}
		}

		void CPanoSceneNode::UpdateTexture(ITexture* text)
		{
			SMeshBuffer* pMeshBuf = Buffer;
			if (pMeshBuf && text)
			{
				getSceneManager()->getVideoDriver()->removeTexture(GetTexture());
				pMeshBuf->Material.setTexture(0,text);
				GenerateMesh();
			}
		}

		void CPanoSceneNode::UpdateTexture(const CHdPanoData* pPanoData, video::ITexture* panoTextureExt)
		{
			// 为原完整全景
			if (pPanoData && pPanoData->m_panoData->nSize > 0)
			{
				m_bTile = false;
				SMeshBuffer* pMeshBuf = Buffer;
				getSceneManager()->getVideoDriver()->removeTexture(GetTexture());

				// 读取纹理
				irr::io::IReadFile* pReadFile = irr::io::createMemoryReadFile(pPanoData->m_panoData->pImageData,
					pPanoData->m_panoData->nSize,pPanoData->m_panoData->strImageID,false);
				IVideoDriver* driver = SceneManager->getVideoDriver();
				ITexture* pPanoTexture = driver->getTexture(pReadFile);
				pReadFile->drop();
    //! lhq on 2016/11/21 当纹理图片(全景图片)大小超过16M时,通过内存流获取纹理将会失败,
    //! 导致全景图片无法显示,为解决此问题从外部传入纹理
    if (pPanoTexture == NULL && panoTextureExt != NULL)
    {
        pPanoTexture = panoTextureExt;
    }
				if (pPanoTexture)
				{
					// 设置并重构纹理坐标
					pMeshBuf->Material.setTexture(0,pPanoTexture);
					GenerateMesh();
				}
			}
		}

		void CPanoSceneNode::UpdateTextureScene(const CHdPanoData* pPanoData)
		{
			// 分别进行处理
			if (pPanoData) 
			{
				// 记录全景ID
				m_strPanoID = pPanoData->m_panoData->strImageID;
				if ( pPanoData->m_panoData->nSize <= 0)
				{
					m_bTile = true;
					CreateTexture_mogo();
				}
				else
				{
					
					// 屏蔽GDAL缓存
					int nCacheSize = GDALGetCacheMax();
					GDALSetCacheMax(0);

					// 打开图片
					std::string strMemFileName = "/vsimem/" + m_strPanoID;
					VSIFCloseL(VSIFileFromMemBuffer(strMemFileName.data(), pPanoData->m_panoData->pImageData, 
						pPanoData->m_panoData->nSize, 0));
					GDALDataset* pDataset = (GDALDataset*)GDALOpen(strMemFileName.data(), GA_ReadOnly);
					if (pDataset)
					{
						// 获取图片大小
						int nImageWidth = pDataset->GetRasterXSize();
						int nImageHeight = pDataset->GetRasterYSize();

						// 如果不存在纹理，则先创建一个纹理
						IVideoDriver* driver = SceneManager->getVideoDriver();
						ITexture* pTexture = GetTexture();
						if (pTexture == NULL)
						{
							// 纹理大小限制为 4096 * 8192 以内
							unsigned int nWidth = (nImageWidth > 8192) ? 8192 : nImageWidth;
							unsigned int nHeight = (nImageHeight > 4096) ? 4096 : nImageHeight;
							pTexture = driver->addTexture(irr::core::dimension2du(nWidth, nHeight), 
								irr::io::path(m_strPanoID.data()), irr::video::ECF_A8R8G8B8);
						}
						else
						{
							// 先清除纹理缓存
							Buffer->Material.setTexture(0, NULL);
						}

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

							Buffer->Material.setTexture(0, pTexture);
						}
						
						GDALClose(pDataset);
					}

					VSIUnlink(strMemFileName.data());

					GDALSetCacheMax(nCacheSize);
					/*
					// 更新纹理
					m_bTile = false;
					irr::io::IReadFile* pReadFile = irr::io::createMemoryReadFile(pPanoData->m_panoData->pImageData,
						pPanoData->m_panoData->nSize,pPanoData->m_panoData->strImageID,false);
					IVideoDriver* driver = SceneManager->getVideoDriver();
					ITexture* pPanoTexture = driver->getTexture(pReadFile);
					if (pPanoTexture)
					{
						UpdateTexture(pPanoTexture);
					}
					pReadFile->drop();
					*/
				}
			}
		}
	
		void CPanoSceneNode::SetRadius( f32 radius )
		{
			bool bChange = false;
			if (radius != Radius)
			{
				bChange = true;
			}
			Radius = radius;

			// 若为mongo服务器视图
			if (bChange && m_pView && m_bTile)
			{
				CreateTexture_mogo();
			}
			else
			{
				if (!m_bTile)
				{
					//指定大小后 重新生成mesh
					GenerateMesh();
				}
			}
		}

		void CPanoSceneNode::GetImageScaleByScreenPos2( core::position2di pos, core::position2df& imgScale )
		{
			core::line3df line = SceneManager->getSceneCollisionManager()->getRayFromScreenCoordinates(pos);
			core::vector3df vect = line.getVector();

			core::matrix4 m = ISceneNode::getRelativeTransformation();

			vect.normalize();

			double x = vect.X * m(0,0) + vect.Y * m(0,1) + vect.Z * m(0,2);
			double y = vect.X * m(1,0) + vect.Y * m(1,1) + vect.Z * m(1,2);
			double z = vect.X * m(2,0) + vect.Y * m(2,1) + vect.Z * m(2,2);

			vect.X = x;
			vect.Y = y;
			vect.Z = z;

			core::vector2df angle;
			angle.Y = asin(vect.Z);
			angle.X = atan2(-vect.Y, vect.X);
			if (angle.X < 0)
			{
				angle.X += (2 * core::PI);
			}

			imgScale.X = (f32)(/*1.0 - */(angle.X - HoriStartAngle)/(HoriEndAngle - HoriStartAngle));//core::PI * 2 - 
			imgScale.Y = (VertStartAngle - angle.Y)/(VertStartAngle - VertEndAngle);
			clamp(imgScale.X,0.0f,1.0f);
			clamp(imgScale.Y,0.0f,1.0f);
		}

		// 根据指定向量得到像素坐标，imgScale是比例
		void CPanoSceneNode::GetImageScaleByScreenPos2(core::vector3df& norm,core::position2df& imgScale)
		{
			
			core::vector3df vect = norm;

			core::matrix4 m = ISceneNode::getRelativeTransformation();

			vect.normalize();

			double x = vect.X * m(0,0) + vect.Y * m(0,1) + vect.Z * m(0,2);
			double y = vect.X * m(1,0) + vect.Y * m(1,1) + vect.Z * m(1,2);
			double z = vect.X * m(2,0) + vect.Y * m(2,1) + vect.Z * m(2,2);

			vect.X = x;
			vect.Y = y;
			vect.Z = z;

			core::vector2df angle;
			angle.Y = asin(vect.Z);
			angle.X = atan2(-vect.Y, vect.X);
			if (angle.X < 0)
			{
				angle.X += (2 * core::PI);
			}

			imgScale.X = (f32)(/*1.0 - */(angle.X - HoriStartAngle)/(HoriEndAngle - HoriStartAngle));//core::PI * 2 - 
			imgScale.Y = (VertStartAngle - angle.Y)/(VertStartAngle - VertEndAngle);
			clamp(imgScale.X,0.0f,1.0f);
			clamp(imgScale.Y,0.0f,1.0f);
		}

		//! 像素坐标得到屏幕坐标,imgPt是像素比例。
		void CPanoSceneNode::GetScreenPosByImageScale2(core::position2df imgScale, core::position2di& screenPos)
		{
			core::vector3df spherePos;
			Get3DPosByImageScale2(imgScale, spherePos);

			screenPos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(spherePos);
		}

		//! 通过屏幕坐标得到在全景球上的3D显示坐标
		void CPanoSceneNode::Get3DPosByScreenPos2(core::position2di screenPos, core::vector3df& pos)
		{
			pos = SceneManager->getSceneCollisionManager()->getRayFromScreenCoordinates(screenPos).getVector();
			core::matrix4 m = ISceneNode::getRelativeTransformation();

			pos.normalize();

			double x = pos.X * m(0,0) + pos.Y * m(0,1) + pos.Z * m(0,2);
			double y = pos.X * m(1,0) + pos.Y * m(1,1) + pos.Z * m(1,2);
			double z = pos.X * m(2,0) + pos.Y * m(2,1) + pos.Z * m(2,2);

			pos.X = x * Radius + getPosition().X;
			pos.Y = y * Radius + getPosition().Y;
			pos.Z = z * Radius + getPosition().Z;
		}

		//! 通过像素坐标来得到全景球上的3D显示坐标
		void CPanoSceneNode::Get3DPosByImageScale2(core::position2df imgScale, core::vector3df& pos)
		{
			core::vector2df angle;
			angle.X = (/*1 - */imgScale.X) * fabs(HoriEndAngle - HoriStartAngle) + HoriStartAngle;
			angle.Y = VertStartAngle - imgScale.Y * fabs(VertStartAngle - VertEndAngle);

			angle.X *= core::RADTODEG;
			angle.Y *= core::RADTODEG;
			GetSpherePos(angle, pos);

			core::matrix4 m = ISceneNode::getRelativeTransformation();

			double x = pos.X * m(0,0) + pos.Y * m(1,0) + pos.Z * m(2,0);
			double y = pos.X * m(0,1) + pos.Y * m(1,1) + pos.Z * m(2,1);
			double z = pos.X * m(0,2) + pos.Y * m(1,2) + pos.Z * m(2,2);

			pos.X = x + getPosition().X;
			pos.Y = y + getPosition().Y;
			pos.Z = z + getPosition().Z;
		}

		//! 外部调用，设置记录全景ID
		void CPanoSceneNode::SetPanoID( string strPanoID )
		{
			m_strPanoID = strPanoID;

			if (!m_pPanoTileHelper)
			{
				// new一个管理器对象，并初始化球体
				m_pPanoTileHelper = new CPanoTileAidHelper(m_pView);
			}
		}

		//! 外部设置从mogo服务器获取切片数据，创建sn时调用
		void CPanoSceneNode::SetDataFrmMogo()
		{
			if (m_pView && m_bTile)
			{
				CreateTexture_mogo();
			}
		}

		void CPanoSceneNode::CreateTexture_mogo()
		{
			// 存在性判断
			if (!m_pView)
			{
				return;
			}
			ISceneView* pSceneView = (ISceneView*)(m_pView);
			if (!pSceneView)
			{
				return;
			}
			pSceneView->OnloadTitlePanoData();
		}

		// 根据全景ID获取0级切片数据
		CHdSvTileInfoBuffer* CPanoSceneNode::GetZeroPanoTile( const char* strPanoID )
		{
			// 条件判断
			if (!m_pPanoTileHelper)
			{
				return NULL;
			}

			// 调用方法
			return m_pPanoTileHelper->GetZeroPanoTile(strPanoID);
		}

		// 设置0级切片纹理贴图
		void CPanoSceneNode::SetZeroPanoTexture(CHdSvTileInfoBuffer* pZeroInfo)
		{
			// 条件判断
			if (!m_pPanoTileHelper)
			{
				return;
			}

			// 调用方法
			m_pPanoTileHelper->SetZeroPanoTexture(pZeroInfo);
		}

		// 根据传入参数获取数据,在此前，若为定位，则需先调用获取0级数据用于更新设置新球、全景ID等
		void CPanoSceneNode::GetPanoTiles( vector<CHdSvTileInfoBuffer*>& vecTileBuffer )
		{
			// 条件判断
			if (!m_pPanoTileHelper)
			{
				return;
			}

			// 获取pos及相对旋转矩阵,此处意味着外部调用时需先设置pos再进行贴图
			irr::core::vector3df pos = getPosition();

			// 获得相对旋转矩阵
			core::matrix4 m = ISceneNode::getRelativeTransformation();

			// 调用接口,vecTileBuffer仅填充类对象，二进制影像数据由外部多线程获取
			m_pPanoTileHelper->GetPanoTiles(pos,m,vecTileBuffer);
		}

		// 设置0级球纹理
		void CPanoSceneNode::SetZeroTextureRadius( CHdSvTileInfoBuffer* pZeroInfo )
		{
			// 重新设置0级纹理
			if (!pZeroInfo)
			{
				return;
			}

			// 获得之前的纹理，若存在，则移除
			ITexture* pTexture = Buffer_Zero->Material.getTexture(0);
			if (pTexture)
			{
				getSceneManager()->getVideoDriver()->removeTexture(pTexture);
			}

			// 获得切片
			HD_SV_TILEINFO* pInfo = pZeroInfo->GetTileInfo();

			// 切片二进制数据存在则添加纹理
			if (!pInfo || pZeroInfo->GetSize() <= 0)
			{
				return;
			}

			// 视图判断
			if (!m_pView)
			{
				return;
			}

			// 视图获得
			ISceneView* pSceneView = (ISceneView*)(m_pView);
			if (!pSceneView)
			{
				return;
			}

			// 构建纹理数据
			irr::io::IReadFile* pReadFile = irr::io::createMemoryReadFile(pInfo->pTileData,
				pInfo->nSize,pInfo->strTileID,false);
			IVideoDriver* driver = pSceneView->GetSceneManager()->getVideoDriver();
			ITexture* pTileTexture = driver->getTexture(pReadFile);

			// 设置纹理
			if (pTileTexture)
			{
				Buffer_Zero->Material.setTexture(0,pTileTexture);
			}

			// drop
			pReadFile->drop();
		}


	} // namespace scene
} // namespace irr

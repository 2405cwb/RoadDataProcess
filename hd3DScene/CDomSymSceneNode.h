/*! @file
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : CDomSymSceneNode.h
相关文件     : CDomSymSceneNode.cpp
文件实现功能 : 实现dom图片的显示。在此类中，构造一个平板，然后将图片又纹理贴到此平板上;
作者         : 软件部，朱旭波
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2015/07/09   1.0      朱旭波              新增加内容，在planarSceneNode基础改进
2015/12/28   1.1      张阳                从HdApplication移动至Hd3DScene中
</PRE>
*******************************************************************************/
#pragma once
#include "stdafx.h"
#include "..\hd3DEngine\include\irrlicht.h"
#include "..\hd3DEngine\include\SMeshBuffer.h"
#include "..\hd3DEngine\include\ITexture.h"
#include "iobjectscenenode.h"

#include "..\hd3DScene\include\BasicObject\hdRaster.h"
#include "..\hd3DScene\include\BasicObject\BaseRect.h"

using namespace irr;
using namespace irr::video;
using namespace irr::scene;

namespace hd
{
	namespace scene
	{
		//! Scene node which is a plane. A plane is like a 3d sprite: A 2d element,
		//! which always looks to the camera. 
		class HD3DSCENE_API CDomSymbSceneNode : virtual public IObjectSceneNode
		{
		public:

			//! constructor
			CDomSymbSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,	
				const core::vector3df& position, f32 radius = 2.0f);
			CDomSymbSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,const core::vector3df& position,
				const io::path &filename);
			~CDomSymbSceneNode();

			//added by wkl 2012-7-13 17:03:49
			inline core::dimension2d<f32> getTextureSize(){return m_textureSize;};

			//! pre render event
			virtual void OnRegisterSceneNode();

			//! render
			virtual void render();

			//! returns the axis aligned bounding box of this node
			virtual const core::aabbox3d<f32>& getBoundingBox() const;

			//! sets the size of the plane
			virtual void setSize(const core::dimension2d<f32>& size);

			//! gets the size of the plane
			virtual const core::dimension2d<f32>& getSize() const;

			virtual video::SMaterial& getMaterial(u32 i);
	
			//! returns amount of materials used by this scene node.
			virtual u32 getMaterialCount() const;
	
			//! Set the color of all vertices of the plane
			//! \param overallColor: the color to set
			virtual void setColor(const video::SColor & overallColor);

			//! Set the color of the top and bottom vertices of the plane
			//! \param topColor: the color to set the top vertices
			//! \param bottomColor: the color to set the bottom vertices
			virtual void setColor(const video::SColor & topColor, const video::SColor & bottomColor);

			//! Gets the color of the top and bottom vertices of the plane
			//! \param[out] topColor: stores the color of the top vertices
			//! \param[out] bottomColor: stores the color of the bottom vertices
			virtual void getColor(video::SColor& topColor, video::SColor& bottomColor) const;

			//! Writes attributes of the scene node.
			virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const;

			//! Reads attributes of the scene node.
			virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0);

			//! Returns type of the scene node
			virtual ESCENE_NODE_TYPE getType() const { return ESNT_HD_DOM; }

			//! Creates a clone of this scene node and its children.
			virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);
			//! 根据屏幕位置获取平板位置	gsl-2012/7/17
			void getPlanePosByScreenPos(core::position2di pos, core::vector3df& planePos);
			//! 根据屏幕上指定的点与相机位置，构造直线，计算此直线与平面相交点，换算成纹理图像的行、列比例后返回。gsl-2012/7/17
			void getImageScaleByScreenPos(core::position2di pos, core::position2df& planePos);
			//! 像素坐标得到屏幕坐标,imgPt是像素比例。gsl-2012/7/17
			void getScreenPosByImageScale(core::position2df imgPt, core::position2di& srcpos);	
			//! 获取纹理,便于修改
			ITexture* getTexture();

			// 设置dom纹理文件绝对路径
			void SetDomTexturePath(std::string strPath);

			// 获取dom纹理文件路径
			std::string GetDomTexturePah();

			// 更新纹理
			void UpdateTexture(ITexture* pTexture,Chd2DBoundingBoxd box);

			// 设置大小，若设置为2，表示为缩略，为10则按原尺寸显示
			void SetRadius(f32 radius);

			//! 获取设置的半径
			f32 GetRadius() { return Radius;}

            // ! 根据视口重新加载数据
            BOOL ReloadData();

			////! 设置center XY
			//void SetCentXY(double dx,double dy,double dz);

		private:
			// 贴图板大小
			core::dimension2d<f32> Size;
			core::vector3df centerPos; // 贴图中心位置
			SMeshBuffer* Buffer; // 贴图buffer
			core::dimension2d<f32> m_textureSize; // 纹理大小

			// boundingbox
			Chd2DBoundingBoxd					m_BoundingBox;

			// 记录dom纹理文件绝对路径
			std::string m_strDomPath;

			// 记录一个缩略半径
			f32 Radius;

			// 记录显示缩略时，记录tfw坐标
			double m_fCentX;
			double m_fCentY;
			double m_fCentZ;
		};
	} // end namespace scene
} // end namespace irr

//#endif


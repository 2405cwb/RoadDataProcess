/*! @file
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : CPlanarSceneNode.h
相关文件     : CPlanarSceneNode.cpp, IPlanarSceneNode.h
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
#pragma once
//#ifndef __C_PLANAR_SCENE_NODE_H_INCLUDED__
//#define __C_PLANAR_SCENE_NODE_H_INCLUDED__

#include "..\hd3DEngine\include\irrlicht.h"
#include "..\hd3DEngine\include\SMeshBuffer.h"
#include "..\hd3DEngine\include\ITexture.h"
#include "..\hd3DScene\iobjectscenenode.h"

using namespace irr;
using namespace irr::video;
using namespace irr::scene;

namespace hd
{
	namespace scene
	{
		//! Scene node which is a plane. A plane is like a 3d sprite: A 2d element,
		//! which always looks to the camera. 
		class HD3DSCENE_API CPlanarSceneNode : virtual public IObjectSceneNode
		{
		public:

			//! constructor
			CPlanarSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,	
				const core::vector3df& position, const core::dimension2d<f32>& size, video::ITexture* text);
			CPlanarSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,const core::vector3df& position
				,const io::path &filename);
			~CPlanarSceneNode();

			//added by wkl 2012-7-13 17:03:49
			inline core::dimension2d<f32> getTextureSize();

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
			virtual ESCENE_NODE_TYPE getType() const { return ESNT_PLANAR; }

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

            //! 获取纹理图片的大小
            core::dimension2di GetImageSize(){return m_imageSize;}

        private:
            //！根据指定的纹理图片路径，创建纹理
            video::ITexture* CreateTexture(const char* pcFileName);

		private:

			core::dimension2d<f32> Size;
			core::vector3df centerPos;
			SMeshBuffer* Buffer;
			//core::dimension2d<f32> m_textureSize;

            core::dimension2di m_imageSize;  // 纹理图片的实际大小
		};
	} // end namespace scene
} // end namespace irr

//#endif


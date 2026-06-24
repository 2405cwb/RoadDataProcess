/*! @file
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : CPanoSceneNode.h
相关文件     : CPanoSceneNode.cpp, IPanoSceneNode.h
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
//#ifndef __C_PANO_SCENE_NODE_H_INCLUDED__
//#define __C_PANO_SCENE_NODE_H_INCLUDED__

#include "..\hd3DEngine\include\ISceneNode.h"
#include "..\hd3DEngine\include\SMeshBuffer.h"
#include "IObjectSceneNode.h"
#include "..\hd3DEngine\include\ITexture.h"
#include "..\hdCommon\sceneData\HdFileData.h"
#include "..\hdCommon\hdSvDBStructDef.h"

#include "PanoTileAidHelper.h"

using namespace irr;
using namespace irr::video;
using namespace hd;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CPanoSceneNode : public IObjectSceneNode
		{
		public:
			CPanoSceneNode(video::ITexture* texture,
				f32 horiStart, f32 horiEnd, f32 vertStart, f32 vertEnd, 
				f32 radius,	ISceneNode* parent, ISceneManager* mgr, s32 id);
			CPanoSceneNode(f32 horiStart, f32 horiEnd, f32 vertStart, f32 vertEnd, 
				f32 radius,	bool bIsTile, ISceneNode* parent, ISceneManager* mgr, s32 id);
			virtual ~CPanoSceneNode();
			virtual void OnRegisterSceneNode();
			virtual void render();
			virtual const core::aabbox3d<f32>& getBoundingBox() const;
			virtual video::SMaterial& getMaterial(u32 i);
			virtual u32 getMaterialCount() const;
			virtual ESCENE_NODE_TYPE getType() const { return ESNT_PANO; }

			virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;
			virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);
			virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);

			void GetSpherePos(const core::vector2df angle, core::vector3df& spherePos);
			
			//根据影像比例坐标得到球体上的坐标
			void GetSpherePosByImageScale(const core::vector2df& imageScale, core::vector3df& spherePos);
			
			//! 根据屏幕上指定的点与相机位置
			void GetImageScaleByScreenPos(core::position2di pos, core::position2df& imgScale);
			
			//! 像素坐标得到屏幕坐标,imgPt是像素比例。
			void GetScreenPosByImageScale(core::position2df imgPt, core::position2di& screenPos);

			//! 获得屏幕上的像素坐标
			void GetScreenPostion(core::position2di pos, core::position2df& imgScale);
			
			//! 获取纹理,便于修改
			ITexture* GetTexture();
			
			//! 更新纹理
			void Update();

			//! 更新纹理 
			void UpdateTexture(ITexture* text);

			//! 更新纹理
   //! lhq on 2016/11/21 修改,增加默认参数,如果获取纹理失败,全景用外部纹理渲染
			void UpdateTexture(const CHdPanoData* pPanoData, video::ITexture* panoTextureExt = NULL);
			//! 更新纹理-hdScene 用于解决变白问题
			void UpdateTextureScene(const CHdPanoData* pPanoData);

			//! 设置其注册模式
			void SetRegisterMode(bool bLikeSkyBox) { IsRenderLikeSkyBox = bLikeSkyBox; }

			//! 获取其注册模式
			bool GetRegisterMode() { return IsRenderLikeSkyBox;}

			//! 设置全景球的半径
			void SetRadius(f32 radius);

			//! 获取全景球的半径
			f32 GetRadius() { return Radius;}

			// 根据屏幕坐标得到像素坐标，imgScale是比例
			void GetImageScaleByScreenPos2( core::position2di pos, core::position2df& imgScale );

			// 根据指定向量得到像素坐标，imgScale是比例
			void GetImageScaleByScreenPos2(core::vector3df& norm,core::position2df& imgScale);

			//! 像素坐标得到屏幕坐标,imgPt是像素比例。
			void GetScreenPosByImageScale2(core::position2df imgScale, core::position2di& screenPos);

			//! 通过屏幕坐标得到在全景球上的3D显示坐标
			void Get3DPosByScreenPos2(core::position2di screenPos, core::vector3df& pos);

			//! 通过像素坐标来得到全景球上的3D显示坐标
			void Get3DPosByImageScale2(core::position2df imgScale, core::vector3df& pos);

			//! 外部调用，设置记录全景ID
			void SetPanoID(string strPanoID);

			//! 外部设置从mogo服务器获取切片数据，创建sn时调用
			void SetDataFrmMogo();

			//! 外部调用，获取记录全景ID
			string GetPanoID() {return m_strPanoID;}
		protected:
			//! 添加接口，通过屏幕坐标获得该点在球面上的角度
			void GetAngleByScreenPos(core::position2di pos,core::vector2df& angle);

			//! 构造时初次创建,初次创建时指定层级为2,根据半径设置进行构建，当视图上下帧浏览等更新
			void CreateTexture_mogo();

			void GenerateMesh();

			// 原完整全景sn
			SMeshBuffer* Buffer;

			// 用于显示0级全景（半径为2.0）
			SMeshBuffer* Buffer_Zero;

			u32 HorizontalResolution, VerticalResolution;
			f32 HoriStartAngle, HoriEndAngle, VertStartAngle, VertEndAngle;
			f32 Radius;

			bool IsRenderLikeSkyBox;

			//! 记录该pano sn的ID
			string m_strPanoID;

/******************************pano sn切片显示新方法 start**************************/
		public:
			// 根据全景ID获取0级切片数据，无二进制影像数据，由外部对CHdSvTileInfoBuffer进行填充
			CHdSvTileInfoBuffer* GetZeroPanoTile(const char* strPanoID);

			// 球体半径为2时，设置0级纹理
			void SetZeroTextureRadius(CHdSvTileInfoBuffer* pZeroInfo);

			// 设置0级切片纹理贴图，包含二进制影像数据，进行纹理贴图
			void SetZeroPanoTexture(CHdSvTileInfoBuffer* pZeroInfo);

			// 根据传入参数获取数据,在此前，若为定位，则需先调用获取0级数据用于更新设置新球、全景ID等
			void GetPanoTiles(vector<CHdSvTileInfoBuffer*>& vecTileBuffer);

			// 外部设置当前需渲染为切片全景=true
			void SetTileModel(bool bTile){m_bTile = bTile;}

			// 外部获得当前渲染全景为tile还是整体
			bool isTileModel(){return m_bTile;}
		private:
			// 全景切片贴图助手
			CPanoTileAidHelper* m_pPanoTileHelper;

			// 标记当前渲染为切片全景还是整体全景
			bool m_bTile;
/******************************pano sn切片显示新方法   end**************************/
		};
	} // end of namespace scene
}// end of namespace hd

//#endif


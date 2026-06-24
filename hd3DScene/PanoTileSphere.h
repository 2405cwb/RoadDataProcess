/*! @PanoTileSphere
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : PanoTileSphere.h
相关文件     : PanoTileSphere.cpp, CPanoSceneNode
文件实现功能 : 全景切片显示球控制类
作者         : 软件部，朱旭波
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2016/05/03   1.0      朱旭波              新增加内容
</PRE>
*******************************************************************************/
#pragma once
#include "..\hd3DEngine\include\ISceneNode.h"
#include "..\hd3DEngine\include\SMeshBuffer.h"
#include "IObjectSceneNode.h"
#include "..\hd3DEngine\include\ITexture.h"
#include "..\hdCommon\sceneData\HdFileData.h"
#include "..\hdCommon\hdSvDBStructDef.h"

#include "..\hdCommon\HdSvTileInfoBuffer.h"

using namespace irr;
using namespace irr::video;
using namespace hd;

namespace hd
{
	namespace scene
	{
		class CPanoTileSphere
		{
		public:
			// 构造
			CPanoTileSphere(IHdView* pView,int nHoriCount,int nVertCount,float radius);
			
			// 析构
			~CPanoTileSphere(void);

			// 获得包围盒
			core::aabbox3d<f32>& getBoundingBox();

			// 清除球所有材质的纹理
			void clearAllTextures();

			// 0级球构建纹理
			void GenerateZero();

			// 设置0级纹理
			void SetZeroTexture( CHdSvTileInfoBuffer* pZeroInfo );

			// 设置球体所在位置
			void SetPanoSnPosition(irr::core::vector3df pos);

			// 设置相对位置矩阵
			void SetPanoSnMatrix(core::matrix4 m);

			// 计算当前视口范围内切片
			vector<int>& CalInView();

			// 获得球体水平切片个数
			int GetHoriTileCount(){return m_nHoriCount;}

			// 获得球体垂直切片个数
			int GetVertTileCount(){return m_nVertCount;}

			// 外部获得单个SMaterial的纹理,适用于三、四级
			ITexture* GetTexture(int index);

			// 外部获得0级切片纹理
			ITexture* GetZeroTexture();

			// 外部设置单个SMaterial纹理，适用于三、四级
			void SetTexture(int index,CHdSvTileInfoBuffer* pInfoBuf);

			// 刷新时调用
			void Render();
		private:
			// 由 角度获得球体位置坐标
			void GetSpherePos(const core::vector2df angle, core::vector3df& spherePos);

			// 由水平、垂直角度获得屏幕坐标(参照PANO SN实现)
			void GetScreenPosByAngle(const core::vector2df angle,core::position2di& screenPos);

		private:
			// vec标记计算当前哪些mesh在视口范围内，方便后续查询，渲染使用
			vector<int> m_vecInView;

			// 记录视图
			IHdView* m_pView;

			// 顶点索引
			u16 m_Indices[4];

			// 球半径设置
			f32 m_Radius;

			//// 记录三级切片构架顶点及材质
			//video::S3DVertex* m_Vertices;
			//video::SMaterial* m_Material;

			// 记录总材质个数-hori
			int m_nHoriCount;

			// 记录材质个数-vert
			int m_nVertCount;

			// 用于记录0级buf
			SMeshBuffer* Buffer_0;

			// 用于记录其他层级（3、4级）buf
			std::vector<SMeshBuffer*> m_vecBuffer;
			//SMeshBuffer* m_vecBuffers;

			// 记录pano sn所在视图位置getPosition
			irr::core::vector3df m_panoPosition;

			// 用于记录sn的相对位置矩阵，由pano sn传入
			core::matrix4 m_ralativeMatrix;
			// 范围
			core::aabbox3d<f32> m_box;
		};
	}
}



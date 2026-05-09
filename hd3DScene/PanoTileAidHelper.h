/*! @PanoTileAidHelper
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : PanoTileAidHelper.h
相关文件     : PanoTileAidHelper.cpp, CPanoSceneNode
文件实现功能 : 全景切片显示辅助类，用于实现全景切片显示
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

#include "PanoTileSphere.h"
#include <list>
#include "..\hdCommon\HdSvTileInfoBuffer.h"

using namespace irr;
using namespace irr::video;
using namespace hd;

namespace hd
{
	namespace scene
	{
		class CPanoTileAidHelper
		{
		public:
			// 构造
			CPanoTileAidHelper(IHdView* pView);

			// 析构
			~CPanoTileAidHelper(void);

			// 获得包围盒
			core::aabbox3d<f32>& getBoundingBox();

			// 根据全景ID获取0级切片数据
			CHdSvTileInfoBuffer* GetZeroPanoTile(const char* strPanoID);

			// 设置0级切片纹理贴图
			void SetZeroPanoTexture(CHdSvTileInfoBuffer* pZeroInfo);

			// 获得0级切片纹理
			ITexture* GetZeroPanoTexture();

			// 根据传入参数获取数据,在此前，若为定位，则需先调用获取0级数据用于更新设置新球、全景ID等
			void GetPanoTiles(irr::core::vector3df pos,core::matrix4 m,
				vector<CHdSvTileInfoBuffer*>& vecTileBuffer);

			// render时调用
			void Render();

		private:
			// 初始化设置球体信息
			void InitialSpheres();

			// 内部计算，球体切片设置获取(适用于计算三、四级球体)
			void CalPanoTile(int nCurLevel,CPanoTileSphere* pTileSphere,
				irr::core::vector3df pos,core::matrix4 m,
				vector<CHdSvTileInfoBuffer*>& vecTileBuffer); // vecTileBuffer用于记录需要查询本地or服务器的切片

			// 清空缓存列表,此时数据内存也会释放
			void ClearCache();

			// 传入全景ID，若与原记录不同，则更换球体
			void SetPanoID(const char* strNewPanoID);

			// 清空指定球纹理
			void ClearSphereTexture(int nSphereID);

			// 检查内存缓存是否存在
			bool CheckTileCacheExist(const char* strTitleName,CHdSvTileInfoBuffer*& pInfoBuf);

		private:

			// 由于可能存在多线程请求服务器与刷新跟不上，
			// 仍缓存存储当前站点切片，站点切换时清空
			list<CHdSvTileInfoBuffer*> m_CacheList;

			// 记录视图
			IHdView* m_pView;

			// 记录当前全景站点名
			string m_strCurPanoID;

			// 标记当前全景站点使用球体
			int m_nCurSphere;

			// 标记当前全景应渲染层级（只支持3、4级）
			int m_nCurLevel;

			// 球体标记-1号球0级
			CPanoTileSphere* m_pSphere_1_0;

			// 球体标记-1号球3级
			CPanoTileSphere* m_pSphere_1_3;

			// 球体标记-1号球4级
			CPanoTileSphere* m_pSphere_1_4;

			// 球体标记-2号球0级
			CPanoTileSphere* m_pSphere_2_0;

			// 球体标记-2号球3级
			CPanoTileSphere* m_pSphere_2_3;

			// 球体标记-2号球4级
			CPanoTileSphere* m_pSphere_2_4;
		};
	}
}



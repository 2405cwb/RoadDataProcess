/*! hdQuickView.h
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : hdQuickView.h
相关文件     : 
文件实现功能 : 实现快速视图封装 
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/03/7    1.0      龚书林					
</PRE>
*******************************************************************************/

#pragma once
#include "ISceneView.h"
#include "CPanoSceneNode.h"
#include "CScanSceneNode.h"
//#include "sceneData\hdSceneScan.h"

using namespace irr;
using namespace hd;
using namespace hd::scene;

namespace hd
{
	class CHdPanoPicFileData;
	class CHdPanoData;

	namespace scene
	{
		//! 快速视图，相机位置在原点旋转浏览
		class HD3DSCENE_API CHdQuickView :
			public ISceneView
		{		
		public:
			CHdQuickView(void);
			virtual ~CHdQuickView(void);

			//! 获取全景SceneNode
			CPanoSceneNode* GetPanoSceneNode(){return m_pPanoSN;}

			//! 添加全景图片文件
			CPanoSceneNode* AddPanoObject(const CHdPanoPicFileData* pPanoFile);

			//! 添加全景图片内存对象
			CPanoSceneNode* AddPanoObject(const CHdPanoData* pPanoData);

			//! 根据图像比例获取球面坐标
			void GetSpherePosByImageScale( const core::vector2df& imageScale, core::vector3df& spherePos );

			//! 根据图像比例获取屏幕坐标
			//void GetScreenPosByImageScale(const core::vector2df imgScale, core::vector2di& ptSC);

			//! 根据屏幕坐标获取图像比例
			//void GetImageScaleByScreenPos(const core::vector2di& ptSC, core::vector2df& imgScale);

			//! 根据扫描点坐标获取球面坐标
			void GetSpherePosByScanPos(const core::vector3df& scanPos, core::vector3df& spherePos);

			//! 根据扫描点坐标获取角度
			void GetAngleByScanPos(const core::vector3df& scanPos, core::vector2df& angle);

		protected:
			//! 重载设置相机函数，以区别于3D视图
			virtual void SetCamera();

		protected:
			CPanoSceneNode* m_pPanoSN;

		};
	}
}

/*! @file
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : CAnimatorFlyCamera.h
相关文件     : CAnimatorFlyCamera.cpp
文件实现功能 : 实现相机scenenode移动
			   在相机目标点不动的情况下，保持移动过程中没有翻转等影响浏览效果
			   实现方式1.首先移动结点到target点与目标点的连线上，其次再移动到目标点
			   实现方式2.首先将终点作为target点，同时移动结点至终点，之后将target点设置为之前的target点 问题是开始会产生跳变效果
作者         : 危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/08/09   1.0      危迟              新增加内容
</PRE>
*******************************************************************************/
#pragma once

#include "..\hdFramework\hdView.h"
#include "IObjectSceneNode.h"

using namespace irr;
using namespace irr::scene;

namespace hd
{
	namespace scene
	{

		class CAnimatorFlyCamera
		{
		public:
			CAnimatorFlyCamera(IHdView* pView,const core::vector3df& startPos,
				const core::vector3df& endPos, long timeForWay);
			~CAnimatorFlyCamera(void);

			void Run();

			// 设置所在的视图
			void SetView(IHdView* pView) { m_pView = pView; }

		private:
			void CalculateNodePos();

		private:
			// 起始位置
			core::vector3df m_vStartPos;
			// 终止位置
			core::vector3df m_vEndPos;
			// 持续时间 单位为毫秒ms
			long m_nConsistTime;
			// animator所在视图
			IHdView*		m_pView;	
			// 路径向量1
			core::vector3df m_vPathFirst;
			// 路径向量2
			core::vector3df m_vPathSecond;
			// 时间因子
			f32 m_fTimeFactor;

		};
	}
}


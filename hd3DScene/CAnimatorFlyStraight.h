/*! @file
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : CAnimatorFlyStraight.h
相关文件     : CAnimatorFlyStraight.cpp
文件实现功能 : 实现scenenode的直线运动
作者         : 危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/04/15   1.0      危迟              新增加内容
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
		class HD3DSCENE_API CAnimatorFlyStraight 
		{
		public:
			CAnimatorFlyStraight(ISceneNode* node,IHdView* pView,const core::vector3df& startPos,
				const core::vector3df& endPos, long timeForWay);
			~CAnimatorFlyStraight(void);

			void Run();

			// 设置所在的视图
			void SetView(IHdView* pView) { m_pView = pView; }

		private:
			void CalculateNodePos();
		private:
			// 起始位置
			core::vector3df StartPos;
			// 终止位置
			core::vector3df EndPos;
			// 持续时间 单位为毫秒ms
			long ConsistTime;
			// 运动的scenenode
			ISceneNode* Node;
			// animator所在视图
			IHdView*		m_pView;	
			// 路径向量
			core::vector3df Vector;
			// 时间因子
			f32 TimeFactor;
			
		};
	}
}




/*! @file
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : CAnimatorHighlightSn.h
相关文件     : CAnimatorHighlightSn.cpp
文件实现功能 : 通过闪烁实现scenenode的高亮显示渲染
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

		class HD3DSCENE_API CAnimatorHighlightSn
		{
		public:
			CAnimatorHighlightSn(ISceneNode* node,IHdView* pView, long timeForWay);
			~CAnimatorHighlightSn(void);

			void Run();

			// 设置所在的视图
			void SetView(IHdView* pView) { m_pView = pView; }

		private:
			// 持续时间 单位为毫秒ms
			long ConsistTime;

			// 高亮的scenenode
			ISceneNode* Node;

			// animator所在视图
			IHdView*		m_pView;	

		};
	}
}


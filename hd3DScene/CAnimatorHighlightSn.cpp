/*! @file
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : CAnimatorHighlightSn.cpp
相关文件     : CAnimatorHighlightSn.h
文件实现功能 : 实现scenenode的高亮显示渲染
作者         : 危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/09/16   1.0      危迟              新增加内容
</PRE>
*******************************************************************************/
#include "StdAfx.h"

#include "CAnimatorHighlightSn.h"
#include "ISceneView.h"
#include <time.h>


namespace hd
{
	namespace scene
	{

		CAnimatorHighlightSn::CAnimatorHighlightSn(ISceneNode* node,IHdView* pView,long timeForWay) 
			: Node(node),m_pView(pView),ConsistTime(timeForWay)
		{

		}


		CAnimatorHighlightSn::~CAnimatorHighlightSn(void)
		{
		}

		void CAnimatorHighlightSn::Run()
		{
			// 获取场景结点
			if (!m_pView)
			{
				return;
			}

			ISceneView* pView = dynamic_cast<ISceneView*>(m_pView);

			if (!pView)
			{
				return;
			}

			// 获取场景节点管理器
			ISceneManager* smgr = pView->GetSceneManager();

			// 记下初始时间
			long startTime = clock();

			// 当前时间
			long curTime = startTime;

			long spendTime = curTime - startTime;

			// 刷新次数 
			int refreshCount = 0;

			// 在没有达到终止时间之前，一直刷新
			while (spendTime < ConsistTime)
			{
				if (refreshCount%2 == 0)
				{
					Node->setVisible(false);
				}
				else
				{
					Node->setVisible(true);
				}

				refreshCount++;

				//刷新视图	
				pView->RefreshViewBySendMessage();
 
				curTime = clock();
				spendTime = curTime - startTime;
			}

			Node->setVisible(true);

			pView->Refresh();
		}
	}
}
/*! @file
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : CAnimatorFlyStraight.cpp
相关文件     : CAnimatorFlyStraight.h
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
#include "StdAfx.h"
#include "CAnimatorFlyStraight.h"
#include "ISceneView.h"
#include <time.h>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	namespace scene
	{
		CAnimatorFlyStraight::CAnimatorFlyStraight(ISceneNode* node,IHdView* pView,const core::vector3df& startPos,
			const core::vector3df& endPos, long timeForWay) : Node(node),m_pView(pView),StartPos(startPos),EndPos(endPos),ConsistTime(timeForWay)
		{
			if (!Node)
			{
				return;
			}
			CalculateNodePos();
		}


		CAnimatorFlyStraight::~CAnimatorFlyStraight(void)
		{
		}

		void CAnimatorFlyStraight::Run()
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

			// 在没有达到终止时间之前，一直刷新
			while (spendTime < ConsistTime)
			{
				f32 phase = fmodf((f32)spendTime,(f32)ConsistTime);
				core::vector3df rel = Vector*phase*TimeFactor;

				core::vector3df newPos = StartPos + rel;
				Node->setPosition(newPos);
				//刷新视图	
				pView->RefreshViewBySendMessage();

				curTime = clock();
				spendTime = curTime - startTime;
			}
			//循环结束时，保证移动到目的位置
			Node->setPosition(EndPos);

			pView->Refresh();
		}

		void CAnimatorFlyStraight::CalculateNodePos()
		{
			// 通过当前时间，计算每次刷新时，node应该所在的位置
			Vector = EndPos - StartPos;
			TimeFactor = (f32)Vector.getLength() / ConsistTime;
			Vector.normalize();
		}

	}
}


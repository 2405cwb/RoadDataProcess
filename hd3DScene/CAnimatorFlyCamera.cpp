#include "StdAfx.h"
#include "CAnimatorFlyCamera.h"
#include "ISceneView.h"
#include <time.h>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	namespace scene
	{

		CAnimatorFlyCamera::CAnimatorFlyCamera(IHdView* pView,const core::vector3df& startPos,
			const core::vector3df& endPos, long timeForWay) : m_pView(pView),m_vStartPos(startPos),m_vEndPos(endPos),m_nConsistTime(timeForWay)
		{
			CalculateNodePos();
		}


		CAnimatorFlyCamera::~CAnimatorFlyCamera(void)
		{
		}

		void CAnimatorFlyCamera::Run()
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

			// 
			ISceneManager* smgr = pView->GetSceneManager();

			if (!smgr)
			{
				return;
			}
			ICameraSceneNode* cam = smgr->getActiveCamera();
			if (!cam)
			{
				return;
			}
			// 记下初始时间
			long startTime = clock();
			
			// 开始，将移动终点设为target点
			//core::vector3df iniTar = cam->getTarget();

			//cam->setTarget(m_vEndPos);

			// 当前时间
			long curTime = startTime;

			long spendTime = curTime - startTime;

			// 在没有达到终止时间之前，一直刷新
			while (spendTime < m_nConsistTime)
			{
				f32 phase = fmodf((f32)spendTime,(f32)m_nConsistTime);
				core::vector3df rel = m_vPathFirst*phase*m_fTimeFactor;

				core::vector3df newPos = m_vStartPos + rel;
				//core::vector3df newTar = m_vEndPos + rel;
				// 通过距离判断是否已经超过第一段

				cam->setPosition(newPos);
				//cam->setTarget(newTar);
				
				//刷新视图	
				pView->RefreshViewBySendMessage();

				curTime = clock();
				spendTime = curTime - startTime;
			}

			//循环结束时，保证移动到目的位置
			cam->setPosition(m_vEndPos);
			
			// 将原来的target点设置为target点
			//cam->setTarget(iniTar);

			pView->Refresh();
		}

		void CAnimatorFlyCamera::CalculateNodePos()
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

			// 
			ISceneManager* smgr = pView->GetSceneManager();

			if (!smgr)
			{
				return;
			}

			ICameraSceneNode* cam = smgr->getActiveCamera();
			if (!cam)
			{
				return;
			}

			// 计算初始位置到目标位置与target点的位置交点
			core::vector3df target = cam->getTarget();
			core::line3df line(target,m_vEndPos);

			core::vector3df interSec;
			line.getFOPFromPointP(m_vStartPos,interSec);

			// 通过当前时间，计算每次刷新时，node应该所在的位置

			// 第一段路径
			m_vPathFirst = interSec - m_vStartPos;

			// 第二段路径
			m_vPathSecond = m_vEndPos - interSec;

			m_fTimeFactor = (f32)(m_vPathFirst.getLength() + m_vPathSecond.getLength()) / m_nConsistTime;

			m_vPathFirst.normalize();
			m_vPathSecond.normalize();
		}
	}
}

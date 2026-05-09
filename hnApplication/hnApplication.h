#ifndef _HN_3D_APPLICATION_H_
#define _HN_3D_APPLICATION_H_

#include "hnapplication_global.h"
#include "..\hdframework\hdapp.h"
#include "..\hdCommon\CClassificationMap.h"
#include "..\hd3DScene\HdViewScrBuffer.h"
#include "..\hnCommon\hnPointXYZIDef.h"
#include "hnView.h"
#include "hn2dPixWidget.h"
#include "hn2dPixScrollWidget.h"
#include "hn3dPixScrollWidget.h"
#include "..\hnCommon\hnRoadStruct.h"
#include "hnStreetCameraView.h"
using namespace hd::fm;
using namespace hd;
using namespace hnCommon;

namespace hnApp
{
	class HNAPPLICATION_EXPORT hnApplication : public CHdApp
	{
	private:
		hnApplication();

		virtual ~hnApplication();

		// 定义一个单实例销毁辅助类
		class hdAppCleaner
		{
		public:
			hdAppCleaner() {}
			virtual ~hdAppCleaner()
			{
				if (m_pHnApp != NULL)
				{
					delete m_pHnApp;
					m_pHnApp = NULL;
				}
			}
		};

	public:
		// 获取单实例
		static hnApplication* getApp();

		// 析构单实例
		static void destroyApp();

		//创建路面破损窗口
		hn2dPixScrollWidget* newRaodDamageContinousBrowserPixWidget();

		//创建点云影像窗口
		hn3dPixScrollWidget* new3DImageViewWidget();

		//! 创建3D视图
		IHdView* new3DView(HWND hwnd);

		//！ 创建影像视图
		hnView* newImageView(QWidget *parent, QString strViewName, VIEW_TYPE nViewType);

		//! 根据类型获取影像视图
		hnView* getViewByType(VIEW_TYPE nViewType);

		//! 关闭文档,释放数据IHdView
		void closeDocument();

		//! 删除所有命令
		void DeleteAllCommand();

		//! 关闭视图
		virtual void CloseView(IHdView* pView);

		//! 注册命令
		virtual void RegisterCommand();

		//! 设置当前工具
		virtual void SetCurrentTool(int id);

		//! 设置当前工具,工具由外部创建
		virtual void SetCurrentTool(CHdTool* pTool);

		//! 窗口消息处理
		virtual void WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		// 只设置活动视图
		void SetActiveViewOnly(IHdView* pView);

		// 在测量工具和全景工具进行切换
		void ChangeMeasureTool(bool bChange);

		//！设置是否需要转换模型参数
		void SetIsResetTransModel(bool IsReset)
		{
			m_bIsResetModel = IsReset;
		}

		// 添加点云
		void addPtCloud(const char* strPcd);

		// 视图中是否有点云
		bool isExistPcdInView(const char* pViewName = "absoluteView");

		// 删除所有节点
		void clearAllNode(const char* pViewName = "absoluteView");

		// 更新点云参数
		void updatePcdInfo(double dDist, int n3DViewCnt, int nPanoViewCnt);

		// 根据传入句柄获取对应视图;
		IHdView* getViewByHwnd(const HWND& hwnd);

		// 将点坐标添加到指定视图中，创建sn为CScanPartPointsSceneNode，不存在则创建，存在则更新视图;
		bool updatePartPcdInView(vector<hnPointXYZIF>& pPartPts, const char* pViewName);

	private:

		// 单实例对象
		static hnApplication* m_pHnApp;

		//! 命令列表
		std::vector<CHdCommand*>	m_cmdList1;

		//! 是否重置转换模型
		bool						m_bIsResetModel;

		// 影像视图
		vector<hnView*> m_vecImageView;
	};
}

#endif

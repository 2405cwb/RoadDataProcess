/*! hdView.h
********************************************************************************
<PRE>
模块名       : hdApplication
文件名       : hdView.h
相关文件     : 
文件实现功能 : 实现视图基类封装，三维视图、平面视图、快速视图由此派生
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/06/20   1.0      龚书林    
2013/07/15	 1.1      危迟				 增加name属性及相关接口
2014/02/21	 1.2      龚书林			 增加事件相关接口
</PRE>
*******************************************************************************/

#ifndef _HDVIEW_H_
#define _HDVIEW_H_
#pragma  warning(disable:4251)
#include "..\hdCore\hdDefs.h"
#include "..\hdCommon\HDObject.h"
#include "..\hdCommon\hdSceneType.h"
#include "..\hdCore\hdColor.h"
#include "stdafx.h"
#include <vector>
#include <Windows.h>
using namespace std;
using namespace hd;

namespace hd
{
	namespace scene
	{
		enum ENUM_HD_VIEW_TYPE
		{
			E_HVT_SCENE 			= 0,
			E_HVT_3D				= 1,			// 三维视图
			E_HVT_QUICK				= 2,			// 快速视图
			E_HVT_SCALEIMAGE		= 3,			// 平面视图和快速视图的基类
			E_HVT_PLANAR			= 4,			// 二维视图
			E_HVT_REG				= 5,			// 拼接视图
			E_HVT_PICTURE			= 6,			// 图片浏览视图
			E_HVT_PLANARMATCH		= 7,			// 全景配准平板视图
			E_HVT_PICTUREMATCH		= 8,			// 全景配准平板视图图片视图
			E_HVT_MULTISCAN3D		= 9,			// 多测站3D视图
			E_HVT_MLS3D				= 10,			// MLS数据3D视图
			E_HVT_OVL3D				= 11,			// Ovl数据3D视图
			E_HVT_FACADEEDIT		= 12,			// 面片编辑视图——StreetView
			E_HVT_PANO				= 13,			// 全景影像视图——StreetView
			E_HVT_PANOBLUR			= 14,			// 全景平面视图——StreetView
			E_HVT_ORTHO3D			= 15,           // 正交三维视图 俯视图、侧视图、后视图
			E_HVT_3DVIEWMATCH       = 16,           // 全景配准平板三维视图
			E_HVT_PANOMATCH         = 17,           // 任意影像与点云配准全景图片视图
			E_HVT_IMAGEMATCH		= 18,			// 车载全景与点云选点配准
			E_HVT_DEM_PLANAR		= 19,			// Tif平面视图
			E_HVT_MCAM_CALIBPCD		= 20,			// M-CAM标定点云灰度图视图
			E_HVT_MCAM_CALIBPIC		= 21,			// M-CAM标定图片视图
			E_HVT_MODEL_3D_VIEW     = 22,           // 标记模型视图类型
			E_HVT_DOM_3D_VIEW       = 23,           // 点云与dom叠加显示视图
            E_HVT_SKETCH_ISCAN3D    = 24,           // 草图视图
            E_HVT_REG_QUICK         = 25,           // 快速视图拼接视图
            E_HVT_OVERALL_VIEW      = 26,           // 总览视图
			E_HVT_UNKNOWN			= 99,			// 未知类型
		};

		[event_source(native)]
		class HDFRAMEWORK_API IHdView
		{
		protected:
			//! 窗口宽度
			int m_width;
			//! 窗口高度
			int m_height;
			//! 窗口大小发生改变
			bool m_bSizeChanged;	
			//! 窗口句柄
			HWND m_hWnd;
			//! 视图类型
			ENUM_HD_VIEW_TYPE m_viewType;
			//! 背景颜色
			CHdColor	m_bkColor;
			//! 名字属性
			string m_sName;
			//! PANO播放状态控制
			bool m_bPanoPlaySt;

		    
		public:// 声明事件 gsl-2014/02/21

			// 窗口大小改变事件
			__event void OnSizeEvent(int cx, int cy);
			// 视图异步刷新事件
			__event void OnRefreshEvent();
			// 视图同步刷新事件
			__event void OnRefreshSendEvent();
		public:
			//! 构造函数 
			IHdView()
				:m_width(0),m_height(0),m_bSizeChanged(false),m_bkColor(255,0,0,0),
				m_hWnd(NULL),m_viewType(E_HVT_UNKNOWN),m_sName("IHdView"),m_bPanoPlaySt(false)
			{
			}

			IHdView(ENUM_HD_VIEW_TYPE viewtype)
				:m_width(0),m_height(0),m_bSizeChanged(false),m_bkColor(255,0,0,0),
				m_viewType(viewtype),m_hWnd(NULL),m_sName("IHdView"),m_bPanoPlaySt(false)
			{				
			}

			virtual ~IHdView()
			{
			}

			//! 初始化目标窗口
			virtual void InitialView(HWND hwnd)
			{
				if (hwnd)
				{
					m_hWnd = hwnd;
					RECT rect;
					::GetClientRect(hwnd,&rect);
					m_width = rect.right;
					m_height = rect.bottom;
				}
			}

			//! 大小改变时调用OnSize
			virtual void OnSize(int cx, int cy)
			{
				if (cx != m_width || cy != m_height)
				{
					m_bSizeChanged = true;
					m_width = cx;
					m_height = cy;

					// 触发窗口大小改变事件
					__raise OnSizeEvent(cx,cy);
				}
			}

			//! 刷新视图
			virtual void Refresh() = 0;
			//! 通过发送消息刷新视图 满足即时刷新需求 
			virtual void RefreshViewBySendMessage() = 0;

			//! 当前视图类型
			ENUM_HD_VIEW_TYPE GetViewType() const {return m_viewType;}

			//! 设置背景颜色
			void SetBkColor(const CHdColor& bkColor)
			{
				m_bkColor = bkColor;
			}
			//! 设置背景颜色
			void SetBkColor(u32 clr)
			{
				m_bkColor.set(clr);
			}
			//! 设置背景颜色
			void SetBkColor(u32 a, u32 r, u32 g, u32 b)
			{
				m_bkColor.set((((a & 0xff)<<24) | ((r & 0xff)<<16) | ((g & 0xff)<<8) | (b & 0xff)));				
			}
			//! 获取背景颜色
			CHdColor GetBkColor() const{ return m_bkColor;}

			HWND GetHWnd() 
			{
				return m_hWnd;
			};
			//! 获取窗口宽度
			int GetWindowWidth() { return m_width; }

			//! 获取窗口高度
			int GetWindowHeight() { return m_height; }

			//! 设置视图名
			void SetName(const char* name) { m_sName = name; }

			//! 获取视图名
			const char* GetName() { return m_sName.data(); }

			//! 窗口消息处理
			virtual void WindowProc(UINT message, WPARAM wParam, LPARAM lParam) = 0;

			//! 向Windows视图窗口发送消息
			void SendMsgToWindowsView(UINT message, WPARAM wParam, LPARAM lParam)
			{
				if(m_hWnd)
					::SendMessage(m_hWnd, message, wParam, lParam);
			}

			void SetPanoPlayStatus(bool bplay){m_bPanoPlaySt = bplay;}
			bool GetPanoPlayStatus(){return m_bPanoPlaySt;}

			//! 关闭窗口,仅仅适合于MFC ChildFrame的子窗口CView的关闭
			/*virtual void Close()
			{
			if (m_hWnd)
			{
			HWND hParent = GetParent(m_hWnd);
			if(hParent)
			::SendMessage(hParent, WM_CLOSE,0,LPARAM(0));
			}
			}*/
		};
	}
}

#endif
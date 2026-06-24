#include "StdAfx.h"
#include "hdView.h"
#include <Windows.h>
namespace hd
{
	namespace app
	{
		void hdView::SendMsgToWindowsView(UINT message, WPARAM wParam, LPARAM lParam)
		{
			::SendMessage(m_wnd, message, wParam, lParam);
			//::PostMessage(m_wnd, message, wParam, lParam);
		}

		//void hdView::refresh()
		//{
		//	if (m_wnd)
		//	{
		//		::InvalidateRect(m_wnd,NULL,TRUE);
		//		//::SendMessage(m_wnd, WM_PAINT, 0, 0);
		//		//SendMessage会导致关闭平面/快速后其他平面/快速视图变白 
		//		//wkl 2012-8-1 11:01:40
		//	}
		//}

		//! 关闭窗口,仅仅适合于MFC ChildFrame的子窗口CView的关闭
		/*void IHdView::Close()
		{
			if (m_hWnd)
			{
				HWND hParent = GetParent(m_hWnd);
				if(hParent)
					::SendMessage(m_hWnd, WM_USER_CLOSEVIEW,0,LPARAM(0));
			}
		}*/
	}
	

}
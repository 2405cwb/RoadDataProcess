/*! hd3DCmdViewTop.h
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : hd3DCmdViewTop.h
相关文件     : 
文件实现功能 : 三维视图俯视图浏览工具
作者         : 危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/03/05   1.0      危迟    				
2013/09/30   1.1      蔡红云               控制视图不能重复点击
</PRE>
*******************************************************************************/
#ifndef _C_HD_3DCMDVIEW_TOP_H_
#define _C_HD_3DCMDVIEW_TOP_H_

#include "..\hdFramework\hdCommand.h"
#include "stdafx.h"
using namespace hd::fm;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CHd3DCmdViewTop
			:public CHdCommand
		{
		public:
			CHd3DCmdViewTop(void);

			~CHd3DCmdViewTop(void);
			virtual void OnClick();
			virtual void OnCreate(CHdApp* app);

			virtual bool GetEnable();
		};
	}
}


#endif



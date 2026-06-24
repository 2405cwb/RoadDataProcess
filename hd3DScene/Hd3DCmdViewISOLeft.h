/*! hd3DCmdViewISOLeft.h
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : hd3DCmdViewISOLeft.h
相关文件     : 
文件实现功能 : 三维视图左视图浏览工具
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
#ifndef _C_HD_3DCMDVIEW_ISO_LEFT_H_
#define _C_HD_3DCMDVIEW_ISO_LEFT_H_

#include "..\hdFramework\hdCommand.h"
using namespace hd::fm;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CHd3DCmdViewISOLeft
			:public CHdCommand
		{
		public:
			CHd3DCmdViewISOLeft(void);
			~CHd3DCmdViewISOLeft(void);

			virtual void OnClick();

			virtual void OnCreate(CHdApp* app);

			virtual bool GetEnable();
		};
	}
}

#endif

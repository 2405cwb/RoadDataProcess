/*! hd3DViewLookForward.h
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : hd3DViewLookForward.h
相关文件     : 
文件实现功能 : 实现三维视图下向上看浏览功能 
作者         : 危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/03/05   1.0      危迟    				
</PRE>
*******************************************************************************/
#ifndef _C_HD_3DVIEW_LOOK_FORWARD_H_
#define _C_HD_3DVIEW_LOOK_FORWARD_H_

#include "..\hdFramework\hdCommand.h"

using namespace hd::fm;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CHd3DViewLookForward
			:public CHdCommand
		{
		public:
			CHd3DViewLookForward(void);
			~CHd3DViewLookForward(void);

			virtual void OnClick();

			virtual void OnCreate(CHdApp* app);

			virtual bool GetEnable();
		};
	}
}


#endif
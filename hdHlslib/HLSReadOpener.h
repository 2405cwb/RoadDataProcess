/*! HLSReadOpener.h
********************************************************************************
<PRE>
模块名       : hdHLSlib
文件名       : HLSReadOpener.h
相关文件     : 
文件实现功能 : 海达数云点文件hls读取，打开辅助类
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/06/5   1.0      龚书林    
</PRE>
*******************************************************************************/

#pragma once
#include "IHLSReader.h"

namespace hd
{
	class IHLSReader;
	class HLS_API CHLSReadOpener
	{
	public:
		CHLSReadOpener(void);
		~CHLSReadOpener(void);

		IHLSReader* Open(const char* filePath);
	};
}

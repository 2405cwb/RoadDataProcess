/*! ReadImageThread.h
********************************************************************************
<PRE>
模块名       : HdDrawCanvas
文件名       : ReadRasterThread.h
相关文件     : 
文件实现功能 : 继承于ZThread,是读取影像的线程类
作者         : 马振明
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2015/06/17   1.0      马振明				新建
</PRE>
*******************************************************************************/

#pragma once
#include "..\3rd\ZThread\include\zthread\Runnable.h"
#include "..\3rd\ZThread\include\zthread\Thread.h"
#include "..\3rd\ZThread\include\zthread\CountedPtr.h"
#include "OverViewInfo.h"
#include <string>

using namespace std;
class CHdGeoRaster;
//using namespace ZThread;

namespace hd
{
	namespace scene
	{
		class CHdRasterBuffer;
		class HD3DSCENE_API CReadRasterThread : public ZThread::Runnable
		{
		public:
			CReadRasterThread(const ZThread::CountedPtr<CHdRasterBuffer>& pGeoRater,const char* strPath="",double dScale=0.5);
			~CReadRasterThread(void);

			// 执行
			void run();

		private:
			ZThread::CountedPtr<CHdRasterBuffer>	m_pImageBuffer;		// 影像缓存
			string m_strImagePath;							// 设置影像路径
			double m_dScale;								// 读取的缩放大小
		};
	}
}



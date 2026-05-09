/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdDrawCanvas
文件名		：RasterCache.h
相关文件	: HdMdcPaint.h
文件实现功能：矢量数据的缓存
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/6/15	1.0			马振明		创建
</PRE>
******************************************************************************************************/

#pragma once
#include "HdRasterBuffer.h"
#include <list>
#include "..\3rd\ZThread\include\zthread\ThreadedExecutor.h"
#include "..\3rd\ZThread\include\zthread\CountedPtr.h"
#include "include\BasicObject\BaseRect.h"

//using namespace ZThread;
using namespace std;
using namespace hd;

namespace hd
{
	namespace scene
	{
		class CHdRasterBuffer;

		// 定义迭代器，简化代码
		typedef list<ZThread::CountedPtr<CHdRasterBuffer>>::iterator imageCacheIt;
		// 影像缓存指针的对象，用了智能指针
		typedef ZThread::CountedPtr<CHdRasterBuffer> CHdRasterBufferPtr;

		class HD3DSCENE_API CHdRaterCache
		{
		public:
			explicit CHdRaterCache(float fMemory=0.1/*,int nThreadNum =5*/);
			~CHdRaterCache();

			// 添加一个影像缓存
			bool AddCache(CHdRasterBuffer* pImageBuffer);

			// 清空缓存列表,此时数据内存也会释放
			void ClearCache(); 

			// 检查缓存是否存在
			bool CheckCacheExist(CHdRasterBuffer* pImageBuffer);

			// 得到某个影像缓存的数据
			CHdRasterBufferPtr GetRaster(const char* strImageName,float dScale,const Chd2DBoundingBoxd& box);

			// 获取当前缓存大小
			float GetMemorySize();

			// 成员变量
		private:

			float m_fMemory;								// 内存大小,单位为G
			list<CHdRasterBufferPtr> m_CacheList;			// 影像缓存列表,列表存储的是智能指针的对象
			// 当list列表清除时，其内存会跟着指针释放
			ZThread::ThreadedExecutor m_pTherdExe;					// 线程执行器，
			// int	m_nThreadNum;								// 线程池个数,用PoolExecutor的情况，由于PoolExcutor在析构时存在一个单实例的线程队列，析构时是其状态设为cancel；而当该状态为cancel时，又无法执行线程，暂时修改为直接使用线程器
		};

	}
}



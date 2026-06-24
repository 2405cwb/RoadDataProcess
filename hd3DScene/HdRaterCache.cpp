#include "StdAfx.h"
#include "HdRaterCache.h"
#include "ReadRasterThread.h"
#include "include\BasicObject\hdGeoRaster.h"
#include "include\BasicObject\common.h"
#include "include\BasicObject\BaseMath.h"
#include "include\BasicObject\hdCheckMemory.h"
//using namespace base;

namespace hd
{
	namespace scene
	{
		CHdRaterCache::CHdRaterCache(float fMemory/*,int nThreadNum*/)
			:m_fMemory(fMemory)
			//,m_nThreadNum(nThreadNum)
			//,m_pTherdExe(nThreadNum)
		{
			// 检测内存泄露
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		}


		CHdRaterCache::~CHdRaterCache()
		{
			ClearCache();
		}


		// 添加一个影像缓存
		bool CHdRaterCache::AddCache(CHdRasterBuffer* pImageBuffer)
		{
			// 循环查找
			if (pImageBuffer)
			{
				// 判断是否存在
				bool bExit = CheckCacheExist(pImageBuffer);

				// 如果已经存在
				if (bExit)
				{
					delete pImageBuffer;
					pImageBuffer = NULL;
					return true;
				}
				else
				{
					// 如果内存过大，从顶部删除,y
					while(GetMemorySize() > m_fMemory)
					{
						// 先取消，再删除
						imageCacheIt iter = m_CacheList.begin();
						(*iter)->cancel();
						m_CacheList.pop_front();
					}

					// 定义指针对象
					CHdRasterBufferPtr pImage(pImageBuffer);

					m_CacheList.push_back(pImage);

					// 开始执行线程
					m_pTherdExe.execute(new CReadRasterThread(pImage));

					return true;
				}
			}
			return false;
		}

		// 清空缓存列表,此时数据内存也会释放
		void CHdRaterCache::ClearCache()
		{
			// 先取消之前的线程执行
			for (imageCacheIt iter = m_CacheList.begin();iter!=m_CacheList.end();iter++)
			{
				(*iter)->cancel();
			}

			m_CacheList.clear();
			//m_pTherdExe.cancel();
		}

		// 获取当前缓存大小
		float CHdRaterCache::GetMemorySize()
		{
			float fMemory =0;
			// 循环查找
			for (imageCacheIt iter = m_CacheList.begin();iter!=m_CacheList.end();iter++)
			{
				CHdRasterBufferPtr pBuffer = (*iter);

				// 如果有存在影像名称相等的缓存
				if (pBuffer && pBuffer->GetRater())
				{
					int nRow(0),nCol(0);
					pBuffer->GetRater()->GetRowsCols(nRow,nCol);
					// 转换成G
                    float temp = static_cast<float>(nRow*nCol*pBuffer->GetRater()->GetBandNum());
					fMemory += temp / 1024 / 1024 / 1024;
				}
			}

			return fMemory;
		}


		// 检查缓存是否存在
		bool CHdRaterCache::CheckCacheExist(CHdRasterBuffer* pImageBuffer)
		{
			// 如果为空，返回false
			if (!pImageBuffer)
			{
				return false;
			}

			// 循环查找
			for (imageCacheIt iter = m_CacheList.begin();iter!=m_CacheList.end();iter++)
			{
				CHdRasterBufferPtr pBuffer = (*iter);
				// 如果有存在影像名称相等的缓存
				if (pBuffer && pBuffer->GetRater() 
					&& strcmp(pBuffer->GetStrImagePath().c_str(),pImageBuffer->GetStrImagePath().c_str()) == 0
					&& (pBuffer->GetBox().isContain(pImageBuffer->GetBox()))
					&& equals(pBuffer->GetScale(),pImageBuffer->GetScale()))
				{
					return true;
				}
			}

			return false;
		}

		// 得到某个影像缓存的数据
		// 得到某个影像缓存的数据
		CHdRasterBufferPtr CHdRaterCache::GetRaster(const char* strImageName,float dScale,const Chd2DBoundingBoxd& box)
		{
			// 循环查找
			for (imageCacheIt iter = m_CacheList.begin();iter!=m_CacheList.end();iter++)
			{
				CHdRasterBufferPtr pBuffer = (*iter);
				// 如果有存在影像名称相等的缓存
				if (pBuffer 
					&& strcmp(pBuffer->GetStrImagePath().c_str(),strImageName) == 0
					&& (pBuffer->GetBox().isContain(box))
					&& equals(pBuffer->GetScale(),dScale))
				{
					// 如果没有加载完
					while(!(*iter)->IsLoad())
					{
						ZThread::Thread::sleep(10);	
					}

					// 如果加载失败
					if ((*iter)->IsFailed())
					{
						return CHdRasterBufferPtr();
					}

					return *iter;
				}
			}

			return CHdRasterBufferPtr();
		}
	}
}
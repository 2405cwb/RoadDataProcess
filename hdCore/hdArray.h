#pragma once
#include "hdCore.h"
#include "hdDefs.h"
#include <vector>
using namespace std;

namespace hd
{
	// 二维数组
	template <class T>
	class hdArray//HDCORE_API
	{
	private:
		u32							m_countInBlock;		// 每个内存块存放点数
		u64							m_count;			// 总点数
		std::vector<std::vector<T>> m_pts;				// 内存块数组
	public:
		hdArray()
			:m_countInBlock(1000000),m_count(0){}
		~hdArray(){}

		inline u64 count() const {return m_count;}

		inline u32 countInBlock(){return m_countInBlock;}

		inline u32 blockCount(){return m_count / m_countInBlock;}

		inline void setCountInBlock(u32 nInBlock)
		{
			if (nInBlock != 0)
			{
				m_countInBlock = nInBlock;
			}
		}

		inline std::vector<T>& getBlock(unsigned int blockIndex)
		{
			return m_pts.at(blockIndex);
		}

		inline T& operator[](unsigned int index)
		{ 
			std::vector<T>& pts = m_pts.at(index / m_countInBlock);
			return pts.at(index % m_countInBlock);
		}

		inline const T& operator[](unsigned int index) const
		{ 
			const std::vector<T>& pts = m_pts.at(index / m_countInBlock); // 
			return pts.at(index % m_countInBlock);
		}

		inline void clear()
		{
			for (std::vector<std::vector<T>>::iterator it = m_pts.begin();
				it != m_pts.end();it++)
			{
				(*it).clear();
			}
			m_pts.clear();
			m_count = 0;
		}

		inline void push_back(const T& val)
		{
			int blkCountBefore = (int)(m_count / m_countInBlock + 
				((m_count % m_countInBlock == 0) ? 0 : 1));
			m_count++;
			int blkCountAfter = (int)(m_count / m_countInBlock + 
				((m_count % m_countInBlock == 0) ? 0 : 1));
			if (blkCountAfter > blkCountBefore)
			{
				m_pts.resize(blkCountAfter);
			}
			(m_pts.at(blkCountAfter - 1)).push_back(val);
		}

		inline T* data()
		{
			if(m_count == 0)
				return NULL;
			//return (*m_pts._Myfirst)._Myfirst;
			m_pts.at(0).at(0);
		}

		// 排序,注意这里只是实现分块内部排序
		inline void sort()
		{
			// 计算块数
			int blkCount = m_count / m_countInBlock + 
				((m_count % m_countInBlock == 0) ? 0 : 1);

			for (int i = 0;i < blkCount;i++)
			{
				std::vector<T>& pts = m_pts.at(i);
				std::sort(pts.begin(), pts.end(),greater<T>());
			}
		}
		// 设置点个数
		inline void resize(u64 count)
		{
			if (count == m_count || m_countInBlock == 0)
			{
				return;
			}
			m_count = count;
			// 计算块数
			int blkCount = (int)(m_count / m_countInBlock + 
				((m_count % m_countInBlock == 0) ? 0 : 1));
			// 计算第count在最后一个块所在序号
			int lastBlkCount = (m_count % m_countInBlock == 0) ? m_countInBlock : (m_count % m_countInBlock);
			m_pts.resize(blkCount);
			for (int i = 0;i < blkCount - 1;i++)
			{
				m_pts.at(i).resize(m_countInBlock);
			}
			m_pts.at(blkCount - 1).resize(lastBlkCount);
		}
	};
}
#pragma once
#include "hdCore.h"
#include "hdDefs.h"
#include "hdMath.h"
#include <vector>
#include <new>
// necessary for older compilers
#include <memory.h>
using namespace std;

namespace hd
{
	//! Sinks an element into the heap.
	template<class T>
	inline void heapsink(T*array, s32 element, s32 max)
	{
		while ((element<<1) < max) // there is a left child
		{
			s32 j = (element<<1);

			if (j+1 < max && array[j] > array[j+1])
				j = j+1; // take right child

			if (array[element] > array[j])
			{
				T t = array[j]; // swap elements
				array[j] = array[element];
				array[element] = t;
				element = j;
			}
			else
				return;
		}
	}


	//! Sorts an array with size 'size' using heapsort.
	template<class T>
	inline void heapsort(T* array_, s32 size)
	{
		// for heapsink we pretent this is not c++, where
		// arrays start with index 0. So we decrease the array pointer,
		// the maximum always +2 and the element always +1

		T* virtualArray = array_ - 1;
		s32 virtualSize = size + 2;
		s32 i;

		// build heap

		for (i=((size-1)/2); i>=0; --i)
			heapsink(virtualArray, i+1, virtualSize-1);

		// sort array, leave out the last element (0)
		for (i=size-1; i>0; --i)
		{
			T t = array_[0];
			array_[0] = array_[i];
			array_[i] = t;
			heapsink(virtualArray, 1, i + 1);
		}
	}

	template <class T>
	class hdVector
	{
	private:
		u32 m_count;
		bool m_sorted:1;
	public:
		T*	_Myfirst;

		hdVector()
			:_Myfirst(0),m_count(0),m_sorted(false){}
		~hdVector()
		{
			clear();
		}
		// 拷贝构造函数
		hdVector(const hdVector<T>& other) : _Myfirst(0)
		{
			*this = other;
		}

		//! 赋值函数
		const hdVector<T>& operator=(const hdVector<T>& other)
		{
			if (this == &other)
				return *this;
			
			if (_Myfirst)
				clear();

			m_count = other.m_count;
			m_sorted = other.m_sorted;
			u32 byteCount = m_count * sizeof(T);
			try
			{
				_Myfirst = (T*)(operator new(byteCount));
			}
			catch (...)
			{
				if (_Myfirst)
				{
					delete[] _Myfirst;
					_Myfirst = NULL;
				}
				
				return *this;
			}

			//_Myfirst = (T*)malloc(byteCount);
			memcpy(_Myfirst,other._Myfirst,byteCount);

			return *this;
		}

		inline void resize(u32 size)
		{
			if(size == 0 || size == m_count)
				return;
			
			if (_Myfirst)
			{
				T* oldData = _Myfirst;
				u32 byteCount = size * sizeof(T);
				_Myfirst = (T*)(operator new(byteCount));
				//_Myfirst = (T*)malloc(byteCount);
				memset(_Myfirst,0,byteCount);
				memcpy(_Myfirst,oldData,MIN(size,m_count) * sizeof(T));
				operator delete((void*)oldData);
				//free(oldData);
			}
			else
			{
				u32 byteCount = size * sizeof(T);
				_Myfirst = (T*)(operator new(byteCount));
				//_Myfirst = (T*)malloc(byteCount);
				memset(_Myfirst,0,byteCount);
			}
			m_count = size;
			m_sorted = false;
		}

		inline void clear()
		{
			if (_Myfirst)
			{
				// operator new 是申请的void* 指针,operator delete对应的需要delete void*
				operator delete((void*)_Myfirst);//delete[] _Myfirst;
				//free(_Myfirst);
				_Myfirst = NULL;
			}
			m_count = 0;
		}

		inline u32 size()const{return m_count;}

		inline void sort()
		{
			/*if (!m_sorted && m_count>1)
				heapsort(_Myfirst, m_count);
			m_sorted = true;*/
			if (m_count>1)
			{
				heapsort(_Myfirst, m_count);
			}
			m_sorted = true;//5.30.2014	by dongdai
		}

		inline T& operator [](u32 index)
		{
			_HD_DEBUG_BREAK_IF(index>=m_count) // access violation

			return _Myfirst[index];
		}

		inline const T& operator [](u32 index) const
		{
			_HD_DEBUG_BREAK_IF(index>=m_count) // access violation

			return _Myfirst[index];
		}
	};

	// 数据块数组
	template <class T>
	class hdBlkArray//HDCORE_API
	{
	private:
		u32							m_countInBlock;		// 块内点数
		u32							m_blkCount;			// 块数
		u32							m_curBlk;			// 当前读取的块
		u64							m_count;			// 总点数
		hdVector<hdVector<T>>		m_pts;				// 内存块数组
		hdVector<u64>				m_suffixs;			// 每块起始记录,在整个数组中的位置
		hdVector<s32>				m_loopIdx;			// 记录每块对应物理数据圈号
		hdVector<void*>				m_ppExtAttrs;		// 存储扩展属性
		//! 根据记录序号获取所在圈数据
		inline void getBlkIndex(u64 index) 
		{
			if (!(index >= (*(m_suffixs._Myfirst + m_curBlk)) &&
				index < (*(m_suffixs._Myfirst + m_curBlk + 1))))		
			{
				// 先判断是否落入下一个扫描圈,如果外部顺序读取,则肯定在下一个扫描圈
				if (m_curBlk < m_suffixs.size() - 2 && 
					index >= (*(m_suffixs._Myfirst + m_curBlk + 1)) &&
					index < (*(m_suffixs._Myfirst + m_curBlk + 2)))
				{
					m_curBlk++;
				}
				else
				{
					// 二分法查找落入哪个块
					u32 blkCount = m_blkCount;
					u32 low = 0,high = blkCount -1,middle;
					while(low <= high)
					{
						middle = (low + high)/2;
						if (*(m_suffixs._Myfirst  + middle) > index)
						{
							high = middle -1;
						}
						else if(*(m_suffixs._Myfirst  + middle) < index)
						{
							low = middle + 1;
						}
						else
						{
							// 正好相等情况
							low = middle + 1;
							break;
						}
					}

					m_curBlk = low -1;
				}
				
				while ((*(m_pts._Myfirst + m_curBlk)).size() == 0 && m_curBlk < m_blkCount)
				{
					m_curBlk++;
				}
			}
		}

		//! 根据记录序号获取所在圈数据,这里重载是为了实现const []() const
		inline void getBlkIndex(u64 index,u32& blkIndex) const
		{
			static u32 curIndex = m_curBlk;
			blkIndex = curIndex;
			if (!(index >= (*(m_suffixs._Myfirst + blkIndex)) &&
				index < (*(m_suffixs._Myfirst + blkIndex + 1))))		
			{
				// 先判断是否落入下一个扫描圈,如果外部顺序读取,则肯定在下一个扫描圈
				if (blkIndex < m_suffixs.size() - 2 && 
					index >= (*(m_suffixs._Myfirst + blkIndex + 1)) &&
					index < (*(m_suffixs._Myfirst + blkIndex + 2)))
				{
					curIndex++;
				}
				else
				{
					// 二分法查找落入哪个扫描圈
					u32 blkCount = m_blkCount;
					u32 low = 0,high = blkCount -1,middle;
					while(low <= high)
					{
						middle = (low + high)/2;
						if (*(m_suffixs._Myfirst  + middle) > index)
						{
							high = middle -1;
						}
						else if(*(m_suffixs._Myfirst  + middle) < index)
						{
							low = middle + 1;
						}
						else
						{
							// 正好相等情况
							low = middle + 1;
							break;
						}
					}

					curIndex = low -1;	
				}
				
				while ((*(m_pts._Myfirst + curIndex)).size() == 0 && curIndex < m_blkCount)
				{
					curIndex++;
				}
			}
			blkIndex = curIndex;
		}

	public:
		hdBlkArray()
			:m_countInBlock(1000000),m_blkCount(0),m_count(0),m_curBlk(0){}
		~hdBlkArray(){clear();}
		
		//! 添加扩展属性
		template<class A>
		inline void addAttrs()
		{
			if(m_blkCount == 0)
				return;
			delAttrs();
			m_ppExtAttrs.resize(m_blkCount);
			u32 byteCount;
			for (u32 i = 0;i<m_blkCount;i++)
			{
				const hdVector<T>& blk = *(m_pts._Myfirst + i);
				if(blk.size() > 0)
				{
					byteCount = blk.size() * sizeof(A);
					*(m_ppExtAttrs._Myfirst + i) = operator new(byteCount);
					memset(*(m_ppExtAttrs._Myfirst + i),0,byteCount);
				}
			}
		}
		//! 删除扩展属性
		inline void delAttrs()
		{
			if (m_ppExtAttrs.size() > 0)
			{
				for (u32 i = 0;i<m_blkCount;i++)
				{
					void*& pAttr = *(m_ppExtAttrs._Myfirst + i);
					if(pAttr)
					{	
						operator delete(pAttr);
						pAttr = NULL;
					}
				}
				m_ppExtAttrs.clear();	
			}
		}
		// 是否包含扩展属性
		inline bool hasAttr(){return m_ppExtAttrs.size() != 0;}
		//! 获取块扩展属性
		template<class A>
		inline A* getBlockAttr(u32 blockIndex,u32& size)
		{
			if(blockIndex >= m_ppExtAttrs.size())
				return NULL;
			const hdVector<T>& blk = *(m_pts._Myfirst + blockIndex);
			size = blk.size();
			return (A*)(*(m_ppExtAttrs._Myfirst + blockIndex));
		}
		//! 获取扩展属性
		template<class A>
		inline A& getAttr(u64 index)
		{
			u32 blkIndex = 0;
			getBlkIndex(index,blkIndex);
			A* blkAttrs = (A*)(*(m_ppExtAttrs._Myfirst + blkIndex));
			u64 startGlobalIndex = *(m_suffixs._Myfirst + blkIndex);
			return blkAttrs[index - startGlobalIndex];
		}

		//! 总点数
		inline u64 count() const {return m_count;}
		//! 块数
		inline u32 blockCount(){return m_blkCount;}
		//! 设置块数
		inline void setBlockCount(u32 blkCount)
		{
			if(blkCount == 0)
				return;
			m_suffixs.resize(blkCount);
			m_pts.resize(blkCount);
			m_loopIdx.resize(blkCount);
			m_blkCount = blkCount;
		}
		//! 获取块对应物理数据圈号
		inline s32 getLoopIdx(u32 blockIndex)
		{
			if(blockIndex >= m_blkCount)
				return -1;

			return *(m_loopIdx._Myfirst + blockIndex);
		}
		//! 设置块对应物理数据圈号
		inline void setLoopIdx(u32 blockIndex,s32 loopIdx)
		{
			if(blockIndex >= m_blkCount)
				return ;

			*(m_loopIdx._Myfirst + blockIndex) = loopIdx;
		}
		//! 获取块
		inline bool getBlock(u32 blockIndex,hdVector<T>** ary)
		{
			if(blockIndex >= m_blkCount)
				return false;
			(*ary) = (m_pts._Myfirst + blockIndex);
			return true;
		}
		//! 获取块
		inline hdVector<T>& getBlock(u32 blockIndex)
		{
			return *(m_pts._Myfirst + blockIndex);
		}
		//! 设置块元素大小
		inline void resizeBlock(u32 blockIndex,u32 count)
		{
			return (*(m_pts._Myfirst + blockIndex)).resize(count);
		}
		//! 清理块
		inline void clearBlock(u32 blockIndex)
		{
			if (blockIndex >= m_blkCount)
			{
				return;
			}
			(*(m_pts._Myfirst + blockIndex)).clear();
			//std::vector<T>().swap(*(m_pts._Myfirst + blockIndex));
		}
		//! 更新块内部点下标和总点数
		inline void update()
		{
			m_count = 0;
			for (u32 i = 0;i<m_blkCount;i++)
			{
				const hdVector<T>& blk = *(m_pts._Myfirst + i);
				*(m_suffixs._Myfirst + i) = m_count;
				m_count += blk.size();
			}
		}
		//! 根据总的下标获取记录
		inline T& operator[](u64 index)
		{ 
			getBlkIndex(index);
			hdVector<T>& blk = *(m_pts._Myfirst + m_curBlk);
			u64 startGlobalIndex = *(m_suffixs._Myfirst + m_curBlk);
			return *(blk._Myfirst + index - startGlobalIndex);
		}

		inline const T& operator[](u64 index) const
		{ 
			u32 blkIndex = 0;
			getBlkIndex(index,blkIndex);
			hdVector<T>& blk = *(m_pts._Myfirst + blkIndex);
			u64 startGlobalIndex = *(m_suffixs._Myfirst + blkIndex);
			return *(blk._Myfirst + index - startGlobalIndex);
		}

		inline void clear()
		{
			delAttrs();
			m_suffixs.clear();
			m_loopIdx.clear();

			for (u32 i = 0;i<m_blkCount;i++)
			{
				if (!(m_pts._Myfirst + i))
				{
					continue;
				}

				hdVector<T>& pts = *(m_pts._Myfirst + i);
				pts.clear();
			}
			m_pts.clear();

			m_count = 0;
			m_blkCount = 0;
			m_curBlk = 0;
		}

		// 此处只返回第一块的数据指针,适合于单块情况使用
		inline T* data()
		{
			if(m_count == 0)
				return NULL;
			return (*m_pts._Myfirst)._Myfirst;
		}

		// 排序,注意这里只是实现分块内部排序
		inline void sort()
		{
			for (u32 i = 0;i < m_blkCount;i++)
			{
				hdVector<T>& pts = *(m_pts._Myfirst + i);
				pts.sort();
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
			m_blkCount = (u32)(m_count / m_countInBlock + 
				((m_count % m_countInBlock == 0) ? 0 : 1));
			// 计算第count在最后一个块所在序号
			u32 lastBlkCount = (m_count % m_countInBlock == 0) ? m_countInBlock : (m_count % m_countInBlock);
			//m_pts.resize(m_blkCount);
			setBlockCount(m_blkCount);
			for (u32 i = 0;i < m_blkCount - 1;i++)
			{
				(*(m_pts._Myfirst + i)).resize(m_countInBlock);
			}
			(*(m_pts._Myfirst + m_blkCount - 1)).resize(lastBlkCount);
			update();
		}
	};
}
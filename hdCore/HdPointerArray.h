/*! HdPointerArray.h
********************************************************************************
<PRE>
模块名       : hdCore
文件名       : HdPointerArray.h
相关文件     : 
文件实现功能 : 定义一个简单的指针数组类，对 STL 的 vector 进行封装
               用于管理指针类型的数据，指针的释放一律由内部负责，且指针必须是使用 new 函数创建而来
作者         : 朱立雄
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2017/1/12   1.0      朱立雄                创建
</PRE>
*******************************************************************************/
#ifndef HDCORE_HDPOINTERARRAY_H
#define HDCORE_HDPOINTERARRAY_H

#include <vector>

namespace hd
{
	template < class T >
	class CHdPointerArray
	{
	public:
		CHdPointerArray(){}
		~CHdPointerArray()
		{
			DeleteAll();
		}
	private:
		CHdPointerArray(const CHdPointerArray&);
		CHdPointerArray& operator= (const CHdPointerArray&);

	public:

		// 获取元素个数
		unsigned int GetCount() const
		{
			return m_vpElem.size();
		}

		// 获取元素（外部进行指针越界判断，避免重复判断）
		T* GetAt(unsigned int nIndex) const
		{
			//if (nIndex >= 0 && nIndex < m_vpElem.size())
			//{
				return m_vpElem[nIndex];
			//}
			//return NULL;
		}

		// 添加一个元素
		void Add(T* pElem)
		{
			m_vpElem.push_back(pElem);
		}

		// 插入一个元素
		void Insert(unsigned int nIndex, T* pElem)
		{
			//if (nIndex > m_vpElem.size())
			//{
			//	nIndex = m_vpElem.size();
			//}
			m_vpElem.insert(m_vpElem.begin() + nIndex, pElem);
		}

		// 删除一个元素（外部进行指针越界判断，避免重复判断）
		void Delete(unsigned int nIndex)
		{
			//if (nIndex >= 0 && nIndex < m_vpElem.size())
			//{
				std::vector< T* >::iterator iter = m_vpElem.begin() + nIndex;
				delete *iter;
				m_vpElem.erase(iter);
			//}
		}

		// 删除所有元素
		void DeleteAll()
		{
			std::vector< T* >::iterator iter = m_vpElem.begin();
			std::vector< T* >::iterator iterEnd = m_vpElem.end();
			while (iter != iterEnd)
			{
				delete *iter;
				++ iter;
			}
			m_vpElem.clear();
		}

	private:

		std::vector< T* > m_vpElem;
	};
}

#endif
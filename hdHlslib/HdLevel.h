/*! HdLevel.h
********************************************************************************
<PRE>
模块名       : hdHLSLib
文件名       : HdLevel.h
相关文件     : 
文件实现功能 : hlz点云层管理类
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              创建
2015/01/22   1.0      龚书林    
</PRE>
*******************************************************************************/

#pragma once
#include <map>
#include "mydefs.hpp"
#include "HlzDefs.h"

using namespace std;

namespace hd
{
	struct HdLevel;
	class CHdBlockset;

	class HLS_API CHdLevel
	{
	public:
		// 层对象
		HdLevel			      m_level;
		// 块集列表
		map<I32,CHdBlockset*> m_pListBlockset;
		
		// 层编号
		U16					  m_levelNo;
	public:
		CHdLevel(void);
		~CHdLevel(void);

		// 更新内部点数
		virtual void Update();

		// 更据单位面积点数,更新计算比例尺
		void UpdateScale();

		// 获取点数
		virtual u64  GetPtCount();

		// 获取块集数
		int GetBlockSetCount(){return m_pListBlockset.size();}
		// 清除对象
		void Clear();
		// 添加块集记录
		BOOL AddBlockSetRec(CHdBlockset* pBlockSetRec);
		// 添加块集记录
		BOOL AddBlockSetRec(I32 nBlockSetNo,CHdBlockset* pBlockSetRec);
		// 获取块集记录
		CHdBlockset* GetBlockSetRec(I32 nBlockSetNo);
		// 
		// 获取当前层显示比例尺范围
		BOOL GetScaleRange(F64& dMinScale,F64& dMaxScale);

		// 获取一层的数据范围
		CHdBox3df GetExtent();

		// 重载并私有化拷贝构造函数和赋值操作符,避免赋值拷贝
	private:
		CHdLevel(const CHdLevel& other){}
		CHdLevel& operator=(const CHdLevel& other) { return *this;}

		// 重载比较操作符
		bool operator < (const CHdLevel& other) const
		{
			return m_levelNo < other.m_levelNo;
		}
	};
}


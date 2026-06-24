/*! HdBlock.h
********************************************************************************
<PRE>
模块名       : hdHLSLib
文件名       : HdParcelBase.h
相关文件     : 
文件实现功能 : hlz点云块管理类
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              创建
2015/03/13   1.0      龚书林    
</PRE>
*******************************************************************************/

#pragma once
#include <map>
#include "inc\mydefs.hpp"
#include "HlzDefs.h"
using namespace std;

namespace hd
{
	struct HlzPoint;
class HLS_API CHdParcelBase
{
public:

	// 记录指向父节点的指针
	CHdParcelBase* m_pParent;

	// 当前块的点云,解析后内存点云
	HlzPoint*	m_pHlzPoint;
	F32 m_distance; //视点到中心点的距离
    int m_SimpleInval;// 抽希间隔

protected:
	//! 内存记录包的外包围盒，为相对于头文件偏移量的相对坐标系
	CHdBox3df m_box;

	//only for hlz 3.1
	//! 空间实体所在的块集绝对坐标包围盒，用于解析坐标
	CHdBox3dd m_blockset_box;

public:
	CHdParcelBase(void);
	virtual ~CHdParcelBase(void);
	
    // 获取点个数
	virtual int GetPtCount() = 0;
	
    // 获取坐标数据地址
	virtual HdAddr GetCoordAddr() const = 0;

	// 外部设置坐标数据地址
	virtual void SetCoordAddr(U64 addrs) = 0;

	// 外部设置强度数据地址
	virtual void SetIntensityAddr(U64 addrs) = 0;

	// 外部设置颜色数据地址   袁亮  20160625
	virtual void SetColorAddr(U64 addrs) = 0;
	
    // 获取强度数据地址
	virtual HdAddr GetIntensityAddr() = 0;
	
    // 获取时间数据地址
	virtual HdAddr GetTimeAddr() = 0;
	
    // 获取颜色数据地址
    virtual HdAddr GetColorAddr() = 0;
	
    // 获取分类数据地址
	virtual HdAddr GetClassAddr() = 0;

	//判断数据是否压缩
	virtual bool IsCompress() = 0;

	//获取压缩后坐标长度
	virtual U32 GetCmpCoordLen() = 0;

	//only for hlz 3.1 
	//获取压缩后强度长度
	virtual U32 GetCmpIntenLen() = 0;

    // 释放数据
    void Release();

	bool operator<( const CHdParcelBase& other)  const
	{
		return GetCoordAddr().GetAddress()<other.GetCoordAddr().GetAddress();
	}

	CHdBox3df GetExtent()
	{
		return m_box;
	}

	void  UpdateExtent(const CHdBox3df& extent)
	{
		m_box = extent;
	}

	//only for hlz 3.1
	void SetBlocksetExtent(const CHdBox3dd& blockset_extent)
	{
		m_blockset_box = blockset_extent;
	}

	//only for hlz 3.1
	CHdBox3dd GetBlocksetExtent()
	{
		return m_blockset_box;
	}
};

}
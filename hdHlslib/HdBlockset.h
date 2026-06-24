/*! HdBlockset.h
********************************************************************************
<PRE>
模块名       : hdHLSLib
文件名       : HdBlockset.h
相关文件     : 
文件实现功能 : hlz点云块集管理类
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
#include "HlzDefs31.h"
#include "HdParcelBase.h"
using namespace std;

namespace hd
{

struct HlzPoint;
struct HdBlockset;
class CHdBlock;

// 块集类
class HLS_API CHdBlockset
	:public CHdParcelBase
{
public:
	// 所在层编号
	U16         m_levelNo;
	// 块集序号
	U32         m_nBlockSetNo;
	// 块集左下角点及大小
	//HdRefPoint  m_refPoint;
	// 块集索引对象
	HdBlockset  m_blockSet;

	/* 当前块集包含的块列表,数组元素顺序:x从左到右,y从下到上,z从低到高.
	// 无效的块,也占用空间,便于直接根据xyz方向编号直接获取CHdBlock
	*/
	map<I32,CHdBlock*> m_pListBlock;
	
	// 重载并私有化拷贝构造函数和赋值操作符,避免赋值拷贝
private:
	CHdBlockset(const CHdBlockset& other){}
	CHdBlockset& operator=(const CHdBlockset& other) { return *this;}
public:
	CHdBlockset()
		:CHdParcelBase(),m_levelNo(0xffff),m_nBlockSetNo(0xffff)//:m_xNo(0),m_yNo(0),m_zNo(0)
	{
	}
	~CHdBlockset();	

	// 更新内部点数
	virtual void Update();

	// 清除对象
	void Clear();

	// 添加块集记录
	BOOL AddBlockRec(CHdBlock* pBlockRec);
	// 获取块集记录
	CHdBlock* GetBlockRec(I32 nBlockNo);

	// 根据xyz网格编号获取绝对序号
	I32 GetIndex();
	
	// 添加块记录
	
	// 重载
public:
	virtual int GetPtCount();

	virtual HdAddr GetCoordAddr() const;

	virtual HdAddr GetIntensityAddr();

	virtual HdAddr GetTimeAddr();

	virtual HdAddr GetColorAddr();

	virtual HdAddr GetClassAddr();

	virtual bool IsCompress();

	//云存储中点云数据均未压缩，上传索引文件到云端前，需要调用此接口取消空间实体的压缩标记
	//除此之外，禁用此接口；
	virtual void SetUnCompress();

	virtual U32 GetCmpCoordLen();

	virtual U32 GetCmpIntenLen();

	// 外部设置坐标数据地址
	virtual void SetCoordAddr(U64 addrs);

	// 外部设置强度数据地址
	virtual void SetIntensityAddr(U64 addrs);

	// 外部设置颜色数据地址   袁亮  20160625
	virtual void SetColorAddr(U64 addrs);
};

}
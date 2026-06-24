/*! HdBlock.h
********************************************************************************
<PRE>
模块名       : hdHLSLib
文件名       : HdBlock.h
相关文件     : 
文件实现功能 : hlz点云块管理类
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
#include "HdParcelBase.h"
using namespace std;

namespace hd
{
	struct HlzPoint;
	struct HdBlock;
	class CHdParcel;

	// 块类
	class HLS_API CHdBlock
		:public CHdParcelBase
	{
	public:
		// 所在块集编号
		U32         m_bsNo;
		// 块序号
		U16         m_nBlockNo;
		// 块左下角点及大小
		//HdRefPoint  m_refPoint;
		// 块对象
		HdBlock     m_block;
		/* 当前块包含的包列表,数组元素顺序:x从左到右,y从下到上,z从低到高.
	    // 无效的块,也占用空间,便于直接根据xyz方向编号直接获取CHdBlock
	    */
		map<I32,CHdParcel*> m_pListParcel;
		
		// 重载并私有化拷贝构造函数和赋值操作符,避免赋值拷贝
	private:
		CHdBlock(const CHdBlock& other){}
		CHdBlock& operator=(const CHdBlock& other) { return *this;}

	public:
		CHdBlock()
			:CHdParcelBase(),m_bsNo(0xffff),m_nBlockNo(0xffff)//:m_xNo(0),m_yNo(0),m_zNo(0)
		{			
		}
		~CHdBlock();
		// 更新内部点数
		virtual void Update();

		// 清除对象
		void Clear();
		// 根据当前块xyz网格编号获取绝对序号
		I32 GetIndex();
		// 添加包记录
		BOOL AddParcel(CHdParcel* pParcel);
		// 获取包记录
		CHdParcel* GetParcel(I32 nParcelNo);

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

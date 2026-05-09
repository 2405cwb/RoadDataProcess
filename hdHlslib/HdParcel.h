/*! HlzDefs.h
********************************************************************************
<PRE>
模块名       : hdHLSLib
文件名       : HdParcel.h
相关文件     : 
文件实现功能 : hlz点云数据包管理类
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
#include "mydefs.hpp"
#include "HlzDefs.h"
#include "HdParcelBase.h"

namespace hd
{
	struct HlzPoint;
	struct HdParcel;
	// 包类
	class HLS_API CHdParcel
		:public CHdParcelBase
	{
	public:
		// 所在块编号
		U16         m_bkNo;
		// 块序号
		U16         m_nParcelNo;
		// 包对象
		HdParcel    m_parcel;
		
		// 重载并私有化拷贝构造函数和赋值操作符,避免赋值拷贝
	private:
		CHdParcel(const CHdParcel& other){}
		CHdParcel& operator=(const CHdParcel& other) { return *this;}

	public:
		CHdParcel()
			:CHdParcelBase(),m_bkNo(0xffff),m_nParcelNo(0xffff)//:m_xNo(0),m_yNo(0),m_zNo(0)
		{
		}
		~CHdParcel();
		
		// 清除对象
		void Clear();
		// 根据当前包xyz网格编号获取绝对序号
		I32 GetIndex(){ return m_nParcelNo;}

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
#pragma once
#include "mydefs.hpp"
#include "HlzDefs31.h"
#include "HdParcel.h"

namespace hd
{
	class  HLS_API CHdParcel31 :
		public CHdParcel
	{
	public:
		CHdParcel31(void)
			:CHdParcel()
		{
		}

		~CHdParcel31(void)
		{
		}

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

		// 重载并私有化拷贝构造函数和赋值操作符,避免赋值拷贝
	private:
		CHdParcel31(const CHdParcel31& other){}
		CHdParcel31& operator=(const CHdParcel31& other) { return *this;}

	public:
		// hlz3.1 包对象
		HdParcel31  m_parcel31;
	};
}


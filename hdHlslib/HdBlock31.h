#pragma once
#include <map>
#include "mydefs.hpp"
#include "HlzDefs31.h"
#include "HdBlock.h"
using namespace std;

namespace hd
{

	class HLS_API CHdBlock31 :
		public CHdBlock
	{
	public:
		CHdBlock31(void)
			:CHdBlock()
		{
		}

		~CHdBlock31(void);

		// 重载
	public:
		// 更新内部点数
		virtual void Update();

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
		CHdBlock31(const CHdBlock31& other){}
		CHdBlock31& operator=(const CHdBlock31& other) { return *this;}

	public:
		// hlz 3.1 块对象
		HdBlock31     m_block31;
	};
}

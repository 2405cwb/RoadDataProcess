/*!@HdSvTileInfoBuffer
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：HdSvTileInfoBuffer.h
相关文件	: hdCommon.h、hdHdiStruct.h
文件实现功能：封装切片互斥加锁类
作者		：朱旭波
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2016/5/04   1.0			朱旭波		创建并实现,添加互斥锁对象
</PRE>
******************************************************************************************************/
#pragma once
#include "hdCommon.h"
#include "hdSvDBStructDef.h"

namespace hd
{
	class HDCOMMON_API CHdSvTileInfoBuffer
	{
	public:
		// 构造
		CHdSvTileInfoBuffer(const char* strTileID);

		// 析构
		~CHdSvTileInfoBuffer(void);

		// new 内存块并拷贝
		void AllocSize(BYTE* data,long nBytes);

		// 取消线程获取操作
		void cancel();

		// 设置结束
		void setFinished();

		bool isCanceled();

		bool isFinished();

		// 获取存储切片对象，若正在使用，则返回空
		HD_SV_TILEINFO* GetTileInfo();

		// 获得切片ID
		const char* GetTileID();

		// 获得切片二进制数据长度值
		long GetSize();
	private:
		// 存储切片对象
		HD_SV_TILEINFO* m_pInfo;

		//// 互斥变量
		CRITICAL_SECTION m_cs; 

		// 标记取消，操作是否完成
		bool m_bCancel;

		// 标记内存处理完成
		bool m_bFinished;
	};
}



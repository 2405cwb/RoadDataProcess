/*!@HdSvTileInfoBuffer
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：HdSvTileInfoBuffer.cpp
相关文件	: hdCommon.h、hdHdiStruct.h
文件实现功能：封装切片互斥加锁类
作者		：朱旭波
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2016/5/04   1.0			朱旭波		创建并实现，目前未添加互斥锁对象
</PRE>
******************************************************************************************************/
#include "StdAfx.h"
#include "HdSvTileInfoBuffer.h"

namespace hd
{
	// 构造在主线程中进行，不存在加锁问题
	CHdSvTileInfoBuffer::CHdSvTileInfoBuffer(const char* strTileID)
	{
		// 代码锁
		InitializeCriticalSection(&m_cs);

		// new参数
		m_pInfo = new HD_SV_TILEINFO;

		// 赋值ID
		strcpy_s(m_pInfo->strTileID,strTileID);
		m_bCancel = false;
		m_bFinished = false;
	}

	// 析构
	CHdSvTileInfoBuffer::~CHdSvTileInfoBuffer(void)
	{
		DeleteCriticalSection(&m_cs);

		// 内存释放
		if (m_pInfo)
		{
			delete m_pInfo;
			m_pInfo = NULL;
		}

		m_bCancel = true;
	}

	// new 内存块
	void CHdSvTileInfoBuffer::AllocSize(BYTE* data, long nBytes )
	{
		//// 互斥量
		EnterCriticalSection(&m_cs);


		// 在拷贝之前要确保操作未取消
		if (!m_bCancel)
		{
			if (m_pInfo && m_pInfo->nSize <= 0)
			{
				// new内存
				m_pInfo->AllocSize(nBytes);
				memcpy(m_pInfo->pTileData,data,(long)nBytes);
			}
		}
		
		// 标记操作完成
		m_bCancel = true;
		m_bFinished = true;

		LeaveCriticalSection(&m_cs);
	}

	// 取消线程获取操作
	void CHdSvTileInfoBuffer::cancel()
	{
		// 取消操作
		EnterCriticalSection(&m_cs);
		m_bCancel = true;
		LeaveCriticalSection(&m_cs);
	}

	// 获取存储切片对象
	HD_SV_TILEINFO* CHdSvTileInfoBuffer::GetTileInfo()
	{
		// 互斥量
		return m_pInfo;
	}

	// 获得切片ID
	const char* CHdSvTileInfoBuffer::GetTileID()
	{
		return m_pInfo->strTileID;
	}

	// 获得切片二进制数据长度值
	long CHdSvTileInfoBuffer::GetSize()
	{
		EnterCriticalSection(&m_cs);
		long nSize = m_pInfo->nSize;
		LeaveCriticalSection(&m_cs);

		return nSize;
	}

	// 取消是否完成
	bool CHdSvTileInfoBuffer::isCanceled()
	{
		EnterCriticalSection(&m_cs);
		bool bCancel = m_bCancel;
		LeaveCriticalSection(&m_cs);

		return bCancel;
	}

	// 外部获取操作是否完成，未完成时该内存不能释放
	bool CHdSvTileInfoBuffer::isFinished()
	{
		EnterCriticalSection(&m_cs);
		bool bFinish = m_bFinished;
		LeaveCriticalSection(&m_cs);

		return bFinish;
	}

	// 设置结束
	void CHdSvTileInfoBuffer::setFinished()
	{
		EnterCriticalSection(&m_cs);
		m_bCancel = true;
		m_bFinished = true;
		LeaveCriticalSection(&m_cs);
	}

}


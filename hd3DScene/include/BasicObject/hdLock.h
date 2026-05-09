/*! hdLog.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : hdLock.h
相关文件     : hdLog.h
文件实现功能 : 快速lock类，对需要互斥的资源进行保护，只适用于本线程加锁、解锁（贾辉）
作者         : 马振明
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/12/24   1.0      马振明				新建
</PRE>
*******************************************************************************/

#pragma once
#include <Windows.h>
#include <string>
#include <vector>


class BASICOBJECT_API CFastLock
{
public:
	CFastLock()
	{
		InitializeCriticalSection(&m_criticalSection);
	}

	~CFastLock()
	{
		DeleteCriticalSection(&m_criticalSection);
	}

	// 加锁
	void Lock()
	{
		EnterCriticalSection(&m_criticalSection);
	}

	// 解锁
	void Unlock()
	{
		LeaveCriticalSection(&m_criticalSection);
	}

protected:
	CRITICAL_SECTION m_criticalSection;
};

// 
class CLocker
{
public:
	CLocker(CFastLock *pLock)
		: m_pLock(pLock)
	{
		m_pLock->Lock();
	}

	~CLocker()
	{
		m_pLock->Unlock();
	}

private:
	CFastLock *m_pLock;
};



#include "SimpleLog.h"
#include <sstream>
#include <iomanip>

CSimpleLog::CSimpleLog()
	: m_fp(NULL)
{
	InitializeCriticalSection(&m_lock);
}

CSimpleLog::~CSimpleLog()
{
	Close();
	DeleteCriticalSection(&m_lock);
}

bool CSimpleLog::Open(const char* path)
{
	Close();
	fopen_s(&m_fp, path, "wb");
	return m_fp != NULL;
}

void CSimpleLog::Close()
{
	EnterCriticalSection(&m_lock);
	if (m_fp != NULL)
	{
		fflush(m_fp);
		fclose(m_fp);
		m_fp = NULL;
	}
	LeaveCriticalSection(&m_lock);
}

void CSimpleLog::Flush()
{
	EnterCriticalSection(&m_lock);
	if (m_fp != NULL)
	{
		fflush(m_fp);
	}
	LeaveCriticalSection(&m_lock);
}

void CSimpleLog::PrintLine(const std::string& text)
{
	PrintInternal(text, true);
}

void CSimpleLog::PrintLineNoFlush(const std::string& text)
{
	PrintInternal(text, false);
}

void CSimpleLog::PrintInternal(const std::string& text, bool flushNow)
{
	EnterCriticalSection(&m_lock);
	if (m_fp != NULL)
	{
		std::string line = GetTimePrefix() + text + "\r\n";
		fwrite(line.c_str(), 1, line.size(), m_fp);
		if (flushNow)
		{
			fflush(m_fp);
		}
	}
	LeaveCriticalSection(&m_lock);
}

std::string CSimpleLog::GetTimePrefix()
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	char buf[64];
	sprintf_s(buf,
		"[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond,
		st.wMilliseconds);
	return std::string(buf);
}

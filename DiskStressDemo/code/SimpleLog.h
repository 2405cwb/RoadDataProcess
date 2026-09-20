#pragma once

#include <windows.h>
#include <stdio.h>
#include <string>

class CSimpleLog
{
public:
	CSimpleLog();
	~CSimpleLog();

	bool Open(const char* path);
	void Close();
	void Flush();
	void PrintLine(const std::string& text);
	void PrintLineNoFlush(const std::string& text);

private:
	void PrintInternal(const std::string& text, bool flushNow);
	std::string GetTimePrefix();

private:
	FILE* m_fp;
	CRITICAL_SECTION m_lock;
};

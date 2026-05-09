/*! hdLog.cpp
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : hdLog.cpp
相关文件     : hdLock.h
文件实现功能 : 打印日志文件类
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

#include "stdafx.h"
#include "hdLog.h"
#include <stdio.h>

namespace hd
{

#define  HY_LOGGER_MAXFILESIZE   10*1024*1024 //10M
#define  HY_LOGGER_MAXFILECOUNT  100
#define  HY_LOGGER_PATTERN       "[%d{%y-%m-%d %H:%M:%S}][%p][%t]%m%n"//"Gaya[%p] %d{%y-%m-%d %H:%M:%S}:%m [code:%l]%n" 
#define  HY_LOGGER_LEVEL         ALL_LOG_LEVEL

	//日志级别名称
#define  HY_LOGGER_TRACE_NAME             " TRACE: "
#define  HY_LOGGER_INFO_NAME              " INFO: "
#define  HY_LOGGER_DEBUG_NAME             " DEBUG: "
#define  HY_LOGGER_WARN_NAME              " WARN: "
#define  HY_LOGGER_ERROR_NAME             " ERROR: "
#define  HY_LOGGER_FATAL_NAME             " FATAL: "
#define  HY_LOGGER_UNKNOWN_NAME           " UNKNOWN: "
#define  HY_LOGGER_LINE_SEPARATOR         "\r\n"


	CHdLog* CHdLog::m_log = NULL;
	CHdLog::CHdLog(void)   
	{
		m_pf = NULL;
		m_LogLevel = logLevelAll;
		InitPath();
		// 定义一个静态对象，程序退出时自动调用hdSettingCleaner析构销毁CHdSysSetting
		static hdLogCleaner mCleaner;
	}   

	CHdLog::~CHdLog()   
	{   
		if (m_pf)   
			fclose(m_pf); 
	}

	//// 创建入口
	//CLog* CLog::GetInstance()
	//{
	//	if (NULL == m_log)
	//	{
	//		m_log = new CLog;
	//	}

	//	return m_log;
	//}

	//// 销毁入口
	//void CLog::destroyInstance()
	//{
	//	if (m_log)
	//	{
	//		delete m_log;
	//		m_log = NULL;
	//	}
	//}

	void CHdLog::InitPath()
	{
			string strPath = getCurrentDir();
			std::string strSystemTime = "" ;   
			char chTmp[20] = "";   

			SYSTEMTIME temp;   
			GetLocalTime(&temp);
			sprintf(chTmp, "%04d-%02d-%02d %02d %02d %02d", temp.wYear, temp.wMonth, temp.wDay, temp.wHour, temp.wMinute, temp.wSecond);
			strSystemTime = (char *)chTmp;
			m_strPath = strPath + strSystemTime;
			m_strPath += ".log";
			m_nWriteSize = 0;
	}

	// 获取日期和时间
	std::string CHdLog::getDateTime()   
	{   
		std::string strSystemTime = "" ;   
		char chTmp[20] = "";   

		SYSTEMTIME temp;   
		GetLocalTime(&temp);
		sprintf(chTmp, "%04d-%02d-%02d %02d %02d %02d", temp.wYear, temp.wMonth, temp.wDay, temp.wHour, temp.wMinute, temp.wSecond);
		strSystemTime = (char *)chTmp;

		return strSystemTime;   
	}

	void CHdLog::setLevel(enumLogLevel level)   
	{
		m_LogLevel = level;
	}

	enumLogLevel CHdLog::getLevel()
	{
		return m_LogLevel;
	}

	std::string CHdLog::formatLog(std::string level, const char *fmt, va_list argList)
	{
		_vsnprintf((char*)&m_strBuf, TBUF_SIZE, fmt, argList);
		std::string strTime = getDateTime();
		std::string strBuf = m_strBuf;
		return strTime + level + strBuf;
	}

	void CHdLog::logTrace(const char *fmt,...)
	{
		CLocker locker(&m_lock);

		if (canTrace(logLevelTrace))
		{
			va_list argList;
			va_start(argList, fmt);
			std::string strBuf = formatLog(HY_LOGGER_TRACE_NAME, fmt, argList);
			va_end(argList);
			writeLog(strBuf.c_str(), strBuf.size());
		}
	}

	void CHdLog::logInfo(const char *fmt,...)
	{
		CLocker locker(&m_lock);

		if (canTrace(logLevelInfo))
		{
			va_list argList;
			va_start(argList, fmt);
			std::string strBuf = formatLog(HY_LOGGER_INFO_NAME, fmt, argList);
			va_end(argList);	
			writeLog(strBuf.c_str(), strBuf.size());
		}
	}

	void CHdLog::logDebug(const char *fmt,...)
	{
		CLocker locker(&m_lock);

		if (canTrace(logLevelDebug))
		{
			va_list argList;
			va_start(argList, fmt);
			std::string strBuf = formatLog(HY_LOGGER_DEBUG_NAME, fmt, argList);
			va_end(argList);	
			writeLog(strBuf.c_str(), strBuf.size());
		}
	}

	void CHdLog::logWarn(const char *fmt,...)
	{
		CLocker locker(&m_lock);

		if (canTrace(logLevelWarn))
		{
			va_list argList;
			va_start(argList, fmt);
			std::string strBuf = formatLog(HY_LOGGER_WARN_NAME, fmt, argList);
			va_end(argList);
			writeLog(strBuf.c_str(), strBuf.size());
		}
	}

	void CHdLog::logError(const char *fmt,...)
	{
		CLocker locker(&m_lock);

		if (canTrace(logLevelError))
		{
			va_list argList;
			va_start(argList, fmt);
			std::string strBuf = formatLog(HY_LOGGER_ERROR_NAME, fmt, argList);
			va_end(argList);
			writeLog(strBuf.c_str(), strBuf.size());
		}
	}

	void CHdLog::logFatal(const char *fmt,...)
	{
		CLocker locker(&m_lock);

		if (canTrace(logLevelFatal))
		{
			va_list argList;
			va_start(argList, fmt);
			std::string strBuf = formatLog(HY_LOGGER_FATAL_NAME, fmt, argList);
			va_end(argList);
			writeLog(strBuf.c_str(), strBuf.size());
		}
	}

	bool CHdLog::canTrace(unsigned level)
	{
		return (level >= m_LogLevel);
	}

	void CHdLog::writeLog(const char *buf, int size)   
	{   
		static int iLen = strlen(HY_LOGGER_LINE_SEPARATOR);
		if (m_nWriteSize >= HY_LOGGER_MAXFILESIZE)
		{
			InitPath();
		}
		if (!m_pf)
		{
			m_pf = fopen(m_strPath.c_str(), "at+");

			// 创建失败取得错误代码
			if (!m_pf)
			{
				int err = errno;
			}
		}

		if (m_pf != NULL)
		{
			fwrite(buf, 1, size, m_pf);
			fwrite(HY_LOGGER_LINE_SEPARATOR, 1, iLen, m_pf);
			fflush(m_pf);
			m_nWriteSize += size + 20;
		}
	}
}
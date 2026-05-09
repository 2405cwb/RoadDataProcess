/*! hdLog.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : hdLogd.h
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


#ifndef  __HD_LOG_H__
#define  __HD_LOG_H__ 

#include "hdBasicObject.h"
#include "hdLock.h"
#include <string>

#define TBUF_SIZE 10240

using namespace std;

#pragma warning(disable:4251)
//定义了日志功能可能设置的日志输出级别
enum  enumLogLevel
{
	//不输出任何日志信息
	logLevelNone    = 60000,
	//只输出级别不低于Fatal的日志,仅Fatal
	logLevelFatal   = 50000,
	//只输出级别不低于Error的日志,包括Fatal、Error
	logLevelError   = 40000,
	//只输出级别不低于Warn的日志,包括Fatal、Error、Warn
	logLevelWarn    = 30000,
	//只输出级别不低于Info的日志,包括Fatal、Error、Warn、Info
	logLevelInfo    = 20000,
	//只输出级别不低于Debug的日志,包括Fatal、Error、Warn、Info、Debug
	logLevelDebug   = 10000,
	//只输出级别不低于Trace的日志,包括Fatal、Error、Warn、Info、Debug、Trace
	logLevelTrace   = 0,
	//输出全部级别的日志
	logLevelAll     = 0,
};

// 操作日志类
class BASICOBJECT_API CLog
{ 
public:		 
	CLog();
	~CLog(); 

	// 初始化路径
	void InitPath(string strPath)
	{
		m_strPath = strPath;
	}

	// 关闭日志
	void CloseLog()
	{
		fclose(m_pf);
		m_pf = NULL;
	}

	// 设置等级 
	void setLevel(enumLogLevel level);	 
	enumLogLevel getLevel();

	// 格式化日志
	std::string formatLog(std::string level, const char *fmt, va_list argList);

	//写日志的函数，书写格式与printf类似仅仅多了一个日志级别参数 
	void logTrace(const char *fmt,...);
	void logInfo(const char *fmt,...);
	void logDebug(const char *fmt,...);
	void logWarn(const char *fmt,...);
	void logError(const char *fmt,...);
	void logFatal(const char *fmt,...);

protected: 
	FILE *m_pf;
	enumLogLevel m_LogLevel; 
	CFastLock m_lock;
	char m_strBuf[TBUF_SIZE]; 
	string m_strPath;

	// 获取日期和时间
	std::string getDateTime(); 

	// 将日志写入文件
	void writeLog(const char *buf, int size);

	//判断调试等级是否达到 
	bool canTrace(unsigned level); 
}; 


// 全局的日志对象
class BASICOBJECT_API CStaticLog
{
public:
	static CLog* GetInstance()
	{
		static CLog s_log;
		return &s_log;
	}

private:
	CStaticLog()
	{
	}
};


#endif // (__HY_CLOG_H__) 
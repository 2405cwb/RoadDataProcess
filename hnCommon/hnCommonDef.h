#ifndef _HN_COMMON_DEF_
#define _HN_COMMON_DEF_
#include "stdafx.h"

// sql名称长度
#define SQL_NAME_LEN 256

// sql查询语句长度
#define SQL_QUERY_LEN 2048

// 数据表扩展字段长度
#define  SQL_ADDFILE_LEN 1024

// 
#define  MAXDATA    -100000000.0
#define  MINDATA    100000000.0
#define  INITALLOCMEMORY 30000
#define  ALLOCMEMORYSTEP 4

// 时间
struct DATE_TIME_INFO
{
	DATE_TIME_INFO()
	{
		year = month = day = hour = minute = second = 0;
		milliseconds = 0;
	}
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
	unsigned int milliseconds;// 为微妙，及1/1000000秒
};

#endif // _HN_COMMON_DEF_

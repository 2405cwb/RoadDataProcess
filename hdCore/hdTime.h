#pragma once
#include "hdCore.h"
#include "hdDefs.h"
#include <stdio.h>
#include <windows.h>

namespace hd
{
	
	//! 根据月日获取一年中的第几天
	inline int GetDayOfYear(int month, int day)
	{
		static const int MonthDays[] = {31,59,90,120,151,181,212,243,273,304,334,365};
		if (month >= 1 && month <= 12)
		{
			if(month > 1)
			{
				if (day > (MonthDays[month-1] - MonthDays[month-2]))
				{
					return 0;
				}

				return MonthDays[month-2] + day;
			}
			else
				return day;
		}
		else
			return 0;
	}

    //! 根据月日获取一年中的第几天(判断是不是闰年)
    inline int GetDayOfYear(int month, int day, int year)
    {
        // 定义月份天数序列(平年)
        static const int MonthDays[] = {31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

        // 是否为闰年
        bool flag = year % 400 == 0 ? true : (year % 100 == 0 ? false : (year % 4 == 0 ? true : false));

        // 是否需要增加天数
        if (flag && month > 2)
        {
            return MonthDays[month - 1] + day + 1;
        }
        else
        {
            return MonthDays[month - 1] + day;
        }
    }

	//! 时间结构体,包含与字符串转换功能.	gsl-2013/2/27
	struct HDTIME	//加HDCORE_API后调用不了函数 typedef去掉 yf-2013/3/1
	{
		HDTIME()
		{
			year = 1980;
			month = 6;
			day = 1;
			hour = 0;
			minute = 0;
			second = 0;
			milliSecond = 0;
		}

		HDTIME(const SYSTEMTIME& sysTime)
		{
			 year		  =		sysTime.wYear;
			 month		  =		sysTime.wMonth;
			 day		  =		sysTime.wDay;
			 hour		  =		sysTime.wHour;
			 minute		  =		sysTime.wMinute;
			 second		  =		sysTime.wSecond;
			 milliSecond  =		sysTime.wMilliseconds;
		}

		inline void FromSysTime(const SYSTEMTIME& sysTime)
		{
			year		=		sysTime.wYear;
			month		=		sysTime.wMonth;
			day			=		sysTime.wDay;
			hour		=		sysTime.wHour;
			minute		=		sysTime.wMinute;
			second		=		sysTime.wSecond;
			milliSecond =		sysTime.wMilliseconds;
		}

		//! 将字符串转为时间
		inline bool FromTimeString(const char* strTime)
		{
			// 有效性检查
			if (strTime != NULL)
			{
				u32 wYear,wMonth,wDay,wHour,wMinute,wSecond,wMilliSecond;
				int count = sscanf_s(strTime,"%04u%02u%02u%02u%02u%02u%03u",&wYear,&wMonth,&wDay,&wHour,&wMinute,&wSecond,&wMilliSecond);
				year = wYear;
				month = wMonth;
				day = wDay;
				hour = wHour;
				minute = wMinute;
				second = wSecond;
				milliSecond = wMilliSecond;
				return count == 7;
			}
			return false;
		}

		//! 将字符串转为日期 2014-02-10 12：12：01：51.111
		inline bool FromDateString(const char* strTime)
		{
			// 有效性检查
			if (strTime != NULL)
			{
				u32 wYear,wMonth,wDay,wHour,wMinute,wSecond,wMilliSecond;
				int count = sscanf_s(strTime,"%04u-%02u-%02u %02u:%02u:%02u.%03u",&wYear,&wMonth,&wDay,&wHour,&wMinute,&wSecond,&wMilliSecond);
				year = wYear;
				month = wMonth;
				day = wDay;
				hour = wHour;
				minute = wMinute;
				second = wSecond;
				milliSecond = wMilliSecond;
				return count == 7;
			}
			return false;
		}

		//! 将时间转换为字符串,strTime内存在外部申请
		inline void ToTimeString(char* strTime) const
		{
			// 有效性检查
			if (strTime != NULL)
			{
				sprintf_s(strTime,32,"%04u%02u%02u%02u%02u%02u%03u",year,month,day,hour,minute,second,milliSecond);
			}
		}

		//! 将时间转换为字符串,strTime内存在外部申请
		inline void ToDateTime(char* strTime) const
		{
			// 有效性检查
			if (strTime != NULL)
			{
				sprintf_s(strTime,32,"%04u-%02u-%02u %02u:%02u:%02u.%03u",year,month,day,hour,minute,second,milliSecond);
			}
		}

		inline void ToSystemTime(SYSTEMTIME& sysTime) const
		{
			sysTime.wYear = year;
			sysTime.wMonth = month;
			sysTime.wDay = day;
			sysTime.wHour = hour;
			sysTime.wMinute = minute;
			sysTime.wSecond = second;
			sysTime.wMilliseconds = milliSecond;
		}

		inline time_t ToTimeT() const
		{
			SYSTEMTIME st;
			ToSystemTime(st);
			FILETIME ft;
			SystemTimeToFileTime(&st,&ft);
			ULARGE_INTEGER ui;
			ui.LowPart = ft.dwLowDateTime;
			ui.HighPart = ft.dwHighDateTime;

			return (time_t)((LONGLONG)(ui.QuadPart - 116444736000000000)/10000);
		}

		//! 计算与另一个HDTIME相差天数(time-otherTime)
		inline int GetDistofDay(HDTIME otherTime)
		{
			int nYears = year - otherTime.year;

			// 获得在本年中第多少天
			int culDays = GetDayOfYear(month, day, year);
			int otherDays = GetDayOfYear(otherTime.month, otherTime.day, otherTime.year);

            // 需要增加的天数
            int nAddDays = 0;

            // 目标年份小于类年份
            if (nYears < -1)
            {
                for (int nTempYear = year + 1; nTempYear < otherTime.year; nTempYear ++)
                {
                    if (nTempYear % 400 == 0 ? true : (nTempYear % 100 == 0 ? false : (nTempYear % 4 == 0 ? true : false)))
                    {
                        nAddDays --;
                    }
                }
            }
            else if (nYears > 1)        // 目标年份大于类年份
            {
                for (int nTempYear = otherTime.day + 1; nTempYear < year; nTempYear ++)
                {
                    if (nTempYear % 400 == 0 ? true : (nTempYear % 100 == 0 ? false : (nTempYear % 4 == 0 ? true : false)))
                    {
                        nAddDays ++;
                    }
                }
            }

			// 返回差值
			return (int)(nYears*365 + culDays - otherDays + nAddDays);
		}

        // 根据一年中的第几天和年份得到HDTIME(由于函数GetDayOfYear没有判断是否闰年, 故此处不区分是否闰年)
        inline void FromDayOfYear(int nYear, int nDayOfYear)
        {
            // 年份直接得到
            year = nYear;

            // 定义月份天数序列
            static int MonthDays[] = {31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

            // 更新序列
            if (nYear % 400 == 0 ? true : (nYear % 100 == 0 ? false : (nYear % 4 == 0 ? true : false)))
            {
                for (int i = 1; i < 12; i ++)
                {
                    MonthDays[i] ++;
                }
            }

            for (int i = 0; i < 12; i ++)
            {
                // 到达该月
                if (nDayOfYear - MonthDays[i] <= 0)
                {
                    month = i + 1;
                    day = nDayOfYear;
                }
                else
                {
                    // 跨过改月
                    nDayOfYear -= MonthDays[i];
                }
            }
        }

		u16 year;
		u16 month;
		u16 day;
		u16 hour;
		u16 minute;
		u16 second;
		u16 milliSecond;
	};
}
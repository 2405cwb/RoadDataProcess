#pragma once

const int positive = +1;
const int negative = -1;

static int sign = positive;

struct PreciseTime               // 秒精确时间
{
	long   sn;
	double tos;
};

struct GPSTime                   // GPS 时间 
{
	long        week;
	PreciseTime tow;
};

struct CommonTime                // 格林尼治时间
{ 
	short          year;
	unsigned short month;
	unsigned short day;
	unsigned short hour;
	unsigned short minute;
	double         second;
};

struct ModifyJulianDay           // 约化儒略日
{
	long        day;
	PreciseTime tod;
};


void CommonTimeToGPSTime(const CommonTime* pct, GPSTime* pgt);              // 普通时间转为GPS时
void GPSTimeToCommonTime(const GPSTime* pgt, CommonTime* pct);              // GPS时转为普通时间
void GPSTimeToModifyJulianDay(const GPSTime* pgt, ModifyJulianDay* pmjd);
void ModifyJulianDayToGPSTime(const ModifyJulianDay* pmjd, GPSTime* pgt);
void ModifyJulianDayToCommonTime(const ModifyJulianDay* pmjd, CommonTime* pct);
void CommonTimeToModifyJulianDay(const CommonTime* pct, ModifyJulianDay* pmjd);
void CommonTimeToDayOfYear(const CommonTime* pct, int& Doy);
void TimeCopy(CommonTime &ct1,CommonTime &ct2);
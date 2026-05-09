#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include ".\inc\mydefs.hpp"

// 设置结构体成员对齐1byte
#pragma  pack(push,1)
//扫描点结构 长度为6 bytes
typedef struct MDL_SCANPOINT
{
	U16  distance;				//距离	  (0-65536)
	U16	 intensity;				//反射强度(0-4095)
	U16  angle;					//角度	  (0-35999)
}MDL_SCANPOINT;

struct MDL_BUFHEADER
{
	MDL_BUFHEADER()
		:timeHigh(0),timeLow(0),ptCount(0){}

	MDL_BUFHEADER(I64 time,U16 count)
		:ptCount(count)
	{
		SetTime(time);
	}
	
	U16 ptCount;
	U32 timeLow;
	U16 timeHigh;

	//time绝对时间,一般由QueryPerformanceCounter()获得
	inline void SetTime(I64 time)
	{
		timeLow = (U32)time;
		timeHigh = (U16)(time >> 32);
	}

	inline void SetTime(const char* str6)
	{
		memcpy_s(&timeLow,4,str6,4);
		memcpy_s(&timeHigh,2,str6 + 4,2);
	}

	inline I64 GetTime()
	{
		I64 t = timeHigh;
		t = (t << 32) + timeLow;
		return t;
	}

	inline void Reset()
	{
		timeLow = 0;
		timeHigh = 0;
		ptCount = 0;
	}
};

// 恢复默认结构体对齐
#pragma  pack(pop)
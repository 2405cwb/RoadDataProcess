/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：HdSlopeMonitorStruct.h
相关文件	: hdCommon.h、hdConstDef.h
文件实现功能：定义滑坡监测数据库结构
作者		：朱旭波
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2014/08/11	1.0			朱旭波		创建
</PRE>
******************************************************************************************************/

#pragma once
#include "hdCommon.h"
#include "hdConstDef.h"
#include "..\hdCore\hdTime.h"

namespace hd
{
	// 滑坡监测 数据表结构
	struct HD_SLOPEMONITOR_DATA
	{
		// 构造函数，赋初值
		HD_SLOPEMONITOR_DATA()
		{
			memset(strScanRouteName, 0, OBJECT_ID_LEN_L);
			nPtIndex = 0;
			dX = 0.0;
			dY = 0.0;
			dZ = 0.0;
		}

		char		strScanRouteName[OBJECT_ID_LEN_L];	// 采集工程名称
		HDTIME		tGatherTime;				// 外业数据采集时间（由解析工程名得到）
		int			nPtIndex;						// 采集点编号
		double		dX;							// x坐标
		double 		dY;							// y坐标
		double 		dZ;							// z坐标
	};
}
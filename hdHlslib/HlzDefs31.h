/*! HlzDefs.h
********************************************************************************
<PRE>
模块名       : hdHLSLib
文件名       : HlzDefs.h
相关文件     : 
文件实现功能 : 海达数云点文件hlz读写模块基本类型定义
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              创建
2015/01/08   1.0      龚书林    
</PRE>
*******************************************************************************/

#pragma once
#include <map>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <Windows.h>

#include "..\hdCore\hdMath.h"
#include "..\hdCore\hdBox3d.h"
#include "inc\mydefs.hpp"
#include "HlzDefs.h"

using namespace std;
namespace hd
{
#pragma  pack(push,1)

	//const F64 GridSize_List[15] = {2.0,4.0,8.0,16.0,32.0,64.0,128.0,256.0,512.0,
	//                               1024.0,2048.0,4096.0,8192.0,16384.0,32768.0}

	// 块集结构体,0层格网大小2,n层格网大小=2*pow(2,n)
	struct HdLevel31
	{
		// 层级编号
		U8 levelNo;	

		// 有效块集个数
		U64	numBlockset;

		// 该层内点数
		U64 pointNum;

		HdLevel31()
			:levelNo(0),numBlockset(0),pointNum(0){}
	};

	// 块集结构体
	struct HdBlockset31
	{
		// 当前块集点数
		U64 numPoint;

		// 当前块集全局x编号
		I32 XNo;

		// 当前块集全局y编号
		I32 YNo;

		// 当前块集全局z编号
		I32 ZNo;

		//分块/压缩/颜色/时间标记， 请使用对应的get/set接口访问该成员
		U8 Attribute;

		//三个必选属性地址合并为一个，按照坐标、强度、颜色的顺序存放
		HdAddr addrBaseAttri;

		// 颜色数据地址
		HdAddr addrColor;

		// 时间数据地址
		HdAddr addrTime;

		//属性数据压缩后长度信息，请使用对应的get/set接口访问该成员
		HdAddr attriCmpLen;
        		
		HdBlockset31()
			:numPoint(0),XNo(0),YNo(0),ZNo(0),Attribute(0)
		{	
		}

		~HdBlockset31()
		{
		}

		//! 判断数据是否压缩
		inline bool isCompress()
		{
			return (Attribute & 0x80) == 0x80;
		}

		//! 设置块集压缩标记
		inline void setCompress(bool compress)
		{
			Attribute = compress ? (Attribute | 0x80) : (Attribute & 0x7F);
		}

		//! 获取有效块个数
		inline U8 getBlockNum()
		{
			return (Attribute & 0x78) >> 3;
		}

		//! 设置有效块个数
		inline void setBlockNum(U8 blockNum)
		{
			Attribute = (Attribute & 0x87) | (blockNum << 3);
		}

		//! 判断是否包含颜色
		inline bool hasColor()
		{
			return (Attribute & 0x04) == 0x04;
		}

		//! 设置颜色标记
		inline void setHasColor(bool hasColor)
		{
			Attribute = hasColor ? (Attribute | 0x04) : (Attribute & 0xFB);
		}

		//! 判断是否包含时间
		inline bool hasTime()
		{
			return (Attribute & 0x02) == 0x02;
		}

		//! 设置时间标记
		inline void setHasTime(bool hasTime)
		{
			Attribute = hasTime ? (Attribute | 0x02) : (Attribute & 0xFD);
		}

		//! 获取压缩后坐标数据大小
		inline U32 getCoordLen()
		{
			return ((U32)attriCmpLen.high << 4) | (attriCmpLen.low >> 28);
		}

		//! 设置压缩后坐标数据大小
		inline void setCoordLen(U32 coordLen)
		{
			attriCmpLen.high = (U16)(coordLen >> 4);
			attriCmpLen.low = (attriCmpLen.low & 0x0FFFFFFF) | (coordLen << 28);
		}

		//! 获取压缩后强度数据大小
		inline U16 getIntenLen()
		{
			return (attriCmpLen.low & 0x0FFFF000) >> 12;
		}

		//! 设置压缩后强度数据大小
		inline void setIntenLen(U16 intenLen)
		{
			attriCmpLen.low = (attriCmpLen.low & 0xF0000FFF) | ((U32)intenLen << 12);
		}
	};
	
	// 块结构体
	struct HdBlock31
	{
		// 当前包点数
		U32 numPoint;

		//分包标记、压缩/颜色/时间标记，请使用对应的get/set接口访问该成员
		U8 Attribute;

		// 有效包个数
		U16	numParcel;
		
		//三个必选属性地址合并为一个，按照坐标、强度、颜色的顺序存放
		HdAddr addrBaseAttri;

		// 颜色数据地址
		HdAddr addrColor;

		// 时间数据地址
		HdAddr addrTime;

		//!属性数据压缩后长度信息，请使用对应的get/set接口访问该成员
		HdAddr attriCmpLen;

		HdBlock31()
			:numPoint(0),Attribute(0),numParcel(0)
		{	
		}

		~HdBlock31()
		{
		}

		//! 判断块数据是否压缩
		inline bool isCompress()
		{
			return (Attribute & 0x80) == 0x80;
		}

		//! 设置块压缩标记
		inline void setCompress(bool compress)
		{
			Attribute = compress ? (Attribute | 0x80) : (Attribute & 0x7F);
		}

		//! 获取块x方向编号，只能为0、1
		inline U8 getXNo()
		{
			return ((Attribute & 0x40) == 0x40) ? 1 : 0;
		}

		//! 设置块x方向编号，只能为0、1
		inline void setXNo(U8 no)
		{
			Attribute = (no == 0) ? (Attribute & 0xBF) : (Attribute | 0x40);
		}

		//! 获取块y方向编号，只能为0、1
		inline U8 getYNo()
		{
			return ((Attribute & 0x20) == 0x20) ? 1 : 0;
		}

		//! 设置块y方向编号，只能为0、1
		inline void setYNo(U8 no)
		{
			Attribute = (no == 0) ? (Attribute & 0xDF) : (Attribute | 0x20);
		}

		//! 获取块z方向编号，只能为0、1
		inline U8 getZNo()
		{
			return ((Attribute & 0x10) == 0x10) ? 1 : 0;
		}

		//! 设置块z方向编号，只能为0、1
		inline void setZNo(U8 no)
		{
			Attribute = (no == 0) ? (Attribute & 0xEF) : (Attribute | 0x10);
		}

		//! 判断块是否包含颜色数据
		inline bool hasColor()
		{
			return (Attribute & 0x08) == 0x08;
		}

		//! 设置块颜色标记
		inline void setHasColor(bool hasColor)
		{
			Attribute = hasColor ? (Attribute | 0x08) : (Attribute & 0xF7);
		}

		//! 判断块是否包含时间数据
		inline bool hasTime()
		{
			return (Attribute & 0x04) == 0x04;
		}

		//! 设置块是否包含时间数据
		inline void setHasTime(bool hasTime)
		{
			Attribute = hasTime ? (Attribute | 0x04) : (Attribute & 0xFB);
		}

		//! 获取压缩后坐标数据大小
		inline U32 getCoordLen()
		{
			return ((U32)attriCmpLen.high << 4) | (attriCmpLen.low >> 28);
		}

		//! 设置压缩后坐标数据大小
		inline void setCoordLen(U32 coordLen)
		{
			attriCmpLen.high = (U16)(coordLen >> 4);
			attriCmpLen.low = (attriCmpLen.low & 0x0FFFFFFF) | (coordLen << 28);
		}

		//! 获取压缩后强度数据大小
		inline U16 getIntenLen()
		{
			return (attriCmpLen.low & 0x0FFFF000) >> 12;
		}

		//! 设置压缩后强度数据大小
		inline void setIntenLen(U16 intenLen)
		{
			attriCmpLen.low = (attriCmpLen.low & 0xF0000FFF) | ((U32)intenLen << 12);
		}
	};
	
	// 包结构体
	struct HdParcel31
	{
		// 当前包点数
		U16 numPoint;

		// 当前块集x编号
		U16 XNo;

		// 当前块集y编号
		U16 YNo;

		// 当前块集z编号
		U16 ZNo;

		//压缩、颜色、时间标记、包深度，请使用对应的get/set接口访问该成员
		U8 Attribute;

		//三个必选属性地址合并为一个，按照坐标、强度、颜色的顺序存放
		HdAddr addrBaseAttri;

		// 颜色数据地址
		HdAddr addrColor;

		// 时间数据地址
		HdAddr addrTime;

		//属性数据压缩后长度信息，请使用对应的get/set接口访问该成员
		HdAddr attriCmpLen;

		HdParcel31()
			:numPoint(0),XNo(0),YNo(0),ZNo(0),Attribute(0)
		{
		}

		~HdParcel31()
		{
		}

		//! 判断包数据是否压缩
		inline bool isCompress()
		{
			return (Attribute & 0x80) == 0x80;
		}

		//! 设置包压缩标记
		inline void setCompress(bool compress)
		{
			Attribute = compress ? (Attribute | 0x80 ) : (Attribute & 0x7F);
		}

		//! 获取包切割深度
		inline U8 getDivCount()
		{
			return (Attribute & 0x78) >> 3;
		}

		//! 设置包切割深度
		inline void setDivCount(U8 divCount)
		{
			Attribute = (Attribute & 0x87) | (divCount << 3);
		}

		//! 判断包是否包含颜色数据
		inline bool hasColor()
		{
			return (Attribute & 0x04) == 0x04;
		}

		//! 设置包颜色标记
		inline void setHasColor(bool hasColor)
		{
			Attribute = hasColor ? (Attribute | 0x04) : (Attribute & 0xFB);
		}

		//! 判断包是否包含时间数据
		inline bool hasTime()
		{
			return (Attribute & 0x02) == 0x02;
		}

		//! 设置包是否包含时间数据
		inline void setHasTime(bool hasTime)
		{
			Attribute = hasTime ? (Attribute | 0x02) : (Attribute & 0xFD);
		}

		//! 获取压缩后坐标数据大小
		inline U32 getCoordLen()
		{
			return ((U32)attriCmpLen.high << 4) | (attriCmpLen.low >> 28);
		}

		//! 设置压缩后坐标数据大小
		inline void setCoordLen(U32 coordLen)
		{
			attriCmpLen.high = (U16)(coordLen >> 4);
			attriCmpLen.low = (attriCmpLen.low & 0x0FFFFFFF) | (coordLen << 28);
		}

		//! 获取压缩后强度数据大小
		inline U16 getIntenLen()
		{
			return (attriCmpLen.low & 0x0FFFF000) >> 12;
		}

		//! 设置压缩后强度数据大小
		inline void setIntenLen(U16 intenLen)
		{
			attriCmpLen.low = (attriCmpLen.low & 0xF0000FFF) | ((U32)intenLen << 12);
		}
	};

	class CHdLevel31;
	class CHdBlockset31;
	class CHdBlock31;
	class CHdParcel31;

	typedef pair<U32,CHdLevel31*> HdLevel31Pair;
	typedef pair<U32,CHdBlockset31*> HdBlockSet31Pair;
	typedef pair<U16,CHdBlock31*> HdBlock31Pair;
	typedef pair<U16,CHdParcel31*> HdParcel31Pair;
#pragma  pack(pop)
}
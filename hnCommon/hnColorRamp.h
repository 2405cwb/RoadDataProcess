/*! hnColorRamp.h
********************************************************************************
<PRE>
模块名       : hnPointCloud
文件名       : hnColorRamp.h
相关文件     : hnColorRamp.cpp
文件实现功能 : 渐变色带
作者         : 谢卓
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 :
日 期			版本			修改人				修改内容
2017/8/2		1.0			谢卓					创建
</PRE>
*******************************************************************************/

#ifndef HNCOLORRAMP_H
#define HNCOLORRAMP_H

//#include "hnImageAlgorithm.h"
#include <windows.h>
#include <map>
#include "hnTypeDefs.h"
#include "stdafx.h"

using namespace hnCommon;

#define MAP_LEN	256

namespace hnImageAlgorithm
{
	// 色带类型枚举;
	enum COLORRAMP_TYPE
	{
		ENUM_BLUE_TO_RED_N,		// 蓝色到红色不等间距渐变
		ENUM_RED_TO_BLUE_N,		// 红色到蓝色不等间距渐变
		ENUM_BLUE_TO_RED_Y,		// 蓝色到红色等间距渐变
		ENUM_RED_TO_BLUE_Y,		// 红色到蓝色等间距渐变
		ENUM_JET,
		ENUM_GB_TO_RED_N,		// 灰色到红色不等间距渐变
	};

	class HNCOMMOMAPI hnColorRamp
	{
	public:
		// 构造函数;
		hnColorRamp();

		// 构造函数;
		hnColorRamp(COLORRAMP_TYPE index);

		// 析构函数;
		~hnColorRamp();

	public:
		// 获取红色通道;
		Byte Red(Byte index) { return m_Color[0][index]; }

		// 获取绿色通道;
		Byte Green(Byte index) { return m_Color[1][index]; }

		// 获取蓝色通道;
		Byte Blue(Byte index) { return m_Color[2][index]; }

		// 从彩色反映射到灰度;
		Byte Gray(Byte r, Byte g, Byte b);

	private:
		// 初始化颜色;
		void InitColor();

		// 获取每个渐变色节点;
		void BuildNodes();

		// 生成每个索引的颜色;
		void Build();
	private:
		int m_NbNode;
		int m_Size;
		Byte m_Color[4][MAP_LEN];
		int m_Node[MAP_LEN];
		COLORRAMP_TYPE m_colorRamp;
		std::map<unsigned int, short> m_rgb2gray;
	};

}

#endif //HNCOLORRAMP_H


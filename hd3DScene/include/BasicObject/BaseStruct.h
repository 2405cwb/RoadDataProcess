/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdBasicObject
文件名		：BaseStruct.h
相关文件	: 
文件实现功能：定义基本的数据类型结构体
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/5/15	1.0			马振明		创建
</PRE>
******************************************************************************************************/
#pragma once
#include <string>
#include <memory>
using namespace std;
using namespace std::tr1;

// 转成类的声明式
class CHdGeoRaster;

// 图像的名称与坐标范围，宽高，类似创建一个图像的索引
typedef struct _IMAGEEXTINFO
{
	int nWidth;					// 宽
	int nHeight;				// 高
	int nBand;					// 波段
	float fScale;				// 分辨率
	double dTop;				// 最上的Y
	double dBottom;				// 最下的Y
	double dLeft;				// 最左的X
	double dRight;				// 最右的X
	string strImagePath;		// 影像路径
	string strImageName;		// 图像名称
	string strExtName;			// 后缀名

	float GetScale()
	{
		return float((dTop-dBottom)/nHeight);
	}		
}IMAGEEXTINFO,*IMAGEEXTINFOPTR;
//

// 智能指针对象
typedef shared_ptr<IMAGEEXTINFO> ImageExtInfoSPtr;


// 级别
typedef struct _OVERVIEWLEVEL
{
	int nLevelIndex;						// 级别索引
	int nRow;								// 行数
	int nCol;								// 列数
	float fScaleX;							// 分辨率X
	float fScaleY;							// 分辨率Y
}OVERVIEWLEVEL;


//// 金字塔影像
//typedef struct _OVERVIEWINFO 
//{
//	IMAGEEXTINFO* pImageInfo;				// 影像信息 
//	int nCurLevel;							// 当前显示级别
//	OVERVIEWLEVEL* pLevel;					// 级别列表
//	int nCount;								// 级别总数
//}OVERVIEWINFO;


//// 级别
//typedef struct _OVERVIEWLEVEL
//{
//	_OVERVIEWLEVEL()
//		:pOverImage(NULL)
//	{
//	}
//
//	int nLevelIndex;						// 级别索引
//	int nRow;								// 行数
//	int nCol;								// 列数
//	OVERVIEWIMAGE* pOverImage;				// 记录金字塔的数据
//}OVERVIEWLEVEL;
//
//// 金字塔，外部调用时初始化指针，申请内存等
//typedef struct _OVERVIEWDATA
//{
//	_OVERVIEWDATA()
//		:pOverLevel(NULL)
//	{
//	}
//
//	int nLevelCount;						// 级别个数
//	int nHeight;							// 高
//	int nWidth;								// 宽
//	OVERVIEWLEVEL* pOverLevel;				// 记录金字塔影像每个级别
//	string strOverViewPath;					// 金字塔路径
//}OVERVIEWDATA;
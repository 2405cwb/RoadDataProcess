/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：hdBillBoardDTStruct.h
相关文件	: hdCommon.h、hdConstDef.h
文件实现功能：定义广告牌标注数据库表结构。
作者		：杨峰
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2013/6/26	1.0			杨峰		创建
2014/4/23	1.0			马振明		创建
</PRE>
******************************************************************************************************/

#pragma once
#include "hdCommon.h"
#include "hdConstDef.h"
#include "..\hdCore\hdTime.h"

namespace hd
{
	// 广告牌标注 数据表结构
	struct HD_BILLBOARDDATA
	{
		// 构造函数，赋初值
		HD_BILLBOARDDATA()
		{
			memset(strMarkerID, 0, MARK_ID_LEN_L);
			memset(strCompany, 0, MARK_ID_LEN_L);
			memset(strAddress, 0, MARK_ID_LEN_L);
			memset(strPhone,0,16);
			memset(strWebSite, 0, MARK_ID_LEN_L);
			memset(strDuration, 0, MARK_ID_LEN_L);
			memset(strState, 0, 2);
			memset(strDuration, 0, MARK_ID_LEN_L);
			memset(strRemark, 0, 1000);

			nPhotoSize = 0;
			pPhoto = NULL;
		}

		// 析构函数，清空内存
		~HD_BILLBOARDDATA()
		{
			if (pPhoto)
			{
				delete []pPhoto;
				pPhoto = NULL;
			}
		}

		// 分配位置坐标内存大小
		void AllocSize(int nSize)
		{
			if (nPhotoSize > 0)
			{
				nPhotoSize = nSize;
				pPhoto = new BYTE[nSize];
				memset(pPhoto, 0, nSize);
			}
		}				

		char	strMarkerID[MARK_ID_LEN_L];		// 标注ID
		char	strCompany[MARK_ID_LEN_L];		// 公司名称
		char	strAddress[MARK_ID_LEN_L];		// 地址
		char	strPhone[16];					// 公司电话
		char	strWebSite[MARK_ID_LEN_L];		// 公司网址
		char	strDuration[MARK_ID_LEN_L];		// 广告投放日期
		double	dArea;							// 广告投放面积
		double	dPrices;						// 价钱
		double	dWidth;							// 广告牌宽度
		double	dHeight;						// 广告牌高度
		int		eSymbolType;					// 标注类型
		BYTE*	pPhoto;							// 图片
		long	nPhotoSize;						// 图片二进制大小
		char	strState[2];					// 状态
		char	strLocation[MARK_ID_LEN_L];		// 广告牌
		HDTIME	tEditDate;						// 时间
		char	strRemark[1000];				// 备注
	};
}
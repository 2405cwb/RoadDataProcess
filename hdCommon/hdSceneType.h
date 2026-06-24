/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：hdSceneType.h
相关文件	: hdCommon.h
文件实现功能：定义三维场景刷新类型
作者		：龚书林
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2016/1/5	1.0			龚书林		创建
</PRE>
******************************************************************************************************/
#pragma once
namespace hd
{
// 定义刷新内容
enum ESCENE_REFRESH_TYPE
{
	// 刷新hls/hlz点云
	HDView_PointCloud = 1,
	// 刷新Pano全景
	HDView_PANO =       (1<<1),
	// 刷新正射影像
	HDView_DOM  =       (1<<2),
	// 刷新DEM
	HDView_DEM  =       (1<<3),
	// 刷新TIN
	HDView_TIN  =       (1<<4),
	// 刷新矢量图层
	HDView_Geography =  (1<<5),
	// 刷新临时测量线、靶球、标记点、标靶
	HDVIEW_Graphics =   (1<<6),
	// 刷新全景点云及4D背景数据
	HDView_BACKDATA = HDView_PointCloud | HDView_PANO | HDView_DOM | HDView_DEM | HDView_TIN | HDView_Geography,
	HDVIEW_ALL =        0x00000fff,
};

}
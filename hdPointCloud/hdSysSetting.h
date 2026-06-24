/*! CHdSysSetting.h
********************************************************************************
<PRE>
模块名       : CHdSysSetting
文件名       : CHdSysSetting.h
相关文件     : 
文件实现功能 : 系统设置
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/06/29   1.00     龚书林
2013/08/29   1.01     朱旭波              增加选中点颜色、标记点颜色（绘至CAD点等）
                                          选择线颜色、测量线颜色系统设置，对应
										  SysSettingSelColorDlg.h对话框
2013/09/17   1.02     朱旭波              增加iscan工程系统设置	
2013/09/30   1.03     张飞                将颜色标签修改为测量，增加测量点的两种模式
</PRE>
*******************************************************************************/

#pragma once
#include "stdafx.h"
#include "hdPointCloud.h"
#include <string>

using namespace std;

namespace hd
{
struct hdCommonSettng
{
	hdCommonSettng()
	{
		UILanguage = 0;
		autoSaveTime = 30;
		renderSimple = 2000000;
		loadSimple = 10000000;
		showNormal = 0;
		showVector = 1;
		editMode = 1;
		space_filter_dist = 0.02f;
		load_simple_mode = 0;
		
	}
	// 用户界面语言,0中文（简体）代号,1英文
	int UILanguage;
	// 自动保存时间间隔
	int autoSaveTime;
	// 动态显示抽样阀值
	int renderSimple;
	// 文件加载抽稀
	int loadSimple;
	// 浏览三维视图时,是否显示法向量
	int showNormal;
	// 是否为编辑模式,1编辑模式,0浏览模式
	int editMode;
	 //是否显示外部导入矢量图
	int showVector;
	// 空间抽稀的距离阀值
	float space_filter_dist;
	// 加载抽稀的方式，目前有2种，0 表示按数值抽稀，1表示按空间进行抽稀
	// 默认采用按数值进行抽稀
	int  load_simple_mode;

};

struct hdSelectionSetting
{
	hdSelectionSetting()
	{
		inside = 1;
		selectMode = 0;
	}
	// 选择内部还是外部 1是内部 0 是外部
	int inside;
	// 选择模式,0增加选择,1减少选择
	int selectMode;
};

struct CENTROID_PICKER
{
	CENTROID_PICKER()
	{
		minIntensity = 500;
		maxIntensity = 800;
		radius = 0.1;
		pickCtrlPtMode = 0;
		openZoomWnd = 1;
		zoomPcdRadius = 1.f;
	}
	// 获取点时,使用的最小反射强度
	int minIntensity;
	// 获取点时,使用的最大反射强度
	int maxIntensity;
	// 搜索距离
	double radius;
	// 同名点选取方式,0用户点取位置,1用户点取的3*3位置的平均值
	int pickCtrlPtMode;
	// 同名点选取是否开启放大窗口选取，1打开，0不打开
	int openZoomWnd;

	// 放大镜窗口显示点云半径
	float zoomPcdRadius;
};
enum hdUnits
{
	hdMeters = 0,
	hdCenterMeter,
	hdDeciMeter,
};

struct hdImportSetting
{
	// 坐标单位
	hdUnits units;
	// 最大行数
	int maxRow;
	// 最大列数
	int maxCol;
	// 最大反射值
	int maxIntensity;
};

struct hdExportSetting
{
	// 最远距离
	float maxDistance;
	// 最近距离
	float minDistance;
	// 行抽样
	int rowSimple;
	// 列抽样
	int colSimple;
};

struct hdMatchSetting
{
	// 靶球半径,单位m
	float sphereRadius;
	// 靶球半径均差
	float radiusStdDev;
	// 靶球搜索块大小
	int blockSize;
	// 标靶半径，单位m
	float chessboardSize;

	// 平差报告输出路径
	std::string strReportPath;
};

struct hdFilterSetting
{
	// 最大距离
	float maxDistance;
	// 最大反射值
	int   maxIntensity;
	// 最小反射值
	int   minIntensity;
	float MaxGridSize;		// 最大网格大小：dem生成tin过滤掉三角形包围盒xy方向上大于该值的三角形
	float MaxTriaSIzeLen;		// 最大三角形边长：过滤掉大于该边长的三角形
	int AddFilter;       // 是否叠加过滤,1为是，0为否
};

//! 测量参数设置
struct hdMeasureSetting
{
	// 选中点颜色
	COLORREF selPointColor;
	// 选择线颜色
	COLORREF selPLineColor;
	// 测量线颜色
	COLORREF measureLineColor;
	// 绘至CAD点颜色
	COLORREF cadPointColor;
	// 发送到CAD模式,0发送点线面对象,1发送坐标
	int      nSendMode;
	// 量测工具以及选择工具在屏幕中选择点的模式，0 - 针对内存中的点，1 - 针对显示的点 fengjing 20140905
	int     nSelPtsMode;
	// 选点模式,0:最近点云;1:邻域内点云均值
	int		nPickPointMode;
// 	// 平均值模式
// 	int		nAveragePoint;
	// 邻域大小
	double	dNearFieldSize;

	int selPointSize;//选中点的大小
	int MarkPointSize;//标记点的大小
	int selLineWidth;//选择线的宽度
	int meaLineWidth;//测量线的宽度

};

//! iScan工程系统配置
struct hdiScanSetting
{
	// iScan全景视图是否显示原图，为1时显示原图，为0时显示缩略图
	int panoOnFullSize;
    
	// 是否根据pos的时间显示全景视图HDI时间点处前后50m范围内点云,为否时根据hdi距离范围显示
	int showByPos;
	
	// 是否根据lin文件计算pos得到方向box来渲染
	int loadByObbBox;

	// 定义全景视图根据距离显示点云的距离值
	float pcdDistInPanoView;

	// iScan点云着色设备类型
	int m_nIScanDeviceType;
	
	// iScan点云着色单帧着色范围
	float m_fColorRange;

	// 快速相机下显示点云抽样阀值
	int renderSimple;

	//3D相机下显示点云抽样阈值
	int renderSimple_3D;



};

struct hd719Setting
{
	hd719Setting()
	{
		pcdDistInPanoView = 50;
		ptNumThredInQuickCam = 12000000;
		ptNumThredIn3DCam = 2000000;
		AutoFilter = 0;
	}
	// 定义全景视图根据距离显示点云的距离值
	float pcdDistInPanoView;

	// 快速相机下点云显示阈值
	int  ptNumThredInQuickCam;

	//3D相机下点云显示阈值
	int  ptNumThredIn3DCam;

	// 过滤后是否实时计算
	int  AutoFilter;

};

//! 渲染设置
struct hdRenderSetting
{
	// 背景色 
	COLORREF bkColor;

	// 渲染方式
	int renderStyle;

	// 点大小
	int ptSize;

	// 强度拉伸极小值
	float fMinIntenThre;

	// 强度拉伸极大值
	float fMaxIntenThre;

};

//! 点云触点拟合面参数设置
struct hdFitRowColSetting
{
	// 行宽度
	int nRow;

	// 列宽度
	int nCol;

	// 每次减小的行宽度
	int nRowCut;

	// 每次减小的列宽度
	int nColCut;

	// 拟合的次数
	int nFitCount;
};

//! 草图模式下默认参数设置  by liuzhaoliang
struct hdDraftEditSetting 
{
	hdDraftEditSetting()
	{
		//草图模式参数设置
		FilterMode = 0;
		lHeight = 3.5;
		uHeight = 0.5;
		MaxHeight = 3.5;
		MinHeight = 0.5;
		LoadSimpleThres = 2000000;
	}


	//高度过滤模式  0.低于；1.高于；2.介于
	int FilterMode;

	//高于模式下默认参数,upper height
	double uHeight;

	//低于模式下默认参数，lower height
	double lHeight;

	//介于模式下参数左区间
	double MinHeight;

	//介于模式下参数右区间
	double MaxHeight;

	//草图模式下默认的加载阈值
	int LoadSimpleThres;
};

//! 系统配置单实例对象
class HDPOINTCLOUD_API CHdSysSetting
{
private:
	static CHdSysSetting* sysSetting;
	// 定义一个单实例销毁辅助类
	class hdSettingCleaner
	{
	public:
		hdSettingCleaner(){}
		virtual ~hdSettingCleaner()
		{
			if (sysSetting != NULL)
			{
				delete sysSetting;
				sysSetting = NULL;
			}
		}
	};
	
	CHdSysSetting(void);
	~CHdSysSetting(void);
public:
	// int m_IsFirstRun; // 标志是否第一次启动程序
	hdCommonSettng  commonSetting;
	hdSelectionSetting selectionSetting;	
	//hdImportSetting importSetting;
	//hdExportSetting exportSetting;
	hdMatchSetting   matchSetting;
	hdFilterSetting filterSetting;
	CENTROID_PICKER centrioidPicker;
	hdMeasureSetting measureSetting;
	hdiScanSetting  iScanSetting;
	hdRenderSetting	renderSetting;
	hdFitRowColSetting fitRowColSetting;
	hdDraftEditSetting draftSetting;
	hd719Setting i719Setting;

public:

	//! 从配置文件读取，获取系统设置,外部不需要delete
	static CHdSysSetting* getSysSetting();
	//! 删除静态唯一对象
	static void destroySysSetting();
	//! 保存配置
	void saveSetting();
};

}
#pragma once
#include <QVector>
#include <QString>
#include"configService.h" 
#include "../hnQtCommon/HnProjectEnums.h"

class HNCONFIGSERVICE_EXPORT HnXRSettings
{
public:
	static HnXRSettings* getInstance();
	~HnXRSettings();
	void SetConfigFilePath(const QString&);
	void Init();
	void readData();
private:
	QString m_iniFilePath;
	
	HnXRSettings();
	HnXRSettings& operator= (const HnXRSettings&);
	static HnXRSettings* _XRSetting;
	configService* m_Setting;
	//CfgInterface* m_Setting;
public:
	void writeData();


public: 
	//报表报错信息
	QStringList ExcelErrorMessageList;

public:
	/// <summary>
	/// 导入的工程数据默认路径
	/// </summary>
	QString DefaultPath;


	//最后工程名称
	QString  lastProjectName;
	//最后工程帧号
	int lastProjectFn;

	//excel导出文件地址
	QString OutPath;
	QString MpdInterveneFAactor;


	/// <summary>
	/// 报表数值修约方式，0-四舍五入修约，1-奇进偶舍修约
	/// </summary>
	int sheetRoundingOffType;

	/// <summary>
	/// 导出的报表中数值小数修约位数
	/// </summary>
	int sheetRoundingOffNum;

	/// <summary>
	/// 导出的报表中DR数值小数修约位数
	/// </summary>
	int sheetRoundingOffNum_Dr;

	double rutKCorrect;
	double rutBCorrect;

	//路面车辙病害的影响宽度0.4m 只对小方格的2018等级公路会用到
	double RutDisWidth;

	/// 高速路采集了双轮迹的平整度报表导出设置，0-只导出左侧DAQ0，1-只导出右侧DAQ1，2-默认选项所有数据都导出
	int IRIExcelSide; 


	
	/// 当路面有水的时候，构造深度值特别小，用IRI_threshval来和路段的构造深度值比较，小于这个值就认为路面有水，调整IRI计算策略，只用加速度的位移来计算IRI
	
	double IRI_threshval;


	/// 平整度评定方式，0-取双轮迹平整度的平均值（默认），1-取双轮迹平整度的最大值 ,2-低等级农村路湖南规范平整度计算标准 
	int RQIJudgeType;

	//平整度计算时，激光测距机的数据是否需要剔除异常值，正常激光测距机数据质量较好不需要，当路面有水或者沥青雾封罩面工艺的新路,测距机数据里面异常值比较多需要特殊处理
	bool Las_Filter;
	double Las_Filter_Thresh0;
	double Las_Filter_Thresh1;
 
	/// 是否要将，车辙值控制在 ErrorRut 异常值上限的范围内
	 bool IsThresholdRut;
	
	 /// 车辙要处理的异常值上限
	
	 double ErrorRut;

	 /// 左右车辙调整的异常差值上限
	 double ErrorRutTh1;
	 /// 前后相邻断面需要处理的车辙异常差值上限
	 double ErrorRutTh2;

	 
	 //软件出表界面设置 

	 //路面车辙病害导出选择  0不导出  1导出 2仅导出二三级路车辙病害   
	 int czDisOutSelectExcel; 

	 //路面病害程度区分选择  0 区分病害程度   1 所有病害按照重度计算
	 int roadDisDegreeExcel;

	 //水泥路是否有刻槽  0 无  1 有
	 int roadSnKcShowExcel;

	 //代表车辙的计算公式  0 最大值 1 平均值 2最大值平均
	 int  rutOutMode;
	 //表格是否排序
	 bool outExcelNeedSort;


	 bool outMarkInfoFile;

	 //报表导出内容  0 路面病害导出图像名称 
	// bool  outDisPicNameExcel;
	 //报表导出车速和打标
	 bool outSpeedAndMarkExcel;
	 
	 /// 是否将构造的激光测距值转换成明码输出，调试用
	  
	 bool IsOutputLasval;

	 
	 /// 构造要处理的异常值上限 
	 double ErrorMTD; 

	 double  MPD_K;
	 double  MPD_B;

	 /// <summary>
	 /// 水泥路面，破碎板的面积计算方式，0--病害框面积，1--板块面积
	 /// </summary>
	  int BrokenPlatetype;

	  /// <summary>
	  /// 水泥路面，水泥板块的宽度，单位m
	  /// </summary>
	   double PlateWidth;

	  /// <summary>
	  /// 水泥路面，水泥板块的长度，单位m
	  /// </summary>
	   double PlateLength;

	   //界面移动鼠标退回阈值
	   int movePictureBackMouseRatio;

	   //是否根据打标分段
	   bool  outMileWithMark;

	   //路面单元是否分段
	   bool outRoadUnitMark;

	   //按照里程出表(长短链需求) 贵州乾通需求
	   bool outExcelFormatDmi;

	   //输出报表成功
	   bool outExcel;

	   /// <summary>
	   /// 0 模块化设备
	   /// 1 二三维设备
	   /// 在高精度定位 gps桩号匹配时 供用户选择确定 
	   /// </summary>
	   int equipType = 0;

	   //病害表是否导出图片
	   int diseaseExcelOutPicture = false;

	   //病害表是否导出病害坐标
	   bool diseaseExcelOutLocation = false;

	   //显示病害备注
	   bool diseaseMark = false;

	   //高精度设备显示定位
	   bool showGpsInfo = false;

	   //false 显示大地坐标 
	   bool gpsFormat = true;

	   //显示矩形框
	   bool diseaseRectShow;

	   //校桩选择整数
	   bool mile2dmiToInt;

	   //记录最近的五条备注信息
	   QString diseaseMarkTxts;

	   QString diseaseMarkTxt;

};
 

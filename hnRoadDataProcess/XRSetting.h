#pragma once
#include <string>
#include <iostream>
#include <vector>

using namespace std;
/// <summary>
/// 养护标准类型
/// </summary>
enum StandardParmType
{
	/// <summary>
	/// 0--等级公路2007
	/// </summary>
	DegreeRoad2007,

	/// <summary>
	/// 1--城镇道路
	/// </summary>
	CityRoad,

	/// <summary>
	/// 2--北京农村公路
	/// </summary>
	RuralRoadBeijing,

	/// <summary>
	/// 3--等级公路2018
	/// </summary>
	DegreeRoad2018,

	/// <summary>
	/// 4--等级公路2001
	/// </summary>
	DegreeRoad2001,

	/// <summary>
	/// 5--上海城市道路
	/// </summary>
	CityRoadShanghai,

	/// <summary>
	/// 6--辽宁农村路
	/// </summary>
	RuralRoadLiaoning,

	/// <summary>
	/// 7-广西农村路
	/// </summary>
	RuralRoadGuangxi,

	/// <summary>
	/// 8-重庆农村路
	/// </summary>
	RuralRoadChongqing,

	/// <summary>
	/// 9-湖南农村路
	/// </summary>
	RuralRoadHunan,
	/// <summary>
	/// 10-低等级农村公路
	/// </summary>
	RuralRoadlowLevel

};


class XRSetting
{
public:
	 static XRSetting* getInstance();
	~XRSetting();
private:
	XRSetting();
	XRSetting(const XRSetting&);
	XRSetting& operator= (const XRSetting&);
	static XRSetting* _XRSetting ;
public:
	void readData();
	void writeData();
public:
	/// <summary>
	/// 界面风格
	/// </summary>
	 string SkinName;

	/// <summary>
	/// 软件大图标
	/// </summary>
	 string  ICO;

	/// <summary>
	/// 左上角软件小图标
	/// </summary>
	 string  ICODX;

	/// <summary>
	/// 软件的公司信息
	/// </summary>
	 string  CompanyInfo;

	/// <summary>
	/// IRM的异常值处理方式，0--异常值不处理，1--异常值根据设置方式调整
	/// </summary>
	 int ErrorVal;

	/// <summary>
	/// IRI要调整处理的异常值上限
	/// </summary>
	 double ErrorIRI;

	/// <summary>
	/// 构造要处理的异常值上限
	/// </summary>
	 double ErrorMTD;

	/// <summary>
	/// 车辙要处理的异常值上限
	/// </summary>
	 double ErrorRut;

	/// <summary>
	/// 左右车辙调整的异常差值上限
	/// </summary>
	 double ErrorRutTh1;

	/// <summary>
	/// 前后相邻断面需要处理的车辙异常差值上限
	/// </summary>
	 double ErrorRutTh2;

	/// <summary>
	/// 是否要将，车辙值控制在 ErrorRut 异常值上限的范围内
	/// </summary>
	 bool IsThresholdRut;

	/// <summary>
	/// 0--等级公路2007，1--城镇道路，2--北京农村公路，3--等级公路2018, 4--等级公路2001, 5--上海城市道路，6--辽宁农村路，7-广西农村路，8-重庆农村路，9-湖南农村路  10-低等级农村公路
	/// </summary>
	 StandardParmType ParmStyle;

	/// <summary>
	/// 0--各项指标单独出表，1--所有指标综合出表，2--中南安环，3--中交国通，4--带GPS模板，5--奥路通，7--上海浦公，8-厦门捷航，9-河南焦作, 10-广东华路 ,12-csv报表
	/// </summary>
	 int ExcelType;

	/// <summary>
	/// false--导出的报表内容不排序，true--导出的报表内容根据桩号从小到大排序
	/// </summary>
	 bool IsExcelSort;

	/// <summary>
	/// false--报表不输出指标统计信息，true--报表输出指标统计信息
	/// </summary>
	 bool IsStatistics;

	/// <summary>
	/// 是否给图像名中添加桩号信息
	/// </summary>
	 bool IsRename;

	/// <summary>
	/// 养护类型，0--广西标准，1--辽宁标准，2--广西PCI标准，生成广西桂兴达报表时会用到
	/// </summary>
	 int YHType;

	/// <summary>
	/// 导入的工程数据默认路径
	/// </summary>
	 string  DefaultPath;

	/// <summary>
	/// 图像文件的后缀
	/// </summary>
	 string  ImgType;

	/// <summary>
	/// 不同指标报表数量
	/// </summary>
	 int LenExcelNum;

	/// <summary>
	/// 是否要导出不同指标的报表
	/// </summary>
	 vector<bool> IsExcel;

	/// <summary>
	/// 不同指标报表的单元区间长度
	/// </summary>
	 vector<string > LenExcel;

	/// <summary>
	/// 检测年，报表转换为奥路通平台输入模板时会用到
	/// </summary>
	 string  DetectYear;
	/// <summary>
	/// 检测次数，报表转换为奥路通平台输入模板时会用到
	/// </summary>
	 string  DetectNum;
	/// <summary>
	/// 县区代码，报表转换为奥路通平台输入模板时会用到
	/// </summary>
	 string  DistrictCode;

	/// <summary>
	/// 管养单位
	/// </summary>
	 string  DutyUnit;

	/// <summary>
	/// 道路的车道描述，比如双向四车道，生成城镇路的报告报表时会用到
	/// </summary>
	 string  RoadSideType;

	/// <summary>
	/// 将城镇路的病害合并，让客户能自己再画到CAD上的，病害合并长度
	/// </summary>
	 int CADLength;

	 bool IsRepair;

	/// <summary>
	/// 导出路面车辙病害  0--不导出车辙病害 1--导出所有等级路车辙病害 2--只导出二三四级公路车辙病害
	/// </summary>
	 int OutRut;

	/// <summary>
	/// 区分病害程度  0--区分 1--所有病害程度按重度计算
	/// </summary>
	 int Qufen_dis_degree;

	/// <summary>
	/// 不同的平整度算法，正常应该设置为0
	/// </summary>
	 int Acc_IRI;
	 double Acc_IRI_K_1;
	 double Acc_IRI_B_1;
	 double IRIk;
	 double IRIb;

	/// <summary>
	/// 平整度计算时，激光测距机的数据是否需要剔除异常值，正常激光测距机数据质量较好不需要，当路面有水或者沥青雾封罩面工艺的新路，测距机数据里面异常值比较多需要特殊处理
	/// </summary>
	 bool Las_Filter;
	 double Las_Filter_Thresh0;
	 double Las_Filter_Thresh1;

	/// <summary>
	/// 路面病害列表的报表里面是否要输出病害所在的路面图像名称和路径
	/// </summary>
	 int Out_roadimg;

	/// <summary>
	/// 导入多个工程的报表是否输出到同一个文件夹-0，或新建多个各自的文件夹-1
	/// </summary>
	 int Is_Multfolder;

	/// <summary>
	/// 当路面有水的时候，构造深度值特别小，用IRI_threshval来和路段的构造深度值比较，小于这个值就认为路面有水，调整IRI计算策略，只用加速度的位移来计算IRI
	/// </summary>
	 double IRI_threshval;

	/// <summary>
	/// CMOP调查表每页的行数，打印的时候用
	/// </summary>
	 int cmop_rows;

	/// <summary>
	/// 病害勾画选择   0--拉框  1--小方格，对于2018年的公路标准有区别，其他标准都用大框
	/// </summary>
	 int SelectDrawDis;

	/// <summary>
	/// 0--中交国通CPMS模板 1--按路面类型出整10* 米病害
	/// </summary>
	 int ZJGT_dismodel;

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

	/// <summary>
	/// 路面车辙病害的影响宽度0.4m，只对于小方格的2018等级公路会用到
	/// </summary>
	 double RutDisWidth;

	/// <summary>
	/// 水泥路面是否有刻槽，0-没有刻槽，1-有刻槽，有刻槽的高等级水泥路面PWI不参与PQI的计算
	/// </summary>
	 int Is_SnCarve;

	/// <summary>
	/// IRM窗口数据分析的时候是否显示计算车辙的包络线，默认是不给客户显示的
	/// </summary>
	 bool IsShowAnalysis;

	/// <summary>
	/// MPD的计算系数k
	/// </summary>
	 double MPD_K;

	/// <summary>
	/// MPD的计算系数b 
	/// </summary>
	 double MPD_B;

	/// <summary>
	/// 出报表的时候是否弹窗显示IRM计算失败
	/// </summary>
	 bool IsWarning;

	/// <summary>
	/// 0-整桩号分段，1-整里程分段，仅对城镇有区别，等级公路始终使用整桩号分段
	/// </summary>
	 int PartType;

	/// <summary>
	/// 整里程分段，分段区间的长度
	/// </summary>
	 int PartType_Dmi_Len;

	/// <summary>
	/// 生成报表的时候是否输出车速和打标备注，有的客户要，有的客户不要
	/// </summary>
	 int Out_roadinfo;

	/// <summary>
	/// 报表数值修约方式，0-四舍五入修约，1-奇进偶舍修约
	/// </summary>
	 int sheetRoundingOffType;

	/// <summary>
	/// 导出的报表中数值小数修约位数
	/// </summary>
	 int sheetRoundingOffNum;

	/// <summary>
	/// 和景观相关的，SCI和TCI报表数量
	/// </summary>
	 int StreetLenExcelNum;

	/// <summary>
	/// SCI和TCI的单元区间长度
	/// </summary>
	 vector<string> StreetLenExcel;

	/// <summary>
	/// 是否输出SCI和TCI的报表
	/// </summary>
	 vector<bool> StreetIsExcel;

	/// <summary>
	/// 是否将构造的激光测距值转换成明码输出，调试用
	/// </summary>
	 bool IsOutputLasval;

	/// <summary>
	/// 是否禁止病害框有重叠区域
	/// </summary>
	 bool IsForbidOverLapping;

	/// <summary>
	/// 是否给路面病害添加备注
	/// </summary>
	 bool IsCrackRemark;

	/// <summary>
	/// 跳秒，UTC时=GPS时-18（秒）
	/// </summary>
	 int GPSJumpTime;

	/// <summary>
	/// 平整度评定方式，0-取双轮迹平整度的平均值（默认），1-取双轮迹平整度的最大值
	/// </summary>
	 int RQIJudgeType = 0;

	/// <summary>
	/// 高速路采集了双轮迹的平整度报表导出设置，0-只导出左侧DAQ0，1-只导出右侧DAQ1，2-默认选项所有数据都导出
	/// </summary>
	 int IRIExcelSide = 2;

	/// <summary>
	/// 病害汇总表是否导出整公里小计，true-导出（默认），false-不导出
	/// </summary>
	 bool IsOutputDisAreaSubtotal = true;

	/// <summary>
	/// 是否检查平整度的同步时间，减少额外计算
	/// </summary>
	 bool IsCheckIRIGPSTime = false;
	//农村路是否需要进行惯导计算（惯导计算的工程项目 不支持 车辙，跳车等的出表 此处提供依据是否展示相应出表按钮）
	// bool isGDIriCalculate = false;

	/// <summary>
	/// 低等级农村路  是否进行图像校准   0 模块化 1高等级农村路 2低等级农村路 3畸变矫正 4 lm300 5 MM800 6 HM800
	/// </summary>
	 int isImageCorrect = 0;
	//低等级农村路设备参数 格式 真实宽度|真实高度（例如：3.5|2）
#ifdef UNICODE
	 string  real_HM800 = "";
	 string  real_lm300 = "";
	 string  real_MM800 = "";
#else
	 string  real_HM800 = "";
	 string  real_lm300 = "";
	 string  real_MM800 = "";
#endif // UNICODE

	
};


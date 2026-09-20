#pragma once
#include <QString>
#include <QVector>
#include "..\hnProject\hnProject.h"
#include "..\hnProject\hn2DProject.h"
#include "..\ReportConfig.h"
#include <QProgressDialog>
#include "xlsxdocument.h" 
#include "hnOutExcelMileManage.h"
#include "..\hnConfigService\HnXRSettings.h"
#include<QSharedPointer>

#include "..\hnQtCommon\EquipmentList.h" 
QXLSX_USE_NAMESPACE
//报表管理类
using namespace hnPro;
class HnXRSettings;
class hnOutExcelManage {

public:

	hnOutExcelManage();
	static void OutExcelManager(const QString&excelDir, const QString&selectModelTxt,
		HnProjectEnums::StandardParmTypeEnum standard,
		hnCommon::ROAD_WORK_TYPE DrawType,
		ReportItem*reportItem,
		hnPro::hnProject*curProject,
		double sMile = 0,
		double eMile = 0,
		int& progressValue = progressDefault,
		QProgressDialog* process = nullptr
	);



	static void OutExcelManager_Street(
		const QString& excelDir,
		int key,
		const QVector<double>& splits,
		HnProjectEnums::StandardParmTypeEnum standard,
		hnCommon::ROAD_WORK_TYPE DrawType,
		hnPro::hnProject*curProject,
		double sMile = 0,
		double eMile = 0
		, int& progressValue = progressDefault,
		QProgressDialog* process = nullptr
	);

public:


	//等级公路2018出表 人工模式
	static	void outBigRectExcel2018(const QString& saveExcelDir, const QString&selectModelTxt, int key, double xlslen, hnPro::hnProject*curProject, double sMile = 0, double eMile = 0);

	//等级公路2018出表 自动化模式
	static void outExcelSmallRectDegreeRoad2018(const QString& saveExcelDir, const QString&selectModelTxt, int   key, double xlslen, hnPro::hnProject*curProject, double sMile = 0, double eMile = 0);


	//等级公路2018出表 设计模式
	static void outDesignExcel2018(const QString& saveExcelDir, const QString&selectModelTxt, int  key, double xlslen, hnPro::hnProject*curProject, double sMile = 0, double eMile = 0);

	//城镇道路出表 人工模式
	static void outExcelCityRoad(const QString& saveExcelDir, const QString&selectModelTxt, int key, double xlslen, hnPro::hnProject*curProject, double sMile = 0, double eMile = 0);

	//城镇道路出表 自动化模式
	static  void outExcelSmallRectCityRoad(const QString& saveExcelDir, const QString& selectModelTxt, int key, double xlslen, hnPro::hnProject*curProject, double sMile = 0, double eMile = 0);


	//城镇道路出表 设计模式 
	static void outExcelDesignCityRoad(const QString& saveExcelDir, const QString&selectModelTxt, int  key, double xlslen, hnPro::hnProject*curProject, double sMile = 0, double eMile = 0);

	//  低等级农村公路出表 人工模式
	static void outBigRectExcelRuralRoadlowLevelRoad(const QString& saveExcelDir, const QString&selectModelTxt, int  key, double xlslen, hnPro::hnProject*curProject, double sMile = 0, double eMile = 0);

	//低等级农村公路出表 自动化模式
	static void ouSmallRectlRuralRoadlowLevelRoad(const QString& saveExcelDir, const QString&selectModelTxt, int  key, double xlslen, hnPro::hnProject*curProject, double sMile = 0, double eMile = 0);

private:
	static bool exportCPMSStreet(const QString& directory, int category, hnPro::hnProject* project);
	static bool exportRural5211(const QString& directory, int kind, double interval, hnPro::hnProject* project);
	//初始化分段区间
	static	bool initSegmentInterval(hnPro::hnProject * project,
		const double lenth, const MyQtCommon::MyEquipment& equip, double sMile = 0, double eMile = 0);

	//输出工程信息到表格中  sheet页的名字必须叫 ：工程信息 模板中也要有工程信息的模板，否则写入失败
	static bool exportProjectInfoSheet(Document &xlsx, hnPro::hnProject*curProject);

	//路面平整度评价等级记录表  贵州乾通
	static	bool exportIRIexcel_GZQT(const QString& saveExcelDir,const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	//路面综合评价等级记录表
	static	bool exportLMZHexcel(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);
	static bool exportLMZHexcel_GZQT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	//车辙深度评价登记记录表  RUT
	static	bool exportCZSDPJDJJLB(const QString& saveExcelDir,  const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);
	static	bool exportCZSDPJDJJLB_GZQT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	//路面磨耗评价等级记录表   
	static	bool exportLMMHPJDJB(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	//路面平整度评价等级记录表 
	static bool exportIRIexcel(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);


	//  pci 路面破损评价等级记录表
	static bool exportLMPSexcel(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);
	//  pci 路面破损评价等级记录表
	static bool exportLMPSexcel_City(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);


	//路面磨耗评价等级记录表   
	static	 bool exportLMMHPJDJB_GZQT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	//人工模式路面病害面积统计表
	static	 bool exportLMBHMJTJB_RECT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	//输出3d人工模式路面病害面积统计表
	static bool export3DLMBHMJTJB_RECT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject, double sMile, double eMile);

	//自动化模式路面病害面积统计表
	static	 bool exportLMBHMJTJB_Smart(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	//输出病害列表sheet页面
	static	 void  exportDiseaseAreaSheet(Document& xlsx, const QVector<hnCommon::hnRoadDiseaseInfo> diss, hnPro::hnProject*curProject);

	//输出gps数据
	static bool exportGPSExcel(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	//贵州乾通定制
	static bool exportLMBHMJTJB_Smart_GZQT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	//输出3d自动化模式路面病害面积统计表
	static	 bool export3DLMBHMJTJB_Smart(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject, double sMile, double eMile);


	//路面跳车评价等级记录表  PWI
	static	 bool exportLMTCPJDJJLB(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, const MyQtCommon::MyEquipment& equip, hnPro::hnProject*curProject);

	static bool exportLMTCPJDJJLB_GZQT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, const MyQtCommon::MyEquipment& equip, hnPro::hnProject*curProject);

	//CPMS路面病害调查表
	static bool exportCPMS_LMBHDCB(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	////路面构造深度评价等级记录表  
	static bool exportLMGZSDPJDJJLB(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);
	static bool exportLMGZSDPJDJJLB_City(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	//路面构造深度MPD评价等级记录表
	static bool exportGZSD_MPD_PJDJJLB(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	//路面几何状况检测数据统计表
	static bool exportLMJHZKJCSJTJB(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	//技术状况评定明细表
	static bool exportJSZKPDMXB(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);


	//沿线设置损坏汇总表
	static bool exportStreetSumExcel(const QString& saveExcelDir, const QString& modelBasePath, int type, double xlslen, hnPro::hnProject*curProject);



	//设计模式 水泥路面段落统计表
	static bool exporDesignSnDiseaseSum(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject);

	//跳车：路面跳车统计表
	static bool exportTC_LMTCTJB(Document &xlsx, hnPro::hnProject*curProject);

	//跳车：路面跳车纵断面高差统计表  10m表格
	static	 bool exportTC_LMTCZDMGCTJB(Document &xlsx, hnPro::hnProject*curProject);

	// 贵州乾通 输出工程信息到表格中 
	static bool exportProjectInfoSheet_GZQT(Document &xlsx, hnPro::hnProject*curProject);

	static	 bool  WritePrj2CPMSXls(Document&xlsx, QString sheetName, hnPro::hnProject*curProject);

	// 填写《xx病害统计表》sheet页，sheet页的名字必须是《xx病害统计表》,传入的Document也必须有相应的模板，否则写入失败。
	static	 bool writeDiseasesStatisticsSheet(Document &xlsx, int roadType, const QVector<hnCommon::hnRoadDiseaseInfo>& diss, HnProjectEnums::StandardParmTypeEnum type, hnPro::hnProject*curProject);

	// 填写《xx病害汇总表》sheet
	static bool writeDiseasesSumSheet(Document &xlsx, int roadType, QVector<hnOutExcelMile> & excelMiles, HnProjectEnums::StandardParmTypeEnum type, hnPro::hnProject*curProject);


	// 填写《沥青病害统计表》sheet页，sheet页的名字必须是《沥青病害统计表》,传入的Document也必须有相应的模板，否则写入失败。 
	static bool writeAsphaltDiseasesStatisticsSheet_Smart_QTDZ(Document &xlsx, const  QVector<hnCommon::hnRoadDiseaseInfo>& diss, hnPro::hnProject*curProject);

	// 填写《沥青病害汇总表》sheet页，sheet页的名字必须是《沥青病害汇总表》,传入的Document也必须有相应的模板，否则写入失败。 
	static bool writeAsphaltDiseasesSumSheet_Smart_QTDZ(Document &xlsx, QVector<hnOutExcelMile> & excelMiles, hnPro::hnProject*curProject);

	// 填写《水泥病害统计表》sheet页，sheet页的名字必须是《水泥病害统计表》,传入的Document也必须有相应的模板，否则写入失败。 
	static bool writeCementDiseasesStatisticsSheet_Smart_QTDZ(Document &xlsx, QVector<hnOutExcelMile> & excelMiles, hnPro::hnProject*curProject);

	// 填写《水泥病害汇总表》sheet页，sheet页的名字必须是《水泥病害统计表》,传入的Document也必须有相应的模板，否则写入失败。 
	static bool writeCementDiseasesSumSheet_Smart_QTDZ(Document &xlsx, QVector<hnOutExcelMile> & excelMiles, hnPro::hnProject*curProject);



	//等级公路2018景观病害输出
	static	 void outExcelStreetDegreeRoad2018(const QString& saveExcelDir, int  key, double xlslen, hnPro::hnProject*curProject, double sMile = 0, double eMile = 0);

	//低等级农村公路景观病害输出
	static  void outExcelStreetRuralRoadlowLevelRoad(const QString& saveExcelDir, int  key, double xlslen, hnPro::hnProject*curProject, double sMile = 0, double eMile = 0);
private://帮助
		//给表名加米 比如 123.xlsx  加完10米后是这样的：123_10m.xlsx
	static	QString addMetersToTable(const double meter, const QString &tableName);

	static  double getCloseMile(const double& value, hnPro::hnProject*curProject);

	//保存表格
	static bool saveExcel(const QString& saveDir, const QString& ExcelName, Document &xlsx);
private:

	static QSharedPointer<hnOutExcelMileManage>  m_outExcelMileManage;

	//单例  全局设置
	static HnXRSettings* m_xrSetting;
	static int progressDefault;
	//static QString saveExcelDir;
};
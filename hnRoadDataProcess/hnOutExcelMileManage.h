#pragma once
#include "HnProjectEnums.h"
#include <QVector>
#include "..\hnCommon\hnRoadStruct.h"
#include "..\hnProject\hnProject.h"
#include "hnOutExcelMile.h"
#include "../hnQtCommon/EquipmentList.h"
#include "RutParm.h"
//各项指标 计算 供出表
using namespace hnCommon;

class   hnOutExcelMileManage
{
public:
	//当前工程  起点桩号   终点桩号   分割区间
	//道路标准 起点桩号 终点桩号  分割区间使用 传入的  其他的  如公路等级 采用 project获得
	hnOutExcelMileManage(HnProjectEnums::StandardParmTypeEnum standard, hnPro::hnProject * project, 
		double sMile, double eMile, double splitValue,const MyQtCommon::MyEquipment& equips, bool ignoreMarksForExport = false);
	~hnOutExcelMileManage();

	double getStartMile() { return m_sMile; }
	double getEndMile() { return m_eMile; }
	//整数桩号转换成 字符形式  比如 599-> K0+599
	hnPro::hnProject *  getProject() { return m_project; }

	QVector<hnOutExcelMile> getRoadMessageVec();
	QVector<hnOutExcelMile> getRoadMessage_10m_Vec();
public:
	//获得平整度计算状态
	bool getIriState() { return m_iriOk; }
	//获得跳车计算状态
	bool getPbState() { return m_pbState; }
	//获得车辙计算状态
	bool getRutState() { return m_rutState; }

	bool getDataComplete() { return m_dataCompletion; }

	// Collect errors during a batch export; the dialog shows one deduplicated summary at the end.
	void reportExcelError(const QString& message);

	QVector<hnCommon::hnRoadDiseaseInfo> getRutDis() { return rutDiss; }
private:
	//根据传入分段区间进行分段
	QVector<hnOutExcelMile> m_roadSplietVec;

	//提供给跳车 以10m进行分段
	QVector<hnOutExcelMile> m_roadSplit_pwi10m_Vec;
	//车辙病害 10m分段辅助
	QVector<hnOutExcelMile> m_roadSplit_rutDis10m_Vec;

	//区间标准
	HnProjectEnums::StandardParmTypeEnum m_standard;

	//总起点 
	double m_sMile;
	//总终点
	double m_eMile;
	//每段的长度
	double m_xlslen;
	//1 上行 -1 下行 
	int m_direction; 
	hnPro::hnProject * m_project;
	QVector<hnMile> getMilesInRange(const QVector<hnMile>& miles, int line, double sMile, double eMile);

	//根据区间分段数据 和起点终点桩号 进行分段 保存到m_roadSplietVec
	//进行剩余的处理  给其各种参数进行赋值   设置各项指标参数
	bool handelRoadSplietVec(QVector<hnOutExcelMile>& miles);

	void getLuKuangCha(QVector<hnCommon::hnRoadDiseaseInfo>& arrdis);

	bool LoadRutData();

	//当用户选择自动导出车辙病害的时候，这里计算车辙值为后续计算提供依据
	bool getRutDisVal( QVector<hnOutExcelMile>& miles,QVector<double>& sRutVals, QVector<double>& sMiles );

	QVector<hnCommon::hnRoadDiseaseInfo> rutDiss;
	void setRutDis(const QVector<double>sRutVlas, const QVector<double>sMiles);

	//根据10m区间内的跳车值   计算当前设置区间跳车轻中重情况
	void handelPbiValues(QVector<hnOutExcelMile>& miles,const QVector<hnOutExcelMile>& mile_10m);

	//根据分段区间初步分段
	QVector<hnOutExcelMile>  splitMile(const QVector<hnMile>& firstMile, double xlsLen);

	// 城镇分段使用实际桩号，不能套用长短链的显示桩号。
	bool m_cityDistanceSegments = false;
	bool m_useDmiFormat = false;
	QVector<hnCommon::hnMarkInfo> cityReportMarks() const;
	QVector<hnOutExcelMile> splitCityMile(const QVector<hnCommon::hnMarkInfo>& marks);

	//根据里程分段
	QVector<hnOutExcelMile>  splitMile_dmi(const QVector<hnMile>& firstMile, double xlsLen);

	//根据打标处理分段列表
	// 公里评定在属性切分后、备注追加前自动合并农村路短单元。
	void handelMark(QVector<hnOutExcelMile>& miles, const QVector<hnCommon::hnMarkInfo> marks,
		bool evaluateKilometer = false);
	 
	//为每个分段写入平整度值
	 bool writeIriValue(bool& hasLeftIRI, bool& hasRightIRI, double BaseLen);

	//为每个分段写入车速
	 bool writeSpeedValue(QVector<hnOutExcelMile>& miles,double BaseLen);

	//写入跳车值
	bool writePbiValue(QVector<hnOutExcelMile>& miles);

	 //写入几何线型
	bool writeJHXXValue( double BaseLen);

	//辅助类  从文件获取跳车 纵断面数值
	void readPbiValueFromFile(QVector<hnOutExcelMile>& miles, QStringList& leftValues, QVector<double>& val);

	//为分段写入车辙信息
	bool writeRutValue();

	//为分段写入构造信息
	bool writeMtdValue();

	//为分段写入磨耗信息
	bool writeMpdValue();

	//为分段写入定位信息
	bool writeGpsStrValue();

	//在处理打标函数中  给新建立的ExcelMile赋值
	void initMarkMehtodExcelMile(hnOutExcelMile& newMile, const hnOutExcelMile& otherMile);

	

	QVector<hnRoadDiseaseInfo>  m_allVecData;
private:
	//查找与目标桩号最接近的 _EXCELGPS_
	_EXCELGPS_ findNearestGps(const QVector<_EXCELGPS_>& gpsInfos, double targetMile, int line);

	//为roadSplitVec中的每个元素填充 sGps和eGps
	void fillRoadSplitVec(QVector<_EXCELGPS_>gpsInfos, QVector<hnOutExcelMile>&roadSplitVec, int line);

	 
	//在 m_roadSplietVec中根据里程 找到前方最近桩号 位置   然后当前桩号就是 前方桩号+里程差
	double getCloseMile(const double& value);

	//工程设置信息
	hnProjectSetInfo  m_projectSet;
	//各种指标是否有效可用  出对应指标表的时候进行判断 
	//指标完整标志
	bool m_dataCompletion;

	//平整度
	bool m_iriOk;
	//速度
	bool m_speedOk;
	 
	//跳车
	bool m_pbState; 

	//mpd
	bool m_mpdState;

	//mtd
	bool m_mtdState;

	//gps
	bool m_gpsState;
	//几何线型
	bool m_jhxxState;
	//车辙
	bool m_rutState;
	//计算各种指标的时候   用这个参数来判断 哪些需要计算;
	MyQtCommon::MyEquipment m_equipMentList;

	

	//单例  全局设置
	HnXRSettings* m_xrSetting;

	//计算路况差的辅助参数 
	QStringList rutfilepaths_L;
	QStringList rutfilePaths_R;
	RutParm rutparm_L  ;
	RutParm rutparm_R  ; 

	QVector<uint8_t> rbarr;
	QVector<int16_t> profile;
	QVector<float> profileZ;
	QVector<float> profileZtmp; 
};

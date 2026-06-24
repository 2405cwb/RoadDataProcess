#pragma once
#include <QString>
#include "../hnProject/hnProject.h"
#include "..\hnConfigService\HnXRSettings.h"
#include <QString> 
#include "StreetDiseaseManage.h"
#include "..\hnQtCommon\ExcelGPS.h"
struct  hn_RoadDiseaseType
{
	QVector < QVector<double>>_MiduScore;
	 
	hnDiseaseSetInfo type;
	double d_sumArea;
};
class  hnOutExcelMile
{
public:
	hnOutExcelMile();
	hnOutExcelMile(const hnOutExcelMile&) = default;
	hnOutExcelMile(hnPro::hnProject *, HnProjectEnums::StandardParmTypeEnum type); 
	~hnOutExcelMile();

	 
public: 
	//路面材质
	ROAD_SURFACE_TYPE RoadSurface;

	QString RoadSurfaceStr;
	HnProjectEnums::StandardParmTypeEnum Type;
	/// <summary>
	/// 第i到i+1个里程区间内的公路等级，0-高速、一级，1-二三四级 或者 0-快速路，1-主干路次干路，2-支路
	/// </summary>
	int RoadGrad;
	//道路等级文字
	QString RoadDegreestr;  
	// 1- 上行 -1-下行
	int  Direction;



public: 
	void setStartMile(double value);

	double getStartMile() const;

	void setEndMile(double mile);

	double getEndMile() const;

	void setStartDmi(double value);

	double getStartDmi() const;

	void setEndDmi(double dmi);

	double getEndDmi() const;



	double getRoadLength() const;


	//设置备注
	void setUnitStr(QString str);

	QString getUnitStr() const;

	//获取速度
	double getSpeed();

	void setSpeed(double value);
	//设置起点gps
	 void setStartGpsInfo( _EXCELGPS_ value);

	//设置终点结束
	void setEndGpsInfo( _EXCELGPS_ value);

	//设置左边平整度
	void setLeftIriValue(double value);

	//获得左边平整度
	double getLeftIriValue();

	//设置右边平整度
	void setRightIriValue(double value);
	
	//获得右边平整度
	double getRightIriValue();  

	//设置评价平整度值
	double getJudgeIirValue();

	//获得最大平整度
	double getMaxIriValue();



	//获得平整度指标评价
	QString getIriExcelStr();

	//获得DR值
	double getDRScore();
   void	setDrScore(double value);

	//获得路面破损Pci评价
	QString getPCIExcelStr();



	QString getPciEvaluateStr(QString colStr, int row);
	QString getMtdEvaluateStr(QString colStr, int row);
	//当用户需要自动导出车辙病害的时候，这里设置当前段的车辙值
	void setRutDisVlaue(double value );

	void setRightRutValue(double value);
	void setMaxRutValue(double value);
    double getRightRutValue( );
	void setLeftRutValue(double value);
 

     _EXCELGPS_ getStartGpsStr();
	 _EXCELGPS_ getEndGpsStr();
	double getLeftRutValue();
	void setjudgeRutValue(double value);
	 
	double getjudgeRutValue();
	double getMaxRutValue();

	void setLeftPbValue(double value);
	void setRightPbValue(double value);

 double getLeftPbValue 	(int length )const;

	double getRightPbValue(int length) const;

	QString getjudgePbValue(int length);
	//获取跳车程度
	QString getPbEvaluateStr();
	void setLeftMtdValue(double value);
	void setRightMtdValue(double value);
	void setCenterMtdValue(double value);

	double getLeftMtdValue( );
	double getRightMtdValue( );
	double getCenterMtdValue( );


	double getPwiValue( );



	void setLeftMpdValue(double value);
	void setRightMpdValue(double value);
	void setCenterMpdValue(double value);

	double  getLeftMpdValue( );
	double  getRightMpdValue();
	double  getCenterMpdValue( );
	double getjudgePbiValue();
	QString getRutExcelStr();
	QString getRutMaxExcelStr();
	QString getRutEvaluateStr(QString colStr, int row);
	QString getIriEvaluateStr(QString colStr,int row);
	QString getPBIScore();
	QString getPbiEvaluateStr(QString colStr, int row);
	QString getPwiValueStr();
	double getMtdWrValue();

	//获取代表SMTD值
	double getRepresentSMtdValue();
	 

	double getMpdWrValue();
	QString getPwiEvaluateStr(); 
	QString getMpdValueStr(QString colStr, int row);


	QString getMpdEvaluateStr(QString colStr, int row);
	

	//设置曲率
	void setCurvature(double value);
	double getCurvature();

	//设置纵坡
	void setLongitudianalSlope(double value);
	double getLongitudianalSlope();

	//设置横坡
	void setCrossSlope(double value);
	double getCrossSlope();

	//获得该路段道路计算参数
	hnRoadTypeSetInfo getRoadTypeSetInfo();

	void   setPbiNumbers(int index);
	int  getPbiNumber(int index);
	QString getMqiValue(int rowCount, QString sciIndex, QString pqiIndex, QString bciIndex, QString tciIndex);
	QString getPqiValue(int rowCount, QString pciIndex, QString rqiIndex, QString rdiIndex, QString pbiIndex, QString pwiIndex);
	QString getPqiEvaluateStr(int rowCnt,QString pqiIndex);
	void setBptt(bool has);
	void setTciValue(double value);
	void setSciValue(double value);

	double getTciValue();
	double getSciValue();
	//获取pci评价
	QString getTciEvaluate();
	QString getSciEvaluate();
	public:
		//注意这里面返回的病害 
		QVector<hnCommon::hnRoadDiseaseInfo> getRoadDisVec() { return m_roadDisVec; }
		QVector<hnCommon::hnRoadDiseaseInfo> getStreetVec() { return m_streetDisVec; }
		QVector<StreetDiseaseManage>  getStreetYxMap() { return m_streetYXDisManageVec; }
		QVector<StreetDiseaseManage>  getStreetLjMap() { return  m_streetLjDisManageVec; }
		operator QString() const
		{

			QString message;
			int roadType1 = static_cast<int>(RoadSurface);
			QString typeStr = roadType1 == 0 ? QStringLiteral("沥青") : roadType1 == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
			message = QStringLiteral("_区间起点桩号:") + QString::number(StartMile) + QStringLiteral("\n") +
				QStringLiteral("区间终点桩号") + QString::number(EndMile) + QStringLiteral("\n") +
				QStringLiteral("路面材质") + typeStr + QStringLiteral("\n") +
				QStringLiteral("道路等级") + RoadDegreestr + QStringLiteral("\n") +
				QStringLiteral("道路单元") + UnitStr + QStringLiteral("\n");
			return message;
		}
		// 根据当前区间  填充相应的hnMile Vector  病害vecotr
		bool StartCalculate(bool onlyInitSetInfo /*= false */);

		//计算破损 pci 评价指标  里面会获得区间病害  传入 道路宽度
		bool calculateDrScore(double width, QVector<hnCommon::hnRoadDiseaseInfo>& diss);
		//计算 rqi  平整度 评价指标
		bool calculateRQIScore(bool hasleftValue, bool hasRightValue);
		//计算pbi 跳车 评价指标
		bool calculatePBIScore();
		//计算rut  车辙 评价指标
		bool calculateRUTScore();

		// 1 沿线设施  2 路基损坏
		bool calcaulateStreetScore(int type, QVector<hnCommon::hnRoadDiseaseInfo>& diss, QVector<hnDiseaseSetInfo>& disSetting);
		//计算PWI评价
		bool calculatePwiScore();
		//城镇路计算pci 差值算法
		double ChaZhi(QVector<QVector<double>>  MiduScore, double mval);

		 
public:
	//生成各项分数评价字符串  ,   需要列对应的英文字母， 行数
	void getallEvaluate1(const QString&colStr, int row);
	void GetRutDis(QVector<hnCommon::hnRoadDiseaseInfo> &rutDis, int side = 0);
private:

	void setRoadLength(double length);

	//终点
	double EndMile;

	//起点
	double StartMile;

	 
	double StartDmi;
	 
	double EndDmi;

	//道路长度
	double RoadLength;

	//路面单元文字  备注
	QString UnitStr;
	//速度
	int SpeedVal;

	double Tci;
	double Sci;
	//指标分数 \
#pragma region pci
	double DRScore;
	QString PCIExcelStr;

	// 需要调用getallEvaluate();才有值
	QString PciEvaluateStr;
	QString MtdEvaluateStr;
#pragma endregion
#pragma region IRI
	double LeftIriValue;
	double RightIriValue;
	double judgeIirValue;
	QString IriExcelStr;
	 
	QString IriEvaluateStr;

#pragma endregion

#pragma region PBI
    //Δhsheet页面

	//左跳车值		仅10可用
	double LeftPbValue;
	//右跳车值		 仅10可用
	double RightPbValue; 
	//代表跳车值   仅10m可用
	double judgePbiValue;



	QString judgePbStr;
	QString PbEvaluateStr;
	 QString PBIScoreStr;
	//pbi sheet页面   跳车 轻，中，重
	int  PbiNumbers[4]{0};
#pragma endregion

#pragma region Rut
	double LeftRutValue;
	double RutMaxValue;
	double RightRutValue;
	double judgeRutValue;
	QString RutExcelStr;
	QString RutMaxExcelStr;
	// 需要调用getallEvaluate();才有值
	QString RutEvaluateStr;
	//区间车辙值 供计算车辙病害
	double m_sRutVal;
#pragma endregion

#pragma region Mtd
	double LeftMtdValue;
	double RightMtdValue;
	double CenterMtdValue;

//	QString MtdValueStr;

	QString PwiValueStr;
	QString PwiEvaluateStr;
#pragma endregion

#pragma region Mpd
	double LeftMpdValue;
	double RightMpdValue;
	double CenterMpdValue;
	
	QString MpdValueStr;
	QString MpdEvaluateStr;
#pragma endregion 
#pragma region Jhxx
	double Curvature;
	double LongitudianalSlope;
	double CrossSlope;
	 
#pragma endregion 

	 _EXCELGPS_  StartGpsInfo;
 	_EXCELGPS_ EndGpsInfo; 
	//存在边坡坍塌的时 mqi为0 //等级公路2018
	bool hasBPTT; 
private:
	 
	//当前区间  路面病害列表
	QVector<hnCommon::hnRoadDiseaseInfo> m_roadDisVec;

	//当前区间  景观病害列表
	QVector<hnCommon::hnRoadDiseaseInfo> m_streetDisVec;
	//记录景观病害  临时用 外部访问用  m_streetLjDisManageMap  |m_streetYXDisManageMap
	QMap<QString, StreetDiseaseManage> m_streetDisManageMap;
	//路基
	QVector<StreetDiseaseManage>  m_streetLjDisManageVec; 
	//沿线
		QVector<StreetDiseaseManage>  m_streetYXDisManageVec;

	//当前区间 桩号列表
	//QVector<hnMile> m_currentMileVec;

	hnPro::hnProject* m_project;

	hnRoadTypeSetInfo m_roadTypeSetInfo;

	


	//单例  全局设置
	HnXRSettings* m_xrSetting;
};


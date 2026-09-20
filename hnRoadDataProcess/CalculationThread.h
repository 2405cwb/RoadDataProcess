#pragma once
#include <QObject>
#include <QThread>
#include <QMap>
#include "../hnApplication/hnDataManager.h"
#include "../hnProject/hnProjectManager.h"
#include <QMutex>
#include <QPair>
#include "..\hnConfigService\HnXRSettings.h"
#include <iostream>
#include <memory>
#include <random>
#include <chrono>
#include <mutex>
#include<atomic>
#include <QRunnable>
#include <tuple>
using tuple_bool_vec_vec_vec_int = std::tuple<bool, std::vector<double>, std::vector<double>, std::vector<double>, int>;
using tuple_vec_vec_vec_int_int = std::tuple<std::vector<double>, std::vector<double>, std::vector<int>, std::vector<std::string>, int>;
using tuple_vec_int = std::tuple<std::vector<double>, std::vector<int>>;
class HnXRSettings;
class CalculationThread :public QObject, public QRunnable
{
	Q_OBJECT

public:
	enum CalculationType
	{
		//平整度
		IRI,
		//车辙
		Rut,
		//磨耗
	 
		Smtd,

		//
		Smpd,

		Jhxx
	};
public:
	CalculationThread(CalculationType type, int taskId, hnPro::hnProject* const project);
	~CalculationThread();
	void run() override; 
	CalculationType calculationType() const { return m_type; }
	hnPro::hnProject* project() const { return m_Project; }
signals:
	void progressUpdated(hnPro::hnProject* project, int calculationType,double progress,bool isInt);

	void error(QString message);

	void stopped();

public slots:
	void stop();
	int taskId()const { return m_taskID; }
private:
	CalculationType m_type;
	std::atomic<bool> m_stopRequested;
	int m_taskID;
	QMutex m_mutex;
private:
	void StartIRMThread(hnPro::hnProject* pro, const QString  iriPath,const QString & daqBasePath, const QString& resamplePath,int side);
	void  CheckSetting( hnPro::hnProject* const);
private:
	//剔除激光测距机里面的异常值，resample.txt文件
	void FilterLaserData(QString path, double thresh1=5, double thresh2=20);

	//单例  全局设置
	HnXRSettings* _Setting;
	hnPro::hnProject* currentPorject ;
private: //计算函数 
	void startCalculateIRI( QString dataPath, const QString& outPath,double dIntervel, vector<double> listIRI,const QVector<double>speeds, bool datasrc);

#pragma region 修正后平整度算法 可计算0.1与0.25

	void GenerateIRI_NEW(const std::string& fpath, int vallen, const std::string& fname, bool datasrc, int effectiveLength);
	// 加载速度修正参数从 Coeff.dat
	tuple_bool_vec_vec_vec_int LoadParameters(const std::string& fpath, const std::string& fname);

	// 加载和解析 resample.txt 数据
	tuple_vec_vec_vec_int_int LoadData(const std::string& fpath);

	// 抽样数据到指定间隔（0.1m 或 0.25m）
	tuple_vec_int ResampleData(const std::vector<double>& oridata, const std::vector<int>& oritime, int len, int qplusenum);
	// 初始化状态变量 oldZSU
	std::vector<double> InitializeState(const std::vector<double>& iridata, double DeltLen, int len);
	// 解析时间戳为秒（支持脉冲计数或 HHMMSSmmm 格式）
	double ParseTime(int timeValue);
	// 计算 YSU（输入坡度）
	double ComputeYSU(const std::vector<double>& iridata, int i, double DeltLen);
	// 更新状态向量 ZSU
	void UpdateState(std::vector<double>& ZSU, const std::vector<double>& oldZSU, double YSU, double DeltLen);


	// 应用 IRI 修正（加速度或速度因子）
	double ApplyCorrection(double irival, bool datasrc, double speedval, bool isParmFile,
		const std::vector<double>& speedparms, const std::vector<double>& kparms,
		const std::vector<double>& bparms, int parmnum);
#pragma endregion

	 
	bool ComputeRut(bool Isbar, int valnum, int rutmode);
	void AdjustRutVal(int valnum);
	void CreateGaussFilter(std::unique_ptr<float[]>&  gaus, int size, float sigma);
	bool JudgMTDval(QString prj,int side);
	//单侧的车辙前后之间调整异常值
	void RemoveBigErr(float* rutval,int length);
	bool loadParm(bool isIri,int side);

	//构造深度参数获取
	double GetLaserThresh(const QString& fpath);
	void ComputeMTD(const QString& prj, int side, int featurelen, double threshval);
	void 	AdjustVal(QString fname, double Thrval, double scale);
	void ComputeMPD(const QString& prj, int side, int featurelen, double threshval);
	
private:
	//随机数
	  std::uniform_int_distribution<>dis;
	  double  m_IRI_k;
	  double  m_IRI_b;
	  bool m_speedtype;
	  int m_Frequency;
	  QVector < QVector < double >> _MTDCali;
	  QVector < QVector < double >>_MPDCali;
	  QVector < double >_mmPerPoint;
	 // std::mutex  mutex;
	  QMutex m_mmutex;
	  QMutex m_smutex;

	 
	  int m_stdProcessValue;
	  int m_spdProcessValue;

	  hnPro::hnProject* m_Project;
};

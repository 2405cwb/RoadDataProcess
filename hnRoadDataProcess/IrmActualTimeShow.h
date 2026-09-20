#pragma once 
#include <QWidget> 
#include <QVBoxLayout>
#include <QVector>
#include <QtCharts/QLineSeries>
#include "QtCharts/QtCharts" 
#include "../hnRoadDataProcess/CustomChartView.h"
#include "..\hnApplication\hnDataManager.h"
#include "..\hnQtCommon\MyCommonMethods.h"
using namespace QtCharts;
class IrmActualTimeShow : public QWidget
{
	Q_OBJECT
public:
	IrmActualTimeShow(QWidget *parent);
	~IrmActualTimeShow();
signals:
	void requestCalculation();
public slots:
	void resetData();
//更新IRI界面
void slot_updateIriFormSlots(double mile,double dmi);
void selectLengthTextChanged(const QString& str);
void selectLengthChanged(int index);

private:
	//初始化UI
	void setupUI();

	//清空图表
	void clearChart();

	void updateIriCharts(int startIndex);
	
	//更新折线
	void updateTargetChart(int& startIndex, int& endIndex, int length, const QVector<double>& targeDatas, QVector<double>&datas, QLineSeries* series);
	
	//更新横纵坐标
	void updateValueAxis(int sIndex,int eSindex, QChart* chart,const QVector<double>& datas );
	hnPro::hnProject * curProject = nullptr;

	hnPro::hn2DProject* cur2DProject = nullptr;
	QLabel* m_emptyHint = nullptr;

	//当前用户设定区间  10m一个点
	int curUserLength = 100;

	//当前左边平整度值
	QVector<double> curLeftIriDatas;

	//当前右边平整度值
	QVector<double> curRightIriDatas;
	 

	//当前左边车辙值
	QVector<double> curLeftRutDatas;

	//当前右边车辙值
	QVector<double> curRightRutDatas;

	//当前构造深度值
	QVector<double> curLeftSmtdDatas;
	QVector<double> curRightSmtdDatas; 
	QVector<double> curCenterSmtdDatas; 


	QVBoxLayout *mainLayout;

	//左边平整度
	QLineSeries* leftIriSeries;

	//右边平整度
	QLineSeries* rightIriSeries;

	//左SMTD
	QLineSeries* leftSmtdSeries;

	//右SMTD
	QLineSeries* rightSmtdSeries;

	//中SMTD
	QLineSeries* centerSmtdSeries;

	//左边rut
	QLineSeries* leftRutSeries;

	//右边rut
	QLineSeries* rightRutSeries;


	QChart* iriChartForm;

	QChart* rutChartForm;

	QChart* smtdChartForm;

};

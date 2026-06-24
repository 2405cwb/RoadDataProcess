#pragma once
#include <QtCharts/QLineSeries>
#include "QtCharts/QtCharts" 

class CustomChartView : public QChartView
{
	Q_OBJECT

public:
	CustomChartView(QChart *chart = nullptr);
	~CustomChartView();

protected:
	void mouseMoveEvent(QMouseEvent *event) override;
};

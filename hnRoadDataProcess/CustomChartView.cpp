#include "CustomChartView.h"

 

CustomChartView::CustomChartView(QChart *chart /*= nullptr*/) : QChartView(chart)
{
	setMouseTracking(true);
	setToolTip("X: 0,Y: 0");
}

CustomChartView::~CustomChartView()
{
}

void CustomChartView::mouseMoveEvent(QMouseEvent *event) 
{
	QPointF mousePos = event->pos();
	QChart * chart = this->chart();
	QPointF chartPos = chart->mapToValue(mousePos);

	//存储所有系列信息
	struct PointInfo {
		QPointF point;
		QString seriesName;
		double distance;
	};

	QList<PointInfo> nearbyPoints;
	//遍历所有系列
	for (auto abstractSeries:chart->series())
	{
		QLineSeries * series = qobject_cast<QLineSeries*>(abstractSeries);
		if (series)
		{
			//获取名称
			QString seriseName = series->name();
			if (seriseName.isEmpty())
			{
				seriseName = QString("Series %1").arg(chart->series().indexOf(series) + 1);
			}
			//查找最近点
			for (const QPointF &point : series->points())
			{
				double distance = QLineF(chartPos, point).length();
				if (distance<5)
				{ 
						nearbyPoints.append({ point,seriseName,distance }); 	 
				}
			}
		}
	}

	if (!nearbyPoints.isEmpty())
	{
		std::sort(nearbyPoints.begin(), nearbyPoints.end(), [](const PointInfo &a, const PointInfo & b)
		{
			return a.distance < b.distance;
		});
		QString tooltip;
		if (!tooltip.isEmpty())
		{
			tooltip += "\n";
		}
		tooltip += QString("%1: X=%2,Y=%3").arg(nearbyPoints.first().seriesName)
			.arg(nearbyPoints.first().point.x(), 0, 'f', 2)
			.arg(nearbyPoints.first().point.y(), 0, 'f', 2);
		setToolTip(tooltip);
	}
	else
	{
		setToolTip("");
	} 
	QChartView::mouseMoveEvent(event);
}


#pragma once

#include <QWidget>
#include <QWebEngineView>
#include <QPushButton>
#include <QLineEdit>
#include <QCloseEvent>
#include <QWebChannel>
struct GpsInfo
{
	GpsInfo(QString msg, QPointF point)
	{
		gps = point;
		MileInfo = msg;
	}
	QString MileInfo;
	// x lat  y :long
	QPointF gps;
};
// 坐标转换函数
struct Point {
	double lng;
	double lat;
};
class CustomBaiduMapView : public QWidget
{
	Q_OBJECT

public:
	CustomBaiduMapView(QWidget *parent);
	~CustomBaiduMapView();

	Q_INVOKABLE double GetLog();
	Q_INVOKABLE double GetLat();
	Q_INVOKABLE int GetMilePointNum();
	Q_INVOKABLE double GetMileLog();
	Q_INVOKABLE double GetMileLat();
	Q_INVOKABLE QString GetMileStr();
	Q_INVOKABLE void SetCurPoint(double lat, double lng);
signals:
	void signal_loadBaiDuMap();

	public slots:
	void onShowRoadClicked();
	void onShowMileClicked();
	void onClearClicked();
	void onAddMarkClicked();
	void onShowStartClicked();
	 
	void captureScreenshot(); 
private:
	 
	void setupUI();
	void setupWebEngine();

	void  loadGpsData(); //加载GPS数据


	//地图控件
	QWebEngineView * m_webView;
	QWebChannel * m_webChannel;
	QLineEdit * m_coordEdit; 
	QLineEdit * m_tagEdit;
	    

private:
	QList<GpsInfo> m_gpsData;
	QList<GpsInfo> m_mileData;

	double m_curLng =114.360734;
	double m_curLat = 30.541093;
	int m_pointCnt = 0;
	int m_mileCnt = 0;
};

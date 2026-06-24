#include "CustomBaiduMapView.h"
#include <QVBoxLayout>
#include <QFileInfo>
#include <QCoreApplication> 
#include <QMessageBox> 
#include <QPushButton> 
#include <QFileDialog>
#include <QTimer>
#include "../hnProject/hnProject.h"
#include "../hnProject/hn2DProject.h"
#include "..\hnApplication\hnDataManager.h"
#include "..\hnQtCommon\MyCommonMethods.h"
#include <QJsonArray>
#include <QJsonDocument>
CustomBaiduMapView::CustomBaiduMapView(QWidget *parent)
	: QWidget(parent),m_webView(nullptr)
{
	setupUI();
	//读取数据
	 
	connect(this, &CustomBaiduMapView::signal_loadBaiDuMap,this, [&]() { 
		loadGpsData(); 
		setupWebEngine();
	});
	 
	
}

CustomBaiduMapView::~CustomBaiduMapView()
{
	 if (m_webView &&m_webView->page())
	 {
		 m_webView->page()->setWebChannel(nullptr);
	 }
	 if (m_webChannel)
	 {
		 m_webChannel->deregisterObject(this);
	 }
}

 
void CustomBaiduMapView::setupWebEngine()
{
	m_webChannel = new QWebChannel(this);
	m_webChannel->registerObject("external", this);
	m_webView->page()->setWebChannel(m_webChannel);

	QString htmlPaht = QCoreApplication::applicationDirPath() + "/tianditu.html";
	if (QFile::exists(htmlPaht))
	{
		m_webView->setUrl(QUrl::fromLocalFile(htmlPaht));
	}
	else
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("错误"), QStringLiteral("未找到地图文件"));
	}

}


//-----------------c++供js异步调用的接口


double CustomBaiduMapView::GetLog()
{
	double log = m_curLng;
	if (m_pointCnt < m_gpsData.size())
	{
		log = m_gpsData[m_pointCnt].gps.x();
	}
	return log;
}

double CustomBaiduMapView::GetLat()
{
	double lat = m_curLat;
	if (m_pointCnt < m_gpsData.size())
	{
		lat = m_gpsData[m_pointCnt].gps.y();
		m_pointCnt++;
	}
	return lat;
}

int CustomBaiduMapView::GetMilePointNum()
{
	return m_mileData.size();
}

double CustomBaiduMapView::GetMileLog()
{
	if (m_mileCnt < m_mileData.size())
	{
		return m_mileData[m_mileCnt].gps.x();
	}
	return m_curLng;
}

double CustomBaiduMapView::GetMileLat()
{
	if (m_mileCnt < m_mileData.size())
	{
		return m_mileData[m_mileCnt].gps.y();
	}
	return m_curLat;
}

QString CustomBaiduMapView::GetMileStr()
{
	QString str = "";
	if (m_mileCnt < m_mileData.size())
	{
		str = m_mileData[m_mileCnt].MileInfo;
		m_mileCnt++;
	}
	return str;
}

void CustomBaiduMapView::SetCurPoint(double lat, double lng)
{
	m_curLat = lat;
	m_curLng = lng;
	m_coordEdit->setText(QString(QString::fromLocal8Bit("经度:%1,纬度:%2")).arg(lng, 0, 'f', 6).arg(lat, 0, 'f', 6));
}

//界面按钮槽函数

void CustomBaiduMapView::onShowRoadClicked()
{
	QJsonArray coordsArray;
	for (const auto & pt : m_gpsData)
	{
		QJsonArray point; 
		point.append(pt.gps.x());
		point.append(pt.gps.y());
		coordsArray.append(point);
	}
	QJsonDocument doc(coordsArray);
	QString jsonStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
	
	QString script = QString("LoadNewProjectTrajectory(%1);").arg(jsonStr);
	m_webView->page()->runJavaScript(jsonStr);
}

void CustomBaiduMapView::onShowMileClicked()
{
	m_mileCnt = 0;
	m_webView->page()->runJavaScript("ShowMile();");
}

void CustomBaiduMapView::onClearClicked()
{
	m_webView->page()->runJavaScript("ClearAll();");
}

void CustomBaiduMapView::onAddMarkClicked()
{
	if (m_tagEdit->text().isEmpty())
	{
		return;
	}
	QString text = m_tagEdit->text().replace("'", "\\'");
	QString script = QString("AddMark(%1,%2,'%3');")
		.arg(m_curLat, 0, 'f', 6).arg(m_curLng, 0, 'f', 6).arg(text);
	m_webView->page()->runJavaScript(script);
}

void CustomBaiduMapView::onShowStartClicked()
{
	if (m_gpsData.isEmpty())
	{
		return;
	}
	double startLat = m_gpsData.first().gps.y();
	double  startLng = m_gpsData.first().gps.x();

	QString script = QString("ShowStart(%1,%2);")
		.arg(startLat, 0, 'f', 6)
		.arg(startLng, 0, 'f', 6);
	m_webView->page()->runJavaScript(script);
} 
void CustomBaiduMapView::setupUI()
{
	//this->resize(1000, 700);
	QVBoxLayout * mainLayout = new QVBoxLayout(this); 
	QHBoxLayout * topLayout = new QHBoxLayout;
	QPushButton* btnShowStart = new QPushButton(QStringLiteral("定位到起点"), this);
	QPushButton* btnShowRoad = new QPushButton(QStringLiteral("显示轨迹"), this);
	QPushButton* btnShowMile = new QPushButton(QStringLiteral("显示桩号"), this);
	QPushButton* btnClear = new QPushButton(QStringLiteral("清空地图"), this);

	m_coordEdit = new QLineEdit;
	m_coordEdit->setPlaceholderText(QStringLiteral("点击地图获取坐标..."));
	m_coordEdit->setReadOnly(true);

	m_tagEdit = new QLineEdit;
	m_tagEdit->setPlaceholderText(QStringLiteral("输入自定义标签..."));

	QPushButton * btnAddMark = new QPushButton(QStringLiteral("添加标签"));
	topLayout->addWidget(btnShowStart);
	topLayout->addWidget(btnShowRoad);
	topLayout->addWidget(btnShowMile);
	topLayout->addWidget(btnClear);
	topLayout->addWidget(m_coordEdit);
	topLayout->addWidget(m_tagEdit);
	topLayout->addWidget(btnAddMark);

	m_webView = new QWebEngineView;
	mainLayout->addLayout(topLayout);
	mainLayout->addWidget(m_webView);

	//绑定事件
	connect(btnShowRoad, &QPushButton::clicked, this, &CustomBaiduMapView::onShowRoadClicked);
	connect(btnShowMile, &QPushButton::clicked, this, &CustomBaiduMapView::onShowMileClicked);
	connect(btnClear, &QPushButton::clicked, this, &CustomBaiduMapView::onClearClicked);
	connect(btnAddMark, &QPushButton::clicked, this, &CustomBaiduMapView::onAddMarkClicked);
	connect(btnShowStart, &QPushButton::clicked, this, &CustomBaiduMapView::onShowStartClicked);

	//mainLayout->setContentsMargins(0, 0, 0, 0); 
} 
void  CustomBaiduMapView::loadGpsData()
{
	m_gpsData.clear();
	m_mileData.clear();
	auto dataManager = hnApp::hnDataManager::getDataManager();
	hnPro::hnProject* curProject = dataManager->getCurrentProject();

	//弹出界面
	if (!dataManager->isOpenProject())
	{
		return ;
	}
	if (curProject->getProjectType() == PROJECT_TYPE::PROJECT_23D_TYPE || curProject->getProjectType() == PROJECT_TYPE::PROJECT_2D_TYPE)
	{
		//获取gps信息
		QString gpsFilePath = curProject->get2DProject()->getGpsResultFilePath();
		QFile gpsFile(gpsFilePath);
		if (!gpsFile.exists())
		{
			return ;
		}
		QFile file(gpsFilePath);

		if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			qDebug() << QStringLiteral( "无法打开文件：" )<< gpsFilePath;
			return ;
		}

		QTextStream ts(&file);
		int counter = 0;
		int oldMile = 0;
		int mile = 0;
		int direction = curProject->getCurProSetInfo().nLineType; // 假设方向为正（可根据需要修改为动态配置）
		int totalLines = 0;

		// 先计算总行数以确定终点
		QStringList lines;
		while (!ts.atEnd()) {
			lines.append(ts.readLine());
		}
		totalLines = lines.size();

		// 重置流以重新读取
		file.seek(0);
		ts.seek(0);

		while (!ts.atEnd() && counter < totalLines) {
			QString line = lines[counter];
			QStringList sp = line.split(" ");
			if (sp.size() < 6) { // 确保有6个字段
				counter++;
				continue;
			}

			double lat = sp.at(1).toDouble();
			double lon = sp.at(2).toDouble();
			mile = sp.at(5).toInt();

			QPointF curPoint(lat, lon);
			GpsInfo curGps("", curPoint);
			m_gpsData.push_back(curGps);
			// 起点
			if (counter == 0) {
				QPointF point(lat, lon);
				QString msg = QStringLiteral("起点 K%1+%2").arg(mile / 1000).arg(mile % 1000, 3, 10, QChar('0'));
				GpsInfo gps(msg, point);
				m_mileData.push_back(gps);
			}
			// 终点
			else if (counter == totalLines - 1) {
				QPointF point(lat, lon);
				QString msg = QStringLiteral("终点 K%1+%2").arg(mile / 1000).arg(mile % 1000, 3, 10, QChar('0'));
				GpsInfo gps(msg, point);
				m_mileData.push_back(gps);
			}
			// 中间每公里点
			else {
				if (direction > 0) { // 里程递增
					int tt1000 = (oldMile + 500) / 1000 * 1000;
					if (oldMile < tt1000 && mile >= tt1000) {
						QPointF point(lat, lon);
						QString msg = QStringLiteral("K%1").arg(tt1000 / 1000);
						GpsInfo gps(msg, point);
						m_mileData.push_back(gps);
					}
				}
				else { // 里程递减
					int tt1000 = (oldMile + 500) / 1000 * 1000;
					if (oldMile > tt1000 && mile <= tt1000) {
						QPointF point(lat, lon);
						QString msg = QStringLiteral("K%1").arg(tt1000 / 1000);
						GpsInfo gps(msg, point);
						m_mileData.push_back(gps);
					}
				}
			}

			oldMile = mile;
			counter++;
		}

		file.close();
	}
}
void CustomBaiduMapView::captureScreenshot()
{
	// 应用特效：变暗并添加绿色边框
	m_webView->page()->runJavaScript(R"(
        var mapContainer = document.getElementById('map');
        if (mapContainer) {
            mapContainer.style.filter = 'brightness(0.6)';
            mapContainer.style.border = '5px solid green';
        }
    )");

	// 延迟800毫秒后截图并恢复样式
	QTimer::singleShot(800, this, [this]() {
		// 捕获截图
		QPixmap screenshot = m_webView->grab();

		// 恢复原始样式
		m_webView->page()->runJavaScript(R"(
            var mapContainer = document.getElementById('map');
            if (mapContainer) {
                mapContainer.style.filter = '';
                mapContainer.style.border = '';
            }
        )");

		// 保存截图
		QString fileName = QFileDialog::getSaveFileName(this,
			QStringLiteral("保存截图"),
			"",
			"PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp)");

		if (!fileName.isEmpty()) {
			QString format = QFileInfo(fileName).suffix().toUpper();
			screenshot.save(fileName, format.toUtf8().constData());
			qDebug() << QStringLiteral("截图已保存为") << fileName;
		}
	});
}


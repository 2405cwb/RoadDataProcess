#include "IrmActualTimeShow.h" 

IrmActualTimeShow::IrmActualTimeShow(QWidget *parent)
	: QWidget(parent)
{
	setupUI();
	resize(900, 600);
}

IrmActualTimeShow::~IrmActualTimeShow()
{

}




void IrmActualTimeShow::slot_updateIriFormSlots(double mile, double dmi)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	curProject = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (curProject->getProjectType() == PROJECT_2D_TYPE || curProject->getProjectType() == PROJECT_23D_TYPE)
	{
		auto project = curProject->get2DProject();
		if (project != cur2DProject)
		{
			cur2DProject = project;
			curLeftIriDatas.clear();
			curRightIriDatas.clear();

			//获取平整度地址
			QString iriPath = curProject->get2DProject()->getIRIPath();
			QString leftIriFilePath = iriPath + "\\DAQ0\\IRI_10m.txt";
			QString rightIriFilePath = iriPath + "\\DAQ1\\IRI_10m.txt";

			//获取车辙地址
			QString rutPath = curProject->get2DProPath() + "\\RUT\\camera0\\orirut.txt";
			QStringList	curRutFileTxts = MyCommonMethods::ReadAllLines(rutPath); 
			QStringList curLeftIriTxts = MyCommonMethods::ReadAllLines(leftIriFilePath);
			QStringList curRightTxts = MyCommonMethods::ReadAllLines(rightIriFilePath);


			//获取构造深度地址
			QString leftSmtdFilePath = iriPath + "\\Laser0\\MTD_10m.txt";
			QString rightSmtdFilePath = iriPath + "\\Laser1\\MTD_10m.txt";
			QString centerSmtdFilePath = iriPath + "\\Laser2\\MTD_10m.txt";

			QStringList leftSmtdFileTxts = MyCommonMethods::ReadAllLines(leftSmtdFilePath);
			QStringList rightSmtdFileTxts = MyCommonMethods::ReadAllLines(rightSmtdFilePath);
			QStringList centerSmtdFileTxts = MyCommonMethods::ReadAllLines(centerSmtdFilePath);

			//处理数据 平整度 
			for (QString& line : curLeftIriTxts)
			{
				QStringList sp = line.split(' ');
				if (sp.length() <= 1)
				{
					sp = line.split('/t');
				}
				bool ok = false;
				double value =  sp.at(1).toDouble(&ok);
				if (ok)
				{
					curLeftIriDatas.push_back(value);

				} 
			}
			for (QString& line : curRightTxts)
			{
				QStringList sp = line.split(' ');
				if (sp.length() <= 1)
				{
					sp = line.split('/t');
				}
				bool ok = false;
				double value = sp.at(1).toDouble(&ok);
				if(ok)
				curRightIriDatas.push_back(value);
			}

			//处理车辙
			for (QString& line: curRutFileTxts)
			{
				QStringList spStr= line.split(',');
				if (spStr.length()!=4)
				{
					continue;
				}
				bool ok = false;
				double lValue = spStr.at(1).toDouble(&ok);
				if (ok)
				{
					curLeftRutDatas.push_back(lValue);
				}
				double RValue = spStr.at(3).toDouble(&ok); 
				if (ok)
				{
					curRightRutDatas.push_back(RValue);
				}
			}


			//处理构造深度
			for (QString&line: leftSmtdFileTxts)
			{
				QStringList sp = line.split(' ');
				if (sp.length() <= 1)
				{
					sp = line.split('/t');
				}
				bool ok = false;
				double value = sp.at(1).toDouble(&ok);
				if (ok)
				{
					curLeftSmtdDatas.push_back(value);

				}
			}
			for (QString&line : rightSmtdFileTxts)
			{
				QStringList sp = line.split(' ');
				if (sp.length() <= 1)
				{
					sp = line.split('/t');
				}
				bool ok = false;
				double value = sp.at(1).toDouble(&ok);
				if (ok)
				{
					curRightSmtdDatas.push_back(value);

				}
			}
			for (QString&line : centerSmtdFileTxts)
			{
				QStringList sp = line.split(' ');
				if (sp.length() <= 1)
				{
					sp = line.split('/t');
				}
				bool ok = false;
				double value = sp.at(1).toDouble(&ok);
				if (ok)
				{
					curCenterSmtdDatas.push_back(value);

				}
			}
			
		}

		//根据里程和当前用户设置的区间获取相应数据 
		int index = dmi / 10;
		updateIriCharts(index);
	}
}



void IrmActualTimeShow::selectLengthTextChanged(const QString& str)
{
	curUserLength = str.toInt();
}

void IrmActualTimeShow::selectLengthChanged(int index)
{

}

void IrmActualTimeShow::setupUI()
{
	//创建主布局
	mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(5, 5, 5, 5);
	mainLayout->setSpacing(5);
	QLabel * lable = new QLabel(QStringLiteral("显示距离(最小单位10m)"));
	QComboBox * box = new QComboBox(this);
	QStringList items;
	items << "100" <<"500" << "1000";
	box->addItems(items);
	box->setCurrentIndex(0);
	//connect(box, SIGNAL(QComboBox::currentTextChanged(const QString&)), this, SLOT(selectLengthChanged(const QString&))); 
	bool connOk = connect(box, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentTextChanged), this, &IrmActualTimeShow::selectLengthTextChanged);

	QHBoxLayout* hbox = new QHBoxLayout;
	hbox->addWidget(lable);
	hbox->addWidget(box);
	hbox->addStretch(); //右侧留空

	mainLayout->addLayout(hbox);
	leftIriSeries = new QLineSeries;
	leftIriSeries->append(0, 6);
	leftIriSeries->append(2, 4);
	leftIriSeries->append(3, 8);
	leftIriSeries->append(7, 4);
	leftIriSeries->append(10, 5);
	*leftIriSeries << QPointF(11, 1) << QPointF(13, 3) << QPointF(17, 6) << QPointF(18, 3) << QPointF(20, 2);
	leftIriSeries->setName(QStringLiteral("左边平整度"));
	rightIriSeries = new QLineSeries;
	rightIriSeries->append(0, 3);
	rightIriSeries->append(2, 2);
	rightIriSeries->append(3, 13);
	rightIriSeries->append(7, 13);
	rightIriSeries->append(10, 5);
	rightIriSeries->setName(QStringLiteral("右边平整度"));
	//创建图表
	iriChartForm = new QChart;
	iriChartForm->legend()->setVisible(true);
	iriChartForm->legend()->setAlignment(Qt::AlignBottom);
	iriChartForm->addSeries(leftIriSeries);
	iriChartForm->addSeries(rightIriSeries);
	iriChartForm->createDefaultAxes();
	iriChartForm->setTitle(QString::fromLocal8Bit("平整度(高度(m/Km)/里程(m))"));
	//创建图表视图
	CustomChartView* charview = new CustomChartView(iriChartForm);

	charview->setRenderHint(QPainter::Antialiasing);//设置抗锯齿 
	mainLayout->addWidget(charview, 1);



	leftRutSeries = new QLineSeries;
	leftRutSeries->append(0, 3);
	leftRutSeries->append(2, 2);
	leftRutSeries->append(3, 1);
	leftRutSeries->append(7, 0);
	leftRutSeries->append(10, 9);
	leftRutSeries->setName(QStringLiteral("左边车辙"));

	rightRutSeries = new QLineSeries;
	rightRutSeries->append(0, 3);
	rightRutSeries->append(2, 2);
	rightRutSeries->append(3, 13);
	rightRutSeries->append(7, 13);
	rightRutSeries->append(10, 5);
	rightRutSeries->setName(QStringLiteral("右边车辙"));
	//创建图表
	rutChartForm = new QChart;
	rutChartForm->legend()->setVisible(true);
	rutChartForm->legend()->setAlignment(Qt::AlignBottom);
	rutChartForm->addSeries(leftRutSeries);
	rutChartForm->addSeries(rightRutSeries);
	rutChartForm->createDefaultAxes();
	rutChartForm->setTitle(QString::fromLocal8Bit("车辙(高度(mm)/里程(m))"));
	//创建图表视图
	CustomChartView* charview1 = new CustomChartView(rutChartForm);
	charview1->setRenderHint(QPainter::Antialiasing);//设置抗锯齿 
	mainLayout->addWidget(charview1, 1);



	leftSmtdSeries = new QLineSeries;
	leftSmtdSeries->append(0, 3);
	leftSmtdSeries->append(2, 2);
	leftSmtdSeries->append(3, 13);
	leftSmtdSeries->append(7, 13);
	leftSmtdSeries->append(10, 5);
	leftSmtdSeries->setName(QStringLiteral("右边构造"));

	rightSmtdSeries = new QLineSeries;
	rightSmtdSeries->append(0, 5);
	rightSmtdSeries->append(2, 3);
	rightSmtdSeries->append(3, 6);
	rightSmtdSeries->append(7, 7);
	rightSmtdSeries->append(10, 2);
	rightSmtdSeries->setName(QStringLiteral("左边构造"));

	centerSmtdSeries = new QLineSeries;
	centerSmtdSeries->append(0, 4);
	centerSmtdSeries->append(2, 1);
	centerSmtdSeries->append(3, 4);
	centerSmtdSeries->append(7, 2);
	centerSmtdSeries->append(10, 6);
	centerSmtdSeries->append(11, 7);
	centerSmtdSeries->append(12, 9);
	centerSmtdSeries->append(13, 21);
	centerSmtdSeries->append(14, 13);
	centerSmtdSeries->append(15, 15);
	centerSmtdSeries->append(16, 13);
	centerSmtdSeries->append(17, 5);
	centerSmtdSeries->append(18, 9);
	centerSmtdSeries->setName(QStringLiteral("中间构造"));
	//创建图表
	smtdChartForm = new QChart;
	smtdChartForm->legend()->setVisible(true);
	smtdChartForm->legend()->setAlignment(Qt::AlignBottom);
	smtdChartForm->addSeries(leftSmtdSeries);
	smtdChartForm->addSeries(rightSmtdSeries);
	smtdChartForm->addSeries(centerSmtdSeries);
	smtdChartForm->createDefaultAxes();
	smtdChartForm->setTitle(QString::fromLocal8Bit("构造深度(高度(mm)/里程(m))"));
	//创建图表视图
	CustomChartView* charview2 = new CustomChartView(smtdChartForm);
	charview2->setRenderHint(QPainter::Antialiasing);//设置抗锯齿 
	mainLayout->addWidget(charview2, 1);
}

void IrmActualTimeShow::clearChart()
{

}

void IrmActualTimeShow::updateIriCharts(int startIndex)
{
	int length = curUserLength / 10;
	QVector<double> iriDatas;
	int endIndex = 0;
	updateTargetChart(startIndex, endIndex, length, curLeftIriDatas, iriDatas, leftIriSeries);
	updateTargetChart(startIndex, endIndex, length, curRightIriDatas, iriDatas, rightIriSeries); 
	updateValueAxis(startIndex, endIndex, iriChartForm, iriDatas);
	QVector<double>rutDatas;
	updateTargetChart(startIndex, endIndex, length, curLeftRutDatas, rutDatas, leftRutSeries);
	updateTargetChart(startIndex, endIndex, length, curRightRutDatas, rutDatas, rightRutSeries);
	updateValueAxis(startIndex, endIndex, rutChartForm, rutDatas);
 
	QVector<double>smtdDatas;
	updateTargetChart(startIndex, endIndex, length, curLeftSmtdDatas, smtdDatas, leftSmtdSeries);
	updateTargetChart(startIndex, endIndex, length, curRightSmtdDatas, smtdDatas, rightSmtdSeries);
	updateTargetChart(startIndex, endIndex, length, curCenterSmtdDatas, smtdDatas, centerSmtdSeries);
	updateValueAxis(startIndex, endIndex, smtdChartForm, smtdDatas);
}


void IrmActualTimeShow::updateTargetChart(int& startIndex, int& endIndex, int length, const QVector<double>&  targeDatas, QVector<double>&datas, QLineSeries* series)
{
	if (targeDatas.size() == 0)
	{
		return;
	}
	if (targeDatas.size() <= startIndex)
	{
		return;
	}
	endIndex = startIndex + length;
	if (endIndex >= targeDatas.size())
	{
		endIndex = targeDatas.size();
	}
	series->clear();
	for (int i = startIndex; i < endIndex; i++)
	{
		double value = targeDatas.at(i);
			datas.append(value); 
		//	double mile=  hnApp::hnDataManager::getDataManager()->getCurrentProject()->enclToTrueMile(i * 10);
		//	series->append(int(mile), value);

			double dmi = i * 10;
			series->append(dmi, value);

	}
}

void IrmActualTimeShow::updateValueAxis(int sIndex, int eSindex, QChart* chart, const QVector<double>& datas)
{
	//计算范围
	qreal minx = sIndex * 10, maxX = eSindex * 10;
	qreal minY = 0, maxY = 0;
	for (const double& point : datas)
	{
		if (point < minY)
		{
			minY = point;
		}
		if (point > maxY)
		{
			maxY = point;
		}
	}
	//获取或创建坐标轴
	QValueAxis * axisX = qobject_cast<QValueAxis*>(chart->axisX());
	QValueAxis * axisY = qobject_cast<QValueAxis*>(chart->axisY());
	if (!axisX)
	{
		axisX = new QValueAxis;
		chart->addAxis(axisX, Qt::AlignBottom);
	}
	if (!axisY)
	{
		axisY = new QValueAxis;
		chart->addAxis(axisY, Qt::AlignLeft);
	}

	//设置坐标轴范围
	axisX->setRange(minx, maxX);
	axisY->setRange(minY - 0.1*(maxY - minY), maxY + 0.1*(maxY - minY));

}

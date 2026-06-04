#include "hnOutputExcelDialog.h"
#include "../hnProject/hnProjectManager.h"
#include <QButtonGroup>
#include <QMessageBox>
#include "..\hnConfigService\HnXRSettings.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QAbstractButton>
#include <QComboBox>
#include "hnXlsxInterface.h"
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QProgressDialog>
#include <QDesktopServices>
#include "../hnQtCommon/MyCommonMethods.h" 
#include "../ActiveQt/QAxObject"
#include <QFontMetrics>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QProgressBar>
#include <QTextStream>
#include <QTextCodec>

namespace
{
	const QString kReportProjectSection = QStringLiteral("二三维设置信息");
	const QString kReportRoadWidthKey = QStringLiteral("报表路面宽度");
	const QString kReportMaintenanceUnitKey = QStringLiteral("报表管养单位");
	const QString kReportLaneTypeKey = QStringLiteral("报表车道类型");
	const QString kReportTaskYearKey = QStringLiteral("报表检测任务年份");
	const QString kReportDetectCountKey = QStringLiteral("报表检测次数");
	const QString kReportRegionCodeKey = QStringLiteral("报表行政区划代码");

	QString reportProjectInfoPath(hnPro::hnProject* project)
	{
		if (!project)
		{
			return QString();
		}
		if (project->get2DProject())
		{
			return QDir::toNativeSeparators(project->get2DProject()->getBasePath() + QStringLiteral("/ProjectInfo.txt"));
		}
		QString projectPath = project->getAbsulotelyPath();
		if (projectPath.isEmpty())
		{
			return QString();
		}
		return QDir::toNativeSeparators(projectPath + QStringLiteral("/ProjectInfo.txt"));
	}

	int keyValueSeparatorIndex(const QString& line)
	{
		int halfIndex = line.indexOf(':');
		int fullIndex = line.indexOf(QStringLiteral("："));
		if (halfIndex < 0)
		{
			return fullIndex;
		}
		if (fullIndex < 0)
		{
			return halfIndex;
		}
		return qMin(halfIndex, fullIndex);
	}

	QMap<QString, QString> readReportProjectSection(const QString& filePath)
	{
		QMap<QString, QString> values;
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			return values;
		}

		QTextStream in(&file);
		in.setCodec(QTextCodec::codecForName("UTF-8"));
		QString currentSection;
		while (!in.atEnd())
		{
			QString line = in.readLine().trimmed();
			if (line.startsWith('[') && line.endsWith(']'))
			{
				currentSection = line.mid(1, line.length() - 2).trimmed();
				continue;
			}
			if (currentSection != kReportProjectSection)
			{
				continue;
			}

			int sepIndex = keyValueSeparatorIndex(line);
			if (sepIndex <= 0)
			{
				continue;
			}
			QString key = line.left(sepIndex).trimmed();
			QString value = line.mid(sepIndex + 1).trimmed();
			values[key] = value;
		}
		return values;
	}

	bool writeReportProjectSection(const QString& filePath, QMap<QString, QString> values)
	{
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			return false;
		}

		QStringList lines;
		QTextStream in(&file);
		in.setCodec(QTextCodec::codecForName("UTF-8"));
		while (!in.atEnd())
		{
			lines.append(in.readLine());
		}
		file.close();

		int sectionStart = -1;
		int sectionEnd = lines.size();
		for (int i = 0; i < lines.size(); ++i)
		{
			QString trimmed = lines.at(i).trimmed();
			if (trimmed.startsWith('[') && trimmed.endsWith(']'))
			{
				QString section = trimmed.mid(1, trimmed.length() - 2).trimmed();
				if (section == kReportProjectSection)
				{
					sectionStart = i;
					sectionEnd = lines.size();
				}
				else if (sectionStart >= 0)
				{
					sectionEnd = i;
					break;
				}
			}
		}

		if (sectionStart < 0)
		{
			if (!lines.isEmpty() && !lines.last().trimmed().isEmpty())
			{
				lines.append(QString());
			}
			lines.append(QStringLiteral("[%1]").arg(kReportProjectSection));
			sectionStart = lines.size() - 1;
			sectionEnd = lines.size();
		}

		for (int i = sectionStart + 1; i < sectionEnd; ++i)
		{
			int sepIndex = keyValueSeparatorIndex(lines.at(i));
			if (sepIndex <= 0)
			{
				continue;
			}
			QString key = lines.at(i).left(sepIndex).trimmed();
			if (!values.contains(key))
			{
				continue;
			}
			lines[i] = QStringLiteral("%1：%2").arg(key, values.value(key));
			values.remove(key);
		}

		for (auto it = values.begin(); it != values.end(); ++it)
		{
			lines.insert(sectionEnd, QStringLiteral("%1：%2").arg(it.key(), it.value()));
			++sectionEnd;
		}

		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
		{
			return false;
		}
		QTextStream out(&file);
		out.setCodec(QTextCodec::codecForName("UTF-8"));
		for (const QString& line : qAsConst(lines))
		{
			out << line << '\n';
		}
		return true;
	}

	void setComboBoxText(QComboBox* comboBox, const QString& value)
	{
		if (!comboBox || value.trimmed().isEmpty())
		{
			return;
		}
		int index = comboBox->findText(value);
		if (index < 0)
		{
			comboBox->addItem(value);
			index = comboBox->findText(value);
		}
		comboBox->setCurrentIndex(index);
	}

	QString writeReportExportIssueFile(const QString& outPath, const QStringList& issues)
	{
		if (issues.isEmpty())
		{
			return QString();
		}

		QDir dir(outPath);
		if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
		{
			return QString();
		}

		QString filePath = dir.filePath(QStringLiteral("报表导出问题汇总_%1.txt")
			.arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))));
		QFile file(filePath);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
		{
			return QString();
		}

		QTextStream out(&file);
		out.setCodec(QTextCodec::codecForName("UTF-8"));
		out << QStringLiteral("报表导出问题汇总") << '\n';
		out << QStringLiteral("生成时间：") << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")) << '\n';
		out << QStringLiteral("问题数量：") << issues.size() << "\n\n";
		for (int i = 0; i < issues.size(); ++i)
		{
			out << QString::number(i + 1) << QStringLiteral(". ") << issues.at(i) << '\n';
		}
		return filePath;
	}
}

hnOutputExcelDialog::hnOutputExcelDialog(QWidget *parent)
	: QDialog(parent), m_dEMile(0), m_dSMile(0), isSingleProject(false), p_modelGroup(nullptr), p_excelLayout(nullptr)
	, m_nowDrawType( ROAD_WORK_TYPE::ROAD_WORK_LARGE_RECT)
	, m_nowStandard(HnProjectEnums::StandardParmTypeEnum::DegreeRoad2018),
	m_progressDialog(nullptr)
{
	ui.setupUi(this);

	m_xrSetting = HnXRSettings::getInstance();
	//读取配置文件
	readSetting();
	hnPro::hnProjectManager* manager = hnApp::hnDataManager::getDataManager()->getProjectManager();

	auto allPorject = manager->getAllBaseProject();
	//多工程必须保证多个工程的道路类型一致
	auto defaultStarndar = HnProjectEnums::roadTypeQStringToEnum(allPorject.at(0)->getCurProSetInfo().strRoadStandard);
	QString title = QStringLiteral("出表界面_");
	QString strStandard = HnProjectEnums::roadTypeEnumToQString(defaultStarndar);
	m_nowDrawType =  (ROAD_WORK_TYPE)allPorject.at(0)->getCurProSetInfo().nDrawType;
	QString defaultDrawType = m_nowDrawType ==  ROAD_WORK_TYPE::ROAD_WORK_LARGE_RECT ? QStringLiteral("人工模式") : QStringLiteral("自动化模式");
	//设计模式出表使用人工模式出表
	if (m_nowDrawType ==2)
	{
		defaultDrawType = QStringLiteral("设计模式"); 
	}
	m_nowStandard = defaultStarndar;

	preStandard = defaultStarndar;
	m_nowProjectType = allPorject.at(0)->getProjectType();
	preNowProjectType = m_nowProjectType;
	//做个检查  必须保证 导入的工程 工程类型 ，道路标准是一致的才允许出表  
	for (auto pro : allPorject)
	{
		auto	nowTempStarndar = HnProjectEnums::roadTypeQStringToEnum(pro->getCurProSetInfo().strRoadStandard);
		if (nowTempStarndar != m_nowStandard)
		{
			QMessageBox::critical(nullptr, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("请保证所有工程的道路标准一致!"));
			return;
		}
		auto	 nowTempProjectType = pro->getProjectType();
		if (nowTempProjectType != m_nowProjectType)
		{
			QMessageBox::critical(nullptr, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("请保证所有工程的工程类型一致!"));
			return;
		}
	}
	if (m_nowProjectType == PROJECT_TYPE::PROJECT_2D_TYPE || m_nowProjectType == PROJECT_TYPE::PROJECT_23D_TYPE)
	{
		//这个地方在打开工程的时候已经运行过了 
		//保险起见 在进行一次桩号的计算
		hnApp::hnDataManager::getDataManager()->getProjectManager()->initAllProjectMileVector();
	}
	else
	{
		//TODO 单三维的桩号处理

	}
	setWindowTitle(title + strStandard + "_" + defaultDrawType);
	int projectCount = allPorject.size();
	if (projectCount == 1)
	{
		//单工程出表设置界面
		setSingleProjectFrom();
		isSingleProject = true;

	}
	else
	{
		isSingleProject = false;
		ui.tabWidget->removeTab(1);

	}

	readExcelConfigData();
	setModelChooseForm();
	setSettingFrom();
	ui.tabWidget->setCurrentIndex(0);
	//信号槽连接
	this->connectMethods();
}


void hnOutputExcelDialog::setSettingFrom()
{
	ui.groupBox_12->setVisible(true);
	ui.groupBox_12->setEnabled(true);

	hnPro::hnProjectManager* projectManager = hnApp::hnDataManager::getDataManager()->getProjectManager();
	if (!projectManager)
	{
		return;
	}
	auto allProject = projectManager->getAllBaseProject();
	if (allProject.empty())
	{
		return;
	}
	loadReportProjectInfo(allProject.at(0));
}

void hnOutputExcelDialog::loadReportProjectInfo(hnPro::hnProject* project)
{
	if (!project)
	{
		return;
	}

	QMap<QString, QString> values = readReportProjectSection(reportProjectInfoPath(project));
	auto projectInfo = project->getCurProSetInfo();

	QString reportRoadWidth = values.value(kReportRoadWidthKey);
	if (reportRoadWidth.isEmpty())
	{
		reportRoadWidth = values.value(QStringLiteral("检测路面宽度"));
	}
	if (reportRoadWidth.isEmpty())
	{
		reportRoadWidth = QString::number(projectInfo.dRoadWidth);
	}
	ui.lineEdit_7->setText(reportRoadWidth);

	QString maintenanceUnit = values.value(kReportMaintenanceUnitKey);
	if (maintenanceUnit.isEmpty())
	{
		maintenanceUnit = values.value(QStringLiteral("管养单位"));
	}
	if (!maintenanceUnit.isEmpty())
	{
		ui.lineEdit_4->setText(maintenanceUnit);
	}

	QString laneType = values.value(kReportLaneTypeKey);
	if (laneType.isEmpty())
	{
		laneType = values.value(QStringLiteral("车道类型"));
	}
	if (!laneType.isEmpty())
	{
		ui.lineEdit_6->setText(laneType);
	}

	QString taskYear = values.value(kReportTaskYearKey);
	if (taskYear.isEmpty())
	{
		taskYear = values.value(QStringLiteral("检测任务年份"));
	}
	setComboBoxText(ui.comboBox_5, taskYear);

	QString detectCount = values.value(kReportDetectCountKey);
	if (detectCount.isEmpty())
	{
		detectCount = values.value(QStringLiteral("检测次数"));
	}
	setComboBoxText(ui.comboBox_6, detectCount);

	QString regionCode = values.value(kReportRegionCodeKey);
	if (regionCode.isEmpty())
	{
		regionCode = values.value(QStringLiteral("行政区划代码"));
	}
	if (!regionCode.isEmpty())
	{
		ui.lineEdit_8->setText(regionCode);
	}
}

bool hnOutputExcelDialog::saveReportProjectInfo(hnPro::hnProject* project)
{
	if (!project)
	{
		return false;
	}
	QString filePath = reportProjectInfoPath(project);
	if (filePath.isEmpty())
	{
		return false;
	}

	QMap<QString, QString> values;
	values[kReportRoadWidthKey] = QString::number(getReportRoadWidthFromUi());
	values[kReportMaintenanceUnitKey] = ui.lineEdit_4->text().trimmed();
	values[kReportLaneTypeKey] = ui.lineEdit_6->text().trimmed();
	values[kReportTaskYearKey] = ui.comboBox_5->currentText().trimmed();
	values[kReportDetectCountKey] = ui.comboBox_6->currentText().trimmed();
	values[kReportRegionCodeKey] = ui.lineEdit_8->text().trimmed();
	return writeReportProjectSection(filePath, values);
}

double hnOutputExcelDialog::getReportRoadWidthFromUi() const
{
	return ui.lineEdit_7->text().trimmed().toDouble();
}

void hnOutputExcelDialog::setupReportProgressDialog(int maximum)
{
	if (!m_progressDialog)
	{
		m_progressDialog = new QProgressDialog(this);
	}

	m_progressDialog->setObjectName(QStringLiteral("reportProgressDialog"));
	m_progressDialog->setWindowTitle(QStringLiteral("导出报表"));
	m_progressDialog->setLabelText(QStringLiteral("正在准备导出任务..."));
	m_progressDialog->setMinimum(0);
	m_progressDialog->setMaximum(qMax(1, maximum));
	m_progressDialog->setValue(0);
	m_progressDialog->setMinimumDuration(0);
	m_progressDialog->setAutoClose(false);
	m_progressDialog->setAutoReset(false);
	m_progressDialog->setCancelButton(nullptr);
	m_progressDialog->setWindowModality(Qt::ApplicationModal);
	m_progressDialog->setFixedSize(QSize(520, 112));
	m_progressDialog->setWindowFlags((m_progressDialog->windowFlags() & ~Qt::WindowContextHelpButtonHint) | Qt::WindowTitleHint);
	m_progressDialog->setStyleSheet(QStringLiteral(
		"QProgressDialog#reportProgressDialog{"
		"background:#FFFFFF;"
		"border:1px solid #D8E0EA;"
		"}"
		"QProgressDialog#reportProgressDialog QLabel{"
		"font-family:'Microsoft YaHei';"
		"font-size:13px;"
		"font-weight:500;"
		"color:#26384D;"
		"padding:0;"
		"}"
		"QProgressDialog#reportProgressDialog QProgressBar{"
		"height:14px;"
		"border:1px solid #C9D5E2;"
		"border-radius:7px;"
		"background:#EEF3F8;"
		"text-align:center;"
		"font-family:'Microsoft YaHei';"
		"font-size:10px;"
		"font-weight:600;"
		"color:#21354D;"
		"}"
		"QProgressDialog#reportProgressDialog QProgressBar::chunk{"
		"border-radius:6px;"
		"margin:1px;"
		"background:#1677FF;"
		"}"));
	if (m_progressDialog->layout())
	{
		m_progressDialog->layout()->setContentsMargins(24, 16, 24, 18);
		m_progressDialog->layout()->setSpacing(10);
	}

	QLabel* label = m_progressDialog->findChild<QLabel*>();
	if (label)
	{
		label->setWordWrap(true);
		label->setMinimumHeight(28);
		label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	}

	QProgressBar* progressBar = m_progressDialog->findChild<QProgressBar*>();
	if (progressBar)
	{
		progressBar->setTextVisible(true);
		progressBar->setFormat(QStringLiteral("%p%"));
	}
}

void hnOutputExcelDialog::setSingleProjectFrom()
{
	hnPro::hnProjectManager* manager = hnApp::hnDataManager::getDataManager()->getProjectManager();
	auto allPorject = manager->getAllBaseProject();
	auto project = allPorject.at(0);
	QVector<hnMile> miles;
	miles = project->getCurrentMileVector();


	if (allPorject.size() > 0)
	{
		m_currentPorject = allPorject[0];
	}
	m_dSMile = m_currentPorject->getCurProSetInfo().dBegMile;
	m_dEMile = m_currentPorject->getCurProSetInfo().dEndMile;

	
	if (m_currentPorject->getCurProSetInfo().dUserBegMile >= 0 && m_currentPorject->getCurProSetInfo().dUserEndMile >= 0)
	{
		m_dSMile = m_currentPorject->getCurProSetInfo().dUserBegMile;
		m_dEMile = m_currentPorject->getCurProSetInfo().dUserEndMile;
		if  (m_currentPorject->getCurProSetInfo().nLineType==-1)
		{
			double temp = 0;
			if (m_dSMile< m_dEMile)
			{
				temp = m_dSMile;
				m_dSMile = m_dEMile;
				m_dEMile = temp;

			}
		} 
	}
	ui.lineEdit_3->setText(QString::number(m_dSMile, 'f', 2));
	ui.lineEdit_5->setText(QString::number(m_dEMile, 'f', 2));

	//添加打标区间表格 
	QStringList lis;
	lis << QStringLiteral("起点桩号") << QStringLiteral("终点桩号") << QStringLiteral("道路标准");
	ui.tableWidget->setColumnCount(3);
	ui.tableWidget->setRowCount(0);
	ui.tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	ui.tableWidget->setHorizontalHeaderLabels(lis);
	ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	//获得标准切换 
	QVector<hnMile> datas;
	if (m_nowProjectType == PROJECT_TYPE::PROJECT_2D_TYPE || m_nowProjectType == PROJECT_TYPE::PROJECT_23D_TYPE)
	{
		datas = m_currentPorject->getCurrentMileVector();
	}
	QVector<QString> result;
	hnMile oriType;
	if (datas.size() > 1)
		oriType = datas[0];
	for (int i = 1; i < datas.size() - 1; ++i)
	{
		if (datas[i].roadStandard != oriType.roadStandard)
		{
			//道路标准切换了 
			QString standStr = HnProjectEnums::roadTypeEnumToQString(oriType.roadStandard);
			QString sMile = QString::number(oriType.dTrueMile);
			QString eMile = QString::number(datas[i - 1].dTrueMile);
			oriType = datas[i];
			QString temp = sMile + "," + eMile + "," + standStr;
			result.push_back(temp);
		}
		if (i == datas.size() - 2)
		{
			//到了最后一个 
			QString standStr = HnProjectEnums::roadTypeEnumToQString(oriType.roadStandard);
			QString sMile = QString::number(oriType.dTrueMile, 'f', 2);
			QString eMile = QString::number(m_currentPorject->getCurProSetInfo().dEndMile, 'f', 2);
			QString temp = sMile + "," + eMile + "," + standStr;
			result.push_back(temp);
		}
	}

	for (int i = 0; i < result.size(); ++i)
	{
		QStringList row;
		QStringList msgs = result.at(i).split(",");

		row << msgs[0];
		row << msgs[1];
		row << msgs[2];
		int rowIndex = ui.tableWidget->rowCount();
		ui.tableWidget->insertRow(rowIndex);
		ui.tableWidget->setItem(rowIndex, 0, new QTableWidgetItem(row[0]));
		ui.tableWidget->setItem(rowIndex, 1, new  QTableWidgetItem(row[1]));
		ui.tableWidget->setItem(rowIndex, 2, new  QTableWidgetItem(row[2]));
	}
	connect(ui.tableWidget, &QTableWidget::cellDoubleClicked, this, [&](int row, int coloum) {
		//获取用户的数据
		QTableWidgetItem * item1 = ui.tableWidget->item(row, 0);
		QTableWidgetItem * item2 = ui.tableWidget->item(row, 1);
		QTableWidgetItem * item3 = ui.tableWidget->item(row, 2);
		double sMile = item1->text().toDouble();
		double eMile = item2->text().toDouble();
		QString standard = item3->text();
		changeSingleMileAndStandard(sMile, eMile, HnProjectEnums::roadTypeQStringToEnum(standard));
		QString title = QStringLiteral("出表界面_");
		QString defaultDrawType = m_nowDrawType == 0 ? QStringLiteral("人工模式") : QStringLiteral("自动化模式");

		setWindowTitle(title + standard + "_" + defaultDrawType);
	}
	);
}

void hnOutputExcelDialog::setModelChooseForm()
{
	int radioIndex = 0;
	QWidget * widget = new QWidget(ui.scrollArea);

	p_modelGroup = new QButtonGroup(widget);
	p_modelGroup->setExclusive(false);
	ui.scrollArea->setWidget(widget);
	QGridLayout * layout = new QGridLayout(widget);
	layout->setSpacing(5);
	widget->setLayout(layout);
	 

	//配置模表选择框
	for (auto it = m_ExcelSelectConfig.categories.begin() ; it!= m_ExcelSelectConfig.categories.end(); ++it)
	{
		HnProjectEnums::StandardParmTypeEnum  typeF = it.key();
		RoadCategory innerMap = it.value();
		if (typeF == m_nowStandard)
		{
			for (auto iit = innerMap.reportGroups.begin(); iit != innerMap.reportGroups.end(); ++iit)
			{
				QString modelKey = iit.key();
				addRadioButtonToModelGroup(radioIndex, modelKey, widget, p_modelGroup, layout);
				for (ReportItem excelName : iit.value())
				{

				}
			}
		}
	} 
	QRadioButton* btn1 = qobject_cast<QRadioButton*>(p_modelGroup->buttons().at(0));
	btn1->setChecked(true);
	onModelClicked(btn1);
	if (m_nowStandard == HnProjectEnums::CityRoad)
	{
		ui.groupBox_11->setEnabled(false);
		ui.groupBox_11->setVisible(false);

	}
	// connect(btnGroup, SIGNAL(buttonClicked(int)), this, SLOT(onModelClicked(int)));
	connect(p_modelGroup, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(onModelClicked(QAbstractButton *)));
	p_modelGroup->setExclusive(true);
	ui.scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	ui.scrollArea->setWidgetResizable(true);
	ui.scrollArea->setWidget(widget);
}

void hnOutputExcelDialog::addRadioButtonToModelGroup(int& index, const QString& text, QWidget* widget, QButtonGroup*btnGroup, QGridLayout*layout)
{
	QRadioButton * radioButton = new  QRadioButton(text, widget);

	btnGroup->addButton(radioButton, index);
	radioButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	layout->addWidget(radioButton, index / 3, index % 3);
	index++;
}

void hnOutputExcelDialog::getUserSelectExcelName(QGridLayout* grid, QMap<int, QVector<double>>& selectMessage)
{
	selectMessage.clear();
	//遍历gridLayout获得用户选中的表格 和 分段
	for (int i = 0; i < grid->rowCount(); ++i)
	{
		//获取第一列单选框
		QCheckBox * cheBt = qobject_cast<QCheckBox*> (grid->itemAtPosition(i, 0)->widget());
		if (cheBt&&cheBt->isChecked())
		{

			//如果被选中 获取第二列中的Combox内容
			QComboBox * comBt = qobject_cast<QComboBox*> (grid->itemAtPosition(i, 1)->widget());
			if (comBt)
			{
				QString currentText = comBt->currentText();
				QStringList split = currentText.split(QStringLiteral(","));
				QVector<double> splits;
				for each (QString str in split)
				{
					splits.push_back(str.toDouble());
				}
				selectMessage.insert(i, splits);
			}
		}
	}
}

void hnOutputExcelDialog::onTableChange(int index)
{
	//单表模式有三个页 
	//多表模式有二页
	switch (index)
	{
	case  0:
		break;
	case  1:
		if (!isSingleProject)
		{
			if (m_nowStandard != preStandard)
			{
				setModelChooseForm();
			}
		}
		break;
	case  2:
		if (isSingleProject)
		{
			if (m_nowStandard != preStandard)
			{
				setModelChooseForm();
			}
		}


		break;
	default:
		break;
	}
}

void hnOutputExcelDialog::onModelClicked(QAbstractButton * radBtn)
{
	
	m_ItemWidgetMap.clear();
	QRadioButton* btn = qobject_cast<QRadioButton*> (radBtn);
	QString text = btn->text();
	QList<ReportItem>& items = m_ExcelSelectConfig.categories[m_nowStandard].reportGroups[text];
	ReportItem * data = &m_ExcelSelectConfig.categories.first().reportGroups.first().first();
	ReportItem* test = &items.first();
//	m_ItemWidgetMap;
	
	QWidget* widget = new QWidget(this);
	ui.scrollArea_2->setWidget(widget);
	p_excelLayout = new QGridLayout(widget);
	p_excelLayout->setSpacing(5);
	widget->setLayout(p_excelLayout);
	int rowIndex = 0;
	for (ReportItem& item : items)
	{
		QCheckBox* checkBtn = new QCheckBox(item.displayName, widget);
		if (!item.isShow)
		{
			item.isChecked = false;
			checkBtn->setEnabled(false);
		}
		checkBtn->setChecked(item.isChecked);
	
		p_excelLayout->addWidget(checkBtn, rowIndex, 0);

		QComboBox* comBox = new QComboBox(widget);
		for (QString segment : item.segments)
		{
			comBox->addItem(segment);
		}
		comBox->setCurrentIndex(item.selectSegmentIndex);
		p_excelLayout->addWidget(comBox, rowIndex, 1);

		m_ItemWidgetMap[&item] = qMakePair(checkBtn, comBox);
		
		rowIndex++;
	}
 
	ui.scrollArea_2->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	ui.scrollArea_2->setWidgetResizable(true);
	ui.scrollArea_2->setWidget(widget);

}

void hnOutputExcelDialog::onOkButton()
{
	m_xrSetting->ExcelErrorMessageList.clear();
	setSetting();
	if (getReportRoadWidthFromUi() <= 0)
	{
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("检测路面宽度必须大于0!"));
		return;
	}
	//弹出对话框 让用户设置输出位置
	QString projectPath = QFileDialog::getExistingDirectory(this, QStringLiteral("请选择输出路径"), m_xrSetting->OutPath);
	if (projectPath.isEmpty())
	{
		return;
	}
	m_xrSetting->OutPath = projectPath;
	m_xrSetting->writeData();
	//出表
	//获得用户选中的节点
	QString selectModelTxt = "";
	saveConfig(); 
	QMap<int, QVector<double>>StreetSelectMsg;
	getUserSelectExcelName(ui.street_gridLayout, StreetSelectMsg);
	for (auto btn : p_modelGroup->buttons())
	{
		QRadioButton* raBt = qobject_cast<QRadioButton*>(btn);
		if (raBt->isChecked())
		{
			selectModelTxt = raBt->text();
		}
	}
	if (selectModelTxt.isEmpty())
	{
		hide();
		return;
	}   
	 
	hnPro::hnProjectManager* manager = hnApp::hnDataManager::getDataManager()->getProjectManager();
	auto allProject = manager->getAllBaseProject(); 
	for (auto project : allProject)
	{
		if (!saveReportProjectInfo(project))
		{
			QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("写入ProjectInfo.txt报表工程信息失败!"));
			return;
		}
	}
	 
	int count = allProject.size();
#pragma region 设置进度条
	
	hnOutExcelMileManage::beginCollectExportIssues();
	m_progressValue = 0;
	const int progressScale = 10;
	setupReportProgressDialog(m_UserSelectCount * count * progressScale);
	m_progressDialog->show();

	for (int i = 0; i < count; ++i)
	{
		m_currentPorject = allProject.at(i);
		//进度条设置
	//	m_progressDialog->reset();
		QString projectName ="";
		if (m_nowProjectType == PROJECT_TYPE::PROJECT_23D_TYPE || m_nowProjectType == PROJECT_TYPE::PROJECT_2D_TYPE)
		{
			projectName = QStringLiteral("正在导出：%1").arg(m_currentPorject->get2DProName());
		}
		else
		{
			projectName = QStringLiteral("正在导出：%1").arg(m_currentPorject->get3DProName());
		} 
		 
		m_progressDialog->setLabelText(projectName);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		
#pragma endregion

#pragma region 设置出表桩号，创建出表文件夹

		if (isSingleProject)
		{

			m_startMile = m_dSMile;
			m_endMile = m_dEMile;
			m_standard = m_nowStandard;
		}
		else
		{
			m_startMile = m_currentPorject->getCurProSetInfo().dBegMile;
			m_endMile = m_currentPorject->getCurProSetInfo().dEndMile;
			m_standard = m_nowStandard;
			if (m_currentPorject->getCurProSetInfo().dUserBegMile >= 0 &&
				m_currentPorject->getCurProSetInfo().dUserEndMile >= 0)
			{
				m_startMile = m_currentPorject->getCurProSetInfo().dUserBegMile;
				m_endMile = m_currentPorject->getCurProSetInfo().dUserEndMile;
			}
		}
		QString saveExcelDir;
		if (saveExcelDir.isEmpty())
		{
			if (m_nowProjectType == PROJECT_TYPE::PROJECT_23D_TYPE || m_nowProjectType == PROJECT_TYPE::PROJECT_2D_TYPE)
			{
				saveExcelDir = m_xrSetting->OutPath + "//" + m_currentPorject->get2DProName() +
					"(" + MyCommonMethods::convertMileToString(m_startMile) + "~" + MyCommonMethods::convertMileToString(m_endMile) + ")" + "//";
			}
			else
			{
				saveExcelDir = m_xrSetting->OutPath + "//" + m_currentPorject->get3DProName() + "//";
			}
			QDir dir(saveExcelDir);
			if (!dir.exists())
			{
				dir.mkdir(saveExcelDir);
			}
		}
#pragma endregion 

		startOutExcelManager(saveExcelDir, selectModelTxt, m_currentPorject	 ,m_progressValue, m_progressDialog, progressScale);

		startOutExcelManager_Street(saveExcelDir,selectModelTxt, allProject, StreetSelectMsg,	m_progressValue, m_progressDialog, progressScale);




		if (m_xrSetting->outMarkInfoFile)
		{
		  auto marks = 	m_currentPorject->getCurrentMarkVector();
			QString markFilePath = m_currentPorject->get2DProject()->getFullRoadTypeMarkFilePath();
			QFile markFile(markFilePath);
			if (markFile.exists())
			{ 
				QString newFilePath = saveExcelDir + "\\RoadTypeInfo.txt";
				QFile fileTemp1(newFilePath);
				if (!fileTemp1.exists())
				{
					markFile.copy(newFilePath);
				}
			}
		}
	
		saveExcelDir = QString();

	}
	if(m_progressDialog)
	m_progressDialog->hide();

	QStringList exportIssues = hnOutExcelMileManage::endCollectExportIssues();
	QString issueFilePath = writeReportExportIssueFile(m_xrSetting->OutPath, exportIssues);
	 
	QDialog * tipDlg = new QDialog(this);
	tipDlg->setWindowTitle(QStringLiteral("导出完成"));
	QPushButton* openFld = new QPushButton(QStringLiteral("打开目录"));
	QPushButton* okBtn = new QPushButton(QStringLiteral("确定"));
	QPushButton* openIssueFile = nullptr;
	QLabel * lable = new QLabel(this);
	if (exportIssues.isEmpty())
	{
		lable->setText(QStringLiteral("所有出表任务已经完成。"));
	}
	else if (issueFilePath.isEmpty())
	{
		lable->setText(QStringLiteral("导出完成，但发现 %1 条数据问题。\n问题文件写入失败，请检查导出目录权限。")
			.arg(exportIssues.size()));
	}
	else
	{
		lable->setText(QStringLiteral("导出完成，但发现 %1 条数据问题。\n已生成问题汇总文件：\n%2")
			.arg(exportIssues.size())
			.arg(QDir::toNativeSeparators(issueFilePath)));
		openIssueFile = new QPushButton(QStringLiteral("查看问题明细"));
	}
	lable->setWordWrap(true);
	m_xrSetting->outExcel = true;
	QGridLayout * laout = new QGridLayout(tipDlg);
	laout->addWidget(lable, 0, 0, 1, 3, Qt::AlignCenter);
	laout->addWidget(openFld, 1, 0);
	if (openIssueFile)
	{
		laout->addWidget(openIssueFile, 1, 1);
		laout->addWidget(okBtn, 1, 2);
	}
	else
	{
		laout->addWidget(okBtn, 1, 1);
	}
	tipDlg->resize(exportIssues.isEmpty() ? 260 : 560, exportIssues.isEmpty() ? 150 : 190);
	tipDlg->setLayout(laout);
	connect(okBtn, &QPushButton::clicked, this, [&]()
	{
		tipDlg->reject();
		tipDlg->close();
	}
	);
	connect(openFld, &QPushButton::clicked, this, [&]()
	{

		//打开文件夹
		QDesktopServices::openUrl(QUrl::fromLocalFile(m_xrSetting->OutPath));
		tipDlg->reject();
	}
	);
	if (openIssueFile)
	{
		connect(openIssueFile, &QPushButton::clicked, this, [issueFilePath]()
		{
			QDesktopServices::openUrl(QUrl::fromLocalFile(issueFilePath));
		}
		);
	}
	tipDlg->exec();

}

void hnOutputExcelDialog::saveConfig()
{
	//遍历所有保存的映射关系
	m_UserSelectCount = 0;
	QMap<ReportItem*, QPair<QCheckBox*, QComboBox*>>::iterator it;
	for (it = m_ItemWidgetMap.begin(); it!= m_ItemWidgetMap.end();++it)
	{
		ReportItem * item = it.key();
		QCheckBox* checkBox = it.value().first;
		QComboBox* comBox = it.value().second;

		//更新选中情况
		item->isChecked = checkBox->isChecked();
	
		//获取选中的Segment值
		item->selectSegmentIndex = comBox->currentIndex();
		if (item->isChecked&& item->isShow)
		{ 
		
			QStringList splits = item->segments.at(item->selectSegmentIndex).split(','); 
			m_UserSelectCount += splits.size();
		}
	}
	QMap<int, QVector<double>>StreetSelectMsg;
	getUserSelectExcelName(ui.street_gridLayout, StreetSelectMsg);
 
	for (auto item : StreetSelectMsg)
	{
		m_UserSelectCount += item.size();
	}
	 
	if (m_ExcelSelectConfig.saveToFile(m_configPath))
	{

	}
}

void hnOutputExcelDialog::readSetting()
{
	if (m_xrSetting->czDisOutSelectExcel == 0)
	{
		ui.radioButton->setChecked(true);
	}
	else if (m_xrSetting->czDisOutSelectExcel == 1)
	{
		ui.radioButton_2->setChecked(true);
	}
	else if (m_xrSetting->czDisOutSelectExcel == 2)
	{
		ui.radioButton_3->setChecked(true);
	}

	if (m_xrSetting->roadDisDegreeExcel == 0)
	{
		ui.radioButton_4->setChecked(true);
	}
	else
	{
		ui.radioButton_5->setChecked(true);
	}

	if (m_xrSetting->roadSnKcShowExcel == 0)
	{
		ui.radioButton_6->setChecked(true);
	}
	else
	{
		ui.radioButton_7->setChecked(true);
	}

	if (m_xrSetting->RQIJudgeType == 0 )
	{
		ui.average_iri_check_btn->setChecked(true);
	}
	else if (m_xrSetting->RQIJudgeType == 1)
	{
		ui.max_iri_check_btn->setChecked(true);
	}
	else
	{
		
	}
	if (m_xrSetting->outExcelNeedSort)
	{
		ui.radioButton_sort_true->setChecked(true);
	}
	else
	{
		ui.radioButton_sort_false->setChecked(true);

	}

	if (m_xrSetting->outMarkInfoFile)
	{
		ui.radioButton_sort_true_2->setChecked(true);
	}
	else
	{
		ui.radioButton_sort_false_2->setChecked(true);
	}

	 
	if (m_xrSetting->outSpeedAndMarkExcel)
	{
		ui.checkBox_2->setChecked(true);
	}
	else
	{
		ui.checkBox_2->setChecked(false);
	}

	if (m_xrSetting->roadSnKcShowExcel == 0)
	{
		ui.radioButton_6->setChecked(true);
	}
	else
	{
		ui.radioButton_7->setChecked(true);
	}

	if (m_xrSetting->outMileWithMark == true)
	{
		ui.radioButton_outMileWithMark->setChecked(true);
	}
	else
	{
		ui.radioButton_outMileRemoveMark->setChecked(true);
	}

	if (m_xrSetting->rutOutMode==0)
	{
		ui.radioButton_rutMode0->setChecked(true);
	}
	else if (m_xrSetting->rutOutMode ==1)
	{
		ui.radioButton_rutMode1->setChecked(true);
	}
	else if (m_xrSetting->rutOutMode == 2)
	{
		ui.radioButton_rutMode2->setChecked(true);
	}

	if (m_xrSetting->outRoadUnitMark == true)
	{
		ui.radioButton_13->setChecked(true);
	}
	else
	{
		ui.radioButton_14->setChecked(true);
	}
	if (m_xrSetting->outExcelFormatDmi )
	{
		ui.radioButton_15->setChecked(true);
	}

	else
	{
		ui.radioButton_16->setChecked(true);
	}

	if (m_xrSetting->diseaseExcelOutPicture)
	{
		ui.radioButton_17->setChecked(true);

	}
	else
	{
		ui.radioButton_18->setChecked(true);

	}
	if (m_xrSetting->diseaseExcelOutLocation)
	{
		ui.radioButton_19->setChecked(true);
	}
	else
	{
		ui.radioButton_20->setChecked(true);
	}


	ui.lineEdit_15->setText(QString::number(m_xrSetting->sheetRoundingOffNum));
	ui.lineEdit_16->setText(QString::number(m_xrSetting->sheetRoundingOffNum_Dr));
}

void hnOutputExcelDialog::setSetting()
{
	if (ui.radioButton_rutMode0->isChecked())
	{
		m_xrSetting->rutOutMode = 0; 
	}
	else if(ui.radioButton_rutMode1->isChecked())
	{
		m_xrSetting->rutOutMode = 1;
	}
	else
	{
		m_xrSetting->rutOutMode = 2;
	}

	if (ui.radioButton->isChecked())
	{
		m_xrSetting->czDisOutSelectExcel = 0;
	}
	else if (ui.radioButton_2->isChecked())
	{
		m_xrSetting->czDisOutSelectExcel = 1;
	}
	else if (ui.radioButton_3->isChecked())
	{
		m_xrSetting->czDisOutSelectExcel = 2;
	}
	if (ui.radioButton_4->isChecked())
	{
		m_xrSetting->roadDisDegreeExcel = 0;

	}
	else if (ui.radioButton_5->isChecked())
	{
		m_xrSetting->roadDisDegreeExcel = 1;
	}
	if (ui.radioButton_6->isChecked())
	{
		m_xrSetting->roadSnKcShowExcel = 0;
	}
	else
	{
		m_xrSetting->roadSnKcShowExcel = 1;
	}

	if (ui.checkBox_2->isChecked())
	{
		m_xrSetting->outSpeedAndMarkExcel = true;
	}
	else
	{
		m_xrSetting->outSpeedAndMarkExcel = false;
	}
	if (ui.radioButton_10->isChecked())
	{
		m_xrSetting->BrokenPlatetype = 0;
	}
	else
	{
		m_xrSetting->BrokenPlatetype = 1;
	}
	if (ui.radioButton_outMileWithMark->isChecked())
	{
		m_xrSetting->outMileWithMark = true;
	}
	else
	{
		m_xrSetting->outMileWithMark = false;
	}
	if (ui.radioButton_13->isChecked())
	{
		m_xrSetting->outRoadUnitMark = true;
	}
	else
	{
		m_xrSetting->outRoadUnitMark = false;
	}

	
	if (ui.radioButton_17->isChecked())
	{
		m_xrSetting->diseaseExcelOutPicture = true;
	}
	else
	{
		m_xrSetting->diseaseExcelOutPicture = false;

	}
	if (ui.radioButton_19->isChecked())
	{
		m_xrSetting->diseaseExcelOutLocation = true;
	}
	else
	{
		m_xrSetting->diseaseExcelOutLocation = false;

	}

	if (ui.radioButton_15->isChecked())
	{
		m_xrSetting->outExcelFormatDmi = true;
	}
	else
	{
		m_xrSetting->outExcelFormatDmi = false;
	}

	if (ui.average_iri_check_btn->isChecked())
	{
		m_xrSetting->RQIJudgeType= 0;
	}
	else if (ui.max_iri_check_btn->isChecked())
	{
		m_xrSetting->RQIJudgeType = 1;

	}
	else
	{
		m_xrSetting->RQIJudgeType = 1;
	}
	if (ui.radioButton_sort_true->isChecked())
	{
		m_xrSetting->outExcelNeedSort = true;
	}
	else
	{
		m_xrSetting->outExcelNeedSort = false;

	}

	if (ui.radioButton_sort_true_2->isChecked())
	{
		m_xrSetting->outMarkInfoFile = true;
	}
	else
	{
		m_xrSetting->outMarkInfoFile = false;
	}
	 

	m_xrSetting->PlateWidth = ui.lineEdit_9->text().toDouble();
	m_xrSetting->PlateLength = ui.lineEdit_10->text().toDouble();
	m_xrSetting->sheetRoundingOffNum = ui.lineEdit_15->text().toInt();
	m_xrSetting->sheetRoundingOffNum_Dr = ui.lineEdit_16->text().toInt();


	m_xrSetting->writeData();
}

void hnOutputExcelDialog::onCannelButton()
{
	hide();
}

void hnOutputExcelDialog::onSelectAllExcel(bool value)
{
	if (value == true)
	{
		for (int row = 0; row < p_excelLayout->rowCount(); ++row)
		{
			for (int col = 0; col < p_excelLayout->columnCount(); ++col)
			{
				QWidget *  nowWidget = p_excelLayout->itemAtPosition(row, col)->widget();
				QCheckBox *box = qobject_cast<QCheckBox*>(nowWidget);
				if (box)
				{
					box->setChecked(true);
				}
			}
		}
	}
	else
	{
		for (int row = 0; row < p_excelLayout->rowCount(); ++row)
		{
			for (int col = 0; col < p_excelLayout->columnCount(); ++col)
			{
				QWidget *  nowWidget = p_excelLayout->itemAtPosition(row, col)->widget();
				QCheckBox *box = qobject_cast<QCheckBox*>(nowWidget);
				if (box)
				{
					box->setChecked(false);
				}
			}
		}

	}
}

void hnOutputExcelDialog::onReturnMile()
{

	hnPro::hnProjectManager* manager = hnApp::hnDataManager::getDataManager()->getProjectManager();
	auto allPorject = manager->getAllBaseProject();
	auto project = allPorject.at(0);
	QVector<hnMile> miles = project->getCurrentMileVector();
	if (miles.size() > 1)
	{
		//m_dSMile = miles.at(0).dTrueMile;
		//m_dEMile = miles.last().dTrueMile;
		hnPro::hnProjectManager* manager = hnApp::hnDataManager::getDataManager()->getProjectManager();
		auto allProject = manager->getAllBaseProject();
		if (allProject.size() > 0)
		{
			m_currentPorject = allProject[0];
		}
		double sTempMile = 0;
		double eTempMile = 0;
		sTempMile = m_currentPorject->getCurProSetInfo().dBegMile;
		eTempMile = m_currentPorject->getCurProSetInfo().dEndMile;
		if (m_currentPorject->getCurProSetInfo().dUserBegMile >= 0 && m_currentPorject->getCurProSetInfo().dUserEndMile >= 0)
		{
			sTempMile = m_currentPorject->getCurProSetInfo().dUserBegMile;
			eTempMile = m_currentPorject->getCurProSetInfo().dUserEndMile;
		}
		changeSingleMileAndStandard(sTempMile, eTempMile, m_nowStandard);

	}
}

void hnOutputExcelDialog::onReturnMileOk()
{
	double sMile = m_currentPorject->getCurProSetInfo().dBegMile;
	double eMile = m_currentPorject->getCurProSetInfo().dEndMile;

	double userSmile = ui.lineEdit->text().toDouble();
	double uesrEmile = ui.lineEdit_2->text().toDouble();
	if (m_currentPorject->getCurProSetInfo().nLineType == 1)
	{
		if (userSmile >= sMile &&  uesrEmile <= eMile)
		{
			double temp = 0;
			if (userSmile > uesrEmile)
			{
				temp = userSmile;
				userSmile = uesrEmile;
				uesrEmile = temp;
			}
			ui.lineEdit_3->setText(ui.lineEdit->text());
			ui.lineEdit_5->setText(ui.lineEdit_2->text());
			m_dSMile = ui.lineEdit_3->text().toDouble();
			m_dEMile = ui.lineEdit_5->text().toDouble(); 

		}
		else
		{
			QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("所设置的桩号超出范围!"), QMessageBox::StandardButton::Ok);
			onReturnMile();
		}
	}
	else
	{
		if (userSmile <= sMile &&  uesrEmile >= eMile)
		{
			double temp = 0;
			if (userSmile < uesrEmile)
			{
				temp = userSmile;
				userSmile = uesrEmile;
				uesrEmile = temp;
			}
			ui.lineEdit_3->setText(ui.lineEdit->text());
			ui.lineEdit_5->setText(ui.lineEdit_2->text());
			m_dSMile = ui.lineEdit_3->text().toDouble();
			m_dEMile = ui.lineEdit_5->text().toDouble();
	 
		}
		else
		{
			QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("所设置的桩号超出范围!"), QMessageBox::StandardButton::Ok);
			onReturnMile();
		}
	}


}

void hnOutputExcelDialog::connectMethods()
{

	connect(ui.tabWidget, &QTabWidget::currentChanged, this, &hnOutputExcelDialog::onTableChange);
	connect(ui.okButton, &QPushButton::clicked, this, &hnOutputExcelDialog::onOkButton);
	connect(ui.cannelBtn, &QPushButton::clicked, this, &hnOutputExcelDialog::onCannelButton);
	connect(ui.radioButton_8, &QRadioButton::clicked, this, &hnOutputExcelDialog::onSelectAllExcel);
	connect(ui.pushButton, &QRadioButton::clicked, this, &hnOutputExcelDialog::onReturnMile);
	connect(ui.pushButton_2, &QRadioButton::clicked, this, &hnOutputExcelDialog::onReturnMileOk);

}

bool hnOutputExcelDialog::changeSingleMileAndStandard(double sMile, double eMile, HnProjectEnums::StandardParmTypeEnum type)
{
	ui.lineEdit->setText(QString::number(sMile, 'f', 2));
	ui.lineEdit_2->setText(QString::number(eMile, 'f', 2));
	ui.lineEdit_3->setText(ui.lineEdit->text());
	ui.lineEdit_5->setText(ui.lineEdit_2->text());
	m_dSMile = sMile;
	m_dEMile = eMile;
	m_nowStandard = type;
	return true;
}
 

void hnOutputExcelDialog::readExcelConfigData()
{

	QMap<QString, QMap<QString, QStringList>> allDatas;

	/*m_configPath = QCoreApplication::applicationDirPath() + "\\config\\OutDataSmallSetting.json";

	if (m_nowDrawType == 1 || m_nowDrawType == 0)
	{
		m_configPath = QCoreApplication::applicationDirPath() + "\\config\\OutDataSmallSetting.json";

	}*/

	  if (m_nowDrawType == 0)
	{

		m_configPath = QCoreApplication::applicationDirPath() + "\\config\\OutDataSetting.json";
	}
	else if (m_nowDrawType==1)
	{
		m_configPath = QCoreApplication::applicationDirPath() + "\\config\\OutDataSmallSetting.json";

	} 
	else if (m_nowDrawType == 2)
	{
		m_configPath = QCoreApplication::applicationDirPath() + "\\config\\OutDataDesignSetting.json";
	}
	m_ExcelSelectConfig = AppConfig::loadFromFile(m_configPath,m_nowDrawType);
}

void hnOutputExcelDialog::startOutExcelManager(const QString& excelDir, const QString&selectModelTxt, hnPro::hnProject*curProject,
	int & progressValue,
	QProgressDialog* process, int progressScale)
{
	    
		QMap<ReportItem*, QPair<QCheckBox*, QComboBox*>>::iterator it;
		for (it = m_ItemWidgetMap.begin(); it != m_ItemWidgetMap.end(); ++it)
		{
			ReportItem* reportItem = it.key();  
		 
			hnOutExcelManage::OutExcelManager
			(
				excelDir,
				selectModelTxt,
				m_nowStandard,
				m_nowDrawType,
				reportItem,
				curProject , 
				m_startMile, m_endMile, 
				progressValue, process, progressScale);
		}
	
}

void hnOutputExcelDialog::startOutExcelManager_Street(const QString& excelDir,
	const QString&selectModelTxt, 
	std::vector<hnPro::hnProject*>& allProject,
	const QMap<int, QVector<double>>&streetSelect,
	int & progressValue,
	QProgressDialog* process, int progressScale)
{
	//景观出表
	for (auto it = streetSelect.begin(); it != streetSelect.end(); ++it)
	{
		int key = it.key();
		QVector<double> value = it.value();
		hnOutExcelManage::OutExcelManager_Street(
			excelDir,key,
			value,
			m_nowStandard,
			m_nowDrawType,
			m_currentPorject,
			m_startMile,
			m_endMile
			,progressValue, process, progressScale
		
		);
	}
}

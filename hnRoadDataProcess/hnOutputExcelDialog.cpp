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
#include <QStandardPaths>
#include <QDir>
#include <QFileDialog>
#include <QProgressDialog>
#include <QProgressBar>
#include <QEventLoop>
#include <QDesktopServices>
#include "../hnQtCommon/MyCommonMethods.h" 
#include "../ActiveQt/QAxObject"
#include <QFontMetrics>
#include <QPainter>
#include <QDoubleValidator>
#include <QHeaderView>
#include <QTableWidget>
#include <QtGlobal>
#include "hnReportProjectInfo.h"

namespace
{
	void setReportComboText(QComboBox* comboBox, const QString& text)
	{
		if (!comboBox || text.trimmed().isEmpty())
		{
			return;
		}
		int index = comboBox->findText(text);
		if (index < 0)
		{
			comboBox->addItem(text);
			index = comboBox->findText(text);
		}
		comboBox->setCurrentIndex(index);
	}

	void reportCommonProjectInfoFromUi(const Ui::hnOutputExcelDialog& ui, hnReportProjectInfo::Data& data)
	{
		data.maintenanceUnit = ui.lineEdit_4->text().trimmed();
		data.laneType = ui.lineEdit_6->text().trimmed();
		data.taskYear = ui.comboBox_5->currentText().trimmed();
		data.inspectionCount = ui.comboBox_6->currentText().trimmed();
		data.regionCode = ui.lineEdit_8->text().trimmed();
	}

	QString projectDisplayName(hnPro::hnProject* project)
	{
		if (!project)
		{
			return QStringLiteral("<空工程>");
		}
		QString name = project->get2DProName();
		if (name.isEmpty())
		{
			name = project->get3DProName();
		}
		return name.isEmpty() ? QStringLiteral("<未命名工程>") : name;
	}

	QString drawTypeText(int drawType)
	{
		switch (drawType)
		{
		case 0:
			return QStringLiteral("人工模式");
		case 1:
			return QStringLiteral("自动化模式");
		case 2:
			return QStringLiteral("设计模式");
		default:
			return QStringLiteral("未知模式(%1)").arg(drawType);
		}
	}

	QString projectTypeText(PROJECT_TYPE projectType)
	{
		switch (projectType)
		{
		case PROJECT_TYPE::PROJECT_2D_TYPE:
			return QStringLiteral("二维工程");
		case PROJECT_TYPE::PROJECT_23D_TYPE:
			return QStringLiteral("二三维工程");
		case PROJECT_TYPE::PROJECT_XD_3D_TYPE:
			return QStringLiteral("三维工程");
		default:
			return QStringLiteral("未知工程类型(%1)").arg(static_cast<int>(projectType));
		}
	}
}

bool hnOutputExcelDialog::validateProjectCompatibility(const std::vector<hnPro::hnProject*>& projects, QString& errorMessage)
{
	errorMessage.clear();
	if (projects.empty() || !projects.front())
	{
		errorMessage = QStringLiteral("当前没有可用于出表的工程。");
		return false;
	}

	hnPro::hnProject* baseProject = projects.front();
	const auto baseInfo = baseProject->getCurProSetInfo();
	const PROJECT_TYPE baseProjectType = baseProject->getProjectType();
	const QString baseStandard = QString::fromLocal8Bit(baseInfo.strRoadStandard);
	const int baseDrawType = baseInfo.nDrawType;
	QStringList differences;

	for (hnPro::hnProject* project : projects)
	{
		if (!project)
		{
			differences.append(QStringLiteral("存在空工程对象"));
			continue;
		}
		const auto info = project->getCurProSetInfo();
		const QString name = projectDisplayName(project);
		const QString standard = QString::fromLocal8Bit(info.strRoadStandard);
		if (project->getProjectType() != baseProjectType)
		{
			differences.append(QStringLiteral("工程【%1】工程类型为%2，期望%3")
				.arg(name, projectTypeText(project->getProjectType()), projectTypeText(baseProjectType)));
		}
		if (standard != baseStandard)
		{
			differences.append(QStringLiteral("工程【%1】道路标准为【%2】，期望【%3】")
				.arg(name, standard, baseStandard));
		}
		if (info.nDrawType != baseDrawType)
		{
			differences.append(QStringLiteral("工程【%1】绘制方式为%2，期望%3")
				.arg(name, drawTypeText(info.nDrawType), drawTypeText(baseDrawType)));
		}
	}

	if (!differences.isEmpty())
	{
		errorMessage = QStringLiteral("同一次多工程出表要求工程类型、道路标准和绘制方式一致。\n\n%1")
			.arg(differences.join(QStringLiteral("\n")));
		return false;
	}
	return true;
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
	ui.lineEdit_7->setValidator(new QDoubleValidator(0.01, 1000.0, 3, ui.lineEdit_7));

	hnPro::hnProjectManager* projectManager = hnApp::hnDataManager::getDataManager()->getProjectManager();
	if (!projectManager)
	{
		return;
	}
	const auto projects = projectManager->getAllBaseProject();
	if (projects.empty())
	{
		return;
	}
	if (projects.size() > 1)
	{
		ui.groupBox_12->setTitle(QStringLiteral("工程信息（公共信息应用到当前全部工程）"));
	}

	const hnReportProjectInfo::Data data = hnReportProjectInfo::load(projects.at(0));
	if (projects.size() == 1)
	{
		ui.lineEdit_7->setText(QString::number(data.roadWidth, 'g', 12));
		const bool lineCamera = projects.at(0)->isLineCameraProject();
		ui.lineEdit_7->setReadOnly(lineCamera);
		ui.lineEdit_7->setToolTip(lineCamera
			? QStringLiteral("线阵工程道路宽度由有效区域自动计算，不允许手工修改。")
			: QString());
	}
	else
	{
		setupMultiProjectWidthTable(projects);
	}
	ui.lineEdit_4->setText(data.maintenanceUnit);
	ui.lineEdit_6->setText(data.laneType);
	setReportComboText(ui.comboBox_5, data.taskYear);
	setReportComboText(ui.comboBox_6, data.inspectionCount);
	ui.lineEdit_8->setText(data.regionCode);
}

void hnOutputExcelDialog::setupMultiProjectWidthTable(const std::vector<hnPro::hnProject*>& projects)
{
	m_reportProjects = projects;
	ui.label_3->setVisible(false);
	ui.lineEdit_7->setVisible(false);

	if (!m_projectWidthTable)
	{
		m_projectWidthTable = new QTableWidget(ui.groupBox_12);
		m_projectWidthTable->setColumnCount(5);
		m_projectWidthTable->setHorizontalHeaderLabels(QStringList()
			<< QStringLiteral("工程名称")
			<< QStringLiteral("道路标准")
			<< QStringLiteral("绘制方式")
			<< QStringLiteral("检测路面宽度(m)")
			<< QStringLiteral("宽度来源"));
		m_projectWidthTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
		m_projectWidthTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
		m_projectWidthTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
		m_projectWidthTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
		m_projectWidthTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
		m_projectWidthTable->verticalHeader()->setVisible(false);
		m_projectWidthTable->setAlternatingRowColors(true);
		m_projectWidthTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		m_projectWidthTable->setMinimumHeight(150);
		m_projectWidthTable->setMaximumHeight(260);
		ui.gridLayout_3->addWidget(m_projectWidthTable, 7, 0, 1, 2);
	}

	m_projectWidthTable->setRowCount(static_cast<int>(projects.size()));
	for (int row = 0; row < static_cast<int>(projects.size()); ++row)
	{
		hnPro::hnProject* project = projects.at(row);
		const auto info = project->getCurProSetInfo();
		const hnReportProjectInfo::Data reportInfo = hnReportProjectInfo::load(project);
		QTableWidgetItem* nameItem = new QTableWidgetItem(projectDisplayName(project));
		QTableWidgetItem* standardItem = new QTableWidgetItem(QString::fromLocal8Bit(info.strRoadStandard));
		QTableWidgetItem* drawTypeItem = new QTableWidgetItem(drawTypeText(info.nDrawType));
		QTableWidgetItem* widthItem = new QTableWidgetItem(QString::number(reportInfo.roadWidth, 'g', 12));
		const bool lineCamera = project->isLineCameraProject();
		QTableWidgetItem* sourceItem = new QTableWidgetItem(lineCamera
			? QStringLiteral("线阵有效区域")
			: (hnReportProjectInfo::hasSavedRoadWidth(project)
				? QStringLiteral("报表配置") : QStringLiteral("工程道路宽度")));
		nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
		standardItem->setFlags(standardItem->flags() & ~Qt::ItemIsEditable);
		drawTypeItem->setFlags(drawTypeItem->flags() & ~Qt::ItemIsEditable);
		sourceItem->setFlags(sourceItem->flags() & ~Qt::ItemIsEditable);
		if (lineCamera)
		{
			widthItem->setFlags(widthItem->flags() & ~Qt::ItemIsEditable);
			widthItem->setToolTip(QStringLiteral("线阵工程道路宽度由有效区域自动计算。"));
		}
		m_projectWidthTable->setItem(row, 0, nameItem);
		m_projectWidthTable->setItem(row, 1, standardItem);
		m_projectWidthTable->setItem(row, 2, drawTypeItem);
		m_projectWidthTable->setItem(row, 3, widthItem);
		m_projectWidthTable->setItem(row, 4, sourceItem);
	}
}

bool hnOutputExcelDialog::projectRoadWidthsFromUi(QMap<hnPro::hnProject*, double>& widths, int& invalidRow) const
{
	widths.clear();
	invalidRow = -1;
	if (!m_projectWidthTable || m_projectWidthTable->rowCount() != static_cast<int>(m_reportProjects.size()))
	{
		return false;
	}
	for (int row = 0; row < m_projectWidthTable->rowCount(); ++row)
	{
		QTableWidgetItem* widthItem = m_projectWidthTable->item(row, 3);
		bool widthOk = false;
		const double width = widthItem ? widthItem->text().trimmed().toDouble(&widthOk) : 0.0;
		if (!widthOk || !qIsFinite(width) || width <= 0.0)
		{
			invalidRow = row;
			return false;
		}
		widths.insert(m_reportProjects.at(row), width);
	}
	return true;
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
	hnReportProjectInfo::Data commonReportProjectInfo;
	reportCommonProjectInfoFromUi(ui, commonReportProjectInfo);
	QMap<hnPro::hnProject*, double> projectWidths;
	int invalidWidthRow = -1;
	if (isSingleProject)
	{
		bool widthOk = false;
		const double width = ui.lineEdit_7->text().trimmed().toDouble(&widthOk);
		if (!widthOk || !qIsFinite(width) || width <= 0.0)
		{
			QMessageBox::critical(this, QStringLiteral("参数错误"), QStringLiteral("检测路面宽度必须是大于 0 的有效数字。"));
			ui.lineEdit_7->setFocus();
			ui.lineEdit_7->selectAll();
			return;
		}
		projectWidths.insert(m_currentPorject, width);
	}
	else if (!projectRoadWidthsFromUi(projectWidths, invalidWidthRow))
	{
		QMessageBox::critical(this, QStringLiteral("参数错误"),
			invalidWidthRow >= 0
			? QStringLiteral("工程【%1】的检测路面宽度必须是大于 0 的有效数字。")
				.arg(m_projectWidthTable->item(invalidWidthRow, 0)->text())
			: QStringLiteral("无法读取多工程检测路面宽度。"));
		if (invalidWidthRow >= 0)
		{
			m_projectWidthTable->setCurrentCell(invalidWidthRow, 3);
			m_projectWidthTable->scrollToItem(m_projectWidthTable->item(invalidWidthRow, 3));
			m_projectWidthTable->editItem(m_projectWidthTable->item(invalidWidthRow, 3));
		}
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
		hnReportProjectInfo::Data projectReportInfo = commonReportProjectInfo;
		projectReportInfo.roadWidth = project->isLineCameraProject()
			? project->effectiveRoadWidth()
			: projectWidths.value(project, 0.0);
		if (!hnReportProjectInfo::save(project, projectReportInfo))
		{
			QMessageBox::critical(this, QStringLiteral("保存失败"),
				QStringLiteral("无法把工程信息写入工程配置文件：\n%1")
				.arg(QDir::toNativeSeparators(hnReportProjectInfo::configPath(project))));
			return;
		}
	}
	 
	int count = allProject.size();
#pragma region 设置进度条
	
	if (!m_progressDialog)
	{
		m_progressDialog = new QProgressDialog(this);
	
	} 
	else
	{

	}
	m_progressDialog->setWindowTitle(QStringLiteral("导出报表"));
	m_progressDialog->setWindowModality(Qt::ApplicationModal);
	m_progressDialog->setMinimumDuration(0);
	m_progressDialog->setMinimumSize(QSize(640, 150));
	m_progressDialog->setAutoClose(false);
	m_progressDialog->setAutoReset(false);
	m_progressDialog->setCancelButton(nullptr);
	m_progressValue = 0;
	const int totalTaskCount = m_UserSelectCount * count;
	m_progressDialog->setRange(0, totalTaskCount);
	m_progressDialog->setValue(0);
	m_progressDialog->setLabelText(QStringLiteral("正在准备导出任务..."));
	m_progressDialog->setStyleSheet(QStringLiteral(
		"QProgressDialog { background: #F7F9FC; }"
		"QLabel { color: #243247; font-size: 14px; padding: 10px 18px 4px 18px; }"
		"QProgressBar { min-height: 24px; margin: 4px 18px 14px 18px; border: 1px solid #CBD5E1; "
		"border-radius: 7px; background: #E8EDF4; color: #172033; font-weight: 600; text-align: center; }"
		"QProgressBar::chunk { border-radius: 6px; background: #3478D4; }"));
	if (QProgressBar* progressBar = m_progressDialog->findChild<QProgressBar*>())
	{
		progressBar->setFormat(QStringLiteral("总体进度  %p%    %v / %m 项"));
		progressBar->setTextVisible(true);
	}

	m_progressDialog->show();

	for (int i = 0; i < count; ++i)
	{
		m_currentPorject = allProject.at(i);
		//进度条设置
	//	m_progressDialog->reset();
		QString projectName ="";
		if (m_nowProjectType == PROJECT_TYPE::PROJECT_23D_TYPE || m_nowProjectType == PROJECT_TYPE::PROJECT_2D_TYPE)
		{
			projectName = m_currentPorject->get2DProName();
		}
		else
		{
			projectName = m_currentPorject->get3DProName();
		} 
		 
		const QString projectProgressText = QStringLiteral("工程 %1 / %2：%3")
			.arg(i + 1).arg(count).arg(projectName);
		m_progressDialog->setProperty("projectProgressText", projectProgressText);
		m_progressDialog->setLabelText(projectProgressText + QStringLiteral("\n正在准备报表..."));
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

		startOutExcelManager(saveExcelDir, selectModelTxt, m_currentPorject	 ,m_progressValue, m_progressDialog);

		startOutExcelManager_Street(saveExcelDir,selectModelTxt, allProject, StreetSelectMsg,	m_progressValue, m_progressDialog);




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
	{
		m_progressDialog->setValue(m_progressDialog->maximum());
		m_progressDialog->setLabelText(QStringLiteral("所有工程已处理完成\n正在整理导出结果..."));
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		m_progressDialog->hide();
	}

	m_xrSetting->outExcel = true;
	const QStringList exportErrors = m_xrSetting->ExcelErrorMessageList;
	QMessageBox resultBox(this);
	resultBox.setWindowTitle(exportErrors.isEmpty() ? QStringLiteral("报表导出完成")
		: QStringLiteral("报表导出完成（有数据提示）"));
	resultBox.setIcon(exportErrors.isEmpty() ? QMessageBox::Information : QMessageBox::Warning);
	resultBox.setText(exportErrors.isEmpty()
		? QStringLiteral("所有报表任务已处理完成。")
		: QStringLiteral("报表任务已处理完成，共汇总 %1 项数据问题。\n导出过程中未重复弹窗，请展开“详细信息”统一查看。")
			.arg(exportErrors.size()));
	resultBox.setInformativeText(QStringLiteral("输出目录：%1").arg(QDir::toNativeSeparators(m_xrSetting->OutPath)));
	if (!exportErrors.isEmpty())
	{
		resultBox.setDetailedText(exportErrors.join(QStringLiteral("\n\n")));
	}
	QPushButton* openFolderButton = resultBox.addButton(QStringLiteral("打开输出目录"), QMessageBox::ActionRole);
	resultBox.addButton(QStringLiteral("关闭"), QMessageBox::AcceptRole);
	resultBox.exec();
	if (resultBox.clickedButton() == openFolderButton)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(m_xrSetting->OutPath));
	}

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
	QProgressDialog* process)
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
				progressValue, process);
		}
	
}

void hnOutputExcelDialog::startOutExcelManager_Street(const QString& excelDir,
	const QString&selectModelTxt, 
	std::vector<hnPro::hnProject*>& allProject,
	const QMap<int, QVector<double>>&streetSelect,
	int & progressValue,
	QProgressDialog* process)
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
			,progressValue, process
		
		);
	}
}

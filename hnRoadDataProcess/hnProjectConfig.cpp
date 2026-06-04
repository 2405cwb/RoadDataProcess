#include "hnProjectConfig.h"
#include <QButtonGroup>
#include <QMessageBox>
#include <QTextBrowser>
#include "..\hnConfigService\HnXRSettings.h"
hnProjectConfig::hnProjectConfig(QWidget *parent)
{
	m_xrSetting = HnXRSettings::getInstance();
	
	ui.setupUi(this);
	this->setWindowTitle(QString::fromLocal8Bit("软件设置"));
	QTextBrowser* diseaseHelpBrowser = new QTextBrowser(this);
	diseaseHelpBrowser->setOpenExternalLinks(false);
	diseaseHelpBrowser->setReadOnly(true);
	diseaseHelpBrowser->setText(QStringLiteral(
		"病害操作说明\n\n"
		"通用：\n"
		"1. 普通状态下，左键点击病害可选中病害。\n"
		"2. 选中病害后，按 Delete 可删除当前选中的病害。\n"
		"3. F1 为添加病害，F2 为删除病害，F3 为编辑病害。\n\n"
		"人工模式：\n"
		"1. F1 后左键拖框或点选绘制病害；正在绘制时右键取消当前绘制。\n"
		"2. F2 后左键点击病害删除。\n"
		"3. F3 后左键点击病害区域编辑。\n\n"
		"自动化模式：\n"
		"1. F1 后左键开始/完成自动化小方格病害绘制。\n"
		"2. F1 下未处于绘制状态时，右键点击病害可选中病害。\n"
		"3. D 键切换拉框填充小方格模式；拖动时只显示大框，确认病害类型后显示小方格病害。\n"
		"4. B 键切换线状小方格绘制；N 键结束线状绘制并进入病害选择。\n"
		"5. F2 后左键删除整个病害，右键删除单个小方格，按住右键拖动可连续删除小方格。\n"
		"6. F3 后左键点击病害区域编辑。\n\n"
		"设计模式：\n"
		"1. 添加病害请使用“添加面状病害”或“添加线状病害”按钮。\n"
		"2. F2 后左键点击设计病害删除。\n"
		"3. F3 后左键点击设计病害编辑。"));
	ui.tabWidget->addTab(diseaseHelpBrowser, QStringLiteral("病害操作说明"));
	connect(ui.horizontalScrollBar, &QScrollBar::valueChanged, [=](int value)
	{
		ui.label_9->setText(QStringLiteral("图片退回阈值:%1").arg(QString::number(value)));
	});
}

hnProjectConfig::~hnProjectConfig()
{
}

void hnProjectConfig::on_cancelButton_clicked()
{
	this->reject();
}

void hnProjectConfig::on_okButton_clicked()
{	
	//视图设置
	this->widgetSetting();

	//数据处理设置
	this->dataProcessSetting();

	m_xrSetting->writeData();
	//提示用户设置成功
	QMessageBox::information(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("设置成功")
		,QString::fromLocal8Bit("确定"));
}

void hnProjectConfig::initWidgetSetting()
{
	ui.horizontalScrollBar->setValue(m_xrSetting->movePictureBackMouseRatio);
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}


	//二维镜像设置
	if (hnDataManager::getDataManager()->getCurrentProject()->get2DProject())
	{
		ui.hMirrored2dCheckBox->setChecked(hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getIsHMirrored());
		ui.vMirrored2dCheckBox->setChecked(hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getIsVMirrored());
	}
	//三维镜像设置
	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		ui.hMirrored3dCheckBox->setChecked(hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getIsHMirrored());
		ui.vMirrored3dCheckBox->setChecked(hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getIsVMirrored());
	}
	//三维路面宽度
	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		QString roadWidth = QString::number(hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getRoadWidth());
		ui.width3dLineEdit->setText(roadWidth);
	}
}

void hnProjectConfig::initDataProcessSettting()
{
	configService config;
	QString configName = QApplication::applicationDirPath() + "/config/XRSetting.ini";
	config.loadCfg(configName);
	bool value = config.getValueDft("PROJECT", "IS_DEAP_CALCULATE", "0").toInt();
	if (value)
	{
		ui.isDepthCaculateCheckBox->setChecked(value);
	}
}

void hnProjectConfig::widgetSetting()
{
	//镜像设置
	emit signal_Mirrored(ui.hMirrored2dCheckBox->isChecked(), 
		ui.vMirrored2dCheckBox->isChecked(),
		ui.hMirrored3dCheckBox->isChecked(),
		ui.vMirrored3dCheckBox->isChecked());
	//三维路面宽度设置
	if (hnDataManager::getDataManager()->isOpenProject())
	{
		if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
		{
			hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->setRoadWidth(ui.width3dLineEdit->text().toDouble());
		}
	}


}

void hnProjectConfig::dataProcessSetting()
{
	configService config;
	QString configName = QApplication::applicationDirPath() + "/config/XRSetting.ini";
	config.loadCfg(configName);
	config.setValue("PROJECT", "IS_DEAP_CALCULATE", QString::number(ui.isDepthCaculateCheckBox->isChecked()));
	emit signal_isDepthCaculate(ui.isDepthCaculateCheckBox->isChecked());

	m_xrSetting->movePictureBackMouseRatio = ui.horizontalScrollBar->value();
	
}

void hnProjectConfig::closeEvent(QCloseEvent * event)
{
	event->ignore();
	hide();
}

void hnProjectConfig::showEvent(QShowEvent * event)
{
	//初始化视图设置
	this->initWidgetSetting();
	//初始化数据处理设置
	this->initDataProcessSettting();
	
	
}



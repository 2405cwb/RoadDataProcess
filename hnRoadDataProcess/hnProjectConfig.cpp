#include "hnProjectConfig.h"
#include <QButtonGroup>
#include <QMessageBox>
#include "..\hnConfigService\HnXRSettings.h"
hnProjectConfig::hnProjectConfig(QWidget *parent)
{
	m_xrSetting = HnXRSettings::getInstance();
	
	ui.setupUi(this);
	this->setWindowTitle(QString::fromLocal8Bit("软件设置"));

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



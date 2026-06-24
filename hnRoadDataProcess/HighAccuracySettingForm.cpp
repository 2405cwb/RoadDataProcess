#include "HighAccuracySettingForm.h"
#include "../hnQtCommon/MyCommonMethods.h"
#include <QCoreApplication>
#include "../hnQtCommon/BaseException.h"
#include <QPushButton>
#include<QComboBox>
#include <QCheckBox>
HighAccuracySettingForm::HighAccuracySettingForm(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	 
	connect(ui.pushButton, &QPushButton::clicked, this, &HighAccuracySettingForm::ok_slot);
	connect(ui.pushButton_2, &QPushButton::clicked, this, &HighAccuracySettingForm::cancel_slot);
	connect(ui.comboBox3, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HighAccuracySettingForm::comboxChanged);
	connect(ui.checkBox, &QCheckBox::stateChanged, this, &HighAccuracySettingForm::checkedChanged);
	readParams();

	setContainerEnabled(ui.groupBox_3, true);
}

HighAccuracySettingForm::~HighAccuracySettingForm()
{
	 
}

POS_CONVERT_INFO HighAccuracySettingForm::getConfigInfo()
{
	return localInfo;
}

void HighAccuracySettingForm::ok_slot()
{
	setParams();
	saveParams();
	this->accept();
}
void HighAccuracySettingForm::cancel_slot()
{
	this->reject();
}

void HighAccuracySettingForm::comboxChanged(int index)
{
	switch (index)
	{
	case  0 :
		setContainerEnabled(ui.groupBox_3, false);
		setContainerEnabled(ui.groupBox_4, true);
		break;
	case  1:
		setContainerEnabled(ui.groupBox_3, true);
		setContainerEnabled(ui.groupBox_4, false);

		break;
	default:
		break;
	}
}

void HighAccuracySettingForm::checkedChanged(bool status)
{ 
	setContainerEnabled(ui.groupBox_3, status);
	setContainerEnabled(ui.groupBox_4, status);
}

void HighAccuracySettingForm::readParams()
{
	QString baseAppDirPath = QCoreApplication::applicationDirPath() + "/HighAccuracySettingFormParams.ini";
	QString localCfgPath =  MyCommonMethods::GetUserPath() + "/HighAccuracySettingFormParams.ini";
	if (QFile::exists(localCfgPath))
	{
		m_iniFilePath = localCfgPath;
	}
	else
	{
		QFile::copy(baseAppDirPath, localCfgPath);
		m_iniFilePath = localCfgPath;

	}

	if (m_iniFilePath.isEmpty())
	{
		THROW_RUNTIME(QStringLiteral("HighAccuracySettingFormParams配置文件缺失！"));  
		return;

	}
	m_Setting = configService::getPtr();
	m_Setting->loadCfg(m_iniFilePath); 
	localInfo.dCenterL = m_Setting->ReadDouble("SETTING", "CenterL", 0);
	localInfo.nSphereType = m_Setting->ReadInteger("SETTING", "nSphereType", 0);
	localInfo.nProjectType = m_Setting->ReadInteger("SETTING", "nProjectType", 0);
	localInfo.dProjectHeight = m_Setting->ReadDouble("SETTING", "dProjectHeight", 0);
	localInfo.dEastAdd = m_Setting->ReadDouble("SETTING", "dEastAdd", 0);
	localInfo.dAverageLat = m_Setting->ReadDouble("SETTING", "dAverageLat", 0);
	localInfo.dProjectScale = m_Setting->ReadDouble("SETTING", "dProjectScale", 0);
	localInfo.nUseConvertModel = m_Setting->ReadInteger("SETTING", "nUseConvertModel", 0);
	localInfo.dFourX = m_Setting->ReadDouble("SETTING", "dFourX", 0);
	localInfo.dFourY = m_Setting->ReadDouble("SETTING", "dFourY", 0);
	localInfo.dFourR = m_Setting->ReadDouble("SETTING", "dFourR", 0);
	localInfo.dFourK = m_Setting->ReadDouble("SETTING", "dFourK", 0);
	localInfo.dOffsetX = m_Setting->ReadDouble("SETTING", "dOffsetX", 0);
	localInfo.dOffsetY = m_Setting->ReadDouble("SETTING", "dOffsetY", 0);
	localInfo.dOffsetZ = m_Setting->ReadDouble("SETTING", "dOffsetZ", 0);
	localInfo.dRotateX = m_Setting->ReadDouble("SETTING", "dRotateX", 0);
	localInfo.dRotateY = m_Setting->ReadDouble("SETTING", "dRotateY", 0);
	localInfo.dRotateZ = m_Setting->ReadDouble("SETTING", "dRotateZ", 0);
	localInfo.dK = m_Setting->ReadDouble("SETTING", "dK", 0);
	setFormParam();
}

void HighAccuracySettingForm::saveParams()
{
	m_Setting->WriteDouble("SETTING", "CenterL", localInfo.dCenterL);
	m_Setting->WriteInteger("SETTING", "nSphereType", localInfo.nSphereType);
	m_Setting->WriteInteger("SETTING", "nProjectType", localInfo.nProjectType);
	m_Setting->WriteDouble("SETTING", "dProjectHeight", localInfo.dProjectHeight);
	m_Setting->WriteDouble("SETTING", "dEastAdd", localInfo.dEastAdd);
	m_Setting->WriteDouble("SETTING", "dAverageLat", localInfo.dAverageLat);
	m_Setting->WriteDouble("SETTING", "dProjectScale", localInfo.dProjectScale);
	m_Setting->WriteInteger("SETTING", "nUseConvertModel", localInfo.nUseConvertModel);
	m_Setting->WriteDouble("SETTING", "dFourX", localInfo.dFourX);
	m_Setting->WriteDouble("SETTING", "dFourY", localInfo.dFourY);
	m_Setting->WriteDouble("SETTING", "dFourR", localInfo.dFourR);
	m_Setting->WriteDouble("SETTING", "dFourK", localInfo.dFourK); 
	m_Setting->WriteDouble("SETTING", "dOffsetX", localInfo.dOffsetX);
	m_Setting->WriteDouble("SETTING", "dOffsetY", localInfo.dOffsetY);
	m_Setting->WriteDouble("SETTING", "dOffsetZ", localInfo.dOffsetZ);
	m_Setting->WriteDouble("SETTING", "dRotateX", localInfo.dRotateX);
	m_Setting->WriteDouble("SETTING", "dRotateY", localInfo.dRotateY);
	m_Setting->WriteDouble("SETTING", "dRotateZ", localInfo.dRotateZ);
	m_Setting->WriteDouble("SETTING", "dK", localInfo.dK);
}

void HighAccuracySettingForm::setParams()
{
	localInfo.dCenterL = ui.lineEdit1->text().toDouble();
	localInfo.nSphereType = ui.comboBox1->currentIndex();
	localInfo.nProjectType = ui.comboBox2->currentIndex();
	localInfo.dProjectHeight = ui.lineEdit3->text().toDouble();
	localInfo.dEastAdd = ui.lineEdit2->text().toDouble();
	localInfo.dAverageLat = ui.lineEdit4->text().toDouble();
	localInfo.dProjectScale = ui.lineEdit5->text().toDouble();
	if (!ui.checkBox->isChecked())
	{
		localInfo.nUseConvertModel = 0;
	}
	else
	{
		switch (ui.comboBox3->currentIndex())
		{
		case 0:
			localInfo.nUseConvertModel = 2; break;
		case 1:
			localInfo.nUseConvertModel = 1; break;
		default:
			break;
		}
	}
	//这里注意顺序是否正确 
	localInfo.dFourX = ui.lineEdit6->text().toDouble();
	localInfo.dFourY = ui.lineEdit7->text().toDouble();
	localInfo.dFourR = ui.lineEdit8->text().toDouble();
	localInfo.dFourK = ui.lineEdit9->text().toDouble();
	//9 七参数值设置;
	localInfo.dOffsetX = ui.lineEdit19->text().toDouble();
	localInfo.dOffsetY = ui.lineEdit18->text().toDouble();
	localInfo.dOffsetZ = ui.lineEdit17->text().toDouble();
	localInfo.dRotateX = ui.lineEdit16->text().toDouble();
	localInfo.dRotateY = ui.lineEdit15->text().toDouble();
	localInfo.dRotateZ = ui.lineEdit14->text().toDouble();
	localInfo.dK = ui.lineEdit13->text().toDouble();
}

void HighAccuracySettingForm::setFormParam()
{
	ui.lineEdit1->setText(  QString::number( localInfo.dCenterL,'g',15));
	ui.comboBox1->setCurrentIndex(localInfo.nSphereType);
	ui.comboBox2->setCurrentIndex(localInfo.nProjectType);
	ui.lineEdit3->setText(QString::number(localInfo.dProjectHeight, 'g', 15));
	ui.lineEdit2->setText(QString::number(localInfo.dEastAdd, 'g', 15));
	ui.lineEdit4->setText(QString::number(localInfo.dAverageLat, 'g', 15));
	ui.lineEdit5->setText(QString::number(localInfo.dProjectScale, 'g', 15)); 
	switch (localInfo.nUseConvertModel)
	{
	case 0:
		ui.checkBox->setChecked(false); 
		break;
	case 1:
	{
		ui.checkBox->setChecked(true); 
		ui.comboBox3->setCurrentIndex(1);
	 

	}
	break;
	case 2:
	{
		ui.checkBox->setChecked(true);
		ui.comboBox3->setCurrentIndex(0);
	}
	break;
	default:
		break;
	}
	ui.lineEdit6->setText(QString::number(localInfo.dFourX, 'g', 15));
	ui.lineEdit7->setText(QString::number(localInfo.dFourY,'g',15));
	ui.lineEdit8->setText(QString::number(localInfo.dFourR,'g',15));
	ui.lineEdit9->setText(QString::number(localInfo.dFourK,'g',15));

	ui.lineEdit19->setText(QString::number(localInfo.dOffsetX,'g',15));
	ui.lineEdit18->setText(QString::number(localInfo.dOffsetY,'g',15));
	ui.lineEdit17->setText(QString::number(localInfo.dOffsetZ,'g',15));
	ui.lineEdit16->setText(QString::number(localInfo.dRotateX,'g',15));
	ui.lineEdit15->setText(QString::number(localInfo.dRotateY,'g',15));
	ui.lineEdit14->setText(QString::number(localInfo.dRotateZ,'g',15));
	ui.lineEdit13->setText(QString::number(localInfo.dK, 'g', 15));
	 
}

void HighAccuracySettingForm::setContainerEnabled(QWidget* container, bool enabled)
{

	if (!container)
	{
		return;
	}
	QList<QWidget*> widgets = container->findChildren<QWidget*>();
	for each (QWidget* widget in widgets)
	{
		widget->setEnabled(enabled);
	}
}

#pragma once
#include  "..\hnQtCommon\HighAccuracyInfo.h"

#include "..\HighAccConvertPlane\hnHighAcc2Plane.h"
#include <QDialog>
#include "ui_HighAccuracySettingForm.h"
#include "../hnConfigService/configService.h"

class HighAccuracySettingForm : public QDialog
{
	Q_OBJECT

public:
	HighAccuracySettingForm(QWidget *parent = Q_NULLPTR);
	~HighAccuracySettingForm();
	POS_CONVERT_INFO getConfigInfo();
public slots:
void ok_slot();
void cancel_slot();
void comboxChanged(int index);
void checkedChanged(bool status);
private:
	Ui::HighAccuracySettingForm ui;

	POS_CONVERT_INFO localInfo;

	configService* m_Setting;

	void readParams();
	void saveParams();

	void setParams();
	void setFormParam();
	void setContainerEnabled(QWidget* container, bool enabled);
	QString m_iniFilePath;
};

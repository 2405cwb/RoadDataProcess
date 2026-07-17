#pragma once

#include <QObject>
#include <QDialog> 
#include "ui_hnProjectConfig.h"
#include <QCloseEvent>
#include "../hnApplication/hnDataManager.h"
#include "../hnProject/hnProject.h"
#include "../hnProject/hn2DProject.h"
#include "../hnProject/hn3DProject.h"
#include "configService.h"
using namespace hnApp;

class hnProjectConfig final: public QDialog
{ 
	Q_OBJECT

public:
	hnProjectConfig(QWidget *parent = Q_NULLPTR); 
	~hnProjectConfig();

signals:
	void signal_Mirrored(bool isH2dMirrored, bool isV2dMirrored, bool isH3dMirrored, bool isV3dMirrored);
signals:
	void signal_isDepthCaculate(bool isCaculate);
	void signal_wheelScrollOneImageChanged(bool enabled);

private slots:
	void on_okButton_clicked();
	void on_cancelButton_clicked();

private:
	//初始化视图设置
	void initWidgetSetting();
	//初始化数据处理设置
	void initDataProcessSettting();

private:
	//进行视图设置
	void widgetSetting();
	//进行数据处理设置
	void dataProcessSetting();

protected:
	void closeEvent(QCloseEvent * event) override final;
	void showEvent(QShowEvent * event) override final;

	//单例  全局设置
	HnXRSettings* m_xrSetting;

private:
	Ui::hnProjectConfig ui;
};

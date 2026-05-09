#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QAction>
#include <QGroupBox>
#include <QPushButton>
#include "hnDataManager.h"
#include "../hnCommon/hnRoadStruct.h"
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>


//添加景观病害的对话框
class hnAddStreetDiseaseDialog : public QDialog
{
	Q_OBJECT

public:
	hnAddStreetDiseaseDialog(QVector<hnCommon::hnDiseaseSetInfo> LJInfo, QVector<hnCommon::hnDiseaseSetInfo>YXInfo, QWidget *parent = Q_NULLPTR);
	~hnAddStreetDiseaseDialog();
public:
	void setCurrentHnMile(const hnMile &mile);

	//获取对话框选中的病害
	QVector<hnRoadDiseaseInfo> getSelectDiseases();
private:
	//初始化沿线设施布局
	void initYXLayout();

	//初始化路基损坏布局
	void initLJLayout();

	//初始化病害类型布局
	void initDiseaseLayout(QGridLayout *layout,const QVector<hnCommon::hnDiseaseSetInfo> &setInfos);


private slots:
	void slot_onOkPushButtonCliecked();

	void slot_onCancelPushButtonCliecked();

private:
	void writeDiseaseDataBase(QGridLayout *layout, const QVector<hnCommon::hnDiseaseSetInfo> &setInfos);

private:
	QGridLayout *m_YXlayout;	//沿线设施布局
	QGridLayout *m_LJlayout;	//路基损坏布局

	QVector<hnCommon::hnDiseaseSetInfo> m_LJDiseaseSetInfo;
	QVector<hnCommon::hnDiseaseSetInfo> m_YXDiseaseSetInfo;

	hnMile m_currentMile;

	//对话框选中的病害
	QVector<hnRoadDiseaseInfo> m_selectDiseases;

};

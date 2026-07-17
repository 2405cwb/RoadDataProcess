#pragma once

#include <QtWidgets/QDialog>
#include <QSharedPointer>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include "../hnApplication/hnDataManager.h"
#include <QSizePolicy>
#include <QScrollBar>
#include <QMap>
#include <QCheckBox>
#include "applyAllDialog.h"
#include "../hnProject/hnLineCameraConfig.h"
#include <QSet>
class hnOpenProjectDlg : public QDialog
{
    Q_OBJECT

public:
    hnOpenProjectDlg(const hnCommon::PROJECT_TYPE type,const QStringList &roadTypeNames, std::vector<hnCommon::hnProjectDataInfo> projectSettingInfos,QWidget *parent = Q_NULLPTR);

public:
	std::vector<hnCommon::hnProjectDataInfo> getProjectSettingInfo();

private:
	void slot_onOkPushButtonClicked(bool isClicked);

	void slot_onCancelPushButtonClicked(bool isClicked);

	void slot_onRoadTypeComboBoxCurrentTextChanged(const QString &text);

	void slot_onCheckAllCheckBoxCheckStateChangeed(bool state);

	void slot_onApplyAllPushButtonClicked(bool isClicked);
	void slot_onNextMissingLineAreaClicked();

private:
	QMap<QString, QMap<QString, QString>> readKeyValueFile(QString fileNme);

	void writeKeyValue(QMap<QString, QMap<QString, QString>>& keyValueMap,QString section,QString key,QString value);

	void writeKeyValueFile(QString fileName, QMap<QString, QMap<QString, QString>> keyValueMap);

private:
	//往界面上添加工程信息
	void addProjectInfoToWidget();
	void editLineCameraArea(int row);
	void updateLineCameraRow(int row);
	bool validateSelectedLineCameraAreas(QString& errorMessage) const;
	bool savePendingLineCameraAreas(QString& errorMessage);

	//将配置信息写入文件中   selectProjects使用户选中了的 并且 相关信息通过界面设置后 已经赋值完成的 工程信息 请保证我想要写入xml或者txt的信息是已赋值的
	void writeProjectInfoToFile1(const std::vector<hnCommon::hnProjectDataInfo> selectProjects);

	void writeProjectInfoToTxt(const hnCommon::hnProjectDataInfo projectDataInfo,bool is2d);

	

	//从路面标准中获取道路材质
	QStringList getRoadMeterials(const QString &roadStandard);

	//从路面标准中获得绘制模式
	QStringList getDrawTypes(const QString & roadStandard);

	//从路面标准中获取道路等级
	QStringList getRoadLevels(const QString &roadStandard);
private:
	void adjustWidgetSize();
private:
	QLabel * m_projectNameLabel;
	QLabel * m_roadTypeLabel;
	QLabel * m_drawDiseaseModelLabel;
	QLabel * m_roadWidthLabel;
	QLabel * m_roadMaterialLabel;		//路面材质
	QLabel * m_roadLevelLabel;			//路面等级
	QLabel* m_roadStartMileLabel; 
	QLabel* m_roadEndMileLable;

private:
	QPushButton* m_okPushButton;
	QPushButton* m_cancelPushButton;
	QCheckBox *m_checkAllCheckBox;
	QPushButton *m_applyAllPushButton;
	QPushButton *m_nextMissingLineAreaButton;
private:
	QGridLayout* m_scrollAreaGridLayout;
	QHBoxLayout *m_hBoxLayout;

private:
	QScrollArea* m_scrollArea;
	QWidget* m_scrollAreaWidget;
	QGridLayout* m_mainGridLayout;

private:
	//公路等级名字表格
	QStringList m_roadTypeNames;
	//工程信息
	std::vector<hnCommon::hnProjectDataInfo> m_projectDataInfos;
	QMap<int, hnPro::hnLineCameraInfo> m_lineCameraInfos;
	QMap<int, QLabel*> m_lineCameraStatusLabels;
	QMap<int, QLineEdit*> m_lineCameraWidthEdits;
	QSet<int> m_pendingLineCameraRows;
	hnCommon::PROJECT_TYPE m_proType;
};

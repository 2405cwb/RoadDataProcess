#pragma once

#include <QtWidgets/QMainWindow>
#include <QRadioButton>
#include <QGridLayout>
#include <QDebug>
#include <QPushButton>
#include <QGroupBox>
#include <QSharedPointer>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>
#include <QComboBox>

class HnXRSettings;

class addDiseaseDialog : public QDialog
{
    Q_OBJECT

public:
	addDiseaseDialog(const QList<QPair<QString, QString>>& diseaseNameAndKey,bool showDisProperty);

public:
	//获取选中的病害类型名字
	QString getDiseaseTypeName();

	//设置病害属性编辑权限
	void setDiseaseAttributeEnabled(const bool enabeled);

	//设置病害信息
	void setDiseaseInfo(double lenth,double width, double area,double depth);

	QString getDiseaseMarkInfo();
signals:
	void signal_diseaseRadioButtonToggled(QString &diseaseName);

private slots:
	void slot_onRadioButtonToggled(bool checked);

private:
	void initDiseaseInfoControls();

private:
	QGroupBox* m_diseaseInfoGroupBox = new QGroupBox(QString::fromLocal8Bit("病害信息"));
	QGroupBox* m_markInfoGroupBox = new QGroupBox(QString::fromLocal8Bit("备注信息"));
	QLabel* m_diseaseLenthLabel = new QLabel(QString::fromLocal8Bit("病害长度(m)"));
	QLineEdit* m_diseaseLenthLineEdit = new QLineEdit();
	QLabel* m_diseaseWidthLabel = new QLabel(QString::fromLocal8Bit("病害宽度(m)"));
	QLabel *m_areaLabel;		//面积label
	QLineEdit *m_areaLineEdit;	//面积lineEdit
	QLabel *m_depthLabel;		//深度label
	QLineEdit *m_depthLineEdit;	//深度lineEdit
	QLineEdit* m_diseaseWidthLineEdit = new QLineEdit();
	QSpacerItem* m_lenthWidthSpacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	QGridLayout* m_diseaseInfoGridLayout = new QGridLayout(m_diseaseInfoGroupBox);

	QGridLayout * m_markInfoGridLayout = new QGridLayout(m_markInfoGroupBox);

private:
	void initDiseaseTypeControls(const QList<QPair<QString, QString>>& diseaseNameAndKey);

	void initMarkInfo();
private:
	QGroupBox* m_diseaseTypeGroupBox  = new QGroupBox();
	QGridLayout* m_diseaseTypeGridLayout = new QGridLayout(m_diseaseTypeGroupBox);

private:
	QGridLayout* m_mainGridLayout = new QGridLayout(this);
	QComboBox * markEdit;
private:
	QString m_diseaseTypeName;
	

private:
	//是否显示病害属性
	bool m_isShowDiseaseInfo;

	HnXRSettings* m_xrSetting;
};

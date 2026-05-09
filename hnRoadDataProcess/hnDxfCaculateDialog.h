#pragma once

#include <QDialog>
#include <QGroupBox>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include "hnDxfInfoPreprocess.h"


class hnDxfCaculateDialog : public QDialog
{
	Q_OBJECT

public:
	hnDxfCaculateDialog(QWidget *parent = Q_NULLPTR);
	~hnDxfCaculateDialog();

private:
	//初始化ui
	void initUI();
	//初始化车道excel选择的布局
	void initLaneExcelChooseWidgets(QGridLayout *layout);
	//添加车道每行的窗口
	void addLaneRowWidgets(QGridLayout *layout,int row,const QString &labelText);
	//更新按钮状态 row为当前点击按钮的行序号
	void updatePushButtonState(QGridLayout *layout, int row);
	//更新lineEdit状态   row为当前点击按钮的行序号
	void updateLineEditState(QGridLayout *layout, int row);

private slots:
	//槽函数 选择报表文件按钮点击 
	void slot_chooseExcelPushButtonCliecked();
	//槽函数 取消按钮点击
	void slot_cancelPushButtonClicked();
	//槽函数 确定按钮点击
	void slot_okPushButtonClicked();
	//国省道checkBox状态变化
	void slot_nationProvincialRoadCheckBoxStateChanged(bool state);

private:
	// 国省道消息对话框提示
	void nationProvincialRoadMessageBoxInfo();
	
private:
	//预处理上行的文件
	int preprocessUpFile(const QString &saveDirName);

	//预处理下行的文件
	int preProcessDownFile(const QString &saveDirName);

	// 预处理全幅的文件
	int preProcessAllFile(const QString &saveDirName);

private:
	//根据用户在界面上的选择，获取excel表名，如果选择有问题，则返回空的QStringlist
	QStringList getExcelFileNames();
	//获取excel表名的集合 根据布局来
	QStringList getExcelFileNames(QGridLayout *layout);

private:
	//上行的group box
	QGroupBox *m_upGroupBox;
	//下行的group box
	QGroupBox *m_downGroupBox;
	//主布局 垂直布局
	QVBoxLayout *m_mainVBoxLineEdit;
	//上行的栅格布局
	QGridLayout *m_upGridLayout;
	//下行的栅格布局
	QGridLayout *m_downGridlayout;
	//最下面的选项布局
	QHBoxLayout *m_optionHBoxLayout;
	//全幅 radio button
	QRadioButton *m_allRadioButton;
	//上行右幅 radio button
	QRadioButton *m_rightRadioButton;
	//下行左幅  radio button
	QRadioButton *m_leftRadioButton;
	//国省道 check box
	QCheckBox *m_nationProvincialRoadCheckBox;
	//起始里程line edit
	QLineEdit *m_beginMileLineEdit;
	//终止里程line edit
	QLineEdit *m_endMileLineEdit;
	//确定按钮
	QPushButton *m_okPushButton;
	//取消按钮
	QPushButton *m_cancelPushButton;
	//路面宽度 line edit
	QLineEdit *m_roadWidthLineEdit;
	
};

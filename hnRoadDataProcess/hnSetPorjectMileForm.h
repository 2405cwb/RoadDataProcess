/*! hnIPS2ViewPluginIO.h
********************************************************************************
<PRE>
模块名       :  hnSetPorjectMileForm
文件名       :  hnSetPorjectMileForm.h
相关文件     :  hnSetPorjectMileForm.cpp
文件实现功能 :  多工程设置起始终止桩号窗口
作者         : 程文博
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 :
日期         版本     修改人              修改内容
2024/0905	 1.0     程文博          创建，实现
</PRE>
*******************************************************************************/

#pragma once
#include <QWidget>
#include "ui_hnSetPorjectMileForm.h"
#include <QDialog>
#include<qmap.h>
#include "../hnProject/hnProject.h"
#include <QGridLayout>
#include "SetPorjectMileStruct.h"
class hnSetPorjectMileForm : public QDialog
{
	Q_OBJECT

public:
	hnSetPorjectMileForm(const QMap<QString, QVector<hnPro::hnProject*>> datas, QWidget *parent = Q_NULLPTR);
	~hnSetPorjectMileForm();
private:
	
	//初始化数据
	void initDialog();
private:
	//所有工程信息
	QMap < QString, QVector<hnPro::hnProject*>> m_datas; 
private:
	Ui::hnSetPorjectMileForm ui;
	QWidget* m_scrollAreaWidget;
	QGridLayout* m_scrollAreaGridLayout;

	//所有工程对应的用户桩号信息
	QVector<SetPorjectMileStruct> m_MileInfos;

	//bool comparePorjectSMile(const hnPro::hnProject* a, const  hnPro::hnProject* b)
	//{
	//	if (a->getCurProSetInfo().nLineType==1)
	//	{
	//		return a->getCurProSetInfo().dBegMile < b->getCurProSetInfo().dBegMile;

	//	}
	//	else
	//	{
	//		return a->getCurProSetInfo().dBegMile > b->getCurProSetInfo().dBegMile;

	//	}
	//}
	//还原界面
	void restoreProjectForm(int index);
public slots :

//根据用户选择combox 更新工程界面
void slots_initProjectForm(int index);

//起始桩号修改
void slots_sMileValueChanged(QLineEdit * lineEdit, QString text);

//终点桩号修改
void slots_eMileValueChanged(QLineEdit * lineEdit, QString text);

//点击确定修改按钮
void slots_onOkButton();

//点击取消按钮
void slots_cancelButton();

//还原当前页配置
void slots_restoreButton();
private: 
	//项目索引
	int ProjectIndex = 1;
};

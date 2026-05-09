#pragma once 
#include <QDialog>
#include "ui_hnMileAdjustDlg.h"
#include "../hnCommon/hnRoadStruct.h"
class hnMileAdjustDlg : public QDialog
{
	Q_OBJECT

public:
	hnMileAdjustDlg(QVector<hnCommon::hnMilePile>& milePile,QWidget *parent = Q_NULLPTR);
	~hnMileAdjustDlg();
	QVector< hnCommon::hnMilePile>  getAllMileVec() { return m_milePile; }
private:
	Ui::hnMileAdjustDlg ui;
	QVector< hnCommon ::hnMilePile> m_milePile;

	//用户点击确定按钮后增加的 
	hnCommon::hnMilePile  m_currentInfo;
	//添加对话框
	QDialog* m_addMileFrom;
	QLineEdit * m_mileBox;
	QLineEdit * m_dmiBox;


};

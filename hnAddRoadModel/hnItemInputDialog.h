#pragma once

#include <QDialog>
#include "ui_hnItemInputDialog.h"
#include <QString>
//typedef struct Disease
//{
//	Disease()
//	{
//		drawtype = 0;
//		rodaType = 0;
//		disType = 0;
//		diseaseName = "";
//	}
//	int drawtype;
//	int rodaType;
//	int disType;
//	QString diseaseName;
//
//};
class hnItemInputDialog : public QDialog
{
	Q_OBJECT

public:
	hnItemInputDialog(QWidget *parent = Q_NULLPTR);
	~hnItemInputDialog();
	QString getDis();
	void setDis();
	void setDisStr();
	void clearDst();
private:
	Ui::hnItemInputDialog ui;
	QString itemName_;

private slots:
	void changeStrengSelect(int index);
	//Disease _userDis;
};

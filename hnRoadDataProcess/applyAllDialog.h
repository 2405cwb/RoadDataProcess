#pragma once

#include <QDialog>
#include <QStringList>
#include "ui_applyAllDialog.h"

class applyAllDialog : public QDialog
{
	Q_OBJECT

public:
	applyAllDialog(const QStringList &roadTypeNames, QWidget *parent = Q_NULLPTR);
	~applyAllDialog();

public:	
	QString getRoadType();	//获取道路等级
	QString getFrameType();	//获取人工模式还是自动化模式
	QString getRoadWidth();	//获取道路宽度

private:
	void slot_okPushButtonClicked();
	void slot_cancelPushButtonClicked();

private:
	Ui::applyAllDialog ui;
};

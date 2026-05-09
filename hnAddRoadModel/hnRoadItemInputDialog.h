#pragma once

#include <QWidget>
#include "ui_hnRoadItemInputDialog.h"
#include <QDialog>
class hnRoadItemInputDialog : public QDialog
{
	Q_OBJECT

public:
	hnRoadItemInputDialog(QWidget *parent = Q_NULLPTR);
	~hnRoadItemInputDialog();
	void cleartxt();
	QString getTxt();
public slots :
	void setDis();
	void handelChange(bool);
private:
	Ui::hnRoadItemInputDialog ui;
	QString itemName_;
};

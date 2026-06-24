#pragma once

#include <QDialog>
#include "ui_hnAboutInfoWidgets.h"

class hnAboutInfoWidgets :public QDialog
{
	Q_OBJECT

public:
	hnAboutInfoWidgets(QWidget *parent = Q_NULLPTR);
	~hnAboutInfoWidgets();


public slots:
void okButton_Slot();
void MessageButton_Slot();
private:
	Ui::hnAboutInfoWidgets ui;
};

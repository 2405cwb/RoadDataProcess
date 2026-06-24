#pragma once

#include <QDialog>
#include "ui_hnSelProjectDlg.h"

class hnSelProjectDlg : public QDialog
{
	Q_OBJECT

public:
	hnSelProjectDlg(QWidget *parent = Q_NULLPTR);
	~hnSelProjectDlg();

private:
	Ui::hnSelProjectDlg ui;
};

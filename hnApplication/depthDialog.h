#pragma once

#include <QDialog>
#include "ui_depthDialog.h"

class depthDialog : public QDialog
{
	Q_OBJECT

public:
	depthDialog(QWidget *parent = Q_NULLPTR);
	~depthDialog();

public:

	void setDepth(double depth);

	double getDepth();

public:
	void slot_okPushButtonClicked();

	void slot_cancelPushButtonClicked();

private:
	double m_depth;	//…Ó∂»

private:
	Ui::depthDialog ui;
};

#pragma once

#include <QWidget>
#include <QDialog>
#include "ui_ShowCtrlPointInfoDlg.h"

class ShowCtrlPointInfoDlg : public QDialog
{
	Q_OBJECT

public:
	ShowCtrlPointInfoDlg(QWidget *parent = Q_NULLPTR);
	~ShowCtrlPointInfoDlg();

public:
	void setName(const QString &name);
	void setGpsTime(double time);
	void setPixName(const QString &pixName);
	//123,345
	void setPixCoord(int x,int y);
	// 213,34,355
	void setPix3dCoord(double x, double y, double z);
	

private:
	Ui::ShowCtrlPointInfoDlg ui;
};

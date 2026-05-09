#pragma once 
#include <QDialog>
#include <QtCore>
#include <QtWidgets>
#include "..\hnApplication\hnDataManager.h"
#include "ui_hnDBManagerDlg.h"
using namespace hnApp;
class hnDBManagerDlg : public QDialog
{
	Q_OBJECT

public:
	hnDBManagerDlg(QWidget *parent = Q_NULLPTR);
	~hnDBManagerDlg();

private:
	//初始化界面
	void initialDlg();

	//初始化信号槽
	void initialConnect();

	//获取当前工程的所有成果db
	void getProjectAllDB();

private:
	//选择工程
	void selectProject();

	//db备份
	void dbCopy();

	//设置工程db
	void dbsetting();


	void settingDbFile(QString dbPath);

	//合并db
	void mergeDb();
private:
	Ui::hnDBManagerDlg ui;
};

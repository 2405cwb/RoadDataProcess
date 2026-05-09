#include "hnDBManagerDlg.h" 
#include <QHostInfo>
#include "..\hnQtCommon\MyCommonMethods.h"
#include "..\hnCommon\hnRoadStruct.h"
hnDBManagerDlg::hnDBManagerDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	this->setWindowTitle(QString::fromLocal8Bit("数据库操作"));
	initialDlg();

	initialConnect();
}

hnDBManagerDlg::~hnDBManagerDlg()
{
}

void hnDBManagerDlg::initialDlg()
{
	if (  hnDataManager::getDataManager()->isHasProject())
	{
		auto curProject = hnDataManager::getDataManager()->getCurrentProject();
		QString   projectPath =  curProject->getDbResultDirPath();

	ui.lineEdit->setText(projectPath);
	ui.lineEdit_source_2->setText(curProject->getDbResultFilePath().replace("\\","/"));
	}

	getProjectAllDB();
}

void hnDBManagerDlg::initialConnect()
{
	//选择工程
//	connect(ui.toolButton, &QToolButton::clicked, this, &hnDBManagerDlg::selectProject);

	//db备份
	connect(ui.pushButton_copy, &QPushButton::clicked, this, &hnDBManagerDlg::dbCopy);

	//db设置
	connect(ui.pushButton_db, &QPushButton::clicked, this, &hnDBManagerDlg::dbsetting);

	//合并db
	connect(ui.pushButton_copy_2, &QPushButton::clicked, this, &hnDBManagerDlg::mergeDb);

	
}

void hnDBManagerDlg::getProjectAllDB()
{
	QString projectPath = ui.lineEdit->text();
	QDir dir(projectPath);
	if (!dir.exists())
	{
		return;
	}

	//dir.setFilter(QDir::Files | QDir::NoSymLinks| QDir::AllDirs);
	QStringList filters;
	filters << "*.db";
	//dir.setNameFilters(filters);
	QStringList projectList ;

	  MyCommonMethods::findFilesWithPattern(projectPath,"*.db","*db",projectList);

	ui.comboBox_db->clear();
	for (int i = 0; i < projectList.size(); i++)
	{
		QFileInfo curFile(projectList[i]);
		QString curName = curFile.fileName();
		if (curName.contains(QString::fromLocal8Bit("成果")))
		{
			ui.comboBox_db->addItem(projectList[i]);
		}
		if (curName == QString::fromLocal8Bit("成果.db"))
		{
			ui.lineEdit_dest_2->setText(projectList[i]);
		}
	}

	//获取软件所在电脑名称
	QString PCName = QHostInfo::localHostName();

	QString strpath = ui.lineEdit->text() + "/dbSetting.txt";
	QDir dir1;
	//如果没有那么创建
	if (dir1.exists(strpath))
	{
		QSettings settings(strpath, QSettings::IniFormat);
		settings.setIniCodec(QTextCodec::codecForName("GBK"));

		//根据电脑名称找对应
		QString strValue = settings.value(QString::fromLocal8Bit("Project/DB_%1").arg(PCName)).toString();	
		if (strValue.isEmpty())//|| !dir1.exists(projectPath + "/" + strValue)
		{
			ui.comboBox_db->setCurrentText(QString::fromLocal8Bit("成果.db"));

			return;
		}

		ui.comboBox_db->setCurrentText(strValue.replace("\\", "/"));
	}

}

void hnDBManagerDlg::selectProject()
{
	//选择db文件
	QString strDirName = QFileDialog::getOpenFileName(NULL, QStringLiteral("请选择工程db"), "",
		QStringLiteral("成果db (*.db)"));
	strDirName.replace("\\", "/");
	strDirName.replace("//", "/");

	//获取上一级路径
	QString strProjectPath = "";
	QDir dir(strDirName);
	if (!dir.cdUp())
	{
		return;
	}

	strProjectPath = dir.absolutePath();
	ui.lineEdit->setText(strProjectPath);

	getProjectAllDB();


}

void hnDBManagerDlg::dbCopy()
{
	//源db
	QString strSource = ui.lineEdit->text() + "/" + ui.lineEdit_source->text();

	strSource = QDir::toNativeSeparators(strSource);
	QDir dirSource;
	if (!dirSource.exists(strSource))
	{
		QMessageBox::information(NULL, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请确认'源db'名称是否正确！"));
		return;
	}

	//目标db路径
	QString strDest = ui.lineEdit->text() + "/" + ui.lineEdit_dest->text();
	strDest = QDir::toNativeSeparators(strDest);

	//检查后缀是否正确
	QFileInfo fileInfo(strDest);
	QString suffix = fileInfo.suffix();
	if (suffix.compare("db") != 0)
	{
		QMessageBox::information(NULL, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请确认‘目标db’名称是否正确！"));
		return;
	}

	//文件已存在，是否覆盖
	QDir dirDest;
	if (dirDest.exists(strDest))
	{
		QMessageBox box(QMessageBox::Warning, QStringLiteral("警告"), QString::fromLocal8Bit("已存在该db,是否覆盖？"));
		box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		box.setButtonText(QMessageBox::Ok, QString::fromLocal8Bit("是"));
		box.setButtonText(QMessageBox::Cancel, QString::fromLocal8Bit("否"));
		if (box.exec() == QMessageBox::Cancel)
		{
			return;
		}
		else
		{
			dirDest.remove(strDest);
		}
	}
	auto curProject = hnDataManager::getDataManager()->getCurrentProject();
	auto diseaseTableNames =  curProject->getCurDB()->m_diseaseTable.GetAllDiseaseTableNames();
	if (QFile::copy(strSource, strDest))
	{
		string str = strDest.toLocal8Bit();
		//读取备份数据库 清空病害
		hnDBSqlite * dbSqlite = new hnDBSqlite(str.c_str());
		 
		if (!dbSqlite->connectDB(diseaseTableNames))
		{
			delete dbSqlite;
			dbSqlite = NULL;
			 
		}
		else
		{
			dbSqlite->m_diseaseTable.deleteAllDisease();
			delete dbSqlite;
			dbSqlite = NULL;
		}

		
	}
	settingDbFile(strDest);
	getProjectAllDB();

	QMessageBox::information(NULL, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("备份成功！请重新导入工程！"));
}

void hnDBManagerDlg::dbsetting()
{
	QString dbPath =  ui.comboBox_db->currentText();
	settingDbFile(dbPath);

	if (hnDataManager::getDataManager()->isOpenProject())
	{ 
		QMessageBox::information(NULL, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请重新导入工程！"));
		return;
	}
	else
	{
		QMessageBox::information(NULL, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("未检测到任何已打开的工程！"));
		return;
	}

//	QMessageBox::information(NULL, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("设置成功！"));

}

void hnDBManagerDlg::settingDbFile(QString dbPath)
{
	//获取软件所在电脑名称
	QString PCName = QHostInfo::localHostName();

	//获取要设置的db名称
	QString dbName = dbPath;

	if (ui.lineEdit->text().isEmpty())
	{
		QMessageBox::information(NULL, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请选择工程！"));
	}

	QString strpath = ui.lineEdit->text() + "/dbSetting.txt";
	QFile file(strpath);
	//如果没有那么创建
	if (!file.isOpen())
	{
		file.open(QIODevice::Append);
		file.close();
	}

	//写入参数
	QSettings settings(strpath, QSettings::IniFormat);
	settings.setIniCodec(QTextCodec::codecForName("GBK"));

	QString strKey = QString("Project/DB_%1").arg(PCName);

	//记录参数到配置文件中
	char buffer[256];

	sprintf(buffer, strKey.toLocal8Bit().data());
	settings.setValue(buffer, dbName);
}

void hnDBManagerDlg::mergeDb()
{
	//源db
	QString strSource = ui.lineEdit_source_2->text();

	QString strTarget = ui.lineEdit_dest_2->text() ;

	if (strSource == strTarget)
	{
		QMessageBox::information(NULL, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("不允许合并同一个db文件！"));
		return;
	} 
	
	
	auto curProject = hnDataManager::getDataManager()->getCurrentProject();
	auto diseaseTableNames = curProject->getCurDB()->m_diseaseTable.GetAllDiseaseTableNames();

	string str = strTarget.toLocal8Bit();
	hnDBSqlite * targetDbSqlite = new hnDBSqlite(str.c_str());

	 
	if ( !targetDbSqlite->connectDB(diseaseTableNames))
	{
		
		delete targetDbSqlite;
		targetDbSqlite = NULL;
		QMessageBox::information(NULL, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("操作失败\n"));

		return;
	}
	else
	{ 
		//获取当前数据库的所有病害 
		QVector<	hnCommon::hnRoadDiseaseInfo> newDiss;
		curProject->getDB()->m_diseaseTable.readAllDiseases(curProject->getCurProSetInfo(), newDiss,curProject->getCurrentMarkVector());

		//获取目标数据库的所有病害
		QVector<	hnCommon::hnRoadDiseaseInfo> oldDiss;
		targetDbSqlite->m_diseaseTable.readAllDiseases(curProject->getCurProSetInfo(), oldDiss, curProject->getCurrentMarkVector());
		 
		//写入到目标数据库
		targetDbSqlite->m_diseaseTable.mergeDatas(curProject->getCurProSetInfo(), newDiss,oldDiss);
		delete targetDbSqlite;
		targetDbSqlite = NULL;
	} 
	if (hnDataManager::getDataManager()->isOpenProject())
	{
		ui.comboBox_db->setCurrentText(strTarget);
		settingDbFile(strTarget);
		
		QMessageBox::information(NULL, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("已自动将当前db设置为[成果.db]\n请重新导入工程！"));
		return;
	}
	else
	{
		QMessageBox::information(NULL, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("未检测到任何已打开的工程！"));
		return;
	}

}

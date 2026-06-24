#include "hnAddRoadModel.h"
#include <QInputDialog>
#include <QDebug>
#include <QTreeWidgetItem>
#include <QStringList>
#include <QMessageBox>
#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include <QMap>
#include <vector>
#include <QFileDialog>
hnAddRoadModel::hnAddRoadModel(QWidget *parent): QMainWindow(parent),modelName(""),act1(nullptr),act2(nullptr),tree_menu(nullptr),allStacked(nullptr) 
{
    ui.setupUi(this);
	setConnect();
	init();

}

hnAddRoadModel::~hnAddRoadModel()
{
	clear();
}

void hnAddRoadModel::setConnect()
{
	connect(ui.actionadd, &QAction::triggered, this, &hnAddRoadModel::resetSlot);
	connect(ui.actionchangeName, &QAction::triggered, this, &hnAddRoadModel::changeNameSlot);
	connect(ui.actionwrite, &QAction::triggered, this, &hnAddRoadModel::writeDatatosql);
	connect(this, &hnAddRoadModel::nameChangsign, this, &hnAddRoadModel::setName);
	//connect(ui.treeWidget, &QTreeWidget::customContextMenuRequested, this, &hnAddRoadModel::menuPopupSlot);
	//connect(ui.treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(onItenClicked(QTreeWidgetItem*, int)));
	connect(ui.treeWidget, SIGNAL(itemPressed(QTreeWidgetItem*, int)), this, SLOT(onItenClicked(QTreeWidgetItem*, int)));
	connect(ui.action_open, &QAction::triggered, this, &hnAddRoadModel::readDatabase);
	
}

void hnAddRoadModel::init()
{
	
	
	clear();
	for (int i =ui.stackedWidget->count();i>=0;i--)
	{
		QWidget * widget = ui.stackedWidget->widget(i);
		ui.stackedWidget->removeWidget(widget);
		delete widget;
	}
	 modelName = "";
	 vectorDis.clear();
	 vectorRoad.clear();
	 vecDisDataMap.clear();
	 vecRoadDataMap.clear();
	 vecRoadData.clear();
	 vecDisData.clear();
	 strHeadList.clear();
	 fatherTreeItem.clear();
	 ui.treeWidget->clear();
	 allStacked = new QMap<QString, QWidget*>();
//	 roadPros = new QMap<QString, QVector<QtVariantProperty*>>();
	// disPros = new QMap<QString, QVector<QtVariantProperty*>>();
	 inputdialog = new hnItemInputDialog(this);
	 inputRoadDialog = new hnRoadItemInputDialog(this);
	 ui.treeWidget->setHeaderLabel(modelName);
	//QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
	//__qtreewidgetitem->setText(0, tr("请输入模块名称"));
	QHeaderView * head = ui.treeWidget->header();
	head->setSectionResizeMode(0, QHeaderView::Stretch);
	ui.treeWidget->setColumnCount(1);
	ui.treeWidget->setColumnWidth(1, 30);
	head->setStretchLastSection(false);
	strHeadList << QStringLiteral("病害配置") << QStringLiteral("道路配置");
	

	bhpzItem = new QTreeWidgetItem(ui.treeWidget);
	bhpzItem->setText(0, strHeadList[0]);
	bhpzItem->setData(0, Qt::UserRole, tr("%1").arg(0));
	bhpzItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));
	fatherTreeItem.push_back(bhpzItem);

	dlpzItem = new QTreeWidgetItem(ui.treeWidget);
	dlpzItem->setText(0, strHeadList[1]);
	dlpzItem->setData(0, Qt::UserRole, tr("%1").arg(1));
	dlpzItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));
	fatherTreeItem.push_back(dlpzItem);

	for (int i =2; i<strHeadList.length();++i)
	{
		QTreeWidgetItem * temp = new QTreeWidgetItem(ui.treeWidget);
		temp->setText(0, strHeadList[i]);
		temp->setData(0, Qt::UserRole, tr("%1").arg( i));
		temp->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));
		fatherTreeItem.push_back(temp);
	}
	ui.treeWidget->addTopLevelItems(fatherTreeItem);
	ui.treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	act1 = new MyAction(QStringLiteral("添加(&a)"), this);
	 	act2 = new MyAction(QStringLiteral("删除(&c)"), this);
	tree_menu = new QMenu(ui.treeWidget);
	void(MyAction::* p) (QTreeWidgetItem *) = &MyAction::myTriggered;
	void(hnAddRoadModel:: * p1) (QTreeWidgetItem *) = &hnAddRoadModel::addMenuClickedSlot;
	connect(act1, p, this, p1);
	void(MyAction::* p2) (QTreeWidgetItem *) = &MyAction::myTriggered;
	void(hnAddRoadModel:: * p3) (QTreeWidgetItem *) = &hnAddRoadModel::deletMenuClickedSlot;
	connect(act2, p2, this, p3);
	//connect(act1, SIGNAL(myTriggered(QString &str)), this, SLOT(addMenuClickedSlot(QString &str)));
	//connect(act2, SIGNAL(myTriggered(QString &str)), this, SLOT(deletMenuClickedSlot(QString &str)));
	tree_menu->addAction(act1);
	tree_menu->addAction(act2);
	ui.treeWidget->expandAll();
	
}


void hnAddRoadModel::resetSlot()
{
	init();
}

void hnAddRoadModel::changeNameSlot()
{
	bool ok;
//	QInputDialog * dialog = new QInputDialog();
	QString str = QInputDialog::getText(this, QStringLiteral("设置道路模板名称"), QStringLiteral("请输入名称:"), QLineEdit::Normal, modelName, &ok);
	//dialog->exec();
	if (ok)
	{
		this->modelName = str;
		QString str = tr("dis\\") + modelName + ".db";
		string strDB = str.toLocal8Bit();

		// 
		roadInfDB = new hnDBSqliteRoadInfo(strDB.c_str());
		if (!roadInfDB->connectDB())
		{
			delete roadInfDB;
			roadInfDB = NULL;
			return;
		}
		roadNum = roadInfDB->m_roadTypeSetTable.getMaxID()-1;
		disNum = roadInfDB->m_diseaseSetTable.getMaxID()-1;
		
		emit nameChangsign();
	}
}

void hnAddRoadModel::setName()
{

	
	ui.treeWidget->setHeaderLabel(modelName);
}



void hnAddRoadModel::onItenClicked(QTreeWidgetItem * item, int column)
{
	if (qApp->mouseButtons() == Qt::RightButton)
	{
		act1->setStr(item);
		act2->setStr(item);
		QString s;
	   char s1= *s.toStdString().c_str();
		tree_menu->exec(ui.treeWidget->mapToGlobal(ui.treeWidget->pos()));
	}
	else
	{	
		int count = ui.stackedWidget->count();
		int index = item->data(0, Qt::UserRole + 1).toInt();
		if (item->data(0,Qt::UserRole).toString().length()>=2)

		{
			ui.stackedWidget->setCurrentIndex(index);
		

			ui.statusBar->showMessage(QStringLiteral("堆栈界面总数:%2		当前堆栈界面索引:%1").arg(index).arg(count));
		}
		else
		{
			ui.statusBar->showMessage(QStringLiteral("堆栈界面总数:%1 当前处于根节点").arg	(count));
		}
		
	}
	
}





//删除节点发生的事件

void hnAddRoadModel::deletMenuClickedSlot(QTreeWidgetItem * item)
{
	QString sign = item->data(0, Qt::UserRole).toString();
	bool isDisRoot = item->data(0, Qt::UserRole).toString() == "0" ? true : false;
	int index = item->data(0, Qt::UserRole + 1).toInt();
	if (sign.length()>=2)
	{
		removeItem(item);
		
		auto iter = allStacked->find(sign);
		allStacked->erase(iter);
		ui.stackedWidget->removeWidget(ui.stackedWidget->widget(index));
		delete ui.stackedWidget->widget(index); 
		if (vecDisDataMap.contains(sign))
		{  
			auto& vec = vecDisDataMap[sign];
			for (auto it = vec.begin(); it!= vec.end();)
			{
				QtVariantProperty * temp = *it;
				delete temp;
				it = vec.erase(it);
			}


			vecDisDataMap.remove(sign);
		}
		if (vecRoadDataMap.contains(sign))
		{
			for (QtVariantProperty* temp : vecRoadDataMap.find(sign).value())
			{
				delete temp;
			}

			vecRoadDataMap.remove(sign);
		}


		ui.statusBar->showMessage(QStringLiteral("删除成功"));
	}
	else
	{
		ui.statusBar->showMessage(QStringLiteral("删除失败"));
	}
}



void hnAddRoadModel::writeDatatosql()
{
	vector<hnDiseaseSetInfo>tempDiss;
	vector<hnRoadTypeSetInfo>tempRoads;
	QMap<QString, vector<QtVariantProperty*>>::const_iterator iter = vecDisDataMap.begin();
	while (iter != vecDisDataMap.end())
	{
		hnDiseaseSetInfo tempDis;
		for (QtVariantProperty* item : *iter)
		{
			if (item->propertyName().contains("nID"))
			{
				tempDis.nID = item->value().toInt();continue;
			}
			if (item->propertyName().contains("strDisFullName"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempDis.strDisFullName, "%s", s.c_str()); continue;
			
			}
			if (item->propertyName().contains("nDiseaseIndex"))
			{
				tempDis.nDiseaseIndex = item->value().toInt(); continue;
			}
			if (item->propertyName().contains("strDiseaseName"))
			{

				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempDis.strDiseaseName, "%s", s.c_str()); continue;

			}

			if (item->propertyName().contains("strDiseaseTypeName"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempDis.strDiseaseTypeName, "%s", s.c_str()); continue;
			}

			if (item->propertyName().contains("nDiseaseType"))
			{
				tempDis.nDiseaseType = item->value().toInt(); continue;
			}

			if (item->propertyName().contains("strRoadType"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempDis.strRoadType, "%s", s.c_str()); continue;
			}

			if (item->propertyName().contains("nRoadSurfaceType"))
			{
				tempDis.nRoadSurfaceType = item->value().toInt(); continue;
			}

			if (item->propertyName().contains("nDrawType"))
			{
				tempDis.nDrawType = item->value().toInt(); continue;
			}


			if (item->propertyName().contains("strDBTableName"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempDis.strDBTableName, "%s", s.c_str()); continue;
			}
			if (item->propertyName().contains("nLevel"))
			{
				tempDis.nLevel = item->value().toInt(); continue;
			}
			if (item->propertyName().contains("nShowState"))
			{
				tempDis.nShowState = item->value().toInt(); continue;
			}

			if (item->propertyName().contains("fWidget"))
			{
				tempDis.fWidget = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("fEffectType"))
			{
				tempDis.fEffectType = item->value().toInt(); continue;
			}
			if (item->propertyName().contains("fEffectWid"))
			{
				tempDis.fEffectWid = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("fValidLen"))
			{
				tempDis.fValidLen = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("fValidArea"))
			{
				tempDis.fValidArea = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("nAreaFormula"))
			{
				tempDis.nAreaFormula = item->value().toInt(); continue;
			}
			if (item->propertyName().contains("nShortcutKey"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempDis.nShortcutKey, "%s", s.c_str()); continue;
			}
			if (item->propertyName().contains("nDWKF"))
			{
				tempDis.nDWKF = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dEffectMeasure"))
			{
				tempDis.dEffectMeasure = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("strSHMD"))
			{

				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempDis.strSHMD, "%s", s.c_str()); continue;
			}
			if (item->propertyName().contains("strDXKF"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempDis.strDXKF, "%s", s.c_str()); continue;
			}
			if (item->propertyName().contains("strDescribe"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempDis.strDescribe, "%s", s.c_str()); continue;
			}
			if (item->propertyName().contains("strAddFile1"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempDis.strAddFile1, "%s", s.c_str()); continue;
			}
			if (item->propertyName().contains("strRemark"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempDis.strRemark, "%s", s.c_str()); continue;
			}
		}
		tempDiss.push_back(tempDis);
		iter++;
	}
	
	QMap<QString, vector<QtVariantProperty*>>::const_iterator iter1 = vecRoadDataMap.begin();
	//while (iter1 != vecRoadDataMap.end())
	//{
	//	hnRoadTypeSetInfo tempRoad;
	//	for (QtVariantProperty* item : *iter1)
	//	{
	//		if (item->propertyName().contains("strRoadFullName"))
	//		{
	//			std::string  s = item->value().toString().toLocal8Bit();
	//			/*if ()
	//			{

	//			}*/
	//		}
	//	}
	//}
	

	while (iter1 != vecRoadDataMap.end())
	{
		hnRoadTypeSetInfo tempRoad;
		for (QtVariantProperty* item : *iter1)
		{
			if (item->propertyName().contains("ID"))
			{
				tempRoad.nID = item->value().toInt();
				continue;
			}
		
			if (item->propertyName().contains("strRoadType"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strRoadType, "%s", s.c_str());
				continue;
			}
			if (item->propertyName().contains("nRSurfaceType"))
			{
				tempRoad.nRSurfaceType = item->value().toInt();
				continue;
			}


			if (item->propertyName().contains("nDrawType"))
			{
				tempRoad.nDrawType = item->value().toInt();
				continue;
			}

			if (item->propertyName().contains("nRoadLevel"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.nRoadLevel, "%s", s.c_str());
				continue;
			}
			
			if (item->propertyName().contains("strSN_wi"))
			{
				sprintf_s(tempRoad.strSN_wi, "%.3lf", item->value().toDouble());
				continue;
			}

			if (item->propertyName().contains("strLQ_wi"))
			{
				sprintf_s(tempRoad.strLQ_wi, "%.3lf", item->value().toDouble());
				continue;
			}
		

			if (item->propertyName().contains("dRutThreslodUp"))
			{
				tempRoad.dRutThreslodUp = item->value().toFloat();
				continue;
			}
			if (item->propertyName().contains("dRutThreslodDown"))
			{
				tempRoad.dRutThreslodDown = item->value().toFloat();
				continue;
			}
			if (item->propertyName().contains("nRutIndex"))
			{
				tempRoad.nRutIndex = item->value().toInt();
				continue;
			}
			if (item->propertyName().contains("strRealV"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strRealV, "%s", s.c_str());
				continue;
			}

			if (item->propertyName().contains("strAmendPara"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strAmendPara, "%s", s.c_str());
				continue;
			}	
			if (item->propertyName().contains("strRoadFullName"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strRoadFullName, "%s", s.c_str());
				continue;
			}
			
			if (item->propertyName().contains("strRQILevel"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strRQILevel, "%s", s.c_str());
				continue;
			}
			if (item->propertyName().contains("strRDILevel"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strRDILevel, "%s", s.c_str());
				continue;
			}
			if (item->propertyName().contains("strPWILevel"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strPWILevel, "%s", s.c_str());
				continue;
			}
			if (item->propertyName().contains("strMTDLevel"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strMTDLevel, "%s", s.c_str());
				continue;
			}
			if (item->propertyName().contains("strIRILevel"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strIRILevel, "%s", s.c_str());
				continue;
			}
			if (item->propertyName().contains("strPCILevel"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strPCILevel, "%s", s.c_str());
				continue;
			}
			if (item->propertyName().contains("strPQILevel"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strPQILevel, "%s", s.c_str());
				continue;
			}
			if (item->propertyName().contains("strPBILevel"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strPBILevel, "%s", s.c_str());
				continue;
			}
			if (item->propertyName().contains("strMQILevel"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strMQILevel, "%s", s.c_str());
				continue;
			}
			if (item->propertyName().contains("dRQI_a0"))
			{
				tempRoad.dRQI_a0 = item->value().toFloat();
				continue;
			}
			if (item->propertyName().contains("dRQI_a1"))
			{
				tempRoad.dRQI_a1 = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dRQI_w1"))
			{
				tempRoad.dRQI_w1 = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dRQI_w2"))
			{
				tempRoad.dRQI_w2 = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dPCI_a0"))
			{
				tempRoad.dPCI_a0 = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dPCI_a1"))
			{
				tempRoad.dPCI_a1 = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dPQI_WPCI"))
			{
				tempRoad.dPQI_WPCI = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dPQI_WRQI"))
			{
				tempRoad.dPQI_WRQI = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dPQI_WRDI"))
			{
				tempRoad.dPQI_WRDI = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dPQI_WPBI"))
			{
				tempRoad.dPQI_WPBI = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dPQI_WPWI"))
			{
				tempRoad.dPQI_WPWI = item->value().toFloat(); continue;
			}
			if (item->propertyName() == "dRDI_a")
			{
				tempRoad.dRDI_a = item->value().toFloat(); continue;
			}
			if (item->propertyName()=="dRDI_b")
			{
				tempRoad.dRDI_b = item->value().toFloat(); continue;
			}

			if (item->propertyName().contains("dRDI_RDa"))
			{
				tempRoad.dRDI_RDa = item->value().toFloat(); continue;
			}if (item->propertyName().contains("dRDI_RDb"))
			{
				tempRoad.dRDI_RDb = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dRDI_a0"))
			{
				tempRoad.dRDI_a0 = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dRDI_a1"))
			{
				tempRoad.dRDI_a1 = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dPWI_a0"))
			{
				tempRoad.dPWI_a0 = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dPWI_a1"))
			{
				tempRoad.dPWI_a1 = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dMQI_WSCI"))
			{
				tempRoad.dMQI_WSCI = item->value().toFloat(); continue;
			}
			
			if (item->propertyName().contains("dMQI_WPQI"))
			{
				tempRoad.dMQI_WPQI = item->value().toFloat(); continue;
			}
			if (item->propertyName().contains("dMQI_WBCI"))
			{
				tempRoad.dMQI_WBCI = item->value().toFloat(); continue;
			}if (item->propertyName().contains("dMQI_WTCI"))
			{
				tempRoad.dMQI_WTCI = item->value().toFloat(); continue;
			}

			if (item->propertyName().contains("strPBI_KFBZ"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strPBI_KFBZ, "%s", s.c_str()); continue;
			}
			if (item->propertyName().contains("strPBI_KF"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strPBI_KF, "%s", s.c_str()); continue;
			}
			if (item->propertyName().contains("strAddFile1"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strAddFile1, "%s", s.c_str()); continue;
			}
			if (item->propertyName().contains("strRemark"))
			{
				std::string  s = item->value().toString().toLocal8Bit();
				sprintf_s(tempRoad.strRemark, "%s", s.c_str()); continue;
			}
		}
		tempRoads.push_back(tempRoad);
		
		iter1++;
	}
	if (tempDiss.size()>0&&tempRoads.size()>0)
	{
		if (!roadInfDB->connectDB())
		{
			delete roadInfDB;
			roadInfDB = NULL;
			return;
		}
		for (hnDiseaseSetInfo dis : tempDiss)
		{

			if (!roadInfDB->m_diseaseSetTable.writeData(dis))
			{
				qDebug() << QStringLiteral("病害写入出错") << endl;
			}

		}
		for (hnRoadTypeSetInfo road : tempRoads)
		{

			if (!roadInfDB->m_roadTypeSetTable.writeData(road))
			{
				qDebug() << QStringLiteral("路面写入出错" )<< endl;
			}
		}
	}
	else
	{
		
	}
	
}
//paixu
bool cmp(hnDiseaseSetInfo p1, hnDiseaseSetInfo p2)
{
	return p1.nID < p2.nID ? true : false;
}
bool cmp1(hnRoadTypeSetInfo p1, hnRoadTypeSetInfo p2)
{
	return p1.nID < p2.nID ? true : false;
}


void hnAddRoadModel::readDatabase()
{
	vector<hnDiseaseSetInfo>tempDiss;
	vector<hnRoadTypeSetInfo>tempRoads;

	//QFileDialog::getOpenFileName(this,"请选择db文件",)
	QString str = QFileDialog::getOpenFileName(this, QStringLiteral("打开数据库"), "dis\\", QStringLiteral("数据库文件(*db)"));
	
	string pathDB = str.toLocal8Bit();
	 QDir dirPath(str);
	 QString name =  dirPath.dirName();
	readRoadInfDB = new hnDBSqliteRoadInfo(pathDB.c_str());
	if (!readRoadInfDB->connectDB())
	{
		qDebug() << QStringLiteral("读取数据库打开失败");
		delete readRoadInfDB;
		readRoadInfDB = nullptr;
		return;
	}
	if (readRoadInfDB->m_diseaseSetTable.readData(tempDiss) )
	{
		if (readRoadInfDB->m_roadTypeSetTable.readData(tempRoads))
		{
			init();
			//设置模块名称
			sort(tempDiss.begin(), tempDiss.end(), cmp);
			sort(tempRoads.begin(), tempRoads.end(), cmp1);
			modelName = name.split('.').at(0);
			vector<hnDiseaseSetInfo>::iterator it = tempDiss.begin();
			while (it != tempDiss.end())
			{
				currentDis = *it;
				addTreeWidgetItem(bhpzItem, QString::fromLocal8Bit(it->strDisFullName), 0, true);
				it++;
			}
			vector<hnRoadTypeSetInfo>::iterator it1 = tempRoads.begin();
			while (it1 != tempRoads.end())
			{
				currentRoad = *it1;
				addTreeWidgetItem(dlpzItem, QString::fromLocal8Bit(it1->strRoadFullName), 1, true);
				it1++;
			}
			QString str = tr("dis\\") + name ;
			string strDB = str.toLocal8Bit();
			roadInfDB = new hnDBSqliteRoadInfo(strDB.c_str());
			if (!roadInfDB->connectDB())
			{
				delete roadInfDB;
				roadInfDB = NULL;
				return;
			}
		
			roadNum = roadInfDB->m_roadTypeSetTable.getMaxID()-1;
		    disNum = roadInfDB->m_diseaseSetTable.getMaxID()-1;
			
		}
	}
}

void hnAddRoadModel::addMenuClickedSlot(QTreeWidgetItem * item)
{
	//公共?
	bool isDisRoot = item->data(0, Qt::UserRole).toString() == "0" ? true : false;
	if (this->modelName == "")
	{
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("模块名称为空请点击左上角按钮设置"),
			QString::fromLocal8Bit("确定"));
		return;
	}
	if (isDisRoot)
	{
		inputdialog->clearDst();
		inputdialog->exec();
		QString itemName = inputdialog->getDis();
		addTreeWidgetItem(item, itemName,0); //0病害
	}
	else
	{
		
		inputRoadDialog->cleartxt();
		inputRoadDialog->exec();
		QString itemName = inputRoadDialog->getTxt();
		addTreeWidgetItem(item, itemName,1);
		
	}
		
}

void hnAddRoadModel::addTreeWidgetItem( QTreeWidgetItem* item, const QString& name,int sign,bool isRoad)
{
	
	//两个root节点的  data数据  病害的为0,道路为1

	
	if (sign==0)
	{
		if (allStacked != nullptr&&name != "")
		{
			if (allStacked->find(name) == allStacked->end()) // 
			{
				QtTreePropertyBrowser* w1 = creatMyWidget(sign, name, isRoad);
				w1->setStyleSheet("QtTreePropertyBrowser{ background-color: #5B677A;font - size:17px;color: white;}");
				int index = ui.stackedWidget->addWidget(w1);
				ui.stackedWidget->setCurrentIndex(index);

				QTreeWidgetItem * son = new QTreeWidgetItem();
				son->setText(0, name);
				son->setData(0, Qt::UserRole, tr("%1").arg(name));
				son->setData(0, Qt::UserRole + 1, index);
				item->addChild(son);
				allStacked->insert(name, w1);
				ui.statusBar->showMessage(QStringLiteral("界面生成成功"));
			}
			else
				ui.statusBar->showMessage(QStringLiteral("已存在！添加失败！"));
		}
		else
			ui.statusBar->showMessage(QStringLiteral("添加失败！"));
	}
	else
	{
		if (allStacked != nullptr&&name != "")
		{
			if (allStacked->find(name) == allStacked->end()) // 
			{
				QtTreePropertyBrowser* w1 = creatMyWidget(sign, name,isRoad);
				int index = ui.stackedWidget->addWidget(w1);
				ui.stackedWidget->setCurrentIndex(index);

				QTreeWidgetItem * son = new QTreeWidgetItem();
				son->setText(0, name);
				son->setData(0, Qt::UserRole, tr("%1").arg(name));
				son->setData(0, Qt::UserRole + 1, index);
				item->addChild(son);
				allStacked->insert(name, w1);
				ui.statusBar->showMessage(QStringLiteral("界面生成成功"));
			}
			else
				ui.statusBar->showMessage(QStringLiteral("已存在！添加失败！"));
		}
		else
			ui.statusBar->showMessage(QStringLiteral("添加失败！"));
	}
}
//1:生成属性赋值
//2：加载到树上\
//3:加到 vector里
void hnAddRoadModel::addUserProperty(QtTreePropertyBrowser*  bs,QtVariantPropertyManager* varManager,int value_type,const char* property_name, const QVariant& value_,bool isDis)
{
	
	QtVariantProperty *property;
	
	property = varManager->addProperty(value_type, tr(property_name));
	if (value_type == QVariant::Double)
	{
		varManager->setAttribute(property, QLatin1String("decimals"), 4);

	}
	else
	{
		varManager->setAttribute(property, QLatin1String("decimals"),1);

	}
	property->setValue(value_);
	
	bs->addProperty(property);
	if (isDis)
	{
		vectorDis.push_back(property);
	}
	else
	{
		
		vectorRoad.push_back(property);
	}
	
}


void hnAddRoadModel::removeItem(QTreeWidgetItem *item)
{
	int count = item->childCount();
	if (count == 0)
	{
		delete item;
		return;

	}
	for (int i = 0; i < count; ++i)
	{
		QTreeWidgetItem * childItem = item->child(0);
		removeItem(childItem);
	}
	delete item;
}
QtTreePropertyBrowser * hnAddRoadModel::creatMyWidget(int sign,QString itemName,bool isRead )
{
	//sign ==0 病害
	QStringList qslist = itemName.split('_');

	QString strDiseaseTypeName="";
	QString strRoadType = "";
	QString disName = "";
	int nDrawType = 0;
	int nRoadSurfaceType = 0;  // 病害所属路面材质
	QString strDBTableName = modelName; //病毒所属表名

	int nLevel;
	if (qslist.size()>1)
	{
		if (qslist.size() > 3)
		{
			strDiseaseTypeName = qslist.last() + "." + qslist.at(2);
			if (qslist.at(2) == QStringLiteral("轻"))
			{
				nLevel = 1;
			}
			else if (qslist.at(2) == QStringLiteral("中"))
			{
				nLevel = 2;
			}
			else if (qslist.at(2) == QStringLiteral("重"))
			{
				nLevel = 3;
			}
			else
			{
				strDiseaseTypeName = qslist.last();
				nLevel = 0;
			}
		}
		else
		{
			strDiseaseTypeName = qslist.last();
			nLevel = 0;
		}
		 strRoadType = qslist.at(1);
	     disName = qslist.last();
		strDBTableName = modelName; //病毒所属表名

		if (strRoadType == QStringLiteral("沥青"))
		{
			nRoadSurfaceType = 0;
		}
		else if (strRoadType == QStringLiteral("水泥"))
		{
			nRoadSurfaceType = 1;
		}
		else
		{
			//砂石
			nRoadSurfaceType = 2;

		}
		nDrawType = itemName.contains(QStringLiteral("人工模式")) ? 0 : 1;
	}
	if (sign==0)
	{
		
		
		
		//hnDiseaseSetInfo dis;
		QtTreePropertyBrowser* bs = new QtTreePropertyBrowser(this);
		//只读属性
		QtVariantPropertyManager * varManagerOnlyRead = new QtVariantPropertyManager(bs);
		
		QtVariantPropertyManager * varManager = new QtVariantPropertyManager(bs);
		if (itemName.contains(QStringLiteral("沿线设施")))
		{
			if (isRead)
			{
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nID", currentDis.nID, true);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDiseaseIndex", currentDis.nDiseaseIndex);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDisFullName", QString::fromLocal8Bit(currentDis.strDisFullName));
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDBTableName", QString::fromLocal8Bit(currentDis.strDBTableName));
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nLevel", currentDis.nLevel);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDiseaseType", 1);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDiseaseTypeName", QString::fromLocal8Bit(currentDis.strDiseaseTypeName));
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDiseaseName", QString::fromLocal8Bit(currentDis.strDiseaseName));
				addUserProperty(bs, varManager, QVariant::Double, "fWidget", currentDis.fWidget);
				addUserProperty(bs, varManager, QVariant::Int, "nShortcutKey",currentDis.nShortcutKey);
				addUserProperty(bs, varManager, QVariant::Double, "nDWKF", currentDis.nDWKF);
			
				addUserProperty(bs, varManager, QVariant::Double, "dEffectMeasure", currentDis.dEffectMeasure);
				addUserProperty(bs, varManager, QVariant::String, "strDescribe", QString::fromLocal8Bit(currentDis.strDescribe));
			}
			else
			{
				disNum++;
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nID", disNum, true);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDiseaseIndex", disNum);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDisFullName", itemName);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDBTableName", strDBTableName);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nLevel", nLevel);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDiseaseType", 1);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDiseaseTypeName", strDiseaseTypeName);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDiseaseName", disName);
				addUserProperty(bs, varManager, QVariant::Double, "fWidget", 0);
				addUserProperty(bs, varManager, QVariant::String, "nShortcutKey", "1");
				addUserProperty(bs, varManager, QVariant::Double, "nDWKF", 0);
				addUserProperty(bs, varManager, QVariant::Double, "dEffectMeasure", 0);
				addUserProperty(bs, varManager, QVariant::String, "strDescribe",QStringLiteral( "每10 m扣1分，不足10 m按10 m计"));
			}
			
		
		}
		else if (itemName.contains(QStringLiteral("路基损坏")))
		{
			if (isRead)
			{
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nID", currentDis.nID);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDiseaseIndex",currentDis.nDiseaseIndex);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDisFullName", QString::fromLocal8Bit(currentDis.strDisFullName));
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDBTableName", QString::fromLocal8Bit(currentDis.strDBTableName));
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nLevel",currentDis.nLevel);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDiseaseType", 2);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDiseaseTypeName", QString::fromLocal8Bit(currentDis.strDiseaseTypeName));
				addUserProperty(bs, varManager, QVariant::String, "strDiseaseName", QString::fromLocal8Bit(currentDis.strDiseaseName));
				addUserProperty(bs, varManager, QVariant::String, "nShortcutKey", currentDis.nShortcutKey);
				addUserProperty(bs, varManager, QVariant::Double, "nDWKF", currentDis.nDWKF);
				addUserProperty(bs, varManager, QVariant::Double, "fWidget",currentDis.fWidget);
				addUserProperty(bs, varManager, QVariant::Double, "dEffectMeasure", currentDis.dEffectMeasure);
				addUserProperty(bs, varManager, QVariant::String, "strDescribe", QString::fromLocal8Bit(currentDis.strDescribe));
			}
			else
			{
				disNum++;
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nID", disNum);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDiseaseIndex", disNum);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDisFullName", itemName);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDBTableName", strDBTableName);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nLevel", nLevel);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDiseaseType", 2);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDiseaseTypeName", strDiseaseTypeName);
				addUserProperty(bs, varManager, QVariant::String, "strDiseaseName", disName);
				addUserProperty(bs, varManager, QVariant::String, "nShortcutKey", "1");
				addUserProperty(bs, varManager, QVariant::Double, "nDWKF", 0);
				addUserProperty(bs, varManager, QVariant::Double, "fWidget", 0);
				addUserProperty(bs, varManager, QVariant::Double, "dEffectMeasure", 0);
				addUserProperty(bs, varManager, QVariant::String, "strDescribe", "每10 m扣1分，不足10 m按10 m计");
			}
			
		}
		else
		{
			if (isRead)
			{
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nID", currentDis.nID);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDiseaseIndex", currentDis.nDiseaseIndex);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDisFullName", QString::fromLocal8Bit(currentDis.strDisFullName));
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDiseaseTypeName", QString::fromLocal8Bit(currentDis.strDiseaseTypeName));
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDiseaseType", 0);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDiseaseName", QString::fromLocal8Bit(currentDis.strDiseaseName));
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strRoadType", QString::fromLocal8Bit(currentDis.strRoadType));
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nRoadSurfaceType",currentDis.nRoadSurfaceType);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDrawType", currentDis.nDrawType);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDBTableName", QString::fromLocal8Bit(currentDis.strDBTableName));
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nLevel", currentDis.nLevel);
				
				addUserProperty(bs, varManager, QVariant::Int, "nShowState", currentDis.nShowState);
				addUserProperty(bs, varManager, QVariant::Double, "fWidget", currentDis.fWidget);
				addUserProperty(bs, varManager, QVariant::String, "nShortcutKey", currentDis.nShortcutKey);
				addUserProperty(bs, varManager, QVariant::Int, "fEffectType", currentDis.fEffectType);
				addUserProperty(bs, varManager, QVariant::Double, "fEffectWid", currentDis.fEffectWid);
				addUserProperty(bs, varManager, QVariant::Double, "fValidLen", currentDis.fValidLen);

				addUserProperty(bs, varManager, QVariant::Double, "fValidArea", currentDis.fValidArea);

				addUserProperty(bs, varManager, QVariant::Int, "nAreaFormula", currentDis.nAreaFormula);

				addUserProperty(bs, varManager, QVariant::String, "strSHMD", QString::fromLocal8Bit(currentDis.strSHMD));
				addUserProperty(bs, varManager, QVariant::String, "strDXKF", QString::fromLocal8Bit(currentDis.strDXKF));
			}
			else
			{
				disNum++;
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nID", disNum);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDiseaseIndex", disNum);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDisFullName", itemName);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDiseaseTypeName", strDiseaseTypeName);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDiseaseType", 0);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDiseaseName", disName);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strRoadType", modelName);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nRoadSurfaceType", nRoadSurfaceType);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nDrawType", nDrawType);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strDBTableName", strDBTableName);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nLevel", nLevel);

				addUserProperty(bs, varManager, QVariant::Int, "nShowState", 1);
				addUserProperty(bs, varManager, QVariant::Double, "fWidget", 0);
				addUserProperty(bs, varManager, QVariant::String, "nShortcutKey", "1");
				addUserProperty(bs, varManager, QVariant::Int, "fEffectType", 0);
				addUserProperty(bs, varManager, QVariant::Double, "fEffectWid", 0);
				addUserProperty(bs, varManager, QVariant::Double, "fValidLen", 0);

				addUserProperty(bs, varManager, QVariant::Double, "fValidArea", 0);

				addUserProperty(bs, varManager, QVariant::Int, "nAreaFormula", 0);

				addUserProperty(bs, varManager, QVariant::String, "strSHMD", "0 0.01 0.1 1 10 50 100");
				addUserProperty(bs, varManager, QVariant::String, "strDXKF", "0 3 5 8 16 38 48");
			}
			
		}
		
		QtVariantEditorFactory *variantFactory = new QtVariantEditorFactory(this);
		bs->setFactoryForManager(varManager, variantFactory);
		vecDisDataMap.insert(itemName, vectorDis);
		vectorDis.clear();
		
		return bs;
		
	}
	else
	{
		
		QtTreePropertyBrowser* bs = new QtTreePropertyBrowser(this);
		//只读属性
		QtVariantPropertyManager * varManagerOnlyRead = new QtVariantPropertyManager(bs);
		QtVariantPropertyManager * varManager = new QtVariantPropertyManager();
		if (itemName == QStringLiteral("模块道路公共参数"))
		{	
			if (isRead)
			{
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "ID",currentRoad.nID, false);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strRoadFullName", QString::fromLocal8Bit(currentRoad.strRoadFullName),false);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strRoadType", QString::fromLocal8Bit(currentRoad.strRoadType), false);
				addUserProperty(bs, varManager, QVariant::Double, "dRDI_a", currentRoad.dRDI_a, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRDI_b", currentRoad.dRDI_b, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRDI_RDa", currentRoad.dRDI_RDa, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRDI_RDb", currentRoad.dRDI_RDb, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRDI_a0", currentRoad.dRDI_a0, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRDI_a1", currentRoad.dRDI_a1, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPWI_a0", currentRoad.dPWI_a0, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPWI_a1", currentRoad.dPWI_a1, false);
				addUserProperty(bs, varManager, QVariant::String, "strPBI_KFBZ", QString::fromLocal8Bit(currentRoad.strPBI_KFBZ), false);
				addUserProperty(bs, varManager, QVariant::String, "strPBI_KF", QString::fromLocal8Bit(currentRoad.strPBI_KF), false);
				addUserProperty(bs, varManager, QVariant::Double, "dMQI_WSCI", currentRoad.dMQI_WSCI, false);
				addUserProperty(bs, varManager, QVariant::Double, "dMQI_WPQI", currentRoad.dMQI_WPQI, false);
				addUserProperty(bs, varManager, QVariant::Double, "dMQI_WBCI", currentRoad.dMQI_WBCI, false);
				addUserProperty(bs, varManager, QVariant::Double, "dMQI_WTCI", currentRoad.dMQI_WTCI, false);
				addUserProperty(bs, varManager, QVariant::String, "dMQI_strMQILevel", QString::fromLocal8Bit(currentRoad.strMQILevel) , false);
				addUserProperty(bs, varManager, QVariant::Double, "dRQI_a0", currentRoad.dRQI_a0, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRQI_a1", currentRoad.dRQI_a1, false);
				addUserProperty(bs, varManager, QVariant::String, "strRealV", QString::fromLocal8Bit(currentRoad.strRealV), false);  //湖南农村路
				addUserProperty(bs, varManager, QVariant::String, "strAmendPara", QString::fromLocal8Bit(currentRoad.strAmendPara), false);
				addUserProperty(bs, varManager, QVariant::Double, "dRutThreslodUp", currentRoad.dRutThreslodUp, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRutThreslodDown", currentRoad.dRutThreslodDown, false);
				addUserProperty(bs, varManager, QVariant::Int, "nRutIndex", currentRoad.nRutIndex, false);
			}
			else
			{
				roadNum++;
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "ID", roadNum, false);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strRoadFullName", itemName,false);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strRoadType", modelName, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRDI_a", 100, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRDI_b", 90, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRDI_RDa", 10, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRDI_RDb", 40, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRDI_a0", 1.0, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRDI_a1", 3.0, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPWI_a0", 1.696, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPWI_a1", 0.785, false);
				addUserProperty(bs, varManager, QVariant::String, "strPBI_KFBZ", "20 50 80 1000000", false);
				addUserProperty(bs, varManager, QVariant::String, "strPBI_KF", "0 0 25 50", false);
				addUserProperty(bs, varManager, QVariant::Double, "dMQI_WSCI", 0.08, false);
				addUserProperty(bs, varManager, QVariant::Double, "dMQI_WPQI", 0.70, false);
				addUserProperty(bs, varManager, QVariant::Double, "dMQI_WBCI", 0.12, false);
				addUserProperty(bs, varManager, QVariant::Double, "dMQI_WTCI", 0.10, false);
				addUserProperty(bs, varManager, QVariant::String, "dMQI_strMQILevel", "90 80 70 60 0", false);
				addUserProperty(bs, varManager, QVariant::Double, "dRQI_a0", 4.98, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRQI_a1", -0.34, false);
				addUserProperty(bs, varManager, QVariant::String, "strRealV", "20 30 40 50 200", false);  //湖南农村路
				addUserProperty(bs, varManager, QVariant::String, "strAmendPara", "0.8 0.9 0.95 0.98 1", false);
				addUserProperty(bs, varManager, QVariant::Double, "dRutThreslodUp", 15, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRutThreslodDown", 10, false);
				addUserProperty(bs, varManager, QVariant::Int, "nRutIndex", 15, false);
				//QString temp =at(0)->propertyName();
				//qDebug() <<tem
			}
			
		}
		else
		{
			if (isRead)
			{
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "ID", currentRoad.nID, false);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nRSurfaceType", currentRoad.nRSurfaceType, false);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strRoadFullName", QString::fromLocal8Bit(currentRoad.strRoadFullName),false);
				addUserProperty(bs, varManager, QVariant::String, "nRoadLevel", QString::fromLocal8Bit(currentRoad.nRoadLevel), false);
				addUserProperty(bs, varManager, QVariant::String, "strRQILevel", QString::fromLocal8Bit(currentRoad.strRQILevel), false);
				addUserProperty(bs, varManager, QVariant::Double, "dPCI_a0", currentRoad.dPCI_a0, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPCI_a1", currentRoad.dPCI_a1, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRQI_w1", currentRoad.dRQI_w1, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRQI_w2", currentRoad.dRQI_w2, false);
				addUserProperty(bs, varManager, QVariant::String, "strRDILevel",QString::fromLocal8Bit(currentRoad.strRDILevel), false);
				addUserProperty(bs, varManager, QVariant::String, "strPCILevel",QString::fromLocal8Bit(currentRoad.strPCILevel), false);
				addUserProperty(bs, varManager, QVariant::String, "strPBILevel",QString::fromLocal8Bit(currentRoad.strPBILevel), false);
				addUserProperty(bs, varManager, QVariant::String, "strPWILevel",QString::fromLocal8Bit(currentRoad.strPWILevel), false);
				addUserProperty(bs, varManager, QVariant::String, "strPQILevel",QString::fromLocal8Bit(currentRoad.strPQILevel), false);

				addUserProperty(bs, varManager, QVariant::Double, "dPQI_WPCI", currentRoad.dPQI_WPCI, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPQI_WRQI", currentRoad.dPQI_WRQI, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPQI_WRDI", currentRoad.dPQI_WRDI, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPQI_WPBI", currentRoad.dPQI_WPBI, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPQI_WPWI", currentRoad.dPQI_WPWI, false);
			}
			else
			{
				roadNum++;
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "ID", roadNum, false);
				addUserProperty(bs, varManagerOnlyRead, QVariant::Int, "nRSurfaceType", nRoadSurfaceType, false);
				addUserProperty(bs, varManagerOnlyRead, QVariant::String, "strRoadFullName", itemName, false);
				addUserProperty(bs, varManager, QVariant::String, "nRoadLevel", qslist.last(), false);
				addUserProperty(bs, varManager, QVariant::String, "strRQILevel", "90 80 70 60 0", false);
				addUserProperty(bs, varManager, QVariant::Double, "dPCI_a0", 0, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPCI_a1", 0, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRQI_w1", 0.026, false);
				addUserProperty(bs, varManager, QVariant::Double, "dRQI_w2", 0.65, false);
				addUserProperty(bs, varManager, QVariant::String, "strRDILevel", "90 80 70 60 0", false);
				addUserProperty(bs, varManager, QVariant::String, "strPCILevel", "90 80 70 60 0", false);
				addUserProperty(bs, varManager, QVariant::String, "strPBILevel", "90 80 70 60 0", false);
				addUserProperty(bs, varManager, QVariant::String, "strPWILevel", "90 80 70 60 0", false);
				addUserProperty(bs, varManager, QVariant::String, "strPQILevel", "90 80 70 60 0", false);

				addUserProperty(bs, varManager, QVariant::Double, "dPQI_WPCI", 0.35, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPQI_WRQI", 0.30, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPQI_WRDI", 0.15, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPQI_WPBI", 0.10, false);
				addUserProperty(bs, varManager, QVariant::Double, "dPQI_WPWI", 0.10, false);
			}
			
		}
		QtVariantEditorFactory *variantFactory = new QtVariantEditorFactory(this);
		bs->setFactoryForManager(varManager, variantFactory);
		vecRoadDataMap.insert(itemName, vectorRoad);
		vectorRoad.clear();
		return bs;
	}
}




void hnAddRoadModel::clear()
{
	if (act1 != nullptr)
	{
		delete act1;
		act1 = nullptr;
	}
	if (act2 != nullptr)
	{
		delete act2;
		act2 = nullptr;
	}
	if (tree_menu != nullptr)
	{
		delete  tree_menu;
		tree_menu = nullptr;
	}
	if (allStacked != nullptr)
	{
		delete allStacked;
		allStacked = nullptr;
	}
}

MyAction::MyAction(const QString &text, QObject *parent /*= nullptr*/) :QAction(text, parent)
{
	clickItem = nullptr;	
	connect(this, &QAction::triggered, this, &MyAction::emitMyTriggeredSlot);
}



void MyAction::setStr(QTreeWidgetItem *it)
{
	clickItem = it;
}

QWidget* MyAction::addStacked(QString number)
{
	return new QWidget();
}

void MyAction::emitMyTriggeredSlot()
{
	emit myTriggered(clickItem);
}




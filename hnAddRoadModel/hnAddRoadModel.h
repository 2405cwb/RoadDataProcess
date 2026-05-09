#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_hnAddRoadModel.h"
#include <QVector>
#include "hnItemInputDialog.h"
#include "hnRoadItemInputDialog.h"
#include "qtpropertybrowser.h"
#include "..\hnCommon\hnRoadStruct.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include "..\hnDataTable\hnDataTable.h"
#include "..\hnDataTable\hnSQLite.h"
#include "..\hnDataTable\hnDBSqlite.h"
#include "..\hnDataTable\hnDBSqliteRoadInfo.h"
using namespace hnCommon;
class MyAction :public QAction
{
	Q_OBJECT
public:
	MyAction(const QString &text, QObject *parent = nullptr);
	void setStr(QTreeWidgetItem *);
	//number 为 点击单项后传出的值 ,[0]0 ,[0]1 可以判断new什么界面给用户
	QWidget* addStacked(QString number);
signals:
	void myTriggered(QTreeWidgetItem *);
	public slots:
	void emitMyTriggeredSlot();

private:
	QTreeWidgetItem *	clickItem;
	
};

class hnDBSqliteRoadInfo;
class hnAddRoadModel : public QMainWindow
{
	Q_OBJECT

public:
	hnAddRoadModel(QWidget *parent = Q_NULLPTR);
	~hnAddRoadModel();
	void init();
	
private:
	Ui::hnAddRoadModelClass ui;
	hnDBSqliteRoadInfo* roadInfDB;
	hnDBSqliteRoadInfo* readRoadInfDB;
	//用户定义的模块名称
	QString modelName;
	//右键菜单
	QMenu * tree_menu;
	MyAction *act1;
	MyAction *act2;
	//左边表根节点
	QTreeWidgetItem * bhpzItem;
	QTreeWidgetItem * dlpzItem;
	//根节点名称
	QStringList strHeadList;
	//左边树根节点，不允许添加
	QList<QTreeWidgetItem*> fatherTreeItem;
	//存储所有产生的堆栈界面.
	//key: 节点名称  value  窗口  //从而实现点击节点窗口切换
	QMap<QString, QWidget*>* allStacked;
	QTreeWidgetItem *__qtreewidgetitem; //左表头
	hnItemInputDialog * inputdialog;
	hnRoadItemInputDialog * inputRoadDialog;
	
	//设置槽函数
	void setConnect();
	//用户右键节点  点击添加按钮发送
	void addTreeWidgetItem( QTreeWidgetItem*, const QString& name, int,bool isRoad = false);
	//添加属性  主要函数

	unsigned int roadNum;
	unsigned int disNum;
	void addUserProperty( QtTreePropertyBrowser*, QtVariantPropertyManager* varManager, int value_type, const char* property_name, const QVariant& value_, bool isDis= true);
	//递归删除节点 及子节点
	void removeItem(QTreeWidgetItem *item);
	//释放内存
	void clear();
	
	class QtTreePropertyBrowser *propertyEditor;
	QtTreePropertyBrowser * creatMyWidget(int sign, QString itemName, bool isRead = false);

	//所需信息
	vector<QtVariantProperty*> vectorDis;
	vector<QtVariantProperty*> vectorRoad;
	QMap<QString, vector<QtVariantProperty*>>vecDisDataMap;
	QMap<QString, vector<QtVariantProperty*>>vecRoadDataMap;
    QMap<QString,vector<hnDiseaseSetInfo>>   vecDisData;
    QMap<QString,vector<hnRoadTypeSetInfo>> vecRoadData;
	//QMap<QString, QtVariantProperty *> idToProperty;
	//key的作用是当用户删除时候提供标识
	//QMap<QString,QVector<QtVariantProperty*>>*roadPros;
	//QMap<QString,QVector< QtVariantProperty*>>*disPros;

	//hnDBSqlite * dbsqlite;
public slots:
	void setName();
	void resetSlot();
	void changeNameSlot();
	void onItenClicked(QTreeWidgetItem *, int);
	//根据用户点击的右键菜单执行对应操作
	void addMenuClickedSlot(QTreeWidgetItem *);

	//记得删除界面元素后 清空对应的内存
	void deletMenuClickedSlot(QTreeWidgetItem *);
	//写入数据库按钮
	void writeDatatosql();
	void readDatabase();
private:
	hnDiseaseSetInfo currentDis;
	hnRoadTypeSetInfo currentRoad;
	
signals:
	void nameChangsign();
};

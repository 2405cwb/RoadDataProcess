#include "projectView.h"
#include "../hnQtCommon/MyCommonMethods.h"
#include "QTableWidgetItem"
#include "..\hnApplication\hnDataManager.h"
#include <functional>
#include <algorithm>
#include <QMenu>
#include <QResizeEvent>
projectView::projectView(QWidget *parent)
	: QWidget(parent)
{

	ui.setupUi(this);
	//设置打标窗口 行数0
	ui.markTableWidget->setRowCount(0);
	ui.markTableWidget->setColumnCount(3);
	ui.markTableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	//设置打标窗口表头
	ui.markTableWidget->setHorizontalHeaderLabels(QStringList() << QStringLiteral("打标桩号") << QStringLiteral("打标类型") << QStringLiteral("打标信息"));
	ui.markTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.markTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	ui.label_22->hide();
	ui.lineEdit_18->hide();
	ui.pushButton_7->hide();
	connect(ui.markTableWidget, &QTableWidget::customContextMenuRequested, this, &projectView::MarkMenuClicked);

	//设置较桩窗口
	ui.pileTableWidget->setRowCount(0);
	ui.pileTableWidget->setColumnCount(2);
	ui.pileTableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	ui.pileTableWidget->setHorizontalHeaderLabels(QStringList() << QStringLiteral("桩号") << QStringLiteral("里程"));
	ui.pileTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.pileTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.pileTableWidget, &QTableWidget::customContextMenuRequested, this, &projectView::MilePileMenuClicked);
	//当用户双击打标
	connect(ui.markTableWidget, &QTableWidget::itemDoubleClicked, this, &projectView::slot_doubleClickTableVidgetItem);
	//当用户双击较桩窗口条目发生
	connect(ui.pileTableWidget, &QTableWidget::itemDoubleClicked, this, &projectView::slot_doubleClickTableVidgetItem);
	//当打标窗口选择combox条目发生变化相应的控件产生变化
	connect(ui.comboBox_2, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_MarkComboxIndexChanged(int)));

	//重载按钮按下进行重载界面工作
	connect(ui.pushButton_4, &QPushButton::clicked, this, &projectView::slot_initProjectClicked);

	//添加打标按钮按下
	connect(ui.pushButton_5, &QPushButton::clicked, this, &projectView::slot_addMarkClicked);
	connect(ui.pushButton_7, &QPushButton::clicked, this, &projectView::slot_addMarkClicked);
	//添加较桩按钮按下
	connect(ui.pushButton_3, &QPushButton::clicked, this, &projectView::slot_addMilePileClicked);

	ui.lineEdit_16->setValidator(new QIntValidator(this));
	connect(ui.lineEdit_16, &QLineEdit::textChanged, this, &projectView::updateDmiTxt);
}

projectView::~projectView()
{

}

void projectView::slot_updateProjectSetting(hnCommon::hnProjectSetInfo setting, QVector<hnCommon::hnMarkInfo> marks, QVector<hnCommon::hnMilePile> pile)
{
#pragma region 加载工程信息
	//更新工程信息界面	
	//工程是否为上行
	bool isUp;
	//省
	ui.lineEdit->setText(QString::fromLocal8Bit(setting.strProvince));
	//市
	ui.lineEdit_5->setText(QString::fromLocal8Bit(setting.strCity));
	//县
	ui.lineEdit_2->setText(QString::fromLocal8Bit(setting.strCounty));
	//道路编号
	ui.lineEdit_6->setText(QString::fromLocal8Bit(setting.strRoadNO));
	//车道号
	ui.lineEdit_3->setText(QString::fromLocal8Bit(setting.strNumber));
	//道路名称
	ui.lineEdit_7->setText(QString::fromLocal8Bit(setting.strRoadName));


	//起始桩号
	ui.lineEdit_4->setText(MyCommonMethods::convertMileToString(setting.dBegMile));
	//上下行
	if (setting.nLineType == 1)
	{
		ui.comboBox->setCurrentIndex(0);
		isUp = true;
	}
	else
	{
		ui.comboBox->setCurrentIndex(1);
		isUp = false;
	}
	//采集日期
	QDate dateDay = QDate::fromString(setting.strDate, "yyyyMMdd");
	if (dateDay.isValid())
	{
		ui.dateEdit->setDate(dateDay);

	}

	QString time = QString::fromLocal8Bit(setting.strTimer);
	if (time.length() == 6)
	{
		int hour = time.mid(0, 2).toInt();
		int min = time.mid(2, 2).toInt();
		int second = time.mid(4, 2).toInt();
		QTime dateTime(hour, min, second);
		if (dateTime.isValid())
		{
			ui.timeEdit->setTime(dateTime);
		}
	}


	//采集时间
	//ui.timeEdit
	//公路等级
	ui.lineEdit_9->setText(QString::fromLocal8Bit(setting.strRoadLevel));
	if (setting.nRSurfaceType == 0)
	{
		ui.lineEdit_10->setText(QStringLiteral("沥青"));
	}
	else if (setting.nRSurfaceType == 1)
	{
		ui.lineEdit_10->setText(QStringLiteral("水泥"));

	}
	else
	{
		ui.lineEdit_10->setText(QStringLiteral("砂石"));
	}
	//检测员
	ui.lineEdit_11->setText(QString::fromLocal8Bit(setting.strSurveyor));
	//天气
	ui.lineEdit_13->setText(QString::fromLocal8Bit(setting.strWeather));
	//终点桩号
	ui.lineEdit_12->setText(MyCommonMethods::convertMileToString(setting.dEndMile));
	//长度
	ui.lineEdit_14->setText(QString::number(setting.dLength, 'f', 0) + QStringLiteral("米"));

	if (setting.dUserBegMile >=0 && setting.dUserEndMile >=0)
	{
		ui.lineEdit_19->setText(MyCommonMethods::convertMileToString(setting.dUserBegMile));
		ui.lineEdit_20->setText(MyCommonMethods::convertMileToString(setting.dUserEndMile)); 
	}
#pragma endregion

	 
	updateMarkFrom(marks,isUp);

	updatePileFrom(pile,isUp);

}

void projectView::slot_doubleClickTableVidgetItem(QTableWidgetItem * item)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}

	double mile = 0;
	int row = item->row();

	QTableWidgetItem * firstItem = item->tableWidget()->item(row, 0);

	if (firstItem)
	{
		QString firstColumnValue = firstItem->text();
		mile = firstColumnValue.toDouble();
	}
	emit signal_jumpToMile(mile);
}

void projectView::slot_MarkComboxIndexChanged(int index)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	hnMile currentMile = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentRoadMile();
 
    std::vector<QString> surfaceTyes = 	hnApp::hnDataManager::getDataManager()->getRoadSurfaceType();
	ui.comboBox_3->clear();
	ui.lineEdit_18->setEnabled(true);
	ui.comboBox_3->setEnabled(true);
	switch (index)
	{
	case 0:

		ui.label_22->hide();
		ui.lineEdit_18->hide();
		ui.comboBox_3->show();

		ui.pushButton_7->hide();
		ui.pushButton_5->show();

		//获取当前模块的可用打标类型
		
		ui.comboBox_3->clear();
		for (int i = 0 ; i < surfaceTyes.size(); ++i)
		{
			ui.comboBox_3->addItem(surfaceTyes[i]);
		}
		 
		
		ui.comboBox_3->setCurrentIndex(0);
		break;
	case 1:
		ui.comboBox_3->hide();
		ui.pushButton_7->show();
		ui.pushButton_5->hide();
		ui.label_22->show();
		ui.lineEdit_18->show();
		break;
	case 2:
		//等级
	{
	//	ui.lineEdit_18->setEnabled(false);

		ui.label_22->hide();
		ui.lineEdit_18->hide();
		ui.comboBox_3->show();
		ui.pushButton_7->hide();
		ui.pushButton_5->show();
		ui.comboBox_3->clear();
		QStringList tempStr;
		QVector<QString> levels = hnApp::hnDataManager::getDataManager()->getRoadLevel(currentMile.roadStandard);
		for (auto levelStr : levels)
		{
			tempStr << levelStr;
		}
		ui.comboBox_3->addItems(tempStr);
		ui.comboBox_3->setCurrentIndex(0); break;
	}

	case 3:
	{
		ui.label_22->hide();
		ui.lineEdit_18->hide();
		ui.comboBox_3->show();
		ui.pushButton_7->hide();
		ui.pushButton_5->show();
		QStringList tempStr;
		ui.comboBox_3->clear();
		tempStr << QStringLiteral("等级公路2018") << QStringLiteral("城镇道路") << QStringLiteral("低等级农村公路");
		ui.comboBox_3->addItems(tempStr);
		ui.comboBox_3->setCurrentIndex(0);
	}

	break;
	case 4:
		ui.comboBox_3->hide();
		ui.pushButton_7->show();
		ui.pushButton_5->hide();
		ui.label_22->show();
		ui.lineEdit_18->show();
	
	 

		break;
		break;
	default:
		break;
	}
}
void projectView::slot_updateMileAndDmi(double mile, double dmi)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	ui.lineEdit_16->setText(QString::number(mile, 'f', 0));
	ui.lineEdit_17->setText(QString::number(dmi, 'f', 0));
	ui.lineEdit_8->setText(QString::number(mile, 'f', 0));
	ui.lineEdit_15->setText(QString::number(dmi, 'f', 0));
}

void projectView::slot_initProjectClicked()
{

	emit signal_updateAllWidget();
}

void projectView::slot_addMarkClicked()
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() == PROJECT_JD_3D_TYPE)
	{
		return;
	}
	else if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() == PROJECT_TYPE::PROJECT_XD_3D_TYPE)
	{
		return;
	}
	bool isUp = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nLineType == 1 ? true : false;
	QVector<hnCommon::hnMarkInfo> marks = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMarkVector();
	hnCommon::hnMarkInfo  currentMarkInfo;

	//用户选择的打标内容 
	int markType = 0;
	markType = ui.comboBox_2->currentIndex();

	QString text = "";
	if (!ui.lineEdit_18->isHidden())
	{
		text = ui.lineEdit_18->text();
	}
	if (!ui.comboBox_3->isHidden())
	{
		text = ui.comboBox_3->currentText();
	}
	double curMile = ui.lineEdit_16->text().toDouble();
	double dmi = hnApp::hnDataManager::getDataManager()->getCurrentProject()->trueMileToEncl(curMile);
	currentMarkInfo.dTrueMile = curMile;
	currentMarkInfo.nType = markType;
	currentMarkInfo.dGpsTimer = -1;
	std::string s1 = text.toLocal8Bit().toStdString();
	strcpy(currentMarkInfo.strMark, s1.c_str());
	QApplication::setOverrideCursor(Qt::WaitCursor);
	if (std::find(marks.begin(), marks.end(), currentMarkInfo) == marks.end())
	{


		marks.push_back(currentMarkInfo);
		updateMarkFrom(marks, isUp);

		if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->addMark(currentMarkInfo))
		{
			emit signal_updateAllWidget(); 
		}
	}
	QApplication::restoreOverrideCursor();
}

void projectView::slot_addMilePileClicked()
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() == PROJECT_JD_3D_TYPE)
	{
		return;
	}
	else if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() == PROJECT_TYPE::PROJECT_XD_3D_TYPE)
	{
		return;
	}
	bool isUp =  hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nLineType == 1 ? true : false;
	QVector<hnCommon::hnMilePile> datas = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMilePileVector();
	hnCommon::hnMilePile curMile; 
	curMile.dTrueMile = ui.lineEdit_8->text().toDouble();
	curMile.dEnclMile = ui.lineEdit_15->text().toDouble();
	QApplication::setOverrideCursor(Qt::WaitCursor);
	if (std::find(datas.begin(),datas.end(), curMile) == datas.end())
	{
	 
 		datas.push_back(curMile); 
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->addMilePile(curMile);
		updatePileFrom(datas, isUp);
	}
	
	emit signal_updateAllWidget();
	QApplication::restoreOverrideCursor();
}

void projectView::MarkMenuClicked(const QPoint &pos)
{
	//获取点击位置单元格
	QTableWidgetItem *item = ui.markTableWidget->itemAt(pos);
	if (!item)
	{
		return;
	}

	QMenu menu(ui.markTableWidget);
	QAction * deleteAction = menu.addAction(QStringLiteral("删除"));

	connect(deleteAction, &QAction::triggered, this, &projectView::deleteMark);

	//鼠标位置显示
	menu.exec(ui.markTableWidget->viewport()->mapToGlobal(pos));
}

void projectView::MilePileMenuClicked(const QPoint &pos)
{
	//获取点击位置单元格
	QTableWidgetItem *item = ui.pileTableWidget->itemAt(pos);
	if (!item)
	{
		return;
	}

	QMenu menu(ui.markTableWidget);
	QAction * deleteAction = menu.addAction(QStringLiteral("删除"));

	connect(deleteAction, &QAction::triggered, this, &projectView::deletePipe);

	//鼠标位置显示
	menu.exec(ui.markTableWidget->viewport()->mapToGlobal(pos));
}

void projectView::deleteMark()
{
	//获取选中的行并删除
	QSet<int>rowToRemove;
	QApplication::setOverrideCursor(Qt::WaitCursor);
	for (QTableWidgetItem * item :ui.markTableWidget->selectedItems())
	{
		int row = item->row();
		QTableWidgetItem * firstItem = ui.markTableWidget->item(row, 0);
		int rowId = firstItem->data(Qt::UserRole).toInt();
		rowToRemove.insert(item->row());
		if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->deleteMark(rowId))
		{
			emit signal_updateAllWidget();
		}
	}
	QApplication::restoreOverrideCursor();
	//从高到低删除避免索引错乱
	QList<int> sortedRows = rowToRemove.toList();
	std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());
	for (int row:sortedRows)
	{
		ui.markTableWidget->removeRow(row);
	} 
	bool isUp = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nLineType == 1 ? true : false;
	QVector<hnCommon::hnMarkInfo> marks = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMarkVector();
	updateMarkFrom(marks, isUp);
}

void projectView::deletePipe()
{
	//获取选中的行并删除
	QSet<int>rowToRemove;
	QApplication::setOverrideCursor(Qt::WaitCursor);
	for (QTableWidgetItem * item : ui.pileTableWidget->selectedItems())
	{
		int row = item->row();
		QTableWidgetItem * firstItem = ui.pileTableWidget->item(row, 0);
		int rowId = firstItem->data(Qt::UserRole).toInt();
		rowToRemove.insert(item->row());
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->deleteMilePile(rowId);
		emit signal_updateAllWidget();
	 
	}
	QApplication::restoreOverrideCursor();
	//从高到低删除避免索引错乱
	QList<int> sortedRows = rowToRemove.toList();
	std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());
	for (int row : sortedRows)
	{
		ui.pileTableWidget->removeRow(row);
	}
}

 

void projectView::updateDmiTxt(const QString &text)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	double mile =  text.toDouble();
	int  dmi = qRound( hnApp::hnDataManager::getDataManager()->getCurrentProject()->trueMileToEncl(mile));
	ui.lineEdit_17->setText(QString::number(dmi));
}

void projectView::updateMarkFrom( QVector<hnCommon::hnMarkInfo>& marks,bool isUp)
{
	std::vector<QString> surfaceTyes = hnApp::hnDataManager::getDataManager()->getRoadSurfaceType();
	ui.comboBox_3->clear();
	for (int i = 0; i < surfaceTyes.size(); ++i)
	{
		ui.comboBox_3->addItem(surfaceTyes[i]);
	}


	ui.comboBox_3->setCurrentIndex(0);


	if (isUp)
	{
		//对marks进行排序
		std::sort(marks.begin(), marks.end());
	}
	else
	{
		//对marks进行排序
		std::sort(marks.begin(), marks.end(), compareDeScendingMark);
	}


	ui.markTableWidget->clearContents();
	ui.markTableWidget->setRowCount(0);
	int rowIndex1 = ui.markTableWidget->rowCount();
	if (marks.size() > 0)
	{
		for (int i = 0; i < marks.size(); ++i)
		{
			QStringList row;
			auto mark = marks.at(i);

			QString strTest = QString::number(mark.dTrueMile, 'f', 2);
			row << strTest;

			QString type = mark.nType == 0 ? QStringLiteral("路面材质")
				: mark.nType == 1 ? QStringLiteral("路面单元")
				: mark.nType == 2 ? QStringLiteral("路面等级")
				: mark.nType == 3 ? QStringLiteral("路面标准") : mark.nType == 4 ? QStringLiteral("路面情况") : QStringLiteral("错误");
			row << type;


			row << QString::fromLocal8Bit(mark.strMark);
			//

			int rowIndex = ui.markTableWidget->rowCount();
			ui.markTableWidget->insertRow(rowIndex);
			QTableWidgetItem* item0 = new QTableWidgetItem(row[0]);
			item0->setFlags(item0->flags() & ~Qt::ItemIsEditable);
			item0->setData(Qt::UserRole, mark.nID);
			QTableWidgetItem* item1 = new QTableWidgetItem(row[1]);
			item1->setFlags(item0->flags() & ~Qt::ItemIsEditable);
			QTableWidgetItem* item2 = new QTableWidgetItem(row[2]);
			item2->setFlags(item0->flags() & ~Qt::ItemIsEditable);
			ui.markTableWidget->setItem(rowIndex, 0, item0);
			ui.markTableWidget->setItem(rowIndex, 1, item1);
			ui.markTableWidget->setItem(rowIndex, 2, item2);


		}


	}
}

void projectView::updatePileFrom( QVector<hnCommon::hnMilePile>& pile,bool isUp)
{ 
	if (isUp)
	{
		std::sort(pile.begin(), pile.end());
	}
	else
	{
	std::sort(pile.begin(), pile.end(),compareDeScendingPile);
	}
	
	ui.pileTableWidget->clearContents();
	ui.pileTableWidget->setRowCount(0);
	int	rowIndex1 = ui.pileTableWidget->rowCount();
	if (pile.size() > 0)
	{
		for (int i = 1; i < pile.size()-1; ++i)
		{
			QStringList row;
			auto curPile = pile.at(i);
			QString mileStr = QString::number(curPile.dTrueMile, 'f', 2);
			row << mileStr;

			QString strTest = QString::number(curPile.dEnclMile, 'f', 2);
			row << strTest;
			int rowIndex = ui.pileTableWidget->rowCount();
			ui.pileTableWidget->insertRow(rowIndex);
			QTableWidgetItem* item0 = new QTableWidgetItem(row[0]);
			item0->setData(Qt::UserRole, curPile.nID);
			item0->setFlags(item0->flags() & ~Qt::ItemIsEditable);
			QTableWidgetItem* item1 = new QTableWidgetItem(row[1]);
			item1->setFlags(item0->flags() & ~Qt::ItemIsEditable);
			ui.pileTableWidget->setItem(rowIndex, 0, item0);
			ui.pileTableWidget->setItem(rowIndex, 1, item1);
		}
	}
}

void projectView::resizeEvent(QResizeEvent *event)
{
	QSize oldSize = event->oldSize();
	QSize newSize = event->size();
	qDebug() <<  "[Console]	old Size" <<oldSize.width() << "x" << oldSize.height()
		<< "new Size" << newSize.width() << "x" << newSize.height()
		;
	QWidget::resizeEvent(event);
}

#include "hnMarkInfoDlg.h"
#include <QGridLayout>
#include <QLabel>
#include <QTableWidgetItem>
#include <QGroupBox>
#include <QSizePolicy>
#include <QTableWidget>
#include <QMenu>
#include <QLineEdit>
#include "..\hnProject\hnProject.h"
#include <QComboBox>
#include <QFileDialog>
#include <	QMessageBox>
#include <QHeaderView>
#include "xlsxdocument.h"
QXLSX_USE_NAMESPACE
hnMarkInfoDlg::hnMarkInfoDlg(const QVector<hnCommon::hnMarkInfo>&  marks, QWidget *parent /*= nullptr*/)
	: QDialog(parent), m_marks(marks)
{

	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	setWindowTitle(QStringLiteral("当前打标界面"));

	tableWidget = new QTableWidget(0, 3, this);
	tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	tableWidget->setHorizontalHeaderLabels(QStringList() << QStringLiteral("打标类型") << QStringLiteral("打标桩号") << QStringLiteral("打标信息"));
	mainLayout->addWidget(tableWidget);
	tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	btnBox = new QHBoxLayout(this);
	okPtn = new QPushButton(QStringLiteral("确定"));
	cancelPtn = new QPushButton(QStringLiteral("取消"));
	cleanAllPtn = new QPushButton(QStringLiteral("清空"));
	outExcelBtn = new QPushButton(QStringLiteral("输出报表"));
	btnBox->addWidget(okPtn);
	btnBox->addWidget(cancelPtn);
	btnBox->addWidget(cleanAllPtn);
	btnBox->addWidget(outExcelBtn);

	addPtn = new QPushButton(QStringLiteral("新增"));
	deletePtn = new QPushButton(QStringLiteral("删除"));
	
	for (int i = 0; i < m_marks.size(); ++i)
	{
		QStringList row;
		auto mark = marks.at(i);
		QString type = mark.nType == 0 ? QStringLiteral("路面材质")
			: mark.nType == 1 ? QStringLiteral("路面单元")
			: mark.nType == 2 ? QStringLiteral("路面等级")
			: mark.nType == 3 ? QStringLiteral("路面标准(暂不可用)") : mark.nType==4?QStringLiteral("路面情况"): QStringLiteral("错误");
		row << type;
		
		QString strTest = QString::number(mark.dTrueMile,'f',2);
		row << strTest;
		row << QString::fromLocal8Bit(mark.strMark);
		//
		int rowIndex = tableWidget->rowCount();
		tableWidget->insertRow(rowIndex);
		tableWidget->setItem(rowIndex, 0, new QTableWidgetItem(row[0]));
		tableWidget->setItem(rowIndex, 1, new  QTableWidgetItem(row[1]));
		tableWidget->setItem(rowIndex, 2, new QTableWidgetItem(row[2]));
	}
	mainLayout->addLayout(btnBox);

	this->setLayout(mainLayout);
	resize(420, 400);
	int columnCount = tableWidget->columnCount();
	int tolWidth = tableWidget->viewport()->width();
	int toltalWidth = this->width();
	for (int i = 0; i < columnCount; ++i)
	{
		//tableWidget->column
	}
	//tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	//connect(tableWidget, &QTableWidget::customContextMenuRequested, this, [=](const QPoint& pos)
	//{
	//	QMenu menu;
	//	QAction * addAction = menu.addAction(QStringLiteral("添加(a)"));
	//	addAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
	//	QAction * deleteAction = menu.addAction(QStringLiteral("删除(d)"));
	//	addAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
	//	QAction * selectdItem = menu.exec(tableWidget->mapToGlobal(pos));
	//	if (selectdItem == addAction)
	//	{
	//		m_addMarkFrom = new QDialog(this);
	//		QVBoxLayout * mainLy = new QVBoxLayout(m_addMarkFrom);
	//		m_addMarkFrom->setWindowTitle(QStringLiteral("添加打标"));
	//		QGridLayout * vLayout = new QGridLayout(m_addMarkFrom);
	//		QLabel * label = new QLabel(QStringLiteral("打标桩号"), m_addMarkFrom);
	//		QLabel * label0 = new QLabel(QStringLiteral("打标类型"), m_addMarkFrom);
	//		QLabel * label1 = new QLabel(QStringLiteral("打标内容"), m_addMarkFrom);
	//		QPushButton * okBtn1 = new QPushButton(QStringLiteral("确定"), m_addMarkFrom);
	//		QPushButton * cannelBtn1 = new QPushButton(QStringLiteral("取消"), m_addMarkFrom);
	//		mileBox = new QLineEdit(m_addMarkFrom);
	//		hnMile currentMile = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentRoadMile();
	//		mileBox->setText(QString::number(currentMile.dTrueMile,'f',2));
	//		typeCombox = new QComboBox(m_addMarkFrom);
	//		QStringList typeItems;
	//		typeItems << QStringLiteral("道路材质") << QStringLiteral("道路单元") << QStringLiteral("道路等级") << QStringLiteral("路面标准(暂不可用)")<<QStringLiteral("路面情况");
	//		int markType = -1;
	//		QString text = "";
	//		typeCombox->addItems(typeItems);
	//		typeCombox->setCurrentIndex(0);
	//		markType = 0;
	//		QVector<QString> types0 = hnApp::hnDataManager::getDataManager()->getRoadSurfaceTypes(currentMile.roadStandard);
	//		contextCombox = new QComboBox(m_addMarkFrom);
	//		for (QString cz : types0)
	//		{
	//			contextCombox->addItem(cz);
	//		}
	//		contextCombox->setCurrentIndex(0);
	//		text = types0.at(0);

	//		connect(typeCombox, QOverload<int>::of(&QComboBox::currentIndexChanged), m_addMarkFrom, [=](int index)
	//		{
	//			QLayoutItem * item = vLayout->itemAtPosition(1, 2);
	//			if (item)
	//			{
	//				QWidget * widgetC = item->widget();
	//				if (widgetC != nullptr)
	//				{
	//					delete widgetC;
	//					widgetC = nullptr;
	//				}
	//			}
	//			contextCombox = new QComboBox(m_addMarkFrom);
	//			contextLineBox = new QLineEdit(m_addMarkFrom);

	//			contextCombox->clear();
	//			if (index == 0)
	//			{ //材质
	//				QStringList tempStr;
	//				QVector<QString> types = hnApp::hnDataManager::getDataManager()->getRoadSurfaceTypes(currentMile.roadStandard);
	//				for (auto str : types)
	//				{
	//					tempStr << str;
	//				}
	//				contextCombox->addItems(tempStr);
	//				contextCombox->setCurrentIndex(0);
	//				vLayout->addWidget(contextCombox, 1, 2);

	//			}
	//			else if (index == 2)
	//			{
	//				QStringList tempStr;
	//				QVector<QString> levels = hnApp::hnDataManager::getDataManager()->getRoadLevel(currentMile.roadStandard);
	//				for (auto levelStr : levels)
	//				{
	//					tempStr << levelStr;
	//				}
	//				contextCombox->addItems(tempStr);
	//				contextCombox->setCurrentIndex(0);
	//				vLayout->addWidget(contextCombox, 1, 2);

	//			}
	//			else if (index ==3)
	//			{
	//				QStringList tempStr;
	//				tempStr << QStringLiteral("等级公路2018") << QStringLiteral("城镇道路") << QStringLiteral("低等级农村公路");
	//				contextCombox->addItems(tempStr);
	//				contextCombox->setCurrentIndex(0);
	//				vLayout->addWidget(contextCombox, 1, 2);
	//			}
	//			else if (index == 1|index ==4)
	//			{
	//				vLayout->addWidget(contextLineBox, 1, 2);
	//			}
	//		}
	//		);
	//		connect(okBtn1, &QPushButton::clicked, m_addMarkFrom, [&]()
	//		{
	//			//用户选择的达标类型
	//			QWidget *  nowWidget = vLayout->itemAtPosition(1, 1)->widget();
	//			QComboBox * box = qobject_cast<QComboBox*> (nowWidget);
	//			//用户选择的打标内容
	//			QWidget *  contextWidget = vLayout->itemAtPosition(1, 2)->widget();
	//			QComboBox * box1 = qobject_cast<QComboBox*> (contextWidget);
	//			QLineEdit * box2 = qobject_cast<QLineEdit*> (contextWidget);


	//			if (box)
	//			{
	//				markType = box->currentIndex();

	//			}
	//			if (box1)
	//			{
	//				text = box1->currentText();
	//			}
	//			if (box2)
	//			{
	//				text = box2->text();
	//			}
	//			double curMile = mileBox->text().toDouble();
	//			currentMarkInfo.dTrueMile = curMile;
	//			currentMarkInfo.nType = markType;
	//			currentMarkInfo.dGpsTimer = -1;
	//			std::string s1 = text.toLocal8Bit().toStdString();
	//			strcpy(currentMarkInfo.strMark,s1.c_str());
	//			m_addMarkFrom->accept();
	//		}
	//		);
	//		connect(cannelBtn1, &QPushButton::clicked, m_addMarkFrom, [=]()
	//		{
	//			m_addMarkFrom->reject();
	//		}
	//		);
	//		vLayout->addWidget(label, 0, 0);
	//		vLayout->addWidget(label0, 0, 1);
	//		vLayout->addWidget(label1, 0, 2);
	//		vLayout->addWidget(mileBox, 1, 0);
	//		vLayout->addWidget(typeCombox, 1, 1);
	//		vLayout->addWidget(contextCombox, 1, 2);
	//		QHBoxLayout * btnLayout = new QHBoxLayout(m_addMarkFrom);
	//		btnLayout->addWidget(okBtn1);
	//		btnLayout->addWidget(cannelBtn1);
	//		mainLy->addLayout(vLayout);
	//		mainLy->addLayout(btnLayout);
	//		m_addMarkFrom->setLayout(mainLy);
	//		//addMarkFrom->resize(300, 200);
	//		auto result = m_addMarkFrom->exec();
	//		if (result == QDialog::Accepted)
	//		{
	//			QStringList row;
	//			auto mark = currentMarkInfo;
	//		
	//			if (std::find(m_marks.begin(),m_marks.end(),currentMarkInfo)== m_marks.end())
	//			{
	//				QString type = mark.nType == 0 ? QStringLiteral("路面材质")
	//					: mark.nType == 1 ? QStringLiteral("路面单元")
	//					: mark.nType == 2 ? QStringLiteral("路面等级")
	//					: mark.nType == 3 ? QStringLiteral("路面标准(暂不可用)") : mark.nType == 4 ? QStringLiteral("路面情况") : QStringLiteral("错误");
	//				row << type;
	//				row << QString::number(mark.dTrueMile, 'f', 2);
	//				row << QString::fromLocal8Bit(mark.strMark);
	//				//
	//				int rowIndex = tableWidget->rowCount();
	//				tableWidget->insertRow(rowIndex);


	//				QTableWidgetItem* item0 = new QTableWidgetItem(row[0]);
	//				item0->setFlags(item0->flags() & ~Qt::ItemIsEditable);
	//				QTableWidgetItem* item1 = new QTableWidgetItem(row[1]);
	//				item1->setFlags(item0->flags() & ~Qt::ItemIsEditable);
	//				QTableWidgetItem* item2 = new QTableWidgetItem(row[2]);
	//				item2->setFlags(item0->flags() & ~Qt::ItemIsEditable);
	//				tableWidget->setItem(rowIndex, 0, item0);
	//				tableWidget->setItem(rowIndex, 1, item1);
	//				tableWidget->setItem(rowIndex, 2, item2);

	//				m_marks.push_back(currentMarkInfo); 
	//				m_newMarks.push_back(currentMarkInfo);
	//			} 
	//			else
	//			{

	//			}
	//		}
	//		else if (result == QDialog::Rejected)
	//		{

	//		}

	//	}
	//	else if (selectdItem == deleteAction)
	//	{
	//		if (this->m_newMarks.size()>0)
	//		{
	//			QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("检测到存在新增数据，请先执行完新增点击窗口确定按钮后，重新打开执行删除!"),
	//				QStringLiteral("确定"));
	//		}
	//		//删除选定行
	//		int selectedRow = tableWidget->currentRow();
	//		tableWidget->removeRow(selectedRow);
	//		m_deleteMarkIndexs.append(m_marks.at(selectedRow).nID);
	//		m_marks.remove(selectedRow);
	//		
	//	}
	//}
	//);
	//connect(okPtn, &QPushButton::clicked, this, [&](){
	//	accept();
	//});
	//connect(cancelPtn, &QPushButton::clicked, this, [&]() {
	//	reject();
	//});
	connect(outExcelBtn, &QPushButton::clicked, this, &hnMarkInfoDlg::outMarkFile);

	connect(cleanAllPtn, &QPushButton::clicked, this, &hnMarkInfoDlg::cleanAllMarks);
	this->setFixedSize(450, 500);
}

hnMarkInfoDlg::~hnMarkInfoDlg()
{
}

void hnMarkInfoDlg::resizeEvent(QResizeEvent * event)
{
	QDialog::resizeEvent(event);
}

void hnMarkInfoDlg::outMarkFile()
{
	//弹出用户选择窗口
	//弹出对话框 让用户设置输出位置
	QString projectPath = QFileDialog::getExistingDirectory(this, QStringLiteral("请选择输出路径"));
	if (projectPath.isEmpty())
	{
		return;
	}
	QString outName = "";
	auto curPro = hnApp::hnDataManager::getDataManager()->getCurrentProject() ;
	if (curPro->getProjectType() == PROJECT_TYPE::PROJECT_23D_TYPE|| curPro->getProjectType() == PROJECT_TYPE::PROJECT_2D_TYPE)
	{
		outName ="/"+curPro->get2DProName()+ QStringLiteral("_打标文件.xlsx");
	}
	else
	{
		outName = "/"+curPro->get3DProName() + QStringLiteral("_打标文件.xlsx");
	}
	
	QFile file(projectPath + "/"+outName);
	if (file.exists())
	{
		file.remove();
	}

	Document doc;
	doc.write(QStringLiteral("A1"), QStringLiteral("桩号"));
	doc.write(QStringLiteral("B1"), QStringLiteral("打标类型"));
	doc.write(QStringLiteral("C1"), QStringLiteral("打标信息"));

	for   (int i = 0; i < m_marks.size();++i)
	{
		auto var = m_marks[i];
		QString type = var.nType == 0 ? QStringLiteral("路面材质")
			: var.nType == 1 ? QStringLiteral("路面单元")
			: var.nType == 2 ? QStringLiteral("路面等级")
			: var.nType == 3 ? QStringLiteral("路面标准(暂不可用)") : var.nType == 4 ? QStringLiteral("路面情况") : QStringLiteral("错误");
	
		doc.write(QStringLiteral("A") + QString::number(i + 2), QString::number(var.dTrueMile, 'f', 2));
		doc.write(QStringLiteral("B") + QString::number(i + 2), type);
		doc.write(QStringLiteral("C") + QString::number(i + 2), QString::fromLocal8Bit(var.strMark)); 
	}
	if (doc.saveAs(projectPath + outName))
	{
		QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("输出完成!"));

	}
	else
	{
		QMessageBox::critical(this, QStringLiteral("提示"), QStringLiteral("输出失败,请检查!"));

	}

}

void hnMarkInfoDlg::cleanAllMarks()
{
	for (auto mark : m_marks)
	{
		m_deleteMarkIndexs.push_back(mark.nID);
	}
	this->m_marks.clear();

	tableWidget->clearContents();
	tableWidget->setRowCount(0);
}


#include "hnMileAdjustDlg.h"
#include <QLineEdit>
#include <QMenu>

#include "..\hnProject\hnProject.h"
#include "..\hnApplication\hnDataManager.h"
#include <QLabel>

hnMileAdjustDlg::hnMileAdjustDlg(QVector<hnCommon::hnMilePile>& milePile, QWidget *parent /*= Q_NULLPTR*/):m_milePile(milePile)
{
	ui.setupUi(this);
	ui.tableWidget->setColumnCount(2);
	ui.tableWidget->setRowCount(0);
	ui.tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	ui.tableWidget->setHorizontalHeaderLabels(QStringList() << QStringLiteral("桩号") << QStringLiteral("里程"));
	ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	connect(ui.pushButton, &QPushButton::clicked, this, [&]() {
		accept();
	});
	connect(ui.pushButton_2, &QPushButton::clicked, this, [&]() {
		reject();
	});
	for (int i = 1; i <milePile.size()-1; ++i)
	{
		QStringList row;
		auto mile = milePile.at(i);
		 
		row << QString::number(mile.dTrueMile,  'f', 2);
		row << QString::number(mile.dEnclMile, 'f', 2);
		int rowIndex = ui.tableWidget->rowCount();
		ui.tableWidget->insertRow(rowIndex);
		QTableWidgetItem * item0 = new QTableWidgetItem(row[0]);
		item0->setFlags(item0->flags() & ~Qt::ItemIsEditable);
		QTableWidgetItem * item1 = new QTableWidgetItem(row[1]);
		item1->setFlags(item1->flags() & ~Qt::ItemIsEditable);
		ui.tableWidget->setItem(rowIndex, 0,item0);
		ui.tableWidget->setItem(rowIndex, 1, item1); 
	}
	
	//ui.tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	//connect(ui.tableWidget, &QTableWidget::customContextMenuRequested, this, [=](const QPoint& pos)
	//{
	//	QMenu menu;
	//	QAction * addAction = menu.addAction(QStringLiteral("添加(a)"));
	//	addAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
	//	QAction * deleteAction = menu.addAction(QStringLiteral("删除(d)"));
	//	addAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
	//	QAction * selectdItem = menu.exec(ui.tableWidget->mapToGlobal(pos));
	//	if (selectdItem == addAction)
	//	{
	//		m_addMileFrom = new QDialog(this);
	//		QVBoxLayout * mainLy = new QVBoxLayout(m_addMileFrom);
	//		m_addMileFrom->setWindowTitle(QStringLiteral("添加较桩信息"));
	//		QGridLayout * vLayout = new QGridLayout(m_addMileFrom);
	//		QLabel * label = new QLabel(QStringLiteral("桩号"), m_addMileFrom);
	//		QLabel * label0 = new QLabel(QStringLiteral("里程"), m_addMileFrom);  
	//		QPushButton * okBtn1 = new QPushButton(QStringLiteral("确定"), m_addMileFrom);
	//		QPushButton * cannelBtn1 = new QPushButton(QStringLiteral("取消"), m_addMileFrom);
	//		m_mileBox = new QLineEdit(m_addMileFrom);
	//		m_dmiBox = new QLineEdit(m_addMileFrom);
	//		auto curProject = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	//		
	//		double dmi = 0;
	//		if (curProject->getProjectType() == PROJECT_23D_TYPE || curProject->getProjectType() == PROJECT_2D_TYPE)
	//		{
	//			hnMile currentMile;
	//			currentMile = curProject->getCurrentRoadMile();
	//			dmi = currentMile.dEnclMile;
	//		}
	//		else
	//		{ 
	//			dmi = curProject->getCurrent3DRoadDmi();;
	//		}
	// 
	////		m_mileBox->setText(QString::number(currentMile.dTrueMile, 'f',2));
	//		m_dmiBox->setText(QString::number(dmi, 'f', 2));
	//		m_dmiBox->setReadOnly(true);
	//		connect(okBtn1, &QPushButton::clicked, m_addMileFrom, [&]()
	//		{
	//			double curMile = m_mileBox->text().toDouble();
	//			double dmi = m_dmiBox->text().toDouble();
	//			m_currentInfo.dTrueMile = curMile;
	//			m_currentInfo.dEnclMile = dmi;
	//			m_currentInfo.dGpsTimer = -1;
	//			m_addMileFrom->accept();
	//		}
	//		);
	//		connect(cannelBtn1, &QPushButton::clicked, m_addMileFrom, [=]()
	//		{
	//			m_addMileFrom->reject();
	//		}
	//		);

	//		vLayout->addWidget(label, 0, 0);
	//		vLayout->addWidget(label0, 0, 1);
	//		vLayout->addWidget(m_mileBox, 1, 0);
	//		vLayout->addWidget(m_dmiBox, 1,1);
	//		QHBoxLayout * btnLayout = new QHBoxLayout(m_addMileFrom);
	//		btnLayout->addWidget(okBtn1);
	//		btnLayout->addWidget(cannelBtn1);
	//		mainLy->addLayout(vLayout);
	//		mainLy->addLayout(btnLayout);


	//		auto result = m_addMileFrom->exec();
	//		if (result == QDialog::Accepted)
	//		{
	//			QStringList row;
	//			auto mile = m_currentInfo; 
	//			if (std::find(m_milePile.begin(), m_milePile.end(), m_currentInfo) == m_milePile.end())
	//			{
	//				row << QString::number(mile.dTrueMile, 'f', 2);
	//				row << QString::number(mile.dEnclMile, 'f', 2);
	//				int rowIndex = ui.tableWidget->rowCount();
	//				ui.tableWidget->insertRow(rowIndex);
	//				ui.tableWidget->setItem(rowIndex, 0, new QTableWidgetItem(row[0]));
	//				ui.tableWidget->setItem(rowIndex, 1, new  QTableWidgetItem(row[1]));
	//				m_milePile.push_back(m_currentInfo);
	//			}
	//		
	//		}
	//		else if (result == QDialog::Rejected)
	//		{

	//		}
	//	}
	//	else if (selectdItem == deleteAction)
	//	{
	//		//删除选定行
	//		int selectedRow = ui.tableWidget->currentRow();
	//		ui.tableWidget->removeRow(selectedRow);
	//		m_milePile.remove(selectedRow-1);
	//	}

	//});
}

hnMileAdjustDlg::~hnMileAdjustDlg()
{

}

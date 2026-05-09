#include "hnSetPorjectMileForm.h" 
#include <QLineEdit>
#include <algorithm>
#include <functional>
#include <cstdint> 
#include "..\hnQtCommon\MyCommonMethods.h"
#include <qmessagebox.h>
hnSetPorjectMileForm::hnSetPorjectMileForm(const  QMap<QString, QVector<hnPro::hnProject*>> datas, QWidget *parent /*= Q_NULLPTR*/)
{
	 
	m_datas = datas;
	
	//对数据根据起点桩号进行排序
	/*for (  auto& group : m_datas)
	{
		std::sort<QVector<hnPro ::hnProject*>::iterator,decltype(comparePorjectSMile)>(group.begin(), group.end(), comparePorjectSMile);
	}*/
	ui.setupUi(this);
	initDialog();
	connect(ui.comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slots_initProjectForm(int)));
	slots_initProjectForm(0);
	connect(ui.pushButton, &QPushButton::clicked, this, &hnSetPorjectMileForm::slots_onOkButton);
	connect(ui.pushButton_2, &QPushButton::clicked, this, &hnSetPorjectMileForm::slots_cancelButton);
	connect(ui.pushButton_3, &QPushButton::clicked, this, &hnSetPorjectMileForm::slots_restoreButton);
	 
}

hnSetPorjectMileForm::~hnSetPorjectMileForm()
{
}

void hnSetPorjectMileForm::initDialog()
{

//	ui->comboBox.
	ui.comboBox->clear();
	for (QMap<QString, QVector<hnPro::hnProject*>>::iterator
		it = m_datas.begin(); it != m_datas.end(); ++it)
	{
		QString key = it.key();
		ui.comboBox->addItem(key, 1);
	}
}

void hnSetPorjectMileForm::restoreProjectForm(int index)
{
	this->m_scrollAreaGridLayout = new QGridLayout();
	//this->m_scrollAreaGridLayout->setHorizontalSpacing(20);
	//this->m_scrollAreaGridLayout->setVerticalSpacing(10);

	QLabel * projectNameLable = new QLabel(QStringLiteral("工程名称"));

	this->m_scrollAreaGridLayout->addWidget(projectNameLable, 0, 0);

	QLabel *sMileLable = new QLabel(QStringLiteral("有效起点"));

	this->m_scrollAreaGridLayout->addWidget(sMileLable, 0, 1);

	QLabel *eMileLable = new QLabel(QStringLiteral("有效终点"));
	this->m_scrollAreaGridLayout->addWidget(eMileLable, 0, 2);

	int rowCount = 1;

	for (auto project : m_datas[ui.comboBox->currentText()])
	{

		QLabel* proName = new QLabel(project->get2DProName(), this);
		this->m_scrollAreaGridLayout->addWidget(proName, rowCount, 0);
		double sMile = 0;
		sMile = project->getCurProSetInfo().dBegMile; 
		double eMile = 0;
		eMile = project->getCurProSetInfo().dEndMile;

	 
		QLineEdit * sMileEdit = new QLineEdit(QString::number(sMile), this);
		sMileEdit->setProperty("column", rowCount);
		connect(sMileEdit, &QLineEdit::textChanged, this, std::bind(&hnSetPorjectMileForm::slots_sMileValueChanged, this, sMileEdit, std::placeholders::_1));

		this->m_scrollAreaGridLayout->addWidget(sMileEdit, rowCount, 1);
		QLineEdit * eMileEdit = new QLineEdit(QString::number(eMile), this);

		int line = project->getCurProSetInfo().nLineType == 1;
		QIntValidator *validator = new QIntValidator(this);
		if (line == 1)
		{
			validator->setRange(project->getCurProSetInfo().dBegMile, project->getCurProSetInfo().dEndMile);
		}
		else
		{
			validator->setRange(project->getCurProSetInfo().dEndMile, project->getCurProSetInfo().dBegMile);

		}
		sMileEdit->setValidator(validator);
		eMileEdit->setValidator(validator);
		eMileEdit->setProperty("column", rowCount);
		//	connect(eMileEdit, &QLineEdit::textChanged, this, &hnSetPorjectMileForm::slots_eMileValueChanged);
		connect(eMileEdit, &QLineEdit::textChanged, this, std::bind(&hnSetPorjectMileForm::slots_eMileValueChanged, this, eMileEdit, std::placeholders::_1));

		this->m_scrollAreaGridLayout->addWidget(eMileEdit, rowCount, 2);

		uintptr_t address = reinterpret_cast<uintptr_t>(project);
		long long addressInt = static_cast<long long>(address);
		SetPorjectMileStruct setInfo(addressInt, ui.comboBox->currentIndex(), rowCount - 1, project->get2DProName(), sMile, eMile, false);
		 
		auto it = std::find(m_MileInfos.begin(), m_MileInfos.end(), setInfo);
		if ( it== m_MileInfos.end())
		{
			m_MileInfos.push_back(setInfo);
		}
		else
		{
			it->Emile = eMile;
			it->Smile = sMile;
		}
		rowCount++;
	}
	QSpacerItem * spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	this->m_scrollAreaGridLayout->addItem(spacer, ProjectIndex, 0);

	this->m_scrollAreaWidget = new QWidget();
	this->m_scrollAreaWidget->setLayout(m_scrollAreaGridLayout);
	this->ui.scrollArea->setWidget(m_scrollAreaWidget);
}

void hnSetPorjectMileForm::slots_initProjectForm(int index)
{
	this->m_scrollAreaGridLayout = new QGridLayout();
	//this->m_scrollAreaGridLayout->setHorizontalSpacing(20);
	//this->m_scrollAreaGridLayout->setVerticalSpacing(10);

	QLabel * projectNameLable = new QLabel(QStringLiteral("工程名称"));

	this->m_scrollAreaGridLayout->addWidget(projectNameLable, 0, 0);

	QLabel *sMileLable = new QLabel(QStringLiteral("有效起点"));

	this->m_scrollAreaGridLayout->addWidget(sMileLable, 0, 1);

	QLabel *eMileLable = new QLabel(QStringLiteral("有效终点"));
	this->m_scrollAreaGridLayout->addWidget(eMileLable, 0, 2);

	 int rowCount = 1;
	
	for (auto project : m_datas[ui.comboBox->currentText()])
	{

		QLabel* proName = new QLabel(project->get2DProName(),this);
		this->m_scrollAreaGridLayout->addWidget(proName, rowCount, 0);
		double sMile = 0;
		if (project->getCurProSetInfo().dUserBegMile==-1)
		{
			sMile = project->getCurProSetInfo().dBegMile;
		}
		else
		{
			sMile = project->getCurProSetInfo().dUserBegMile;
		}
		double eMile = 0; 
		
		if (project->getCurProSetInfo().dUserEndMile == -1)
		{
			eMile = project->getCurProSetInfo().dEndMile;
		}
		else
		{
			eMile = project->getCurProSetInfo().dUserEndMile;
		}

		QLineEdit * sMileEdit = new QLineEdit(QString::number(sMile),this);



		sMileEdit->setProperty("column", rowCount-1);
		connect(sMileEdit, &QLineEdit::textChanged, this,   std::bind(&hnSetPorjectMileForm::slots_sMileValueChanged,this,sMileEdit,std::placeholders::_1));

		this->m_scrollAreaGridLayout->addWidget(sMileEdit, rowCount, 1);
		QLineEdit * eMileEdit = new QLineEdit(QString::number(eMile),this);
		int line = project->getCurProSetInfo().nLineType == 1; 
		QIntValidator *validator = new QIntValidator(this); 
		if (line==1)
		{
			validator->setRange(project->getCurProSetInfo().dBegMile, project->getCurProSetInfo().dEndMile);
		}
		else
		{
			validator->setRange(project->getCurProSetInfo().dEndMile, project->getCurProSetInfo().dBegMile);

		}
		sMileEdit->setValidator(validator);
		eMileEdit->setValidator(validator);
		eMileEdit->setProperty("column", rowCount-1);
	//	connect(eMileEdit, &QLineEdit::textChanged, this, &hnSetPorjectMileForm::slots_eMileValueChanged);
		connect(eMileEdit, &QLineEdit::textChanged, this, std::bind(&hnSetPorjectMileForm::slots_eMileValueChanged, this, eMileEdit, std::placeholders::_1));

		this->m_scrollAreaGridLayout->addWidget(eMileEdit, rowCount, 2);

		uintptr_t address = reinterpret_cast<uintptr_t>(project);
		long long addressInt = static_cast<long long>(address);
		SetPorjectMileStruct setInfo(addressInt,ui.comboBox->currentIndex(), rowCount - 1, project->get2DProName(), sMile, eMile,false);

		if ( std::find(m_MileInfos.begin(), m_MileInfos.end(), setInfo) == m_MileInfos.end())
		{
			    m_MileInfos.push_back(setInfo);
		}
		rowCount++;
	}
	QSpacerItem * spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	this->m_scrollAreaGridLayout->addItem(spacer, ProjectIndex, 0);

	this->m_scrollAreaWidget = new QWidget();
	this->m_scrollAreaWidget->setLayout(m_scrollAreaGridLayout);
	this->ui.scrollArea->setWidget(m_scrollAreaWidget);
}



void hnSetPorjectMileForm::slots_sMileValueChanged(QLineEdit * lineEdit, QString text)
{
	bool ok;
	int clo = lineEdit->property("column").toInt();
	double mile = text.toDouble(&ok);
	if (ok)
	{
		  for (auto& seting:m_MileInfos)
		  {
			  if (seting.RoadIndex == ui.comboBox->currentIndex() && seting.ProjectIndex == clo)
			  {
			
					  seting.Smile = mile;
				
				
			  }
		  }
	}
}
 

void hnSetPorjectMileForm::slots_eMileValueChanged(QLineEdit * lineEdit, QString text)
{
	bool ok;
	int clo = lineEdit->property("column").toInt();
	double mile = text.toDouble(&ok);
	if (ok)
	{
		for (auto& seting : m_MileInfos)
		{
			if (seting.RoadIndex == ui.comboBox->currentIndex() && seting.ProjectIndex == clo)
			{
				hnPro::hnProject* curProjectPtr = reinterpret_cast<hnPro::hnProject*>(seting.ProjectPtr);
				//判断桩号区间 
			
					seting.Emile = mile;
				
			}
		}
	}
}

void hnSetPorjectMileForm::slots_onOkButton()
{
	//写配置文件记录用户多工程桩号配置
	for (auto& seting : m_MileInfos)
	{
		hnPro::hnProject* curProjectPtr = reinterpret_cast<hnPro::hnProject*>(seting.ProjectPtr);
		for (QMap<QString, QVector<hnPro::hnProject*>>::iterator
			it = m_datas.begin(); it != m_datas.end(); ++it)
		{
			    for (auto& project : it.value())
			    {
					bool  find = false;
					 if (project == curProjectPtr)
					 {
						 QString projectPath = project->get2DProPath();
						 QDir dir(projectPath);
						 QString filePath =  dir.filePath(QString("23dConfig.txt"));
						 QString line("UserSmile =" + QString::number(seting.Smile)); 
						 QString line1("UserEmile =" + QString::number(seting.Emile));  
						 QStringList txts;
						 txts.push_back(line);
						 txts.push_back(line1);
						 MyCommonMethods::writeAllLines(filePath, txts);
						 find = true;
						 break;
					 }
					 if (find)
					 {
						 break;
					 }
			    }
		}
	}

	this->accept();
}

void hnSetPorjectMileForm::slots_cancelButton()
{
	//退出窗口
	this->reject();
}

void hnSetPorjectMileForm::slots_restoreButton()
{

	//删除文件

	//还原工程赋值
	for (auto& project : m_datas[ui.comboBox->currentText()])
	{
		QString projectPath = project->get2DProPath();
		QDir dir(projectPath);
		QString filePath = dir.filePath(QString("23dConfig.txt"));
		QFile file(filePath);
		if (file.exists())
		{
			file.remove();
		}
	} 
	//还原界面
	restoreProjectForm(ui.comboBox->currentIndex());
	QMessageBox::information(this, "information!", QStringLiteral("该道路所有工程用户自定义桩号设置以还原为初始状态!"));
}

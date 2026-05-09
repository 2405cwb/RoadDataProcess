#include "calculateIrmForm.h"
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QProgressBar>
#include <QLabel>

calculateIrmForm::calculateIrmForm(QWidget* parent)
	: QDialog(parent)
	, m_flatnessCheckbox(nullptr)
	, m_rutCheckbox(nullptr)
	, m_smtdCheckbox(nullptr)
	, m_flatnessProgressBar(nullptr)
	, m_rutProgressBar(nullptr)
	, m_smtdProgressBar(nullptr)
	, m_stopRequested(false), 
	projectCount(0)
 
{
	setModal(true); 
	//进行计算 
	setWindowTitle(QStringLiteral("IRM计算"));
	// 初始化控件
	QLabel* allProjectsLabel = new QLabel(QStringLiteral("总进度"), this);
	m_flatnessCheckbox = new QCheckBox(QStringLiteral("平整度(车速)"), this);
	m_rutCheckbox = new QCheckBox(QStringLiteral("计算车辙RUT"), this);
	m_smtdCheckbox = new QCheckBox(QStringLiteral("计算SMTD"), this);
	m_smpdCheckbox = new QCheckBox(QStringLiteral("计算磨耗"), this);
	m_jhxxCheckbox = new QCheckBox(QStringLiteral("几何线性"), this);
	m_smtdCheckbox->setChecked(true);
		m_smpdCheckbox->setChecked(true);
		m_jhxxCheckbox->setChecked(false);
		m_flatnessCheckbox->setChecked(true);	
		m_rutCheckbox->setChecked(true);
	m_allProjectProgressBar = new QProgressBar(this);
	m_flatnessProgressBar = new QProgressBar(this);
	m_rutProgressBar = new QProgressBar(this);
	m_smtdProgressBar = new QProgressBar(this);
	m_smpdProgressBar = new QProgressBar(this);
	m_jhxxProgressBar = new QProgressBar(this);
	QPushButton* calculateButton = new QPushButton(QStringLiteral("计算"), this);
	QPushButton* cancelButton = new QPushButton(QStringLiteral("取消"), this);

	// 设置进度条的范围和最小值
	m_allProjectProgressBar->setMinimum(0); 
	//m_allProjectProgressBar->setMaximum(projectCount);
	m_flatnessProgressBar->setMinimum(0);
	m_flatnessProgressBar->setMaximum(100);
	m_rutProgressBar->setMinimum(0);
	m_rutProgressBar->setMaximum(100);
	m_smtdProgressBar->setMinimum(0);
	m_smtdProgressBar->setMaximum(200);

	m_smpdProgressBar->setMinimum(0);
	m_smpdProgressBar->setMaximum(200);

	m_jhxxProgressBar->setMinimum(0);
	m_jhxxProgressBar->setMaximum(100);
	resize(650, 400);
	// 使用布局将控件放置在窗口中
	QVBoxLayout* mainLayout = new QVBoxLayout(this); 
	QGridLayout * allProjectBOX = new QGridLayout(this); 
	allProjectBOX->addWidget( allProjectsLabel,0,0); 
	allProjectBOX->addWidget(m_allProjectProgressBar,0,1); 
	allProjectBOX->addWidget(m_flatnessCheckbox,1,0); 
	//flatnessLayout->addSpacerItem(new QSpacerItem(8, 0, QSizePolicy::Fixed, QSizePolicy::Minimum));//添加固定间距
	allProjectBOX->addWidget(m_flatnessProgressBar,1,1); 
	allProjectBOX->addWidget(m_rutCheckbox,2,0); 
	allProjectBOX->addWidget(m_rutProgressBar,2,1);  
	allProjectBOX->addWidget(m_smtdCheckbox,3,0); 
	allProjectBOX->addWidget(m_smtdProgressBar,3,1); 
	allProjectBOX->addWidget(m_smpdCheckbox,4,0); 
	allProjectBOX->addWidget(m_smpdProgressBar,4,1);  
	allProjectBOX->addWidget(m_jhxxCheckbox,5,0); 
	allProjectBOX->addWidget(m_jhxxProgressBar,5,1);  
	QHBoxLayout* buttonLayout = new QHBoxLayout; 
	allProjectBOX->setVerticalSpacing(15);
	mainLayout->addLayout(allProjectBOX);
	buttonLayout->addSpacerItem(new QSpacerItem(35, 20));
	buttonLayout->addWidget(calculateButton);
	buttonLayout->addSpacerItem(new QSpacerItem(20, 20));

	buttonLayout->addWidget(cancelButton);
	buttonLayout->addSpacerItem(new QSpacerItem(35, 20)); 
	mainLayout->addLayout(buttonLayout); 
	// 连接信号和槽
	connect(calculateButton, &QPushButton::clicked, this, &calculateIrmForm::onCalculateButtonClicked);
	connect(cancelButton, &QPushButton::clicked, this, &calculateIrmForm::onCancelButtonClicked);
	
}

calculateIrmForm::~calculateIrmForm()
{
	sumProgressHelpMap.clear();
	 for (int i = 0 ; i<m_threads.size();++i )
	 {
		 delete m_threads[i];
	 }

	 m_threads.clear();
}



void calculateIrmForm::onCalculateButtonClicked()
{
	m_Pools = QThreadPool::globalInstance();
	int idealThreadCount = QThread::idealThreadCount();
	m_Pools->setMaxThreadCount(idealThreadCount *2); 
	m_threads.clear(); 
	if (!m_flatnessCheckbox->isChecked() && !m_rutCheckbox->isChecked() && !m_smtdCheckbox->isChecked()
		&&!m_smpdCheckbox->isChecked()&&!m_jhxxCheckbox->isChecked())
	{
		QMessageBox::warning(this,QStringLiteral("警告"), QStringLiteral("请至少选择一项计算类型"));
		return;
	}
	//进行计算
	hnPro::hnProjectManager* manager = hnApp::hnDataManager::getDataManager()->getProjectManager();
	std::vector<hnPro::hnProject*> allPorject = manager->getAllBaseProject();
	
	 
	for (int i = 0;i<allPorject.size();++i)
	{
		hnPro::hnProject*  currentProject = allPorject.at(i);
		m_stopRequested = false;
		QVector < QPair< int,int> > typeSumVec;
		if (m_flatnessCheckbox->isChecked())
		{
			auto flatnessThread = new CalculationThread(CalculationThread::IRI, i, currentProject);
			connect(flatnessThread, &CalculationThread::progressUpdated, this, &calculateIrmForm::onProgressUpdated); 
			connect(flatnessThread, &CalculationThread::error, this, &calculateIrmForm::error);
			//flatnessThread->start();
			m_threads.push_back(flatnessThread);
			typeSumVec.push_back( qMakePair( CalculationThread::IRI,0));
			
		}

		if (m_rutCheckbox->isChecked())
		{
			auto rutThread = new CalculationThread(CalculationThread::Rut,  i, currentProject);
			connect(rutThread, &CalculationThread::progressUpdated, this, &calculateIrmForm::onProgressUpdated); 
			connect(rutThread, &CalculationThread::error, this, &calculateIrmForm::error);
			//rutThread->start();
			m_threads.push_back(rutThread);
			typeSumVec.push_back(qMakePair(CalculationThread::Rut,0));
		}

		if (m_smtdCheckbox->isChecked())
		{
			auto smtdThread = new CalculationThread(CalculationThread::Smtd,i, currentProject);
			connect(smtdThread, &CalculationThread::progressUpdated, this, &calculateIrmForm::onProgressUpdated); 
			connect(smtdThread, &CalculationThread::error, this, &calculateIrmForm::error);
			//smtdThread->start();
			m_threads.push_back(smtdThread);
			typeSumVec.push_back(qMakePair(CalculationThread::Smtd,0));
		}
		if (m_smpdCheckbox->isChecked())
		{
			auto smpdThread = new CalculationThread(CalculationThread::Smpd, i, currentProject);
			connect(smpdThread, &CalculationThread::progressUpdated, this, &calculateIrmForm::onProgressUpdated);
			connect(smpdThread, &CalculationThread::error, this, &calculateIrmForm::error);
			typeSumVec.push_back(qMakePair(CalculationThread::Smpd,0));
			//smtdThread->start();
			m_threads.push_back(smpdThread);
		}
		if (m_jhxxCheckbox->isChecked())
		{
			auto jhxxThread = new CalculationThread(CalculationThread::Jhxx, i, currentProject);
			connect(jhxxThread, &CalculationThread::progressUpdated, this, &calculateIrmForm::onProgressUpdated); 
			connect(jhxxThread, &CalculationThread::error, this, &calculateIrmForm::error);
			//smtdThread->start();
			m_threads.push_back(jhxxThread);
			typeSumVec.push_back(qMakePair(CalculationThread::Jhxx,0));
		} 

		sumProgressHelpMap.insert(currentProject, typeSumVec);
	}
	if (sumProgressHelpMap.count()>0)
	{
		int userSelectCount = sumProgressHelpMap.first().size();
		projectCount = allPorject.size()*userSelectCount*100;

	}
	m_allProjectProgressBar->setMaximum(projectCount);

	{
		SetProgressRange(CalculationThread::IRI, 0, allPorject.size() * 100);
		SetProgressRange(CalculationThread::Rut, 0, allPorject.size() * 100);
		SetProgressRange(CalculationThread::Smpd, 0, allPorject.size() * 100);
		SetProgressRange(CalculationThread::Smtd, 0, allPorject.size() * 100);
		SetProgressRange(CalculationThread::Jhxx, 0, allPorject.size() * 100);
	}


	for (auto i = 0; i < m_threads.size(); i++)
	{
		m_Pools->start(m_threads[i]);
	}
}

void calculateIrmForm::onCancelButtonClicked()
{
	for (int i = m_threads.size()-1; i >0; --i)
	{
		m_threads[i]->stop();
	}
	hide();
}

void calculateIrmForm::handelControls(int calculationType,QProgressBar*&  progressBar)
{
	switch (calculationType)
	{
	case  CalculationThread::IRI:
		progressBar = m_flatnessProgressBar;
		break;
	case CalculationThread::Rut:
		progressBar = m_rutProgressBar;
		break;
	case CalculationThread::Smtd:
		progressBar = m_smtdProgressBar;
		break;
	case CalculationThread::Smpd:
		progressBar = m_smpdProgressBar;
		break;
	case CalculationThread::Jhxx:
		progressBar = m_jhxxProgressBar;
		break;
	default:
		return;
	}
}

void calculateIrmForm::onProgressUpdated(hnPro::hnProject*project, int calculationType,double progress,bool isInt)
{
	
	QProgressBar* progressBar = nullptr; 
	handelControls(calculationType, progressBar );
	int progressValue = 0;

	
	if (isInt) //是整数
	{
		progressValue = progress;
	}
	else
	{
		progressValue = progress * 100; 
	}
	 
	if (sumProgressHelpMap.contains(project))
	{
		for (int i = 0; i < sumProgressHelpMap[project].size(); i++)
		{
			if (sumProgressHelpMap[project][i].first ==calculationType)
			{
				//设置该工程该指标进度到100
				sumProgressHelpMap[project][i].second = progressValue;
			}
		}
	}
	int sumValue = 0; 
	auto it = sumProgressHelpMap.begin();
	while (it != sumProgressHelpMap.end())
	{ 
		for (int i = 0; i < it.value().size(); ++i)
		{
			if (it.value()[i].first  == calculationType)
			{
				sumValue += it.value()[i].second;
			}
		} 
		++it;

	}
		progressBar->setValue(sumValue);
		ThreadFinish();
}
void calculateIrmForm::ThreadFinish()
{ 
		int completeCount = 0;
		auto it = sumProgressHelpMap.begin(); 
		/*	while (it != sumProgressHelpMap.end())
			{
				bool complete = true;

				for (int i = 0 ; i<it.value().size(); ++i)
				{
					if (it.value()[i].second !=100)
					{
						complete = false;
						break;
					}
				}
				if (complete)
				{
					completeCount++;
				}
				++it;
			}*/
			while (it != sumProgressHelpMap.end())
			{
			for (int i = 0 ; i<it.value().size(); ++i)
			{
				completeCount += it.value()[i].second;
			} 
			++it;
			}
		m_allProjectProgressBar->setValue(completeCount);
		if (completeCount == projectCount)
		{
			QThread::sleep(1);
			emit calculationFinished();
			close();
		}
}
void calculateIrmForm::onSetProgressMaxValue(int calculationType, int value)
{
	QProgressBar* progressBar = nullptr;
	handelControls(calculationType, progressBar);
	progressBar->setMaximum( value);
}

void calculateIrmForm::onSetProgressMinValue(int calculationType, int value)
{
	QProgressBar* progressBar = nullptr;
	handelControls(calculationType, progressBar);
	progressBar->setMinimum(value);
}

void calculateIrmForm::onMessage(QString msg)
{
	emit error(msg);
}

void calculateIrmForm::SetProgressRange(int calculationType, int minValue, int maxValue)
{
	QProgressBar* progressBar = nullptr;
	handelControls(calculationType, progressBar);
	progressBar->setRange(minValue, maxValue);
}


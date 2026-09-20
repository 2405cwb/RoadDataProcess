#include "../hnQtCommon/hnWindowUiState.h"
#include "../hnQtCommon/hnProgressStyle.h"
#include "calculateIrmForm.h"
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QProgressBar>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTimer>
#include <QClipboard>
#include <QApplication>

calculateIrmForm::calculateIrmForm(QWidget* parent)
	: QDialog(parent)
	, m_flatnessCheckbox(nullptr)
	, m_rutCheckbox(nullptr)
	, m_smtdCheckbox(nullptr)
	, m_smpdCheckbox(nullptr)
	, m_jhxxCheckbox(nullptr)
	, m_allProjectProgressBar(nullptr)
	, m_flatnessProgressBar(nullptr)
	, m_rutProgressBar(nullptr)
	, m_smtdProgressBar(nullptr)
	, m_smpdProgressBar(nullptr)
	, m_jhxxProgressBar(nullptr)
	, projectCount(0)
	, m_stopRequested(false)
	, m_hasCalculationError(false)
	, m_Pools(nullptr)
{
	setStyleSheet(hnProgressStyle::taskDialogStyleSheet());
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
	resize(760, 620);
	setMinimumSize(620, 480);
	// 使用布局将控件放置在窗口中
	QVBoxLayout* mainLayout = new QVBoxLayout(this); 
	QGridLayout * allProjectBOX = new QGridLayout();
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
	for (int type = 0; type < 5; ++type)
	{
		QLabel* state = new QLabel(QStringLiteral("待开始"), this);
		state->setMinimumWidth(92);
		m_itemStates.insert(type, state);
		allProjectBOX->addWidget(state, type + 1, 2);
	}
	m_itemStates[CalculationThread::Jhxx]->setText(QStringLiteral("请使用几何计算"));
	m_startButton = calculateButton;
        new hnWindowUiState(this, QStringLiteral("IrmCalculation"));
	m_closeButton = cancelButton;
	m_startButton->setText(QStringLiteral("开始计算"));
	m_closeButton->setText(QStringLiteral("关闭"));
	m_startButton->setDefault(true);
	m_jhxxCheckbox->setEnabled(false);
	m_jhxxCheckbox->setToolTip(QStringLiteral("请使用主界面的几何计算功能。"));
	m_summaryLabel = new QLabel(this);
	m_summaryLabel->setWordWrap(true);
	const auto projects = hnApp::hnDataManager::getDataManager()->getProjectManager()->getAllBaseProject();
	m_summaryLabel->setText(QStringLiteral("计算范围：全部已打开工程，共 %1 个。请选择计算项后开始。") .arg(projects.size()));
	QStringList names;
	for (auto project : projects)
	{
		names.append(project->get2DProName());
	}
	m_summaryLabel->setToolTip(names.join(QStringLiteral("\n")));
	mainLayout->insertWidget(0, m_summaryLabel);
	m_results = new QPlainTextEdit(this);
	m_results->setReadOnly(true);
	m_results->setPlaceholderText(QStringLiteral("运行状态和结果将在此显示；失败信息可复制。"));
	mainLayout->insertWidget(2, m_results, 1);
	QPushButton* copyButton = new QPushButton(QStringLiteral("复制详情"), this);
	buttonLayout->insertWidget(0, copyButton);
	connect(copyButton, &QPushButton::clicked, this, &calculateIrmForm::copyResults);
	m_pollTimer = new QTimer(this);
	m_pollTimer->setInterval(200);
	connect(m_pollTimer, &QTimer::timeout, this, &calculateIrmForm::pollTasks);
	for (auto bar : findChildren<QProgressBar*>())
	{
		bar->setRange(0, 100);
		bar->setValue(0);
	}
	connect(calculateButton, &QPushButton::clicked, this, &calculateIrmForm::onCalculateButtonClicked);
	connect(cancelButton, &QPushButton::clicked, this, &calculateIrmForm::onCancelButtonClicked);
	
}

calculateIrmForm::~calculateIrmForm()
{
	m_stopRequested = true;
	for (CalculationThread* thread : qAsConst(m_threads))
	{
		if (thread) thread->stop();
	}
	if (m_Pools) m_Pools->waitForDone();
	qDeleteAll(m_threads);
	m_threads.clear();
	sumProgressHelpMap.clear();
}



void calculateIrmForm::onCalculateButtonClicked()
{
	if (m_running) return;
	m_hasCalculationError = false;
	if (!m_Pools) m_Pools = new QThreadPool(this);
	qDeleteAll(m_threads);
	sumProgressHelpMap.clear();
	m_failedTasks.clear();
	m_results->clear();
	int idealThreadCount = qMax(1, QThread::idealThreadCount());
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
	if (allPorject.empty())
	{
		m_summaryLabel->setText(QStringLiteral("没有可计算的工程，请先打开工程。"));
		return;
	}
	m_stopRequested = false;
	m_poolDrained = false;
	m_elapsed.start();
	setRunning(true);
	for (auto bar : findChildren<QProgressBar*>()) bar->setValue(0);

	
	 
	for (int i = 0;i<allPorject.size();++i)
	{
		hnPro::hnProject*  currentProject = allPorject.at(i);
		m_stopRequested = false;
		QVector < QPair< int,int> > typeSumVec;
		if (m_flatnessCheckbox->isChecked())
		{
			auto flatnessThread = new CalculationThread(CalculationThread::IRI, i, currentProject);
			connect(flatnessThread, &CalculationThread::progressUpdated, this, &calculateIrmForm::onProgressUpdated); 
			connect(flatnessThread, &CalculationThread::error, this, &calculateIrmForm::onCalculationError);
			//flatnessThread->start();
			m_threads.push_back(flatnessThread);
			typeSumVec.push_back( qMakePair( CalculationThread::IRI,0));
			
		}

		if (m_rutCheckbox->isChecked())
		{
			auto rutThread = new CalculationThread(CalculationThread::Rut,  i, currentProject);
			connect(rutThread, &CalculationThread::progressUpdated, this, &calculateIrmForm::onProgressUpdated); 
			connect(rutThread, &CalculationThread::error, this, &calculateIrmForm::onCalculationError);
			//rutThread->start();
			m_threads.push_back(rutThread);
			typeSumVec.push_back(qMakePair(CalculationThread::Rut,0));
		}

		if (m_smtdCheckbox->isChecked())
		{
			auto smtdThread = new CalculationThread(CalculationThread::Smtd,i, currentProject);
			connect(smtdThread, &CalculationThread::progressUpdated, this, &calculateIrmForm::onProgressUpdated); 
			connect(smtdThread, &CalculationThread::error, this, &calculateIrmForm::onCalculationError);
			//smtdThread->start();
			m_threads.push_back(smtdThread);
			typeSumVec.push_back(qMakePair(CalculationThread::Smtd,0));
		}
		if (m_smpdCheckbox->isChecked())
		{
			auto smpdThread = new CalculationThread(CalculationThread::Smpd, i, currentProject);
			connect(smpdThread, &CalculationThread::progressUpdated, this, &calculateIrmForm::onProgressUpdated);
			connect(smpdThread, &CalculationThread::error, this, &calculateIrmForm::onCalculationError);
			typeSumVec.push_back(qMakePair(CalculationThread::Smpd,0));
			//smtdThread->start();
			m_threads.push_back(smpdThread);
		}
		if (m_jhxxCheckbox->isChecked())
		{
			auto jhxxThread = new CalculationThread(CalculationThread::Jhxx, i, currentProject);
			connect(jhxxThread, &CalculationThread::progressUpdated, this, &calculateIrmForm::onProgressUpdated); 
			connect(jhxxThread, &CalculationThread::error, this, &calculateIrmForm::onCalculationError);
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
	const QList<QCheckBox*> checks = {m_flatnessCheckbox, m_rutCheckbox, m_smtdCheckbox, m_smpdCheckbox, m_jhxxCheckbox};
	for (int type = 0; type < checks.size(); ++type)
	{
		m_itemStates[type]->setText(checks[type]->isChecked() ? QStringLiteral("处理中") : QStringLiteral("未选择"));
	}
	m_pollTimer->start();
}

void calculateIrmForm::onCancelButtonClicked()
{
	if (!m_running)
	{
		QDialog::reject();
		return;
	}
	m_stopRequested = true;
	for (CalculationThread* thread : qAsConst(m_threads))
	{
		thread->stop();
	}
	m_closeButton->setEnabled(false);
	m_closeButton->setText(QStringLiteral("正在停止…"));
	m_summaryLabel->setText(QStringLiteral("正在停止，请等待当前处理步骤退出。"));
}

void calculateIrmForm::reject()
{
	onCancelButtonClicked();
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
	 
	progressValue = qBound(0, progressValue, 100);
	if (!progressBar) return;
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
	int total = 0;
	for (auto it = sumProgressHelpMap.constBegin(); it != sumProgressHelpMap.constEnd(); ++it)
	{
		for (const auto& item : it.value()) total += item.second;
	}
	m_allProjectProgressBar->setValue(total);
}

void calculateIrmForm::onCalculationError(QString message)
{
	m_hasCalculationError = true;
	CalculationThread* task = qobject_cast<CalculationThread*>(sender());
	if (task)
	{
		m_failedTasks.insert(task);
		m_itemStates[task->calculationType()]->setText(QStringLiteral("有失败项"));
	}
	m_results->appendPlainText(message);
}

void calculateIrmForm::setRunning(bool running)
{
	m_running = running;
	m_startButton->setEnabled(!running);
	m_closeButton->setEnabled(true);
	m_closeButton->setText(running ? QStringLiteral("停止计算") : QStringLiteral("关闭"));
	for (auto check : findChildren<QCheckBox*>()) check->setEnabled(!running);
	m_jhxxCheckbox->setEnabled(false);
}

void calculateIrmForm::copyResults()
{
	QApplication::clipboard()->setText(m_summaryLabel->text() + QStringLiteral("\n\n") + m_results->toPlainText());
}

void calculateIrmForm::pollTasks()
{
	if (!m_running) return;
	const qint64 seconds = m_elapsed.elapsed() / 1000;
	if (!m_stopRequested)
	{
		m_summaryLabel->setText(QStringLiteral("正在计算 %1 项任务 · 已用 %2 分 %3 秒 · 已报告失败 %4 项")
			.arg(m_threads.size()).arg(seconds / 60).arg(seconds % 60).arg(m_failedTasks.size()));
	}
	// 私有线程池实际退出后再结束；额外留一轮事件循环接收排队的进度和错误信号。
	if (!m_Pools->waitForDone(0)) return;
	if (!m_poolDrained)
	{
		m_poolDrained = true;
		return;
	}
	m_pollTimer->stop();
	int success = 0;
	int failed = 0;
	int stopped = 0;
	const QStringList types = {QStringLiteral("平整度 IRI"), QStringLiteral("车辙 RUT"),
		QStringLiteral("SMTD"), QStringLiteral("磨耗"), QStringLiteral("几何线形")};
	for (auto task : m_threads)
	{
		int value = 0;
		for (const auto& item : sumProgressHelpMap.value(task->project()))
		{
			if (item.first == task->calculationType()) value = item.second;
		}
		QString state;
		if (m_failedTasks.contains(task)) { ++failed; state = QStringLiteral("失败，见上方详情"); }
		else if (m_stopRequested) { ++stopped; state = QStringLiteral("已停止，结果需核查"); }
		else if (value >= 100) { ++success; state = QStringLiteral("完成"); }
		else { ++failed; m_failedTasks.insert(task); state = QStringLiteral("未完成：任务已退出但未报告完成，请检查输入数据与标定配置"); }
		m_results->appendPlainText(QStringLiteral("%1 | %2 | %3")
			.arg(task->project()->get2DProName(), types.value(task->calculationType()), state));
	}
	for (auto task : m_threads)
	{
		m_itemStates[task->calculationType()]->setText(m_stopRequested ? QStringLiteral("已停止") : QStringLiteral("处理结束"));
	}
	for (auto task : m_failedTasks) m_itemStates[task->calculationType()]->setText(QStringLiteral("有失败项"));
	m_hasCalculationError = failed > 0;
	m_summaryLabel->setText(QStringLiteral("%1 · 完成 %2 项 / 失败 %3 项 / 停止 %4 项 · 用时 %5 分 %6 秒")
		.arg(m_stopRequested ? QStringLiteral("已停止") : QStringLiteral("处理结束"))
		.arg(success).arg(failed).arg(stopped).arg(seconds / 60).arg(seconds % 60));
	setRunning(false);
	if (!failed && !stopped) emit calculationFinished();
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
	onCalculationError(msg);
}

void calculateIrmForm::SetProgressRange(int calculationType, int minValue, int maxValue)
{
	QProgressBar* progressBar = nullptr;
	handelControls(calculationType, progressBar);
	progressBar->setRange(minValue, maxValue);
}


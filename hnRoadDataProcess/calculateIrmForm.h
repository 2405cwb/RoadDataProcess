#pragma once
#include <QDialog>
#include "CalculationThread.h"
#include <QMutex>
#include <QElapsedTimer>
#include <QSet>
#include <QThreadPool >
class QCheckBox;
class QPushButton;
class QPlainTextEdit;
class QTimer;
class QLabel;
class QProgressBar;	
using namespace hnApp;
class calculateIrmForm :public QDialog
{
	Q_OBJECT

public:
	explicit calculateIrmForm(QWidget *parent = nullptr);
	
	~calculateIrmForm();
	
signals:
	void calculationFinished();
	void error(QString meg);
public slots:
	void onCalculateButtonClicked();
	void onCancelButtonClicked();
	void handelControls(int calculationType, QProgressBar* & progressBar); 
	void onProgressUpdated(hnPro::hnProject* project, int calculationType,double progress,bool isInt);
	void onCalculationError(QString message);
	void onSetProgressMaxValue(int calculationType, int value);
	void onSetProgressMinValue(int calculationType, int value);
	void onMessage(QString msg);
protected:
	void reject() override;
private slots:
	void pollTasks();
	void copyResults();
private:
	bool m_running = false;
	bool m_poolDrained = false;
	QPushButton* m_startButton = nullptr;
	QPushButton* m_closeButton = nullptr;
	QLabel* m_summaryLabel = nullptr;
	QPlainTextEdit* m_results = nullptr;
	QTimer* m_pollTimer = nullptr;
	QElapsedTimer m_elapsed;
	QSet<CalculationThread*> m_failedTasks;
	QMap<int, QLabel*> m_itemStates;
	void setRunning(bool running);
	QCheckBox* m_flatnessCheckbox;
	QCheckBox* m_rutCheckbox;
	QCheckBox* m_smtdCheckbox;
	QCheckBox* m_smpdCheckbox;
	QCheckBox* m_jhxxCheckbox;
	QProgressBar* m_allProjectProgressBar;
	QProgressBar* m_flatnessProgressBar; 
	QProgressBar* m_rutProgressBar;
	QProgressBar* m_smtdProgressBar;
	QProgressBar* m_smpdProgressBar;
	QProgressBar* m_jhxxProgressBar;
	int projectCount;
	

	bool m_stopRequested;
	bool m_hasCalculationError;
	QVector<CalculationThread*> m_threads;
 
	QMap<hnPro::hnProject*, QVector<QPair< int,int>>> sumProgressHelpMap; //辅助判断总进度  key 工程名 ，value 存放计算指标

	void ThreadFinish();


	void SetProgressRange(int calculationType, int minValue,int maxValue);
	QMutex m_mutex;
private:
	QThreadPool*m_Pools;
};

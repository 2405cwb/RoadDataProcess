#pragma once
#include <QDialog>
#include <QThread>
#include "hnRoutePhotoExportService.h"

class QLineEdit;
class QSpinBox;
class QPlainTextEdit;
class QPushButton;
class QProgressBar;
class QCloseEvent;

// 一个工作线程串行执行整批检查和输出，结果只在 finished 后读取。
class hnRoutePhotoExportWorker : public QThread, public hnRoutePhotoObserver
{
    Q_OBJECT
public:
    explicit hnRoutePhotoExportWorker(QObject* parent = nullptr);
    QVector<hnRoutePhotoProject> projects;
    int year = 2026;
    QString destination;
    bool exportRequested = false;
    hnRoutePhotoCheck checked;
    QString outputPath;
    QString error;
    bool cancelled() const override;
    void progress(int value, const QString& message) override;
signals:
    void progressChanged(int value, const QString& message);
protected:
    void run() override;
};

// 批量范围由打开窗口时的已导入工程快照确定，窗口为模态以保护工程生命周期。
class hnRoutePhotoExportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit hnRoutePhotoExportDialog(const QVector<hnRoutePhotoProject>& projects, QWidget* parent = nullptr);
    ~hnRoutePhotoExportDialog();
protected:
    void closeEvent(QCloseEvent* event) override;
    void reject() override;
private slots:
    void chooseDirectory();
    void checkProjects();
    void exportProjects();
    void cancelWork();
    void workFinished();
    void updateProgress(int value, const QString& message);
private:
    void startWork(bool exportRequested);
    void setBusy(bool busy);
    QVector<hnRoutePhotoProject> m_projects;
    QLineEdit* m_destination;
    QSpinBox* m_year;
    QPlainTextEdit* m_details;
    QPushButton* m_browse;
    QPushButton* m_check;
    QPushButton* m_export;
    QPushButton* m_cancel;
    QProgressBar* m_progress;
    hnRoutePhotoExportWorker* m_worker;
};

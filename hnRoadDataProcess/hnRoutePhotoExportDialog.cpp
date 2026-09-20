#include "hnRoutePhotoExportDialog.h"
#include <QCloseEvent>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <exception>

hnRoutePhotoExportWorker::hnRoutePhotoExportWorker(QObject* parent)
    : QThread(parent)
{
}

bool hnRoutePhotoExportWorker::cancelled() const
{
    return isInterruptionRequested();
}

void hnRoutePhotoExportWorker::progress(int value, const QString& message)
{
    emit progressChanged(value, message);
}

void hnRoutePhotoExportWorker::run()
{
    try
    {
        hnRoutePhotoExportService service;
        checked = service.check(projects, year, this);
        if (exportRequested && !checked.cancelled && checked.errors.isEmpty())
        {
            service.exportPhotos(checked, year, destination, outputPath, error, this);
        }
    }
    catch (const std::exception& exception)
    {
        error = QStringLiteral("报送处理异常：%1").arg(QString::fromLocal8Bit(exception.what()));
    }
    catch (...)
    {
        error = QStringLiteral("报送处理发生未知异常，未完成导出。");
    }
}

hnRoutePhotoExportDialog::hnRoutePhotoExportDialog(const QVector<hnRoutePhotoProject>& projects, QWidget* parent)
    : QDialog(parent), m_projects(projects), m_destination(new QLineEdit(this)),
      m_year(new QSpinBox(this)), m_details(new QPlainTextEdit(this)),
      m_browse(new QPushButton(QStringLiteral("选择目录"), this)),
      m_check(new QPushButton(QStringLiteral("检查全部工程"), this)),
      m_export(new QPushButton(QStringLiteral("检查并导出"), this)),
      m_cancel(new QPushButton(QStringLiteral("关闭"), this)),
      m_progress(new QProgressBar(this)), m_worker(nullptr)
{
    setWindowTitle(QStringLiteral("路线实景照片报送"));
    resize(1000, 720);
    setMinimumSize(680, 480);
    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* instruction = new QLabel(QStringLiteral("导出全部已导入工程的第一路景观照片；先汇总检查，任一工程不通过则整批不导出。"), this);
    instruction->setWordWrap(true);
    layout->addWidget(instruction);
    QHBoxLayout* directoryRow = new QHBoxLayout;
    directoryRow->addWidget(new QLabel(QStringLiteral("输出目录："), this));
    directoryRow->addWidget(m_destination);
    directoryRow->addWidget(m_browse);
    directoryRow->addWidget(new QLabel(QStringLiteral("报送年份："), this));
    m_year->setRange(1000, 9999);
    m_year->setValue(2026);
    directoryRow->addWidget(m_year);
    layout->addLayout(directoryRow);
    QTableWidget* table = new QTableWidget(projects.size(), 3, this);
    table->setHorizontalHeaderLabels(QStringList() << QStringLiteral("工程") << QStringLiteral("路线编码") << QStringLiteral("二维工程路径"));
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    for (int i = 0; i < projects.size(); ++i)
    {
        table->setItem(i, 0, new QTableWidgetItem(projects[i].name));
        table->setItem(i, 1, new QTableWidgetItem(projects[i].roadCode));
        table->setItem(i, 2, new QTableWidgetItem(projects[i].path));
    }
    layout->addWidget(table, 1);
    m_details->setReadOnly(true);
    m_details->setPlaceholderText(QStringLiteral("检查结果显示在这里，可选中复制；会列出所有工程的问题。"));
    layout->addWidget(m_details, 2);
    m_progress->setRange(0, 100);
    layout->addWidget(m_progress);
    QHBoxLayout* buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(m_check);
    buttons->addWidget(m_export);
    buttons->addWidget(m_cancel);
    layout->addLayout(buttons);
    connect(m_browse, &QPushButton::clicked, this, &hnRoutePhotoExportDialog::chooseDirectory);
    connect(m_check, &QPushButton::clicked, this, &hnRoutePhotoExportDialog::checkProjects);
    connect(m_export, &QPushButton::clicked, this, &hnRoutePhotoExportDialog::exportProjects);
    connect(m_cancel, &QPushButton::clicked, this, &hnRoutePhotoExportDialog::cancelWork);
}

hnRoutePhotoExportDialog::~hnRoutePhotoExportDialog()
{
    if (m_worker)
    {
        m_worker->requestInterruption();
        m_worker->wait();
    }
}

void hnRoutePhotoExportDialog::chooseDirectory()
{
    const QString path = QFileDialog::getExistingDirectory(this, QStringLiteral("选择报送输出目录"), m_destination->text());
    if (!path.isEmpty())
    {
        m_destination->setText(path);
    }
}

void hnRoutePhotoExportDialog::checkProjects()
{
    startWork(false);
}

void hnRoutePhotoExportDialog::exportProjects()
{
    if (m_destination->text().trimmed().isEmpty())
    {
        m_details->setPlainText(QStringLiteral("请先选择输出目录。"));
        return;
    }
    startWork(true);
}

void hnRoutePhotoExportDialog::setBusy(bool busy)
{
    m_check->setEnabled(!busy);
    m_export->setEnabled(!busy);
    m_browse->setEnabled(!busy);
    m_destination->setEnabled(!busy);
    m_year->setEnabled(!busy);
    m_cancel->setText(busy ? QStringLiteral("取消任务") : QStringLiteral("关闭"));
}

void hnRoutePhotoExportDialog::startWork(bool exportRequested)
{
    if (m_worker)
    {
        return;
    }
    m_worker = new hnRoutePhotoExportWorker(this);
    m_worker->projects = m_projects;
    m_worker->year = m_year->value();
    m_worker->destination = m_destination->text().trimmed();
    m_worker->exportRequested = exportRequested;
    connect(m_worker, &hnRoutePhotoExportWorker::progressChanged, this, &hnRoutePhotoExportDialog::updateProgress);
    connect(m_worker, &QThread::finished, this, &hnRoutePhotoExportDialog::workFinished);
    m_details->clear();
    m_progress->setValue(0);
    setBusy(true);
    m_worker->start();
}

void hnRoutePhotoExportDialog::updateProgress(int value, const QString& message)
{
    if (value >= 0)
    {
        m_progress->setValue(value);
    }
    m_progress->setFormat(message + QStringLiteral(" %p%"));
}

void hnRoutePhotoExportDialog::workFinished()
{
    m_worker->wait();
    const hnRoutePhotoCheck& checked = m_worker->checked;
    QStringList lines;
    // 正式目录已提交时，迟到的取消请求不覆盖成功结果。
    if ((checked.cancelled || m_worker->isInterruptionRequested()) && m_worker->outputPath.isEmpty())
    {
        lines << QStringLiteral("任务已取消。未生成正式报送包。");
    }
    else if (!checked.errors.isEmpty())
    {
        lines << QStringLiteral("检查不通过：%1 个工程，共 %2 项阻断问题；整批未导出。")
            .arg(m_projects.size()).arg(checked.errors.size());
    }
    else if (m_worker->error.isEmpty())
    {
        lines << QStringLiteral("全部 %1 个工程检查通过，共 %2 张照片。")
            .arg(m_projects.size()).arg(checked.photos.size());
        if (!m_worker->outputPath.isEmpty())
        {
            lines << QStringLiteral("导出完成：%1").arg(m_worker->outputPath);
        }
        m_progress->setValue(100);
    }
    if (!m_worker->error.isEmpty())
    {
        lines << m_worker->error;
    }
    lines << checked.errors;
    if (!checked.warnings.isEmpty())
    {
        lines << QStringLiteral("提示（不阻断）：") << checked.warnings;
    }
    m_details->setPlainText(lines.join(QStringLiteral("\n\n")));
    m_progress->setFormat(QStringLiteral("任务结束 %p%"));
    m_worker->deleteLater();
    m_worker = nullptr;
    setBusy(false);
}

void hnRoutePhotoExportDialog::cancelWork()
{
    if (m_worker)
    {
        m_worker->requestInterruption();
        m_cancel->setText(QStringLiteral("正在取消…"));
    }
    else
    {
        reject();
    }
}

void hnRoutePhotoExportDialog::closeEvent(QCloseEvent* event)
{
    if (m_worker)
    {
        cancelWork();
        event->ignore();
        return;
    }
    QDialog::closeEvent(event);
}

void hnRoutePhotoExportDialog::reject()
{
    if (m_worker)
    {
        cancelWork();
        return;
    }
    QDialog::reject();
}

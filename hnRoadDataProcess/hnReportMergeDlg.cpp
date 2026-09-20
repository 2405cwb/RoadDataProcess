#include "hnReportMergeDlg.h"
#include "hnReportMergeTool.h"
#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QUuid>

hnReportMergeDlg::hnReportMergeDlg(QWidget* parent)
    : QDialog(parent), m_root(new QLineEdit(this)), m_keyword(new QLineEdit(this)),
      m_sheet(new QLineEdit(QStringLiteral("Sheet1"), this)), m_startRow(new QSpinBox(this)),
      m_roadCode(new QCheckBox(QStringLiteral("在数据区首列插入道路编号（取文件名第一个下划线前的内容）"), this)),
      m_files(new QListWidget(this)), m_summary(new QLabel(this)), m_merge(new QPushButton(QStringLiteral("开始合并"), this))
{
    setWindowTitle(QStringLiteral("同类报表合并"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/iconsNew/merge-reports.png")));
    resize(760, 580);
    setMinimumSize(620, 460);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);
    QLabel* title = new QLabel(QStringLiteral("同类报表合并"), this);
    title->setStyleSheet(QStringLiteral("font-size:20px;font-weight:600;color:#234d68;"));
    layout->addWidget(title);
    QLabel* hint = new QLabel(QStringLiteral("递归查找同类 .xlsx 报表，保留首份报表表头，按以下列表顺序追加指定工作表的数据。"), this);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    QFormLayout* form = new QFormLayout;
    form->setSpacing(12);
    QHBoxLayout* source = new QHBoxLayout;
    m_root->setReadOnly(true);
    m_root->setPlaceholderText(QStringLiteral("选择包含报表的文件夹（含子文件夹）"));
    QPushButton* browse = new QPushButton(QStringLiteral("选择文件夹…"), this);
    source->addWidget(m_root, 1);
    source->addWidget(browse);
    form->addRow(QStringLiteral("报表目录"), source);
    m_keyword->setPlaceholderText(QStringLiteral("例如 PCI、路面损坏、平整度"));
    form->addRow(QStringLiteral("文件名关键词"), m_keyword);
    form->addRow(QStringLiteral("工作表名称"), m_sheet);
    m_startRow->setRange(1, 1048576);
    m_startRow->setValue(5);
    m_startRow->setToolTip(QStringLiteral("首条数据所在行；此前的行作为首份报表的表头保留。"));
    form->addRow(QStringLiteral("数据起始行"), m_startRow);
    layout->addLayout(form);
    layout->addWidget(m_roadCode);
    layout->addWidget(m_summary);
    m_files->setAlternatingRowColors(true);
    m_files->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(m_files, 1);
    QLabel* note = new QLabel(QStringLiteral("自动排除 Excel 临时文件和历史“_合并结果”。结果保存到报表目录，源文件保持原样。"), this);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("color:#657581;"));
    layout->addWidget(note);
    QHBoxLayout* buttons = new QHBoxLayout;
    QPushButton* refresh = new QPushButton(QStringLiteral("刷新预览"), this);
    QPushButton* close = new QPushButton(QStringLiteral("关闭"), this);
    buttons->addWidget(refresh);
    buttons->addStretch();
    buttons->addWidget(close);
    buttons->addWidget(m_merge);
    m_merge->setDefault(true);
    m_merge->setStyleSheet(QStringLiteral("QPushButton{background:#176f96;color:white;border:0;border-radius:4px;padding:8px 20px;} QPushButton:disabled{background:#c5d1d8;}"));
    layout->addLayout(buttons);
    connect(browse, &QPushButton::clicked, this, &hnReportMergeDlg::chooseRoot);
    connect(refresh, &QPushButton::clicked, this, &hnReportMergeDlg::refreshFiles);
    connect(m_keyword, &QLineEdit::editingFinished, this, &hnReportMergeDlg::refreshFiles);
    connect(close, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_merge, &QPushButton::clicked, this, &hnReportMergeDlg::mergeReports);
    refreshFiles();
}

void hnReportMergeDlg::chooseRoot()
{
    QString qstrRoot = QFileDialog::getExistingDirectory(this, QStringLiteral("选择报表文件夹"), m_root->text());
    if (qstrRoot.isEmpty()) return;
    m_root->setText(QDir::toNativeSeparators(qstrRoot));
    refreshFiles();
}

void hnReportMergeDlg::refreshFiles()
{
    hnReportMergeTool tool;
    QStringList files = tool.findReports(m_root->text(), m_keyword->text());
    m_files->clear();
    for (const QString& file : files)
    {
        QListWidgetItem* item = new QListWidgetItem(QDir(m_root->text()).relativeFilePath(file), m_files);
        item->setToolTip(QDir::toNativeSeparators(file));
        item->setData(Qt::UserRole, file);
    }
    m_summary->setText(QStringLiteral("待合并报表  %1  份").arg(files.size()));
    m_merge->setEnabled(!files.isEmpty());
}

void hnReportMergeDlg::mergeReports()
{
    // 点击时重新扫描，避免目录或关键词修改后使用旧预览。
    refreshFiles();
    if (m_files->count() == 0) return;
    if (m_sheet->text().trimmed().isEmpty() || (m_roadCode->isChecked() && m_startRow->value() < 2))
    {
        QMessageBox::information(this, windowTitle(), QStringLiteral("请输入工作表名称；插入道路编号时，数据起始行至少为第 2 行。"));
        return;
    }
    QStringList files;
    for (int i = 0; i < m_files->count(); i++) files.append(m_files->item(i)->data(Qt::UserRole).toString());
    // 输出名不直接拼入筛选词，避免用户输入路径字符，并避免覆盖历史结果。
    QString qstrOutput = QDir(m_root->text()).filePath(QStringLiteral("报表_合并结果_%1_%2.xlsx")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmsszzz")), QUuid::createUuid().toString().mid(1, 8)));
    QProgressDialog progress(QStringLiteral("正在合并报表…"), QStringLiteral("取消"), 0, files.size(), this);
    progress.setWindowTitle(windowTitle());
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setMinimumDuration(0);
    progress.setAutoClose(false);
    progress.setAutoReset(false);
    progress.show();
    m_merge->setEnabled(false);
    QString qstrError;
    bool ok = false;
    {
        hnReportMergeTool tool;
        ok = tool.merge(files, m_sheet->text().trimmed(), m_startRow->value(), m_roadCode->isChecked(),
            qstrOutput, qstrError, [&progress](int current, int total)
        {
            progress.setLabelText(QStringLiteral("正在合并 %1 / %2 份报表…").arg(qMin(current + 1, total)).arg(total));
            progress.setValue(current);
            QApplication::processEvents();
            return !progress.wasCanceled();
        });
    }
    progress.close();
    m_merge->setEnabled(true);
    if (ok)
    {
        QMessageBox::information(this, windowTitle(), QStringLiteral("合并完成，共 %1 份报表。\n\n%2").arg(files.size()).arg(QDir::toNativeSeparators(qstrOutput)));
    }
    else if (!progress.wasCanceled())
    {
        QMessageBox::warning(this, windowTitle(), qstrError);
    }
}

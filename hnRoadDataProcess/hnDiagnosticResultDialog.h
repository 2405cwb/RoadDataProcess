#include "../hnQtCommon/hnWindowUiState.h"
#pragma once

#include <QDialog>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "hnProjectDiagnosticService.h"
#include "../hnQtCommon/hnProgressStyle.h"

// 筛选使用纯文本匹配，路径中的括号和反斜杠不作为正则表达式。
class hnDiagnosticFilterModel : public QSortFilterProxyModel
{
public:
    explicit hnDiagnosticFilterModel(QObject* parent) : QSortFilterProxyModel(parent), m_severity(-1) {}
    void setCriteria(int severity, const QString& text)
    {
        m_severity = severity;
        m_text = text.trimmed();
        invalidateFilter();
    }
protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const override
    {
        if (m_severity >= 0 && sourceModel()->index(row, 0, parent).data(Qt::UserRole).toInt() != m_severity)
        {
            return false;
        }
        for (int column = 0; column < sourceModel()->columnCount(); ++column)
        {
            const QModelIndex index = sourceModel()->index(row, column, parent);
            if (index.data().toString().contains(m_text, Qt::CaseInsensitive)
                || index.data(Qt::UserRole).toString().contains(m_text, Qt::CaseInsensitive)) return true;
        }
        return false;
    }
private:
    int m_severity;
    QString m_text;
};

class hnDiagnosticResultDialog : public QDialog
{
public:
    hnDiagnosticResultDialog(const ProjectDiagnosticResult& result, const QString& summary,
        const QStringList& reports, QWidget* parent = nullptr)
        : QDialog(parent), m_model(new QStandardItemModel(this)), m_filter(new hnDiagnosticFilterModel(this)),
          m_table(new QTableView(this)), m_severity(new QComboBox(this)), m_search(new QLineEdit(this)),
          m_count(new QLabel(this)), m_details(new QPlainTextEdit(this)), m_reports(new QComboBox(this)), m_summary(summary)
    {
        setWindowTitle(QStringLiteral("检查数据 · 检查结果"));
        setStyleSheet(hnProgressStyle::taskDialogStyleSheet() + QStringLiteral(
            "QTableView, QHeaderView { font-family: 'Microsoft YaHei'; font-size: 13px; color: #303E4B; }"
            "QTableView { background: white; alternate-background-color: #F1F4F7; border: 1px solid #BBC6D1; "
            "selection-background-color: #527A9C; selection-color: white; }"
            "QHeaderView::section { background: #E9EDF1; padding: 6px; border: 0px; border-bottom: 1px solid #BBC6D1; }"));
        resize(1060, 720);
        setMinimumSize(740, 520);
        QVBoxLayout* layout = new QVBoxLayout(this);
        QLabel* summaryLabel = new QLabel(summary, this);
        summaryLabel->setTextFormat(Qt::PlainText);
        summaryLabel->setWordWrap(true);
        layout->addWidget(summaryLabel);
        QLabel* hint = new QLabel(QStringLiteral("工程名过长时表格显示缩略名；悬停工程列或选中一行，可查看完整工程名和检查内容。通过项仅表示对应检查满足条件，普通信息不代表工程整体有效。"), this);
        hint->setWordWrap(true);
        layout->addWidget(hint);
        QHBoxLayout* filters = new QHBoxLayout;
        m_severity->addItem(QStringLiteral("全部结果"), -1);
        m_severity->addItem(QStringLiteral("错误"), ProjectDiagnosticError);
        m_severity->addItem(QStringLiteral("警告"), ProjectDiagnosticWarning);
        m_severity->addItem(QStringLiteral("通过 / 信息"), ProjectDiagnosticInfo);
        m_search->setPlaceholderText(QStringLiteral("搜索工程名、检查内容或路径"));
        m_search->setClearButtonEnabled(true);
        filters->addWidget(m_severity);
        filters->addWidget(m_search, 1);
        filters->addWidget(m_count);
        layout->addLayout(filters);
        m_model->setHorizontalHeaderLabels({QStringLiteral("状态"), QStringLiteral("工程"), QStringLiteral("检查内容"), QStringLiteral("相关路径")});
        for (const auto& issue : result.issues)
        {
            QString state = issue.severity == ProjectDiagnosticError ? QStringLiteral("错误") :
                issue.severity == ProjectDiagnosticWarning ? QStringLiteral("警告") :
                issue.code.endsWith(QStringLiteral("_PRESENT")) ? QStringLiteral("通过") : QStringLiteral("信息");
            const QString fullProjectName = issue.projectName.isEmpty() ? QStringLiteral("未标注工程") : issue.projectName;
            const QString projectDisplay = m_table->fontMetrics().elidedText(fullProjectName, Qt::ElideMiddle, 190);
            QList<QStandardItem*> items;
            QStandardItem* statusItem = new QStandardItem(state);
            QStandardItem* projectItem = new QStandardItem(projectDisplay);
            QStandardItem* messageItem = new QStandardItem(issue.message);
            QStandardItem* pathItem = new QStandardItem(issue.path);
            projectItem->setData(fullProjectName, Qt::UserRole);
            items << statusItem << projectItem << messageItem << pathItem;
            statusItem->setData(static_cast<int>(issue.severity), Qt::UserRole);
            statusItem->setData(issue.code, Qt::UserRole + 1);
            statusItem->setForeground(QColor(issue.severity == ProjectDiagnosticError ? "#A63737" :
                issue.severity == ProjectDiagnosticWarning ? "#8B651D" : "#3E6658"));
            statusItem->setToolTip(statusItem->text());
            projectItem->setToolTip(fullProjectName);
            messageItem->setToolTip(messageItem->text());
            pathItem->setToolTip(pathItem->text());
            m_model->appendRow(items);
        }
        m_filter->setSourceModel(m_model);
        m_table->setModel(m_filter);
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        m_table->setAlternatingRowColors(true);
        m_table->setWordWrap(false);
        m_table->verticalHeader()->hide();
        m_table->verticalHeader()->setDefaultSectionSize(30);
        m_table->horizontalHeader()->setStretchLastSection(false);
        m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
        m_table->setColumnWidth(0, 72);
        m_table->setColumnWidth(1, 190);
        m_table->setColumnWidth(3, 240);
        layout->addWidget(m_table, 3);
        m_details->setReadOnly(true);
        m_details->setPlaceholderText(QStringLiteral("完整检查内容与路径显示在此处，可选中复制。"));
        layout->addWidget(m_details, 1);
        QHBoxLayout* buttons = new QHBoxLayout;
        QPushButton* copy = new QPushButton(QStringLiteral("复制筛选结果"), this);
        buttons->addWidget(copy);
        m_reports->addItems(reports);
        m_reports->setMinimumWidth(140);
        m_reports->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        buttons->addWidget(m_reports, 1);
        QPushButton* open = new QPushButton(QStringLiteral("打开报告目录"), this);
        open->setEnabled(!reports.isEmpty());
        m_reports->setEnabled(!reports.isEmpty());
        buttons->addWidget(open);
        QPushButton* close = new QPushButton(QStringLiteral("关闭"), this);
        buttons->addWidget(close);
        layout->addLayout(buttons);
        connect(m_search, &QLineEdit::textChanged, this, &hnDiagnosticResultDialog::applyFilter);
        connect(m_severity, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &hnDiagnosticResultDialog::applyFilter);
        connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &hnDiagnosticResultDialog::showDetails);
        connect(copy, &QPushButton::clicked, this, &hnDiagnosticResultDialog::copyResults);
        connect(open, &QPushButton::clicked, this, &hnDiagnosticResultDialog::openReportDirectory);
        connect(close, &QPushButton::clicked, this, &QDialog::accept);
        applyFilter();
        new hnWindowUiState(this, QStringLiteral("DataCheck"));
    }
private:
    void applyFilter()
    {
        m_filter->setCriteria(m_severity->currentData().toInt(), m_search->text());
        m_count->setText(QStringLiteral("显示 %1 / %2 项").arg(m_filter->rowCount()).arg(m_model->rowCount()));
        m_details->clear();
        if (m_filter->rowCount()) m_table->setCurrentIndex(m_filter->index(0, 0));
        showDetails();
    }
    QString rowText(int row) const
    {
        return QStringLiteral("工程：%1\n[%2] %3\n检查代码：%4\n路径：%5")
            .arg(m_filter->index(row, 1).data(Qt::UserRole).toString(), m_filter->index(row, 0).data().toString(),
                m_filter->index(row, 2).data().toString(), m_filter->index(row, 0).data(Qt::UserRole + 1).toString(),
                m_filter->index(row, 3).data().toString());
    }
    void showDetails()
    {
        const QModelIndex index = m_table->currentIndex();
        m_details->setPlainText(index.isValid() ? rowText(index.row()) : QString());
    }
    void copyResults()
    {
        QStringList text;
        text << m_summary << QStringLiteral("当前筛选结果：%1 项").arg(m_filter->rowCount());
        for (int row = 0; row < m_filter->rowCount(); ++row) text << rowText(row);
        QApplication::clipboard()->setText(text.join(QStringLiteral("\n\n")));
    }
    void openReportDirectory()
    {
        if (!m_reports->currentText().isEmpty())
        {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_reports->currentText()).absolutePath()));
        }
    }
    QStandardItemModel* m_model;
    hnDiagnosticFilterModel* m_filter;
    QTableView* m_table;
    QComboBox* m_severity;
    QLineEdit* m_search;
    QLabel* m_count;
    QPlainTextEdit* m_details;
    QComboBox* m_reports;
    QString m_summary;
};

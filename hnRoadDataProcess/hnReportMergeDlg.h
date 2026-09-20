#pragma once
#include <QDialog>
class QLineEdit;
class QSpinBox;
class QCheckBox;
class QListWidget;
class QLabel;
class QPushButton;

// 集中选择来源、合并规则及预览，减少分散弹窗。
class hnReportMergeDlg : public QDialog
{
public:
    explicit hnReportMergeDlg(QWidget* parent = nullptr);
private:
    QLineEdit* m_root;
    QLineEdit* m_keyword;
    QLineEdit* m_sheet;
    QSpinBox* m_startRow;
    QCheckBox* m_roadCode;
    QListWidget* m_files;
    QLabel* m_summary;
    QPushButton* m_merge;
    void chooseRoot();
    void refreshFiles();
    void mergeReports();
};

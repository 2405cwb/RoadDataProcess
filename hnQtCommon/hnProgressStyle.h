#pragma once

#include <QString>

// 在应用启动时统一加载，确保进度控件首次显示即使用最终样式。
namespace hnProgressStyle
{
inline QString styleSheet()
{
    return QStringLiteral(
        "QProgressBar { border: 1px solid #BBC6D1; border-radius: 2px; "
        "background: #E9EDF1; color: #202E3B; min-height: 20px; "
        "padding: 0px; margin: 0px; text-align: center; "
        "font-family: 'Microsoft YaHei'; font-size: 12px; font-weight: normal; }"
        "QProgressBar::chunk { background: #527A9C; border-radius: 0px; margin: 0px; }"
        "QProgressDialog { background: #F5F6F8; }"
        "QProgressDialog QLabel { color: #303E4B; background: transparent; "
        "font-family: 'Microsoft YaHei'; font-size: 13px; padding: 0px; }"
        "QProgressDialog QPushButton { min-width: 76px; min-height: 26px; "
        "border: 1px solid #BBC6D1; border-radius: 2px; background: #F9FAFB; color: #303E4B; }"
        "QProgressDialog QPushButton:hover { background: #E7EDF3; }"
        "QProgressDialog QPushButton:pressed { background: #D8E2EB; }");
}
inline QString taskDialogStyleSheet()
{
    return QStringLiteral(
        "QDialog { background: #F5F6F8; }"
        "QLabel, QCheckBox, QRadioButton, QGroupBox, QTabWidget, QLineEdit, QComboBox, QPlainTextEdit, QPushButton, QTableView, QHeaderView, QSpinBox { "
        "font-family: 'Microsoft YaHei'; font-size: 13px; color: #303E4B; }"
        "QPlainTextEdit { background: white; border: 1px solid #BBC6D1; padding: 6px; }"
        "QPushButton { min-height: 28px; min-width: 78px; padding: 0px 12px; }"
        "QCheckBox:disabled, QLabel:disabled { color: #89939C; }");
}
}

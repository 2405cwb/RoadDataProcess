#pragma once
#include <QObject>
#include <QStringList>
#include <functional>
namespace QXlsx { class Worksheet; }

// 保留 Excel 模板格式、公式和合并单元格，按工作表纵向追加数据。
class hnReportMergeTool : public QObject
{
    Q_OBJECT
public:
    explicit hnReportMergeTool(QObject* parent = nullptr);
    ~hnReportMergeTool();
    QStringList findReports(const QString& root, const QString& keyword) const;
    bool merge(const QStringList& files, const QString& sheet, int startRow, bool roadCode,
        const QString& output, QString& error, const std::function<bool(int, int)>& progress);
private:
    bool copyCell(QXlsx::Worksheet* source, int sourceRow, int sourceColumn,
        QXlsx::Worksheet* target, int targetRow, int targetColumn) const;
    QString shiftedFormula(const QString& formula, int rowOffset, int columnOffset) const;
};

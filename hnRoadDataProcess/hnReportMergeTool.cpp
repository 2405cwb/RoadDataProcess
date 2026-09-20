#include "hnReportMergeTool.h"
#include "xlsxcell.h"
#include "xlsxcellformula.h"
#include "xlsxcellrange.h"
#include "xlsxdocument.h"
#include "xlsxworksheet.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

hnReportMergeTool::hnReportMergeTool(QObject* parent)
    : QObject(parent)
{
}

hnReportMergeTool::~hnReportMergeTool()
{
}

QStringList hnReportMergeTool::findReports(const QString& root, const QString& keyword) const
{
    QStringList files;
    if (root.trimmed().isEmpty() || keyword.trimmed().isEmpty()) return files;
    QDirIterator iterator(root, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (iterator.hasNext())
    {
        QString qstrPath = iterator.next();
        QFileInfo info(qstrPath);
        if (info.suffix().compare(QStringLiteral("xlsx"), Qt::CaseInsensitive) != 0 ||
            info.fileName().startsWith(QStringLiteral("~$")) ||
            info.completeBaseName().contains(QStringLiteral("_合并结果")) ||
            !info.completeBaseName().contains(keyword.trimmed(), Qt::CaseInsensitive)) continue;
        files.append(info.absoluteFilePath());
    }
    files.sort(Qt::CaseInsensitive);
    return files;
}

QString hnReportMergeTool::shiftedFormula(const QString& formula, int rowOffset, int columnOffset) const
{
    // 只平移公式中的相对 A1 引用；字符串常量和绝对行列保持原样。
    QString shifted;
    shifted.reserve(formula.size());
    bool inString = false;
    for (int i = 0; i < formula.size();)
    {
        const QChar current = formula.at(i);
        if (current == QLatin1Char('"'))
        {
            shifted += current;
            if (inString && i + 1 < formula.size() && formula.at(i + 1) == current)
            {
                shifted += current;
                i += 2;
                continue;
            }
            inString = !inString;
            ++i;
            continue;
        }
        if (inString || (i > 0 && (formula.at(i - 1).isLetterOrNumber() ||
            formula.at(i - 1) == QLatin1Char('_') || formula.at(i - 1) == QLatin1Char('.'))))
        {
            shifted += current;
            ++i;
            continue;
        }
        int pos = i;
        const bool fixedColumn = pos < formula.size() && formula.at(pos) == QLatin1Char('$');
        if (fixedColumn) ++pos;
        const int columnStart = pos;
        while (pos < formula.size() && formula.at(pos).isLetter() && pos - columnStart < 4) ++pos;
        const int columnLength = pos - columnStart;
        if (columnLength < 1 || columnLength > 3)
        {
            shifted += current;
            ++i;
            continue;
        }
        const bool fixedRow = pos < formula.size() && formula.at(pos) == QLatin1Char('$');
        if (fixedRow) ++pos;
        const int rowStart = pos;
        while (pos < formula.size() && formula.at(pos).isDigit()) ++pos;
        if (rowStart == pos || (pos < formula.size() &&
            (formula.at(pos).isLetterOrNumber() || formula.at(pos) == QLatin1Char('_') ||
             formula.at(pos) == QLatin1Char('.') || formula.at(pos) == QLatin1Char('('))))
        {
            shifted += current;
            ++i;
            continue;
        }
        int column = 0;
        for (int j = columnStart; j < columnStart + columnLength; ++j)
            column = column * 26 + formula.at(j).toUpper().unicode() - QLatin1Char('A').unicode() + 1;
        bool validRow = false;
        int row = formula.mid(rowStart, pos - rowStart).toInt(&validRow);
        if (!validRow || column > 16384 || row < 1 || row > 1048576)
        {
            shifted += formula.mid(i, pos - i);
            i = pos;
            continue;
        }
        if (!fixedColumn) column += columnOffset;
        if (!fixedRow) row += rowOffset;
        if (column < 1 || column > 16384 || row < 1 || row > 1048576)
        {
            shifted += QStringLiteral("#REF!");
            i = pos;
            continue;
        }
        QString columnName;
        while (column > 0)
        {
            --column;
            columnName.prepend(QChar(QLatin1Char('A').unicode() + column % 26));
            column /= 26;
        }
        if (fixedColumn) shifted += QLatin1Char('$');
        shifted += columnName;
        if (fixedRow) shifted += QLatin1Char('$');
        shifted += QString::number(row);
        i = pos;
    }
    return shifted;
}

bool hnReportMergeTool::copyCell(QXlsx::Worksheet* source, int sourceRow, int sourceColumn,
    QXlsx::Worksheet* target, int targetRow, int targetColumn) const
{
    const QXlsx::Cell* cell = source->cellAt(sourceRow, sourceColumn);
    if (!cell)
        return source == target && target->cellAt(targetRow, targetColumn) ?
            target->writeBlank(targetRow, targetColumn) : true;
    const QXlsx::Format format = cell->format();
    if (cell->formula().isValid())
    {
        const QString formula = shiftedFormula(cell->formula().formulaText(),
            targetRow - sourceRow, targetColumn - sourceColumn);
        return target->writeFormula(targetRow, targetColumn, QXlsx::CellFormula(formula),
            format, cell->value().toDouble());
    }
    const QVariant value = cell->value();
    if (value.isNull()) return target->writeBlank(targetRow, targetColumn, format);
    if (value.userType() == QMetaType::QString)
        return target->writeString(targetRow, targetColumn, value.toString(), format);
    return target->write(targetRow, targetColumn, value, format);
}

bool hnReportMergeTool::merge(const QStringList& files, const QString& sheetName, int startRow,
    bool roadCode, const QString& output, QString& error, const std::function<bool(int, int)>& progress)
{
    error.clear();
    if (files.isEmpty() || sheetName.trimmed().isEmpty() || startRow < 1 ||
        (roadCode && startRow < 2) || QFileInfo::exists(output))
    {
        error = QStringLiteral("请检查源文件、工作表名称及数据起始行；输出文件不得已存在。");
        return false;
    }
    // 只在输出目录中的临时副本工作，源报表始终不写入。
    QTemporaryDir staging(QDir(QFileInfo(output).absolutePath()).filePath(QStringLiteral(".report-merge-XXXXXX")));
    if (!staging.isValid())
    {
        error = QStringLiteral("无法创建报表临时目录，请检查输出目录。");
        return false;
    }
    const QString temporary = QDir(staging.path()).filePath(QStringLiteral("merged.xlsx"));
    if (!QFile::copy(files.first(), temporary))
    {
        error = QStringLiteral("无法创建报表副本，请检查源文件和输出目录。");
        return false;
    }
    QXlsx::Document target(temporary);
    if (!target.isLoadPackage() || !target.selectSheet(sheetName) || !target.currentWorksheet())
    {
        error = QStringLiteral("首份报表无法打开指定工作表“%1”：%2").arg(sheetName, files.first());
        return false;
    }
    const QStringList targetNames = target.sheetNames();
    for (const QString& name : targetNames)
    {
        if (name != sheetName && !target.deleteSheet(name))
        {
            error = QStringLiteral("无法移除首份报表中的其他工作表：%1").arg(name);
            return false;
        }
    }
    if (!target.selectSheet(sheetName) || !target.currentWorksheet())
    {
        error = QStringLiteral("无法重新选择合并结果工作表。");
        return false;
    }
    QXlsx::Worksheet* targetSheet = target.currentWorksheet();
    int nextRow = startRow;
    int columnCount = -1;
    for (int i = 0; i < files.size(); ++i)
    {
        if (progress && !progress(i, files.size()))
        {
            error = QStringLiteral("已取消合并，源报表未修改。");
            return false;
        }
        QXlsx::Document source(files.at(i));
        if (!source.isLoadPackage() || !source.selectSheet(sheetName) || !source.currentWorksheet())
        {
            error = QStringLiteral("无法读取报表的工作表“%1”：\n%2").arg(sheetName, files.at(i));
            return false;
        }
        QXlsx::Worksheet* sheet = source.currentWorksheet();
        const QXlsx::CellRange dimension = sheet->dimension();
        const int lastRow = dimension.isValid() ? dimension.lastRow() : 0;
        const int lastColumn = dimension.isValid() ? dimension.lastColumn() : 0;
        if (columnCount < 0) columnCount = lastColumn;
        const int count = qMax(0, lastRow - startRow + 1);
        if (columnCount != lastColumn || nextRow + count - 1 > 1048576 ||
            (roadCode && lastColumn >= 16384))
        {
            error = QStringLiteral("报表列数不一致或超过 Excel 行列上限：\n%1").arg(files.at(i));
            return false;
        }
        const QList<QXlsx::CellRange> merges = sheet->mergedCells();
        for (const QXlsx::CellRange& range : merges)
        {
            if (range.firstRow() < startRow && range.lastRow() >= startRow)
            {
                error = QStringLiteral("表头与数据区存在跨行合并单元格，无法安全合并：\n%1").arg(files.at(i));
                return false;
            }
        }
        if (count > 0 && i == 0 && roadCode)
        {
            // 首份报表的数据区原地右移一列，表头不移动。
            for (const QXlsx::CellRange& range : merges)
                if (range.firstRow() >= startRow && !targetSheet->unmergeCells(range))
                {
                    error = QStringLiteral("无法移动首份报表的数据区合并单元格。");
                    return false;
                }
            for (int row = startRow; row <= lastRow; ++row)
            {
                if ((row - startRow) % 256 == 0 && progress && !progress(i, files.size()))
                {
                    error = QStringLiteral("已取消合并，源报表未修改。");
                    return false;
                }
                for (int column = lastColumn; column >= 1; --column)
                    if (!copyCell(targetSheet, row, column, targetSheet, row, column + 1))
                    {
                        error = QStringLiteral("无法移动首份报表的数据列。");
                        return false;
                    }
                if (!targetSheet->writeString(row, 1, QFileInfo(files.at(i)).completeBaseName().section('_', 0, 0)))
                {
                    error = QStringLiteral("无法写入道路编号。");
                    return false;
                }
            }
            for (const QXlsx::CellRange& range : merges)
                if (range.firstRow() >= startRow && !targetSheet->mergeCells(QXlsx::CellRange(
                    range.firstRow(), range.firstColumn() + 1, range.lastRow(), range.lastColumn() + 1)))
                {
                    error = QStringLiteral("无法恢复首份报表的数据区合并单元格。");
                    return false;
                }
            // QXlsx 合并时会重写非左上角单元格，需从原报表恢复各格格式。
            for (const QXlsx::CellRange& range : merges)
                if (range.firstRow() >= startRow)
                    for (int row = range.firstRow(); row <= range.lastRow(); ++row)
                        for (int column = range.firstColumn(); column <= range.lastColumn(); ++column)
                            if (!copyCell(sheet, row, column, targetSheet, row, column + 1))
                            {
                                error = QStringLiteral("无法恢复首份报表的合并区域格式。");
                                return false;
                            }
        }
        if (count > 0 && i > 0)
        {
            const int rowOffset = nextRow - startRow;
            const int columnOffset = roadCode ? 1 : 0;
            for (int row = startRow; row <= lastRow; ++row)
            {
                if ((row - startRow) % 256 == 0 && progress && !progress(i, files.size()))
                {
                    error = QStringLiteral("已取消合并，源报表未修改。");
                    return false;
                }
                const int destinationRow = row + rowOffset;
                targetSheet->setRowHeight(destinationRow, destinationRow, sheet->rowHeight(row));
                targetSheet->setRowFormat(destinationRow, destinationRow, sheet->rowFormat(row));
                targetSheet->setRowHidden(destinationRow, destinationRow, sheet->isRowHidden(row));
                if (roadCode && !targetSheet->writeString(destinationRow, 1,
                    QFileInfo(files.at(i)).completeBaseName().section('_', 0, 0)))
                {
                    error = QStringLiteral("无法写入道路编号：%1").arg(files.at(i));
                    return false;
                }
                for (int column = 1; column <= lastColumn; ++column)
                    if (!copyCell(sheet, row, column, targetSheet, destinationRow, column + columnOffset))
                    {
                        error = QStringLiteral("无法复制报表单元格：%1").arg(files.at(i));
                        return false;
                    }
            }
            for (const QXlsx::CellRange& range : merges)
                if (range.firstRow() >= startRow && !targetSheet->mergeCells(QXlsx::CellRange(
                    range.firstRow() + rowOffset, range.firstColumn() + columnOffset,
                    range.lastRow() + rowOffset, range.lastColumn() + columnOffset)))
                {
                    error = QStringLiteral("无法复制报表合并单元格：%1").arg(files.at(i));
                    return false;
                }
            for (const QXlsx::CellRange& range : merges)
                if (range.firstRow() >= startRow)
                    for (int row = range.firstRow(); row <= range.lastRow(); ++row)
                        for (int column = range.firstColumn(); column <= range.lastColumn(); ++column)
                            if (!copyCell(sheet, row, column, targetSheet,
                                row + rowOffset, column + columnOffset))
                            {
                                error = QStringLiteral("无法恢复报表合并区域格式：%1").arg(files.at(i));
                                return false;
                            }
        }
        nextRow += count;
    }
    if (roadCode && !targetSheet->writeString(startRow - 1, 1, QStringLiteral("道路编号")))
    {
        error = QStringLiteral("无法写入道路编号表头。");
        return false;
    }
    if (progress && !progress(files.size(), files.size()))
    {
        error = QStringLiteral("已取消合并，源报表未修改。");
        return false;
    }
    if (!target.saveAs(temporary) || !QFile::rename(temporary, output))
    {
        error = QStringLiteral("保存合并结果失败，请检查输出目录。");
        return false;
    }
    return true;
}

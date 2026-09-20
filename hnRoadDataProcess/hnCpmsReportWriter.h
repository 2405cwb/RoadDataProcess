#pragma once
#include "hnOutExcelMile.h"
#include "hnReportProjectInfo.h"
#include "xlsxdocument.h"
#include "xlsxworksheet.h"
#include "xlsxcellformula.h"
#include <QRegularExpression>
#include <cmath>
#include <algorithm>

// CPMS 模板按病害名称、程度和单位匹配，避免依赖数据库记录的排列顺序。
class hnCpmsReportWriter
{
public:
    bool write(QXlsx::Document& book, hnPro::hnProject* project,
        QVector<hnOutExcelMile> segments, double interval, int category, bool sort, QString& error)
    {
        if (!project || interval <= 0 || segments.isEmpty())
        {
            error = QStringLiteral("CPMS 没有可输出的有效分段。");
            return false;
        }
        if (sort && segments.first().getStartMile() > segments.last().getEndMile())
        {
            std::reverse(segments.begin(), segments.end());
        }
        const QStringList sheets = book.sheetNames();
        QMap<QString, int> pages;
        QString activeSheet;
        qint64 activeBlock = 0;
        int activeSurface = -1;
        QString activeUnit;
        int offset = 0;
        double pageStart = 0, pageEnd = 0;
        for (hnOutExcelMile segment : segments)
        {
            const int surface = static_cast<int>(segment.RoadSurface);
            if (category == 0 && surface != 0 && surface != 1)
            {
                error = QStringLiteral("CPMS 路面调查模板仅支持沥青和水泥路面，所选范围含砂石路面。");
                return false;
            }
            const QString sheet = category == 0
                ? (surface == 0 ? QStringLiteral("沥青路面损坏调查表") : QStringLiteral("水泥路面损坏调查表"))
                : sheets.value(0);
            if (!book.selectSheet(sheet))
            {
                error = QStringLiteral("CPMS 模板缺少工作表：%1").arg(sheet);
                return false;
            }
            const double low = qMin(segment.getStartMile(), segment.getEndMile());
            const double high = qMax(segment.getStartMile(), segment.getEndMile());
            const qint64 block = static_cast<qint64>(std::floor((low + high) / (20.0 * interval)));
            // 材质、道路单元或整页桩号边界改变时另起一页。
            const bool newPage = sheet != activeSheet || block != activeBlock
                || surface != activeSurface || segment.getUnitStr() != activeUnit;
            const bool rural = project->getBaseStandard() == HnProjectEnums::RuralRoadlowLevel;
            const int firstColumn = category == 1 && rural ? 5 : 6;
            const int height = category == 1 ? 19 : category == 2 ? (rural ? 18 : 32)
                : (project->getCurProSetInfo().nDrawType == 1 ? (surface == 0 ? 24 : 23) : 33);
            if (newPage)
            {
                offset = pages.value(sheet, 0) * height;
                if (offset > 0) copyPage(book, height, offset);
                pages[sheet]++;
                activeSheet = sheet;
                activeBlock = block;
                activeSurface = surface;
                activeUnit = segment.getUnitStr();
                pageStart = sort ? low : segment.getStartMile();
                pageEnd = sort ? high : segment.getEndMile();
                // 清除模板示例及上一页的分段数值。
                for (int row = rural ? 8 : 7; row <= lastDiseaseRow(book, category); ++row)
                    for (int col = firstColumn; col < firstColumn + 10; ++col) put(book, offset + row, col, 0.0);
            }
            pageEnd = sort ? high : segment.getEndMile();
            const auto info = project->getCurProSetInfo();
            put(book, offset + 3, 1, QStringLiteral("路线：%1 %2")
                .arg(QString::fromLocal8Bit(info.strNumber), QString::fromLocal8Bit(info.strRoadName)));
            put(book, offset + 3, 4, info.nLineType == 1 ? QStringLiteral("上行") : QStringLiteral("下行"));
            put(book, offset + 3, 8, QString::fromLocal8Bit(info.strDate));
            if (category != 0) put(book, offset + 3, 13, QString::fromLocal8Bit(info.strSurveyor));
            put(book, offset + 4, firstColumn + 2, pageStart);
            put(book, offset + 4, firstColumn + 7, pageEnd);
            put(book, offset + 5, firstColumn + 2, qAbs(pageEnd - pageStart));
            put(book, offset + 5, firstColumn + 7, hnReportProjectInfo::roadWidth(project));
            const int column = qBound(0, static_cast<int>(std::floor((low + high) / (2.0 * interval)))
                - static_cast<int>(block * 10), 9) + firstColumn;
            const QMap<QString, int> rows = diseaseRows(book, category);
            if (category == 0)
            {
                for (const auto& disease : segment.getRoadDisVec())
                {
                    const QString name = normalize(QString::fromLocal8Bit(disease.strDisName));
                    const int row = findRow(rows, name);
                    if (!row)
                    {
                        error = QStringLiteral("CPMS 模板中未找到病害：%1").arg(name);
                        return false;
                    }
                    QString unit;
                    for (int r = row; r >= 7 && unit.isEmpty(); --r) unit = book.read(r, 5).toString();
                    // 分段缓存为加权面积；模板会乘权重，写入前还原一次。
                    double value = disease.diseaseWeight > 0 ? disease.dArea / disease.diseaseWeight : 0.0;
                    if (unit == QStringLiteral("m") || unit == QStringLiteral("条状m"))
                    {
                        // 长度按实际相交 DMI 比例分配；面积继续使用二三维已有分段结果。
                        const double length = qAbs(disease.dDmiEnd - disease.dDmiStart);
                        const double overlap = qMax(0.0, qMin(qMax(disease.dDmiStart, disease.dDmiEnd), segment.getEndDmi())
                            - qMax(qMin(disease.dDmiStart, disease.dDmiEnd), segment.getStartDmi()));
                        value = disease.dLength * (length > 1e-8 ? overlap / length : 1.0);
                    }
                    add(book, offset + row, column, value);
                }
            }
            else
            {
                const QVector<StreetDiseaseManage> values = category == 1 ? segment.getStreetYxMap() : segment.getStreetLjMap();
                for (const auto& value : values)
                {
                    const double quantity = value.Area + value.Count;
                    if (quantity == 0) continue;
                    const QString name = normalize(QString::fromLocal8Bit(value.StreetDis.strDiseaseTypeName));
                    const int row = findRow(rows, name);
                    if (!row)
                    {
                        error = QStringLiteral("CPMS 景观模板中未找到病害：%1").arg(name);
                        return false;
                    }
                    add(book, offset + row, column, quantity);
                }
            }
        }
        if (category != 0)
        {
            const bool rural = project->getBaseStandard() == HnProjectEnums::RuralRoadlowLevel;
            for (const QString& sheet : sheets)
            {
                if (!pages.contains(sheet) || !book.selectSheet(sheet)) continue;
                for (int page = 0; page < pages.value(sheet); ++page)
                {
                    const int height = category == 1 ? 19 : rural ? 18 : 32;
                    finishStreetPage(book, page * height, category, rural);
                }
            }
        }
        for (const QString& sheet : sheets)
            if (!pages.contains(sheet) && book.sheetNames().size() > 1) book.deleteSheet(sheet);
        return true;
    }

private:
    QString normalize(QString name) const
    {
        name.remove(' ');
        name.replace(QStringLiteral("（"), QStringLiteral(".")).remove(QStringLiteral("）"));
        name.replace(QStringLiteral("绿化管护不善"), QStringLiteral("绿化管护不良"));
        return name;
    }

    int findRow(const QMap<QString, int>& rows, const QString& name) const
    {
        if (rows.contains(name)) return rows.value(name);
        // 自动化模板不分程度的项目合并到本病害行；车辙和修补仍保留分类。
        const QString base = name.section('.', 0, 0);
        return rows.value(base, 0);
    }

    int lastDiseaseRow(QXlsx::Document& book, int category) const
    {
        if (category != 0)
        {
            for (int row = 7; row < 33; ++row)
                if (book.read(row, 1).toString().startsWith(QStringLiteral("评定结果"))) return row - 2;
            return 0;
        }
        for (int row = 7; row < 33; ++row)
            if (book.read(row, 1).toString().startsWith(QStringLiteral("DR="))) return row - 2;
        return 28;
    }

    QMap<QString, int> diseaseRows(QXlsx::Document& book, int category) const
    {
        QMap<QString, int> rows;
        QString base;
        for (int row = 7; row <= lastDiseaseRow(book, category); ++row)
        {
            const QString text = book.read(row, 1).toString().trimmed();
            if (!text.isEmpty()) base = text;
            QString level = book.read(row, category == 0 ? 3 : 2).toString().trimmed();
            if (base == QStringLiteral("修补"))
                level = book.read(row, 5).toString().startsWith(QStringLiteral("条状")) ? QStringLiteral("条状") : QStringLiteral("块状");
            if (level != QStringLiteral("轻") && level != QStringLiteral("中")
                && level != QStringLiteral("重")) level.clear();
            rows.insert(normalize(base + (level.isEmpty() ? QString() : QStringLiteral(".") + level)), row);
        }
        return rows;
    }

    // QXlsx 不计算模板公式，直接写出每页的合计、扣分和 SCI/TCI。
    void finishStreetPage(QXlsx::Document& book, int offset, int category, bool rural)
    {
        const int first = rural ? 8 : 7;
        const int last = category == 1 ? 12 : rural ? 11 : 24;
        const int firstColumn = rural && category == 1 ? 5 : 6;
        const int totalColumn = firstColumn + 10;
        const int scoreColumn = totalColumn + 1;
        const int deductionColumn = rural ? 2 : 3;
        const int weightColumn = rural && category == 1 ? 3 : 4;
        const double length = book.read(offset + 5, firstColumn + 2).toDouble();
        double pageScore = 0.0;
        int groupStart = first;
        for (int row = first; row <= last + 1; ++row)
        {
            if (row <= last && (row == first || book.read(offset + row, 1).toString().trimmed().isEmpty()))
                continue;
            double deduction = 0.0;
            for (int member = groupStart; member < row; ++member)
            {
                double total = 0.0;
                for (int col = firstColumn; col < firstColumn + 10; ++col)
                    total += book.read(offset + member, col).toDouble();
                put(book, offset + member, totalColumn, total);
                deduction += total * book.read(offset + member, deductionColumn).toDouble();
            }
            const double weight = book.read(offset + groupStart, weightColumn).toDouble();
            const double score = length > 0.0
                ? weight * qMax(0.0, 100.0 - deduction * 1000.0 / length) : 0.0;
            put(book, offset + groupStart, scoreColumn, score);
            pageScore += score;
            groupStart = row;
        }
        if (!rural && category == 2 && book.read(offset + 17, totalColumn).toDouble() > 0.0)
            pageScore = 0.0;
        put(book, offset + (category == 1 ? 15 : rural ? 14 : 27), 2, pageScore);
        if (rural && category == 1) put(book, offset + 15, 19, QString());
    }

    void put(QXlsx::Document& book, int row, int col, const QVariant& value)
    {
        const auto cell = book.cellAt(row, col);
        book.write(row, col, value, cell ? cell->format() : QXlsx::Format());
    }

    void add(QXlsx::Document& book, int row, int col, double value)
    {
        put(book, row, col, book.read(row, col).toDouble() + value);
    }

    // 保留合并单元格、行高和相对公式引用，不能使用只复制值的 copyRange。
    void copyPage(QXlsx::Document& book, int height, int offset)
    {
        const auto merges = book.currentWorksheet()->mergedCells();
        const QRegularExpression ref(QStringLiteral("(\\$?[A-Z]{1,3})(\\$?)([0-9]+)"));
        for (int row = 1; row <= height; ++row)
        {
            book.setRowHeight(row + offset, book.rowHeight(row));
            for (int col = 1; col <= 21; ++col)
            {
                const auto cell = book.cellAt(row, col);
                if (!cell) continue;
                const QXlsx::Format format = cell->format();
                QVariant value = cell->value();
                if (cell->formula().isValid())
                {
                    const QString formula = cell->formula().formulaText();
                    QString shifted;
                    int end = 0;
                    auto matches = ref.globalMatch(formula);
                    while (matches.hasNext())
                    {
                        const auto match = matches.next();
                        shifted += formula.mid(end, match.capturedStart() - end) + match.captured(1)
                            + match.captured(2) + QString::number(match.captured(3).toInt() + offset);
                        end = match.capturedEnd();
                    }
                    value = QStringLiteral("=") + shifted + formula.mid(end);
                }
                book.write(row + offset, col, value, format);
            }
        }
        for (const auto& range : merges)
            if (range.lastRow() <= height)
                book.mergeCells(QXlsx::CellRange(range.firstRow() + offset, range.firstColumn(),
                    range.lastRow() + offset, range.lastColumn()));
    }
};

#pragma once

#include "hnOutExcelMile.h"
#include "hnReportProjectInfo.h"
#include "xlsxdocument.h"
#include <QRegularExpression>
#include <QtMath>
#include <algorithm>

// 5211 报表只读取成果库形成的分段缓存，不依赖二维软件或 Office。
class hnRural5211ReportWriter
{
public:
    enum Kind
    {
        PciSummary = 7,
        RqiSummary = 8,
        DiseaseSurvey = 9,
        MqiSummary = 10,
        Detail = 11
    };

    bool write(QXlsx::Document& book, hnPro::hnProject* project,
        QVector<hnOutExcelMile> segments, Kind kind, bool sort, QString& error)
    {
        if (!project || segments.isEmpty())
        {
            error = QStringLiteral("5211 报表没有可输出的分段数据。");
            return false;
        }
        if (sort && project->getCurProSetInfo().nLineType == -1)
        {
            std::reverse(segments.begin(), segments.end());
        }
        if (kind == DiseaseSurvey)
            return writeSurvey(book, project, segments, error);
        if (!book.selectSheet(QStringLiteral("Sheet1")))
        {
            error = QStringLiteral("5211 模板缺少 Sheet1 工作表。");
            return false;
        }
        const auto info = project->getCurProSetInfo();
        const QString date = QString::fromLocal8Bit(info.strDate);
        const bool hasIri = project->get2DProject() && project->get2DProject()->_IsIRIMTD;
        const int dateColumn = kind == Detail ? 14 : kind == MqiSummary ? 14 : 10;
        if (date.size() >= 8)
        {
            put(book, 2, dateColumn, date.mid(0, 4));
            put(book, 2, dateColumn + 2, date.mid(4, 2));
            put(book, 2, dateColumn + 4, date.mid(6, 2));
        }
        if (kind == Detail)
        {
            int row = 4;
            for (hnOutExcelMile& segment : segments)
            {
                copyDataRowFormat(book, 4, row, 20);
                Scores score;
                if (!calculate(segment, hasIri, kind, score, error)) return false;
                const double start = segment.getStartMile();
                const double end = segment.getEndMile();
                put(book, row, 1, QString::fromLocal8Bit(info.strNumber));
                put(book, row, 2, QString::fromLocal8Bit(info.strNumber).right(6));
                put(book, row, 3, QString::fromLocal8Bit(info.strRoadName));
                put(book, row, 4, qMin(start, end));
                put(book, row, 5, qMax(start, end));
                put(book, row, 6, info.nLineType == 1 ? QStringLiteral("上行") : QStringLiteral("下行"));
                put(book, row, 7, segment.RoadDegreestr);
                put(book, row, 8, segment.RoadSurfaceStr);
                put(book, row, 9, qAbs(end - start));
                put(book, row, 10, hnReportProjectInfo::roadWidth(project));
                put(book, row, 11, score.mqi);
                put(book, row, 12, score.sci);
                put(book, row, 13, score.pqi);
                put(book, row, 14, 100.0);
                put(book, row, 15, score.tci);
                put(book, row, 16, score.pci);
                if (hasIri) put(book, row, 17, score.rqi);
                put(book, row, 18, 100.0);
                put(book, row, 19, QStringLiteral("自动化检测"));
                put(book, row, 20, date.left(4));
                ++row;
            }
            return true;
        }
        return writeSummary(book, project, segments, kind, hasIri, error);
    }

private:
    struct Scores
    {
        double pci = 0.0;
        double rqi = 0.0;
        double pqi = 0.0;
        double mqi = 0.0;
        double sci = 100.0;
        double tci = 100.0;
    };

    void put(QXlsx::Document& book, int row, int col, const QVariant& value)
    {
        const QXlsx::Cell* cell = book.cellAt(row, col);
        book.write(row, col, value, cell ? cell->format() : QXlsx::Format());
    }

    void copyDataRowFormat(QXlsx::Document& book, int sourceRow, int targetRow, int lastColumn)
    {
        book.setRowHeight(targetRow, book.rowHeight(sourceRow));
        QXlsx::Format fallback;
        fallback.setBorderStyle(QXlsx::Format::BorderThin);
        fallback.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
        for (int col = 1; col <= lastColumn; ++col)
        {
            const QXlsx::Cell* source = book.cellAt(sourceRow, col);
            book.write(targetRow, col, QVariant(), source ? source->format() : fallback);
        }
    }

    bool calculate(hnOutExcelMile& segment, bool hasIri, Kind kind, Scores& score, QString& error)
    {
        if (segment.getPCIExcelStr().isEmpty())
        {
            error = QStringLiteral("5211 报表缺少有效的路面破损计算结果。");
            return false;
        }
        const hnRoadTypeSetInfo param = segment.getRoadTypeSetInfo();
        score.pci = 100.0 - param.dPCI_a0 * qPow(segment.getDRScore(), param.dPCI_a1);
        if (kind == PciSummary) return qIsFinite(score.pci);
        if (!hasIri)
        {
            error = QStringLiteral("5211 报表缺少 IRI 数据。");
            return false;
        }
        if (hasIri)
        {
            if (segment.getIriExcelStr().isEmpty())
            {
                error = QStringLiteral("5211 报表缺少有效的平整度计算结果。");
                return false;
            }
            score.rqi = 100.0 / (1.0 + param.dRQI_a0 * qExp(param.dRQI_a1 * segment.getJudgeIirValue()));
        }
        if (kind == RqiSummary) return qIsFinite(score.rqi);
        const double pqiWeight = param.dPQI_WPCI + param.dPQI_WRQI;
        if (pqiWeight <= 0.0)
        {
            error = QStringLiteral("5211 报表的 PCI/RQI 权重无效。");
            return false;
        }
        score.pqi = (score.pci * param.dPQI_WPCI + score.rqi * param.dPQI_WRQI) / pqiWeight;
        score.sci = segment.getSciValue();
        score.tci = segment.getTciValue();
        score.mqi = score.sci * param.dMQI_WSCI + score.pqi * param.dMQI_WPQI
            + 100.0 * param.dMQI_WBCI + score.tci * param.dMQI_WTCI;
        if (!qIsFinite(score.pci) || !qIsFinite(score.rqi) || !qIsFinite(score.pqi) || !qIsFinite(score.mqi))
        {
            error = QStringLiteral("5211 报表的评价指标计算结果无效。");
            return false;
        }
        return true;
    }

    int grade(double value, const char* levelText)
    {
        const QStringList levels = QString::fromLocal8Bit(levelText).split(
            QRegularExpression(QStringLiteral("\\s+")), QString::SkipEmptyParts);
        for (int index = 0; index < qMin(4, levels.size()); ++index)
        {
            if (value >= levels.at(index).toDouble()) return index;
        }
        return 4;
    }

    bool writeSummary(QXlsx::Document& book, hnPro::hnProject* project,
        QVector<hnOutExcelMile>& segments, Kind kind, bool hasIri, QString& error)
    {
        if (kind == RqiSummary && !hasIri)
        {
            error = QStringLiteral("5211 平整度汇总表缺少 IRI 数据。");
            return false;
        }
        const auto info = project->getCurProSetInfo();
        double totalLength = 0.0;
        double weighted = 0.0;
        double weightedPqi = 0.0;
        double grades[5] = {0, 0, 0, 0, 0};
        double pqiGrades[5] = {0, 0, 0, 0, 0};
        for (hnOutExcelMile& segment : segments)
        {
            Scores score;
            if (!calculate(segment, hasIri, kind, score, error)) return false;
            const double length = qAbs(segment.getEndMile() - segment.getStartMile());
            if (length <= 0.0) continue;
            const hnRoadTypeSetInfo param = segment.getRoadTypeSetInfo();
            const double value = kind == PciSummary ? score.pci
                : kind == RqiSummary ? score.rqi : score.mqi;
            const char* levels = kind == PciSummary ? param.strPCILevel
                : kind == RqiSummary ? param.strRQILevel : param.strMQILevel;
            grades[grade(value, levels)] += length;
            if (kind == MqiSummary)
                pqiGrades[grade(score.pqi, param.strPQILevel)] += length;
            totalLength += length;
            weighted += value * length;
            weightedPqi += score.pqi * length;
        }
        if (totalLength <= 0.0)
        {
            error = QStringLiteral("5211 汇总表没有有效里程。");
            return false;
        }
        copyDataRowFormat(book, 5, 5, kind == MqiSummary ? 21 : 14);
        put(book, 5, 1, QString::fromLocal8Bit(info.strNumber));
        put(book, 5, 2, QString::fromLocal8Bit(info.strRoadName));
        put(book, 5, 3, qMin(segments.first().getStartMile(), segments.last().getEndMile()));
        put(book, 5, 4, qMax(segments.first().getStartMile(), segments.last().getEndMile()));
        put(book, 5, 5, totalLength / 1000.0);
        put(book, 5, 6, weighted / totalLength);
        for (int index = 0; index < 5; ++index)
            put(book, 5, 7 + index, grades[index] / 1000.0);
        put(book, 5, 12, (grades[0] + grades[1]) * 100.0 / totalLength);
        put(book, 5, 13, (grades[0] + grades[1] + grades[2]) * 100.0 / totalLength);
        if (kind != MqiSummary)
        {
            put(book, 5, 14, (grades[3] + grades[4]) * 100.0 / totalLength);
        }
        else
        {
            put(book, 5, 14, weightedPqi / totalLength);
            for (int index = 0; index < 5; ++index)
                put(book, 5, 15 + index, pqiGrades[index] / 1000.0);
            put(book, 5, 20, (pqiGrades[0] + pqiGrades[1]) * 100.0 / totalLength);
            put(book, 5, 21, (pqiGrades[0] + pqiGrades[1] + pqiGrades[2]) * 100.0 / totalLength);
        }
        return true;
    }

    QString normalized(QString text)
    {
        text.remove(' ').remove('\n').remove('\r');
        text.replace(QStringLiteral("（"), QStringLiteral("("));
        text.replace(QStringLiteral("）"), QStringLiteral(")"));
        return text.section('(', 0, 0).section('.', -1);
    }

    bool writeSurvey(QXlsx::Document& book, hnPro::hnProject* project,
        QVector<hnOutExcelMile>& segments, QString& error)
    {
        const auto info = project->getCurProSetInfo();
        const QStringList sheets = book.sheetNames();
        QMap<QString, int> rows;
        for (hnOutExcelMile& segment : segments)
        {
            const int surface = static_cast<int>(segment.RoadSurface);
            if (surface < 0 || surface >= sheets.size())
            {
                error = QStringLiteral("5211 病害统计表不支持当前路面材质。");
                return false;
            }
            const QString sheet = sheets.at(surface);
            if (!book.selectSheet(sheet))
            {
                error = QStringLiteral("5211 病害统计模板缺少工作表：%1").arg(sheet);
                return false;
            }
            const int row = rows.value(sheet, 5) + 1;
            rows[sheet] = row;
            copyDataRowFormat(book, 6, row, surface == 2 ? 7 : 10);
            put(book, 2, 2, QString::fromLocal8Bit(info.strNumber));
            put(book, 2, 4, QString::fromLocal8Bit(info.strRoadName));
            put(book, 2, 6, info.nLineType == 1 ? QStringLiteral("上行") : QStringLiteral("下行"));
            if (row == 6)
            {
                put(book, 3, 2, segment.getStartMile());
                put(book, 3, 4, segment.getRoadLength());
            }
            put(book, row, 1, segment.getStartMile());
            put(book, row, 2, segment.getEndMile());
            for (int col = 3; col <= (surface == 2 ? 6 : 9); ++col)
                put(book, row, col, 0.0);
            for (const auto& disease : segment.getRoadDisVec())
            {
                const QString fullName = QString::fromLocal8Bit(disease.strDisName);
                const QStringList parts = fullName.split('.');
                const bool suffixLevel = parts.last() == QStringLiteral("轻")
                    || parts.last() == QStringLiteral("重") || parts.last() == QStringLiteral("中");
                const QString name = normalized(suffixLevel && parts.size() > 1
                    ? parts.at(parts.size() - 2) : parts.last());
                const QString level = suffixLevel ? parts.last()
                    : disease.nLevel == 1 ? QStringLiteral("轻")
                    : disease.nLevel >= 2 ? QStringLiteral("重") : QString();
                int column = 0;
                QString heading;
                for (int col = 3; col <= (surface == 2 ? 6 : 9); ++col)
                {
                    const QString current = normalized(book.read(4, col).toString());
                    if (!current.isEmpty()) heading = current;
                    const QString columnLevel = book.read(5, col).toString();
                    if (name == heading && (columnLevel.isEmpty() || columnLevel == level))
                    {
                        column = col;
                        break;
                    }
                }
                if (!column)
                {
                    error = QStringLiteral("5211 病害统计模板中未找到病害：%1").arg(name);
                    return false;
                }
                const double area = disease.diseaseWeight > 0.0
                    ? disease.dArea / disease.diseaseWeight : disease.dArea;
                put(book, row, column, book.read(row, column).toDouble() + area);
            }
        }
        for (int surface = 0; surface < sheets.size(); ++surface)
        {
            const QString sheet = sheets.at(surface);
            book.selectSheet(sheet);
            put(book, 2, 2, QString::fromLocal8Bit(info.strNumber));
            put(book, 2, 4, QString::fromLocal8Bit(info.strRoadName));
            put(book, 2, 6, info.nLineType == 1 ? QStringLiteral("上行") : QStringLiteral("下行"));
            const int totalRow = rows.value(sheet, 5) + 1;
            copyDataRowFormat(book, 6, totalRow, surface == 2 ? 7 : 10);
            put(book, totalRow, 1, QStringLiteral("总计"));
            for (int col = 3; col <= (surface == 2 ? 6 : 9); ++col)
            {
                double sum = 0.0;
                for (int row = 6; row < totalRow; ++row)
                    sum += book.read(row, col).toDouble();
                put(book, totalRow, col, sum);
            }
        }
        return true;
    }
};

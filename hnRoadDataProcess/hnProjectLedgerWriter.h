#pragma once
#include "../hnProject/hnProject.h"
#include "xlsxdocument.h"
#include <QDir>
#include <algorithm>

// 公共输出：与二维工程台账保持 A:U 共 21 列一致。
class hnProjectLedgerWriter
{
public:
    bool write(const std::vector<hnPro::hnProject*>& projects, const QString& path, QString& error)
    {
        QXlsx::Document book(QStringLiteral(":/reports/project-ledger.xlsx"));
        if (!book.selectSheet(QStringLiteral("Sheet1")))
        {
            error = QStringLiteral("工程台账模板缺少 Sheet1 工作表。");
            return false;
        }
        QXlsx::Format format;
        format.setBorderStyle(QXlsx::Format::BorderThin);
        format.setVerticalAlignment(QXlsx::Format::AlignVCenter);
        int row = 2;
        for (hnPro::hnProject* project : projects)
        {
            if (!project)
            {
                error = QStringLiteral("工程列表中存在无效工程，请重新加载后导出。");
                return false;
            }
            if (!project->getCurDB() || !project->getCurDB()->isOpen())
            {
                error = QStringLiteral("工程数据库未打开，无法生成工程台账：%1").arg(project->getProjectName());
                return false;
            }
            hnProjectSetInfo info;
            if (!project->getCurDB()->m_projectSetTable.readData(info))
            {
                error = QStringLiteral("无法从工程数据库读取工程信息：%1").arg(project->getProjectName());
                return false;
            }
            std::vector<hnMarkInfo> marks;
            if (!project->getCurDB()->m_markerInfoTable.readData(marks))
            {
                error = QStringLiteral("无法从工程数据库读取打标信息：%1").arg(project->getProjectName());
                return false;
            }
            const QString basePath = project->getAbsulotelyPath();
            std::sort(marks.begin(), marks.end(), [](const hnMarkInfo& left, const hnMarkInfo& right) {
                return left.dTrueMile < right.dTrueMile;
            });
            QStringList markLines;
            for (const hnMarkInfo& mark : marks)
            {
                const QString text = QString::fromLocal8Bit(mark.strMark).trimmed();
                if (text.isEmpty()) continue;
                markLines << QStringLiteral("%1：%2（%3）")
                    .arg(QString::number(mark.dTrueMile, 'f', 3), text)
                    .arg(mark.nType);
            }
            const QString markText = markLines.join(QStringLiteral("\r\n"));
            const QString surface = info.nRSurfaceType == 0 ? QStringLiteral("沥青")
                : info.nRSurfaceType == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
            const QVariantList values = QVariantList()
                << QString::fromLocal8Bit(info.strProvince) << QString::fromLocal8Bit(info.strCity)
                << QString::fromLocal8Bit(info.strCounty) << QString::fromLocal8Bit(info.strNumber)
                << QString::fromLocal8Bit(info.strRoadName)
                << (info.nLineType == 1 ? QStringLiteral("上行") : QStringLiteral("下行"))
                << QString::fromLocal8Bit(info.strRoadNO) << info.dBegMile << info.dEndMile
                << QStringLiteral("=ABS(H%1-I%1)*0.001").arg(row)
                << QString::fromLocal8Bit(info.strRoadLevel) << surface
                << QString::fromLocal8Bit(info.strDate) << QString::fromLocal8Bit(info.strTimer)
                << QString::fromLocal8Bit(info.strSurveyor) << QString::fromLocal8Bit(info.strWeather)
                << QDir::toNativeSeparators(basePath) << QVariant() << QVariant() << QVariant() << markText;
            for (int col = 0; col < values.size(); ++col)
            {
                QXlsx::Format cellFormat = format;
                if (col == 20) cellFormat.setTextWrap(true);
                book.write(row, col + 1, values[col], cellFormat);
            }
            ++row;
        }
        if (!book.saveAs(path))
        {
            error = QStringLiteral("工程台账保存失败，请检查目录权限及文件是否被占用：%1").arg(path);
            return false;
        }
        return true;
    }
};

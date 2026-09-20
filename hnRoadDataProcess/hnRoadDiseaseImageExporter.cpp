#include "hnRoadDiseaseImageExporter.h"
#include "../hnProject/hnProject.h"
#include "../hnProject/hn2DProject.h"
#include "../hnApplication/hnDataManager.h"
#include "../hnApplication/hnDiseaseService.h"
#include "xlsxdocument.h"
#include "xlsxformat.h"
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QRegularExpression>
#include <QPen>
#include <QTextStream>
#include <algorithm>

QString hnRoadDiseaseImageExporter::safeName(const QString& value) const
{
    QString result = value.trimmed();
    result.replace(QRegularExpression(QStringLiteral("[\\/:*?\"<>|]")), QStringLiteral("_"));
    return result.isEmpty() ? QStringLiteral("未命名病害") : result;
}

QString hnRoadDiseaseImageExporter::imagePath(hnPro::hnProject* project, double dmi) const
{
    const QVector<hnMile>& miles = project->getCurrentMileVector();
    if (miles.isEmpty()) return QString();
    const hnMile* selected = &miles.first();
    int selectedIndex = 0;
    double distance = qAbs(selected->dEnclMile - dmi);
    for (int i = 0; i < miles.size(); i++)
    {
        const hnMile& mile = miles.at(i);
        double distanceToImage = qAbs(mile.dEnclMile - dmi);
        if (distanceToImage < distance) { selected = &mile; selectedIndex = i; distance = distanceToImage; }
    }
    QString path = selected->picturePath;
    if (path.isEmpty()) return QString();
    if (QFileInfo(path).isAbsolute()) return path;
    return QDir(project->get2DProPath()).filePath(QStringLiteral("RoadImg/Camera0/Image_%1/%2")
        .arg(selectedIndex / 1000, 4, 10, QChar('0')).arg(path));
}

bool hnRoadDiseaseImageExporter::exportProject(hnPro::hnProject* project, const QString& root, int& exported,
    int& skipped, QString& error, const std::function<bool(int, int, const QString&)>& progress,
    int& progressIndex, int total) const
{
    if (!project || !project->get2DProject()) return true;
    const QString projectFolder = QDir(root).filePath(safeName(project->get2DProName()));
    if (!QDir().mkpath(projectFolder)) { error = QStringLiteral("无法创建工程图像目录：%1").arg(projectFolder); return false; }
    QVector<hnRoadDiseaseInfo> diseases;
    if (!project->getDB()->getDiseaseTable()->readAllDiseases_Service(project->getCurProSetInfo(), diseases, project->getCurrentMarkVector()))
    {
        error = QStringLiteral("无法读取工程“%1”的路面病害数据。").arg(project->getProjectName());
        return false;
    }
    QXlsx::Document workbook;
    const QStringList headers = QStringList() << QStringLiteral("工程名称") << QStringLiteral("图片文件") << QStringLiteral("病害ID")
        << QStringLiteral("桩号(DMI)") << QStringLiteral("真实桩号") << QStringLiteral("病害名称") << QStringLiteral("病害表")
        << QStringLiteral("长度") << QStringLiteral("宽度") << QStringLiteral("面积") << QStringLiteral("深度")
        << QStringLiteral("病害GPS时间") << QStringLiteral("GPS UTC时间") << QStringLiteral("经度")
        << QStringLiteral("纬度") << QStringLiteral("高程") << QStringLiteral("备注");
    QXlsx::Format header;
    header.setFontBold(true);
    header.setPatternBackgroundColor(QColor(QStringLiteral("#dceef7")));
    for (int col = 0; col < headers.size(); col++) workbook.write(1, col + 1, headers.at(col), header);
    int row = 2;
    for (const hnRoadDiseaseInfo& disease : diseases)
    {
        if (disease.ndiseaseType != 0) continue;
        if (progress && !progress(progressIndex, total, project->getProjectName())) { error = QStringLiteral("用户取消导出。"); return false; }
        progressIndex++;
        const QString source = imagePath(project, disease.dDmi);
        QImage image(source);
        if (image.isNull() || disease.vec2dRect.empty()) { skipped++; continue; }
        QImage canvas = image.convertToFormat(QImage::Format_RGB32);
        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(220, 30, 30), qMax(2, canvas.width() / 300)));
        QRect bounds;
        for (const hn2dRectI& rect : disease.vec2dRect)
            bounds = bounds.isNull() ? QRect(rect.p0.x, rect.p0.y, rect.p2.x - rect.p0.x, rect.p2.y - rect.p0.y) : bounds.united(QRect(rect.p0.x, rect.p0.y, rect.p2.x - rect.p0.x, rect.p2.y - rect.p0.y));
        bounds = bounds.normalized().intersected(canvas.rect());
        painter.drawRect(bounds);
        painter.setPen(Qt::white);
        painter.setBrush(QColor(140, 0, 0, 190));
        QRect label(bounds.left(), qMax(0, bounds.top() - 34), qMin(canvas.width() - bounds.left(), 900), 34);
        painter.drawRect(label);
        painter.setPen(Qt::white);
        painter.drawText(label.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::TextSingleLine,
            QString::fromLocal8Bit(disease.strDisName));
        painter.end();
        const QString fileName = QStringLiteral("%1_%2_%3.jpg").arg(QString::number(disease.dDmi, 'f', 3))
            .arg(disease.nID).arg(safeName(QString::fromLocal8Bit(disease.strDisName)));
        const QString output = QDir(projectFolder).filePath(fileName);
        if (!canvas.save(output, "JPG", 95)) { skipped++; continue; }
        workbook.write(row, 1, project->getProjectName()); workbook.write(row, 2, fileName); workbook.write(row, 3, disease.nID);
        workbook.write(row, 4, disease.dDmi); workbook.write(row, 5, disease.dMileage); workbook.write(row, 6, QString::fromLocal8Bit(disease.strDisName));
        workbook.write(row, 7, QString::fromLocal8Bit(disease.strDiseaseTableName)); workbook.write(row, 8, disease.dLength);
        workbook.write(row, 9, disease.dWidth); workbook.write(row, 10, disease.dArea); workbook.write(row, 11, disease.dDepth);
        QStringList gps; for (double value : disease.vecGpsTimer) gps << QString::number(value, 'f', 3);
        QPoint center = bounds.center();
        const _EXCELGPS_ gpsInfo = project->get2DProject()->findCloseGpsInfoFromDmi(disease.dDmi, center.x(), center.y());
        workbook.write(row, 12, gps.join(QStringLiteral(",")));
        workbook.write(row, 13, gpsInfo._utctime);
        workbook.write(row, 14, gpsInfo._longitude);
        workbook.write(row, 15, gpsInfo._latitude);
        workbook.write(row, 16, gpsInfo._elevation);
        workbook.write(row, 17, QString::fromLocal8Bit(disease.strRemark)); row++; exported++;
    }
    const QString report = QDir(projectFolder).filePath(QStringLiteral("路面病害图像信息.xlsx"));
    if (!workbook.saveAs(report)) { error = QStringLiteral("无法保存病害图像信息表：%1").arg(report); return false; }
    return true;
}

hnRoadDiseaseImageExporter::Result hnRoadDiseaseImageExporter::exportProjects(const std::vector<hnPro::hnProject*>& projects,
    const QString& outputRoot,
    const std::function<bool(int, int, const QString&)>& progress) const
{
    Result result;
    const QString root = outputRoot.isEmpty() ? QDir::currentPath() : outputRoot;
    int total = 0;
    for (hnPro::hnProject* project : projects)
    {
        if (!project || !project->get2DProject() || !project->getDB()) continue;
        QVector<hnRoadDiseaseInfo> diseases;
        if (project->getDB()->getDiseaseTable()->readAllDiseases_Service(project->getCurProSetInfo(), diseases, project->getCurrentMarkVector()))
            total += diseases.size();
    }
    const QString output = QDir(root).filePath(QStringLiteral("路面病害图像"));
    if (!QDir().mkpath(output)) { result.error = QStringLiteral("无法创建输出目录：%1").arg(output); return result; }
    int progressIndex = 0;
    for (hnPro::hnProject* project : projects)
    {
        if (!exportProject(project, output, result.exported, result.skipped, result.error, progress, progressIndex, total)) return result;
    }
    if (progress && !progress(progressIndex, qMax(1, total), QStringLiteral("全部工程")))
    {
        result.error = QStringLiteral("用户取消导出。");
        return result;
    }
    result.success = true; result.outputPath = output; return result;
}

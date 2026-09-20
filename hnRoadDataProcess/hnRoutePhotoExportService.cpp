#include "hnRoutePhotoExportService.h"
#include <QBuffer>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QImageReader>
#include <QPainter>
#include <QRegularExpression>
#include <QSettings>
#include <QSet>
#include <QTemporaryDir>
#include <QTextStream>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include "xlsxdocument.h"
#include "xlsxformat.h"

QString hnRoutePhotoExportService::pile(double mile) const
{
    const qint64 meters = qRound64(std::abs(mile));
    return QStringLiteral("%1K%2+%3").arg(mile < 0 ? QStringLiteral("-") : QString())
        .arg(meters / 1000).arg(meters % 1000, 3, 10, QLatin1Char('0'));
}

bool hnRoutePhotoExportService::readGps(const QString& path, QVector<GpsPoint>& points, hnRoutePhotoObserver* observer)
{
    points.clear();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }
    while (!file.atEnd())
    {
        if (observer && observer->cancelled())
        {
            points.clear();
            return false;
        }
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty())
        {
            continue;
        }
        const QStringList values = line.split(QRegularExpression(QStringLiteral("\\s+")), QString::SkipEmptyParts);
        if (values.size() != 6)
        {
            points.clear();
            return false;
        }
        bool lonOk = false, latOk = false, mileOk = false;
        GpsPoint point;
        point.longitude = values[1].toDouble(&lonOk);
        point.latitude = values[2].toDouble(&latOk);
        point.mile = values[5].toDouble(&mileOk);
        if (!lonOk || !latOk || !mileOk || !std::isfinite(point.longitude)
            || !std::isfinite(point.latitude) || !std::isfinite(point.mile)
            || std::abs(point.longitude) > 180 || std::abs(point.latitude) > 90
            || (point.longitude == 0 && point.latitude == 0))
        {
            points.clear();
            return false;
        }
        points.append(point);
    }
    std::sort(points.begin(), points.end());
    return !points.isEmpty();
}

hnRoutePhotoCheck hnRoutePhotoExportService::check(const QVector<hnRoutePhotoProject>& projects,
    int year, hnRoutePhotoObserver* observer)
{
    hnRoutePhotoCheck result;
    if (projects.isEmpty())
    {
        result.errors.append(QStringLiteral("没有已导入工程。"));
    }
    if (year < 1000 || year > 9999)
    {
        result.errors.append(QStringLiteral("报送年份必须是四位年份。"));
    }
    for (int i = 0; i < projects.size(); ++i)
    {
        if (observer && observer->cancelled())
        {
            result.cancelled = true;
            break;
        }
        if (observer)
        {
            observer->progress(i * 100 / projects.size(), QStringLiteral("检查工程 %1/%2：%3")
                .arg(i + 1).arg(projects.size()).arg(projects[i].name));
        }
        checkProject(projects[i], year, result, observer);
    }
    if (result.photos.size() > 1048575)
    {
        result.errors.append(QStringLiteral("照片数量 %1 超过 Excel 单表上限 1048575，请缩小导出批次。")
            .arg(result.photos.size()));
    }
    return result;
}

void hnRoutePhotoExportService::checkProject(const hnRoutePhotoProject& project, int year,
    hnRoutePhotoCheck& result, hnRoutePhotoObserver* observer)
{
    const QString prefix = QStringLiteral("【%1】%2\n").arg(project.name, project.path);
    if (!QDir::isAbsolutePath(project.path) || !QDir(project.path).exists())
    {
        result.errors.append(prefix + QStringLiteral("工程目录不存在或不是绝对路径。"));
        return;
    }
    if (!QRegularExpression(QStringLiteral("^[A-Za-z0-9]+$")).match(project.roadCode).hasMatch())
    {
        result.errors.append(prefix + QStringLiteral("路线编码为空或含非法字符：%1").arg(project.roadCode));
    }
    if (!project.date.isValid())
    {
        result.errors.append(prefix + QStringLiteral("工程采集日期无效。"));
    }
    else if (project.date.year() != year)
    {
        result.warnings.append(prefix + QStringLiteral("采集年份 %1 与报送年份 %2 不同，请核实历史照片适用性；水印保留真实时间。")
            .arg(project.date.year()).arg(year));
    }
    if (!std::isfinite(project.startMile) || !std::isfinite(project.endMile)
        || project.startMile == project.endMile)
    {
        result.errors.append(prefix + QStringLiteral("工程起终点桩号无效。"));
        return;
    }
    const QString settingsPath = QDir(project.path).filePath(QStringLiteral("Setting.ini"));
    QSettings settings(settingsPath, QSettings::IniFormat);
    settings.setIniCodec("UTF-8");
    // 不使用默认间隔，避免缺项工程被误认为符合要求。
    bool intervalOk = false;
    double interval = 0;
    const QStringList keys = settings.allKeys();
    for (const QString& key : keys)
    {
        if (key.section(QLatin1Char('/'), -1) == QStringLiteral("StreetDis"))
        {
            interval = settings.value(key).toDouble(&intervalOk);
            break;
        }
    }
    if (!intervalOk || interval != 10)
    {
        result.errors.append(prefix + QStringLiteral("%1：StreetDis 必须为 10 米，当前为 %2。")
            .arg(settingsPath, intervalOk ? QString::number(interval) : QStringLiteral("缺失或无效")));
    }
    QVector<GpsPoint> gps;
    const QString highPath = QDir(project.path).filePath(QStringLiteral("HighGps2Mile.txt"));
    const QString gpsPath = QDir(project.path).filePath(QStringLiteral("GPS2Mile.txt"));
    if (!readGps(highPath, gps, observer))
    {
        if (QFileInfo::exists(highPath))
        {
            result.warnings.append(prefix + QStringLiteral("高精度 GPS 无效，尝试普通 GPS：%1").arg(highPath));
        }
        if (!readGps(gpsPath, gps, observer))
        {
            result.errors.append(prefix + QStringLiteral("缺少有效 GPS 桩号匹配文件：%1 或 %2，请先完成 GPS 桩号匹配。")
                .arg(highPath, gpsPath));
        }
    }
    const QString cameraPath = QDir(project.path).filePath(QStringLiteral("StreetImg/Camera0"));
    const QString indexPath = QDir(cameraPath).filePath(QStringLiteral("Street2Mile.txt"));
    QFile index(indexPath);
    if (!index.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        result.errors.append(prefix + QStringLiteral("无法读取景观桩号索引：%1").arg(indexPath));
        return;
    }
    QSet<QString> indexedPaths;
    QDate date = project.date;
    QTime previousTime;
    bool haveMile = false;
    double firstMile = 0, previousMile = 0;
    int row = 0, count = 0;
    const double minimum = qMin(project.startMile, project.endMile);
    const double maximum = qMax(project.startMile, project.endMile);
    const int direction = project.endMile > project.startMile ? 1 : -1;
    const QString canonicalCamera = QFileInfo(cameraPath).canonicalFilePath() + QLatin1Char('/');
    while (!index.atEnd())
    {
        if (observer && observer->cancelled())
        {
            result.cancelled = true;
            return;
        }
        ++row;
        if (observer && row % 25 == 0)
        {
            observer->progress(-1, QStringLiteral("检查 %1：已处理 %2 条照片索引").arg(project.name).arg(row));
        }
        const QString line = QString::fromUtf8(index.readLine()).trimmed();
        if (line.isEmpty())
        {
            continue;
        }
        const QRegularExpressionMatch match = QRegularExpression(QStringLiteral("^(\\S+)\\s+(.+)$")).match(line);
        bool mileOk = false;
        const double mile = match.captured(1).toDouble(&mileOk);
        if (!match.hasMatch() || !mileOk || !std::isfinite(mile))
        {
            result.errors.append(prefix + QStringLiteral("%1 第 %2 行格式或桩号无效。").arg(indexPath).arg(row));
            continue;
        }
        QString relative = match.captured(2);
        relative.replace(QLatin1Char('\\'), QLatin1Char('/'));
        if (relative.startsWith(QLatin1Char('/')))
        {
            relative.remove(0, 1);
        }
        const QString source = QDir::cleanPath(QDir(cameraPath).filePath(relative));
        const QString canonical = QFileInfo(source).canonicalFilePath();
        if (QDir::isAbsolutePath(relative) || relative.split(QLatin1Char('/')).contains(QStringLiteral(".."))
            || !canonical.startsWith(canonicalCamera, Qt::CaseInsensitive) || canonical.isEmpty())
        {
            result.errors.append(prefix + QStringLiteral("索引图片缺失或超出第一路相机目录：%1").arg(source));
            continue;
        }
        const QString pathKey = canonical.toLower();
        if (indexedPaths.contains(pathKey))
        {
            result.errors.append(prefix + QStringLiteral("照片重复索引：%1").arg(source));
            continue;
        }
        indexedPaths.insert(pathKey);
        const QRegularExpressionMatch timeMatch = QRegularExpression(QStringLiteral("_([0-9]{9})\\.[jJ][pP][gG]$"))
            .match(QFileInfo(source).fileName());
        const QTime time = QTime::fromString(timeMatch.captured(1), QStringLiteral("HHmmsszzz"));
        if (!time.isValid())
        {
            result.errors.append(prefix + QStringLiteral("无法解析照片拍摄时间：%1").arg(source));
        }
        else if (previousTime.isValid() && time < previousTime)
        {
            // 仅把跨越半天的回退解释为午夜；普通时间倒序不悄悄改日期。
            if (time.msecsTo(previousTime) > 12 * 60 * 60 * 1000)
            {
                date = date.addDays(1);
            }
            else
            {
                result.errors.append(prefix + QStringLiteral("照片采集时间倒序：%1").arg(source));
            }
        }
        previousTime = time;
        if (mile < minimum || mile > maximum)
        {
            continue;
        }
        ++count;
        if (!haveMile)
        {
            firstMile = mile;
            haveMile = true;
        }
        else
        {
            const double gap = (mile - previousMile) * direction;
            if (gap <= 0 || gap > 10.000001)
            {
                result.errors.append(prefix + QStringLiteral("照片桩号间隔异常：%1 → %2，间隔 %3 米（须顺向且不超过 10 米）。")
                    .arg(pile(previousMile), pile(mile)).arg(gap, 0, 'f', 3));
            }
        }
        previousMile = mile;
        QImageReader reader(source);
        const QImage image = reader.read();
        if (image.isNull() || image.width() < 2048 || image.height() < 1280)
        {
            result.errors.append(prefix + QStringLiteral("照片损坏或尺寸不足 2048×1280：%1（%2×%3）")
                .arg(source).arg(image.width()).arg(image.height()));
        }
        if (gps.isEmpty())
        {
            continue;
        }
        if (mile < gps.first().mile || mile > gps.last().mile)
        {
            result.errors.append(prefix + QStringLiteral("%1 超出 GPS 桩号覆盖范围：%2").arg(pile(mile), source));
            continue;
        }
        GpsPoint target;
        target.mile = mile;
        auto position = std::lower_bound(gps.constBegin(), gps.constEnd(), target);
        if (position != gps.constBegin() && (position == gps.constEnd()
            || mile - (position - 1)->mile <= position->mile - mile))
        {
            --position;
        }
        hnRoutePhotoRecord photo;
        photo.source = source;
        photo.roadCode = project.roadCode;
        photo.mile = mile;
        photo.longitude = position->longitude;
        photo.latitude = position->latitude;
        photo.captured = QDateTime(date, time);
        result.photos.append(photo);
    }
    if (!count)
    {
        result.errors.append(prefix + QStringLiteral("工程范围内没有可用的第一路景观照片：%1").arg(indexPath));
    }
    else if (std::abs(firstMile - project.startMile) > 10.000001
        || std::abs(previousMile - project.endMile) > 10.000001)
    {
        result.errors.append(prefix + QStringLiteral("首尾覆盖缺口超过 10 米：工程 %1 → %2，照片 %3 → %4。")
            .arg(pile(project.startMile), pile(project.endMile), pile(firstMile), pile(previousMile)));
    }
    QDirIterator files(cameraPath, QStringList() << QStringLiteral("*.jpg"), QDir::Files, QDirIterator::Subdirectories);
    while (files.hasNext())
    {
        if (observer && observer->cancelled())
        {
            result.cancelled = true;
            return;
        }
        const QString file = files.next();
        if (!indexedPaths.contains(QFileInfo(file).canonicalFilePath().toLower()))
        {
            result.errors.append(prefix + QStringLiteral("照片没有桩号索引：%1").arg(file));
        }
    }
}

bool hnRoutePhotoExportService::savePhoto(const hnRoutePhotoRecord& photo, const QString& path, QString& error)
{
    QImageReader reader(photo.source);
    QImage image = reader.read().convertToFormat(QImage::Format_RGB32);
    if (image.isNull() || image.width() < 2048 || image.height() < 1280)
    {
        error = QStringLiteral("照片读取失败或尺寸不足：%1").arg(photo.source);
        return false;
    }
    image.setDotsPerMeterX(3780);
    image.setDotsPerMeterY(3780);
    QPainter painter(&image);
    QFont font(QStringLiteral("SimSun"));
    font.setPointSizeF(16.0);
    painter.setFont(font);
    const QStringList lines = QStringList()
        << QStringLiteral("编码：%1").arg(photo.roadCode)
        << QStringLiteral("桩号：%1").arg(pile(photo.mile))
        << QStringLiteral("坐标：%1,%2").arg(photo.longitude, 0, 'f', 6).arg(photo.latitude, 0, 'f', 6)
        << QStringLiteral("时间：%1").arg(photo.captured.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    const QFontMetrics metrics(font, &image);
    int width = 0;
    for (const QString& line : lines)
    {
        width = qMax(width, metrics.width(line));
    }
    const int margin = 12;
    const int lineHeight = metrics.height() + 4;
    const QRect box(margin, image.height() - margin - lineHeight * 4 - 16, width + 20, lineHeight * 4 + 16);
    painter.fillRect(box, QColor(0, 0, 0, 128));
    painter.setPen(Qt::white);
    for (int i = 0; i < lines.size(); ++i)
    {
        painter.drawText(box.left() + 10, box.top() + 8 + metrics.ascent() + i * lineHeight, lines[i]);
    }
    painter.end();
    for (int quality = 95; quality >= 5; quality -= 5)
    {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        if (!image.save(&buffer, "JPG", quality))
        {
            break;
        }
        if (bytes.size() <= 2000000)
        {
            QFile file(path);
            if (file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.flush())
            {
                return true;
            }
            error = QStringLiteral("写入照片失败：%1，%2").arg(path, file.errorString());
            return false;
        }
    }
    error = QStringLiteral("无法将照片编码为不超过 2,000,000 字节的 JPG：%1").arg(photo.source);
    return false;
}

bool hnRoutePhotoExportService::exportPhotos(const hnRoutePhotoCheck& checked, int year,
    const QString& destination, QString& outputPath, QString& error, hnRoutePhotoObserver* observer)
{
    outputPath.clear();
    error.clear();
    if (checked.cancelled || !checked.errors.isEmpty() || checked.photos.isEmpty()
        || checked.photos.size() > 1048575 || year < 1000 || year > 9999)
    {
        error = QStringLiteral("批量检查未通过，未导出任何工程。");
        return false;
    }
    if (!QDir::isAbsolutePath(destination) || !QDir(destination).exists())
    {
        error = QStringLiteral("输出目录不存在或不是绝对路径：%1").arg(destination);
        return false;
    }
    QTemporaryDir staging(QDir(destination).filePath(QStringLiteral(".route-photo-XXXXXX")));
    if (!staging.isValid())
    {
        error = QStringLiteral("无法创建临时输出目录：%1").arg(destination);
        return false;
    }
    QXlsx::Document workbook;
    QXlsx::Format kmFormat, gpsFormat;
    kmFormat.setNumberFormat(QStringLiteral("0.000"));
    gpsFormat.setNumberFormat(QStringLiteral("0.000000"));
    const QStringList headers = QStringList() << QStringLiteral("路线编码") << QStringLiteral("所在桩号")
        << QStringLiteral("经度") << QStringLiteral("纬度") << QStringLiteral("照片路径");
    for (int column = 0; column < headers.size(); ++column)
    {
        workbook.write(1, column + 1, headers[column]);
    }
    workbook.setColumnWidth(1, 4, 18);
    workbook.setColumnWidth(5, 5, 85);
    for (int i = 0; i < checked.photos.size(); ++i)
    {
        if (observer && observer->cancelled())
        {
            error = QStringLiteral("已取消导出，未生成正式报送包。");
            return false;
        }
        const hnRoutePhotoRecord& photo = checked.photos[i];
        const QString relative = QStringLiteral("line/%1/%2/%3.jpg").arg(photo.roadCode)
            .arg(year).arg(QUuid::createUuid().toString().mid(1, 36));
        const QString target = QDir(staging.path()).filePath(relative);
        if (!QDir().mkpath(QFileInfo(target).absolutePath()))
        {
            error = QStringLiteral("无法创建照片目录：%1").arg(QFileInfo(target).absolutePath());
            return false;
        }
        if (!savePhoto(photo, target, error))
        {
            return false;
        }
        const int row = i + 2;
        if (!workbook.write(row, 1, photo.roadCode) || !workbook.write(row, 2, photo.mile / 1000.0, kmFormat)
            || !workbook.write(row, 3, photo.longitude, gpsFormat) || !workbook.write(row, 4, photo.latitude, gpsFormat)
            || !workbook.write(row, 5, relative))
        {
            error = QStringLiteral("写入照片索引失败，第 %1 行。").arg(row);
            return false;
        }
        if (observer)
        {
            observer->progress((i + 1) * 99 / checked.photos.size(), QStringLiteral("导出照片 %1/%2")
                .arg(i + 1).arg(checked.photos.size()));
        }
    }
    if (observer && observer->cancelled())
    {
        error = QStringLiteral("已取消导出，未生成正式报送包。");
        return false;
    }
    if (!workbook.saveAs(QDir(staging.path()).filePath(QStringLiteral("照片索引.xlsx"))))
    {
        error = QStringLiteral("保存照片索引.xlsx 失败。");
        return false;
    }
    if (observer && observer->cancelled())
    {
        error = QStringLiteral("已取消导出，未生成正式报送包。");
        return false;
    }
    const QString finalPath = QDir(destination).filePath(QStringLiteral("路线实景照片报送_%1_%2")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")),
            QUuid::createUuid().toString().mid(1, 8)));
    if (!QDir().rename(staging.path(), finalPath))
    {
        error = QStringLiteral("临时目录转为正式报送包失败：%1").arg(finalPath);
        return false;
    }
    staging.setAutoRemove(false);
    outputPath = finalPath;
    return true;
}

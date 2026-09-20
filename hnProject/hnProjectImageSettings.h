#pragma once
#include <QSettings>
#include <QDir>

// 复用工程已有的图像显示配置，不写入原始图片或采集参数。
class hnProjectImageSettings
{
public:
    enum { MinBrightness = -100, MaxBrightness = 300 };

    QString filePath(const QString& projectPath) const
    {
        return QDir(projectPath).filePath(QStringLiteral("mirroredSetting.ini"));
    }

    int streetBrightness(const QString& projectPath) const
    {
        if (projectPath.isEmpty())
        {
            return 0;
        }
        QSettings settings(filePath(projectPath), QSettings::IniFormat);
        bool valid = false;
        const int value = settings.value(QStringLiteral("StreetView/Brightness"), 0).toInt(&valid);
        return valid ? qBound(int(MinBrightness), value, int(MaxBrightness)) : 0;
    }

    bool saveStreetBrightness(const QString& projectPath, int value) const
    {
        if (projectPath.isEmpty() || !QDir(projectPath).exists())
        {
            return false;
        }
        QSettings settings(filePath(projectPath), QSettings::IniFormat);
        settings.setValue(QStringLiteral("StreetView/Brightness"),
            qBound(int(MinBrightness), value, int(MaxBrightness)));
        settings.sync();
        return settings.status() == QSettings::NoError;
    }
};

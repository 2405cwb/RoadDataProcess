#pragma once

#include <QDateTime>
#include <QStringList>
#include <QVector>

// 主线程复制工程元数据；后台不持有工程、界面或数据库对象。
struct hnRoutePhotoProject
{
    QString name;
    QString path;
    QString roadCode;
    QDate date;
    double startMile = 0;
    double endMile = 0;
};

struct hnRoutePhotoRecord
{
    QString source;
    QString roadCode;
    double mile = 0;
    double longitude = 0;
    double latitude = 0;
    QDateTime captured;
};

struct hnRoutePhotoCheck
{
    QVector<hnRoutePhotoRecord> photos;
    QStringList errors;
    QStringList warnings;
    bool cancelled = false;
};

// 后台进度与取消接口；同步测试可不传观察者。
class hnRoutePhotoObserver
{
public:
    virtual ~hnRoutePhotoObserver() {}
    virtual bool cancelled() const = 0;
    virtual void progress(int value, const QString& message) = 0;
};

// 只读检查客户数据，导出仅写入调用者指定的独立目录。
class hnRoutePhotoExportService
{
public:
    hnRoutePhotoCheck check(const QVector<hnRoutePhotoProject>& projects, int year,
        hnRoutePhotoObserver* observer = nullptr);
    bool exportPhotos(const hnRoutePhotoCheck& checked, int year, const QString& destination,
        QString& outputPath, QString& error, hnRoutePhotoObserver* observer = nullptr);

private:
    struct GpsPoint
    {
        double mile;
        double longitude;
        double latitude;
        bool operator<(const GpsPoint& other) const
        {
            return mile < other.mile;
        }
    };
    void checkProject(const hnRoutePhotoProject& project, int year, hnRoutePhotoCheck& result,
        hnRoutePhotoObserver* observer);
    bool readGps(const QString& path, QVector<GpsPoint>& points, hnRoutePhotoObserver* observer);
    QString pile(double mile) const;
    bool savePhoto(const hnRoutePhotoRecord& photo, const QString& path, QString& error);
};

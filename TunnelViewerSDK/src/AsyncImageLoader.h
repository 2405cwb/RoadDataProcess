#pragma once

#include <functional>
#include <QCache>
#include <QMutex>
#include <QObject>
#include <QPixmap>
#include <QThreadPool>

class AsyncImageLoader : public QObject
{
    Q_OBJECT
public:
    /* 全局共用一个 Loader 避免每个图层重复开线程池 */ static AsyncImageLoader* instance();
    // Display-only adjustments used by the legacy database-tile pipeline.
    void setImageAdjustments(int brightness, int contrast, int sharpen);
    // 兼容 SDK0831 的旧接口：只修改亮度，不会把已设置的对比度/锐化重置掉。
    void setBrightness(int value) { m_brightness = qBound(-100, value, 100); }
    int brightness() const { return m_brightness; }
    /* 只查缩略图缓存 不碰磁盘 所以绘制时调用也不卡 */ QPixmap getSyncThumbnail(const QString& pathUri);
    /* 异步加载缩略图 pathUri 可以是 db 也可以是图片路径 */ void requestThumbnail(const QString& pathUri, const QSize& targetSize = QSize());
    /* 异步加载高清图 支持 db|col|row 和普通图片路径 */ void requestImage(const QString& path, bool useSharedCache = true);

    /* 第二阶段：统一按内存上限配置旧数据库瓦片/缩略图缓存。 */
    void setCacheMemoryLimits(qint64 imageBytes, qint64 thumbnailBytes);
    void setThumbnailThreadCount(int count);
    void clearCaches();
    qint64 imageCacheBytesApprox() const;
    qint64 thumbnailCacheBytesApprox() const;

signals:
    /* 缩略图加载完成 path 原样带回 方便图层核对 */ void sigThumbnailLoaded(const QString& path, QPixmap pix);
    /* 高清图加载完成 path 原样带回 pending 表要靠它核销 */ void sigImageLoaded(const QString& path, QPixmap pix);

private:
    /* 单例构造函数 外部不要自己 new */ AsyncImageLoader();

private:
    QCache<QString, QPixmap> m_cache;
    mutable QMutex m_mutex;
    int m_brightness = 0;
    int m_contrast = 100;
    int m_sharpen = 0;
    QCache<QString, QPixmap> m_thumbnailCache;
    QThreadPool m_thumbThreadPool;
};

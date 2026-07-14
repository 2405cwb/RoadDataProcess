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
    /* 设置亮度偏移 正数变亮 负数变暗 */ void setBrightness(int value);
    /* 读取当前亮度值 给外部状态栏或调试用 */ int brightness() const;
    /* 只查缩略图缓存 不碰磁盘 所以绘制时调用也不卡 */ QPixmap getSyncThumbnail(const QString& pathUri);
    /* 异步加载缩略图 pathUri 可以是 db 也可以是图片路径 */ void requestThumbnail(const QString& pathUri, const QSize& targetSize = QSize());
    /* 异步加载高清图 支持 db|col|row 和普通图片路径 */ void requestImage(const QString& path, bool useSharedCache = true);

signals:
    /* 缩略图加载完成 path 原样带回 方便图层核对 */ void sigThumbnailLoaded(const QString& path, QPixmap pix);
    /* 高清图加载完成 path 原样带回 pending 表要靠它核销 */ void sigImageLoaded(const QString& path, QPixmap pix);

private:
    /* 单例构造函数 外部不要自己 new */ AsyncImageLoader();

private:
    QCache<QString, QPixmap> m_cache;
    QMutex m_mutex;
    int m_brightness = 0;
    QCache<QString, QPixmap> m_thumbnailCache;
    QThreadPool m_thumbThreadPool;
};

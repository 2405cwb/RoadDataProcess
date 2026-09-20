#pragma once

#include <QCache>
#include <QImage>
#include <QString>
#include <QtGlobal>
#include <climits>

/*
 * 图像解码缓存统一封装。
 *
 * QCache 的 cost 是 int，本类统一约定 cost 单位为 KiB，对外则使用字节数配置，
 * 避免业务层继续按“缓存多少张图片”估算内存。当前虚拟序列直接使用本类；
 * 数据库瓦片仍保留旧 AsyncImageLoader，后续可逐步接入同一配置入口。
 */
class ImageCacheManager
{
public:
    ImageCacheManager() { setMemoryLimitBytes(256LL * 1024LL * 1024LL); }

    void setMemoryLimitBytes(qint64 bytes)
    {
        m_memoryLimitBytes = qMax<qint64>(1LL * 1024LL * 1024LL, bytes);
        m_cache.setMaxCost(static_cast<int>(qMin<qint64>(INT_MAX, (m_memoryLimitBytes + 1023LL) / 1024LL)));
    }

    qint64 memoryLimitBytes() const { return m_memoryLimitBytes; }
    qint64 usedBytesApprox() const { return static_cast<qint64>(m_cache.totalCost()) * 1024LL; }

    bool contains(const QString& key) const { return m_cache.contains(key); }
    QImage* object(const QString& key) { return m_cache.object(key); }
    const QImage* object(const QString& key) const { return m_cache.object(key); }

    bool insert(const QString& key, const QImage& image)
    {
        const qint64 bytes = qMax<qint64>(1, static_cast<qint64>(image.bytesPerLine()) * image.height());
        return insert(key, new QImage(image), static_cast<int>(qMin<qint64>(INT_MAX, (bytes + 1023LL) / 1024LL)));
    }

    /* 兼容原 QCache 调用形式，cost 继续以 KiB 为单位。 */
    bool insert(const QString& key, QImage* image, int costKiB)
    {
        return m_cache.insert(key, image, qMax(1, costKiB));
    }

    void clear() { m_cache.clear(); }

    /* 下面两个接口仅用于兼容现有 View 内部代码，建议新代码使用字节接口。 */
    void setMaxCost(int costKiB)
    {
        m_memoryLimitBytes = qMax<qint64>(1024LL, static_cast<qint64>(costKiB) * 1024LL);
        m_cache.setMaxCost(qMax(1, costKiB));
    }
    int totalCost() const { return m_cache.totalCost(); }

private:
    QCache<QString, QImage> m_cache;
    qint64 m_memoryLimitBytes = 256LL * 1024LL * 1024LL;
};

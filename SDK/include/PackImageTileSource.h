#ifndef PACK_IMAGE_TILE_SOURCE_H
#define PACK_IMAGE_TILE_SOURCE_H

#include "AbstractTileSource.h"

#include <QByteArray>
#include <QFileInfo>
#include <QUrl>

#include "PackReaderQt.h"

class PackImageTileSource : public AbstractTileSource
{
public:
    explicit PackImageTileSource(const QString& packRoot,
        quint64 globalIndex,
        quint64 sourceIndex,
        quint64 timeValue,
        const QSize& imageSize,
        const QString& imageName,
        int virtualTileSize = 0,
        bool hMirrored = false,
        bool vMirrored = false)
        : m_packRoot(QFileInfo(packRoot).absoluteFilePath()),
          m_globalIndex(globalIndex),
          m_sourceIndex(sourceIndex),
          m_timeValue(timeValue),
          m_size(imageSize),
          m_imageName(imageName),
          m_virtualTileSize(virtualTileSize),
          m_hMirrored(hMirrored),
          m_vMirrored(vMirrored)
    {
    }

    bool isValid() const override {
        return m_size.isValid() && QFileInfo(m_packRoot + "/PackIndex.idx").exists();
    }

    QString oriImageName() const override {
        return m_imageName.isEmpty() ? QString::number(m_sourceIndex) : m_imageName;
    }

    QSize totalSize() const override {
        return m_size;
    }

    int tileSize() const override {
        return m_virtualTileSize > 0 ? m_virtualTileSize : qMax(m_size.width(), m_size.height());
    }

    QString getDbPath() const override {
        return QString();
    }

    QString getThumbnailImage() const override {
        return tileRequestKey(0, 0);
    }

    ImageSourceMode sourceMode() const override {
        return ImageSourceMode::WholeImage;
    }

    bool isHMirrored() const override {
        return m_hMirrored;
    }

    bool isVMirrored() const override {
        return m_vMirrored;
    }

    QString cacheKey() const override {
        return QString("%1#%2").arg(m_packRoot).arg(m_globalIndex);
    }

    QString thumbnailCacheKey() const override {
        return tileRequestKey(0, 0);
    }

    QString tileRequestKey(int col, int row) const override {
        return hasTile(col, row) ? makePackFrameUri(m_packRoot, m_globalIndex) : QString();
    }

    bool hasTile(int col, int row) const override {
        return col == 0 && row == 0;
    }

    QByteArray tileData(int col, int row) const override {
        if (!hasTile(col, row)) return QByteArray();
        if (!QFileInfo(m_packRoot + "/PackIndex.idx").exists()) return QByteArray();
        try {
            PackReaderQt reader(m_packRoot);
            return reader.readJpeg(m_globalIndex);
        }
        catch (...) {
            return QByteArray();
        }
    }

    QList<TileImageData> tileDataRange(int startCol, int endCol, int startRow, int endRow) const override {
        QList<TileImageData> result;
        if (startCol <= 0 && endCol >= 0 && startRow <= 0 && endRow >= 0) {
            TileImageData data;
            data.col = 0;
            data.row = 0;
            data.data = tileData(0, 0);
            if (!data.data.isEmpty()) result.append(data);
        }
        return result;
    }

    QByteArray thumbnailData() const override {
        return tileData(0, 0);
    }

    QImage tileImage(int col, int row) const override {
        if (!hasTile(col, row)) return QImage();
        return QImage::fromData(tileData(col, row), "JPG");
    }

    QImage thumbnailImage() const override {
        return tileImage(0, 0);
    }

    static QString makePackFrameUri(const QString& packRoot, quint64 globalIndex) {
        const QString root = QFileInfo(packRoot).absoluteFilePath();
        return QString("packv2://%1/%2")
            .arg(QString::fromLatin1(QUrl::toPercentEncoding(root)))
            .arg(globalIndex);
    }

    static bool parsePackFrameUri(const QString& uri, QString* packRoot, quint64* globalIndex) {
        const QString prefix = QStringLiteral("packv2://");
        if (!uri.startsWith(prefix)) return false;

        const QString payload = uri.mid(prefix.size());
        const int split = payload.lastIndexOf('/');
        if (split <= 0 || split >= payload.size() - 1) return false;

        bool ok = false;
        const quint64 index = payload.mid(split + 1).toULongLong(&ok);
        if (!ok) return false;

        if (packRoot) {
            const QByteArray encodedRoot = payload.left(split).toLatin1();
            *packRoot = QUrl::fromPercentEncoding(encodedRoot);
        }
        if (globalIndex) {
            *globalIndex = index;
        }
        return true;
    }

private:
    QString m_packRoot;
    quint64 m_globalIndex = 0;
    quint64 m_sourceIndex = 0;
    quint64 m_timeValue = 0;
    QSize m_size;
    QString m_imageName;
    int m_virtualTileSize = 0;
    bool m_hMirrored = false;
    bool m_vMirrored = false;
};

#endif

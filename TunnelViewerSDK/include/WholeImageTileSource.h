#ifndef WHOLE_IMAGE_TILE_SOURCE_H
#define WHOLE_IMAGE_TILE_SOURCE_H

#include "AbstractTileSource.h"

#include <QFile>
#include <QFileInfo>
#include <QImageReader>

class WholeImageTileSource : public AbstractTileSource
{
public:
	/* 传入一张普通图片路径 就能作为一个连续段使用 */ explicit WholeImageTileSource(const QString& imagePath,
		const QString& displayName = QString(),
		int virtualTileSize = 0,
		bool hMirrored = false,
		bool vMirrored = false)
		: m_imagePath(QFileInfo(imagePath).absoluteFilePath()),
		  m_displayName(displayName),
		  m_virtualTileSize(virtualTileSize),
		  m_hMirrored(hMirrored),
		  m_vMirrored(vMirrored)
	{
		loadImageInfo();
	}

	/* 图片存在并且能读到尺寸 就认为有效 */ bool isValid() const override {
		return m_valid;
	}

	/* 返回给外部看的图片名 */ QString oriImageName() const override {
		if (!m_displayName.isEmpty()) return m_displayName;
		return QFileInfo(m_imagePath).completeBaseName();
	}

	/* 整图模式的场景尺寸就是原图尺寸 */ QSize totalSize() const override {
		return m_size;
	}

	/* 给整图一个虚拟 tileSize 让现有 LOD 调度能复用 */ int tileSize() const override {
		return m_virtualTileSize > 0 ? m_virtualTileSize : qMax(m_size.width(), m_size.height());
	}

	/* 整图源不是数据库 老接口返回空 */ QString getDbPath() const override {
		return QString();
	}

	/* 缩略图直接用原图路径 Loader 会按目标尺寸压缩 */ QString getThumbnailImage() const override {
		return m_imagePath;
	}

	/* 标记这是整图源 */ ImageSourceMode sourceMode() const override {
		return ImageSourceMode::WholeImage;
	}

	/* 整图源由视图设置决定是否镜像显示。 */ bool isHMirrored() const override {
		return m_hMirrored;
	}

	bool isVMirrored() const override {
		return m_vMirrored;
	}

	/* 用绝对路径做缓存 key 简单也稳定 */ QString cacheKey() const override {
		return m_imagePath;
	}

	/* 整图只有一个高清块 请求 key 就是图片路径 */ QString tileRequestKey(int col, int row) const override {
		return hasTile(col, row) ? m_imagePath : QString();
	}

	/* 整图源只认 0 0 这一块 */ bool hasTile(int col, int row) const override {
		return col == 0 && row == 0;
	}

	/* 导出时直接读取原图字节 */ QByteArray tileData(int col, int row) const override {
		if (!hasTile(col, row)) return QByteArray();
		QFile file(m_imagePath);
		if (!file.open(QIODevice::ReadOnly)) return QByteArray();
		return file.readAll();
	}

	/* 区域覆盖到整图时 返回唯一的 0 0 块 */ QList<TileImageData> tileDataRange(int startCol, int endCol, int startRow, int endRow) const override {
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

	/* 缩略图数据也来自原图 */ QByteArray thumbnailData() const override {
		return tileData(0, 0);
	}

	/* 同步解码整图 导出时会用到 */ QImage tileImage(int col, int row) const override {
		if (!hasTile(col, row)) return QImage();
		return QImage(m_imagePath);
	}

	/* 同步解码缩略图 由调用方决定是否缩放 */ QImage thumbnailImage() const override {
		return QImage(m_imagePath);
	}

private:
	/* 构造时读一次尺寸 后面渲染不用反复碰磁盘 */ void loadImageInfo() {
		QImageReader reader(m_imagePath);
		m_size = reader.size();
		m_valid = QFileInfo::exists(m_imagePath) && m_size.isValid();
	}

private:
	QString m_imagePath;
	QString m_displayName;
	QSize m_size;
	int m_virtualTileSize = 0;
	bool m_hMirrored = false;
	bool m_vMirrored = false;
	bool m_valid = false;
};

#endif // WHOLE_IMAGE_TILE_SOURCE_H

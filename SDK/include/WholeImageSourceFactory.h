#ifndef WHOLE_IMAGE_SOURCE_FACTORY_H
#define WHOLE_IMAGE_SOURCE_FACTORY_H

#include "AbstractSourceFactory.h"
#include "WholeImageTileSource.h"

class WholeImageSourceFactory : public AbstractSourceFactory
{
public:
	/* 构造时就把拼接方向和滚动步长定下来。路面图片默认按纵向连续拼接。 */ explicit WholeImageSourceFactory(LayoutOrientation orientation = LayoutOrientation::Vertical,
		int scrollSpeed = 50,
		int virtualTileSize = 0,
		bool hMirrored = false,
		bool vMirrored = false)
		: m_orientation(orientation),
		  m_scrollSpeed(scrollSpeed),
		  m_virtualTileSize(virtualTileSize),
		  m_hMirrored(hMirrored),
		  m_vMirrored(vMirrored) {
	}

	/* 把普通图片路径包装成 WholeImageTileSource */ AbstractTileSource* create(const QString& filePath) override {
		return new WholeImageTileSource(filePath, QString(), m_virtualTileSize, m_hMirrored, m_vMirrored);
	}

	/* 默认扫描常见工程图片格式 */ QStringList sourceFileFilters() const override {
		return QStringList()
			<< "*.jpg" << "*.jpeg" << "*.png"
			<< "*.bmp" << "*.tif" << "*.tiff";
	}

	/* 返回整图序列的拼接方向 */ LayoutOrientation layoutOrientation() const override {
		return m_orientation;
	}

	/* 返回整图序列的滚动步长 */ int scrollSpeed() const override {
		return m_scrollSpeed;
	}

	bool horizontalMirror() const override { return m_hMirrored; }
	bool verticalMirror() const override { return m_vMirrored; }

private:
	LayoutOrientation m_orientation;
	int m_scrollSpeed = 50;
	int m_virtualTileSize = 0;
	bool m_hMirrored = false;
	bool m_vMirrored = false;
};

#endif // WHOLE_IMAGE_SOURCE_FACTORY_H

#ifndef ABSTRAC_TSOURCE_FACTORY_H
#define ABSTRAC_TSOURCE_FACTORY_H

#include <AbstractTileSource.h>
#include <QCollator>
#include <QStringList>

#include "TunnelGlobal.h"

class AbstractSourceFactory {
public:
	virtual ~AbstractSourceFactory() {}

	/* 根据文件路径创建数据源 路径可以是 db 也可以是普通图片 */ virtual AbstractTileSource* create(const QString& filePath) = 0;

	/* 告诉 Controller 扫哪些文件 老项目默认只扫 db */ virtual QStringList sourceFileFilters() const {
		return QStringList() << "*.db";
	}

	/* 默认自然排序 例如 2 会排在 10 前面 */ virtual void sortDatabaseFolder(QList<DbImageInfo>& names) {
		QCollator collator;
		collator.setNumericMode(true);
		std::sort(names.begin(), names.end(), [&collator](const DbImageInfo& a, const DbImageInfo& b) {
			return collator.compare(a.originalName, b.originalName) < 0;
		});
	}

	/* 返回图片序列拼接方向 */ virtual LayoutOrientation layoutOrientation() const {
		return LayoutOrientation::Vertical;
	}

	/* 返回浏览步长 */ virtual int scrollSpeed() const {
		return 50;
	}
	virtual bool horizontalMirror() const { return false; }
	virtual bool verticalMirror() const { return false; }
};

#endif // !ABSTRAC_TSOURCE_FACTORY_H

#ifndef ABSTRACTTILESOURCE_H
#define ABSTRACTTILESOURCE_H

#include <QByteArray>
#include <QHash>
#include <QImage>
#include <QList>
#include <QSize>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QThread>

#include "TunnelGlobal.h"

struct TileImageData {
	int col;
	int row;
	QByteArray data;
};

/* 数据源粗分类 渲染层只拿它做提示 */ enum class ImageSourceMode {
	TileDatabase,
	WholeImage,
	Custom
};

class AbstractTileSource
{
public:
	/* 子类通常由 TunnelSectionItem 接管生命周期 */ virtual ~AbstractTileSource() {}
	/* 数据源能不能用 */ virtual bool isValid() const = 0;
	/* 给外部看的原始图片名 */ virtual QString oriImageName() const = 0;
	/* 当前段完整像素尺寸 */ virtual QSize totalSize() const = 0;
	/* 网格调度用的 tileSize */ virtual int tileSize() const = 0;
	/* 老 SQLite 接口 非数据库源可以返回空 */ virtual QString getDbPath() const = 0;
	/* 缩略图请求入口 db 源通常返回 db 路径 整图源返回图片路径 */ virtual QString getThumbnailImage() const = 0;

	/* 返回数据源类型 */ virtual ImageSourceMode sourceMode() const {
		return ImageSourceMode::TileDatabase;
	}

	/* 当前段图片是否按视图设置做水平镜像显示。 */
	virtual bool isHMirrored() const {
		return false;
	}

	/* 当前段图片是否按视图设置做垂直镜像显示。 */
	virtual bool isVMirrored() const {
		return false;
	}

	/* 基础缓存 key */ virtual QString cacheKey() const {
		return getDbPath();
	}

	/* 缩略图缓存 key 默认沿用缩略图入口 */ virtual QString thumbnailCacheKey() const {
		return getThumbnailImage();
	}

	/* 高清块缓存 key 默认兼容 db|col|row */ virtual QString tileCacheKey(int col, int row) const {
		return QString("%1|%2|%3").arg(cacheKey()).arg(col).arg(row);
	}

	/* 真正发给异步 Loader 的请求 key */ virtual QString tileRequestKey(int col, int row) const {
		return tileCacheKey(col, row);
	}

	/* 判断某个网格位置有没有图 */ virtual bool hasTile(int col, int row) const {
		Q_UNUSED(col);
		Q_UNUSED(row);
		return true;
	}

	/* 读取单块原始字节 默认查 SQLite tiles 表 */ virtual QByteArray tileData(int col, int row) const {
		QSqlDatabase db = databaseForCurrentThread();
		if (!db.isOpen()) return QByteArray();

		QSqlQuery query(db);
		query.prepare("SELECT data FROM tiles WHERE col = ? AND row = ?");
		query.addBindValue(col);
		query.addBindValue(row);
		if (query.exec() && query.next()) {
			return query.value(0).toByteArray();
		}
		return QByteArray();
	}

	/* 批量读取网格 导出高清图会用到 */ virtual QList<TileImageData> tileDataRange(int startCol, int endCol, int startRow, int endRow) const {
		QList<TileImageData> result;
		QSqlDatabase db = databaseForCurrentThread();
		if (!db.isOpen()) return result;

		QSqlQuery query(db);
		query.prepare("SELECT col, row, data FROM tiles WHERE col >= ? AND col <= ? AND row >= ? AND row <= ?");
		query.bindValue(0, startCol);
		query.bindValue(1, endCol);
		query.bindValue(2, startRow);
		query.bindValue(3, endRow);
		if (query.exec()) {
			while (query.next()) {
				TileImageData tile;
				tile.col = query.value(0).toInt();
				tile.row = query.value(1).toInt();
				tile.data = query.value(2).toByteArray();
				result.append(tile);
			}
		}
		return result;
	}

	/* 读取缩略图字节 默认查 SQLite thumbnail 表 */ virtual QByteArray thumbnailData() const {
		QSqlDatabase db = databaseForCurrentThread();
		if (!db.isOpen()) return QByteArray();

		QSqlQuery query(db);
		if (query.exec("SELECT data FROM thumbnail LIMIT 1") && query.next()) {
			return query.value(0).toByteArray();
		}
		return QByteArray();
	}

	/* 同步解码单块图 */ virtual QImage tileImage(int col, int row) const {
		return QImage::fromData(tileData(col, row), "JPG");
	}

	/* 同步解码缩略图 数据库没有时尝试按普通图片读 */ virtual QImage thumbnailImage() const {
		QImage img = QImage::fromData(thumbnailData());
		if (img.isNull()) {
			img = QImage(getThumbnailImage());
		}
		return img;
	}

protected:
	/* 每个线程持有自己的 SQLite 连接 避免 Qt SQL 跨线程问题 */ QSqlDatabase databaseForCurrentThread() const {
		const QString dbPath = getDbPath();
		if (dbPath.isEmpty()) return QSqlDatabase();

		const int maxConnectionsPerThread = 32;
		static thread_local QHash<QString, QString> connectionNames;
		static thread_local QList<QString> lruKeys;

		QString connName = connectionNames.value(dbPath);
		if (connName.isEmpty()) {
			while (connectionNames.size() >= maxConnectionsPerThread && !lruKeys.isEmpty()) {
				const QString oldPath = lruKeys.takeFirst();
				const QString oldConnName = connectionNames.take(oldPath);
				if (!oldConnName.isEmpty() && QSqlDatabase::contains(oldConnName)) {
					{
						QSqlDatabase oldDb = QSqlDatabase::database(oldConnName);
						oldDb.close();
					}
					QSqlDatabase::removeDatabase(oldConnName);
				}
			}

			connName = QString("TileSource_%1_%2")
				.arg((quintptr)QThread::currentThreadId())
				.arg(qHash(dbPath));
			connectionNames.insert(dbPath, connName);
		}

		lruKeys.removeAll(dbPath);
		lruKeys.append(dbPath);

		QSqlDatabase db;
		if (QSqlDatabase::contains(connName)) {
			db = QSqlDatabase::database(connName);
		}
		else {
			db = QSqlDatabase::addDatabase("QSQLITE", connName);
			db.setDatabaseName(dbPath);
			db.setConnectOptions("QSQLITE_OPEN_READONLY;PRAGMA mmap_size=268435456;");
		}

		if (!db.isOpen()) {
			db.open();
		}
		return db;
	}
};

#endif

#include "AsyncImageLoader.h"
#include "PackImageTileSource.h"
#include "PackReaderQt.h"
#include <QtConcurrent>
#include <QThread>
#include <QMetaObject>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QImageReader>
#include <QTimer>
#include <QPointer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QHash>
#include <QList>
#include <QFileInfo>
#include <QSharedPointer>

#include <stdexcept>

namespace {
	const int kMaxSqlConnectionsPerThread = 32;

	struct PackReaderSlot {
		QSharedPointer<PackReaderQt> reader;
		QMutex mutex;
	};

	QSharedPointer<PackReaderSlot> packReaderSlotForRoot(const QString& packRoot)
	{
		static QMutex cacheMutex;
		static QHash<QString, QSharedPointer<PackReaderSlot> > cache;

		const QString root = QFileInfo(packRoot).absoluteFilePath();
		QMutexLocker locker(&cacheMutex);
		if (!cache.contains(root)) {
			QSharedPointer<PackReaderSlot> slot(new PackReaderSlot);
			slot->reader.reset(new PackReaderQt(root));
			cache.insert(root, slot);
		}
		return cache.value(root);
	}

	bool isTileDatabasePath(const QString& path)
	{
		return QFileInfo(path).suffix().compare("db", Qt::CaseInsensitive) == 0;
	}

	QImage loadImageFromPackUri(const QString& uri, const QSize& targetSize)
	{
		QString packRoot;
		quint64 globalIndex = 0;
		if (!PackImageTileSource::parsePackFrameUri(uri, &packRoot, &globalIndex)) {
			return QImage();
		}
		if (!QFileInfo(QDir(packRoot).filePath("PackIndex.idx")).exists()) {
			qWarning() << "Pack v2 index not found:" << packRoot;
			return QImage();
		}

		try {
			QSharedPointer<PackReaderSlot> slot = packReaderSlotForRoot(packRoot);
			QByteArray jpegBytes;
			{
				QMutexLocker readerLocker(&slot->mutex);
				jpegBytes = slot->reader->readJpeg(globalIndex);
			}

			QImage img;
			img.loadFromData(jpegBytes, "JPG");
			if (!img.isNull() && targetSize.isValid()) {
				img = img.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			}
			return img;
		}
		catch (const std::exception& ex) {
			qWarning() << "Pack image read failed:" << uri << QString::fromLocal8Bit(ex.what());
			return QImage();
		}
	}

	QImage loadImageFromFile(const QString& path, const QSize& targetSize)
	{
		QImageReader reader(path);
		reader.setAutoTransform(true);
		if (targetSize.isValid()) {
			const QSize srcSize = reader.size();
			if (srcSize.isValid()) {
				reader.setScaledSize(srcSize.scaled(targetSize, Qt::KeepAspectRatio));
			}
		}
		return reader.read();
	}

	void applyBrightness(QImage& img, int br)
	{
		if (br == 0 || img.isNull()) return;
		if (img.format() != QImage::Format_RGB888) {
			img = img.convertToFormat(QImage::Format_RGB888);
		}
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
		int totalBytes = img.sizeInBytes();
#else
		int totalBytes = img.byteCount();
#endif
		uchar* bits = img.bits();
		for (int i = 0; i < totalBytes; ++i) {
			int val = bits[i] + br;
			bits[i] = (val < 0) ? 0 : (val > 255 ? 255 : val);
		}
	}

	QSqlDatabase databaseForCurrentThread(const QString& dbPath, const QString& prefix)
	{
		static thread_local QHash<QString, QString> connectionNames;
		static thread_local QList<QString> lruKeys;

		const QString cacheKey = QString("%1|%2").arg(prefix, dbPath);
		QString connName = connectionNames.value(cacheKey);
		if (connName.isEmpty()) {
			while (connectionNames.size() >= kMaxSqlConnectionsPerThread && !lruKeys.isEmpty()) {
				const QString oldKey = lruKeys.takeFirst();
				const QString oldConnName = connectionNames.take(oldKey);
				if (!oldConnName.isEmpty() && QSqlDatabase::contains(oldConnName)) {
					{
						QSqlDatabase oldDb = QSqlDatabase::database(oldConnName);
						oldDb.close();
					}
					QSqlDatabase::removeDatabase(oldConnName);
				}
			}

			connName = QString("%1_%2_%3")
				.arg(prefix)
				.arg((quintptr)QThread::currentThreadId())
				.arg(qHash(dbPath));
			connectionNames.insert(cacheKey, connName);
		}

		lruKeys.removeAll(cacheKey);
		lruKeys.append(cacheKey);

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
}

// 单例入口，所有图层共享缓存和线程池。
AsyncImageLoader* AsyncImageLoader::instance()
{
	static AsyncImageLoader inst;
	return &inst;
}

// 初始化两套缓存：高清图一套，缩略图一套，避免互相挤掉。
AsyncImageLoader::AsyncImageLoader()
{
	// 高清图缓存：100 MB = 100 * 1024 KB
	// 50 * 1024 大概三张切片
	m_cache.setMaxCost(1000 * 1024);

	m_thumbnailCache.setMaxCost(500 * 1024); // 限制缩略图最大占用 500MB
	m_thumbThreadPool.setMaxThreadCount(2);  // 给缩略图分配 2 个专属的后台线程

	QPointer<QObject> guard(this);
}

// 保存亮度偏移值，后续新加载的图会按这个值处理。
void AsyncImageLoader::setBrightness(int value)
{
	m_brightness = value;

	// 注意：修改亮度后，理论上应该清空高清图缓存，让它们重新生成
	// m_cache.clear();
}

// 返回当前亮度设置，主要给外部状态栏或调试用。
int AsyncImageLoader::brightness() const
{
	return m_brightness;
}

// 只查缓存，不做磁盘读取；绘制函数里调用它不会拖慢界面。
QPixmap AsyncImageLoader::getSyncThumbnail(const QString& pathUri)
{
	int br = m_brightness;
	QString key = QString("%1_thumb_br_%2").arg(pathUri).arg(br);

	QMutexLocker locker(&m_mutex);
	if (m_thumbnailCache.contains(key)) {
		return *m_thumbnailCache.object(key);
	}

	return QPixmap(); // 没找到就返回空
}

// 异步加载缩略图；数据库读 thumbnail 表，普通图片直接按目标尺寸读。
void AsyncImageLoader::requestThumbnail(const QString& pathUri, const QSize& targetSize /*= QSize()*/)
{
	// 获取主线程当前亮度设置
	int br = m_brightness;

	// 生成带亮度的唯一 Key，比如 "C:/xxx.db_thumb_br_20"
	QString key = QString("%1_thumb_br_%2").arg(pathUri).arg(br);

	// 1. 查缓存，极速返回
	{
		QMutexLocker locker(&m_mutex);
		if (m_thumbnailCache.contains(key)) {
			QPixmap result = *m_thumbnailCache.object(key);
			QTimer::singleShot(0, this, [this, pathUri, result]() {
				emit sigThumbnailLoaded(pathUri, result);
				});
			return;
		}
	}

	// 2. 丢入专属的缩略图后台线程池
	// 核心修正 2：必须捕获当前亮度 br
	QtConcurrent::run(&m_thumbThreadPool, [this, pathUri, targetSize, key, br]() {
		QImage img;

		if (PackImageTileSource::parsePackFrameUri(pathUri, nullptr, nullptr)) {
			img = loadImageFromPackUri(pathUri, targetSize.isValid() ? targetSize : QSize(2048, 2048));
		}
		else if (isTileDatabasePath(pathUri)) {
			QSqlDatabase db = databaseForCurrentThread(pathUri, "ThumbConn");

			// 数据库切片源走 thumbnail 表，这是老项目最快的路径。
			if (db.isOpen()) {
				QSqlQuery query(db);
				if (query.exec("SELECT data FROM thumbnail LIMIT 1") && query.next()) {
					QByteArray thumbData = query.value(0).toByteArray();
					if (thumbData.size() > 0) {
						img = QImage::fromData(thumbData);
					}
				}
			}
		}
		else {
			// 整图源没有 thumbnail 表，直接从图片文件读一张缩略图。
			img = loadImageFromFile(pathUri, targetSize.isValid() ? targetSize : QSize(2048, 2048));
		}

		if (img.isNull()) {
			img = QImage(2048, 2048, QImage::Format_RGB888);
			img.fill(QColor(60, 60, 60)); // 灰底
		} 
		applyBrightness(img, br);

		// --- D. 移交主线程组装并缓存 ---
		// 这里需要将 img 捕获进去，因为 Lambda 结束后 img 就析构了
		QTimer::singleShot(0, this, [this, pathUri, key, img]() {
			QPixmap pix = QPixmap::fromImage(img);

			{
				QMutexLocker locker(&m_mutex);
				if (!m_thumbnailCache.contains(key)) {
					// RGB888/ARGB32 显存 cost 计算，单位 KB
					int cost = (pix.width() * pix.height() * 4) / 1024;
					m_thumbnailCache.insert(key, new QPixmap(pix), cost);
				}
			}

			emit sigThumbnailLoaded(pathUri, pix);
			});
		});
}

// 异步加载高清图；老切片请求和整图请求都从这里进。
void AsyncImageLoader::requestImage(const QString& pathUri, bool useSharedCache /*= true*/)
{
	const QString safePathUri = QString::fromLocal8Bit(pathUri.toLocal8Bit().constData());

	// 或者更简单的强行拷贝：
	// const QString safePathUri = pathUri;
	// Qt QString 具有自动写时复制，显式拷贝能降低 Bug 风险。

	// 工业级最稳妥写法：
	const QString threadLocalUri = pathUri;

	threadLocalUri.toUpper(); // 触发无意义的写操作，强制其在内存中生成独立副本
	threadLocalUri.toLower(); // 还原

	// 获取当前亮度设置，切片和整图都走同一套亮度逻辑。
	int br = m_brightness;

	// 生成带亮度的唯一 Key
	QString key = QString("%1_br_%2").arg(pathUri).arg(br);

	{
		QMutexLocker locker(&m_mutex);
		if (useSharedCache && m_cache.contains(key)) {
			QPixmap result = *m_cache.object(key);

			QTimer::singleShot(0, this, [this, pathUri, result]() {
				emit sigImageLoaded(pathUri, result);
				});
			return;
		}
	}

	QtConcurrent::run([this, threadLocalUri, br, key, useSharedCache]() {
		QImage img;

		// A. 解析 URI，判断是数据库直读还是本地文件读取
		if (PackImageTileSource::parsePackFrameUri(threadLocalUri, nullptr, nullptr)) {
			img = loadImageFromPackUri(threadLocalUri, QSize());
		}
		else {
			QStringList parts = threadLocalUri.split("|");
			if (parts.size() == 3) {
			// 这是从 TunnelSectionItem 传来的数据库切片请求
			QString dbPath = parts[0];
			int col = parts[1].toInt();
			int row = parts[2].toInt();

			QSqlDatabase db = databaseForCurrentThread(dbPath, "ThreadConn");

			if (db.isOpen()) {
				QSqlQuery query(db);
				query.prepare("SELECT data FROM tiles WHERE col = ? AND row = ?");
				query.addBindValue(col);
				query.addBindValue(row);

				if (query.exec() && query.next()) {
					QByteArray imgData = query.value(0).toByteArray();

					// qDebug() << "Thread ID:" << QThread::currentThreadId()
					//          << " decoding tile:" << threadLocalUri
					//          << " size:" << imgData.size();

					img = QImage::fromData(imgData, "JPG"); // 内存解码
				}
			}
		}
			else {
				// 向下兼容：普通的本地图片文件加载，整图模式就走这里。
				img = loadImageFromFile(threadLocalUri, QSize());
			}
		}

		// B. 数据损坏或空图的防闪退兜底处理
		if (img.isNull()) {
			img = QImage(1024, 1024, QImage::Format_RGB888);
			img.fill(Qt::gray);
		}

		// C. 格式标准化，为亮度调节算法做准备
		if (img.format() != QImage::Format_RGB888) {
			img = img.convertToFormat(QImage::Format_RGB888);
		}

		// D. 亮度调节计算，CPU 密集型操作。
		applyBrightness(img, br);

		// E. 移交主线程组装为 QPixmap，并通知渲染更新
		QObject* safeLoader = AsyncImageLoader::instance();
		QTimer::singleShot(0, safeLoader, [=]() {
			if (!img.isNull()) {
				QPixmap pix = QPixmap::fromImage(img);

				{
					QMutexLocker locker(&AsyncImageLoader::instance()->m_mutex);
					if (useSharedCache && !AsyncImageLoader::instance()->m_cache.contains(key)) {
						int cost = (int)(pix.width() * pix.height() * 4 / 1024);
						AsyncImageLoader::instance()->m_cache.insert(key, new QPixmap(pix), cost);
					}
				}

				// 务必使用原始请求字符串返回，
				// 以便接收端能在 Pending 字典中成功核销
				emit AsyncImageLoader::instance()->sigImageLoaded(threadLocalUri, pix);
			}
			});
		});
}

#include "AsyncImageLoader.h"
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

#include <opencv2/imgproc.hpp>

#include <stdexcept>
#include <climits>

namespace {
	const int kMaxSqlConnectionsPerThread = 32;


	bool isTileDatabasePath(const QString& path)
	{
		return QFileInfo(path).suffix().compare("db", Qt::CaseInsensitive) == 0;
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

	void applyImageAdjustments(QImage& img, int br, int contrast, int sharpen)
	{
		br = qBound(-100, br, 100);
		contrast = qBound(0, contrast, 200);
		sharpen = qBound(0, sharpen, 100);
		if (img.isNull() || (br == 0 && contrast == 100 && sharpen == 0)) return;
		if (img.format() != QImage::Format_RGB888) {
			img = img.convertToFormat(QImage::Format_RGB888);
		}
		const double contrastFactor = contrast / 100.0;
		for (int y = 0; y < img.height(); ++y) {
			uchar* row = img.scanLine(y);
			for (int x = 0; x < img.width() * 3; ++x) {
				row[x] = static_cast<uchar>(qBound(0,
					qRound((row[x] - 128) * contrastFactor + 128 + br), 255));
			}
		}
		if (sharpen == 0 || img.width() < 3 || img.height() < 3) return;
		cv::Mat source(img.height(), img.width(), CV_8UC3, img.bits(), img.bytesPerLine());
		cv::Mat blurred;
		cv::GaussianBlur(source, blurred, cv::Size(), 1.2, 1.2, cv::BORDER_REPLICATE);
		const double amount = sharpen * 3.0 / 100.0;
		cv::addWeighted(source, 1.0 + amount, blurred, -amount, 0.0, source);
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

void AsyncImageLoader::setCacheMemoryLimits(qint64 imageBytes, qint64 thumbnailBytes)
{
    QMutexLocker locker(&m_mutex);
    const qint64 imageKiB = qMax<qint64>(1024, (imageBytes + 1023) / 1024);
    const qint64 thumbKiB = qMax<qint64>(1024, (thumbnailBytes + 1023) / 1024);
    m_cache.setMaxCost(static_cast<int>(qMin<qint64>(INT_MAX, imageKiB)));
    m_thumbnailCache.setMaxCost(static_cast<int>(qMin<qint64>(INT_MAX, thumbKiB)));
}

void AsyncImageLoader::setThumbnailThreadCount(int count)
{
    m_thumbThreadPool.setMaxThreadCount(qBound(1, count, 8));
}

void AsyncImageLoader::clearCaches()
{
    QMutexLocker locker(&m_mutex);
    m_cache.clear();
    m_thumbnailCache.clear();
}

qint64 AsyncImageLoader::imageCacheBytesApprox() const
{
    QMutexLocker locker(&m_mutex);
    return static_cast<qint64>(m_cache.totalCost()) * 1024LL;
}

qint64 AsyncImageLoader::thumbnailCacheBytesApprox() const
{
    QMutexLocker locker(&m_mutex);
    return static_cast<qint64>(m_thumbnailCache.totalCost()) * 1024LL;
}

// 保存亮度偏移值，后续新加载的图会按这个值处理。
void AsyncImageLoader::setImageAdjustments(int brightness, int contrast, int sharpen)
{
	m_brightness = qBound(-100, brightness, 100);
	m_contrast = qBound(0, contrast, 200);
	m_sharpen = qBound(0, sharpen, 100);
}

// 只查缓存，不做磁盘读取；绘制函数里调用它不会拖慢界面。
QPixmap AsyncImageLoader::getSyncThumbnail(const QString& pathUri)
{
	const int br = m_brightness;
	const int contrast = m_contrast;
	const int sharpen = m_sharpen;
	QString key = QString("%1_thumb_b_%2_c_%3_s_%4").arg(pathUri).arg(br).arg(contrast).arg(sharpen);

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
	const int br = m_brightness;
	const int contrast = m_contrast;
	const int sharpen = m_sharpen;

	// 生成带亮度的唯一 Key，比如 "C:/xxx.db_thumb_br_20"
	QString key = QString("%1_thumb_b_%2_c_%3_s_%4").arg(pathUri).arg(br).arg(contrast).arg(sharpen);

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
	QtConcurrent::run(&m_thumbThreadPool, [this, pathUri, targetSize, key, br, contrast, sharpen]() {
		QImage img;

		if (isTileDatabasePath(pathUri)) {
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
		applyImageAdjustments(img, br, contrast, sharpen);

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
	const int br = m_brightness;
	const int contrast = m_contrast;
	const int sharpen = m_sharpen;

	// 生成带亮度的唯一 Key
	QString key = QString("%1_b_%2_c_%3_s_%4").arg(pathUri).arg(br).arg(contrast).arg(sharpen);

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

	QtConcurrent::run([this, threadLocalUri, br, contrast, sharpen, key, useSharedCache]() {
		QImage img;

		// A. 数据库切片仍由 AsyncImageLoader 处理；普通整图直接从文件读取。
		// 当前 Pack 不再走 AsyncImageLoader，它与单图虚拟序列统一由 ISequenceFrameSource 解码。
		QStringList parts = threadLocalUri.split("|");
		if (parts.size() == 3) {
			// 这是从 TunnelSectionItem 传来的数据库切片请求。
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
					const QByteArray imgData = query.value(0).toByteArray();
					img = QImage::fromData(imgData, "JPG");
				}
			}
		}
		else {
			img = loadImageFromFile(threadLocalUri, QSize());
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
		applyImageAdjustments(img, br, contrast, sharpen);

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

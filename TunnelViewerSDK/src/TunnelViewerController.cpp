#include "TunnelViewerController.h"

// 这里才需要包含具体的实现类
#include "TiledGraphicsView.h" 
#include "AbstractSourceFactory.h"
#include "VirtualImageSequence.h"
#include "ImagePackReader.h"
#include "./items/DefectShapeItem.h"   
#include <QDebug> 
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include<QSqlDatabase>
#include<QSqlQuery>
#include<QUuid>
#include <QImageReader>
#include <QSqlRecord>
#include <QRegularExpression>

#include <stdexcept>
#include <climits>
#include <algorithm>

namespace {

QString normalizedMileageKey(const QString& key)
{
	QString result = key.trimmed().toLower();
	result.remove("_");
	result.remove("-");
	return result;
}

bool isStartMileageKey(const QString& key)
{
	const QString normalized = normalizedMileageKey(key);
	return normalized == "realstartmileage"
		|| normalized == "realbeginmileage"
		|| normalized == "realmileagestart"
		|| normalized == "dtruebegmile"
		|| normalized == "beginmileage"
		|| normalized == "startmileage"
		|| normalized == "begmileage";
}

bool isEndMileageKey(const QString& key)
{
	const QString normalized = normalizedMileageKey(key);
	return normalized == "realendmileage"
		|| normalized == "realmileageend"
		|| normalized == "dtrueendmile"
		|| normalized == "endmileage"
		|| normalized == "stopmileage";
}

void assignMileageValue(DbImageInfo& info, const QString& key, const QVariant& value,
	bool& hasStartMileage, bool& hasEndMileage)
{
	bool ok = false;
	const double mileage = value.toString().trimmed().toDouble(&ok);
	if (!ok) return;

	if (isStartMileageKey(key)) {
		info.realStartMileage = mileage;
		hasStartMileage = true;
	}
	else if (isEndMileageKey(key)) {
		info.realEndMileage = mileage;
		hasEndMileage = true;
	}
}

}


TunnelViewerController::TunnelViewerController(TiledGraphicsView* view, QObject* parent)
    : QObject(parent), m_view(view), m_factory(nullptr), m_maxRouteSections(2000)
{
	m_strBegImageName = "";
	m_strEndImageName = "";

	m_curTunnelNames.clear();
}

TunnelViewerController::~TunnelViewerController()
{
    if (m_factory) delete m_factory;
    // 如果工厂是你接管的，可以在这里 delete m_factory;
    // 但通常建议谁 new 谁 delete，或者用智能指针
}

void TunnelViewerController::setSourceFactory(AbstractSourceFactory* factory)
{
	if (m_factory == factory) {
		return;
	}
	if (m_factory) {
		delete m_factory;
	}
    m_factory = factory;
}

void TunnelViewerController::setMaxRouteSections(int maxSections)
{
	m_maxRouteSections = qMax(1, maxSections);
}

int TunnelViewerController::maxRouteSections() const
{
	return m_maxRouteSections;
}

void TunnelViewerController::restoreViewUpdates(bool blockSignals)
{
	if (!m_view) {
		return;
	}
	if (m_view->scene()) {
		m_view->scene()->blockSignals(blockSignals);
	}
	m_view->setUpdatesEnabled(true);
}

bool TunnelViewerController::loadRoute(const QString& rootPath)
{
    if (!m_view || !m_factory) {
        qWarning() << QString::fromLocal8Bit( "Controller未初始化 View 或 Factory");
        return false;
    }
    m_rootPath = rootPath;
	m_packFrameInfos.clear();
	m_view->setLayoutOrientation(m_factory->layoutOrientation());

    m_view->set_scrollSpeed(m_factory->scrollSpeed());

    clear(); // 先清空旧的

    QDir dir(rootPath);
    if (!dir.exists()) return false;

    // 1. 获取所有子文件夹
	QList<DbImageInfo> subImages  = scanDatabaseFolder(rootPath);
	if (subImages.isEmpty()) {
		qWarning() << "No valid tile databases found in route:" << rootPath;
		return false;
	}
	if (subImages.size() > m_maxRouteSections) {
		qWarning() << "Route section count exceeds safety limit:"
			<< subImages.size() << "limit:" << m_maxRouteSections
			<< "root:" << rootPath;
		return false;
	}

    // 2.  排序  
    if (m_factory) {
     //   m_factory->sortDatabaseFolder(subImages);
    }
	    
	// 只记录真正创建成功的图层，避免无效 DB 让上层索引错位。
	m_curTunnelNames.clear();
	 
     
    //  获取拼接方向
    auto orientation = m_factory->layoutOrientation();

    // =========================================================
    // 🟢 性能优化 1: 暂时禁止 View 的视口更新
    // 否则每添加一个 Item，View 可能会尝试重绘或计算，拖慢 10倍 速度
    // =========================================================
    m_view->setUpdatesEnabled(false);
    if (m_view->scene()) m_view->scene()->blockSignals(true); // 暂时屏蔽场景信号
    // =========================================================
    // 🟢 UI 优化: 创建进度条
    // =========================================================
    int totalCount = subImages.size();
    QProgressDialog progress(("正在加载项目数据..."),
		("取消"),
        0, totalCount, m_view);
    progress.setWindowModality(Qt::WindowModal); // 模态窗口，阻塞用户操作其他地方
    progress.setMinimumDuration(500); // 只有超过0.5秒的任务才显示进度条，避免闪烁
    progress.setValue(0);

    // 5. 循环加载
    int i = 0;


    // 3. 循环加载
    for (const DbImageInfo& dbInfo : subImages) {
       
        // 🟢 更新进度条
        progress.setValue(i++);
        // 如果用户点了取消
        if (progress.wasCanceled()) {
            qDebug() << "Loading canceled by user.";
            // 恢复更新
			restoreViewUpdates(false);
            return false;
        }
		QString dbPath = dbInfo.dbFilePath;

        // 使用工厂创建数据源 (多态调用)
        AbstractTileSource* source = m_factory->create(dbPath);

        qreal fixed = 0;

        // 检查数据源是否有效
        if (source && source->isValid()) {
            // 把真实里程元数据一并交给 View；普通模式没有里程也完全兼容。
            m_view->addLayer(source, dbInfo);
            m_curTunnelNames.push_back(dbInfo.originalName);
           
        }
        else {
           
            if (source) delete source;
			qWarning() << "Skipped invalid section:" << dbPath;
        }
    }
    // 进度条完成
    progress.setValue(totalCount);
    // =========================================================
       // 🟢 恢复 View 更新
       // =========================================================
    if (m_view->scene()) m_view->scene()->blockSignals(false);
    m_view->setUpdatesEnabled(true);
	 
    m_view->resetToFit();
    return true;
}

bool TunnelViewerController::loadImages(const QStringList& imagePaths)
{
    return loadImages(imagePaths, SequenceLoadOptions());
}

bool TunnelViewerController::loadSequenceSource(const QSharedPointer<ISequenceFrameSource>& source,
    const SequenceLoadOptions& sequenceOptions,
    LayoutOrientation orientation,
    bool hMirrored,
    bool vMirrored,
    int scrollSpeed)
{
    if (!m_view || source.isNull() || source->frameCount() == 0) return false;

    // 这是单图模式和 Pack 模式的“汇合点”。
    // 从这里开始，两种模式完全共用：Scene 建模、缩略图/高清图切换、缓存、预加载、滚动和绘制。
    m_curTunnelNames.clear();
    m_curTunnelNames.reserve(static_cast<int>(qMin<quint64>(source->frameCount(), INT_MAX)));
    for (quint64 i = 0; i < source->frameCount(); ++i) {
        m_curTunnelNames.append(source->descriptor(i).imageName);
    }

    m_view->set_scrollSpeed(scrollSpeed);
    return m_view->loadVirtualSequence(source, sequenceOptions,
        orientation, hMirrored, vMirrored);
}

bool TunnelViewerController::loadImages(const QStringList& imagePaths, const SequenceLoadOptions& options)
{
    if (!m_view || !m_factory) {
        qWarning() << QString::fromLocal8Bit("Controller未初始化 View 或 Factory");
        return false;
    }

    QStringList candidatePaths;
    candidatePaths.reserve(imagePaths.size());
    for (const QString& imagePath : imagePaths) {
        if (!imagePath.isEmpty()) candidatePaths.append(imagePath);
    }
    if (candidatePaths.isEmpty()) {
        qWarning() << "No valid images for SDK view.";
        return false;
    }

    QStringList skippedPaths;
    QSharedPointer<FileSequenceFrameSource> source =
        FileSequenceFrameSource::create(candidatePaths, options.knownFrameSize, &skippedPaths);
    if (source.isNull() || source->frameCount() == 0) {
        qWarning() << "SDK image sequence contains no readable frames.";
        return false;
    }

    // 普通单图模式没有 Pack 元数据。
    m_packFrameInfos.clear();
    const bool loaded = loadSequenceSource(source, options,
        m_factory->layoutOrientation(), m_factory->horizontalMirror(), m_factory->verticalMirror(),
        m_factory->scrollSpeed());

    qDebug() << "[HN_SDK_VIRTUAL_IMAGE_LOAD] loaded=" << source->frameCount()
        << "skipped=" << skippedPaths.size()
        << "chunks=" << (source->frameCount() + qMax(1, options.chunkFrameCount) - 1)
            / qMax(1, options.chunkFrameCount);
    return loaded;
}

bool TunnelViewerController::loadPackRoute(const QString& packPath, const PackRouteOptions& options)
{
    if (!m_view) {
        qWarning() << "Controller has no view.";
        return false;
    }

    // 当前只支持正式 ImagePack V1.1.1 文件命名：
    // img_<tag>.jph + img_<tag>_0001.jpd ...
    // 不再包含任何旧 PackIndex.idx、旧命名或旧 Reader 兼容分支。
    QString error;
    QSharedPointer<PackSequenceFrameSource> source =
        PackSequenceFrameSource::create(packPath, options.verifyOnOpen, &error);
    if (source.isNull() || source->frameCount() == 0) {
        qWarning() << "Failed to open current ImagePack:" << packPath << error;
        return false;
    }

    const QString indexPath = ImagePackReader::resolveIndexPath(packPath);
    m_rootPath = QFileInfo(indexPath).absolutePath();

    m_packFrameInfos.clear();
    m_packFrameInfos.reserve(static_cast<int>(qMin<quint64>(source->frameCount(), INT_MAX)));
    for (quint64 i = 0; i < source->frameCount(); ++i) {
        const SequenceFrameDescriptor descriptor = source->descriptor(i);
        PackRouteFrameInfo info;
        info.imageName = descriptor.imageName;
        info.globalIndex = descriptor.globalIndex;
        info.sourceIndex = descriptor.sourceIndex;
        info.timeValue = descriptor.timeValue;
        info.width = descriptor.imageSize.width();
        info.height = descriptor.imageSize.height();
        m_packFrameInfos.append(info);
    }

    SequenceLoadOptions sequenceOptions;
    sequenceOptions.knownFrameSize = source->descriptor(0).imageSize;

    // 注意：Pack 到这里以后与 loadImages() 完全走同一个显示函数。
    return loadSequenceSource(source, sequenceOptions,
        options.orientation, options.hMirrored, options.vMirrored, options.scrollSpeed);
}

QList<PackRouteFrameInfo> TunnelViewerController::packFrameInfos() const
{
	return m_packFrameInfos;
}

QStringList TunnelViewerController::visibleImageFileNames() const
{
	return m_view ? m_view->visibleImageFileNames() : QStringList();
}

 
QList<DbImageInfo> TunnelViewerController::scanDatabaseFolder(const QString& rootFolder)
{
    return scanDatabaseFolderInternal(rootFolder, true);
}

QList<DbImageInfo> TunnelViewerController::scanDatabaseFolderInternal(const QString& rootFolder, bool applyCurrentImageRange)
{
	QList<DbImageInfo> dbList;
	QDir dir(rootFolder);
	if (!dir.exists()) return dbList;

	// 只过滤出 .db 文件
	QStringList filters = m_factory ? m_factory->sourceFileFilters() : (QStringList() << "*.db");
	QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);
	int i = 0; 

	bool bIn = false;
	QFileInfoList saveFileList;

	 
	std::sort(fileList.begin(), fileList.end(), [](const QFileInfo& a, const QFileInfo& b) {
		 
		QString namea = a.fileName();
		int lastDushIdx = namea.lastIndexOf("."); 
		namea = namea.mid(0, lastDushIdx);


		QString nameb = b.fileName();
		lastDushIdx = nameb.lastIndexOf(".");
		nameb = nameb.mid(0, lastDushIdx);

		static QRegularExpression re("(\\d+(\\.\\d+)?)");
		QRegularExpressionMatch matchA = re.match(namea);
		double mileageA = matchA.hasMatch() ? matchA.captured(1).toDouble() : 0.0;

		QRegularExpressionMatch matchB = re.match(nameb);
		double mileageB = matchB.hasMatch() ? matchB.captured(1).toDouble() : 0.0;

		return    	mileageA < mileageB;
	});


	for (const QFileInfo& fileInfo : fileList)
	{

		QString tunnelName = fileInfo.fileName();
		int lastDushIdx = tunnelName.lastIndexOf(".");
		tunnelName = tunnelName.mid(0, lastDushIdx);


		if (applyCurrentImageRange && (!m_strBegImageName.isEmpty() || !m_strEndImageName.isEmpty()))
		{
			if (!bIn && tunnelName.compare(m_strBegImageName) == 0)
			{
				bIn = true;
			}

			if (bIn)
			{
				saveFileList.push_back(fileInfo);

				if (tunnelName.compare(m_strEndImageName) == 0)
				{
					bIn = false;

					break;
				}
			}

		}
		else  // 无区间，全加载
		{
			saveFileList.push_back(fileInfo);
		}
	}


	if (saveFileList.size() > m_maxRouteSections) {
		qWarning() << "Too many database files selected:"
			<< saveFileList.size() << "limit:" << m_maxRouteSections
			<< "root:" << rootFolder;
		return dbList;
	}

	for (const QFileInfo& fileInfo : saveFileList) 
	{
		i++;
		DbImageInfo info;
		info.dbFilePath = fileInfo.absoluteFilePath();
		info.width = 0;
		info.height = 0;
		info.tileSize = 2048; // 默认防错
        bool hasStartMileage = false;
        bool hasEndMileage = false;

		// 为每个文件生成临时连接名，防止冲突
		QString connName = QUuid::createUuid().toString();
		{
			QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
			db.setDatabaseName(info.dbFilePath);
			db.setConnectOptions("QSQLITE_OPEN_READONLY"); // 只读极速模式

			if (db.open()) {
				QSqlQuery query(db);
				// 1. 读尺寸信息
				if (query.exec("SELECT key, value FROM info")) {
					while (query.next()) {
						QString key = query.value(0).toString();
						QString val = query.value(1).toString();
						if (key == "width") info.width = val.toInt();
						else if (key == "height") info.height = val.toInt();
						else if (key == "tileSize") info.tileSize = val.toInt();
						else if (key == "originalName")
						{
							int lastDushIdx = val.lastIndexOf(".");
							QString baseName = val.mid(0, lastDushIdx);

							info.originalName = baseName;
						}

                        // 卷帘兼容：历史数据库真实里程字段名并不完全统一。
                        assignMileageValue(info, key, query.value(1), hasStartMileage, hasEndMileage);
					}
				}

                // 有些旧工程的 info 表不是 key/value，而是“一行多列”，两种格式都兼容。
                QSqlQuery columnQuery(db);
                if (columnQuery.exec("SELECT * FROM info LIMIT 1") && columnQuery.next()) {
                    const QSqlRecord record = columnQuery.record();
                    for (int column = 0; column < record.count(); ++column) {
                        const QString fieldName = record.fieldName(column);
                        const QString normalizedField = normalizedMileageKey(fieldName);
                        const QVariant fieldValue = columnQuery.value(column);
                        if (normalizedField == "width") info.width = fieldValue.toInt();
                        else if (normalizedField == "height") info.height = fieldValue.toInt();
                        else if (normalizedField == "tilesize") info.tileSize = fieldValue.toInt();
                        else if (normalizedField == "originalname") info.originalName = QFileInfo(fieldValue.toString()).completeBaseName();
                        assignMileageValue(info, fieldName, fieldValue, hasStartMileage, hasEndMileage);
                    }
                }
			}
		}
		QSqlDatabase::removeDatabase(connName);

        info.hasRealMileage = hasStartMileage && hasEndMileage
            && !qFuzzyCompare(info.realStartMileage + 1.0, info.realEndMileage + 1.0);
        if (info.originalName.isEmpty()) info.originalName = fileInfo.completeBaseName();

		if (info.width <= 0 || info.height <= 0) {
			/* 如果不是 SQLite 切片库 就按普通图片试一次 整图模式就是走这里 */ QImageReader reader(info.dbFilePath);
			const QSize imageSize = reader.size();
			if (imageSize.isValid()) {
				info.width = imageSize.width();
				info.height = imageSize.height();
				info.tileSize = qMax(info.width, info.height);
				info.originalName = fileInfo.completeBaseName();
			}
		}

		// 只有成功读到了宽高的数据库，才认为是有效工程
		if (info.width > 0 && info.height > 0) {
			dbList.append(info);
		}
	}


	return dbList;

}



// ==================== TunnelViewer 卷帘兼容 API ====================
bool TunnelViewerController::loadCurtainCompareRoute(const QString& compareRootPath)
{
	if (!m_view) return false;

	// 对比期按真实里程匹配，不应用基准期的文件名区间过滤。
	const QList<DbImageInfo> compareImages =
		scanDatabaseFolderInternal(compareRootPath, false);
	if (!m_view->setCurtainCompareImages(compareImages)) {
		qWarning() << "[CurtainCompare] no valid compare images with real mileage";
		return false;
	}

	return m_view->setCurtainCompareEnabled(true);
}

bool TunnelViewerController::setCurtainCompareEnabled(bool enabled)
{
	return m_view ? m_view->setCurtainCompareEnabled(enabled) : false;
}

bool TunnelViewerController::isCurtainCompareEnabled() const
{
	return m_view && m_view->isCurtainCompareEnabled();
}

void TunnelViewerController::setCurtainOrientation(CurtainOrientation orientation)
{
	if (m_view) m_view->setCurtainOrientation(orientation);
}

void TunnelViewerController::setCurtainPosition(qreal ratio)
{
	if (m_view) m_view->setCurtainPosition(ratio);
}

void TunnelViewerController::clearCurtainCompare()
{
	if (m_view) m_view->clearCurtainCompare();
}

void TunnelViewerController::clear()
{
    //if (m_view && m_view->scene()) {
    //    m_view->scene()->clear();
    //}
	 
	m_view->clear();
	m_packFrameInfos.clear();
    //m_currentTotalLength = 0;
}

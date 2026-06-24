
#include "hnBrowsePixWidget.h"
#include <QApplication>
#include <QFile>
#include <QStyle>
#include <QStyleOption>
#include <QtGlobal>
#include <QImageReader>
#include <memory>
#include <QShortcut>
#include <QtConcurrent/QtConcurrent>
#include <qDebug>
#include <QThread>
#include <QFuture>
#include <QElapsedTimer>
#include <QTimer>
#include <QMutexLocker>
#include "imageLoader.h"
#include "hnImagePainter.h"
#include <QTime>
#include <chrono>

#define RECT_BOARD_WIDTH 30	//矩形边框宽度

hnBrowsePixWidget::hnBrowsePixWidget(QWidget *parent)
	: QWidget(parent)
{
	//初始化
	this->init();
	m_setting=m_setting->getInstance();
	selectedDiseaseId = -1;
	selectedDiseaseTableName.clear();
}

hnBrowsePixWidget::~hnBrowsePixWidget()
{
}

void hnBrowsePixWidget::loadPix(const QString & pixDirName)
{
	imageLoader imgloader;
	this->m_pixNameMap.clear();
	{
		QMutexLocker locker(&m_imageMapMutex);
		++m_imageLoadGeneration;
		this->m_imageMap.clear();
	}
	m_lastPreloadBottomFrameIdx = -1;
	m_lastPreloadPriorityFrameIdx = -1;
	m_pendingPreloadBottomFrameIdx = -1;
	m_pendingPreloadPriorityFrameIdx = -1;
	this->m_pixNameMap = imgloader.loadImageNamesToMap(pixDirName,false);

	//获取分辨率
	this->getPixResoluion(pixDirName);

	int scrollBarMaxValue = this->m_pixNameMap.size() * 2;
	emit sig_scrollBarMaxValueChanged(scrollBarMaxValue);

	emit sig_scrollBarValueChanged(scrollBarMaxValue);

	//加载工程后进行的操作，供子类重载
	this->afterLoadPictures();

}

void hnBrowsePixWidget::loadPix(const QStringList & pixNames)
{
	if(pixNames.empty())
	{
		return;
	}
	imageLoader imgloader;
	this->m_pixNameMap.clear();
	{
		QMutexLocker locker(&m_imageMapMutex);
		++m_imageLoadGeneration;
		this->m_imageMap.clear();
	}
	m_lastPreloadBottomFrameIdx = -1;
	m_lastPreloadPriorityFrameIdx = -1;
	m_pendingPreloadBottomFrameIdx = -1;
	m_pendingPreloadPriorityFrameIdx = -1;
	this->m_pixNameMap = imgloader.loadImageNamesToMap(pixNames,this->m_reversePixNameMap);

	int scrollBarMaxValue = this->m_pixNameMap.size() * 2;
	emit sig_scrollBarMaxValueChanged(scrollBarMaxValue);

	emit sig_scrollBarValueChanged(scrollBarMaxValue);

	//加载工程后进行的操作，供子类重载
	this->afterLoadPictures();
}

bool hnBrowsePixWidget::reloadPix(const std::vector<QString>& vecImageName)
{

	//清空当前map
	this->m_pixNameMap.clear();
	this->m_reversePixNameMap.clear();

	//图片的绝对路径的名字
	QString imageAbsolutelyPath;

	QString pixDirName;	//他图片文件夹名字

	//遍历给定的数组，放到map中
	for (auto idx = 0; idx < vecImageName.size(); idx++)
	{
		imageAbsolutelyPath = QString("%1/%2").arg(pixDirName)
											.arg(vecImageName.at(idx));
		this->m_pixNameMap.insert(idx + 1, imageAbsolutelyPath);
		this->m_reversePixNameMap.insert(imageAbsolutelyPath, idx + 1);
	}

	//发送信号  照片数量变了
	int scrollBarMaxValue = this->m_pixNameMap.size() * 2;
	emit sig_scrollBarMaxValueChanged(scrollBarMaxValue);

	//发送信号  设置滚动条为最大值
	emit sig_scrollBarValueChanged(scrollBarMaxValue);

	this->update();

	return true;
}

bool hnBrowsePixWidget::reloadPix()
{
	std::vector<QString> pixNameVec;

	if (this->reloadPix(pixNameVec))
	{
		return false;
	}

	return true;
}

void hnBrowsePixWidget::clearPix()
{
	this->m_pixNameMap.clear();
	this->m_reversePixNameMap.clear();
	{
		QMutexLocker locker(&m_imageMapMutex);
		++m_imageLoadGeneration;
		this->m_imageMap.clear();
	}
	m_lastPreloadBottomFrameIdx = -1;
	m_lastPreloadPriorityFrameIdx = -1;
	m_pendingPreloadBottomFrameIdx = -1;
	m_pendingPreloadPriorityFrameIdx = -1;
	this->update();
}

QPoint hnBrowsePixWidget::getPixPos(const QString & pixName)
{
	QPoint point(-1, -1);

	if (this->m_pixNameMap.isEmpty())
	{
		//map要是空的  就直接返回
		return point;
	}

	//帧序号
	int frameIdx = 0;

	//是否找到图片
	bool isFoundPix = false;

	//遍历map，找到图片名字所在的序号是哪一个
	for (auto iter = this->m_pixNameMap.begin(); iter != this->m_pixNameMap.end(); iter++)
	{
		if (iter.value().contains(pixName))
		{
			frameIdx = iter.key();
			isFoundPix = true;
			break;
		}
	}

	//没找到就返回默认值
	if (isFoundPix == false)
	{
		return point;
	}

	//左下角的x坐标   永远是0
	point.setX(0);

	//设置y值坐标
	point.setY((frameIdx - 1) * m_pixHeight);

	return point;
}

QPoint hnBrowsePixWidget::singleImagePointToBigImagePoint(const QPoint & imagePoint, const QString & pixName)
{
#pragma region 遍历图片，找到该图片的帧序号
	//帧序号
	int frameIdx = 0;
	//是否找到图片
	bool isFoundPix = false;
	if (m_pixNameMap.size() == 0)
	{
		qWarning() << QStringLiteral("m_pixNameMap为空！imageToScreenPoint失败！");
		return(QPoint(-1, -1));
	}
	//auto start = std::chrono::high_resolution_clock::now();

	//frameIdx = this->m_pixNameMap.key(pixName);

	frameIdx = this->m_reversePixNameMap.value(pixName, -1);
	/*
		auto end = std::chrono::high_resolution_clock::now();

		auto ddd = std::chrono::duration_cast<std::chrono::microseconds>(end - start);*/
#pragma endregion

#if 0
	//对镜像的情况 做翻转处理
	QPoint newSingleImagePoint;
	if (m_isHMirrored)
	{
		newSingleImagePoint.setX(m_pixWidth - imagePoint.x());
	}
	if (m_isVMirrored)
	{
		newSingleImagePoint.setY(m_pixHeight - imagePoint.y());
	}
#endif

	//转换完成的point
	QPoint dstPoint;

	//设置x值，x值不变
	dstPoint.setX(imagePoint.x());

	//当前帧到底部帧的差值
	qreal difference = (frameIdx * 1.0 - this->m_buttomFrameIdx) * m_pixHeight;

	//y值做计算  从底部往上面算
	//int y = (frameIdx * 1.0 - this->m_buttomFrameIdx) * m_pixHeight + imagePoint.y();
	int y = this->m_tmpPixImageWithoutDisease.height() - difference - (m_pixHeight - imagePoint.y());
	//设置y值
	dstPoint.setY(y);

	return dstPoint;
}

QPoint hnBrowsePixWidget::bigImagePointToSingleImagePoint(const QPoint & labelImagePoint, QString * pixName)
{
	//获取帧序号
	int frameIdx = getFrameIdxFromImagePoint(labelImagePoint);

	//找到该序号对应的图片
	auto iter = m_pixNameMap.find(frameIdx);

	//异常处理
	if (iter == m_pixNameMap.end())
	{
		*pixName = "";
		return QPoint(-1, -1);

		//QPoint point;
		//point.setX(labelImagePoint.x());
		//if (frameIdx < 1)
		//{

		//	*pixName = m_pixNameMap.first();
		//	point.setY(0);
		//}
		//else if (frameIdx > m_pixNameMap.size())
		//{

		//	point.setY(m_pixHeight);
		//	*pixName = m_pixNameMap.last();
		//}
		//else //这种情况基本不会发生，上面两种情况，加正常情况，会涵盖所有情况
		//{
		//	*pixName = "";
		//	point = QPoint(-1, -1);
		//}
		//return point;

	}
	//给图片名字赋值
	*pixName = iter.value();

	//底部帧序号除以1.0 取余数
	qreal idxMod = this->getMod(m_buttomFrameIdx, 1.0);

	//计算的坐标结果
	QPoint dstPoint;

	//如果底部帧序号为一整张
	if (idxMod < 0.3)
	{
		//当前帧与底部帧的差值
		double difference = (frameIdx*1.0 - m_buttomFrameIdx)*m_pixHeight;
		int h = this->m_tmpPixImageWithoutDisease.height();
		int y = (h - difference) - labelImagePoint.y();

		y = m_pixHeight - y;

		dstPoint.setX(labelImagePoint.x());

		dstPoint.setY(y);
	}
	//如果底部帧序号 为半张
	else
	{
		//TODO cwb 这个地方有问题导致 半幅二维绘制后三维不显示
		//弄不清楚就在纸上画一画，算一算他是怎么动的
		int h = this->m_tmpPixImageWithoutDisease.height();
		int y = (frameIdx + 1 - m_buttomFrameIdx)*this->m_pixHeight - (h- labelImagePoint.y());

		dstPoint.setX(labelImagePoint.x());

		dstPoint.setY(y);

	}




	return dstPoint;
}

QPoint hnBrowsePixWidget::screenPointToBigImagePoint(const QPoint & point)
{
	const QSize imageSize = currentPaintImageSize();

	const double xScale = imageSize.width() * 1.0 / qMax(1, this->width());
	const double yScale = imageSize.height() *1.0 / qMax(1, this->height());

	return QPoint(qRound(point.x()*xScale), qRound(point.y()*yScale));

}

QPoint hnBrowsePixWidget::bigImagePointToScreenPoint(const QPoint & point)
{
	const QSize imageSize = currentPaintImageSize();

	const double xScale = imageSize.width() * 1.0 / qMax(1, this->width());
	const double yScale = imageSize.height() *1.0 / qMax(1, this->height());

	return QPoint(qRound(point.x()/xScale), qRound(point.y()/yScale));
}


QSize hnBrowsePixWidget::currentPaintImageSize() const
{
	const int widgetWidth = qMax(1, this->width());
	const int widgetHeight = qMax(1, this->height());

	if (m_pixWidth <=0)
	{
		if (!m_tmpPixImageWithoutDisease.isNull())
		{
			return m_tmpPixImageWithoutDisease.size();
		}
		return QSize(widgetWidth, widgetHeight);
	}
	const double imageScale = m_pixWidth *1.0 / widgetWidth;
	const int imageHeight = qMax(1, qCeil(widgetHeight * imageScale));
	return QSize(m_pixWidth, imageHeight);
}

int hnBrowsePixWidget::getCurrentMousePosFrameIdx()
{
	return m_currentMousePosFrameIdx;
}

QString hnBrowsePixWidget::getImageNameFromFrameIdx(const int frameIdx)
{
	//转换完成后的图片名称
	QString pixName;
	//找到该序号对应的图片
	auto iter = m_pixNameMap.find(frameIdx);

	//异常处理
	if (iter == m_pixNameMap.end())
	{
		return "";
	}

	//图片名称是含有绝对路径的，把图片名称单独提出来
	QString pixdirName = iter.value();

	QFileInfo info(pixdirName);

	pixName = info.fileName();

	return pixName;
}

int hnBrowsePixWidget::getFrameIdxFromImageName(const QString imageName)
{
	QString pixName;
	QStringList a;
	for (auto iter = m_pixNameMap.begin(); iter != m_pixNameMap.end(); iter++)
	{
		//图片名称是含有绝对路径的，把图片名称单独提出来
		QString pixdirName = iter.value();

		QFileInfo info(pixdirName);

		pixName = info.fileName();

		if (pixName == imageName)
		{
			return iter.key();
		}
	}
	//异常处理
	return -1;
}

qreal hnBrowsePixWidget::getButtomFrameNumber()
{
	return m_buttomFrameIdx;
}

void hnBrowsePixWidget::setVMirrored(const bool isMirrored)
{
	this->m_isVMirrored = isMirrored;
}

bool hnBrowsePixWidget::isVMirrored()
{
	return this->m_isVMirrored;
}

void hnBrowsePixWidget::setHMirrored(const bool isMirrroed)
{
	m_isHMirrored = isMirrroed;
}

bool hnBrowsePixWidget::isHMirrored()
{
	return m_isHMirrored;
}

void hnBrowsePixWidget::init()
{
	//初始化显示图片的label
	this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

	this->setMinimumWidth(200);

	this->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	//设置鼠标追踪
	this->setMouseTracking(true);

	//设置当前进度条的值为0
	this->m_currentScrollBarValue = 0;

	//设置当前鼠标位置帧数序号
	this->m_currentMousePosFrameIdx = 0;

	//设置鼠标指针形状为十字
	this->setCursor(Qt::CrossCursor);

	//获取图片宽度
	this->m_pixWidth = 5252; //4096		//5252
	//获取图片高度
	this->m_pixHeight = 2688;//2168		//2688

	//防止键盘事件不响应
	this->setFocusPolicy(Qt::ClickFocus);

	this->m_buttomFrameIdx = 0;

	this->m_imageMapMaxFrameIdx = 1;

	this->m_loadFrameNum = 7;

	this->m_lastScrollBarValue = -1;

	this->m_lastButtonFrameIdx = -1;

	//是否允许画图片
	this->m_isAllowDrawPix = true;



	//是否允许联动
	m_isAllowLinked = true;

	//是否左右镜像
	m_isHMirrored = false;

	//是否上下镜像
	m_isVMirrored = false;

	//当前视图帧数
	m_currentWidgetFrameNum = 10;

}

int hnBrowsePixWidget::getPixResoluion(const QString &pixDirName)
{
	QDir dir(pixDirName);
	if (!dir.exists())
	{
		return -1;
	}
	QStringList filiter;
	filiter << "*.jpg" << "*.jpeg";
	QFileInfoList infolist= dir.entryInfoList(filiter);
	if (infolist.size() == 0)
	{
		return -2;
	}
	QString pictureName =  infolist.at(0).absoluteFilePath();
	QImage image(pictureName);
	this->m_pixHeight = image.height();
	this->m_pixWidth = image.width();
}

void hnBrowsePixWidget::setPixResolution(const int width, const int height)
{
	this->m_pixWidth = width;
	this->m_pixHeight = height;
}

//这个函数要丢到多线程去
void hnBrowsePixWidget::updateImageMapBasedOnBottomFrameIdx(int bottomFrameIdx, int generation, QMap<int, QString> pixNameMap, bool hMirrored, bool vMirrored, int priorityFrameIdx, int loadFrameNum, int maxImagesToLoad)
{
	QElapsedTimer preloadTimer;
	preloadTimer.start();
	int loadedCount = 0;
	qint64 maxSingleLoadMs = 0;
	int slowestFrameIdx = -1;
	QString slowestFileName;

	const int btmIdx = bottomFrameIdx;
	const int LOAD_NUM = loadFrameNum;
	const int maxLoadCount = maxImagesToLoad > 0 ? maxImagesToLoad : 0;
	const int minKeep = qMax(1, btmIdx - LOAD_NUM);
	const int maxKeep = qMin(pixNameMap.size(), btmIdx + LOAD_NUM);

	QVector<int> loadOrder;
	auto appendFrameIdx = [&](int idx)
	{
		if (idx >= minKeep && idx <= maxKeep && !loadOrder.contains(idx))
		{
			loadOrder.append(idx);
		}
	};
	appendFrameIdx(priorityFrameIdx);
	appendFrameIdx(btmIdx);
	for (int offset = 1; offset <= LOAD_NUM; ++offset)
	{
		appendFrameIdx(btmIdx + offset);
		appendFrameIdx(btmIdx - offset);
	}

	for (int idx : qAsConst(loadOrder))
	{
		{
			QMutexLocker locker(&m_imageMapMutex);
			if (generation != m_imageLoadGeneration)
			{
				return;
			}
			if (m_imageMap.contains(idx))
			{
				continue;
			}
		}

		const QString fileName = pixNameMap.value(idx);
		if (fileName.isEmpty())
		{
			continue;
		}

		QElapsedTimer singleLoadTimer;
		singleLoadTimer.start();
		QImage image(fileName);
		const qint64 singleLoadMs = singleLoadTimer.elapsed();
		if (singleLoadMs > maxSingleLoadMs)
		{
			maxSingleLoadMs = singleLoadMs;
			slowestFrameIdx = idx;
			slowestFileName = fileName;
		}
		if (image.isNull())
		{
			continue;
		}

		image = image.mirrored(hMirrored, vMirrored);

		{
			QMutexLocker locker(&m_imageMapMutex);
			if (generation != m_imageLoadGeneration)
			{
				return;
			}
			if (!m_imageMap.contains(idx))
			{
				m_imageMap.insert(idx, image);
				++loadedCount;
				if (maxLoadCount > 0 && loadedCount >= maxLoadCount)
				{
					break;
				}
			}
		}
	}

	QVector<int> removeKeys;
	{
		QMutexLocker locker(&m_imageMapMutex);
		if (generation != m_imageLoadGeneration)
		{
			return;
		}
		for (auto it = m_imageMap.constBegin(); it != m_imageMap.constEnd(); ++it)
		{
			if (it.key() < minKeep || it.key() > maxKeep)
			{
				removeKeys.append(it.key());
			}
		}

		for (int key : qAsConst(removeKeys))
		{
			m_imageMap.remove(key);
		}
	}

	int cacheSize = 0;
	{
		QMutexLocker locker(&m_imageMapMutex);
		cacheSize = m_imageMap.size();
	}
	const qint64 preloadMs = preloadTimer.elapsed();
	if (preloadMs >= 80 || maxSingleLoadMs >= 40)
	{
		qDebug().noquote() << "[HN_PERF][PreloadImages]"
			<< "bottomFrameIdx=" << bottomFrameIdx
			<< "priorityFrameIdx=" << priorityFrameIdx
			<< "elapsedMs=" << preloadMs
			<< "maxSingleLoadMs=" << maxSingleLoadMs
			<< "slowestFrameIdx=" << slowestFrameIdx
			<< "slowestFile=" << slowestFileName
			<< "loadedCount=" << loadedCount
			<< "maxImagesToLoad=" << maxLoadCount
			<< "keepRange=" << QString("%1-%2").arg(minKeep).arg(maxKeep)
			<< "cacheSize=" << cacheSize;
	}

	QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
	QTimer::singleShot(0, this, [this, bottomFrameIdx, generation]()
	{
		QMutexLocker locker(&m_imageMapMutex);
		if (generation != m_imageLoadGeneration)
		{
			return;
		}
		locker.unlock();

		const int pendingBottomFrameIdx = m_pendingPreloadBottomFrameIdx;
		const int pendingPriorityFrameIdx = m_pendingPreloadPriorityFrameIdx;
		if (pendingBottomFrameIdx > 0 && pendingBottomFrameIdx != bottomFrameIdx)
		{
			if (shouldDeferPreload())
			{
				if (m_lastDeferredPreloadBottomFrameIdx != pendingBottomFrameIdx)
				{
					m_lastDeferredPreloadBottomFrameIdx = pendingBottomFrameIdx;
					qDebug().noquote() << "[HN_PERF][PreloadDeferred]"
						<< "bottomFrameIdx=" << pendingBottomFrameIdx
						<< "reason=inactive-or-hidden-after-run";
				}
				return;
			}
			m_pendingPreloadBottomFrameIdx = -1;
			m_pendingPreloadPriorityFrameIdx = -1;
			m_lastPreloadBottomFrameIdx = -1;
			m_lastPreloadPriorityFrameIdx = -1;
			schedulePreloadImages(pendingBottomFrameIdx, pendingPriorityFrameIdx);
		}
	});
}
bool hnBrowsePixWidget::ensureImageLoaded(const int frameIdx)
{
	if (frameIdx < 1 || frameIdx > m_pixNameMap.size())
	{
		return false;
	}

	{
		QMutexLocker locker(&m_imageMapMutex);
		if (m_imageMap.contains(frameIdx))
		{
			return true;
		}
	}

	const QString fileName = m_pixNameMap.value(frameIdx);
	if (fileName.isEmpty())
	{
		return false;
	}

	QImage image(fileName);
	if (image.isNull())
	{
		return false;
	}
	image = image.mirrored(m_isHMirrored, m_isVMirrored);

	QMutexLocker locker(&m_imageMapMutex);
	if (!m_imageMap.contains(frameIdx))
	{
		m_imageMap.insert(frameIdx, image);
	}
	return true;
}

bool hnBrowsePixWidget::getLoadedImage(const int frameIdx, QImage& image)
{
	if (getCachedImage(frameIdx, image))
	{
		return true;
	}

	schedulePreloadImages(static_cast<int>(m_buttomFrameIdx), frameIdx);
	return false;
}

bool hnBrowsePixWidget::getCachedImage(const int frameIdx, QImage& image)
{
	QMutexLocker locker(&m_imageMapMutex);
	auto iter = m_imageMap.constFind(frameIdx);
	if (iter == m_imageMap.constEnd())
	{
		return false;
	}
	image = iter.value();
	return !image.isNull();
}

bool hnBrowsePixWidget::shouldDeferPreload() const
{
	QWidget* topWidget = window();
	if (!isVisible() || (topWidget != Q_NULLPTR && topWidget->isMinimized()))
	{
		return true;
	}
	return QApplication::applicationState() != Qt::ApplicationActive;
}

void hnBrowsePixWidget::schedulePreloadImages(int bottomFrameIdx, int priorityFrameIdx)
{
	if (bottomFrameIdx <= 0)
	{
		return;
	}

	if (shouldDeferPreload())
	{
		m_pendingPreloadBottomFrameIdx = bottomFrameIdx;
		m_pendingPreloadPriorityFrameIdx = priorityFrameIdx;
		if (m_lastDeferredPreloadBottomFrameIdx != bottomFrameIdx)
		{
			m_lastDeferredPreloadBottomFrameIdx = bottomFrameIdx;
			qDebug().noquote() << "[HN_PERF][PreloadDeferred]"
				<< "bottomFrameIdx=" << bottomFrameIdx
				<< "reason=inactive-or-hidden";
		}
		return;
	}
	m_lastDeferredPreloadBottomFrameIdx = -1;

	if (m_preloadFuture.isRunning())
	{
		m_pendingPreloadBottomFrameIdx = bottomFrameIdx;
		m_pendingPreloadPriorityFrameIdx = priorityFrameIdx;
		return;
	}

	if (m_lastPreloadBottomFrameIdx == bottomFrameIdx && m_lastPreloadPriorityFrameIdx == priorityFrameIdx)
	{
		return;
	}

	m_pendingPreloadBottomFrameIdx = -1;
	m_pendingPreloadPriorityFrameIdx = -1;
	m_lastPreloadBottomFrameIdx = bottomFrameIdx;
	m_lastPreloadPriorityFrameIdx = priorityFrameIdx;
	int generation = 0;
	{
		QMutexLocker locker(&m_imageMapMutex);
		generation = m_imageLoadGeneration;
	}
	const QMap<int, QString> pixNameMap = m_pixNameMap;
	const bool hMirrored = m_isHMirrored;
	const bool vMirrored = m_isVMirrored;
	const int priorityFrame = priorityFrameIdx;
	const int loadFrameNum = m_loadFrameNum;
	const int maxImagesToLoad = m_preloadMaxImagesPerRun;
	m_preloadFuture = QtConcurrent::run([this, bottomFrameIdx, generation, pixNameMap, hMirrored, vMirrored, priorityFrame, loadFrameNum, maxImagesToLoad]()
	{
		updateImageMapBasedOnBottomFrameIdx(bottomFrameIdx, generation, pixNameMap, hMirrored, vMirrored, priorityFrame, loadFrameNum, maxImagesToLoad);
	});
}
void hnBrowsePixWidget::drawSomeThingOnImage(QImage & image)
{
}

void hnBrowsePixWidget::afterLoadPictures()
{
}

void hnBrowsePixWidget::drawPicture(QImage &image)
{
	//异常处理
	if (this->height() == 0 || this->width() == 0)
	{
		return;
	}

	//计算每一帧照片的长和宽
	double framePixWidth = m_pixWidth;
	double framePixHeight = m_pixHeight;

	//往label上画图片
	this->drawAllPixOnLabel(image, framePixHeight, framePixWidth, m_heightScale);

	//画完图片后  储存临时的、只含有图片的image 用于局部放大
	this->m_tmpPixImageWithoutDisease = image;

}

void hnBrowsePixWidget::slot_updateCurrentScrollBar(const int scrollBarValue)
{
	QElapsedTimer slotTimer;
	slotTimer.start();

	//设置加载图片的张数为当前帧数的两倍
	QDateTime currentDataTime = QDateTime::currentDateTime();

	qint64 intervalTimeMS = m_lastTime.msecsTo(currentDataTime);

	m_lastTime = currentDataTime;

	const bool isAfterIdle = qAbs(intervalTimeMS) >= 3000;
	if (isAfterIdle)
	{
		m_preloadMaxImagesPerRun = 1;
		this->setLoadFrameNum(qMax(1, qMin(m_currentWidgetFrameNum, 2)));
	}
	else
	{
		m_preloadMaxImagesPerRun = qMax(2, qMin(m_currentWidgetFrameNum + 1, 4));
		this->setLoadFrameNum(m_currentWidgetFrameNum * 2);
	}

	//倒转滚动条的值
	int tmpScrollBarValue = (this->m_pixNameMap.size() * 2) - scrollBarValue;

	//if (qAbs(tmpScrollBarValue - m_currentScrollBarValue) < 5 && qAbs(intervalTimeMS) > 70)
	//{
	//	this->update();
	//}
	////否则，延时进行更新，延时500ms
	//else
	//{
	//	QTimer::singleShot(500, [=]() {
	//		this->update();
	//	});
	//}
	//m_currentScrollBarValue = tmpScrollBarValue;
	const int oldValue = m_currentScrollBarValue;
	m_currentScrollBarValue = tmpScrollBarValue;
	this->update();

	const qint64 slotMs = slotTimer.elapsed();
	if (slotMs >= 20 || qAbs(intervalTimeMS) >= 3000)
	{
		qDebug().noquote() << "[HN_PERF][ScrollBarUpdate]"
			<< "rawValue=" << scrollBarValue
			<< "mappedValue=" << tmpScrollBarValue

			<< "oldMappedValue=" << oldValue
			<< "intervalMs=" << intervalTimeMS
			<< "loadFrameNum=" << m_loadFrameNum
			<< "slotMs=" << slotMs;
	}
	//
}

void hnBrowsePixWidget::slot_moveMouse(bool up,bool is2D)
{


}

void hnBrowsePixWidget::setLoadFrameNum(const int num)
{
	this->m_loadFrameNum = num;
}


void hnBrowsePixWidget::setSelectedDiseaseId(int diseaseId, const QString& tableName)
{
	selectedDiseaseId = diseaseId;
	selectedDiseaseTableName = tableName;
}

void hnBrowsePixWidget::updateMousePosFrameIdx(QMouseEvent *event)
{
	//计算当前高度    最下面是0
	const int currentHeight = (this->height() - event->pos().y()) *( m_pixWidth/this->width() );

	//计算每一帧照片的长和宽
	double framePixWidth = m_pixWidth;
	double framePixHeight = (framePixWidth / m_pixWidth) * m_pixHeight;

	//计算的结果
	int resultFrameIdx;
	//最底下的帧数
	double frameIdx = 0;

	//计算当前坐标的帧数
	if (m_currentScrollBarValue % 2 == 0)	//滚轮值为偶数
	{
		//计算底部帧序号
		frameIdx = (m_currentScrollBarValue + 2) / 2;
		//计算当前坐标的帧数
		resultFrameIdx = (currentHeight / framePixHeight) + frameIdx;
	}
	else									//滚轮值为奇数
	{
		//计算底部帧序号
		frameIdx = (m_currentScrollBarValue + 2) / 2.0;

		frameIdx = frameIdx - 0.5;

		//计算当前坐标的帧数
		resultFrameIdx = ((currentHeight + 0.5 * framePixHeight) / framePixHeight) + frameIdx;
	}

	//如果帧数大于容器的最大值，帧数就等于容器的最大值
	if (resultFrameIdx > this->m_pixNameMap.size())
	{
		resultFrameIdx = this->m_pixNameMap.size();
	}

	//发送信号出去  如果还是同一帧  就不用处理了
	if (resultFrameIdx != this->m_currentMousePosFrameIdx)
	{
		this->m_currentMousePosFrameIdx = resultFrameIdx;

		emit sig_mousePosFrameIdxUpdate(resultFrameIdx);
	}
}

void hnBrowsePixWidget::updateMousePosFrameIdx()
{
	QPoint pos = this->mapFromGlobal(QCursor().pos());

	//计算当前高度    最下面是0
	const int currentHeight = (this->height() - pos.y()) *(m_pixWidth / this->width());

	//计算每一帧照片的长和宽
	double framePixWidth = m_pixWidth;
	double framePixHeight = (framePixWidth / m_pixWidth) * m_pixHeight;

	//计算的结果
	int resultFrameIdx;
	//最底下的帧数
	double frameIdx = 0;

	//计算当前坐标的帧数
	if (m_currentScrollBarValue % 2 == 0)	//滚轮值为偶数
	{
		//计算底部帧序号
		frameIdx = (m_currentScrollBarValue + 2) / 2;
		//计算当前坐标的帧数
		resultFrameIdx = (currentHeight / framePixHeight) + frameIdx;
	}
	else									//滚轮值为奇数
	{
		//计算底部帧序号
		frameIdx = (m_currentScrollBarValue + 2) / 2.0;

		frameIdx = frameIdx - 0.5;

		//计算当前坐标的帧数
		resultFrameIdx = ((currentHeight + 0.5 * framePixHeight) / framePixHeight) + frameIdx;
	}

	//如果帧数大于容器的最大值，帧数就等于容器的最大值
	if (resultFrameIdx > this->m_pixNameMap.size())
	{
		resultFrameIdx = this->m_pixNameMap.size();
	}

	//发送信号出去  如果还是同一帧  就不用处理了
	if (resultFrameIdx != this->m_currentMousePosFrameIdx)
	{
		this->m_currentMousePosFrameIdx = resultFrameIdx;

		emit sig_mousePosFrameIdxUpdate(resultFrameIdx);
	}
}



QPoint hnBrowsePixWidget::transformPos(const QPoint & point)
{
	//目标point
	QPoint dstPoint;

	//放大比例
	double magnifyScale = this->m_tmpPixImageWithoutDisease.width() * 1.0 / this->width();

	//根据放大比例来计算新坐标
	dstPoint.setX(point.x() * 1.0  * magnifyScale);
	dstPoint.setY(point.y() * (this->m_tmpPixImageWithoutDisease.height()* 1.0 / this->height()));

	return dstPoint;
}

int hnBrowsePixWidget::getFrameIdxFromImagePoint(const QPoint & point)
{
	//帧序号
	int frameIdx;
	//先判断底部坐标的帧数是整张 还是半张
	qreal mod = this->getMod(m_buttomFrameIdx, 1.0);

	//我们这里认为  余数大于0.3  他就是半张  如果小于0.3  他就是整张
	auto tempHh = this->height();
	auto tempHHH = this->m_tmpPixImageWithoutDisease.height();
	//当前高度
	int h = this->m_tmpPixImageWithoutDisease.height() -  point.y();

	//图片在底部上面的张数
	int num = 0;

	//算出单张图片映射到大图片上的图片高度
	//int pixMapHeight = (this->width() * m_pixHeight) / m_pixWidth;
	int pixMapHeight = m_pixHeight;
	//整张的时候
	if (mod < 0.3)
	{
		//图片在底部上面的张数
		num = h / pixMapHeight;
		//算帧数
		frameIdx = m_buttomFrameIdx + num;
	}
	//半张的时候
	else
	{
		//如果高度不足半张
		if (h <= 0.5*pixMapHeight)
		{
				 num = h / pixMapHeight;
				frameIdx  =  m_buttomFrameIdx - 0.5+num;

				return frameIdx;

		    //frameIdx = m_buttomFrameIdx - 0.5;
			//return frameIdx;
		}
		num = (h - pixMapHeight*0.5) / pixMapHeight;
		//算帧数
		frameIdx = m_buttomFrameIdx + 0.5 + num;

	}
	//如果h小于零，帧序号就减一
	if (h < 0)
	{
		frameIdx--;
	}

	return frameIdx;
}

qreal hnBrowsePixWidget::getMod(qreal a, qreal b)
{
	//负数情况进行判断
	if (a < 0)
	{
		a = -a;
	}
	if (b < 0)
	{
		b = -b;
	}

	while (a > b)
	{
		a = a - b;
	}
	if (a == b)
	{
		a = 0;
	}

	return a;
}

bool hnBrowsePixWidget::isValidPoint(const QPoint & screenPoint)
{
	//转为大image坐标
	QPoint bigImagePoint = this->screenPointToBigImagePoint(screenPoint);

	//转为单张图片坐标
	QString imageName;
	QPoint singleImagePoint = this->bigImagePointToSingleImagePoint(bigImagePoint, &imageName);

	if (imageName.isEmpty())
	{
		return false;
	}
	else
	{
		return true;
	}

	return false;
}



QImage hnBrowsePixWidget::getOriginalImage(QPoint mousePos ,const QImage &tmpImageWithoutDisease,
	const int originalWidgetWidth, const int originalWidgetHeight)
{
	//获取鼠标的坐标
	QPoint mousePosPoint = mousePos;

	//获取到显示图片label的image
	QImage srcImage = tmpImageWithoutDisease;

	//鼠标坐标转换为iamge中的坐标 //这个1.0必须要加,不然会有误差,下同
	mousePosPoint.setX(mousePosPoint.x() * (srcImage.width()* 1.0 / this->width()));
	mousePosPoint.setY(mousePosPoint.y() * (srcImage.height()* 1.0 / this->height()));

	//截取局部图片
	QImage dstImage;

	//左上角坐标
	QPoint leftTopPoint;
	//右下角坐标
	QPoint rightButtonPoint;

	int leftTopX = 0, leftTopY = 0, rightButtonX = 0, rightButtonY = 0;

	leftTopX = mousePosPoint.x() - originalWidgetWidth / 2.0;
	leftTopY = mousePosPoint.y() - originalWidgetHeight / 2.0;
	rightButtonX = mousePosPoint.x() + originalWidgetWidth / 2.0;
	rightButtonY = mousePosPoint.y() + originalWidgetHeight / 2.0;

#pragma region 越界控制

	//做一下判断，要是越界就控制一下

	//如果左上角的x小于0
	if (leftTopX < 0)
	{
		leftTopX = 0;	//左上角的x就等于0
		rightButtonX = originalWidgetWidth;//右下角的x等于原始比例显示窗口的宽
	}

	//如果右下角的y大于image的最大值
	if (rightButtonY > srcImage.height())
	{
		rightButtonY = srcImage.height();
		leftTopY = srcImage.height() - originalWidgetHeight;
	}

	//如果左上角的y小于0
	if (leftTopY < 0)
	{
		leftTopY = 0;
		rightButtonY = originalWidgetHeight;
	}

	//如果右下角的x大于 image的宽的最大值
	if (rightButtonX > srcImage.width())
	{
		rightButtonX = srcImage.width();
		leftTopX = srcImage.width() - originalWidgetWidth;
	}
#pragma endregion

	//设置点的xy
	leftTopPoint.setX(leftTopX);
	leftTopPoint.setY(leftTopY);
	rightButtonPoint.setX(rightButtonX);
	rightButtonPoint.setY(rightButtonY);

	//截图
	dstImage = srcImage.copy(QRect(leftTopPoint, rightButtonPoint));

	//返回image
	return dstImage;
}

void hnBrowsePixWidget::drawAllPixOnLabel(QImage &labelImage, const int framePixHeight, const int framePixWidth, const double heightScale)
{
	//底部帧序号
	double frameIdx = 0;
	this->m_currentWidgetPixNames.clear();

	//判断滚动条值是奇数还是偶数
	if (m_currentScrollBarValue % 2 == 0)	//如果是偶数   就直接画
	{
		//要画的帧序号
		frameIdx = (m_currentScrollBarValue + 2) / 2;

		//底部帧序号赋值
		this->m_buttomFrameIdx = frameIdx;

		//QtConcurrent::run(this, &hnBrowsePixWidget::updateImageMapBasedOnBottomFrameIdx);
		schedulePreloadImages(static_cast<int>(frameIdx));
		//计算开始里程
		this->m_beginEncoderMile = (m_buttomFrameIdx - 1) * heightScale * m_pixHeight;

		//算出要画的帧数
		int frameNum = labelImage.height() / framePixHeight;

		if (frameNum == 0) {
			frameNum = 1;
		}
		//整数给舍掉了
		frameNum++;

		this->m_currentWidgetFrameNum = frameNum;

		//计算终止里程
		this->m_endEncoderMile = (frameNum * heightScale * m_pixHeight) + m_beginEncoderMile;

		//开始循环画
		for (auto num = 0; num < frameNum; num++)
		{
			int cnt;
			cnt = frameNum - num;

			QImage image;
			if (!getLoadedImage(static_cast<int>(frameIdx), image))
			{
				this->delayReupdate();
				return;
			}

			//往当前视图的图片名称数组中添加
			auto pixNameiter = this->m_pixNameMap.find(frameIdx);
			if (pixNameiter != this->m_pixNameMap.end())
			{
				this->m_currentWidgetPixNames.insert(frameIdx, pixNameiter.value());
			}
			int tempV1 = labelImage.height();
			int v2 = labelImage.height() - (num + 1) * framePixHeight;
			hnImagePainter imagePainter;
			imagePainter.drawImageOnAnotherImage(image, labelImage, 0, labelImage.height() - (num + 1) * framePixHeight,
				framePixWidth, framePixHeight, false);

			frameIdx++;
		}
	}
	else //奇数的时候 ，先把一半画出来 再画剩余的
	{
		//计算帧序号
		frameIdx = (m_currentScrollBarValue + 2) / 2.0;

		//底部帧序号赋值
		this->m_buttomFrameIdx = frameIdx;

	//	QtConcurrent::run(this, &hnBrowsePixWidget::updateImageMapBasedOnBottomFrameIdx);
		schedulePreloadImages(static_cast<int>(frameIdx));
		//计算开始里程
		this->m_beginEncoderMile = (m_buttomFrameIdx - 1) * heightScale * m_pixHeight;

		frameIdx = frameIdx - 0.5;

		QImage image;
		if (!getLoadedImage(static_cast<int>(frameIdx), image))
		{
			this->delayReupdate();
			return;
		}



		//底部的一半帧也要计入当前的病害名字数组
		auto pixNameiter = this->m_pixNameMap.find(frameIdx);
		if (pixNameiter != this->m_pixNameMap.end())
		{
			this->m_currentWidgetPixNames.insert(frameIdx, pixNameiter.value());
		}


		//画帧序号的一半
		hnImagePainter imagePainter;
		imagePainter.drawImageOnAnotherImage(image, labelImage, 0, labelImage.height() - m_pixHeight / 2,
			framePixWidth, framePixHeight, true);


		//然后画剩余的图片
		//帧序号 加 1
		frameIdx++;

		//算出要画的帧数
		int frameNum = (labelImage.height() - framePixHeight / 2) / framePixHeight;

		if (frameNum == 0) {
			frameNum = 1;
		}
		//整数给舍掉了
		frameNum++;

		this->m_currentWidgetFrameNum = frameNum;

		//计算终止里程
		this->m_endEncoderMile = ((frameNum + 0.5) * heightScale * m_pixHeight) + m_beginEncoderMile;

		//开始循环画
		for (auto num = 0; num < frameNum; num++)
		{
			int cnt;
			cnt = frameNum - num;
			QImage image;
			if (!getLoadedImage(static_cast<int>(frameIdx), image))
			{
				this->delayReupdate();
				return;
			}

			if (image.isNull() || image.bytesPerLine() == 0)
			{
				this->delayReupdate();
				return;
			}

			//往当前视图的图片名称数组中添加
			auto pixNameiter = this->m_pixNameMap.find(frameIdx);
			if (pixNameiter != this->m_pixNameMap.end())
			{
				this->m_currentWidgetPixNames.insert(frameIdx, pixNameiter.value());
			}
			int v2 = labelImage.height() - framePixHeight * 0.5 - (num + 1) * framePixHeight;
			hnImagePainter imagePainter;
			imagePainter.drawImageOnAnotherImage(image, labelImage,
				0, labelImage.height() - framePixHeight * 0.5 - (num + 1) * framePixHeight, framePixWidth, framePixHeight, false);

			frameIdx++;
		}
	}

}



void hnBrowsePixWidget::paintEvent(QPaintEvent * event)
{
	Q_UNUSED(event);
	QElapsedTimer paintTimer;
	paintTimer.start();

	//异常处理，图片数组为空就推出
	if (m_pixNameMap.isEmpty())
	{
		return;
	}
	int tempH = this->height();
	int tempw = this->width();
	int tempValue = this->height()*(m_pixWidth / this->width());

	//QImage image(this->m_pixWidth, this->height()*(m_pixWidth / this->width()), QImage::Format_RGB888);



	const QSize  imageSize = currentPaintImageSize();

	QImage image(imageSize, QImage::Format_RGB888);



	if (!m_tmpPixImageWithoutDisease.isNull()&&
		m_tmpPixImageWithoutDisease.size() == image.size())
	{
		image = m_tmpPixImageWithoutDisease.copy();
	}
	else
	{
		image.fill(Qt::black);
	}

	//允许画图片，就画上去
	QElapsedTimer baseDrawTimer;
	baseDrawTimer.start();
	if (this->m_isAllowDrawPix)
	{
		//画图片 传image 进去  这个是固定的，把几张分开的图片画到image上
		this->drawPicture(image);
	}



	//画其他的 传image 进去 这个函数是可重载的
	//这里面的临时内容判断，交由子类去自行处理
	const qint64 baseDrawMs = baseDrawTimer.elapsed();

	QElapsedTimer overlayTimer;
	overlayTimer.start();
	this->drawSomeThingOnImage(image);
	const qint64 overlayMs = overlayTimer.elapsed();

	//将图片一次性画到this窗口上
	QPainter painter(this);
	painter.drawImage(this->rect(), image);
	const qint64 paintMs = paintTimer.elapsed();
	if (paintMs >= 80 || baseDrawMs >= 40 || overlayMs >= 40)
	{
		qDebug().noquote() << "[HN_PERF][BrowsePaint]"
			<< "totalMs=" << paintMs
			<< "baseDrawMs=" << baseDrawMs
			<< "overlayMs=" << overlayMs
			<< "scrollValue=" << m_currentScrollBarValue
			<< "bottomFrame=" << m_buttomFrameIdx
			<< "visibleFrames=" << m_currentWidgetFrameNum
			<< "imageSize=" << QString("%1x%2").arg(image.width()).arg(image.height());
	}


	//记录上次滚动条的值
	this->m_lastScrollBarValue = this->m_currentScrollBarValue;
}

void hnBrowsePixWidget::resizeEvent(QResizeEvent* event)
{
	m_isAllowDrawPix = true;
	m_tmpContectImage = QImage();
	m_tmpPixImageWithoutDisease = QImage();
	QWidget::resizeEvent(event);
	update();
}

void hnBrowsePixWidget::delayReupdate()
{
	if (m_reupdatePending)
	{
		return;
	}
	m_reupdatePending = true;
	const int delayTime = 30;

	QTimer::singleShot(delayTime, this, [this]()
	{
		m_reupdatePending = false;
		m_isAllowDrawPix = true;
		this->update();
	});
}

void hnBrowsePixWidget::setOriginalWidgetWidthHeight(const int w, const int h)
{
	m_originalWidgetWidth = w;
	m_originalWidgetHeight = h;
}

bool hnBrowsePixWidget::getIsAllowLinked()
{
	return m_isAllowLinked;
}




QPoint hnBrowsePixWidget::screenToSingleImagePoint(const QPoint & point, QString & pixName)
{
	QPoint bigImagePoint = this->screenPointToBigImagePoint(point);
	QPoint singleImagePoint = this->bigImagePointToSingleImagePoint(bigImagePoint, &pixName);
	return singleImagePoint;
}

void hnBrowsePixWidget::setIsHiddenDisease(const bool isHidden)
{
	this->m_isHiddenDisease = isHidden;
}

bool hnBrowsePixWidget::getIsHiddenDisease()
{
	return this->m_isHiddenDisease;
}


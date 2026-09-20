#include "hn2d3dPixBaseWidget.h"
#include <QEvent>
#include <QTimer>
#include <QScrollBar>
#include <QCoreApplication>
#include <QPainter>
#include <QWheelEvent>
#include <QFileInfo>
#include <QCursor>
#include <QToolTip>
#include <QDateTime>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QDebug>
#include <QSet>
#include <QDataStream>
#include <QCryptographicHash>
#include <QtMath>
#include "hnDiseaseService.h"
#include "hn2d3dCoordinates.h"
#include "../TunnelViewerSDK/src/TiledGraphicsView.h"
#include "../TunnelViewerSDK/src/TunnelViewerController.h"
#include "../TunnelViewerSDK/include/WholeImageSourceFactory.h"

class SdkDiseaseOverlayWidget : public QWidget
{
public:
	explicit SdkDiseaseOverlayWidget(hn2d3dPixBaseWidget* owner)
		: QWidget(owner), m_owner(owner)
	{
		setAttribute(Qt::WA_TransparentForMouseEvents, true);
		setAttribute(Qt::WA_NoSystemBackground, true);
		setAttribute(Qt::WA_TranslucentBackground, true);
	}

protected:
	void paintEvent(QPaintEvent* event) override
	{
		if (m_owner)
		{
			m_owner->paintSdkDiseaseOverlay(event);
		}
	}

private:
	hn2d3dPixBaseWidget* m_owner = nullptr;
};
static bool isSdkNavigationKey(int key)
{
	switch (key)
	{
	case Qt::Key_W:
	case Qt::Key_A:
	case Qt::Key_D:
	case Qt::Key_S:
	case Qt::Key_Up:
	case Qt::Key_Down:
	case Qt::Key_Left:
	case Qt::Key_Right:
	case Qt::Key_Space:
		return true;
	default:
		return false;
	}
}


hn2d3dPixBaseWidget::hn2d3dPixBaseWidget(QWidget *parent) : hnBrowsePixWidget(parent)
{
	this->m_workMode = WorkMode::NO_MODE;
	this->m_isRightDeleteMouseDown = false;
	this->m_isEndAddPoint = false;
}

hn2d3dPixBaseWidget::~hn2d3dPixBaseWidget()
{
	// QObject children are normally deleted by QObject's base destructor.  That is
	// too late here: TiledGraphicsView::clear() may emit view-change signals after
	// hnBrowsePixWidget (and m_pixNameMap) has already been destroyed.
	if (m_sdkImageView)
	{
		m_sdkImageView->blockSignals(true);
		QObject::disconnect(m_sdkImageView, nullptr, this, nullptr);
		m_sdkImageView->removeEventFilter(this);
		if (m_sdkImageView->viewport())
		{
			m_sdkImageView->viewport()->removeEventFilter(this);
		}
	}

	// The graphics layer is a value member.  Detach it while the scene is still
	// alive so its destructor will not touch a scene already owned by the view.
	m_sdkDiseaseGraphicsLayer.setScene(nullptr);

	// The controller keeps a raw view pointer, so destroy it before the view.
	delete m_sdkImageController;
	m_sdkImageController = nullptr;

	delete m_sdkImageView;
	m_sdkImageView = nullptr;
}

void hn2d3dPixBaseWidget::ensureSdkImageView()
{
	if (m_sdkImageView && m_sdkImageController)
	{
		return;
	}

	setSkipLegacyImagePaint(true);
	m_sdkImageView = new TiledGraphicsView(this);
	m_sdkImageView->setObjectName(QStringLiteral("sdkImageView"));
	m_sdkImageView->setKeyboardNavigationEnabled(true);
	m_sdkImageView->setSingleFrameNavigationEnabled(m_singleFrameNavigationEnabled);
	m_sdkImageView->setFocusPolicy(Qt::StrongFocus);
	m_sdkImageView->setMouseTracking(true);
	m_sdkImageView->viewport()->setMouseTracking(true);
	m_sdkImageView->viewport()->setFocusPolicy(Qt::NoFocus);
	m_sdkImageView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_sdkImageView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_sdkImageView->setAttribute(Qt::WA_TransparentForMouseEvents, false);
	m_sdkImageView->setGeometry(rect());
	m_sdkImageView->viewport()->installEventFilter(this);
	m_sdkImageView->installEventFilter(this);
	m_sdkImageView->show();
	m_sdkImageView->raise();
	m_sdkImageView->setFocus();
	m_sdkDiseaseGraphicsLayer.setScene(m_sdkImageView->scene());
	ensureSdkDiseaseOverlay();
	m_sdkDiseaseRefreshTimer = new QTimer(this);
	m_sdkDiseaseRefreshTimer->setSingleShot(true);
	m_sdkDiseaseRefreshTimer->setInterval(60);
	connect(m_sdkDiseaseRefreshTimer, &QTimer::timeout, this, [this]() { refreshSdkDiseaseItems(); });

	m_sdkInspectionRefreshTimer = new QTimer(this);
	m_sdkInspectionRefreshTimer->setSingleShot(true);
	m_sdkInspectionRefreshTimer->setInterval(120);
	connect(m_sdkInspectionRefreshTimer, &QTimer::timeout, this, [this]()
	{
		renderSdkInspectionView(m_pendingSdkInspectionPoint);
	});
	connect(m_sdkImageView, &TiledGraphicsView::sigSequenceImageReady, this,
		[this](int, bool highResolution)
	{
		if (highResolution && m_sdkInspectionRefreshTimer
			&& m_pendingSdkInspectionPoint.x() >= 0)
		{
			m_sdkInspectionRefreshTimer->start(0);
		}
	});

	connect(m_sdkImageView, &TiledGraphicsView::sigViewCenterSceneChanged, this, [this](const QPointF& centerScenePos)
	{
		emit signal_sdkCenterEncoderMileChanged(sdkSceneYToEncoderMile(centerScenePos.y()));
	});

	connect(m_sdkImageView, &TiledGraphicsView::sigViewBottomAnchorChanged, this, [this](const TiledViewAnchor& anchor)
	{
		if (!anchor.valid)
		{
			return;
		}

		const double encoderMile = sdkAnchorToEncoderMile(anchor);
		syncLegacyBrowseStateFromEncoderMile(encoderMile);
		emit signal_sdkBottomEncoderMileChanged(encoderMile);
		scheduleSdkDiseaseRefresh();
		refreshStatusInfoFromSdkCurrentMouseOrAnchor(anchor);
		updateSdkDiseaseOverlay();
	});

	connect(m_sdkImageView, &TiledGraphicsView::sigUserViewBottomAnchorChanged, this, [this](const TiledViewAnchor& anchor)
	{
		if (!anchor.valid || m_isProgrammaticSdkScroll)
		{
			return;
		}

		const double encoderMile = sdkAnchorToEncoderMile(anchor);
		syncLegacyBrowseStateFromEncoderMile(encoderMile);
		emit signal_sdkUserBottomEncoderMileChanged(encoderMile);
		scheduleSdkDiseaseRefresh();
		refreshStatusInfoFromSdkCurrentMouseOrAnchor(anchor);
		updateSdkDiseaseOverlay();
		QTimer::singleShot(0, this, [this]()
		{
			restoreLittleFrameCursorAfterWheelBrowse();
		});
	});

	m_sdkImageController = new TunnelViewerController(m_sdkImageView, this);
}

void hn2d3dPixBaseWidget::setSingleFrameNavigationEnabled(bool enabled)
{
	m_singleFrameNavigationEnabled = enabled;
	if (m_sdkImageView)
	{
		m_sdkImageView->setSingleFrameNavigationEnabled(enabled);
	}
}

bool hn2d3dPixBaseWidget::stepSingleFrame(int visualDelta)
{
	return m_sdkImageView && m_sdkImageView->stepSingleFrame(visualDelta);
}

void hn2d3dPixBaseWidget::updateSdkInspectionViews(const QPoint& viewportPoint)
{
	if (!m_sdkInspectionRefreshTimer)
	{
		return;
	}
	m_pendingSdkInspectionPoint = viewportPoint;
	// Coalesce the mouse burst generated when the user returns from another
	// application. The actual preview render runs once after the pointer settles.
	m_sdkInspectionRefreshTimer->start();
}

void hn2d3dPixBaseWidget::renderSdkInspectionView(const QPoint& viewportPoint)
{
	if (!m_sdkImageView || !m_sdkImageView->viewport() || !m_sdkImageView->scene() ||
		!m_sdkImageView->viewport()->rect().contains(viewportPoint))
	{
		return;
	}

	const QPointF scenePoint = m_sdkImageView->mapToScene(viewportPoint);
	const QRectF sceneBounds = m_sdkImageView->scene()->sceneRect();
	if (!sceneBounds.contains(scenePoint))
	{
		return;
	}

	auto centeredSceneRect = [&sceneBounds, &scenePoint](int requestedWidth, int requestedHeight)
	{
		const qreal cropWidth = qMin<qreal>(qMax(1, requestedWidth), sceneBounds.width());
		const qreal cropHeight = qMin<qreal>(qMax(1, requestedHeight), sceneBounds.height());
		const qreal maxLeft = sceneBounds.right() - cropWidth;
		const qreal maxTop = sceneBounds.bottom() - cropHeight;
		const qreal left = qBound(sceneBounds.left(), scenePoint.x() - cropWidth / 2.0, maxLeft);
		const qreal top = qBound(sceneBounds.top(), scenePoint.y() - cropHeight / 2.0, maxTop);
		return QRectF(left, top, cropWidth, cropHeight);
	};

	const int previewWidth = qMax(1, m_originalWidgetWidth);
	const int previewHeight = qMax(1, m_originalWidgetHeight);
	const QPixmap preview = m_sdkImageView->previewRegionData(
		centeredSceneRect(previewWidth, previewHeight));
	if (!preview.isNull())
	{
		emit sig_mousePosImageChanged(preview.toImage());
	}

}

void hn2d3dPixBaseWidget::loadSdkVerticalImageSequence(const QStringList& pixNames)
{
	if (pixNames.isEmpty())
	{
		return;
	}

	ensureSdkImageView();
	clearSdkLittleFrameRenderCache();
	m_sdkDiseaseRenderFingerprints.clear();
	m_sdkDiseaseGraphicsLayer.clearAll();
	m_sdkImageController->setSourceFactory(new WholeImageSourceFactory(LayoutOrientation::VerticalReverse, 50, 0, m_isHMirrored, m_isVMirrored));


	SequenceLoadOptions sequenceOptions;
	sequenceOptions.knownFrameSize = QSize(m_pixWidth, m_pixHeight);
	if (!m_sdkImageController->loadImages(pixNames, sequenceOptions))
	{
		qWarning().noquote() << "[HN_SDK_IMAGE_LOAD_FAIL]"
			<< "count=" << pixNames.size();
		return;
	}
	m_sdkDiseaseGraphicsLayer.setScene(m_sdkImageView->scene());
	m_sdkImageView->setGeometry(rect());
	m_sdkImageView->raise();
	ensureSdkDiseaseOverlay();
	updateSdkDiseaseOverlay();
	m_sdkImageView->setFocus();
	setSdkBottomEncoderMile(0.0);
}

void hn2d3dPixBaseWidget::resizeEvent(QResizeEvent* event)
{
	hnBrowsePixWidget::resizeEvent(event);
	if (m_sdkImageView)
	{
		m_sdkImageView->setGeometry(rect());
		m_sdkImageView->raise();
	}
	if (m_sdkDiseaseOverlay)
	{
		m_sdkDiseaseOverlay->setGeometry(rect());
		// SDK scene 已经负责病害显示，旧透明 overlay 不能再盖到 SDK 视图上。
		if (m_sdkImageView)
		{
			m_sdkDiseaseOverlay->hide();
		}
		else if (!m_isDrawingDisease)
		{
			m_sdkDiseaseOverlay->raise();
			m_sdkDiseaseOverlay->update();
		}
	}
}

void hn2d3dPixBaseWidget::setImageDistanceMeters(double meters)
{
	if (meters <= 0.0)
	{
		return;
	}
	m_imageDistanceMeters = meters;
}

void hn2d3dPixBaseWidget::setSdkImageAdjustments(const ImageDisplayAdjustments& adjustments)
{
	ensureSdkImageView();
	if (m_sdkImageView)
	{
		m_sdkImageView->setImageAdjustments(adjustments);
	}
}

double hn2d3dPixBaseWidget::currentCenterEncoderMile() const
{
	if (m_sdkImageView)
	{
		pixImagePoint centerPoint;
		if (sdkScenePointToPixPoint(m_sdkImageView->currentCenterScenePos(), centerPoint))
		{
			return sdkPixPointToEncoderMile(centerPoint);
		}
	}

	if (m_imageDistanceMeters > 0.0)
	{
		return qMax(0.0, (m_buttomFrameIdx - 1.0) * m_imageDistanceMeters);
	}
	return 0.0;
}

void hn2d3dPixBaseWidget::scrollToEncoderMile(double encoderMile)
{
	setSdkCenterEncoderMile(encoderMile);
    refreshSdkViewState();
}

double hn2d3dPixBaseWidget::encoderMileToSdkSceneY(double encoderMile) const
{
	if (!m_sdkImageView || m_imageDistanceMeters <= 0.0 || m_pixHeight <= 0 || m_pixNameMap.isEmpty())
	{
		return 0.0;
	}

	const double maxEncoderMile = qMax(0.0, m_pixNameMap.size() * m_imageDistanceMeters);
	encoderMile = qBound(0.0, encoderMile, maxEncoderMile);
	int imageIndex = qFloor(encoderMile / m_imageDistanceMeters);
	if (imageIndex >= m_pixNameMap.size())
	{
		imageIndex = m_pixNameMap.size() - 1;
	}
	const QString imageName = m_pixNameMap.value(imageIndex + 1);
	const double imageBeginMile = imageIndex * m_imageDistanceMeters;
	const double offsetInImage = qBound(0.0, encoderMile - imageBeginMile, m_imageDistanceMeters);
	const double pixelY = m_pixHeight - offsetInImage * m_pixHeight / m_imageDistanceMeters;
	return const_cast<TiledGraphicsView*>(m_sdkImageView)->mapToGlobalScene(
		imageName, m_pixWidth * 0.5, pixelY).y();
}

double hn2d3dPixBaseWidget::sdkSceneYToEncoderMile(double sceneY) const
{
	if (!m_sdkImageView || m_imageDistanceMeters <= 0.0 || m_pixHeight <= 0)
	{
		return 0.0;
	}

	const QRectF sceneRect = m_sdkImageView->scene() ? m_sdkImageView->scene()->sceneRect() : QRectF();
	const qreal sceneX = sceneRect.isValid() ? sceneRect.center().x() : m_pixWidth * 0.5;
	QString imageName;
	int localX = 0;
	int localY = 0;
	TiledGraphicsView* view = const_cast<TiledGraphicsView*>(m_sdkImageView);
	if (view->GlobalSceneToMap(QPointF(sceneX, sceneY), imageName, localX, localY))
	{
		pixImagePoint point;
		point.pixName = imageName;
		point.pixPoint = QPoint(localX, localY);
		return sdkPixPointToEncoderMile(point);
	}

	// Scene gaps do not belong to a frame; probe the nearest image pixel.
	for (int delta = 1; delta <= 64; ++delta)
	{
		if (view->GlobalSceneToMap(QPointF(sceneX, sceneY - delta), imageName, localX, localY) ||
			view->GlobalSceneToMap(QPointF(sceneX, sceneY + delta), imageName, localX, localY))
		{
			pixImagePoint point;
			point.pixName = imageName;
			point.pixPoint = QPoint(localX, localY);
			return sdkPixPointToEncoderMile(point);
		}
	}
	return currentBottomEncoderMile();
}

bool hn2d3dPixBaseWidget::currentSdkVisibleEncoderMileRange(double& beginMile, double& endMile) const
{
	beginMile = m_beginEncoderMile;
	endMile = m_endEncoderMile;
	if (!m_sdkImageView || !m_sdkImageView->viewport())
	{
		return beginMile <= endMile;
	}

	const QRectF visibleSceneRect = m_sdkImageView->mapToScene(m_sdkImageView->viewport()->rect()).boundingRect();
	if (!visibleSceneRect.isValid() || visibleSceneRect.isEmpty())
	{
		return beginMile <= endMile;
	}

	beginMile = sdkSceneYToEncoderMile(visibleSceneRect.top());
	endMile = sdkSceneYToEncoderMile(visibleSceneRect.bottom());
	if (beginMile > endMile)
	{
		qSwap(beginMile, endMile);
	}

	// 给边界线留出一点余量，避免刚好在视口边缘时被过滤掉。
	const double margin = qMax(1.0, m_imageDistanceMeters);
	beginMile = qMax(0.0, beginMile - margin);
	endMile += margin;
	return true;
}

double hn2d3dPixBaseWidget::projectEncoderMileToSdkViewEncoderMile(double projectEncoderMile) const
{
	return projectEncoderMile;
}
double hn2d3dPixBaseWidget::sdkPixPointToEncoderMile(const pixImagePoint& point) const
{
	if (m_imageDistanceMeters <= 0.0 || m_pixHeight <= 0 || point.pixName.isEmpty())
	{
		return 0.0;
	}

	QString projectImageName;
	int frameIdx = -1;
	if (!resolveSdkImageName(point.pixName, projectImageName, &frameIdx) || frameIdx <= 0)
	{
		return 0.0;
	}

	const double localY = qBound(0.0, point.pixPoint.y() * 1.0, m_pixHeight * 1.0);
	const double imageBeginMile = (frameIdx - 1) * m_imageDistanceMeters;
	const double offsetInImage = (m_pixHeight - localY) * m_imageDistanceMeters / m_pixHeight;
	return imageBeginMile + offsetInImage;
}

double hn2d3dPixBaseWidget::sdkAnchorToEncoderMile(const TiledViewAnchor& anchor) const
{
	if (!anchor.valid)
	{
		return 0.0;
	}
	pixImagePoint point;
	point.pixName = anchor.imageName;
	point.pixPoint = QPoint(qRound(anchor.imagePixelPos.x()), qRound(anchor.imagePixelPos.y()));
	return sdkPixPointToEncoderMile(point);
}

void hn2d3dPixBaseWidget::setSdkCenterEncoderMile(double encoderMile)
{
	if (!m_sdkImageView)
	{
		return;
	}
	const int ticket = ++m_programmaticSdkScrollTicket;
	m_isProgrammaticSdkScroll = true;
	m_sdkImageView->scrollToSceneY(encoderMileToSdkSceneY(encoderMile));
	QTimer::singleShot(80, this, [this, ticket]()
	{
		if (ticket == m_programmaticSdkScrollTicket)
		{
			m_isProgrammaticSdkScroll = false;
		}
	});
}

void hn2d3dPixBaseWidget::setSdkBottomEncoderMile(double encoderMile)
{
	if (!m_sdkImageView || m_imageDistanceMeters <= 0.0 || m_pixHeight <= 0)
	{
		return;
	}

	encoderMile = qMax(0.0, encoderMile);
	const int maxIndex = qMax(0, m_pixNameMap.size() - 1);
	int imageIndex = qFloor(encoderMile / m_imageDistanceMeters);
	imageIndex = qBound(0, imageIndex, maxIndex);
	const QString imageName = m_pixNameMap.value(imageIndex + 1);
	if (imageName.isEmpty())
	{
		return;
	}

	const double imageBeginMile = imageIndex * m_imageDistanceMeters;
	const double offsetInImage = encoderMile - imageBeginMile;
	const double pixelY = m_pixHeight - offsetInImage * m_pixHeight / m_imageDistanceMeters;
	const int ticket = ++m_programmaticSdkScrollTicket;
	m_isProgrammaticSdkScroll = true;
	m_sdkImageView->scrollToImagePixel(imageName, pixelY, true);
	QTimer::singleShot(80, this, [this, ticket]()
	{
		if (ticket == m_programmaticSdkScrollTicket)
		{
			m_isProgrammaticSdkScroll = false;
		}
	});
}


//
//
void hn2d3dPixBaseWidget::syncLegacyBrowseStateFromEncoderMile(double encoderMile)
{
	if (m_imageDistanceMeters <= 0.0)
	{
		return;
	}
	const qreal bottomFrameIdx = encoderMile / m_imageDistanceMeters + 1.0;
	syncBrowseStateFromBottomFrame(bottomFrameIdx);

	if (m_sdkImageView)
	{
		// SDK 视图的病害查询范围必须和 SDK 的单图里程间隔一致，不能继续用旧拼图的 m_heightScale * m_pixHeight 推算。
		m_beginEncoderMile = qMax(0.0, (m_buttomFrameIdx - 1.0) * m_imageDistanceMeters);
		m_endEncoderMile = m_beginEncoderMile + qMax(1, m_currentWidgetFrameNum) * m_imageDistanceMeters;
	}
}

void hn2d3dPixBaseWidget::ensureSdkDiseaseOverlay()
{
	// SDK 模式下正式病害、临时病害、标签和引线都进入 QGraphicsScene，不再创建旧 QWidget overlay。
	if (m_sdkImageView)
	{
		if (m_sdkDiseaseOverlay)
		{
			m_sdkDiseaseOverlay->hide();
		}
		return;
	}

	if (m_sdkDiseaseOverlay)
	{
		m_sdkDiseaseOverlay->setGeometry(rect());
		m_sdkDiseaseOverlay->raise();
		return;
	}

	m_sdkDiseaseOverlay = new SdkDiseaseOverlayWidget(this);
	m_sdkDiseaseOverlay->setObjectName(QStringLiteral("sdkDiseaseOverlay"));
	m_sdkDiseaseOverlay->setGeometry(rect());
	m_sdkDiseaseOverlay->show();
	m_sdkDiseaseOverlay->raise();
}

void hn2d3dPixBaseWidget::updateSdkDiseaseOverlay()
{
	if (!m_sdkDiseaseOverlay)
	{
		return;
	}

	// SDK 视图存在时，旧 overlay 永远隐藏，避免和 SDK scene 重复显示或遮挡鼠标事件。
	if (m_sdkImageView || m_isDrawingDisease || m_workMode == WorkMode::GET_MILE)
	{
		m_sdkDiseaseOverlay->hide();
		return;
	}

	m_sdkDiseaseOverlay->show();
	m_sdkDiseaseOverlay->raise();
	m_sdkDiseaseOverlay->update();
}

void hn2d3dPixBaseWidget::paintSdkDiseaseOverlay(QPaintEvent* event)
{
	Q_UNUSED(event);
	// 保留函数是为了兼容旧对象生命周期；SDK 模式不再通过 QWidget overlay 绘制任何病害内容。
	return;
}

bool hn2d3dPixBaseWidget::refreshStatusInfoFromSdkViewportPoint(const QPoint& viewportPoint)
{
	SdkStatusContext context;
	if (!sdkStatusContextFromViewportPoint(viewportPoint, context))
	{
		return false;
	}

	m_lastSdkStatusViewportPoint = viewportPoint;
	m_hasLastSdkStatusViewportPoint = true;

	QString statusInfo = sdkStatusInfoFromContext(context);
	if (!statusInfo.isEmpty())
	{
		emit signal_statusInfoChanged(statusInfo);
		return true;
	}
	return false;
}
void hn2d3dPixBaseWidget::refreshStatusInfoFromSdkAnchor(const TiledViewAnchor& anchor)
{
	SdkStatusContext context;
	if (!sdkStatusContextFromAnchor(anchor, context))
	{
		return;
	}

	QString statusInfo = sdkStatusInfoFromContext(context);
	if (!statusInfo.isEmpty())
	{
		emit signal_statusInfoChanged(statusInfo);
	}
}

void hn2d3dPixBaseWidget::refreshStatusInfoFromSdkCurrentMouseOrAnchor(const TiledViewAnchor& anchor)
{
	if (m_sdkImageView && m_sdkImageView->viewport())
	{
		if (m_hasLastSdkStatusViewportPoint &&
			m_sdkImageView->viewport()->rect().contains(m_lastSdkStatusViewportPoint) &&
			refreshStatusInfoFromSdkViewportPoint(m_lastSdkStatusViewportPoint))
		{
			return;
		}

		const QPoint viewportPoint = m_sdkImageView->viewport()->mapFromGlobal(QCursor::pos());
		if (m_sdkImageView->viewport()->rect().contains(viewportPoint) &&
			refreshStatusInfoFromSdkViewportPoint(viewportPoint))
		{
			return;
		}
	}

	refreshStatusInfoFromSdkAnchor(anchor);
}
QString hn2d3dPixBaseWidget::currentStreetPictureNameForStatus(double fallbackTrueMile) const
{
    if (!m_currentStreetPictureNameForStatus.isEmpty())
    {
        return QFileInfo(m_currentStreetPictureNameForStatus).fileName();
    }

    auto dataManager = hnDataManager::getDataManager();
    if (!dataManager || !dataManager->isOpenProject() || !dataManager->getCurrentProject())
    {
        return QString();
    }

    return QFileInfo(dataManager->getCurrentProject()->getStreetPicturePath(fallbackTrueMile)).fileName();
}
QString hn2d3dPixBaseWidget::sdkStatusInfoFromWidgetPoint(const QPoint& widgetPoint)
{
	Q_UNUSED(widgetPoint);
	return QString();
}

QString hn2d3dPixBaseWidget::sdkStatusInfoFromContext(const SdkStatusContext& context)
{
	Q_UNUSED(context);
	return QString();
}

bool hn2d3dPixBaseWidget::sdkStatusContextFromViewportPoint(const QPoint& viewportPoint, SdkStatusContext& context) const
{
	if (!m_sdkImageView)
	{
		return false;
	}

	const QPointF scenePos = m_sdkImageView->mapToScene(viewportPoint);
	QString sdkImageName;
	int localX = 0;
	int localY = 0;
	if (!m_sdkImageView->GlobalSceneToMap(scenePos, sdkImageName, localX, localY))
	{
		return false;
	}

	QString projectImageName;
	int frameIdx = -1;
	if (!resolveSdkImageName(sdkImageName, projectImageName, &frameIdx))
	{
		return false;
	}

	const int maxPixelX = qMax(0, m_pixWidth - 1);
	const int imageExtentX = qMax(0, m_pixWidth);
	const int imageExtentY = qMax(0, m_pixHeight);
	const int roadX = qBound(0, localX, maxPixelX);
	const int roadY = qBound(0, localY, imageExtentY);
	const int imagePixelX = qBound(0, localX, imageExtentX);
	const int imagePixelY = qBound(0, localY, imageExtentY);
	const int originalX = m_isHMirrored ? imageExtentX - imagePixelX : imagePixelX;
	const int originalY = m_isVMirrored ? imageExtentY - imagePixelY : imagePixelY;

	context.valid = true;
	context.imageName = projectImageName;
	context.imageIndex = frameIdx;
	context.viewportPoint = viewportPoint;
	context.scenePoint = scenePos;
	context.singleImagePoint = QPoint(originalX, originalY);
	context.routeImagePoint = QPoint(roadX, (frameIdx - 1) * m_pixHeight + (m_pixHeight - roadY));
	pixImagePoint pixPoint;
	pixPoint.pixName = projectImageName;
	pixPoint.pixPoint = QPoint(roadX, roadY);
	context.encoderMile = sdkPixPointToEncoderMile(pixPoint);
	return true;
}

bool hn2d3dPixBaseWidget::sdkStatusContextFromAnchor(const TiledViewAnchor& anchor, SdkStatusContext& context) const
{
	if (!m_sdkImageView || !anchor.valid || anchor.imageName.isEmpty())
	{
		return false;
	}

	QString projectImageName;
	int frameIdx = -1;
	if (!resolveSdkImageName(anchor.imageName, projectImageName, &frameIdx))
	{
		return false;
	}
	const int maxPixelX = qMax(0, m_pixWidth - 1);
	const int imageExtentX = qMax(0, m_pixWidth);
	const int imageExtentY = qMax(0, m_pixHeight);
	const int rawX = qRound(anchor.imagePixelPos.x());
	const int rawY = qRound(anchor.imagePixelPos.y());
	const int roadX = qBound(0, rawX, maxPixelX);
	const int roadY = qBound(0, rawY, imageExtentY);
	const int imagePixelX = qBound(0, rawX, imageExtentX);
	const int imagePixelY = qBound(0, rawY, imageExtentY);
	const int originalX = m_isHMirrored ? imageExtentX - imagePixelX : imagePixelX;
	const int originalY = m_isVMirrored ? imageExtentY - imagePixelY : imagePixelY;

	context.valid = true;
	context.imageName = projectImageName;
	context.imageIndex = frameIdx;
	context.scenePoint = anchor.scenePos;
	context.viewportPoint = m_sdkImageView->mapFromScene(anchor.scenePos);
	context.singleImagePoint = QPoint(originalX, originalY);
	context.routeImagePoint = QPoint(roadX, (frameIdx - 1) * m_pixHeight + (m_pixHeight - roadY));
	pixImagePoint pixPoint;
	pixPoint.pixName = projectImageName;
	pixPoint.pixPoint = QPoint(roadX, roadY);
	context.encoderMile = sdkPixPointToEncoderMile(pixPoint);
	return true;
}

bool hn2d3dPixBaseWidget::resolveSdkImageName(const QString& sdkImageName, QString& projectImageName, int* frameIdx) const
{
	if (sdkImageName.isEmpty())
	{
		return false;
	}

	const int exactIdx = m_reversePixNameMap.value(sdkImageName, -1);
	if (exactIdx > 0)
	{
		projectImageName = sdkImageName;
		if (frameIdx) *frameIdx = exactIdx;
		return true;
	}

	const QFileInfo sdkInfo(sdkImageName);
	const QString sdkFileName = sdkInfo.fileName();
	const QString sdkBaseName = sdkInfo.completeBaseName();

	for (auto it = m_pixNameMap.constBegin(); it != m_pixNameMap.constEnd(); ++it)
	{
		const QString candidate = it.value();
		const QFileInfo candidateInfo(candidate);
		if (candidate == sdkImageName ||
			candidateInfo.fileName() == sdkImageName ||
			candidateInfo.completeBaseName() == sdkImageName ||
			(!sdkFileName.isEmpty() && candidateInfo.fileName() == sdkFileName) ||
			(!sdkBaseName.isEmpty() && candidateInfo.completeBaseName() == sdkBaseName) ||
			candidate.contains(sdkImageName))
		{
			projectImageName = candidate;
			if (frameIdx) *frameIdx = it.key();
			return true;
		}
	}

	return false;
}

QPoint hn2d3dPixBaseWidget::sdkViewportPointToLegacyWidgetPoint(const QPoint& viewportPoint)
{
	if (!m_sdkImageView)
	{
		return viewportPoint;
	}

	const QPointF scenePos = m_sdkImageView->mapToScene(viewportPoint);
	pixImagePoint pixPoint;
	if (!sdkScenePointToPixPoint(scenePos, pixPoint))
	{
		return m_sdkImageView->viewport()->mapTo(const_cast<hn2d3dPixBaseWidget*>(this), viewportPoint);
	}

	const QPoint bigImagePoint = singleImagePointToBigImagePoint(pixPoint.pixPoint, pixPoint.pixName);
	return bigImagePointToScreenPoint(bigImagePoint);
}

bool hn2d3dPixBaseWidget::shouldForwardSdkMouseToLegacy() const
{
	return m_workMode != WorkMode::NO_MODE || m_isDrawingDisease || m_isRightDeleteMouseDown;
}

bool hn2d3dPixBaseWidget::hasSdkImageView() const
{
	return m_sdkImageView != nullptr;
}

bool hn2d3dPixBaseWidget::forwardSdkMouseEventToLegacy(QMouseEvent* mouseEvent)
{
	if (!mouseEvent || !m_sdkImageView)
	{
		return false;
	}

	const QPoint legacyPoint = sdkViewportPointToLegacyWidgetPoint(mouseEvent->pos());
	QMouseEvent translatedEvent(mouseEvent->type(), legacyPoint, mouseEvent->globalPos(),
		mouseEvent->button(), mouseEvent->buttons(), mouseEvent->modifiers());
	QCoreApplication::sendEvent(this, &translatedEvent);
	updateSdkDiseaseOverlay();
	return translatedEvent.isAccepted();
}

bool hn2d3dPixBaseWidget::forwardSdkWheelEventToLegacy(QWheelEvent* wheelEvent)
{
	if (!wheelEvent || !m_sdkImageView)
	{
		return false;
	}

	const QPoint legacyPoint = sdkViewportPointToLegacyWidgetPoint(wheelEvent->pos());
	QWheelEvent translatedEvent(legacyPoint, wheelEvent->globalPos(), wheelEvent->pixelDelta(),
		wheelEvent->angleDelta(), wheelEvent->delta(), wheelEvent->orientation(),
		wheelEvent->buttons(), wheelEvent->modifiers(), wheelEvent->phase(), wheelEvent->source());
	QCoreApplication::sendEvent(this, &translatedEvent);
	updateSdkDiseaseOverlay();
	return translatedEvent.isAccepted();
}

bool hn2d3dPixBaseWidget::sdkScenePointToPixPoint(const QPointF& scenePos, pixImagePoint& point) const
{
	if (!m_sdkImageView)
	{
		return false;
	}

	QString imageName;
	int localX = 0;
	int localY = 0;
	if (!m_sdkImageView->GlobalSceneToMap(scenePos, imageName, localX, localY))
	{
		return false;
	}

	QString projectImageName;
	if (resolveSdkImageName(imageName, projectImageName))
	{
		point.pixName = projectImageName;
	}
	else
	{
		point.pixName = imageName;
	}
	point.pixPoint = QPoint(localX, localY);
	return true;
}

bool hn2d3dPixBaseWidget::sdkScenePointToDiseasePoint(const QPointF& scenePos, pixImagePoint& point) const
{
	if (!sdkScenePointToPixPoint(scenePos, point))
	{
		return false;
	}

	if (point.pixName.isEmpty())
	{
		return false;
	}

	if (point.pixPoint.x() < 0 || point.pixPoint.y() < 0 ||
		point.pixPoint.x() > m_pixWidth || point.pixPoint.y() > m_pixHeight)
	{
		return false;
	}

	return true;
}

QPoint hn2d3dPixBaseWidget::sdkPixPointToBigImagePoint(const pixImagePoint& point) const
{
	QString projectImageName;
	int frameIdx = -1;
	if (!resolveSdkImageName(point.pixName, projectImageName, &frameIdx) || frameIdx <= 0 || m_pixHeight <= 0)
	{
		return QPoint(-1, -1);
	}

	const QSize imageSize = currentPaintImageSize();
	const double difference = (frameIdx * 1.0 - this->m_buttomFrameIdx) * m_pixHeight;
	const double y = imageSize.height() - difference - (m_pixHeight - point.pixPoint.y());
	return QPoint(point.pixPoint.x(), qRound(y));
}

QPointF hn2d3dPixBaseWidget::sdkPixPointToScenePoint(const pixImagePoint& point) const
{
	if (!m_sdkImageView || point.pixName.isEmpty())
	{
		return QPointF();
	}

	// Normalize to the image name loaded by the SDK view. Database records,
	// project paths and SDK item names may differ by directory or prefix.
	QString sdkImageName = point.pixName;
	QString resolvedImageName;
	if (resolveSdkImageName(point.pixName, resolvedImageName))
	{
		sdkImageName = resolvedImageName;
	}
	return m_sdkImageView->mapToGlobalScene(sdkImageName, point.pixPoint.x(), point.pixPoint.y());
}

QRect hn2d3dPixBaseWidget::sdkPixRectToBigImageRect(const pixImagePoint& first, const pixImagePoint& second) const
{
	const QPoint p1 = sdkPixPointToBigImagePoint(first);
	const QPoint p2 = sdkPixPointToBigImagePoint(second);
	if (p1.x() < 0 || p2.x() < 0)
	{
		return QRect();
	}
	return QRect(p1, p2).normalized();
}

bool hn2d3dPixBaseWidget::currentSdkBigFrameSingleRects(QVector<SdkSingleImageRect>& rects) const
{
	rects.clear();
	if (!m_sdkImageView || m_diseaseStartPoint.pixName.isEmpty() || m_diseaseEndPoint.pixName.isEmpty() ||
		m_pixWidth <= 0 || m_pixHeight <= 0)
	{
		return false;
	}

	const QPointF startScene = sdkPixPointToScenePoint(m_diseaseStartPoint);
	const QPointF endScene = sdkPixPointToScenePoint(m_diseaseEndPoint);
	const QRectF normalizedSceneRect = QRectF(startScene, endScene).normalized();
	if (!normalizedSceneRect.isValid() || normalizedSceneRect.width() < 2.0 || normalizedSceneRect.height() < 2.0)
	{
		return false;
	}

	QString startImage;
	QString endImage;
	int startFrameIdx = -1;
	int endFrameIdx = -1;
	if (!resolveSdkImageName(m_diseaseStartPoint.pixName, startImage, &startFrameIdx) ||
		!resolveSdkImageName(m_diseaseEndPoint.pixName, endImage, &endFrameIdx) ||
		startFrameIdx <= 0 || endFrameIdx <= 0)
	{
		return false;
	}

	if (startFrameIdx > endFrameIdx)
	{
		qSwap(startFrameIdx, endFrameIdx);
	}

	for (int frameIdx = startFrameIdx; frameIdx <= endFrameIdx; ++frameIdx)
	{
		const QString pixName = m_pixNameMap.value(frameIdx);
		if (pixName.isEmpty())
		{
			continue;
		}

		const QPointF imageTopLeft = sdkPixPointToScenePoint(pixImagePoint{ pixName, QPoint(0, 0) });
		const QPointF imageBottomRight = sdkPixPointToScenePoint(pixImagePoint{ pixName, QPoint(m_pixWidth, m_pixHeight) });
		const QRectF imageSceneRect = QRectF(imageTopLeft, imageBottomRight).normalized();
		const QRectF overlap = normalizedSceneRect.intersected(imageSceneRect);
		if (!overlap.isValid() || overlap.width() < 2.0 || overlap.height() < 2.0)
		{
			continue;
		}

		const int left = qBound(0, qRound(overlap.left() - imageSceneRect.left()), m_pixWidth);
		const int top = qBound(0, qRound(overlap.top() - imageSceneRect.top()), m_pixHeight);
		const int right = qBound(0, qRound(overlap.right() - imageSceneRect.left()), m_pixWidth);
		const int bottom = qBound(0, qRound(overlap.bottom() - imageSceneRect.top()), m_pixHeight);

		SdkSingleImageRect sdkRect;
		sdkRect.pixName = pixName;
		sdkRect.singleRect = QRect(QPoint(left, top), QPoint(right, bottom)).normalized();
		if (sdkRect.isValid() && sdkRect.singleRect.width() >= 2 && sdkRect.singleRect.height() >= 2)
		{
			rects.append(sdkRect);
		}
	}

	return !rects.isEmpty();
}
bool hn2d3dPixBaseWidget::currentSdkBigFrameRect(QRect& rect) const
{
	rect = QRect();
	if (!m_sdkImageView)
	{
		return false;
	}
	if (m_diseaseStartPoint.pixName.isEmpty() || m_diseaseEndPoint.pixName.isEmpty())
	{
		return false;
	}

	const QPoint startPoint = sdkPixPointToBigImagePoint(m_diseaseStartPoint);
	const QPoint endPoint = sdkPixPointToBigImagePoint(m_diseaseEndPoint);
	if (startPoint.x() < 0 || startPoint.y() < 0 || endPoint.x() < 0 || endPoint.y() < 0)
	{
		#ifdef _DEBUG
		qWarning().noquote() << "[HN_SDK_BIG_FRAME_INVALID]"
			<< "reason=invalidPoint"
			<< "start=" << startPoint
			<< "end=" << endPoint
			<< "startImage=" << m_diseaseStartPoint.pixName
			<< "endImage=" << m_diseaseEndPoint.pixName;
		#endif
		return false;
	}

	const QRect candidate(startPoint, endPoint);
	rect = candidate.normalized();
	if (rect.width() < 2 || rect.height() < 2)
	{
		#ifdef _DEBUG
		qWarning().noquote() << "[HN_SDK_BIG_FRAME_INVALID]"
			<< "reason=flatRect"
			<< "rect=" << rect
			<< "startPix=" << m_diseaseStartPoint.pixPoint
			<< "endPix=" << m_diseaseEndPoint.pixPoint
			<< "startImage=" << m_diseaseStartPoint.pixName
			<< "endImage=" << m_diseaseEndPoint.pixName;
		#endif
		rect = QRect();
		return false;
	}
	return true;
}
bool hn2d3dPixBaseWidget::currentSdkBigFrameCornerPoints(QVector<pixImagePoint>& points) const
{
	points.clear();
	if (!m_sdkImageView || m_diseaseStartPoint.pixName.isEmpty() || m_diseaseEndPoint.pixName.isEmpty())
	{
		return false;
	}

	const QPointF startScene = sdkPixPointToScenePoint(m_diseaseStartPoint);
	const QPointF endScene = sdkPixPointToScenePoint(m_diseaseEndPoint);
	const QRectF sceneRect(startScene, endScene);
	const QRectF normalizedRect = sceneRect.normalized();
	if (!normalizedRect.isValid() || normalizedRect.width() < 2.0 || normalizedRect.height() < 2.0)
	{
		#ifdef _DEBUG
		qWarning().noquote() << "[HN_SDK_BIG_FRAME_INVALID]"
			<< "reason=flatSceneRect"
			<< "sceneRect=" << normalizedRect
			<< "startPix=" << m_diseaseStartPoint.pixPoint
			<< "endPix=" << m_diseaseEndPoint.pixPoint
			<< "startImage=" << m_diseaseStartPoint.pixName
			<< "endImage=" << m_diseaseEndPoint.pixName;
		#endif
		return false;
	}

	const QPointF sceneCorners[4] = {
		normalizedRect.topLeft(),
		normalizedRect.topRight(),
		normalizedRect.bottomRight(),
		normalizedRect.bottomLeft()
	};
	for (int i = 0; i < 4; ++i)
	{
		pixImagePoint point;
		if (!sdkScenePointToDiseasePoint(sceneCorners[i], point))
		{
			#ifdef _DEBUG
			qWarning().noquote() << "[HN_SDK_BIG_FRAME_INVALID]"
				<< "reason=cornerOutOfImage"
				<< "cornerIndex=" << i
				<< "scenePoint=" << sceneCorners[i]
				<< "sceneRect=" << normalizedRect;
			#endif
			points.clear();
			return false;
		}
		points.append(point);
	}
	return points.size() == 4;
}

void hn2d3dPixBaseWidget::addSdkTemporaryRect(const QRectF& rect, const QColor& color, int width, Qt::PenStyle style)
{
	m_sdkDiseaseGraphicsLayer.addTemporaryRect(rect, color, width, style);
}

void hn2d3dPixBaseWidget::addSdkTemporaryLinePath(const QVector<pixImagePoint>& points, const QColor& color, int width, Qt::PenStyle style, const pixImagePoint* tailPoint)
{
	if (!m_sdkImageView || !m_sdkImageView->scene() || points.isEmpty())
	{
		return;
	}

	QPainterPath path;
	bool hasStart = false;
	for (const pixImagePoint& point : qAsConst(points))
	{
		const QPointF scenePoint = sdkPixPointToScenePoint(point);
		if (!hasStart)
		{
			path.moveTo(scenePoint);
			hasStart = true;
		}
		else
		{
			path.lineTo(scenePoint);
		}
	}

	if (tailPoint && !tailPoint->pixName.isEmpty())
	{
		path.lineTo(sdkPixPointToScenePoint(*tailPoint));
	}

	m_sdkDiseaseGraphicsLayer.addTemporaryPath(path, color, width, style);
}

void hn2d3dPixBaseWidget::addSdkTemporaryBigImageRects(const QVector<QRect>& rects, const QColor& color, int width, Qt::PenStyle style)
{
	if (rects.isEmpty())
	{
		return;
	}

	QPainterPath path;
	for (const QRect& bigRect : qAsConst(rects))
	{
		QString pixName;
		const QRect singleRect = bigImageRectToSingleImageRect(bigRect, &pixName).normalized();
		if (pixName.isEmpty() || !singleRect.isValid() || singleRect.isNull())
		{
			continue;
		}

		pixImagePoint p1;
		p1.pixName = pixName;
		p1.pixPoint = singleRect.topLeft();
		pixImagePoint p2;
		p2.pixName = pixName;
		p2.pixPoint = singleRect.bottomRight();
		const QRectF sceneRect(QRectF(sdkPixPointToScenePoint(p1), sdkPixPointToScenePoint(p2)).normalized());
		if (sceneRect.isValid() && !sceneRect.isNull())
		{
			path.addRect(sceneRect);
		}
	}

	if (!path.isEmpty())
	{
		m_sdkDiseaseGraphicsLayer.addTemporaryPath(path, color, width, style);
	}
}
void hn2d3dPixBaseWidget::beginSdkDiseaseAt(const pixImagePoint& point)
{
	m_waitLittleFrameLeftPressAfterCancel = false;
	m_diseaseStartPoint = point;
	m_diseaseEndPoint = point;
	if (!sdkHnMileFromPoint(point, m_firstHnMile))
	{
		qWarning().noquote() << "[HN_SDK_DISEASE_CONTEXT_FAIL]" << "image=" << point.pixName << "point=" << point.pixPoint;
		resetSdkDiseaseDrawingState(true, false);
		return;
	}
	m_isDrawingDisease = true;
	m_isAllowDrawPix = false;
	m_isAllowLinked = false;
	setCursor(Qt::CrossCursor);
	updateSdkDiseaseOverlay();
	refreshSdkTemporaryDiseaseItems();
}

bool hn2d3dPixBaseWidget::finishSdkBigFrameDisease()
{
	clearLastSdkAddedDisease();
	const bool ok = sdkCommitBigFrameDisease();
	m_isDrawingDisease = false;
	// SDK 视图存在时旧 paintEvent 不再负责显示，不能在清理绘制状态时重新打开旧绘图。
	m_isAllowDrawPix = (m_sdkImageView == nullptr);
	m_isAllowLinked = true;
	if (ok)
	{
		m_sdkDiseaseGraphicsLayer.discardTemporary();
		hnRoadDiseaseInfo addedDisease;
		if (takeLastSdkAddedDisease(addedDisease))
		{
			setSelectedDisease(addedDisease);
			ensureSdkDiseaseItemVisible(addedDisease);
			emit signal_selectDisease(addedDisease);
		}
		else
		{
			refreshSdkDiseaseItems();
		}
	}
	else
	{
		resetSdkDiseaseDrawingState(false, false);
		clearLastSdkAddedDisease();
	}
	updateSdkDiseaseOverlay();
	return ok;
}

bool hn2d3dPixBaseWidget::finishSdkLittleFrameDisease()
{
	prepareSdkLittleFrameSelectionsForCommit();
	clearLastSdkAddedDisease();
	const bool ok = sdkCommitLittleFrameDisease();
	m_isDrawingDisease = false;
	// SDK 视图存在时旧 paintEvent 不再负责显示，不能在清理绘制状态时重新打开旧绘图。
	m_isAllowDrawPix = (m_sdkImageView == nullptr);
	m_isAllowLinked = true;
	if (ok)
	{
		m_sdkDiseaseGraphicsLayer.discardTemporary();
		hnRoadDiseaseInfo addedDisease;
		if (takeLastSdkAddedDisease(addedDisease))
		{
			setSelectedDisease(addedDisease);
			ensureSdkDiseaseItemVisible(addedDisease);
			emit signal_selectDisease(addedDisease);
		}
		else
		{
			refreshSdkDiseaseItems();
		}
	}
	else
	{
		resetSdkDiseaseDrawingState(false, false);
		clearLastSdkAddedDisease();
	}
	updateSdkDiseaseOverlay();
	return ok;
}

bool hn2d3dPixBaseWidget::finishSdkLineLittleFrameDisease()
{
	m_isEndAddPoint = true;
	m_tempPoints.clear();
	return finishSdkLittleFrameDisease();
}

void hn2d3dPixBaseWidget::updateSdkLittleFramePreview()
{
	if (!m_isDrawingDisease || m_frameMode != FrameMode::LITTLE_FRAME)
	{
		return;
	}

	// 绘制阶段只刷新轻量预览：R 模式显示外接框，普通/B 模式显示轨迹线。
	// 真正的小格集合统一在提交前生成，避免拖动或滚动时实时铺满小框导致卡顿。
}

void hn2d3dPixBaseWidget::prepareSdkLittleFrameSelectionsForCommit()
{
	if (m_frameMode != FrameMode::LITTLE_FRAME)
	{
		return;
	}

	m_currentLittleFrameSingleSelections.clear();
	m_committedLittleFrameDiseaseRects.clear();

	if (littleDrawRectType)
	{
		rebuildCurrentLittleFrameSingleSelectionsFromRect(m_diseaseStartPoint, m_diseaseEndPoint);
		return;
	}

	QVector<pixImagePoint> points = m_littleSingleImagePoints;
	if (!addLineDiseType && !m_diseaseEndPoint.pixName.isEmpty())
	{
		if (points.isEmpty() || points.last().pixName != m_diseaseEndPoint.pixName || points.last().pixPoint != m_diseaseEndPoint.pixPoint)
		{
			points.append(m_diseaseEndPoint);
		}
	}

	rebuildCurrentLittleFrameSingleSelectionsFromPoints(points);
}
void hn2d3dPixBaseWidget::refreshSdkTemporaryDiseaseItems()
{
	m_sdkDiseaseGraphicsLayer.clearTemporary();

    if (!m_sdkImageView)
    {
        return;
    }

    if (m_workMode == WorkMode::GET_MILE && !m_seclectPoint.pixName.isEmpty())
    {
        pixImagePoint startPoint;
        startPoint.pixName = m_seclectPoint.pixName;
        startPoint.pixPoint = QPoint(0, m_seclectPoint.pixPoint.y());
        pixImagePoint endPoint;
        endPoint.pixName = m_seclectPoint.pixName;
        endPoint.pixPoint = QPoint(m_pixWidth, m_seclectPoint.pixPoint.y());
        QVector<pixImagePoint> linePoints;
        linePoints << startPoint << endPoint;
        addSdkTemporaryLinePath(linePoints, Qt::yellow, 3, Qt::SolidLine, nullptr);
        return;
    }

    if (!m_isDrawingDisease)
    {
        return;
    }

	if (m_frameMode == FrameMode::BIG_FRAME || m_frameMode == FrameMode::DESIGN_FACETS)
	{
		addSdkTemporaryRect(QRectF(sdkPixPointToScenePoint(m_diseaseStartPoint), sdkPixPointToScenePoint(m_diseaseEndPoint)).normalized(),
			m_diseaseDrawStyle.tempRectColor, m_diseaseDrawStyle.tempRectWidth, Qt::DashLine);
		return;
	}

	if (m_frameMode == FrameMode::DESIGN_LINE)
	{
		pixImagePoint tail = m_diseaseEndPoint;
		addSdkTemporaryLinePath(m_tmpLineDiseasePoints, m_diseaseDrawStyle.tempLineDiseaseColor,
			m_diseaseDrawStyle.tempLineDiseaseWidth, Qt::DashLine, &tail);
		return;
	}

	if (m_frameMode == FrameMode::LITTLE_FRAME)
	{
		if (addLineDiseType)
		{
			pixImagePoint tail = m_diseaseEndPoint;
			addSdkTemporaryLinePath(m_littleSingleImagePoints, Qt::yellow, m_diseaseDrawStyle.tempLineDiseaseWidth, Qt::DashLine, &tail);
			return;
		}

		if (littleDrawRectType)
		{
			if (!m_diseaseStartPoint.pixName.isEmpty() && !m_diseaseEndPoint.pixName.isEmpty())
			{
				addSdkTemporaryRect(QRectF(sdkPixPointToScenePoint(m_diseaseStartPoint), sdkPixPointToScenePoint(m_diseaseEndPoint)).normalized(),
					m_diseaseDrawStyle.littleFrameRectColor, m_diseaseDrawStyle.tempRectWidth, Qt::DashLine);
			}
			return;
		}

		// 普通小框轨迹模式只画轨迹线，不在绘制阶段实时铺满小格。
		pixImagePoint tail = m_diseaseEndPoint;
		addSdkTemporaryLinePath(m_littleSingleImagePoints, Qt::yellow, m_diseaseDrawStyle.tempLineDiseaseWidth, Qt::DashLine, &tail);
		return;
	}
}

bool hn2d3dPixBaseWidget::adjustSdkDiseasePointToValidArea(pixImagePoint& point, bool clampToArea) const
{
	Q_UNUSED(point);
	Q_UNUSED(clampToArea);
	return true;
}

bool hn2d3dPixBaseWidget::validateDiseaseGeometryWithinValidArea(const hnRoadDiseaseInfo& disease) const
{
	Q_UNUSED(disease);
	return true;
}
bool hn2d3dPixBaseWidget::handleSdkDiseaseMousePress(QMouseEvent* mouseEvent)
{
	if (!mouseEvent || !m_sdkImageView)
	{
		return false;
	}

	pixImagePoint point;
	const QPointF scenePos = m_sdkImageView->mapToScene(mouseEvent->pos());
	if (!sdkScenePointToDiseasePoint(scenePos, point))
	{
		return false;
	}

	if (mouseEvent->button() == Qt::LeftButton && m_workMode == WorkMode::GET_MILE)
	{
		m_encoderMile = sdkPixPointToEncoderMile(point);
		m_seclectPoint = point;
        refreshSdkTemporaryDiseaseItems();
		updateSdkDiseaseOverlay();
        if (m_sdkImageView->scene())
        {
            m_sdkImageView->scene()->update();
        }
        if (m_sdkImageView && m_sdkImageView->viewport())
        {
            m_sdkImageView->viewport()->update();
        }
        update();
		if (m_sdkDiseaseGraphicsLayer.diseaseKeyAt(scenePos).isEmpty())
		{
			showSdkNoDiseaseModeHint(mouseEvent->pos());
		}
		return true;
	}

	if (m_workMode == WorkMode::DELETE_MODE && m_frameMode == FrameMode::LITTLE_FRAME)
	{
		return handleSdkLittleFrameDeleteMousePress(mouseEvent, point);
	}

	if (m_workMode != WorkMode::ADD_MODE)
	{
		return false;
	}
	if (!adjustSdkDiseasePointToValidArea(point, false))
	{
		return true;
	}

	if (mouseEvent->button() == Qt::RightButton)
	{
		if (!m_isDrawingDisease)
		{
			return false;
		}

		if (m_frameMode == FrameMode::DESIGN_LINE)
		{
			m_tmpPaintLineDiseasePoints = m_tmpLineDiseasePoints;
			lineDiseaseAddDisease();
		}
		else if (m_frameMode == FrameMode::LITTLE_FRAME && addLineDiseType)
		{
			finishSdkLineLittleFrameDisease();
		}
		else
		{
			slot_cancelDrawDiseases();
			m_sdkDiseaseGraphicsLayer.clearTemporary();
		}
		return true;
	}

	if (mouseEvent->button() != Qt::LeftButton)
	{
		return false;
	}

	// 取消病害选择后，只有下一次左键按下才能重新开启小框绘制。
	m_waitLittleFrameLeftPressAfterCancel = false;

	if (m_frameMode == FrameMode::BIG_FRAME || m_frameMode == FrameMode::DESIGN_FACETS)
	{
		if (!m_isDrawingDisease)
		{
			beginSdkDiseaseAt(point);
		}
		return true;
	}

	if (m_frameMode == FrameMode::DESIGN_LINE)
	{
		m_isDrawingDisease = true;
		m_isAllowDrawPix = false;
		if (m_tmpLineDiseasePoints.isEmpty())
		{
			m_diseaseStartPoint = point;
			sdkHnMileFromPoint(point, m_firstHnMile);
		}
		m_tmpLineDiseasePoints.append(point);
		m_tmpLastPaintLineDiseasePoints.clear();
		m_diseaseEndPoint = point;
		refreshSdkTemporaryDiseaseItems();
		return true;
	}

	if (m_frameMode == FrameMode::LITTLE_FRAME)
	{
		if (addLineDiseType)
		{
			m_isDrawingDisease = true;
			m_isAllowDrawPix = false;
			m_isAllowLinked = false;
			m_diseaseAddPoint = point;
			m_diseaseEndPoint = point;
			if (m_littleSingleImagePoints.isEmpty())
			{
				m_diseaseStartPoint = point;
				sdkHnMileFromPoint(point, m_firstHnMile);
			}
			m_littleSingleImagePoints.append(point);
			rebuildLittleFrameBigImagePoints();
			m_tmpPaintLineDiseasePoints = m_tmpLineDiseasePoints;
			m_tmpPaintLineDiseasePoints.append(point);
			m_tmpLastPaintLineDiseasePoints.append(point);
			refreshSdkTemporaryDiseaseItems();
			return true;
		}

		if (!littleDrawRectType)
		{
			if (m_isDrawingDisease)
			{
				updateSdkLittleFramePreview();
				refreshSdkTemporaryDiseaseItems();
				return finishSdkLittleFrameDisease();
			}

			resetLittleFrameDrawState();
			beginSdkDiseaseAt(point);
			appendLittleFrameDrawingPoint(point);
			return true;
		}

		if (!m_isDrawingDisease)
		{
			resetLittleFrameDrawState();
			beginSdkDiseaseAt(point);
		}
		return true;
	}

	return false;
}
bool hn2d3dPixBaseWidget::handleSdkLittleFrameDeleteMousePress(QMouseEvent* mouseEvent, const pixImagePoint& point)
{
	if (!mouseEvent || m_frameMode != FrameMode::LITTLE_FRAME || m_workMode != WorkMode::DELETE_MODE)
	{
		return false;
	}

	if (mouseEvent->button() != Qt::LeftButton && mouseEvent->button() != Qt::RightButton)
	{
		return false;
	}

	hnRoadDiseaseInfo disease;
	int hitIndex = -1;
	if (!findSdkLittleFrameDiseaseAtPoint(point, disease, hitIndex))
	{
		showSdkNoDiseaseModeHint(mouseEvent->pos());
		return true;
	}

	if (mouseEvent->button() == Qt::RightButton)
	{
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(disease);
		m_seclectedDiseases.clear();
		m_selectedSdkDiseaseKey.clear();
		clearSdkLittleFrameRenderCache();
		m_sdkDiseaseGraphicsLayer.clearTemporary();
		refreshSdkDiseaseLayer();
		return true;
	}

	const int beforeCellCount = qMax(disease.vec2dRect.size(), disease.vec3dRect.size());
	if (beforeCellCount <= 1)
	{
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(disease);
		m_seclectedDiseases.clear();
		m_selectedSdkDiseaseKey.clear();
		clearSdkLittleFrameRenderCache();
		m_sdkDiseaseGraphicsLayer.clearTemporary();
		refreshSdkDiseaseLayer();
		return true;
	}

	if (hitIndex >= 0 && hitIndex < disease.vec2dRect.size())
	{
		disease.vec2dRect.erase(disease.vec2dRect.begin() + hitIndex);
	}
	if (hitIndex >= 0 && hitIndex < disease.vec3dRect.size())
	{
		disease.vec3dRect.erase(disease.vec3dRect.begin() + hitIndex);
	}

	recalculateLittleFrameDiseaseAfterCellDelete(disease);
	updateSdkLittleFrameDiseaseAfterCellDelete(disease);
	clearSdkLittleFrameRenderCache();
	m_sdkDiseaseGraphicsLayer.clearTemporary();

	if (!disease.vec2dRect.empty() || !disease.vec3dRect.empty())
	{
		setSelectedDisease(disease);
		emit signal_selectDisease(disease);
	}
	else
	{
		m_seclectedDiseases.clear();
		m_selectedSdkDiseaseKey.clear();
	}
	refreshSdkDiseaseLayer();
	return true;
}
bool hn2d3dPixBaseWidget::handleSdkDiseaseMouseMove(QMouseEvent* mouseEvent)
{
	if (!mouseEvent || !m_sdkImageView || !m_isDrawingDisease || m_waitLittleFrameLeftPressAfterCancel)
	{
		return false;
	}

	pixImagePoint point;
	if (!sdkScenePointToDiseasePoint(m_sdkImageView->mapToScene(mouseEvent->pos()), point))
	{
		return true;
	}
	const bool clampToArea = m_frameMode == FrameMode::BIG_FRAME ||
		(m_frameMode == FrameMode::LITTLE_FRAME && littleDrawRectType);
	if (!adjustSdkDiseasePointToValidArea(point, clampToArea))
	{
		return true;
	}

	if (m_widgetType == WIDGET_3D && hnDataManager::getDataManager()->getCurrentProject() &&
		PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
	{
		int x = point.pixPoint.x();
		autoCorrectXIn3dView(x);
		point.pixPoint.setX(x);
	}

	if (isSdkPlainLittleFrameDrawing() && m_ignoreNextMouseMoveAfterAutoCursorMove)
	{
		m_ignoreNextMouseMoveAfterAutoCursorMove = false;
		m_diseaseEndPoint = point;
		updateSdkDiseaseOverlay();
		updateSdkLittleFramePreview();
		refreshSdkTemporaryDiseaseItems();
		if (mouseEvent)
		{
			mouseEvent->accept();
		}
		return true;
	}

	m_diseaseEndPoint = point;
	updateSdkDiseaseOverlay();

	if (m_frameMode == FrameMode::LITTLE_FRAME)
	{
		if (addLineDiseType)
		{
			m_tempPoints.clear();
			if (!m_littleSingleImagePoints.isEmpty())
			{
				m_tempPoints.push_back(m_littleSingleImagePoints.last());
				m_tempPoints.push_back(point);
			}
		}
		else if (!littleDrawRectType)
		{
			appendLittleFrameDrawingPoint(point);
			rebuildLittleFrameBigImagePoints();
		}
		updateSdkLittleFramePreview();
	}

	if (m_frameMode == FrameMode::DESIGN_LINE)
	{
		m_tempPoints.clear();
		if (!m_tmpLineDiseasePoints.isEmpty())
		{
			m_tempPoints.push_back(m_tmpLineDiseasePoints.last());
			m_tempPoints.push_back(point);
		}
	}

	refreshSdkTemporaryDiseaseItems();
	return true;
}

bool hn2d3dPixBaseWidget::handleSdkDiseaseMouseRelease(QMouseEvent* mouseEvent)
{
	if (!mouseEvent || !m_sdkImageView || m_workMode != WorkMode::ADD_MODE)
	{
		return false;
	}

	if (mouseEvent->button() == Qt::LeftButton && m_frameMode == FrameMode::LITTLE_FRAME && !addLineDiseType && !littleDrawRectType && !m_isDrawingDisease)
	{
		return true;
	}

	if (!m_isDrawingDisease)
	{
		return false;
	}

	if (mouseEvent->button() != Qt::LeftButton)
	{
		return false;
	}

	pixImagePoint point;
	if (sdkScenePointToDiseasePoint(m_sdkImageView->mapToScene(mouseEvent->pos()), point))
	{
		const bool clampToArea = m_frameMode == FrameMode::BIG_FRAME ||
			(m_frameMode == FrameMode::LITTLE_FRAME && littleDrawRectType);
		if (!adjustSdkDiseasePointToValidArea(point, clampToArea))
		{
			return true;
		}
		if (m_widgetType == WIDGET_3D && hnDataManager::getDataManager()->getCurrentProject() &&
			PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
		{
			int x = point.pixPoint.x();
			autoCorrectXIn3dView(x);
			point.pixPoint.setX(x);
		}
		m_diseaseEndPoint = point;
	}
	else if (m_diseaseEndPoint.pixName.isEmpty() || m_diseaseEndPoint.pixPoint.x() < 0 || m_diseaseEndPoint.pixPoint.y() < 0)
	{
		return true;
	}

	if (m_frameMode == FrameMode::BIG_FRAME || m_frameMode == FrameMode::DESIGN_FACETS)
	{
		refreshSdkTemporaryDiseaseItems();
		return finishSdkBigFrameDisease();
	}

	if (m_frameMode == FrameMode::LITTLE_FRAME && !addLineDiseType && littleDrawRectType)
	{
		updateSdkLittleFramePreview();
		refreshSdkTemporaryDiseaseItems();
		return finishSdkLittleFrameDisease();
	}

	return true;
}

QString hn2d3dPixBaseWidget::sdkWorkModeHintText() const
{
	switch (m_workMode)
	{
	case WorkMode::NO_MODE:
		return QString::fromUtf8("\xE5\xBD\x93\xE5\x89\x8D\xE4\xB8\xBA\xE6\xB5\x8F\xE8\xA7\x88\xE6\xA8\xA1\xE5\xBC\x8F\xEF\xBC\x9A\xE6\xAD\xA4\xE5\xA4\x84\xE6\xB2\xA1\xE6\x9C\x89\xE7\x97\x85\xE5\xAE\xB3\xE3\x80\x82\xE6\x8C\x89" " F1 " "\xE6\xB7\xBB\xE5\x8A\xA0\xE3\x80\x81" "F2 " "\xE5\x88\xA0\xE9\x99\xA4\xE3\x80\x81" "F3 " "\xE7\xBC\x96\xE8\xBE\x91\xE3\x80\x81" "F4 " "\xE5\x90\x88\xE5\xB9\xB6\xE3\x80\x82");
	case WorkMode::DELETE_MODE:
		return QString::fromUtf8("\xE5\xBD\x93\xE5\x89\x8D\xE4\xB8\xBA\xE5\x88\xA0\xE9\x99\xA4\xE7\x97\x85\xE5\xAE\xB3\xE6\xA8\xA1\xE5\xBC\x8F\xEF\xBC\x9A\xE8\xAF\xB7\xE7\x82\xB9\xE5\x87\xBB\xE7\x97\x85\xE5\xAE\xB3\xEF\xBC\x9B\xE6\xAD\xA4\xE5\xA4\x84\xE6\xB2\xA1\xE6\x9C\x89\xE7\x97\x85\xE5\xAE\xB3\xE3\x80\x82");
	case WorkMode::EDIT_MODE:
		return QString::fromUtf8("\xE5\xBD\x93\xE5\x89\x8D\xE4\xB8\xBA\xE7\xBC\x96\xE8\xBE\x91\xE7\x97\x85\xE5\xAE\xB3\xE6\xA8\xA1\xE5\xBC\x8F\xEF\xBC\x9A\xE5\x8D\x95\xE5\x87\xBB\xE7\x97\x85\xE5\xAE\xB3\xE4\xBF\xAE\xE6\x94\xB9\xE7\xB1\xBB\xE5\x9E\x8B\xEF\xBC\x9B\xE6\x8B\x96\xE5\x8A\xA8\xE9\x9D\xA2\xE7\x8A\xB6\xE7\x97\x85\xE5\xAE\xB3\xE4\xB8\xBB\xE4\xBD\x93\xE5\x8F\xAF\xE6\x95\xB4\xE4\xBD\x93\xE7\xA7\xBB\xE5\x8A\xA8\xEF\xBC\x9B\xE6\x8B\x96\xE5\x8A\xA8\xE5\x9B\x9B\xE8\xA7\x92\xE5\x8F\xAF\xE8\xB0\x83\xE6\x95\xB4\xE5\xA4\xA7\xE5\xB0\x8F\xE3\x80\x82");
	case WorkMode::MOVE:
		return QString::fromUtf8("\xE5\xBD\x93\xE5\x89\x8D\xE4\xB8\xBA\xE7\xA7\xBB\xE5\x8A\xA8\xE7\x97\x85\xE5\xAE\xB3\xE6\xA8\xA1\xE5\xBC\x8F\xEF\xBC\x9A\xE8\xAF\xB7\xE7\x82\xB9\xE5\x87\xBB\xE7\x97\x85\xE5\xAE\xB3\xEF\xBC\x9B\xE6\xAD\xA4\xE5\xA4\x84\xE6\xB2\xA1\xE6\x9C\x89\xE7\x97\x85\xE5\xAE\xB3\xE3\x80\x82");
	case WorkMode::MERGE:
		return QString::fromUtf8("\xE5\xBD\x93\xE5\x89\x8D\xE4\xB8\xBA\xE5\x90\x88\xE5\xB9\xB6\xE7\x97\x85\xE5\xAE\xB3\xE6\xA8\xA1\xE5\xBC\x8F\xEF\xBC\x9A\xE8\xAF\xB7\xE4\xBE\x9D\xE6\xAC\xA1\xE7\x82\xB9\xE5\x87\xBB\xE4\xB8\xA4\xE4\xB8\xAA\xE5\x90\x8C\xE7\xB1\xBB\xE5\x9E\x8B\xE7\x97\x85\xE5\xAE\xB3\xEF\xBC\x9B\xE6\xAD\xA4\xE5\xA4\x84\xE6\xB2\xA1\xE6\x9C\x89\xE7\x97\x85\xE5\xAE\xB3\xE3\x80\x82");
	case WorkMode::GET_MILE:
		return QString::fromUtf8("\xE5\xBD\x93\xE5\x89\x8D\xE4\xB8\xBA\xE5\x8F\x96\xE9\x87\x8C\xE7\xA8\x8B\xE6\xA8\xA1\xE5\xBC\x8F\xE3\x80\x82");
	case WorkMode::ADD_CTRL_POINT:
		return QString::fromUtf8("\xE5\xBD\x93\xE5\x89\x8D\xE4\xB8\xBA\xE6\xB7\xBB\xE5\x8A\xA0\xE6\x8E\xA7\xE5\x88\xB6\xE7\x82\xB9\xE6\xA8\xA1\xE5\xBC\x8F\xE3\x80\x82");
	default:
		return QString::fromUtf8("\xE5\xBD\x93\xE5\x89\x8D\xE6\xA8\xA1\xE5\xBC\x8F\xE4\xB8\x8B\xE6\xAD\xA4\xE5\xA4\x84\xE6\xB2\xA1\xE6\x9C\x89\xE7\x97\x85\xE5\xAE\xB3\xE3\x80\x82");
	}
}

void hn2d3dPixBaseWidget::showSdkNoDiseaseModeHint(const QPoint& viewportPoint)
{
	if (!m_sdkImageView || !m_sdkImageView->viewport() || m_workMode == WorkMode::ADD_MODE)
	{
		return;
	}
	const qint64 now = QDateTime::currentMSecsSinceEpoch();
	if (now - m_lastSdkNoDiseaseHintMs < 800)
	{
		return;
	}
	m_lastSdkNoDiseaseHintMs = now;
	QToolTip::showText(
		m_sdkImageView->viewport()->mapToGlobal(viewportPoint),
		sdkWorkModeHintText(),
		m_sdkImageView->viewport(),
		QRect(),
		2600);
}

void hn2d3dPixBaseWidget::resetSdkEditGesture()
{
	m_sdkEditPressPending = false;
	m_sdkAreaMoveCandidate = false;
	m_sdkAreaMoveDragging = false;
	m_sdkEditLegacyPoint = QPoint(-1, -1);
	if (m_sdkImageView && !m_sdkAreaResizeDragging)
	{
		m_sdkImageView->unsetCursor();
	}
}
bool hn2d3dPixBaseWidget::selectedSdkAreaDisease(hnRoadDiseaseInfo& disease) const
{
	if (m_workMode != WorkMode::EDIT_MODE || m_selectedSdkDiseaseKey.isEmpty())
	{
		return false;
	}
	for (const hnRoadDiseaseInfo& candidate : m_seclectedDiseases)
	{
		if (isSameSdkDisease(candidate, m_selectedSdkDiseaseKey)
			&& (candidate.nDrawType == 0 || candidate.nDrawType == 1 || candidate.nDrawType == 2))
		{
			disease = candidate;
			return true;
		}
	}
	for (const hnRoadDiseaseInfo& candidate : m_currentWidgetDiseases)
	{
		if (isSameSdkDisease(candidate, m_selectedSdkDiseaseKey)
			&& (candidate.nDrawType == 0 || candidate.nDrawType == 1 || candidate.nDrawType == 2))
		{
			disease = candidate;
			return true;
		}
	}
	return false;
}

void hn2d3dPixBaseWidget::refreshSdkAreaResizeHandles()
{
	if (!m_sdkImageView || m_workMode != WorkMode::EDIT_MODE)
	{
		return;
	}
	m_sdkDiseaseGraphicsLayer.clearTemporary();
	hnRoadDiseaseInfo disease;
	if (!selectedSdkAreaDisease(disease))
	{
		return;
	}
	QPainterPath diseasePath = sdkDiseaseScenePath(disease);
	QRectF rect = (m_sdkAreaResizeDragging || m_sdkAreaMoveDragging)
		? m_sdkAreaResizeRect.normalized()
		: diseasePath.boundingRect().normalized();
	if (!rect.isValid() || rect.isEmpty())
	{
		return;
	}
	if (disease.nDrawType == 1)
	{
		if (m_sdkAreaMoveDragging)
		{
			diseasePath.translate(m_sdkAreaResizeRect.topLeft() - m_sdkAreaMoveOriginalRect.topLeft());
		}
		m_sdkDiseaseGraphicsLayer.addTemporaryPath(diseasePath, QColor(255, 230, 0), 3, Qt::DashLine);
		if (m_sdkImageView->viewport())
		{
			m_sdkImageView->viewport()->update();
		}
		return;
	}
	m_sdkDiseaseGraphicsLayer.addTemporaryRect(rect, QColor(255, 230, 0), 3, Qt::DashLine);
	const qreal scaleX = qMax<qreal>(0.01, qAbs(m_sdkImageView->transform().m11()));
	const qreal scaleY = qMax<qreal>(0.01, qAbs(m_sdkImageView->transform().m22()));
	const qreal halfWidth = 7.0 / scaleX;
	const qreal halfHeight = 7.0 / scaleY;
	const QPointF corners[] = { rect.topLeft(), rect.topRight(), rect.bottomRight(), rect.bottomLeft() };
	for (const QPointF& corner : corners)
	{
		m_sdkDiseaseGraphicsLayer.addTemporaryRect(
			QRectF(corner.x() - halfWidth, corner.y() - halfHeight,
				halfWidth * 2.0, halfHeight * 2.0),
			QColor(255, 230, 0), 4, Qt::SolidLine);
	}
	if (m_sdkImageView->viewport())
	{
		m_sdkImageView->viewport()->update();
	}
}

bool hn2d3dPixBaseWidget::handleSdkAreaResizeMousePress(QMouseEvent* mouseEvent)
{
	if (!mouseEvent || !m_sdkImageView || m_workMode != WorkMode::EDIT_MODE
		|| mouseEvent->button() != Qt::LeftButton)
	{
		return false;
	}

	hnRoadDiseaseInfo selectedDisease;
	if (selectedSdkAreaDisease(selectedDisease) && selectedDisease.nDrawType != 1)
	{
		const QRectF rect = sdkDiseaseScenePath(selectedDisease).boundingRect().normalized();
		const QPointF corners[] = { rect.topLeft(), rect.topRight(), rect.bottomRight(), rect.bottomLeft() };
		for (int i = 0; i < 4; ++i)
		{
			const QPoint handlePoint = m_sdkImageView->mapFromScene(corners[i]);
			if (QLineF(handlePoint, mouseEvent->pos()).length() <= 14.0)
			{
				resetSdkEditGesture();
				m_sdkAreaResizeCorner = i;
				m_sdkAreaResizeDragging = true;
				m_sdkAreaResizeRect = rect;
				m_sdkAreaResizeDisease = selectedDisease;
				m_sdkAreaResizeOpposite = corners[(i + 2) % 4];
				m_sdkImageView->setCursor(
					(i == 0 || i == 2) ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor);
				refreshSdkAreaResizeHandles();
				return true;
			}
		}
	}

	const QPointF scenePoint = m_sdkImageView->mapToScene(mouseEvent->pos());
	const QString diseaseKey = m_sdkDiseaseGraphicsLayer.diseaseKeyAt(scenePoint);
	if (diseaseKey.isEmpty())
	{
		resetSdkEditGesture();
		m_seclectedDiseases.clear();
		m_selectedSdkDiseaseKey.clear();
		hnBrowsePixWidget::setSelectedDiseaseId(-1);
		m_sdkDiseaseGraphicsLayer.setSelectedDiseaseKey(QString());
		refreshSdkDiseaseItems();
		refreshSdkAreaResizeHandles();
		showSdkNoDiseaseModeHint(mouseEvent->pos());
		return true;
	}

	for (const hnRoadDiseaseInfo& disease : m_currentWidgetDiseases)
	{
		if (!isSameSdkDisease(disease, diseaseKey))
		{
			continue;
		}

		const hnRoadDiseaseInfo selected = disease;
		setSelectedDisease(selected);
		emit signal_selectDisease(selected);
		m_sdkEditDisease = selected;
		m_sdkEditPressPending = true;
		m_sdkEditPressViewportPoint = mouseEvent->pos();
		pixImagePoint hitPoint;
		m_sdkEditLegacyPoint = sdkScenePointToDiseasePoint(scenePoint, hitPoint)
			? sdkPixPointToBigImagePoint(hitPoint) : QPoint(-1, -1);
		m_sdkAreaMoveCandidate = selected.nDrawType == 0 || selected.nDrawType == 1 || selected.nDrawType == 2;
		m_sdkAreaMoveDragging = false;
		if (m_sdkAreaMoveCandidate)
		{
			m_sdkAreaMovePressScene = scenePoint;
			m_sdkAreaMoveOriginalRect = sdkDiseaseScenePath(selected).boundingRect().normalized();
			m_sdkAreaResizeRect = m_sdkAreaMoveOriginalRect;
			m_sdkAreaResizeDisease = selected;
		}
		refreshSdkAreaResizeHandles();
		return true;
	}
	return true;
}

bool hn2d3dPixBaseWidget::handleSdkAreaResizeMouseMove(QMouseEvent* mouseEvent)
{
	if (!mouseEvent || !m_sdkImageView)
	{
		return false;
	}

	if (m_sdkAreaResizeDragging)
	{
		pixImagePoint point;
		if (!sdkScenePointToDiseasePoint(m_sdkImageView->mapToScene(mouseEvent->pos()), point))
		{
			return true;
		}
		if (!adjustSdkDiseasePointToValidArea(point, true))
		{
			return true;
		}
		const QPointF scenePoint = sdkPixPointToScenePoint(point);
		m_sdkAreaResizeRect = QRectF(m_sdkAreaResizeOpposite, scenePoint).normalized();
		refreshSdkAreaResizeHandles();
		return true;
	}

	if (!m_sdkEditPressPending || !m_sdkAreaMoveCandidate
		|| !(mouseEvent->buttons() & Qt::LeftButton))
	{
		return false;
	}
	if (!m_sdkAreaMoveDragging
		&& QLineF(mouseEvent->pos(), m_sdkEditPressViewportPoint).length() < 5.0)
	{
		return true;
	}

	const QPointF scenePoint = m_sdkImageView->mapToScene(mouseEvent->pos());
	const QRectF candidate = m_sdkAreaMoveOriginalRect.translated(scenePoint - m_sdkAreaMovePressScene);
	pixImagePoint startPoint;
	pixImagePoint endPoint;
	if (!sdkScenePointToDiseasePoint(candidate.topLeft(), startPoint)
		|| !sdkScenePointToDiseasePoint(candidate.bottomRight(), endPoint))
	{
		return true;
	}
	m_sdkAreaMoveDragging = true;
	m_sdkAreaResizeRect = candidate;
	m_sdkImageView->setCursor(Qt::SizeAllCursor);
	refreshSdkAreaResizeHandles();
	return true;
}

bool hn2d3dPixBaseWidget::handleSdkAreaResizeMouseRelease(QMouseEvent* mouseEvent)
{
	if (!mouseEvent || !m_sdkImageView || mouseEvent->button() != Qt::LeftButton)
	{
		return false;
	}

	if (m_sdkAreaResizeDragging)
	{
		handleSdkAreaResizeMouseMove(mouseEvent);
		const QRectF rect = m_sdkAreaResizeRect.normalized();
		pixImagePoint startPoint;
		pixImagePoint endPoint;
		const bool valid = rect.width() > 2.0 && rect.height() > 2.0
			&& sdkScenePointToDiseasePoint(rect.topLeft(), startPoint)
			&& sdkScenePointToDiseasePoint(rect.bottomRight(), endPoint);
		bool updated = false;
		if (valid)
		{
			m_diseaseStartPoint = startPoint;
			m_diseaseEndPoint = endPoint;
			updated = updateSdkAreaDiseaseGeometry(m_sdkAreaResizeDisease);
		}
		m_sdkAreaResizeDragging = false;
		m_sdkAreaResizeCorner = -1;
		m_sdkImageView->unsetCursor();
		if (updated)
		{
			setSelectedDisease(m_sdkAreaResizeDisease);
			emit signal_selectDisease(m_sdkAreaResizeDisease);
			refreshSdkDiseaseLayer();
		}
		refreshSdkAreaResizeHandles();
		return true;
	}

	if (!m_sdkEditPressPending)
	{
		return false;
	}

	const bool wasMoving = m_sdkAreaMoveDragging;
	hnRoadDiseaseInfo editedDisease = m_sdkEditDisease;
	const QPoint legacyPoint = m_sdkEditLegacyPoint;
	bool updated = false;
	if (wasMoving)
	{
		const QPointF scenePoint = m_sdkImageView->mapToScene(mouseEvent->pos());
		if (editedDisease.nDrawType == 1)
		{
			pixImagePoint releasePoint;
			if (m_sdkEditLegacyPoint.x() >= 0 && m_sdkEditLegacyPoint.y() >= 0
				&& sdkScenePointToDiseasePoint(scenePoint, releasePoint))
			{
				const QPoint offset = sdkPixPointToBigImagePoint(releasePoint) - m_sdkEditLegacyPoint;
				updated = moveSdkLittleFrameDisease(editedDisease, offset);
			}
		}
		else
		{
			const QRectF candidate =
				m_sdkAreaMoveOriginalRect.translated(scenePoint - m_sdkAreaMovePressScene).normalized();
			pixImagePoint startPoint;
			pixImagePoint endPoint;
			if (sdkScenePointToDiseasePoint(candidate.topLeft(), startPoint)
				&& sdkScenePointToDiseasePoint(candidate.bottomRight(), endPoint))
			{
				m_diseaseStartPoint = startPoint;
				m_diseaseEndPoint = endPoint;
				updated = updateSdkAreaDiseaseGeometry(editedDisease);
			}
		}
	}
	resetSdkEditGesture();

	if (wasMoving)
	{
		if (updated)
		{
			setSelectedDisease(editedDisease);
			emit signal_selectDisease(editedDisease);
			refreshSdkDiseaseLayer();
		}
		else
		{
			QMessageBox::warning(this, QString::fromUtf8("\xE6\x8F\x90\xE7\xA4\xBA"),
				QString::fromUtf8("\xE7\x97\x85\xE5\xAE\xB3\xE7\xA7\xBB\xE5\x8A\xA8\xE5\xA4\xB1\xE8\xB4\xA5\xEF\xBC\x8C\xE5\x8E\x9F\xE5\xA7\x8B\xE6\x95\xB0\xE6\x8D\xAE\xE6\x9C\xAA\xE6\x94\xB9\xE5\x8F\x98\xE3\x80\x82"),
				QString::fromUtf8("\xE7\xA1\xAE\xE5\xAE\x9A"));
		}
		refreshSdkAreaResizeHandles();
		return true;
	}

	if (legacyPoint.x() >= 0 && legacyPoint.y() >= 0)
	{
		editDisease(editedDisease, legacyPoint);
	}
	m_selectedSdkDiseaseKey.clear();
	m_seclectedDiseases.clear();
	m_sdkDiseaseGraphicsLayer.setSelectedDiseaseKey(QString());
	clearSdkLittleFrameRenderCache();
	refreshSdkDiseaseLayer();
	return true;
}

bool hn2d3dPixBaseWidget::eventFilter(QObject* watched, QEvent* event)
{
	if (!m_sdkImageView)
	{
		return hnBrowsePixWidget::eventFilter(watched, event);
	}

	if ((watched == m_sdkImageView || watched == m_sdkImageView->viewport()) && event->type() == QEvent::KeyPress)
	{
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

		if (isSdkNavigationKey(keyEvent->key()))
		{
			// 浏览快捷键必须先交给 SDK，否则 ADD_MODE/GET_MILE 会把 W/A/S/D/方向键吃掉。
			if (m_sdkImageView && m_sdkImageView->viewport())
			{
				const QPoint keyViewportPoint = m_sdkImageView->viewport()->mapFromGlobal(QCursor::pos());
				rememberLittleFrameWheelAnchor(keyViewportPoint);
			}
			if (watched == m_sdkImageView->viewport())
			{
				QKeyEvent translatedEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers(), keyEvent->text(),
					keyEvent->isAutoRepeat(), keyEvent->count());
				QCoreApplication::sendEvent(m_sdkImageView, &translatedEvent);
				keyEvent->accept();
				return true;
			}
			return hnBrowsePixWidget::eventFilter(watched, event);
		}

		if (shouldForwardSdkMouseToLegacy())
		{
			QKeyEvent translatedEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers(), keyEvent->text(),
				keyEvent->isAutoRepeat(), keyEvent->count());
			QCoreApplication::sendEvent(this, &translatedEvent);
			updateSdkDiseaseOverlay();
			keyEvent->accept();
			return true;
		}

		if (watched == m_sdkImageView->viewport())
		{
			QKeyEvent translatedEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers(), keyEvent->text(),
				keyEvent->isAutoRepeat(), keyEvent->count());
			QCoreApplication::sendEvent(m_sdkImageView, &translatedEvent);
			keyEvent->accept();
			return true;
		}
	}
	if (watched != m_sdkImageView->viewport())
	{
		return hnBrowsePixWidget::eventFilter(watched, event);
	}

	if (event->type() == QEvent::MouseButtonPress)
	{
		setFocus();
		m_sdkImageView->setFocus();
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
		if (handleSdkMergeMousePress(mouseEvent))
		{
			mouseEvent->accept();
			return true;
		}
		if (handleSdkAreaResizeMousePress(mouseEvent))
		{
			mouseEvent->accept();
			return true;
		}
		// In ADD_MODE an idle right click toggles an existing disease. Once drawing has
		// started, handleSdkDiseaseMousePress keeps the original finish/cancel behavior.
		if (mouseEvent->button() == Qt::RightButton &&
			m_workMode == WorkMode::ADD_MODE && !m_isDrawingDisease &&
			selectSdkDiseaseAtViewportPoint(mouseEvent->pos()))
		{
			mouseEvent->accept();
			return true;
		}
		if (handleSdkDiseaseMousePress(mouseEvent))
		{
			mouseEvent->accept();
			return true;
		}

		if (mouseEvent->button() == Qt::RightButton && selectSdkDiseaseAtViewportPoint(mouseEvent->pos()))
		{
			mouseEvent->accept();
			return true;
		}

		if (mouseEvent->button() == Qt::LeftButton
			&& (m_workMode == WorkMode::NO_MODE || m_workMode == WorkMode::DELETE_MODE
				|| m_workMode == WorkMode::MOVE)
			&& m_sdkDiseaseGraphicsLayer.diseaseKeyAt(
				m_sdkImageView->mapToScene(mouseEvent->pos())).isEmpty())
		{
			showSdkNoDiseaseModeHint(mouseEvent->pos());
			mouseEvent->accept();
			return true;
		}

		if (mouseEvent->button() == Qt::LeftButton
			&& m_workMode == WorkMode::ADD_CTRL_POINT
			&& m_sdkDiseaseGraphicsLayer.diseaseKeyAt(
				m_sdkImageView->mapToScene(mouseEvent->pos())).isEmpty())
		{
			showSdkNoDiseaseModeHint(mouseEvent->pos());
		}
		if (m_workMode != WorkMode::ADD_MODE && shouldForwardSdkMouseToLegacy())
		{
			forwardSdkMouseEventToLegacy(mouseEvent);
			mouseEvent->accept();
			return true;
		}
	}

	if (event->type() == QEvent::MouseButtonRelease ||
		event->type() == QEvent::MouseButtonDblClick)
	{
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
		if (handleSdkAreaResizeMouseRelease(mouseEvent))
		{
			mouseEvent->accept();
			return true;
		}
		if (handleSdkDiseaseMouseRelease(mouseEvent))
		{
			mouseEvent->accept();
			return true;
		}
		if (m_workMode != WorkMode::ADD_MODE && shouldForwardSdkMouseToLegacy())
		{
			forwardSdkMouseEventToLegacy(mouseEvent);
			mouseEvent->accept();
			return true;
		}
	}

	if (event->type() == QEvent::MouseMove)
	{
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
		updateSdkInspectionViews(mouseEvent->pos());
		refreshStatusInfoFromSdkViewportPoint(mouseEvent->pos());
		updateSdkLittleFrameDeleteHover(mouseEvent->pos());
		if (handleSdkAreaResizeMouseMove(mouseEvent))
		{
			mouseEvent->accept();
			return true;
		}
		if (handleSdkDiseaseMouseMove(mouseEvent))
		{
			mouseEvent->accept();
			return true;
		}
		if (m_workMode != WorkMode::ADD_MODE && shouldForwardSdkMouseToLegacy())
		{
			forwardSdkMouseEventToLegacy(mouseEvent);
			return true;
		}
	}

	if (event->type() == QEvent::Wheel)
	{
		QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
		if (m_sdkImageView && m_sdkImageView->viewport() &&
			m_sdkImageView->viewport()->rect().contains(wheelEvent->pos()))
		{
			m_lastSdkStatusViewportPoint = wheelEvent->pos();
			m_hasLastSdkStatusViewportPoint = true;
		}
		const QPoint wheelViewportPoint = wheelEvent->pos();
		rememberLittleFrameWheelAnchor(wheelViewportPoint);
		QTimer::singleShot(0, this, [this, wheelViewportPoint]()
		{
			if (!m_sdkImageView)
			{
				return;
			}
			refreshSdkDrawingAfterBrowse(wheelViewportPoint);
			refreshSdkDiseaseItems();
			refreshStatusInfoFromSdkCurrentMouseOrAnchor(m_sdkImageView->currentBottomAnchor());
			restoreLittleFrameCursorAfterWheelBrowse();
		});
		return false;
	}

	return false;
}


void hn2d3dPixBaseWidget::setSelectedDiseaseId(int diseaseId)
{
	hnBrowsePixWidget::setSelectedDiseaseId(diseaseId);
	m_selectedSdkDiseaseKey.clear();
	m_sdkDiseaseGraphicsLayer.setSelectedDiseaseKey(QString());
	if (m_sdkImageView && m_sdkImageView->viewport())
	{
		m_sdkImageView->viewport()->update();
	}
	update();
}

void hn2d3dPixBaseWidget::setSelectedDisease(const hnRoadDiseaseInfo& disease)
{
	// The input may alias m_currentWidgetDiseases, which is replaced below.
	const hnRoadDiseaseInfo selectedDisease = disease;
	const QString diseaseKey = sdkDiseaseKey(selectedDisease);
	if (selectedDisease.nID < 0 || diseaseKey.isEmpty())
	{
		hnBrowsePixWidget::setSelectedDiseaseId(-1);
		m_seclectedDiseases.clear();
		m_selectedSdkDiseaseKey.clear();
		m_sdkDiseaseGraphicsLayer.setSelectedDiseaseKey(QString());
		refreshSdkDiseaseItems();
		if (m_sdkImageView && m_sdkImageView->viewport())
		{
			m_sdkImageView->viewport()->update();
		}
		update();
		return;
	}

	hnBrowsePixWidget::setSelectedDiseaseId(selectedDisease.nID);
	m_seclectedDiseases.clear();
	m_seclectedDiseases.append(selectedDisease);
	m_selectedSdkDiseaseKey = diseaseKey;
	m_sdkDiseaseGraphicsLayer.setSelectedDiseaseKey(m_selectedSdkDiseaseKey);
	refreshSdkDiseaseItems();
	refreshSdkAreaResizeHandles();
	// A synchronized disease may be outside this view's current mileage query or
	// may not have been rebuilt yet. Keep the clicked canonical disease hittable.
	ensureSdkDiseaseItemVisible(selectedDisease);
	if (m_sdkImageView && m_sdkImageView->viewport())
	{
		m_sdkImageView->viewport()->update();
	}
	update();
}

bool hn2d3dPixBaseWidget::centerSdkDiseaseInView(int diseaseId)
{
	if (!m_sdkImageView || diseaseId < 0)
	{
		return false;
	}

	refreshSdkDiseaseItems();
	for (const hnRoadDiseaseInfo& disease : m_currentWidgetDiseases)
	{
		if (disease.nID == diseaseId)
		{
			return centerSdkDiseaseInView(disease);
		}
	}
	return false;
}

bool hn2d3dPixBaseWidget::centerSdkDiseaseInView(const hnRoadDiseaseInfo& disease)
{
	if (!m_sdkImageView)
	{
		return false;
	}

	// The refresh replaces m_currentWidgetDiseases; retain a value, not its element reference.
	const hnRoadDiseaseInfo selectedDisease = disease;
	hnBrowsePixWidget::setSelectedDiseaseId(selectedDisease.nID);
	m_selectedSdkDiseaseKey = sdkDiseaseKey(selectedDisease);
	refreshSdkDiseaseItems();
	m_sdkDiseaseGraphicsLayer.setSelectedDiseaseKey(m_selectedSdkDiseaseKey);

	QRectF rect;
	if (!m_sdkDiseaseGraphicsLayer.diseaseSceneRect(m_selectedSdkDiseaseKey, rect))
	{
		// 列表选中的病害可能不在当前视口范围内，直接按病害几何生成 scene item，再按 scene rect 居中。
		ensureSdkDiseaseItemVisible(selectedDisease);
		m_sdkDiseaseGraphicsLayer.setSelectedDiseaseKey(m_selectedSdkDiseaseKey);
		if (!m_sdkDiseaseGraphicsLayer.diseaseSceneRect(m_selectedSdkDiseaseKey, rect))
		{
			return false;
		}
	}

	m_sdkImageView->centerOn(rect.center());
	refreshStatusInfoFromSdkCurrentMouseOrAnchor(m_sdkImageView->currentBottomAnchor());
	if (m_sdkImageView->viewport())
	{
		m_sdkImageView->viewport()->update();
	}
	QTimer::singleShot(0, this, [this]()
	{
		refreshSdkDiseaseItems();
		refreshSdkViewState();
	});
	return true;
}

void hn2d3dPixBaseWidget::clearSdkView()
{
	clearContinuousDiseaseDrawing();
	m_selectedSdkDiseaseKey.clear();
	hnBrowsePixWidget::setSelectedDiseaseId(-1);
	m_currentWidgetDiseases.clear();
	clearSdkLittleFrameRenderCache();
	m_sdkDiseaseGraphicsLayer.clearAll();
	if (m_sdkImageController)
	{
		m_sdkImageController->clear();
	}
	else if (m_sdkImageView)
	{
		m_sdkImageView->clear();
	}
	if (m_sdkImageView)
	{
		m_sdkDiseaseGraphicsLayer.setScene(m_sdkImageView->scene());
		m_sdkImageView->viewport()->update();
	}
	updateSdkDiseaseOverlay();
	update();
}

QPointF hn2d3dPixBaseWidget::bigImagePointToSdkScenePoint(const QPoint& point) const
{
	QString pixName;
	const QPoint singlePoint = const_cast<hn2d3dPixBaseWidget*>(this)->bigImagePointToSingleImagePoint(point, &pixName);
	if (pixName.isEmpty())
	{
		return QPointF();
	}

	return sdkPixPointToScenePoint(pixImagePoint{ pixName, singlePoint });
}

QPainterPath hn2d3dPixBaseWidget::sdkPathFromBigImageRect(const QRect& rect) const
{
	QPainterPath path;
	if (!m_sdkImageView || !rect.isValid() || rect.isNull())
	{
		return path;
	}

	QString pixName;
	const QRect singleRect = const_cast<hn2d3dPixBaseWidget*>(this)->bigImageRectToSingleImageRect(rect.normalized(), &pixName).normalized();
	if (pixName.isEmpty() || !singleRect.isValid() || singleRect.isNull())
	{
		return path;
	}

	const QPointF p0 = sdkPixPointToScenePoint(pixImagePoint{ pixName, singleRect.topLeft() });
	const QPointF p1 = sdkPixPointToScenePoint(pixImagePoint{ pixName, singleRect.topRight() });
	const QPointF p2 = sdkPixPointToScenePoint(pixImagePoint{ pixName, singleRect.bottomRight() });
	const QPointF p3 = sdkPixPointToScenePoint(pixImagePoint{ pixName, singleRect.bottomLeft() });
	if (p0.isNull() && p1.isNull() && p2.isNull() && p3.isNull())
	{
		return path;
	}

	path.moveTo(p0);
	path.lineTo(p1);
	path.lineTo(p2);
	path.lineTo(p3);
	path.closeSubpath();
	return path;
}
QPainterPath hn2d3dPixBaseWidget::sdkPathFromBigImagePoints(const QVector<QPoint>& points) const
{
	QPainterPath path;
	bool hasStart = false;
	for (const QPoint& point : points)
	{
		const QPointF scenePoint = bigImagePointToSdkScenePoint(point);
		if (!hasStart)
		{
			path.moveTo(scenePoint);
			hasStart = true;
		}
		else
		{
			path.lineTo(scenePoint);
		}
	}
	return path;
}

QPainterPath hn2d3dPixBaseWidget::sdkDiseaseScenePath(const hnRoadDiseaseInfo& disease)
{
	QPainterPath diseasePath;
	const QVector<QRect> rects = sdkDiseaseBigImageRects(disease);
	for (const QRect& rect : rects)
	{
		const QPainterPath rectPath = sdkPathFromBigImageRect(rect);
		if (!rectPath.isEmpty())
		{
			// 跨图片的大框会被数据库拆成多段；几何并集只保留病害外轮廓。
			diseasePath = diseasePath.isEmpty() ? rectPath : diseasePath.united(rectPath);
		}
	}
	return diseasePath;
}

bool hn2d3dPixBaseWidget::shouldRenderSdkDisease(const hnRoadDiseaseInfo& disease) const
{
	if (m_frameMode == FrameMode::BIG_FRAME)
	{
		return disease.nDrawType == 0;
	}
	if (m_frameMode == FrameMode::LITTLE_FRAME)
	{
		return disease.nDrawType != 0 && disease.nDrawType != 3;
	}
	if (m_frameMode == FrameMode::DESIGN_FACETS || m_frameMode == FrameMode::DESIGN_LINE)
	{
		return disease.nDrawType == 2 || disease.nDrawType == 3;
	}
	return true;
}

QString hn2d3dPixBaseWidget::sdkDiseaseKey(const hnRoadDiseaseInfo& disease) const
{
	return QString::fromLocal8Bit(disease.strDiseaseTableName) + QStringLiteral("#") + QString::number(disease.nID);
}

bool hn2d3dPixBaseWidget::isSameSdkDisease(const hnRoadDiseaseInfo& disease, const QString& diseaseKey) const
{
	return !diseaseKey.isEmpty() && sdkDiseaseKey(disease) == diseaseKey;
}

QString hn2d3dPixBaseWidget::sdkDiseaseLabel(const hnRoadDiseaseInfo& disease, bool selected) const
{
	if (disease.nDrawType == 3)
	{
		return selected ? buildLittleFrameDiseaseDetailLabel(disease, false) : buildLineDiseaseLabel(disease);
	}
	return buildFrameDiseaseLabel(disease, selected, disease.nDrawType == 0 || disease.nDrawType == 2, false);
}

QString hn2d3dPixBaseWidget::sdkRoadMarkTypeName(int markType) const
{
	switch (markType)
	{
	case 0:
		return QString::fromLocal8Bit("路面材质");
	case 1:
		return QString::fromLocal8Bit("路面单元");
	case 2:
		return QString::fromLocal8Bit("路面等级");
	case 3:
		return QString::fromLocal8Bit("路面标准");
	case 4:
		return QString::fromLocal8Bit("路面情况");
	default:
		return QString::fromLocal8Bit("道路打标");
	}
}

QColor hn2d3dPixBaseWidget::sdkMaterialMarkColor(const QString& materialName) const
{
	const uint hue = qHash(materialName) % 360;
	return QColor::fromHsv(hue, 160, 230);
}

QColor hn2d3dPixBaseWidget::sdkRoadMarkColor(const hnCommon::hnMarkInfo& mark) const
{
	const QString markValue = QString::fromLocal8Bit(mark.strMark);
	switch (mark.nType)
	{
	case 0:
		return sdkMaterialMarkColor(markValue);
	case 2:
		return QColor(38, 166, 91);
	case 3:
		return QColor(245, 140, 28);
	case 1:
		return QColor(0, 220, 255);
	case 4:
		return QColor(96, 125, 139);
	default:
		return QColor(90, 90, 90);
	}
}

bool hn2d3dPixBaseWidget::isRoadAttributeMarkType(int markType) const
{
	return markType == 0 || markType == 2 || markType == 3;
}

void hn2d3dPixBaseWidget::refreshSdkMaterialMarks()
{
	m_sdkDiseaseGraphicsLayer.clearMaterialMarks();
	if (!m_sdkImageView || !hnDataManager::getDataManager()->isOpenProject() || !hnDataManager::getDataManager()->getCurrentProject())
	{
		return;
	}

	if (m_sdkImageView->viewport())
	{
		const QRectF visibleSceneRect = m_sdkImageView->mapToScene(m_sdkImageView->viewport()->rect()).boundingRect();
		const QTransform viewTransform = m_sdkImageView->transform();
		m_sdkDiseaseGraphicsLayer.setLabelPlacementContext(visibleSceneRect,
			qAbs(viewTransform.m11()), qAbs(viewTransform.m22()), m_diseaseDrawStyle.calloutGap);
	}

	double visibleBeginMile = 0.0;
	double visibleEndMile = 0.0;
	currentSdkVisibleEncoderMileRange(visibleBeginMile, visibleEndMile);

	const QVector<hnCommon::hnMarkInfo> marks = hnDataManager::getDataManager()->getCurrentProject()->getCurrentMarkVector();
	for (const hnCommon::hnMarkInfo& mark : marks)
	{
		const double viewEncoderMile = projectEncoderMileToSdkViewEncoderMile(mark.dEnclMile);
		if (viewEncoderMile < visibleBeginMile || viewEncoderMile > visibleEndMile)
		{
			continue;
		}

		const bool roadAttributeMark = isRoadAttributeMarkType(mark.nType);
		QColor lineColor = sdkRoadMarkColor(mark);
		QColor fillColor = lineColor;
		fillColor.setAlpha(roadAttributeMark ? 48 : 24);
		lineColor.setAlpha(roadAttributeMark ? 235 : 190);

		const QString markTypeName = sdkRoadMarkTypeName(mark.nType);
		const QString markValue = QString::fromLocal8Bit(mark.strMark);
		const QString label = QString::fromLocal8Bit("%1:%2  桩号:%3")
			.arg(markTypeName)
			.arg(markValue)
			.arg(mark.dTrueMile, 0, 'f', 3);

		m_sdkDiseaseGraphicsLayer.addMaterialBoundaryMark(
			encoderMileToSdkSceneY(viewEncoderMile),
			m_pixWidth,
			label,
			lineColor,
			fillColor,
			roadAttributeMark ? 3 : 2,
			roadAttributeMark ? Qt::SolidLine : Qt::DashLine,
			roadAttributeMark ? 42.0 : 24.0,
			qBound(0, mark.nType, 4));
	}
}

void hn2d3dPixBaseWidget::clearLastSdkAddedDisease()
{
	m_lastSdkAddedDisease = hnRoadDiseaseInfo();
	m_hasLastSdkAddedDisease = false;
}

void hn2d3dPixBaseWidget::rememberLastSdkAddedDisease(const hnRoadDiseaseInfo& disease)
{
	m_lastSdkAddedDisease = disease;
	m_hasLastSdkAddedDisease = true;
}

bool hn2d3dPixBaseWidget::takeLastSdkAddedDisease(hnRoadDiseaseInfo& disease)
{
	if (!m_hasLastSdkAddedDisease)
	{
		return false;
	}

	disease = m_lastSdkAddedDisease;
	clearLastSdkAddedDisease();
	return true;
}


QString hn2d3dPixBaseWidget::sdkLittleFrameRenderCacheKey(const hnRoadDiseaseInfo& disease) const
{
	// Cache is only for SDK display geometry. Include fields that affect path reuse.
	return QString::fromLatin1("%1|type=%2|r2=%3|r3=%4|area=%5|len=%6|wid=%7|real=%8,%9|pix=%10x%11|mirror=%12%13|widget=%14")
		.arg(sdkDiseaseKey(disease))
		.arg(disease.nDrawType)
		.arg(disease.vec2dRect.size())
		.arg(disease.vec3dRect.size())
		.arg(disease.dArea, 0, 'f', 4)
		.arg(disease.dLength, 0, 'f', 4)
		.arg(disease.dWidth, 0, 'f', 4)
		.arg(disease.dRealLen, 0, 'f', 4)
		.arg(disease.dReaWidth, 0, 'f', 4)
		.arg(m_pixWidth)
		.arg(m_pixHeight)
		.arg(m_isHMirrored ? 1 : 0)
		.arg(m_isVMirrored ? 1 : 0)
		.arg(static_cast<int>(m_widgetType));
}

bool hn2d3dPixBaseWidget::cachedSdkLittleFrameScenePath(const hnRoadDiseaseInfo& disease, QPainterPath& path) const
{
	if (disease.nDrawType != 1)
	{
		return false;
	}

	const QString cacheKey = sdkLittleFrameRenderCacheKey(disease);
	const auto iter = m_sdkLittleFrameRenderCache.constFind(cacheKey);
	if (iter == m_sdkLittleFrameRenderCache.constEnd())
	{
		return false;
	}

	path = iter.value().path;
	return !path.isEmpty();
}

void hn2d3dPixBaseWidget::cacheSdkLittleFrameScenePath(const hnRoadDiseaseInfo& disease, const QPainterPath& path, SdkLittleFrameRenderMode mode)
{
	if (disease.nDrawType != 1 || path.isEmpty())
	{
		return;
	}

	const QString cacheKey = sdkLittleFrameRenderCacheKey(disease);
	SdkLittleFrameRenderCacheEntry entry;
	entry.path = path;
	entry.sceneBounds = path.boundingRect();
	entry.mode = mode;
	entry.rectCount = qMax(disease.vec2dRect.size(), disease.vec3dRect.size());
	m_sdkLittleFrameRenderCache.insert(cacheKey, entry);
	m_sdkLittleFrameRenderCacheOrder.removeAll(cacheKey);
	m_sdkLittleFrameRenderCacheOrder.append(cacheKey);

	const int maxCacheCount = 256;
	while (m_sdkLittleFrameRenderCacheOrder.size() > maxCacheCount)
	{
		const QString oldKey = m_sdkLittleFrameRenderCacheOrder.takeFirst();
		m_sdkLittleFrameRenderCache.remove(oldKey);
	}
}

void hn2d3dPixBaseWidget::clearSdkLittleFrameRenderCache()
{
	m_sdkLittleFrameRenderCache.clear();
	m_sdkLittleFrameRenderCacheOrder.clear();
	m_currentSdkLittleFrameHoverKey.clear();
	m_currentSdkLittleFrameHoverIndex = -1;
}

void hn2d3dPixBaseWidget::updateSdkLittleFrameDeleteHover(const QPoint& viewportPoint)
{
	if (!m_sdkImageView || m_workMode != WorkMode::DELETE_MODE || m_frameMode != FrameMode::LITTLE_FRAME)
	{
		return;
	}

	pixImagePoint point;
	if (!sdkScenePointToDiseasePoint(m_sdkImageView->mapToScene(viewportPoint), point))
	{
		m_sdkDiseaseGraphicsLayer.clearTemporary();
		m_currentSdkLittleFrameHoverKey.clear();
		m_currentSdkLittleFrameHoverIndex = -1;
		return;
	}

	if (m_widgetType == WIDGET_3D && hnDataManager::getDataManager()->getCurrentProject() &&
		PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
	{
		int x = point.pixPoint.x();
		autoCorrectXIn3dView(x);
		point.pixPoint.setX(x);
	}

	hnRoadDiseaseInfo disease;
	int hitIndex = -1;
	if (!findSdkLittleFrameDiseaseAtPoint(point, disease, hitIndex))
	{
		m_sdkDiseaseGraphicsLayer.clearTemporary();
		m_currentSdkLittleFrameHoverKey.clear();
		m_currentSdkLittleFrameHoverIndex = -1;
		return;
	}

	const QString diseaseKey = sdkDiseaseKey(disease);
	if (diseaseKey == m_currentSdkLittleFrameHoverKey && hitIndex == m_currentSdkLittleFrameHoverIndex)
	{
		return;
	}

	m_currentSdkLittleFrameHoverKey = diseaseKey;
	m_currentSdkLittleFrameHoverIndex = hitIndex;
	m_sdkDiseaseGraphicsLayer.clearTemporary();

	QRectF sceneRect;
	if (sdkLittleFrameCellSceneRect(disease, hitIndex, sceneRect) && sceneRect.isValid() && !sceneRect.isNull())
	{
		m_sdkDiseaseGraphicsLayer.addTemporaryRect(sceneRect, QColor(255, 230, 0), 3, Qt::DashLine);
	}
}

void hn2d3dPixBaseWidget::addSdkDiseaseItem(const hnRoadDiseaseInfo& disease)
{
	if (!m_sdkImageView || !shouldRenderSdkDisease(disease))
	{
		return;
	}

	const QString diseaseKey = sdkDiseaseKey(disease);
	const bool selected = diseaseKey == m_selectedSdkDiseaseKey;
	QColor color = m_diseaseDrawStyle.littleFrameRectColor;
	int width = m_diseaseDrawStyle.littleFrameRectWidth;
	Qt::PenStyle style = Qt::SolidLine;
	if (disease.nDrawType == 0 || disease.nDrawType == 2)
	{
		color = m_diseaseDrawStyle.bigFrameRectColor;
		width = m_diseaseDrawStyle.bigFrameRectWidth;
	}
	else if (disease.nDrawType == 3)
	{
		color = m_diseaseDrawStyle.lineDiseaseColor;
		width = m_diseaseDrawStyle.lineDiseaseWidth;
	}

	if (selected)
	{
		// SDK selected state changes color only; keep original width and pen style.
		color = m_diseaseDrawStyle.selectedRectColor;
	}
	else if (isSeclectedMergeDisease(disease))
	{
		color = m_diseaseDrawStyle.mergedRectColor;
	}

	const QPainterPath diseasePath = sdkDiseaseScenePath(disease);
	if (disease.nDrawType == 3)
	{
		if (diseasePath.elementCount() >= 2)
		{
			m_sdkDiseaseGraphicsLayer.addDiseasePath(diseaseKey, diseasePath, sdkDiseaseLabel(disease, selected), color, width, style);
		}
		return;
	}
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_SDK_DISEASE_RENDER_ATTEMPT]"
		<< "key=" << diseaseKey
		<< "selected=" << selected
		<< "drawType=" << disease.nDrawType
		<< "pathEmpty=" << diseasePath.isEmpty()
		<< "bounds=" << diseasePath.boundingRect()
		<< "vec2d=" << disease.vec2dRect.size()
		<< "vec3d=" << disease.vec3dRect.size()
		<< "dDmi=" << disease.dDmi
		<< "dMileage=" << disease.dMileage
		<< "range=" << m_beginEncoderMile << m_endEncoderMile;
	#endif
	if (diseasePath.isEmpty())
	{
		if (selected)
		{
			#ifdef _DEBUG
			qWarning().noquote() << "[HN_SDK_DISEASE_RENDER_SKIP]"
				<< "reason=emptyScenePath"
				<< "key=" << diseaseKey
				<< "drawType=" << disease.nDrawType
				<< "rects=" << sdkDiseaseBigImageRects(disease).size()
				<< "mile=" << disease.dMileage
				<< "range=" << m_beginEncoderMile << m_endEncoderMile;
			#endif
		}
		return;
	}

	m_sdkDiseaseGraphicsLayer.addDiseasePath(diseaseKey, diseasePath,
		sdkDiseaseLabel(disease, selected), color, width, style);
	if (selected)
	{
		#ifdef _DEBUG
		qDebug().noquote() << "[HN_SDK_DISEASE_RENDER]"
			<< "key=" << diseaseKey
			<< "drawType=" << disease.nDrawType
			<< "sceneBounds=" << diseasePath.boundingRect();
		#endif
	}
}

void hn2d3dPixBaseWidget::ensureSdkDiseaseItemVisible(const hnRoadDiseaseInfo& disease)
{
	if (!m_sdkImageView)
	{
		return;
	}

	const QString diseaseKey = sdkDiseaseKey(disease);
	QRectF rect;
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_SDK_DISEASE_ENSURE]"
		<< "key=" << diseaseKey
		<< "drawType=" << disease.nDrawType
		<< "vec2d=" << disease.vec2dRect.size()
		<< "vec3d=" << disease.vec3dRect.size();
	#endif
	if (!m_sdkDiseaseGraphicsLayer.diseaseSceneRect(diseaseKey, rect))
	{
		addSdkDiseaseItem(disease);
		m_sdkDiseaseGraphicsLayer.setSelectedDiseaseKey(m_selectedSdkDiseaseKey);
	}

	if (m_sdkImageView->scene())
	{
		m_sdkImageView->scene()->update();
	}
	if (m_sdkImageView->viewport())
	{
		m_sdkImageView->viewport()->update();
	}
	update();
}
void hn2d3dPixBaseWidget::scheduleSdkDiseaseRefresh()
{
	if (!m_sdkImageView || m_sdkImageView->contentMode() != ContentMode::VirtualSequence || !m_sdkDiseaseRefreshTimer)
	{
		refreshSdkDiseaseItems();
		return;
	}
	m_sdkDiseaseRefreshTimer->start(60);
}

QByteArray hn2d3dPixBaseWidget::sdkDiseaseRenderFingerprint(const hnRoadDiseaseInfo& disease)
{
	QPainterPath path;
	path = sdkDiseaseScenePath(disease);
	QByteArray bytes;
	QDataStream stream(&bytes, QIODevice::WriteOnly);
	stream.setVersion(QDataStream::Qt_5_8);
	stream << sdkDiseaseKey(disease) << disease.nDrawType << disease.dDmi << disease.dMileage;
	stream << (sdkDiseaseKey(disease) == m_selectedSdkDiseaseKey) << isSeclectedMergeDisease(disease);
	stream << path.elementCount();
	for (int i = 0; i < path.elementCount(); ++i)
	{
		const QPainterPath::Element element = path.elementAt(i);
		stream << element.x << element.y << static_cast<int>(element.type);
	}
	if (m_sdkImageView && m_sdkImageView->viewport())
	{
		const QRectF visible = m_sdkImageView->mapToScene(m_sdkImageView->viewport()->rect()).boundingRect();
		const bool visibleNow = visible.intersects(path.boundingRect());
		const qreal bucketSize = qMax<qreal>(1.0, visible.height() / 2.0);
		stream << visibleNow << (visibleNow ? qFloor(visible.top() / bucketSize) : 0);
	}
	return QCryptographicHash::hash(bytes, QCryptographicHash::Sha1);
}

void hn2d3dPixBaseWidget::refreshSdkDiseaseItems()
{
	if (!m_sdkImageView || !hnDataManager::getDataManager()->isOpenProject())
	{
		m_currentWidgetDiseases.clear();
		m_sdkDiseaseGraphicsLayer.clearDiseases();
		m_sdkDiseaseRenderFingerprints.clear();
		m_sdkDiseaseGraphicsLayer.clearLabelPlacementContext();
		m_sdkDiseaseGraphicsLayer.clearMaterialMarks();
		clearSdkLittleFrameRenderCache();
		return;
	}

	QRectF visibleSceneRect;
	if (m_sdkImageView->viewport())
	{
		visibleSceneRect = m_sdkImageView->mapToScene(m_sdkImageView->viewport()->rect()).boundingRect();
		const QTransform viewTransform = m_sdkImageView->transform();
		m_sdkDiseaseGraphicsLayer.setLabelPlacementContext(visibleSceneRect,
			qAbs(viewTransform.m11()), qAbs(viewTransform.m22()), m_diseaseDrawStyle.calloutGap);
	}
	else
	{
		m_sdkDiseaseGraphicsLayer.clearLabelPlacementContext();
	}

	refreshSdkMaterialMarks();
	QVector<hnRoadDiseaseInfo> diseases;
	if (m_sdkImageView->contentMode() != ContentMode::VirtualSequence)
	{
		m_sdkDiseaseGraphicsLayer.clearDiseases();
		m_sdkDiseaseRenderFingerprints.clear();
		hnDataManager::getDataManager()->getDiseaseService()->getRoadDiseasesInRange(
			m_beginEncoderMile, m_endEncoderMile, diseases);
		m_currentWidgetDiseases = diseases.toStdVector();
		for (const hnRoadDiseaseInfo& disease : qAsConst(diseases)) addSdkDiseaseItem(disease);
		m_sdkDiseaseGraphicsLayer.setSelectedDiseaseKey(m_selectedSdkDiseaseKey);
		return;
	}

	double visibleBegin = m_beginEncoderMile;
	double visibleEnd = m_endEncoderMile;
	currentSdkVisibleEncoderMileRange(visibleBegin, visibleEnd);
	const double visibleSpan = qMax(m_imageDistanceMeters, visibleEnd - visibleBegin);
	const double queryBegin = qMax(0.0, visibleBegin - visibleSpan);
	const double queryEnd = visibleEnd + visibleSpan;
	hnDataManager::getDataManager()->getDiseaseService()->getRoadDiseasesInRange(queryBegin, queryEnd, diseases);
	m_currentWidgetDiseases = diseases.toStdVector();

	QSet<QString> incomingKeys;
	for (const hnRoadDiseaseInfo& disease : qAsConst(diseases))
	{
		const QString key = sdkDiseaseKey(disease);
		if (key.isEmpty() || !shouldRenderSdkDisease(disease)) continue;
		incomingKeys.insert(key);
		const QByteArray fingerprint = sdkDiseaseRenderFingerprint(disease);
		if (!m_sdkDiseaseGraphicsLayer.containsDisease(key)
			|| m_sdkDiseaseRenderFingerprints.value(key) != fingerprint)
		{
			m_sdkDiseaseGraphicsLayer.removeDisease(key);
			addSdkDiseaseItem(disease);
			m_sdkDiseaseRenderFingerprints.insert(key, fingerprint);
		}
	}

	const QStringList renderedKeys = m_sdkDiseaseGraphicsLayer.diseaseKeys();
	for (const QString& key : renderedKeys)
	{
		if (!incomingKeys.contains(key))
		{
			m_sdkDiseaseGraphicsLayer.removeDisease(key);
			m_sdkDiseaseRenderFingerprints.remove(key);
		}
	}
	m_sdkDiseaseGraphicsLayer.setSelectedDiseaseKey(m_selectedSdkDiseaseKey);
#ifdef _DEBUG
	qDebug().noquote() << "[HN_SDK_DISEASE_INCREMENTAL]"
		<< "visible=" << visibleBegin << visibleEnd
		<< "buffer=" << queryBegin << queryEnd
		<< "queried=" << diseases.size()
		<< "rendered=" << m_sdkDiseaseGraphicsLayer.diseaseKeys().size();
#endif
}

bool hn2d3dPixBaseWidget::selectSdkDiseaseAtViewportPoint(const QPoint& viewportPoint)
{
	if (!m_sdkImageView)
	{
		return false;
	}

	const QString diseaseKey = m_sdkDiseaseGraphicsLayer.diseaseKeyAt(m_sdkImageView->mapToScene(viewportPoint));
	if (diseaseKey.isEmpty())
	{
		return false;
	}

	if (diseaseKey == m_selectedSdkDiseaseKey)
	{
		m_seclectedDiseases.clear();
		m_selectedSdkDiseaseKey.clear();
		hnBrowsePixWidget::setSelectedDiseaseId(-1);
		m_sdkDiseaseGraphicsLayer.setSelectedDiseaseKey(QString());
		refreshSdkDiseaseItems();
		hnRoadDiseaseInfo emptyDisease;
		emptyDisease.nID = -1;
		emit signal_selectDisease(emptyDisease);
		if (m_sdkImageView && m_sdkImageView->viewport())
		{
			m_sdkImageView->viewport()->update();
		}
		update();
		return true;
	}

	m_seclectedDiseases.clear();
	for (const hnRoadDiseaseInfo& disease : m_currentWidgetDiseases)
	{
		if (isSameSdkDisease(disease, diseaseKey))
		{
			// setSelectedDisease() rebuilds the source vector and invalidates disease.
			const hnRoadDiseaseInfo selectedDisease = disease;
			m_seclectedDiseases.append(selectedDisease);
			setSelectedDisease(selectedDisease);
			emit signal_selectDisease(selectedDisease);
			return true;
		}
	}
	return false;
}

bool hn2d3dPixBaseWidget::mergeSdkAreaDiseases()
{
	if (m_seclectedDiseases.size() != 2)
	{
		return false;
	}
	const hnRoadDiseaseInfo first = m_seclectedDiseases.at(0);
	const hnRoadDiseaseInfo second = m_seclectedDiseases.at(1);
	const QRectF mergedRect =
		sdkDiseaseScenePath(first).boundingRect().united(
			sdkDiseaseScenePath(second).boundingRect()).normalized();
	pixImagePoint startPoint;
	pixImagePoint endPoint;
	if (!sdkScenePointToDiseasePoint(mergedRect.topLeft(), startPoint)
		|| !sdkScenePointToDiseasePoint(mergedRect.bottomRight(), endPoint))
	{
		QMessageBox::warning(this, QString::fromUtf8("\xE6\x8F\x90\xE7\xA4\xBA"),
			QString::fromUtf8("\xE5\x90\x88\xE5\xB9\xB6\xE8\x8C\x83\xE5\x9B\xB4\xE6\x97\xA0\xE6\xB3\x95\xE8\xBD\xAC\xE6\x8D\xA2\xE4\xB8\xBA\xE6\x9C\x89\xE6\x95\x88\xE5\x9B\xBE\xE5\x83\x8F\xE5\x9D\x90\xE6\xA0\x87\xEF\xBC\x8C\xE6\x9C\xAA\xE4\xBF\xAE\xE6\x94\xB9\xE5\x8E\x9F\xE7\x97\x85\xE5\xAE\xB3\xE3\x80\x82"), QString::fromUtf8("\xE7\xA1\xAE\xE5\xAE\x9A"));
		return false;
	}
	m_diseaseStartPoint = startPoint;
	m_diseaseEndPoint = endPoint;
	hnRoadDiseaseInfo merged = first;
	if (!updateSdkAreaDiseaseGeometry(merged, false))
	{
		QMessageBox::warning(this, QString::fromUtf8("\xE6\x8F\x90\xE7\xA4\xBA"),
			QString::fromUtf8("\xE5\x90\x88\xE5\xB9\xB6\xE5\x90\x8E\xE7\x9A\x84\xE7\x97\x85\xE5\xAE\xB3\xE5\x87\xA0\xE4\xBD\x95\xE6\x97\xA0\xE6\x95\x88\xEF\xBC\x8C\xE6\x9C\xAA\xE4\xBF\xAE\xE6\x94\xB9\xE5\x8E\x9F\xE7\x97\x85\xE5\xAE\xB3\xE3\x80\x82"), QString::fromUtf8("\xE7\xA1\xAE\xE5\xAE\x9A"));
		return false;
	}
	merged.nID = hnApp::hnDataManager::getDataManager()->getCurrentProject()
		->getDB()->getDiseaseTable()->getMaxID(std::string(merged.strDiseaseTableName));
	hnDiseaseService* service =
		hnApp::hnDataManager::getDataManager()->getDiseaseService();
	if (!service->addDisease(merged))
	{
		QMessageBox::warning(this, QString::fromUtf8("\xE6\x8F\x90\xE7\xA4\xBA"),
			QString::fromUtf8("\xE5\x90\x88\xE5\xB9\xB6\xE7\x97\x85\xE5\xAE\xB3\xE5\x86\x99\xE5\x85\xA5\xE6\x95\xB0\xE6\x8D\xAE\xE5\xBA\x93\xE5\xA4\xB1\xE8\xB4\xA5\xEF\xBC\x8C\xE5\x8E\x9F\xE7\x97\x85\xE5\xAE\xB3\xE6\x9C\xAA\xE5\x88\xA0\xE9\x99\xA4\xE3\x80\x82"), QString::fromUtf8("\xE7\xA1\xAE\xE5\xAE\x9A"));
		return false;
	}
	hnRoadDiseaseInfo firstToDelete = first;
	hnRoadDiseaseInfo secondToDelete = second;
	const bool firstDeleted = service->deleteOneDisease(firstToDelete);
	const bool secondDeleted = service->deleteOneDisease(secondToDelete);
	if (!firstDeleted || !secondDeleted)
	{
		QMessageBox::warning(this, QString::fromUtf8("\xE6\x8F\x90\xE7\xA4\xBA"),
			QString::fromUtf8("\xE6\x96\xB0\xE7\x97\x85\xE5\xAE\xB3\xE5\xB7\xB2\xE7\x94\x9F\xE6\x88\x90\xEF\xBC\x8C\xE4\xBD\x86\xE6\x97\xA7\xE7\x97\x85\xE5\xAE\xB3\xE5\x88\xA0\xE9\x99\xA4\xE4\xB8\x8D\xE5\xAE\x8C\xE6\x95\xB4\xEF\xBC\x8C\xE8\xAF\xB7\xE5\x88\xB7\xE6\x96\xB0\xE5\x88\x97\xE8\xA1\xA8\xE5\x90\x8E\xE6\xA3\x80\xE6\x9F\xA5\xE3\x80\x82"), QString::fromUtf8("\xE7\xA1\xAE\xE5\xAE\x9A"));
	}
	m_seclectedDiseases.clear();
	m_selectedSdkDiseaseKey.clear();
	clearSdkLittleFrameRenderCache();
	refreshSdkDiseaseLayer();
	return firstDeleted && secondDeleted;
}

bool hn2d3dPixBaseWidget::handleSdkMergeMousePress(QMouseEvent* mouseEvent)
{
	if (!mouseEvent || mouseEvent->button() != Qt::LeftButton ||
		m_workMode != WorkMode::MERGE || !m_sdkImageView)
	{
		return false;
	}
	const QPointF scenePoint = m_sdkImageView->mapToScene(mouseEvent->pos());
	const QString diseaseKey = m_sdkDiseaseGraphicsLayer.diseaseKeyAt(scenePoint);
	if (diseaseKey.isEmpty())
	{
		showSdkNoDiseaseModeHint(mouseEvent->pos());
		return true;
	}
	hnRoadDiseaseInfo hitDisease;
	bool found = false;
	for (const hnRoadDiseaseInfo& disease : m_currentWidgetDiseases)
	{
		if (isSameSdkDisease(disease, diseaseKey))
		{
			hitDisease = disease;
			found = true;
			break;
		}
	}
	if (!found)
	{
		showSdkNoDiseaseModeHint(mouseEvent->pos());
		return true;
	}
	if (hitDisease.nDrawType == 3 || m_frameMode == FrameMode::DESIGN_LINE)
	{
		QMessageBox::information(this, QString::fromUtf8("\xE6\x8F\x90\xE7\xA4\xBA"),
			QString::fromUtf8("\xE8\xAE\xBE\xE8\xAE\xA1\xE6\xA8\xA1\xE5\xBC\x8F\xE7\xBA\xBF\xE7\x8A\xB6\xE7\x97\x85\xE5\xAE\xB3\xE6\x9A\x82\xE4\xB8\x8D\xE6\x94\xAF\xE6\x8C\x81\xE5\x90\x88\xE5\xB9\xB6\xE3\x80\x82\xE7\xBA\xBF\xE7\x8A\xB6\xE7\x97\x85\xE5\xAE\xB3\xE5\xAD\x98\xE5\x9C\xA8\xE7\x82\xB9\xE5\xBA\x8F\xE5\x92\x8C\xE6\x96\xB9\xE5\x90\x91\xEF\xBC\x8C\xE8\x87\xAA\xE5\x8A\xA8\xE6\x8B\xBC\xE6\x8E\xA5\xE5\xAE\xB9\xE6\x98\x93\xE4\xBA\xA7\xE7\x94\x9F\xE9\x94\x99\xE8\xAF\xAF\xE8\xBF\x9E\xE6\x8E\xA5\xEF\xBC\x9B\xE8\xAF\xB7\xE5\x88\xA0\xE9\x99\xA4\xE5\x90\x8E\xE9\x87\x8D\xE6\x96\xB0\xE7\xBB\x98\xE5\x88\xB6\xE4\xB8\x80\xE6\x9D\xA1\xE8\xBF\x9E\xE7\xBB\xAD\xE7\xBA\xBF\xE3\x80\x82"), QString::fromUtf8("\xE7\xA1\xAE\xE5\xAE\x9A"));
		m_seclectedDiseases.clear();
		refreshSdkDiseaseItems();
		return true;
	}
	for (int i = 0; i < m_seclectedDiseases.size(); ++i)
	{
		if (sdkDiseaseKey(m_seclectedDiseases.at(i)) == diseaseKey)
		{
			m_seclectedDiseases.removeAt(i);
			refreshSdkDiseaseItems();
			return true;
		}
	}
	if (m_seclectedDiseases.size() >= 2)
	{
		m_seclectedDiseases.clear();
	}
	if (!m_seclectedDiseases.isEmpty() &&
		(m_seclectedDiseases.first().nDrawType != hitDisease.nDrawType ||
			QString::fromLocal8Bit(m_seclectedDiseases.first().strDiseaseTableName) !=
			QString::fromLocal8Bit(hitDisease.strDiseaseTableName)))
	{
		QMessageBox::warning(this, QString::fromUtf8("\xE6\x8F\x90\xE7\xA4\xBA"),
			QString::fromUtf8("\xE5\x8F\xAA\xE8\x83\xBD\xE5\x90\x88\xE5\xB9\xB6\xE7\x9B\xB8\xE5\x90\x8C\xE7\xBB\x98\xE5\x88\xB6\xE7\xB1\xBB\xE5\x9E\x8B\xE5\x92\x8C\xE5\x90\x8C\xE4\xB8\x80\xE7\x97\x85\xE5\xAE\xB3\xE8\xA1\xA8\xE4\xB8\xAD\xE7\x9A\x84\xE7\x97\x85\xE5\xAE\xB3\xE3\x80\x82"), QString::fromUtf8("\xE7\xA1\xAE\xE5\xAE\x9A"));
		m_seclectedDiseases.clear();
		refreshSdkDiseaseItems();
		return true;
	}
	m_seclectedDiseases.append(hitDisease);
	refreshSdkDiseaseItems();
	if (m_seclectedDiseases.size() < 2)
	{
		return true;
	}
	if (m_seclectedDiseases.first().nDrawType == 1)
	{
		const QPoint noLegacyHitPoint(-100000, -100000);
		littleFrameMergeDiseases(noLegacyHitPoint);
		m_selectedSdkDiseaseKey.clear();
		clearSdkLittleFrameRenderCache();
		refreshSdkDiseaseLayer();
		return true;
	}
	if (m_seclectedDiseases.first().nDrawType == 0
		|| m_seclectedDiseases.first().nDrawType == 2)
	{
		mergeSdkAreaDiseases();
		return true;
	}
	QMessageBox::information(this, QString::fromUtf8("\xE6\x8F\x90\xE7\xA4\xBA"),
		QString::fromUtf8("\xE5\xBD\x93\xE5\x89\x8D\xE7\x97\x85\xE5\xAE\xB3\xE7\xBB\x98\xE5\x88\xB6\xE7\xB1\xBB\xE5\x9E\x8B\xE6\x9A\x82\xE4\xB8\x8D\xE6\x94\xAF\xE6\x8C\x81\xE5\x90\x88\xE5\xB9\xB6\xE3\x80\x82"), QString::fromUtf8("\xE7\xA1\xAE\xE5\xAE\x9A"));
	m_seclectedDiseases.clear();
	refreshSdkDiseaseItems();
	return true;
}
bool hn2d3dPixBaseWidget::findSdkLittleFrameDiseaseAtPoint(const pixImagePoint& point, hnRoadDiseaseInfo& disease, int& hitIndex)
{
	disease = hnRoadDiseaseInfo();
	hitIndex = -1;
	if (!m_sdkImageView)
	{
		return false;
	}

	const QPointF scenePoint = sdkPixPointToScenePoint(point);
	const QString preferredKey = m_sdkDiseaseGraphicsLayer.diseaseKeyAt(scenePoint);
	if (!preferredKey.isEmpty())
	{
		for (const hnRoadDiseaseInfo& candidate : m_currentWidgetDiseases)
		{
			if (!isSameSdkDisease(candidate, preferredKey))
			{
				continue;
			}
			const int index = sdkLittleFrameHitIndex(candidate, point);
			if (index >= 0)
			{
				disease = candidate;
				hitIndex = index;
				return true;
			}
		}
	}

	for (int i = static_cast<int>(m_currentWidgetDiseases.size()) - 1; i >= 0; --i)
	{
		const hnRoadDiseaseInfo& candidate = m_currentWidgetDiseases.at(i);
		const int index = sdkLittleFrameHitIndex(candidate, point);
		if (index >= 0)
		{
			disease = candidate;
			hitIndex = index;
			return true;
		}
	}
	return false;
}
void hn2d3dPixBaseWidget::keyPressEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_Escape && m_workMode == WorkMode::ADD_MODE)
	{
		slot_cancelDrawDiseases();
		clearContinuousDiseaseDrawing();
		event->accept();
		return;
	}

	//Qt::Key_Enter是小键盘的回车  Qt::Key_Return 是大键盘的回车
	if ((Qt::Key_Enter == event->key() || Qt::Key_Return == event->key())
		&& FrameMode::DESIGN_LINE == m_frameMode && WorkMode::ADD_MODE == m_workMode)
	{
		if (m_isDrawingDisease)
		{
		
			 for (int i = 0 ; i<	m_tmpPaintLineDiseasePoints.size();++i)
			 {
				 for (int j = 0 ; j<m_tmpLastPaintLineDiseasePoints.size();++j)
				 {
					 if (m_tmpPaintLineDiseasePoints[i] == m_tmpLastPaintLineDiseasePoints[j])
					 {
						 m_tmpPaintLineDiseasePoints.remove(i);
						 --i;
						 break;
					 }
				 }
			 }
			this->lineDiseaseAddDisease();
		}
	}

	// SDK 小框模式快捷键：B 直接进入折线补格模式，R 直接进入拉框填充模式。
	// 两个模式互斥，切换时清掉当前临时图形，避免上一轮点集继续参与新模式计算。
	if ((event->key() == Qt::Key_B || event->key() == Qt::Key_R)
		&& m_workMode == WorkMode::ADD_MODE && m_frameMode == FrameMode::LITTLE_FRAME)
	{
		resetLittleFrameDrawState();
		m_sdkDiseaseGraphicsLayer.clearTemporary();
		m_isDrawingDisease = false;
		m_isAllowDrawPix = (m_sdkImageView == nullptr);
		m_isAllowLinked = true;
		m_waitLittleFrameLeftPressAfterCancel = false;

		if (event->key() == Qt::Key_B)
		{
			this->addLineDiseType = true;
			this->littleDrawRectType = false;
		}
		else
		{
			this->addLineDiseType = false;
			this->littleDrawRectType = true;
		}

		refreshSdkTemporaryDiseaseItems();
		if (m_sdkImageView && m_sdkImageView->viewport())
		{
			m_sdkImageView->viewport()->update();
		}
		update();
		event->accept();
		return;
	}

	QWidget::keyPressEvent(event);
}

void hn2d3dPixBaseWidget::refreshSdkViewState()
{
    if (!m_sdkImageView)
    {
        return;
    }

    refreshSdkTemporaryDiseaseItems();
    const TiledViewAnchor anchor = m_sdkImageView->currentBottomAnchor();
    if (anchor.valid)
    {
        refreshStatusInfoFromSdkCurrentMouseOrAnchor(anchor);
    }
    updateSdkDiseaseOverlay();
    if (m_sdkImageView->scene())
    {
        m_sdkImageView->scene()->update();
    }
    if (m_sdkImageView->viewport())
    {
        m_sdkImageView->viewport()->update();
    }
    update();
}
void hn2d3dPixBaseWidget::refreshSdkDiseaseLayer()
{
	clearSdkLittleFrameRenderCache();
	if (m_sdkImageView)
	{
		// Drawing and hit-testing also resolve image aliases. A stale alias must not
		// decide where database diseases are rebuilt after diseaseChanged().
		m_sdkImageView->clearImageItemLookupCache();
		refreshSdkDiseaseItems();
		refreshSdkViewState();
		return;
	}

	updateSdkDiseaseOverlay();
	update();
}void hn2d3dPixBaseWidget::setCurrentStreetPictureNameForStatus(const QString& pictureName)
{
    m_currentStreetPictureNameForStatus = pictureName;
    refreshSdkViewState();
}

void hn2d3dPixBaseWidget::centerOnEncoderMile(double encoderMile)
{
	setSdkCenterEncoderMile(qMax(0.0, encoderMile));
	if (m_sdkImageView)
	{
		const TiledViewAnchor bottomAnchor = m_sdkImageView->currentBottomAnchor();
		if (bottomAnchor.valid)
		{
			syncLegacyBrowseStateFromEncoderMile(sdkAnchorToEncoderMile(bottomAnchor));
		}
	}
	refreshSdkDiseaseItems();
	refreshSdkViewState();
	QTimer::singleShot(0, this, [this]()
	{
		if (m_sdkImageView)
		{
			const TiledViewAnchor bottomAnchor = m_sdkImageView->currentBottomAnchor();
			if (bottomAnchor.valid)
			{
				syncLegacyBrowseStateFromEncoderMile(sdkAnchorToEncoderMile(bottomAnchor));
			}
		}
		refreshSdkDiseaseItems();
		refreshSdkViewState();
	});
}
void hn2d3dPixBaseWidget::scrollBottomToEncoderMile(double encoderMile)
{
    setSdkBottomEncoderMile(encoderMile);
    syncLegacyBrowseStateFromEncoderMile(encoderMile);
    // 程序定位后立即重建正式病害层，避免等待下一次滚动信号才显示当前视口数据库病害。
    refreshSdkDiseaseItems();
    refreshSdkViewState();
    QTimer::singleShot(0, this, [this]()
    {
        refreshSdkDiseaseItems();
        refreshSdkViewState();
    });
}

double hn2d3dPixBaseWidget::currentBottomEncoderMile() const
{
    if (m_sdkImageView)
    {
        return sdkAnchorToEncoderMile(m_sdkImageView->currentBottomAnchor());
    }

    if (m_imageDistanceMeters > 0.0)
    {
        return qMax(0.0, (m_buttomFrameIdx - 1.0) * m_imageDistanceMeters);
    }

    return 0.0;
}

void hn2d3dPixBaseWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::RightButton
		&& FrameMode::DESIGN_LINE == m_frameMode && WorkMode::ADD_MODE == m_workMode)
	{
		if (m_isDrawingDisease)
		{

			for (int i = 0; i < m_tmpPaintLineDiseasePoints.size(); ++i)
			{
				for (int j = 0; j < m_tmpLastPaintLineDiseasePoints.size(); ++j)
				{
					if (m_tmpPaintLineDiseasePoints[i] == m_tmpLastPaintLineDiseasePoints[j])
					{
						m_tmpPaintLineDiseasePoints.remove(i);
						--i;
						break;
					}
				}
			}
			this->lineDiseaseAddDisease();
			m_isDrawingDisease = false;
		} 
	}
	QWidget::mousePressEvent(event);
}

int hn2d3dPixBaseWidget::diseaseImagePixels(const QImage &image, int screenPixels) const
{
	if (screenPixels <=0)
	{
		return 1;
	}

	if (image.isNull()||width()<= 0 ||height()<=0)
	{
		return screenPixels;
	}

	const double xScale = image.width() *1.0 / width();
	const double yScale = image.height() *1.0 / height();
	const double scale = qMax(xScale, yScale);

	return qMax(1, qCeil(screenPixels * scale));
}

void hn2d3dPixBaseWidget::slot_cancelDrawDiseases()
{
	resetSdkDiseaseDrawingState(true);
}

void hn2d3dPixBaseWidget::clearContinuousDiseaseDrawing()
{
	m_isContinuousDiseaseDrawing = false;
	m_continuousDiseaseTypeName.clear();
}

bool hn2d3dPixBaseWidget::isContinuousDiseaseDrawing() const
{
	return m_isContinuousDiseaseDrawing && !m_continuousDiseaseTypeName.isEmpty();
}

bool hn2d3dPixBaseWidget::selectDiseaseTypeForDrawing(
	const QVector<hnDiseaseSetInfo>& diseaseSetInfos,
	QString& diseaseTypeName,
	QString& diseaseMark)
{
	diseaseTypeName.clear();
	diseaseMark.clear();

	if (isContinuousDiseaseDrawing())
	{
		for (const hnDiseaseSetInfo& diseaseSetInfo : diseaseSetInfos)
		{
			if (QString::fromLocal8Bit(diseaseSetInfo.strDiseaseTypeName) == m_continuousDiseaseTypeName)
			{
				diseaseTypeName = m_continuousDiseaseTypeName;
				return true;
			}
		}
		clearContinuousDiseaseDrawing();
	}

	QList<QPair<QString, QString> > diseaseNameAndKey;
	for (const hnDiseaseSetInfo& diseaseSetInfo : diseaseSetInfos)
	{
		diseaseNameAndKey.append(qMakePair(QString::fromLocal8Bit(diseaseSetInfo.strDiseaseTypeName),
			QString(diseaseSetInfo.nShortcutKey)));
	}

	addDiseaseDialog dialog(diseaseNameAndKey, false);
	dialog.setWindowTitle(QStringLiteral("\u6dfb\u52a0\u75c5\u5bb3"));
	dialog.setDiseaseAttributeEnabled(false);
	if (dialog.exec() != QDialog::Accepted)
	{
		return false;
	}

	diseaseTypeName = dialog.getDiseaseTypeName();
	diseaseMark = dialog.getDiseaseMarkInfo();
	if (diseaseTypeName.isEmpty())
	{
		return false;
	}

	m_isContinuousDiseaseDrawing = dialog.isContinuousDrawingEnabled();
	m_continuousDiseaseTypeName = m_isContinuousDiseaseDrawing ? diseaseTypeName : QString();
	if (m_isContinuousDiseaseDrawing)
	{
		QToolTip::showText(QCursor::pos(), QStringLiteral("\u5df2\u542f\u7528\u8fde\u7eed\u7ed8\u5236\uff1b\u6309 F1 \u66f4\u6362\u75c5\u5bb3\u7c7b\u578b"), this);
	}
	return true;
}



 

void hn2d3dPixBaseWidget::slot_deleteDisease(const hnRoadDiseaseInfo &disease)
{
	Q_UNUSED(disease);
	refreshSdkDiseaseLayer();
}

void hn2d3dPixBaseWidget::slot_moveMouse(bool up, bool is2D)
{
	m_isAllowDrawPix = true;
	if (this->m_isDrawingDisease &&
		this->m_frameMode == FrameMode::LITTLE_FRAME &&
		this->littleDrawRectType)
	{
		// R 拉框模式翻页时只浏览，不提前把经过区域填充成小框。
		refreshSdkTemporaryDiseaseItems();
		return;
	}

	if (!isDrawingLittleFrameDisease())
	{
		return;
	}

	scheduleMoveCursorToBestContinuePointAfterBrowse(up, is2D);
}

bool hn2d3dPixBaseWidget::temporaryDiseaseEncoderMileRange(int frameType, double& beginMile, double& endMile) const
{
	bool hasMile = false;
	beginMile = 0.0;
	endMile = 0.0;

	auto appendMile = [&](double mile)
	{
		if (!hasMile)
		{
			beginMile = mile;
			endMile = mile;
			hasMile = true;
			return;
		}
		beginMile = qMin(beginMile, mile);
		endMile = qMax(endMile, mile);
	};

	if (frameType == 0)
	{
		if (m_diseaseStartPoint.pixName.isEmpty() || m_diseaseEndPoint.pixName.isEmpty())
		{
			return false;
		}
		const QPoint startPoint = sdkPixPointToBigImagePoint(m_diseaseStartPoint);
		const QPoint endPoint = sdkPixPointToBigImagePoint(m_diseaseEndPoint);
		if (startPoint.x() < 0 || endPoint.x() < 0)
		{
			return false;
		}
		appendMile(const_cast<hn2d3dPixBaseWidget*>(this)->caculateEncoderMileByBigImagePoint(startPoint));
		appendMile(const_cast<hn2d3dPixBaseWidget*>(this)->caculateEncoderMileByBigImagePoint(endPoint));
	}
	else if (frameType == 1)
	{
		if (hasSdkImageView())
		{
			const QVector<LittleFrameSingleRectSelection> selections = currentLittleFrameSelectionsForSdk();
			for (const LittleFrameSingleRectSelection& selection : qAsConst(selections))
			{
				const QRect singleRect = selection.singleRect.normalized();
				if (selection.pixName.isEmpty() || !singleRect.isValid() || singleRect.isNull())
				{
					continue;
				}
				pixImagePoint topPoint;
				topPoint.pixName = selection.pixName;
				topPoint.pixPoint = singleRect.topLeft();
				pixImagePoint bottomPoint;
				bottomPoint.pixName = selection.pixName;
				bottomPoint.pixPoint = singleRect.bottomLeft();
				const QPoint topBigPoint = sdkPixPointToBigImagePoint(topPoint);
				const QPoint bottomBigPoint = sdkPixPointToBigImagePoint(bottomPoint);
				if (topBigPoint.x() >= 0)
				{
					appendMile(const_cast<hn2d3dPixBaseWidget*>(this)->caculateEncoderMileByBigImagePoint(topBigPoint));
				}
				if (bottomBigPoint.x() >= 0)
				{
					appendMile(const_cast<hn2d3dPixBaseWidget*>(this)->caculateEncoderMileByBigImagePoint(bottomBigPoint));
				}
			}
		}
		else
		{
			for (const QRect& rect : qAsConst(m_tmpLittleFrameDiseaseRects))
			{
				if (!rect.isValid() || rect.isNull())
				{
					continue;
				}
				const QRect normalized = rect.normalized();
				appendMile(const_cast<hn2d3dPixBaseWidget*>(this)->caculateEncoderMileByBigImagePoint(normalized.topLeft()));
				appendMile(const_cast<hn2d3dPixBaseWidget*>(this)->caculateEncoderMileByBigImagePoint(normalized.bottomLeft()));
			}
		}
	}
	else if (frameType == 3)
	{
		const QVector<pixImagePoint> linePoints = m_tmpPaintLineDiseasePoints.isEmpty()
			? m_tmpLineDiseasePoints
			: m_tmpPaintLineDiseasePoints;
		for (const pixImagePoint& linePoint : linePoints)
		{
			const QPoint bigImagePoint = sdkPixPointToBigImagePoint(linePoint);
			if (bigImagePoint.x() < 0)
			{
				continue;
			}
			appendMile(const_cast<hn2d3dPixBaseWidget*>(this)->caculateEncoderMileByBigImagePoint(bigImagePoint));
		}
	}
	else
	{
		return false;
	}

	return hasMile;
}

bool hn2d3dPixBaseWidget::isTmpDiseaseRoadMarkRangeValid(int frameType, QString* invalidReason) const
{
	if (!hnDataManager::getDataManager()->isOpenProject() || !hnDataManager::getDataManager()->getCurrentProject())
	{
		return true;
	}

	double beginMile = 0.0;
	double endMile = 0.0;
	if (!temporaryDiseaseEncoderMileRange(frameType, beginMile, endMile))
	{
		return true;
	}

	if (beginMile > endMile)
	{
		qSwap(beginMile, endMile);
	}

	const double eps = 0.01;
	const QVector<hnCommon::hnMarkInfo> marks = hnDataManager::getDataManager()->getCurrentProject()->getCurrentMarkVector();
	for (const hnCommon::hnMarkInfo& mark : marks)
	{
		if (!isRoadAttributeMarkType(mark.nType))
		{
			continue;
		}
		const double viewMarkMile = projectEncoderMileToSdkViewEncoderMile(mark.dEnclMile);
		if (viewMarkMile > beginMile + eps && viewMarkMile < endMile - eps)
		{
			if (invalidReason)
			{
				*invalidReason = QString::fromLocal8Bit("病害跨越%1打标边界，不能绘制，请在边界两侧分段绘制")
					.arg(sdkRoadMarkTypeName(mark.nType));
			}
			return false;
		}
	}

	return true;
}
QStringList hn2d3dPixBaseWidget::getDiseaseTypes()
{
	//获取当前病害种类
	QStringList result;
	const QVector<hnDiseaseSetInfo> diseaseSetInfos = this->getDiseaseSetInfos();

	for (auto diseseSetInfo : diseaseSetInfos)
	{
		result.append(QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName));
	}

	return result;
}

QVector<hnDiseaseSetInfo> hn2d3dPixBaseWidget::getDiseaseSetInfos()
{
	QVector<hnDiseaseSetInfo> result;

	//获取病害类型
	if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() == PROJECT_TYPE::PROJECT_23D_TYPE ||
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() == PROJECT_TYPE::PROJECT_2D_TYPE)
	{
		hnMile mile;
		if (sdkHnMileFromPoint(m_diseaseStartPoint, mile))
		{
			result = hnApp::hnDataManager::getDataManager()->getCurrentProjectRoadDiseases(mile);
		}
	}
	else
	{
		//单三维的就按人工模式的病害类型来
		hnProjectSetInfo setting = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
		result = hnApp::hnDataManager::getDataManager()->getRoadDisease(HnProjectEnums::roadTypeQStringToEnum(setting.strRoadStandard),
			(ROAD_WORK_TYPE)0, (ROAD_SURFACE_TYPE)setting.nRSurfaceType, 0);
	}

	return result;
}

bool hn2d3dPixBaseWidget::lineDiseaseAddDisease()
{
	if (hnApp::hnDataManager::getDataManager()->getCurrentProject() && !hnApp::hnDataManager::getDataManager()->getCurrentProject()->ensureInitialSurfaceMaterial(this)) return false;
	QVector<pixImagePoint> linePoints = m_tmpPaintLineDiseasePoints.isEmpty()
		? m_tmpLineDiseasePoints
		: m_tmpPaintLineDiseasePoints;
	if (linePoints.size() < 2)
	{
		resetSdkDiseaseDrawingState(true, true);
		return false;
	}
	m_tmpPaintLineDiseasePoints = linePoints;

	QSet<int> roadLevels;
	QSet<int> roadTypes;
	for (const pixImagePoint& linePoint : qAsConst(linePoints))
	{
		hnMile mile;
		if (!sdkHnMileFromPoint(linePoint, mile))
		{
			resetSdkDiseaseDrawingState(true, true);
			return false;
		}
		roadLevels.insert(static_cast<int>(mile.roadStandard));
		roadTypes.insert(static_cast<int>(mile.roadType));
	}
	if (roadLevels.size() >= 2 || roadTypes.size() >= 2)
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("所画病害中有不同的路面标准或者路面类型，病害无效，取消绘制"),
			QString::fromLocal8Bit("确定"));
		resetSdkDiseaseDrawingState(true, true);
		return false;
	}

	QString invalidMarkReason;
	if (!isTmpDiseaseRoadMarkRangeValid(3, &invalidMarkReason))
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			invalidMarkReason.isEmpty()
			? QString::fromLocal8Bit("所画病害跨越道路属性打标边界，病害无效，取消绘制")
			: invalidMarkReason,
			QString::fromLocal8Bit("确定"));
		resetSdkDiseaseDrawingState(true, true);
		return false;
	}
	hnDiseaseSetInfo selectDiseaseSetInfo;
	const auto diseaseSetInfos = getDiseaseSetInfos();
	QString diseaseTypeName;
	QString diseaseMark;
	if (!selectDiseaseTypeForDrawing(diseaseSetInfos, diseaseTypeName, diseaseMark))
	{
		resetSdkDiseaseDrawingState(true, true);
		return false;
	}

	QString diseaseTableName;
	for (auto diseseSetInfo : diseaseSetInfos)
	{
		if (QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName) == diseaseTypeName)
		{
			selectDiseaseSetInfo = diseseSetInfo;
			diseaseTableName = QString::fromLocal8Bit(diseseSetInfo.strDBTableName);
			break;
		}
	}
	if (diseaseTableName.isEmpty())
	{
		resetSdkDiseaseDrawingState(true, true);
		return false;
	}

	hnRoadDiseaseInfo diseaseInfo = caculateLineDiseaseInfo(linePoints, selectDiseaseSetInfo);
	diseaseInfo.nID = hnApp::hnDataManager::getDataManager()->getCurrentProject()
		->getDB()->getDiseaseTable()->getMaxID(diseaseTableName.toLocal8Bit().data());

	clearLastSdkAddedDisease();
	if (!validateDiseaseGeometryWithinValidArea(diseaseInfo))
	{
		resetSdkDiseaseDrawingState(true, true);
		return false;
	}
	if (!hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(diseaseInfo))
	{
		resetSdkDiseaseDrawingState(true, true);
		return false;
	}

	resetSdkDiseaseDrawingState();
	setSelectedDisease(diseaseInfo);
	ensureSdkDiseaseItemVisible(diseaseInfo);
	emit signal_selectDisease(diseaseInfo);
	return true;
}

void hn2d3dPixBaseWidget::appendLittleFrameDrawingPoint(const pixImagePoint& point)
{
	if (point.pixName.isEmpty() || point.pixPoint.x() < 0 || point.pixPoint.y() < 0)
	{
		return;
	}

	if (littleDrawRectType)
	{
		return;
	}

	if (!m_littleSingleImagePoints.isEmpty())
	{
		const pixImagePoint lastPoint = m_littleSingleImagePoints.last();
		if (lastPoint.pixName == point.pixName &&
			QLineF(lastPoint.pixPoint, point.pixPoint).length() <= 2.0)
		{
			return;
		}
	}

	m_littleSingleImagePoints.append(point);
}

bool hn2d3dPixBaseWidget::hasLittleFramePointInPix(const QString& pixName) const
{
	for (const auto& point : qAsConst(m_littleSingleImagePoints))
	{
		if (point.pixName == pixName)
		{
			return true;
		}
	}
	return false;
	
}

void hn2d3dPixBaseWidget::drawLineDiseases(const vector<hnRoadDiseaseInfo>& diseases, QImage &image)
{
	if (diseases.empty())
	{
		return;
	}

	QPainter painter(&image);
	QPen pen;
	pen.setWidth(m_diseaseDrawStyle.lineDiseaseWidth);
	pen.setColor(m_diseaseDrawStyle.lineDiseaseColor);
	painter.setPen(pen);

	for (auto disease : qAsConst(diseases))
	{
		if (3 != disease.nDrawType)
		{
			continue;
		}

		const bool selected = selectedDiseaseId == disease.nID;
		const bool mergeSelected = isSeclectedMergeDisease(disease);

		if (selected)
		{
			pen.setColor(m_diseaseDrawStyle.selectedRectColor);
			pen.setStyle(Qt::DashDotDotLine);
		}
		else if (mergeSelected)
		{
			pen.setColor(m_diseaseDrawStyle.mergedRectColor);
			pen.setStyle(Qt::SolidLine);
		}
		else
		{
			pen.setColor(m_diseaseDrawStyle.lineDiseaseColor);
			pen.setStyle(Qt::SolidLine);
		}

		pen.setWidth(selected
			? m_diseaseDrawStyle.selectedRectWidth
			: m_diseaseDrawStyle.lineDiseaseWidth);

		painter.setPen(pen);

		hnRoadDiseaseInfo diseaseCopy = disease;
		QVector<QPoint> points = createBrokenLinePoints(diseaseCopy);
		if (points.size() < 2)
		{
			continue;
		}

		for (int i = 0; i < points.size() - 1; i++)
		{
			QLine line(points.at(i), points.at(i + 1));
			painter.drawLine(line);
		}

		const int fontSize = selected
			? m_diseaseDrawStyle.selectedLabelFontSize
			: m_diseaseDrawStyle.lineLabelFontSize;

		const QString diseaseInfo = selected
			? buildLittleFrameDiseaseDetailLabel(disease, false)
			: buildLineDiseaseLabel(disease);

		drawDiseaseCalloutLabel(
			image,
			unitedRectOfPoints(points),
			diseaseInfo,
			fontSize,
			m_diseaseDrawStyle.labelTextColor,
			m_diseaseDrawStyle.calloutLineColor);
	}
}

void hn2d3dPixBaseWidget::drawTmpLineDiseases(QImage &image)
{
	if (m_tmpPaintLineDiseasePoints.isEmpty())
	{
		return;
	}

	QPainter painter(&image);
	QPen pen;
	pen.setWidth(m_diseaseDrawStyle.tempLineDiseaseWidth);
	pen.setStyle(Qt::DashLine);
	pen.setColor(m_diseaseDrawStyle.tempLineDiseaseColor);
	painter.setPen(pen);

	for (int i = 0; i < m_tmpPaintLineDiseasePoints.size() - 1; i++)
	{
		QPoint begin = singleImagePointToBigImagePoint(
			m_tmpPaintLineDiseasePoints.at(i).pixPoint,
			m_tmpPaintLineDiseasePoints.at(i).pixName);

		QPoint end = singleImagePointToBigImagePoint(
			m_tmpPaintLineDiseasePoints.at(i + 1).pixPoint,
			m_tmpPaintLineDiseasePoints.at(i + 1).pixName);

		painter.drawLine(QLine(begin, end));
	}
}

//2025.11.3 新增绘制最后一个点与鼠标连线（虚线）
void hn2d3dPixBaseWidget::drawTempDashLine(QImage &image)
{
	if (m_tempPoints.size() < 2)
	{
		return;
	}

	QPainter painter(&image);
	QPen pen;
	pen.setWidth(m_diseaseDrawStyle.tempLineDiseaseWidth);
	pen.setStyle(Qt::DashLine);
	pen.setColor(m_diseaseDrawStyle.tempDashLineColor);
	painter.setPen(pen);

	QPoint startPoint = singleImagePointToBigImagePoint(
		m_tempPoints[0].pixPoint,
		m_tempPoints[0].pixName);

	QPoint endPoint = singleImagePointToBigImagePoint(
		m_tempPoints[1].pixPoint,
		m_tempPoints[1].pixName);

	painter.drawLine(QLine(startPoint, endPoint));
}


double hn2d3dPixBaseWidget::calculateLineDiseaseCenterMile(QVector<pixImagePoint> lineDiseasePoints)
{
	double result = 0.0;

	if (true == lineDiseasePoints.isEmpty())
	{
		return result;
	}

	auto mileMap = this->createLineDiseaseMileMap(lineDiseasePoints);

	result = 0.5*(mileMap.first() + mileMap.last());

	return result;
}

double hn2d3dPixBaseWidget::calculateLineDiseaseBeginMile(QVector<pixImagePoint> lineDiseasePoints)
{
	double result = 0.0;
	if (true == lineDiseasePoints.isEmpty())
	{
		return result;
	}

	auto mileMap = this->createLineDiseaseMileMap(lineDiseasePoints);

	result = mileMap.first();

	return result;
}

double hn2d3dPixBaseWidget::calculateLineDiseaseEndMile(QVector<pixImagePoint> lineDiseasePoints)
{
	double result = 0.0;
	if (true == lineDiseasePoints.isEmpty())
	{
		return result;
	}

	auto mileMap = this->createLineDiseaseMileMap(lineDiseasePoints);

	result = mileMap.last();

	return result;
}

QMap<int, double> hn2d3dPixBaseWidget::createLineDiseaseMileMap(const QVector<pixImagePoint>& lineDiseasePoints)
{
	QMap<int, double> result;

	if (true == lineDiseasePoints.isEmpty())
	{
		return result;
	}
	if (nullptr == hnApp::hnDataManager::getDataManager()->getCurrentProject())
	{
		return result;
	}

	int count = 0;

	for (const pixImagePoint& lineDiseasePoint : qAsConst(lineDiseasePoints))
	{
		const double mile = sdkPixPointToEncoderMile(lineDiseasePoint);
		result.insert(count, mile);
		count++;
	}

	return result;
}

void hn2d3dPixBaseWidget::addLineDisease(const QPoint & widgetPoint)
{
	this->m_isDrawingDisease = true;
	this->m_isAllowDrawPix = false;

	QString pixName;
	QPoint singleImagePoint = this->screenToSingleImagePoint(widgetPoint, pixName);

	//三维视图画点越界（相对于二维视图）的处理
	QString className = this->metaObject()->className();
	if (PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType() && true == className.contains("3d"))
	{
		int x = singleImagePoint.x();
		this->autoCorrectXIn3dView(x);
		singleImagePoint.setX(x);
	}

	pixImagePoint pixPoint;
	pixPoint.pixName = pixName;
	pixPoint.pixPoint = singleImagePoint;

	if (true == m_tmpLineDiseasePoints.empty())
	{
		//记录开始点
		m_diseaseStartPoint.pixPoint = this->screenToSingleImagePoint(widgetPoint, m_diseaseStartPoint.pixName);
	}
	m_tmpLineDiseasePoints.append(pixPoint);
	m_tmpLastPaintLineDiseasePoints.clear();
}

void hn2d3dPixBaseWidget::commonDeleteDisease(const QPoint & mousePoint, hnFrameMode::FrameMode frameMode)
{
	for (auto disease : this->m_currentWidgetDiseases)
	{

		if (this->isInDisease(mousePoint, disease, frameMode))
		{
			//选中病害插入要删除的病害
			m_seclectedDiseases.clear();
			m_seclectedDiseases.append(disease);

			 
			hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(disease);

			m_isAllowDrawPix = true;
			//清空选中病害
			m_seclectedDiseases.clear();
			refreshSdkDiseaseLayer();
			return;
		}
	}
}

void hn2d3dPixBaseWidget::commonEditDisease(const QPoint & mousePoint, hnFrameMode::FrameMode frameMode)
{
	for (auto disease : this->m_currentWidgetDiseases)
	{
		if (this->isInDisease(mousePoint, disease, frameMode))
		{
			this->editDisease(disease, mousePoint);
		}
	}
}

void hn2d3dPixBaseWidget::mergeLineDisease(const QPoint & widgetPoint)
{
	//获取选中的病害，添加到数组中
	for (auto disease : this->m_currentWidgetDiseases)
	{
		if (this->isInDisease(widgetPoint, disease, hnFrameMode::DESIGN_LINE))
		{
			if (m_seclectedDiseases.isEmpty())
			{
				m_seclectedDiseases.append(disease);
				this->update();
				break;
			}
			else
			{
				auto existDisease = m_seclectedDiseases.at(0);
				if (QString::fromLocal8Bit(existDisease.strDisName) == QString::fromLocal8Bit(disease.strDisName)
					&& existDisease.nID == disease.nID)
				{
					QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("不能选择相同的病害合并"),
						QString::fromLocal8Bit("确定"));
					return;
				}
				else
				{
					m_seclectedDiseases.append(disease);
					this->update();
					break;
				}
			}
		}
	}

	//如果等于两个,就合并两个，放到数据库中，删除原来的两个
	if (2 == m_seclectedDiseases.size())
	{
		//新病害
		auto newDisease = m_seclectedDiseases.at(0);

		//新病害有些属性继承了合并病害的第一个，有很多属性要重新计算
		hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

		//清空坐标信息
		newDisease.vec3dRect.clear();
		newDisease.vec2dRect.clear();

		//计算2d的坐标信息
		if (nullptr != hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2DProject())
		{
			auto vec2dRect0 = m_seclectedDiseases.at(0).vec2dRect;
			auto vec2dRect1 = m_seclectedDiseases.at(1).vec2dRect;
			vec2dRect0.insert(vec2dRect0.end(), vec2dRect1.begin(), vec2dRect1.end());
			newDisease.vec2dRect = vec2dRect0;
		}

		//计算3d坐标信息
		if (nullptr != hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
		{
			auto vec3dRect0 = m_seclectedDiseases.at(0).vec3dRect;
			auto vec3dRect1 = m_seclectedDiseases.at(1).vec3dRect;
			vec3dRect0.insert(vec3dRect0.end(), vec3dRect1.begin(), vec3dRect1.end());
			newDisease.vec3dRect = vec3dRect0;
			//深度计算，如需
			//...

		}

		// 线状病害长度
		newDisease.dLength = m_seclectedDiseases.at(0).dLength + m_seclectedDiseases.at(1).dLength;

		// 获取病害的数组
		auto lineDiseaseBigImagePoints = this->createBrokenLinePoints(newDisease);
		QVector<pixImagePoint> pixImagePoints;
		for (auto lineDiseaseBigImagePoint : qAsConst(lineDiseaseBigImagePoints))
		{
			pixImagePoint point;
			point.pixPoint = this->bigImagePointToSingleImagePoint(lineDiseaseBigImagePoint, &point.pixName);
			pixImagePoints.append(point);
		}

		//中心里程
		newDisease.dMileage = this->calculateLineDiseaseCenterMile(pixImagePoints);
		//开始里程
		newDisease.dDmiStart = this->calculateLineDiseaseBeginMile(pixImagePoints);
		//结束里程
		newDisease.dDmiEnd = this->calculateLineDiseaseEndMile(pixImagePoints);

		//计算病害的计算面积 
		hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(  newDisease);

		//重新设置ID
		newDisease.nID = hnApp::hnDataManager::getDataManager()->getCurrentProject()
			->getDB()->getDiseaseTable()->getMaxID(newDisease.strDiseaseTableName);

	 

		if (!validateDiseaseGeometryWithinValidArea(newDisease))
		{
			return;
		}
		if (!hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(newDisease))
		{
			QMessageBox::warning(this, QStringLiteral("\u8b66\u544a"), QStringLiteral("\u5408\u5e76\u75c5\u5bb3\u5199\u5165\u5931\u8d25\uff0c\u539f\u75c5\u5bb3\u672a\u5220\u9664\u3002"), QStringLiteral("\u786e\u5b9a"));
			m_seclectedDiseases.clear();
			return;
		}
		auto firstDisease = m_seclectedDiseases.at(0);
		auto secondDisease = m_seclectedDiseases.at(1);
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(firstDisease);
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(secondDisease);

		this->update();

		//清空选中的病害数组
		m_seclectedDiseases.clear();
	}
}

bool hn2d3dPixBaseWidget::isNearbyLineDisease(const QPoint & bigImagePoint, const hnRoadDiseaseInfo & disease)
{
	const  int minDistance = 30;
	hnRoadDiseaseInfo info = disease;
	auto points = createBrokenLinePoints(info);
	if (true == points.empty())
	{
		return false;
	}
	for (int i = 0; i < points.size() - 1; i++)
	{
		const QLineF line(points.at(i), points.at(i + 1));
		const double distance = distanceFromPointToLine(bigImagePoint, line);
		if (distance <= minDistance)
		{
			return true;
		}
	}
	return false;
}



double hn2d3dPixBaseWidget::caculateLineDiseaseLenth(QVector<pixImagePoint> lineDiseasePoints, WidgetType widgetType)
{
	double result = 0.0;
	if (lineDiseasePoints.size() < 2)
	{
		return result;
	}

	// Use stable per-image coordinates; this calculation must not depend on the current viewport.
	double widthScale;
	if (WIDGET_2D == widgetType)
	{
		widthScale = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioX;
	}
	else if (WIDGET_3D == widgetType)
	{
		widthScale = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageWidthScale();
	}
	else
	{
		return result;
	}


	//算每个线段的长度，然后加起来
	for (int i = 0; i < lineDiseasePoints.size() - 1; ++i)
	{
		const pixImagePoint& first = lineDiseasePoints.at(i);
		const pixImagePoint& second = lineDiseasePoints.at(i + 1);
		const double width = (second.pixPoint.x() - first.pixPoint.x()) * widthScale;
		const double height = sdkPixPointToEncoderMile(second) - sdkPixPointToEncoderMile(first);
		result += qSqrt(width * width + height * height);
	}

	return result;
}

void hn2d3dPixBaseWidget::autoCorrectXIn3dView(int & x)
{
	const double roadWidth2d = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRoadWidth;
	const double roadWidth3d = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getRoadWidth();
	const double scale3d = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageWidthScale();
	const int pixel2d3dBoarderWidth = 0.5*((roadWidth3d - roadWidth2d) / scale3d);
	const int leftBoarder = pixel2d3dBoarderWidth;
	const int rightBoarder = m_pixWidth - pixel2d3dBoarderWidth;

	if (x < leftBoarder)
	{
		x = leftBoarder;
		return;
	}
	if (x > rightBoarder)
	{
		x = rightBoarder;
		return;
	}
}



QPoint hn2d3dPixBaseWidget::pixImagePointToBigImagePoint(const pixImagePoint & point)
{
	QPoint result;

	result = this->singleImagePointToBigImagePoint(point.pixPoint, point.pixName);

	return result;
}

void hn2d3dPixBaseWidget::clearVisibleLittleFrameRectCache()
{
	m_cachedVisibleLittleFrameRects.clear();
	m_cachedVisibleLittleFramePixNames.clear();
}

void hn2d3dPixBaseWidget::setLittleDiseaseSize(const QVector<QRect>& diseaseRects, hnRoadDiseaseInfo& disease)
{
	if (diseaseRects.isEmpty())
	{
		disease.nPixelLen = 0;
		disease.nPixelWid = 0;
		disease.dLength = 0.0;
		disease.dWidth = 0.0;
		disease.dRealLen = 0.0;
		disease.dReaWidth = 0.0;
		return;
	}

	QRect boundingRect = diseaseRects.first().normalized();
	const int rectHeight = qMax(1, qAbs(boundingRect.height()));
	const int rectWidth = qMax(1, qAbs(boundingRect.width()));
	for (int i = 1; i < diseaseRects.size(); ++i)
	{
		boundingRect = boundingRect.united(diseaseRects.at(i).normalized());
	}

	const int rowCount = qMax(1, static_cast<int>(std::round(static_cast<double>(qAbs(boundingRect.height())) / rectHeight)));
	const int columnCount = qMax(1, static_cast<int>(std::round(static_cast<double>(qAbs(boundingRect.width())) / rectWidth)));

	// 与大框一致：真实长宽来自最大外接矩形；计算长宽先按真实长宽初始化，再交给 setDiseaseCalcuteSize 按病害规则修正。
	disease.nPixelLen = rowCount * rectHeight;
	disease.nPixelWid = columnCount * rectWidth;
	disease.dLength = rowCount * 0.1;
	disease.dWidth = columnCount * 0.1;
	disease.dRealLen = disease.dLength;
	disease.dReaWidth = disease.dWidth;
}
void hn2d3dPixBaseWidget::reCalculateDiseaseSizeAndSave(hnCommon::hnRoadDiseaseInfo & disease, bool save)
{
	//自动化模式病害计算最大外接矩形尺寸，并且为了适配之前版本 尺寸为0的 情况在此处更新病害
	int drawType =  hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType;
	
	if (disease.nDrawType ==3 ||disease.nDrawType ==2)  //设计模式的不参与计算
	{
		return;
	}
	if (drawType != disease.nDrawType)
	{
		if (save)
		{
			disease.nDrawType = drawType;
			hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(disease);

			hnApp::hnDataManager::getDataManager()->getDiseaseService()->updateDisease(disease);
		 
		}
	}
	
}

void hn2d3dPixBaseWidget::reCalculateOldDiseaseSizeAndSave(const QVector<QRect>& diseaseRects,hnCommon::hnRoadDiseaseInfo& disease)
{
		setLittleDiseaseSize(diseaseRects,disease);
		hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(disease);
		 
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->updateDisease(disease);
}

void hn2d3dPixBaseWidget::CalculateDiseaseSize(const QVector<QRect>&diseaseRects, hnCommon::hnRoadDiseaseInfo& disease)
{
	if (disease.dLength == 0 || disease.dArea == 0)
	{ 
		setLittleDiseaseSize(diseaseRects, disease); 
	}
}
void hn2d3dPixBaseWidget::recalculateLittleFrameDiseaseAfterCellDelete(hnCommon::hnRoadDiseaseInfo& disease)
{
	disease.nRectCnt = disease.vec2dRect.size();
	disease.n3dCnt = disease.vec3dRect.size();
	disease.dArea = 0.01 * qMax(disease.nRectCnt, disease.n3dCnt);

	const QVector<QRect> diseaseRects = caculateLittleFrameBigImageRects(disease);
	if (diseaseRects.isEmpty())
	{
		disease.nPixelLen = 0;
		disease.nPixelWid = 0;
		disease.dLength = 0.0;
		disease.dWidth = 0.0;
		disease.dRealLen = 0.0;
		disease.dReaWidth = 0.0;
		return;
	}

	setLittleDiseaseSize(diseaseRects, disease);
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_SDK_LITTLE_DELETE_RECALC]"
		<< "table=" << QString::fromLocal8Bit(disease.strDiseaseTableName)
		<< "id=" << disease.nID
		<< "rect2d=" << disease.nRectCnt
		<< "rect3d=" << disease.n3dCnt
		<< "length=" << disease.dLength
		<< "width=" << disease.dWidth
		<< "realLen=" << disease.dRealLen
		<< "realWidth=" << disease.dReaWidth
		<< "area=" << disease.dArea;
	#endif
}

 bool hn2d3dPixBaseWidget::isDrawingLittleFrameDisease() const
{
	return this->m_workMode == WorkMode::ADD_MODE
		&& this->m_isDrawingDisease
		&& this->m_frameMode == FrameMode::LITTLE_FRAME
		&& !this->addLineDiseType
		&& !this->littleDrawRectType;
}


bool hn2d3dPixBaseWidget::ignoreMouseMoveAfterAutoCursorMove(QMouseEvent* event)
{
	if (!m_ignoreNextMouseMoveAfterAutoCursorMove)
	{
		return false;
	}

	m_ignoreNextMouseMoveAfterAutoCursorMove = false;
	

	this->m_isAllowDrawPix = true;
	rebuildLittleFrameBigImagePoints();
	this->update();
	if (event)
	{
		event->accept();
	}

	return true;
}


QRect hn2d3dPixBaseWidget::visibleImageWidgetRect() const
{
	// 如果以后图片不是铺满 widget，比如有黑边、边距，再由子类 override
	return this->rect().adjusted(2, 2, -2, -2);
}

bool hn2d3dPixBaseWidget::widgetPointToDiseasePoint(const QPoint& widgetPoint, pixImagePoint& point)
{
	QString pixName;
	QPoint pixPoint = this->screenToSingleImagePoint(widgetPoint, pixName);

	if (pixName.isEmpty())
	{
		return false;
	}

	if (pixPoint.x() < 0 || pixPoint.y() < 0)
	{
		return false;
	}

	point.pixName = pixName;
	point.pixPoint = pixPoint;
	return true;
}


void hn2d3dPixBaseWidget::rebuildLittleFrameBigImagePoints()
{
	m_litteBigImagePoints.clear();

	for (const auto& point : qAsConst(m_littleSingleImagePoints))
	{
		QPoint bigImagePoint = singleImagePointToBigImagePoint(point.pixPoint, point.pixName);
		m_litteBigImagePoints.append(bigImagePoint);
	}
}


void hn2d3dPixBaseWidget::rebuildCurrentLittleFrameSingleSelections(const QVector<QRect>& bigRects)
{
	m_currentLittleFrameSingleSelections.clear();
	for (const QRect& bigRect : bigRects)
	{
		QString pixName;
		QRect singleRect = this->bigImageRectToSingleImageRect(bigRect.normalized(), &pixName).normalized();
		if (pixName.isEmpty() || !singleRect.isValid() || singleRect.isNull())
		{
			continue;
		}

		LittleFrameSingleRectSelection selection;
		selection.pixName = pixName;
		selection.singleRect = singleRect;
		if (!m_currentLittleFrameSingleSelections.contains(selection))
		{
			m_currentLittleFrameSingleSelections.append(selection);
		}
	}
}

QRect hn2d3dPixBaseWidget::littleFrameSingleCellForPoint(const pixImagePoint& point)
{
	PhysicalGridSpec spec = sdkLittleFrameGridSpecForImage(point.pixName);
	GridCell cell = GridSelectionTool::cellForPoint(spec, QPointF(point.pixPoint));
	LittleFrameSingleRectSelection selection;
	if (!littleFrameSelectionFromGridCell(cell, selection))
	{
		return QRect();
	}
	return selection.singleRect;
}

PhysicalGridSpec hn2d3dPixBaseWidget::sdkLittleFrameGridSpecForImage(const QString& pixName) const
{
	PhysicalGridSpec spec;
	spec.imageName = pixName;
	spec.imagePixelSize = QSize(m_pixWidth, m_pixHeight);
	spec.cellSizeMeters = QSizeF(0.1, 0.1);

	auto dataManager = hnDataManager::getDataManager();
	if (!dataManager || !dataManager->isOpenProject() || !dataManager->getCurrentProject() ||
		m_pixWidth <= 0 || m_pixHeight <= 0)
	{
		return spec;
	}

	double widthScale = 0.0;
	double heightScale = 0.0;
	if (m_widgetType == WIDGET_3D && dataManager->getCurrentProject()->get3DProject())
	{
		widthScale = dataManager->getCurrentProject()->get3DProject()->getImageWidthScale();
		heightScale = dataManager->getCurrentProject()->get3DProject()->getImageHeightScale();
	}
	else
	{
		const hnCommon::hnProjectSetInfo projectSetInfo = dataManager->getCurrentProject()->getCurProSetInfo();
		widthScale = projectSetInfo.dRadioX;
		heightScale = projectSetInfo.dRadioY;
	}

	if (widthScale > 0.0 && heightScale > 0.0)
	{
		const hnPro::hnLineCameraInfo lineInfo = dataManager->getCurrentProject()->getLineCameraInfo();
		if (m_widgetType == WIDGET_2D && lineInfo.isLineCamera && lineInfo.validAreaConfigured)
		{
			const double gridLeft = m_isHMirrored
				? lineInfo.imageWidth - lineInfo.rightPixel
				: lineInfo.leftPixel;
			spec.physicalSizeMeters = QSizeF(lineInfo.roadWidthMeters(), m_pixHeight * heightScale);
			spec.gridAreaPixels = QRectF(gridLeft, 0.0,
				lineInfo.rightPixel - lineInfo.leftPixel, m_pixHeight);
		}
		else
		{
			spec.physicalSizeMeters = QSizeF(m_pixWidth * widthScale, m_pixHeight * heightScale);
			spec.gridAreaPixels = QRectF(0.0, 0.0, m_pixWidth, m_pixHeight);
		}
	}
	return spec;
}

bool hn2d3dPixBaseWidget::littleFrameSelectionFromGridCell(const GridCell& cell, LittleFrameSingleRectSelection& selection) const
{
	selection = LittleFrameSingleRectSelection();
	if (!cell.isValid() || cell.imageName.isEmpty())
	{
		return false;
	}

	const QRectF normalizedRect = cell.imageRect.normalized();
	const int left = qBound(0, qRound(normalizedRect.left()), m_pixWidth);
	const int top = qBound(0, qRound(normalizedRect.top()), m_pixHeight);
	const int right = qBound(0, qRound(normalizedRect.right()), m_pixWidth);
	const int bottom = qBound(0, qRound(normalizedRect.bottom()), m_pixHeight);
	const QRect singleRect(QPoint(left, top), QPoint(right, bottom));
	if (!singleRect.isValid() || singleRect.isNull())
	{
		return false;
	}

	selection.pixName = cell.imageName;
	selection.singleRect = singleRect.normalized();
	selection.row = cell.row;
	selection.column = cell.column;
	return true;
}

void hn2d3dPixBaseWidget::appendCurrentLittleFrameSingleSelection(const QString& pixName, const QRect& singleRect)
{
	const QRect normalizedRect = singleRect.normalized();
	if (pixName.isEmpty() || !normalizedRect.isValid() || normalizedRect.isNull())
	{
		return;
	}

	LittleFrameSingleRectSelection selection;
	selection.pixName = pixName;
	selection.singleRect = normalizedRect;
	if (!m_currentLittleFrameSingleSelections.contains(selection))
	{
		m_currentLittleFrameSingleSelections.append(selection);
	}
}

void hn2d3dPixBaseWidget::rebuildCurrentLittleFrameSingleSelectionsFromRect(const pixImagePoint& first, const pixImagePoint& second)
{
	m_currentLittleFrameSingleSelections.clear();
	if (first.pixName.isEmpty() || second.pixName.isEmpty() || m_pixWidth <= 0 || m_pixHeight <= 0)
	{
		return;
	}

	QVector<SdkSingleImageRect> singleRects;
	if (first.pixName != second.pixName)
	{
		currentSdkBigFrameSingleRects(singleRects);
	}
	else
	{
		SdkSingleImageRect singleRect;
		singleRect.pixName = first.pixName;
		singleRect.singleRect = QRect(first.pixPoint, second.pixPoint).normalized().intersected(QRect(0, 0, m_pixWidth, m_pixHeight));
		if (singleRect.isValid())
		{
			singleRects.append(singleRect);
		}
	}

	for (const SdkSingleImageRect& sdkRect : qAsConst(singleRects))
	{
		const PhysicalGridSpec spec = sdkLittleFrameGridSpecForImage(sdkRect.pixName);
		const QVector<GridCell> cells = GridSelectionTool::selectByRect(spec, QRectF(sdkRect.singleRect), GridRectHitMode::Intersects);
		for (const GridCell& cell : cells)
		{
			LittleFrameSingleRectSelection selection;
			if (littleFrameSelectionFromGridCell(cell, selection) && !m_currentLittleFrameSingleSelections.contains(selection))
			{
				m_currentLittleFrameSingleSelections.append(selection);
			}
		}
	}
}

void hn2d3dPixBaseWidget::rebuildCurrentLittleFrameSingleSelectionsFromPoints(const QVector<pixImagePoint>& points)
{
	m_currentLittleFrameSingleSelections.clear();
	if (points.isEmpty())
	{
		return;
	}

	QVector<pixImagePoint> expandedPoints = points;
	addInterpolatedLittleFramePointsForSdkLine(expandedPoints);

	auto appendCell = [this](const GridCell& cell)
	{
		LittleFrameSingleRectSelection selection;
		if (littleFrameSelectionFromGridCell(cell, selection) && !m_currentLittleFrameSingleSelections.contains(selection))
		{
			m_currentLittleFrameSingleSelections.append(selection);
		}
	};

	appendCell(GridSelectionTool::cellForPoint(sdkLittleFrameGridSpecForImage(expandedPoints.first().pixName), QPointF(expandedPoints.first().pixPoint)));
	for (int i = 1; i < expandedPoints.size(); ++i)
	{
		const pixImagePoint previous = expandedPoints[i - 1];
		const pixImagePoint current = expandedPoints[i];
		if (previous.pixName != current.pixName)
		{
			appendCell(GridSelectionTool::cellForPoint(sdkLittleFrameGridSpecForImage(current.pixName), QPointF(current.pixPoint)));
			continue;
		}

		const PhysicalGridSpec spec = sdkLittleFrameGridSpecForImage(current.pixName);
		QVector<QPointF> linePoints;
		linePoints << QPointF(previous.pixPoint) << QPointF(current.pixPoint);
		const QVector<GridCell> cells = GridSelectionTool::selectByPolyline(spec, linePoints);
		for (const GridCell& cell : cells)
		{
			appendCell(cell);
		}
	}
}
void hn2d3dPixBaseWidget::commitCurrentLittleRectDrawSelection()
{
	if (!this->m_isDrawingDisease ||
		this->m_frameMode != FrameMode::LITTLE_FRAME ||
		!this->littleDrawRectType)
	{
		return;
	}

	if (m_currentLittleFrameSingleSelections.isEmpty())
	{
		// SDK rect mode builds grid selections directly; do not use old QRect/crossRectOver here.
		rebuildCurrentLittleFrameSingleSelectionsFromRect(m_diseaseStartPoint, m_diseaseEndPoint);
	}

	for (const LittleFrameSingleRectSelection& selection : qAsConst(m_currentLittleFrameSingleSelections))
	{
		if (!m_committedLittleFrameDiseaseRects.contains(selection))
		{
			m_committedLittleFrameDiseaseRects.append(selection);
		}
	}

	m_currentLittleFrameSingleSelections = m_committedLittleFrameDiseaseRects;
}


QVector<hn2d3dPixBaseWidget::LittleFrameSingleRectSelection> hn2d3dPixBaseWidget::currentLittleFrameSelectionsForSdk() const
{
	QVector<LittleFrameSingleRectSelection> selections = m_committedLittleFrameDiseaseRects;
	for (const LittleFrameSingleRectSelection& selection : qAsConst(m_currentLittleFrameSingleSelections))
	{
		if (!selections.contains(selection))
		{
			selections.append(selection);
		}
	}
	return selections;
}

void hn2d3dPixBaseWidget::buildCurrentLittleFrameStorageRects(QVector<QRect>& rects) const
{
	rects.clear();
	const QVector<LittleFrameSingleRectSelection> selections = currentLittleFrameSelectionsForSdk();
	for (const LittleFrameSingleRectSelection& selection : qAsConst(selections))
	{
		const QRect singleRect = selection.singleRect.normalized();
		if (selection.pixName.isEmpty() || !singleRect.isValid() || singleRect.isNull())
		{
			continue;
		}

		QRect bigRect = const_cast<hn2d3dPixBaseWidget*>(this)->singleImageRectToBigImageRect(singleRect, selection.pixName).normalized();
		if (bigRect.isValid() && !bigRect.isNull() && !rects.contains(bigRect))
		{
			rects.append(bigRect);
		}
	}
}

void hn2d3dPixBaseWidget::appendCommittedLittleRectDrawSelection(QVector<QRect>& rects)
{
	for (const LittleFrameSingleRectSelection& selection : qAsConst(m_committedLittleFrameDiseaseRects))
	{
		QRect bigRect = this->singleImageRectToBigImageRect(selection.singleRect, selection.pixName).normalized();
		if (!rects.contains(bigRect))
		{
			rects.append(bigRect);
		}
	}
}

void hn2d3dPixBaseWidget::clearLittleRectDrawSelection()
{
	m_committedLittleFrameDiseaseRects.clear();
	m_currentLittleFrameSingleSelections.clear();
	m_tmpLittleFrameDiseaseRects.clear();
}


void hn2d3dPixBaseWidget::resetLittleFrameDrawState()
{
	clearLittleRectDrawSelection();
	clearVisibleLittleFrameRectCache();
	m_diseaseEndPoint.pixName.clear();
	m_diseaseEndPoint.pixPoint = QPoint(-1, -1);

	m_diseaseStartPoint.pixName.clear();
	m_diseaseStartPoint.pixPoint = QPoint(-1, -1);

	m_diseaseAddPoint.pixName.clear();
	m_diseaseAddPoint.pixPoint = QPoint(-1,-1);


	m_littleSingleImagePoints.clear();
	m_litteBigImagePoints.clear();


	m_tmpLineDiseasePoints.clear();
	m_tmpPaintLineDiseasePoints.clear();
	m_tmpLastPaintLineDiseasePoints.clear();
	m_tempPoints.clear();


	m_isEndAddPoint = false;

	m_ignoreNextMouseMoveAfterAutoCursorMove = false;
}

void hn2d3dPixBaseWidget::resetSdkDiseaseDrawingState(bool clearSelection, bool exitAddMode)
{
	resetLittleFrameDrawState();
	m_sdkDiseaseGraphicsLayer.clearTemporary();
	clearLastSdkAddedDisease();
	m_isDrawingDisease = false;
	// SDK 视图存在时旧 paintEvent 不再负责显示，不能在清理绘制状态时重新打开旧绘图。
	m_isAllowDrawPix = (m_sdkImageView == nullptr);
	m_isAllowLinked = true;
	if (!exitAddMode && m_workMode == WorkMode::ADD_MODE && m_frameMode == FrameMode::LITTLE_FRAME)
	{
		m_waitLittleFrameLeftPressAfterCancel = true;
	}
	if (exitAddMode && m_workMode == WorkMode::ADD_MODE)
	{
		setMode(WorkMode::NO_MODE);
	}
	unsetCursor();
	if (clearSelection)
	{
		m_seclectedDiseases.clear();
	}
	update();
	updateSdkDiseaseOverlay();
	if (m_sdkImageView && m_sdkImageView->viewport())
	{
		m_sdkImageView->viewport()->update();
	}
}

void hn2d3dPixBaseWidget::refreshSdkDrawingAfterBrowse(const QPoint& viewportPoint)
{
	if (!m_sdkImageView || !m_sdkImageView->viewport() || !m_isDrawingDisease)
	{
		return;
	}

	QPoint effectivePoint = viewportPoint;
	if (!m_sdkImageView->viewport()->rect().contains(effectivePoint))
	{
		effectivePoint = m_sdkImageView->viewport()->mapFromGlobal(QCursor::pos());
	}
	if (!m_sdkImageView->viewport()->rect().contains(effectivePoint))
	{
		return;
	}

	pixImagePoint point;
	if (!sdkScenePointToDiseasePoint(m_sdkImageView->mapToScene(effectivePoint), point))
	{
		return;
	}

	if (m_widgetType == WIDGET_3D && hnDataManager::getDataManager()->getCurrentProject() &&
		PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
	{
		int x = point.pixPoint.x();
		autoCorrectXIn3dView(x);
		point.pixPoint.setX(x);
	}

	if (m_frameMode == FrameMode::LITTLE_FRAME)
	{
		if (addLineDiseType)
		{
			// B 折线模式浏览时只更新临时尾线，固定点仍只能由左键产生。
			m_diseaseEndPoint = point;
			m_tempPoints.clear();
			if (!m_littleSingleImagePoints.isEmpty())
			{
				m_tempPoints.push_back(m_littleSingleImagePoints.last());
				m_tempPoints.push_back(point);
			}
		}
		// R 拉框和普通轨迹模式浏览时不改变几何，避免鼠标不动但路面变化的区间被自动填充。
	}
	else
	{
		m_diseaseEndPoint = point;
	}

	refreshSdkTemporaryDiseaseItems();
	if (m_sdkImageView->scene())
	{
		m_sdkImageView->scene()->update();
	}
	m_sdkImageView->viewport()->update();
}

bool hn2d3dPixBaseWidget::isSdkPlainLittleFrameDrawing() const
{
	return m_sdkImageView
		&& m_isDrawingDisease
		&& m_frameMode == FrameMode::LITTLE_FRAME
		&& !addLineDiseType
		&& !littleDrawRectType;
}

void hn2d3dPixBaseWidget::rememberLittleFrameWheelAnchor(const QPoint& viewportPoint)
{
	m_hasLittleFrameWheelAnchorPoint = false;
	m_littleFrameWheelAnchorPoint = pixImagePoint();
	m_littleFrameWheelViewportPoint = QPoint();
	if (!isSdkPlainLittleFrameDrawing() || !m_sdkImageView || !m_sdkImageView->viewport())
	{
		return;
	}

	if (!m_sdkImageView->viewport()->rect().contains(viewportPoint))
	{
		return;
	}

	pixImagePoint point;
	if (!sdkScenePointToDiseasePoint(m_sdkImageView->mapToScene(viewportPoint), point))
	{
		return;
	}

	if (m_widgetType == WIDGET_3D && hnDataManager::getDataManager()->getCurrentProject() &&
		PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
	{
		int x = point.pixPoint.x();
		autoCorrectXIn3dView(x);
		point.pixPoint.setX(x);
	}

	m_littleFrameWheelAnchorPoint = point;
	m_littleFrameWheelViewportPoint = viewportPoint;
	m_hasLittleFrameWheelAnchorPoint = true;
}

void hn2d3dPixBaseWidget::restoreLittleFrameCursorAfterWheelBrowse()
{
	if (!m_hasLittleFrameWheelAnchorPoint)
	{
		return;
	}

	const pixImagePoint anchorPoint = m_littleFrameWheelAnchorPoint;
	const QPoint wheelViewportPoint = m_littleFrameWheelViewportPoint;
	m_hasLittleFrameWheelAnchorPoint = false;
	m_littleFrameWheelAnchorPoint = pixImagePoint();
	m_littleFrameWheelViewportPoint = QPoint();

	if (!isSdkPlainLittleFrameDrawing() || !m_sdkImageView || !m_sdkImageView->viewport())
	{
		return;
	}

	QRect safeRect = m_sdkImageView->viewport()->rect().adjusted(4, 4, -4, -4);
	if (!safeRect.isValid() || safeRect.isEmpty())
	{
		safeRect = m_sdkImageView->viewport()->rect();
	}
	if (!safeRect.isValid() || safeRect.isEmpty())
	{
		return;
	}

	const QPointF anchorScenePoint = sdkPixPointToScenePoint(anchorPoint);
	QPoint targetViewportPoint = m_sdkImageView->mapFromScene(anchorScenePoint);
	// 普通滚轮只改变纵向浏览位置。不要用 scene 反算后的 X 移动物理鼠标，
	// 否则窗口跨 150%/100% DPI 屏幕后，Qt/Windows 的全局坐标换算会把鼠标推到屏幕左侧。
	targetViewportPoint.setX(qBound(safeRect.left(), wheelViewportPoint.x(), safeRect.right()));
	targetViewportPoint.setY(qBound(safeRect.top(), targetViewportPoint.y(), safeRect.bottom()));

	pixImagePoint targetPoint = anchorPoint;
	if (!sdkScenePointToDiseasePoint(m_sdkImageView->mapToScene(targetViewportPoint), targetPoint))
	{
		targetPoint = anchorPoint;
	}
	else if (m_widgetType == WIDGET_3D && hnDataManager::getDataManager()->getCurrentProject() &&
		PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
	{
		int x = targetPoint.pixPoint.x();
		autoCorrectXIn3dView(x);
		targetPoint.pixPoint.setX(x);
	}

	m_diseaseEndPoint = targetPoint;
	updateSdkLittleFramePreview();
	refreshSdkTemporaryDiseaseItems();

	QPoint globalTarget = m_sdkImageView->viewport()->mapToGlobal(targetViewportPoint);
	const QPoint currentGlobalCursor = QCursor::pos();
	// setPos 使用物理屏幕坐标。保留当前全局 X，只恢复纵向位置，可规避混合 DPI 的 X 缩放差异。
	globalTarget.setX(currentGlobalCursor.x());
	if (currentGlobalCursor != globalTarget)
	{
		m_ignoreNextMouseMoveAfterAutoCursorMove = true;
		QCursor::setPos(globalTarget);
	}

	if (m_sdkImageView->scene())
	{
		m_sdkImageView->scene()->update();
	}
	m_sdkImageView->viewport()->update();
}

void hn2d3dPixBaseWidget::addInterpolatedLittleFramePointsForSdkLine(QVector<pixImagePoint>& points) const
{
	if (points.size() < 2 || !m_sdkImageView)
	{
		return;
	}

	auto appendPointIfChanged = [](QVector<pixImagePoint>& dst, const pixImagePoint& point)
	{
		if (point.pixName.isEmpty())
		{
			return;
		}
		if (!dst.isEmpty() && dst.last().pixName == point.pixName && dst.last().pixPoint == point.pixPoint)
		{
			return;
		}
		dst.append(point);
	};

	QVector<pixImagePoint> expanded;
	for (int i = 0; i < points.size() - 1; ++i)
	{
		const pixImagePoint startPoint = points.at(i);
		const pixImagePoint endPoint = points.at(i + 1);
		appendPointIfChanged(expanded, startPoint);
		if (startPoint.pixName.isEmpty() || endPoint.pixName.isEmpty())
		{
			appendPointIfChanged(expanded, endPoint);
			continue;
		}

		const QPointF startScene = sdkPixPointToScenePoint(startPoint);
		const QPointF endScene = sdkPixPointToScenePoint(endPoint);
		const QLineF sceneLine(startScene, endScene);
		if (sceneLine.length() <= 0.1)
		{
			appendPointIfChanged(expanded, endPoint);
			continue;
		}

		const PhysicalGridSpec startSpec = sdkLittleFrameGridSpecForImage(startPoint.pixName);
		const QRectF gridArea = startSpec.effectiveGridArea();
		const qreal cellWidth = startSpec.columnCount() > 0 ? gridArea.width() / startSpec.columnCount() : 20.0;
		const qreal cellHeight = startSpec.rowCount() > 0 ? gridArea.height() / startSpec.rowCount() : 20.0;
		const qreal sampleStep = qMax<qreal>(2.0, qMin(qAbs(cellWidth), qAbs(cellHeight)) * 0.5);
		const int stepCount = qMax(1, static_cast<int>(qCeil(sceneLine.length() / sampleStep)));

		// SDK scene 是当前显示的唯一坐标真相源。沿黄色预览线采样，再反查单图坐标。
		for (int step = 1; step < stepCount; ++step)
		{
			const qreal ratio = step * 1.0 / stepCount;
			const QPointF scenePoint = startScene + (endScene - startScene) * ratio;
			pixImagePoint interpolatedPoint;
			if (sdkScenePointToDiseasePoint(scenePoint, interpolatedPoint))
			{
				appendPointIfChanged(expanded, interpolatedPoint);
			}
		}
		appendPointIfChanged(expanded, endPoint);
	}
	appendPointIfChanged(expanded, points.last());
	points = expanded;
}
void hn2d3dPixBaseWidget::syncLittleFrameContinueAnchor(const QPoint& targetWidgetPoint)
{
	pixImagePoint anchorPoint;
	if (!widgetPointToDiseasePoint(targetWidgetPoint, anchorPoint))
	{
		return;
	}

	// 三维 23D 项目里，x 需要限制到有效路面范围
	if (m_widgetType == WIDGET_3D &&
		hnDataManager::getDataManager()->getCurrentProject() &&
		PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
	{
		int x = anchorPoint.pixPoint.x();
		this->autoCorrectXIn3dView(x);
		anchorPoint.pixPoint.setX(x);
	}

	m_diseaseEndPoint = anchorPoint;


	// 如果最后一个点和 anchor 非常接近，不重复追加
	if (!m_littleSingleImagePoints.isEmpty())
	{
		const pixImagePoint lastPoint = m_littleSingleImagePoints.last();

		if (lastPoint.pixName == anchorPoint.pixName &&
			QLineF(lastPoint.pixPoint, anchorPoint.pixPoint).length() <= 2.0)
		{
			rebuildLittleFrameBigImagePoints();
			return;
		}
	}

	// 这里追加 anchor 的目的：
	// 防止下一次用户轻微移动时，直接从翻页前的旧点连到新的鼠标点。
	// 这样后续绘制会从“最佳续画点”开始。
	m_littleSingleImagePoints.append(anchorPoint);
	rebuildLittleFrameBigImagePoints();
}


bool hn2d3dPixBaseWidget::moveCursorToBestContinuePointAfterBrowse(bool up, bool is2D)
{
	Q_UNUSED(is2D);

	if (!isDrawingLittleFrameDisease())
	{
		return false;
	}


	pixImagePoint lastPoint;
	if (littleDrawRectType&&
		!m_diseaseEndPoint.pixName.isEmpty()&&
		m_diseaseEndPoint.pixPoint.x() >=0 &&
		m_diseaseEndPoint.pixPoint.y() >=0)
	{
		lastPoint = m_diseaseEndPoint;
	}
	else
	{
		if (m_littleSingleImagePoints.isEmpty())
		{
			return false;
		}
		lastPoint = m_littleSingleImagePoints.last();
	}
	QPoint lastWidgetPoint;

	bool ok = diseasePointToWidgetPointAfterBrowse(lastPoint, up, lastWidgetPoint);
	 

	QRect visibleRect = visibleImageWidgetRect();

	if (!visibleRect.isValid() || visibleRect.isEmpty())
	{
		return false;
	}

	const int margin = 4;
	QRect safeRect = visibleRect.adjusted(margin, margin, -margin, -margin);

	QPoint targetPoint;

	if (ok)
	{
		// 这就是“当前可见区域内，距离 lastPoint 最近的点”
		targetPoint.setX(qBound(safeRect.left(), lastWidgetPoint.x(), safeRect.right()));
		targetPoint.setY(qBound(safeRect.top(), lastWidgetPoint.y(), safeRect.bottom()));
	}
	else
	{
		// 坐标转换失败时，用方向兜底
		QPoint currentWidgetPoint = this->mapFromGlobal(QCursor::pos());

		targetPoint.setX(qBound(safeRect.left(), currentWidgetPoint.x(), safeRect.right()));

		if (up)
		{
			// 向上翻 / 往前看：最后点大概率在当前视图下方
			targetPoint.setY(safeRect.bottom());
		}
		else
		{
			// 向下翻 / 往后看：最后点大概率在当前视图上方
			targetPoint.setY(safeRect.top());
		}
	}

	// 同步绘制锚点，防止下一次 mouseMove 从旧点连到错误点
	syncLittleFrameContinueAnchor(targetPoint);

	this->m_isAllowDrawPix = true;

	rebuildLittleFrameBigImagePoints();

	// 程序自动移动鼠标这一下不能参与绘制
	m_ignoreNextMouseMoveAfterAutoCursorMove = true;

	//移动鼠标到最佳续画点
	QCursor::setPos(this->mapToGlobal(targetPoint));

	this->setCursor(Qt::CrossCursor);
	this->update();

	return true;
}

void hn2d3dPixBaseWidget::scheduleMoveCursorToBestContinuePointAfterBrowse(bool up, bool is2D)
{
	Q_UNUSED(up);
	Q_UNUSED(is2D);
	// SDK browsing must not move the physical cursor after a wheel scroll.
	return;
}

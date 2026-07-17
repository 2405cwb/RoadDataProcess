#include "hnLineCameraValidAreaDialog.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollBar>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <functional>

namespace
{
	QPixmap loadPreviewPixmap(const QString& imagePath)
	{
		QFile imageFile(imagePath);
		if (!imageFile.open(QIODevice::ReadOnly))
		{
			return QPixmap();
		}
		const QByteArray encodedImage = imageFile.readAll();
		if (encodedImage.isEmpty())
		{
			return QPixmap();
		}

		try
		{
			const cv::Mat encodedBytes(1, encodedImage.size(), CV_8UC1,
				const_cast<char*>(encodedImage.constData()));
			const cv::Mat bgrImage = cv::imdecode(encodedBytes, cv::IMREAD_COLOR);
			if (bgrImage.empty())
			{
				return QPixmap();
			}
			cv::Mat rgbImage;
			cv::cvtColor(bgrImage, rgbImage, cv::COLOR_BGR2RGB);
			const QImage image(rgbImage.data, rgbImage.cols, rgbImage.rows,
				static_cast<int>(rgbImage.step), QImage::Format_RGB888);
			return QPixmap::fromImage(image.copy());
		}
		catch (const cv::Exception&)
		{
			return QPixmap();
		}
	}

	class BoundaryLineItem : public QGraphicsLineItem
	{
	public:
		BoundaryLineItem(bool leftBoundary, int imageWidth, qreal imageHeight, QGraphicsItem* parent = nullptr)
			: QGraphicsLineItem(parent), m_leftBoundary(leftBoundary), m_imageWidth(imageWidth)
		{
			setLine(QLineF(0.0, 0.0, 0.0, imageHeight));
			QPen boundaryPen(leftBoundary ? QColor(0, 220, 255) : QColor(255, 190, 0), 3.0);
			boundaryPen.setCosmetic(true);
			setPen(boundaryPen);
			setCursor(Qt::SizeHorCursor);
			setFlags(ItemIsMovable | ItemSendsGeometryChanges | ItemIsFocusable);
			setZValue(1001.0);
		}

		std::function<QPair<qreal, qreal>()> allowedRange;
		std::function<void(bool)> selected;
		std::function<void()> changed;

	protected:
		QVariant itemChange(GraphicsItemChange change, const QVariant& value) override
		{
			if (change == ItemPositionChange)
			{
				QPointF next = value.toPointF();
				const QPair<qreal, qreal> range = allowedRange ? allowedRange() : qMakePair<qreal, qreal>(0.0, m_imageWidth);
				next.setX(qBound(range.first, next.x(), range.second));
				next.setY(0.0);
				return next;
			}
			if (change == ItemPositionHasChanged && changed)
			{
				changed();
			}
			return QGraphicsLineItem::itemChange(change, value);
		}

		void mousePressEvent(QGraphicsSceneMouseEvent* event) override
		{
			if (selected)
			{
				selected(m_leftBoundary);
			}
			QGraphicsLineItem::mousePressEvent(event);
		}

	private:
		bool m_leftBoundary = false;
		int m_imageWidth = 0;
	};
}

class hnLineCameraValidAreaDialog::PreviewView : public QGraphicsView
{
public:
	explicit PreviewView(const QPixmap& image, QWidget* parent = nullptr)
		: QGraphicsView(parent), m_imageWidth(image.width()), m_imageHeight(image.height())
	{
		setScene(&m_scene);
		m_scene.setSceneRect(QRectF(0.0, 0.0, image.width(), image.height()));
		m_scene.addPixmap(image)->setZValue(0.0);
		m_leftShade = m_scene.addRect(QRectF(), Qt::NoPen, QColor(0, 0, 0, 145));
		m_rightShade = m_scene.addRect(QRectF(), Qt::NoPen, QColor(0, 0, 0, 145));
		m_leftShade->setZValue(1000.0);
		m_rightShade->setZValue(1000.0);
		m_leftShade->setAcceptedMouseButtons(Qt::NoButton);
		m_rightShade->setAcceptedMouseButtons(Qt::NoButton);
		m_leftLine = new BoundaryLineItem(true, image.width(), image.height());
		m_rightLine = new BoundaryLineItem(false, image.width(), image.height());
		m_scene.addItem(m_leftLine);
		m_scene.addItem(m_rightLine);
		m_leftLine->allowedRange = [this]() { return qMakePair<qreal, qreal>(0.0, rightPixel() - 1.0); };
		m_rightLine->allowedRange = [this]() { return qMakePair<qreal, qreal>(leftPixel() + 1.0, m_imageWidth); };
		m_leftLine->selected = [this](bool) { m_activeLeft = true; };
		m_rightLine->selected = [this](bool) { m_activeLeft = false; };
		m_leftLine->changed = [this]() { refreshOverlay(); };
		m_rightLine->changed = [this]() { refreshOverlay(); };
		setRenderHint(QPainter::SmoothPixmapTransform, true);
		setDragMode(QGraphicsView::NoDrag);
		setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
		setResizeAnchor(QGraphicsView::AnchorViewCenter);
		setFocusPolicy(Qt::StrongFocus);
		setMinimumSize(760, 430);
	}

	~PreviewView() override
	{
		// m_scene is a value member and is destroyed before the QGraphicsView base.
		// Detach it while it is still alive so the base destructor cannot access a
		// dangling scene pointer.
		setScene(nullptr);
	}

	std::function<void()> boundsChanged;
	int leftPixel() const { return qBound(0, qRound(m_leftLine->pos().x()), m_imageWidth); }
	int rightPixel() const { return qBound(0, qRound(m_rightLine->pos().x()), m_imageWidth); }

	void setBounds(int left, int right)
	{
		const int boundedRight = qBound(1, right, m_imageWidth);
		const int boundedLeft = qBound(0, left, boundedRight - 1);
		// Each line is constrained by the other line's current position. Move the
		// side that expands the available interval first so a valid direct input
		// cannot be clipped by the previous bounds.
		if (boundedRight <= leftPixel())
		{
			m_leftLine->setPos(boundedLeft, 0.0);
			m_rightLine->setPos(boundedRight, 0.0);
		}
		else
		{
			m_rightLine->setPos(boundedRight, 0.0);
			m_leftLine->setPos(boundedLeft, 0.0);
		}
		refreshOverlay();
	}

	void resetToFullImage()
	{
		setBounds(0, m_imageWidth);
	}

	void fitImage()
	{
		fitInView(m_scene.sceneRect(), Qt::KeepAspectRatio);
	}

	void nudgeActive(int delta)
	{
		BoundaryLineItem* line = m_activeLeft ? m_leftLine : m_rightLine;
		line->setPos(line->pos().x() + delta, 0.0);
	}

	protected:
		void wheelEvent(QWheelEvent* event) override
		{
			const qreal factor = event->angleDelta().y() > 0 ? 1.15 : (1.0 / 1.15);
			scale(factor, factor);
			event->accept();
		}

		void mousePressEvent(QMouseEvent* event) override
		{
			if (event->button() == Qt::MiddleButton)
			{
				m_panning = true;
				m_lastPanPoint = event->pos();
				setCursor(Qt::ClosedHandCursor);
				event->accept();
				return;
			}
			QGraphicsView::mousePressEvent(event);
		}

		void mouseMoveEvent(QMouseEvent* event) override
		{
			if (m_panning)
			{
				const QPoint delta = event->pos() - m_lastPanPoint;
				m_lastPanPoint = event->pos();
				horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
				verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
				event->accept();
				return;
			}
			QGraphicsView::mouseMoveEvent(event);
		}

		void mouseReleaseEvent(QMouseEvent* event) override
		{
			if (m_panning && event->button() == Qt::MiddleButton)
			{
				m_panning = false;
				unsetCursor();
				event->accept();
				return;
			}
			QGraphicsView::mouseReleaseEvent(event);
		}

		void keyPressEvent(QKeyEvent* event) override
		{
			if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
			{
				const int step = (event->modifiers() & Qt::ShiftModifier) ? 10 : 1;
				nudgeActive(event->key() == Qt::Key_Left ? -step : step);
				event->accept();
				return;
			}
			QGraphicsView::keyPressEvent(event);
		}

	private:
		void refreshOverlay()
		{
			const qreal left = leftPixel();
			const qreal right = rightPixel();
			m_leftShade->setRect(QRectF(0.0, 0.0, left, m_imageHeight));
			m_rightShade->setRect(QRectF(right, 0.0, m_imageWidth - right, m_imageHeight));
			if (boundsChanged)
			{
				boundsChanged();
			}
		}

		QGraphicsScene m_scene;
		int m_imageWidth = 0;
		int m_imageHeight = 0;
		bool m_activeLeft = true;
		bool m_panning = false;
		QPoint m_lastPanPoint;
		BoundaryLineItem* m_leftLine = nullptr;
		BoundaryLineItem* m_rightLine = nullptr;
		QGraphicsRectItem* m_leftShade = nullptr;
		QGraphicsRectItem* m_rightShade = nullptr;
};

hnLineCameraValidAreaDialog::hnLineCameraValidAreaDialog(const hnPro::hnLineCameraInfo& info, QWidget* parent)
	: QDialog(parent), m_info(info)
{
	setWindowTitle(QStringLiteral("设置线阵相机有效道路区域"));
	setModal(true);
	QPixmap preview = loadPreviewPixmap(info.previewImagePath);
	if (preview.isNull())
	{
		QMessageBox::critical(this, QStringLiteral("图片读取失败"),
			QStringLiteral("无法打开用于设置有效区域的图片：\n%1").arg(info.previewImagePath));
		QTimer::singleShot(0, this, &QDialog::reject);
		return;
	}
	m_previewView = new PreviewView(preview, this);
	const int initialLeft = info.validAreaConfigured ? info.leftPixel : 0;
	const int initialRight = info.validAreaConfigured ? info.rightPixel : preview.width();
	m_previewView->setBounds(initialLeft, initialRight);
	m_previewView->boundsChanged = [this]() { m_pendingInputMode = 0; updateSummary(); };

	m_summaryLabel = new QLabel(this);
	m_summaryLabel->setStyleSheet(QStringLiteral("font-size:14px; font-weight:600; color:#17324D; padding:6px;"));
	m_leftPixelEdit = new QLineEdit(this);
	m_rightPixelEdit = new QLineEdit(this);
	m_roadWidthEdit = new QLineEdit(this);
	m_leftPixelEdit->setValidator(new QIntValidator(0, preview.width(), m_leftPixelEdit));
	m_rightPixelEdit->setValidator(new QIntValidator(0, preview.width(), m_rightPixelEdit));
	QDoubleValidator* widthValidator = new QDoubleValidator(0.0,
		preview.width() * info.meterPerPixelWidth(), 6, m_roadWidthEdit);
	widthValidator->setNotation(QDoubleValidator::StandardNotation);
	m_roadWidthEdit->setValidator(widthValidator);
	m_leftPixelEdit->setMaximumWidth(110);
	m_rightPixelEdit->setMaximumWidth(110);
	m_roadWidthEdit->setMaximumWidth(130);
	connect(m_leftPixelEdit, &QLineEdit::textEdited, this, [this]() { m_pendingInputMode = 1; });
	connect(m_rightPixelEdit, &QLineEdit::textEdited, this, [this]() { m_pendingInputMode = 1; });
	connect(m_roadWidthEdit, &QLineEdit::textEdited, this, [this]() { m_pendingInputMode = 2; });
	connect(m_leftPixelEdit, &QLineEdit::returnPressed, this, [this]() { applyPixelInputs(); });
	connect(m_rightPixelEdit, &QLineEdit::returnPressed, this, [this]() { applyPixelInputs(); });
	connect(m_roadWidthEdit, &QLineEdit::returnPressed, this, [this]() { applyRoadWidthInput(); });

	QPushButton* applyPixelsButton = new QPushButton(QStringLiteral("\u5e94\u7528\u50cf\u7d20\u8fb9\u754c"), this);
	QPushButton* applyWidthButton = new QPushButton(QStringLiteral("\u6309\u5bbd\u5ea6\u5c45\u4e2d"), this);
	connect(applyPixelsButton, &QPushButton::clicked, this, [this]() { applyPixelInputs(); });
	connect(applyWidthButton, &QPushButton::clicked, this, [this]() { applyRoadWidthInput(); });
	QHBoxLayout* inputLayout = new QHBoxLayout();
	inputLayout->addWidget(new QLabel(QStringLiteral("\u5de6\u8fb9\u754c(px)"), this));
	inputLayout->addWidget(m_leftPixelEdit);
	inputLayout->addWidget(new QLabel(QStringLiteral("\u53f3\u8fb9\u754c(px)"), this));
	inputLayout->addWidget(m_rightPixelEdit);
	inputLayout->addWidget(applyPixelsButton);
	inputLayout->addSpacing(18);
	inputLayout->addWidget(new QLabel(QStringLiteral("\u9053\u8def\u5bbd\u5ea6(m)"), this));
	inputLayout->addWidget(m_roadWidthEdit);
	inputLayout->addWidget(applyWidthButton);
	inputLayout->addStretch();
	QLabel* helpLabel = new QLabel(QStringLiteral("拖动青色左边界和黄色右边界；中键拖动平移，滚轮缩放；方向键微调 1px，Shift+方向键微调 10px。"), this);
	helpLabel->setStyleSheet(QStringLiteral("color:#536273;"));
	QPushButton* resetButton = new QPushButton(QStringLiteral("恢复整图"), this);
	connect(resetButton, &QPushButton::clicked, this, [this]() { m_previewView->resetToFullImage(); });
	QPushButton* fitButton = new QPushButton(QStringLiteral("适应窗口"), this);
	connect(fitButton, &QPushButton::clicked, this, [this]() { m_previewView->fitImage(); });
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
	buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
	connect(buttons, &QDialogButtonBox::accepted, this, [this]()
	{
		if (applyPendingInput()) accept();
	});
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	QHBoxLayout* actionLayout = new QHBoxLayout();
	actionLayout->addWidget(resetButton);
	actionLayout->addWidget(fitButton);
	actionLayout->addStretch();
	actionLayout->addWidget(buttons);
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(helpLabel);
	layout->addWidget(m_previewView, 1);
	layout->addWidget(m_summaryLabel);
	layout->addLayout(inputLayout);
	layout->addLayout(actionLayout);
	resize(980, 700);
	updateSummary();
	QTimer::singleShot(0, m_previewView, [this]() { m_previewView->fitImage(); });
}

hnPro::hnLineCameraInfo hnLineCameraValidAreaDialog::selectedInfo() const
{
	hnPro::hnLineCameraInfo selected = m_info;
	selected.leftPixel = m_previewView->leftPixel();
	selected.rightPixel = m_previewView->rightPixel();
	selected.validAreaConfigured = selected.leftPixel >= 0 && selected.leftPixel < selected.rightPixel &&
		selected.rightPixel <= selected.imageWidth;
	return selected;
}

bool hnLineCameraValidAreaDialog::applyPixelInputs()
{
	bool leftOk = false;
	bool rightOk = false;
	const int left = m_leftPixelEdit ? m_leftPixelEdit->text().trimmed().toInt(&leftOk) : -1;
	const int right = m_rightPixelEdit ? m_rightPixelEdit->text().trimmed().toInt(&rightOk) : -1;
	if (!leftOk || !rightOk || left < 0 || left >= right || right > m_info.imageWidth)
	{
		QMessageBox::warning(this, QStringLiteral("\u8f93\u5165\u65e0\u6548"),
			QStringLiteral("\u50cf\u7d20\u8fb9\u754c\u5fc5\u987b\u6ee1\u8db3\uff1a0 <= \u5de6\u8fb9\u754c < \u53f3\u8fb9\u754c <= %1\u3002")
			.arg(m_info.imageWidth));
		return false;
	}
	m_previewView->setBounds(left, right);
	m_pendingInputMode = 0;
	updateSummary();
	return true;
}

bool hnLineCameraValidAreaDialog::applyRoadWidthInput()
{
	bool widthOk = false;
	const double requestedWidth = m_roadWidthEdit
		? m_roadWidthEdit->text().trimmed().toDouble(&widthOk) : 0.0;
	const double metersPerPixel = m_info.meterPerPixelWidth();
	const double fullImageWidth = m_info.imageWidth * metersPerPixel;
	if (!widthOk || requestedWidth <= 0.0 || metersPerPixel <= 0.0 || requestedWidth > fullImageWidth + 1e-9)
	{
		QMessageBox::warning(this, QStringLiteral("\u8f93\u5165\u65e0\u6548"),
			QStringLiteral("\u9053\u8def\u5bbd\u5ea6\u5fc5\u987b\u5927\u4e8e 0\uff0c\u4e14\u4e0d\u80fd\u8d85\u8fc7\u6574\u5e45\u56fe\u50cf\u53ef\u8868\u793a\u7684 %1 m\u3002")
			.arg(fullImageWidth, 0, 'f', 5));
		return false;
	}
	int left = 0;
	int right = 0;
	if (!m_info.centeredBoundsForRoadWidth(requestedWidth, left, right))
	{
		QMessageBox::warning(this, QStringLiteral("\u8f93\u5165\u65e0\u6548"),
			QStringLiteral("\u8be5\u5bbd\u5ea6\u65e0\u6cd5\u6362\u7b97\u4e3a\u5408\u6cd5\u7684\u5c45\u4e2d\u50cf\u7d20\u8fb9\u754c\u3002"));
		return false;
	}
	m_previewView->setBounds(left, right);
	m_pendingInputMode = 0;
	updateSummary();
	return true;
}

bool hnLineCameraValidAreaDialog::applyPendingInput()
{
	if (m_pendingInputMode == 1) return applyPixelInputs();
	if (m_pendingInputMode == 2) return applyRoadWidthInput();
	return true;
}

void hnLineCameraValidAreaDialog::updateSummary()
{
	if (!m_summaryLabel || !m_previewView)
	{
		return;
	}
	const int left = m_previewView->leftPixel();
	const int right = m_previewView->rightPixel();
	const double width = (right - left) * m_info.meterPerPixelWidth();
	if (m_leftPixelEdit && !m_leftPixelEdit->hasFocus()) m_leftPixelEdit->setText(QString::number(left));
	if (m_rightPixelEdit && !m_rightPixelEdit->hasFocus()) m_rightPixelEdit->setText(QString::number(right));
	if (m_roadWidthEdit && !m_roadWidthEdit->hasFocus()) m_roadWidthEdit->setText(QString::number(width, 'f', 5));
	m_summaryLabel->setText(QStringLiteral("左边界：%1 px    右边界：%2 px    有效像素：%3 px    道路宽度：%4 m")
		.arg(left).arg(right).arg(right - left).arg(width, 0, 'f', 5));
}

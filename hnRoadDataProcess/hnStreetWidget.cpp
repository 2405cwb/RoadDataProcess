#include "hnStreetWidget.h"
#include "..\hnApplication\hnDataManager.h"
#include "..\hnApplication\hnApplication.h"
#include <QComboBox>
#include <QStatusBar>
#include <QSignalBlocker>
#include <QFileInfo>
#include <QEvent>
#include "../hnProject/hnProjectImageSettings.h"
using namespace hnApp;

hnStreetWidget::hnStreetWidget(QWidget *parent)
	: QMainWindow(parent), m_bDoubleStreet(true), showModelComBox(nullptr),caculateRoadWidthBtn(nullptr),
	m_brightnessSaveTimer(new QTimer(this)), m_pendingBrightnessValue(0)
{
	//ui.setupUi(this);
	m_rightPicNeedRotate = false;
	m_pStreetView = NULL;
	m_pStreetViewDouble = NULL;

	m_leftImagePanel = new QWidget(this);
	m_rightImagePanel = new QWidget(this);
	m_leftImgLabel = new QLabel(m_leftImagePanel);
	m_rightImgLabel = new QLabel(m_rightImagePanel);
	m_leftImgLabel->setObjectName(QStringLiteral("leftStreetImage"));
	m_rightImgLabel->setObjectName(QStringLiteral("rightStreetImage"));
	m_leftImageCaption = new QLabel(m_leftImagePanel);
	m_rightImageCaption = new QLabel(m_rightImagePanel);
	m_leftImagePanel->setObjectName(QStringLiteral("leftStreetImagePanel"));
	m_rightImagePanel->setObjectName(QStringLiteral("rightStreetImagePanel"));
	m_leftImageCaption->setObjectName(QStringLiteral("leftStreetImageCaption"));
	m_rightImageCaption->setObjectName(QStringLiteral("rightStreetImageCaption"));
	m_leftImgLabel->setScaledContents(true);
	m_rightImgLabel->setScaledContents(true);
	m_leftImgLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_rightImgLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_leftImgLabel->installEventFilter(this);
	m_rightImgLabel->installEventFilter(this);
	QLabel* captions[] = { m_leftImageCaption, m_rightImageCaption };
	QWidget* panels[] = { m_leftImagePanel, m_rightImagePanel };
	QLabel* pictures[] = { m_leftImgLabel, m_rightImgLabel };
	for (int i = 0; i < 2; ++i)
	{
		QVBoxLayout* column = new QVBoxLayout(panels[i]);
		column->setContentsMargins(0, 0, 0, 0);
		column->setSpacing(3);
		column->addWidget(pictures[i], 1);
		QFont smallFont = captions[i]->font();
		smallFont.setPointSize(qMax(8, smallFont.pointSize() - 1));
		captions[i]->setFont(smallFont);
		captions[i]->setTextFormat(Qt::PlainText);
		captions[i]->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		captions[i]->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
		captions[i]->setFixedHeight(captions[i]->fontMetrics().height() + 6);
		captions[i]->setMinimumWidth(0);
		QPalette captionPalette = captions[i]->palette();
		const bool darkBackground = qGray(captionPalette.color(QPalette::Window).rgb()) < 128;
		captionPalette.setColor(QPalette::WindowText,
			darkBackground ? QColor(190, 200, 210) : QColor(80, 88, 100));
		captions[i]->setPalette(captionPalette);
		column->addWidget(captions[i]);
	}
	m_mainLayout = new QHBoxLayout; 

	m_pStreetView = dynamic_cast<hnStreetCameraView*>(hnApplication::getApp()->newImageView(m_leftImgLabel, QString::fromLocal8Bit("景观图像"), VIEW_STREET_CAMERA_TYPE));

	// 绑定消息
	
	connect(this, &hnStreetWidget::signal_updateBrightness, m_pStreetView, &hnStreetCameraView::slot_updatePictureBrightness);
	 
	m_pStreetViewDouble = dynamic_cast<hnStreetCameraView*>(hnApplication::getApp()->newImageView(m_rightImgLabel, QString::fromLocal8Bit("景观图像"), VIEW_STREET_CAMERA_TYPE));
	 toolBar = addToolBar(QStringLiteral("工具栏")); 
	 toolBar->setMinimumHeight(30);
	 QLabel * brightLabel = new QLabel(QStringLiteral("亮度调节:"));
	 brightnessSlider = new QSlider(Qt::Horizontal, this);
	 brightnessSlider->setFixedWidth(150);
	 brightnessSlider->setRange(hnProjectImageSettings::MinBrightness, hnProjectImageSettings::MaxBrightness);
	 brightnessSlider->setValue(0);
	 m_brightnessSaveTimer->setSingleShot(true);
	 m_brightnessSaveTimer->setInterval(400);
	 connect(m_brightnessSaveTimer, &QTimer::timeout, this, &hnStreetWidget::savePendingBrightness);
	 connect(brightnessSlider, &QSlider::sliderReleased, this, &hnStreetWidget::savePendingBrightness);
	 brightnessSlider->setToolTip(QStringLiteral("向右提亮暗部，向左压暗；中间零位恢复原图，范围-100～300，超过100可增强隧道暗部；停止调节后保存到当前工程。"));
	 toolBar->addWidget(brightLabel);
	 toolBar->addWidget(brightnessSlider);
	  
	QWidget* centralWidget = new QWidget;
	centralWidget->setLayout(m_mainLayout);
	setCentralWidget(centralWidget);

	 


	// 绑定消息 

	

	connect(brightnessSlider, &QSlider::valueChanged, this, &hnStreetWidget::slot_updateBrightness);
	this->m_currentFrameIdx = 0;

	//connect(m_pStreetView, &hnStreetCameraView::signal_addDisease, this, &hnStreetWidget::signal_addDisease);
	//connect(m_pStreetView, &hnStreetCameraView::signal_deleteDisease, this, &hnStreetWidget::signal_deleteDisease);

	//connect(m_pStreetViewDouble, &hnStreetCameraView::signal_addDisease, this, //&hnStreetWidget::signal_addDisease);
	//connect(m_pStreetViewDouble, &hnStreetCameraView::signal_deleteDisease, this, &hnStreetWidget::signal_deleteDisease);
	connect(this, &hnStreetWidget::signal_updateBrightness, m_pStreetViewDouble, &hnStreetCameraView::slot_updatePictureBrightness);
	
}

hnStreetWidget::~hnStreetWidget()
{
	savePendingBrightness();
}

// 初始化视图
QString hnStreetWidget::currentStreetImagePath() const
{
    if (m_showModelIndex == 2 && m_pStreetViewDouble)
    {
        return m_pStreetViewDouble->currentImagePath();
    }

    if (m_pStreetView)
    {
        const QString leftPath = m_pStreetView->currentImagePath();
        if (!leftPath.isEmpty())
        {
            return leftPath;
        }
    }

    return m_pStreetViewDouble ? m_pStreetViewDouble->currentImagePath() : QString();
}

void hnStreetWidget::initView()
{
	connect(m_pStreetView, &hnStreetCameraView::updateShowImg, this, &hnStreetWidget::updateViewImage, Qt::UniqueConnection);
	connect(m_pStreetViewDouble, &hnStreetCameraView::updateShowImg, this, &hnStreetWidget::updateViewImage, Qt::UniqueConnection);
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		m_leftImageCaption->clear();
		m_rightImageCaption->clear();
		return;
	}
	auto  project2d = hnDataManager::getDataManager()->getCurrentProject()->get2DProject(); 
	double leftLength = project2d->_StreetImgDis;
	double rightLength = project2d->_StreeRightImgDis;
	int rightStreetCount = project2d->getRightStreetPicturePath().size();
	m_showModelIndex = 0; 
	if (showModelComBox)
	{
		QSignalBlocker blocker(showModelComBox);
		showModelComBox->setCurrentIndex(0);
		showModelComBox->setVisible(rightStreetCount > 0);
	}



	if (leftLength!=rightLength)
	{
		m_rightPicNeedRotate = true;
	}
	else
	{
		m_rightPicNeedRotate = false;
	}
	if (rightStreetCount >0)
	{
		if (showModelComBox == nullptr)
		{
			showModelComBox = new QComboBox;
			showModelComBox->addItems({ QStringLiteral("双侧景观"),QStringLiteral("左侧景观"),QStringLiteral("右侧景观") });
			showModelComBox->setCurrentIndex(0);
			//showModelComBox->setStyleSheet("QComboBox {font-size: 14px;}");
			
			QWidget *leftSpacer = new QWidget;
			leftSpacer->setFixedWidth(5);
			toolBar->addWidget(leftSpacer);
			toolBar->addWidget(showModelComBox);
			connect(showModelComBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
				m_showModelIndex = index;
				if (index == 0)
				{
					//m_pStreetViewDouble->setVisible(true);
					m_leftImagePanel->setVisible(true);
					m_rightImagePanel->setVisible(true);
					m_mainLayout->setStretch(0, 1);
					m_mainLayout->setStretch(1, 1);

				}
				else if (index == 1)
				{
					//	m_pStreetViewDouble->setVisible(false);	
					m_leftImagePanel->setVisible(true);
					m_rightImagePanel->setVisible(false);
					m_mainLayout->setStretch(0, 1);
					m_mainLayout->setStretch(1, 0); 
				}
				else
				{
					//	m_pStreetView->setVisible(false);
					m_rightImagePanel->setVisible(true);
					m_leftImagePanel->setVisible(false);
					m_mainLayout->setStretch(0, 0);
					m_mainLayout->setStretch(1, 1);
					
				}
				/*QApplication::processEvents();
				centralWidget()->adjustSize();*/
				resize(size() + QSize(1, 1));
				resize(size() - QSize(1, 1));
			});

		}
		
   } 
	if (caculateRoadWidthBtn == nullptr)
	{
		caculateRoadWidthBtn = new QRadioButton(QStringLiteral("测量道路宽度"));
		QWidget *leftSpacer = new QWidget;
		leftSpacer->setFixedWidth(5);
		toolBar->addWidget(leftSpacer);
		 toolBar->addWidget(caculateRoadWidthBtn);
		 connect(caculateRoadWidthBtn, &QRadioButton::clicked, this, [&](bool clicked)
		 {
			 if (clicked)
			 {
				 //检测是否存在标定文件 
				 QString project2dBasePath = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getBasePath();
				 QString u_jgPath = project2dBasePath + "\\u_jg.bin";
				 QString v_jgPath = project2dBasePath + "\\v_jg.bin";

				 QFile file;
				 if (file.exists(u_jgPath) && file.exists(v_jgPath))
				 {
					 //获得标定数组

				 }
				 else
				 {
					 QMessageBox::information(this, QString::fromLocal8Bit("温馨提示"), QStringLiteral("该工程下不能存在标定文件，无法使用此功能！"));

					 caculateRoadWidthBtn->setChecked(false);
					 return;
				 } 
			 }
			 m_pStreetView->setCalculateRoadWidthMode(clicked);
			  if (m_pStreetViewDouble !=nullptr)
			  {
				  m_pStreetViewDouble->setCalculateRoadWidthMode(clicked);
			  }
		 });
	}
	m_mainLayout->removeWidget(m_leftImagePanel);
	m_mainLayout->removeWidget(m_rightImagePanel);
	const int leftPictureSize = project2d->getLeftStreetPicturePath().size();
	const int rightPictureSize = project2d->getRightStreetPicturePath().size();
	if (leftPictureSize > 0)
	{
		m_mainLayout->addWidget(m_leftImagePanel, 1);
	}
	if (rightPictureSize > 0)
	{
		m_mainLayout->addWidget(m_rightImagePanel, 1);
	}
	m_leftImagePanel->setVisible(leftPictureSize > 0 && m_showModelIndex != 2);
	m_rightImagePanel->setVisible(rightPictureSize > 0 && m_showModelIndex != 1);

	// 重新加载数据
	reloadData();
	updateImageCaptions();

	//更新景观图片大小，不用定时器的话不会更新
	QTimer::singleShot(100, [this]() {
		this->resizeEvent(nullptr);
	});

}

void hnStreetWidget::clearPix()
{
	savePendingBrightness();
	if (m_pStreetView)
	{
		m_pStreetView->clear();
	}

	if (m_pStreetViewDouble)
	{
		m_pStreetViewDouble->clear();
	}
	updateImageCaptions();
}


void hnStreetWidget::resizeEvent(QResizeEvent * event)
{
	if (m_pStreetView)
	{
		m_pStreetView->resize(m_leftImgLabel->size());
	}

	if (m_pStreetViewDouble)
	{
		m_pStreetViewDouble->resize(m_rightImgLabel->size());
	}
	QTimer::singleShot(0, this, &hnStreetWidget::updateImageCaptions);
}

bool hnStreetWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Resize)
    {
        if (watched == m_leftImgLabel && m_pStreetView)
        {
            m_pStreetView->resize(m_leftImgLabel->size());
        }
        else if (watched == m_rightImgLabel && m_pStreetViewDouble)
        {
            m_pStreetViewDouble->resize(m_rightImgLabel->size());
        }
        QTimer::singleShot(0, this, &hnStreetWidget::updateImageCaptions);
    }
    return QMainWindow::eventFilter(watched, event);
}

void hnStreetWidget::updateImageCaption(hnStreetCameraView* view, QLabel* caption)
{
    if (!view || !caption) return;
    QString imagePath;
    double trueMile = 0.0;
    if (!view->currentDisplayedImageInfo(&imagePath, &trueMile) || !qIsFinite(trueMile))
    {
        caption->clear();
        caption->setToolTip(QString());
        return;
    }

    // 使用当前图片的里程映射，避免页面滚动位置与左右图片的桩号不一致。
    const qint64 centiMile = qRound64(trueMile * 100.0);
    const qint64 absolute = qAbs(centiMile);
    const qint64 kilometer = absolute / 100000;
    const qint64 meterCenti = absolute % 100000;
    const QString stake = QStringLiteral("%1K%2+%3.%4")
        .arg(centiMile < 0 ? QStringLiteral("-") : QString())
        .arg(kilometer)
        .arg(meterCenti / 100, 3, 10, QChar('0'))
        .arg(meterCenti % 100, 2, 10, QChar('0'));
    const QString prefix = stake + QStringLiteral("  ·  ");
    const QString fileName = QFileInfo(imagePath).fileName();
    const int available = caption->width() - caption->fontMetrics().width(prefix) - 4;
    caption->setText(available > 0
        ? prefix + caption->fontMetrics().elidedText(fileName, Qt::ElideMiddle, available)
        : caption->fontMetrics().elidedText(prefix + fileName, Qt::ElideRight, qMax(0, caption->width() - 4)));
    caption->setToolTip(QStringLiteral("桩号：%1\n图片：%2").arg(stake, imagePath));
}

void hnStreetWidget::updateImageCaptions()
{
    updateImageCaption(m_pStreetView, m_leftImageCaption);
    updateImageCaption(m_pStreetViewDouble, m_rightImageCaption);
}

void hnStreetWidget::enterEvent(QEvent * event)
{
	emit signal_enterWidget();
}

// 重新加载数据
void hnStreetWidget::reloadData()
{ 
	auto project2d = hnDataManager::getDataManager()->getCurrentProject()->get2DProject();
	// 先将旧工程的待保存值写回原路径，再加载新工程；不触发滑条写回。
	savePendingBrightness();
	const hnProjectImageSettings settings;
	const int brightness = settings.streetBrightness(project2d->getBasePath());
	{
		QSignalBlocker blocker(brightnessSlider);
		brightnessSlider->setValue(brightness);
	}
	emit signal_updateBrightness(brightness);
	m_currentFrameIdx = -1.0;
	project2d->leftStreetImgIndex = 0;
	project2d->rightStreetImgIndex = 0;

double dis = project2d->_StreetImgDis;
double streetDis = project2d->_StreeRightImgDis;

int rightStreetCount = project2d->getRightStreetPicturePath().size();
int leftStreetCount = project2d->getLeftStreetPicturePath().size();
	if (m_pStreetView&&leftStreetCount >0)
	{
		m_pStreetView->setStreetInterval(dis);
		m_pStreetView->reloadData(false, STREET_LEFT_VIEW);
	}

	if (m_pStreetViewDouble&&rightStreetCount >0)
	{
		m_pStreetViewDouble->setStreetInterval(streetDis);
		m_pStreetViewDouble->reloadData(m_rightPicNeedRotate, STREET_RIGHT_VIEW);
	}
	updateImageCaptions();
}

// 更新图像
void hnStreetWidget::updateViewImage(double nIndex)
{
	auto project2d = hnDataManager::getDataManager()->getCurrentProject()->get2DProject();
	//如果要更新的帧序号等于当前的帧序号，就不更新了，退出
	if (nIndex == this->m_currentFrameIdx)
	{
		return;
	}
	if (nIndex < 0)
	{
		if (m_pStreetView)
		{
			m_pStreetView->resize(m_leftImgLabel->size());
		}

		if (m_pStreetViewDouble)
		{
			m_pStreetViewDouble->resize(m_rightImgLabel->size());
		}
	}
	int rightStreetCount = project2d->getRightStreetPicturePath().size();
	int leftStreetCount = project2d->getLeftStreetPicturePath().size();
	if (m_pStreetView&&leftStreetCount)
	{
		m_pStreetView->addImage(false, nIndex);

	}

	if (m_pStreetViewDouble&&rightStreetCount)
	{
		m_pStreetViewDouble->addImage(m_rightPicNeedRotate, nIndex);
	}
	updateImageCaptions();
	 
	this->m_currentFrameIdx = nIndex;

	emit signal_imageIdxChanged(nIndex);
}

 

void hnStreetWidget::slot_updateBrightness(int value)
{
	//更新亮度
	emit signal_updateBrightness(value);
	const auto manager = hnDataManager::getDataManager();
	if (!manager->isOpenProject() || !manager->getCurrentProject()->get2DProject())
	{
		return;
	}
	const QString projectPath = manager->getCurrentProject()->get2DProject()->getBasePath();
	if (!m_pendingBrightnessProjectPath.isEmpty() && m_pendingBrightnessProjectPath != projectPath)
	{
		savePendingBrightness();
	}
	m_pendingBrightnessProjectPath = projectPath;
	m_pendingBrightnessValue = value;
	m_brightnessSaveTimer->start();
}

void hnStreetWidget::updateCurViewImage()
{
	double nIndex = this->m_currentFrameIdx;
	if (nIndex < 0)
	{
		if (m_pStreetView)
		{
			m_pStreetView->resize(m_leftImgLabel->size());
		}

		if (m_pStreetViewDouble)
		{
			m_pStreetViewDouble->resize(m_rightImgLabel->size());
		}
	}

	if (m_pStreetView)
	{
		m_pStreetView->addImage(false,nIndex);
	}

	if (m_pStreetViewDouble)
	{
		m_pStreetViewDouble->addImage(m_rightPicNeedRotate,nIndex);
	}
	updateImageCaptions();

	this->m_currentFrameIdx = nIndex;

	emit signal_imageIdxChanged(nIndex);
}


void hnStreetWidget::savePendingBrightness()
{
	m_brightnessSaveTimer->stop();
	if (m_pendingBrightnessProjectPath.isEmpty())
	{
		return;
	}
	const QString path = m_pendingBrightnessProjectPath;
	m_pendingBrightnessProjectPath.clear();
	const hnProjectImageSettings settings;
	if (!settings.saveStreetBrightness(path, m_pendingBrightnessValue))
	{
		QMessageBox::warning(this, QStringLiteral("景观亮度保存失败"),
			QStringLiteral("无法保存工程景观亮度：%1\n请检查文件写入权限和磁盘空间。").arg(settings.filePath(path)));
	}
}

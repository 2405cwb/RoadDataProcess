#include "hnStreetWidget.h"
#include "..\hnApplication\hnDataManager.h"
#include "..\hnApplication\hnApplication.h"
#include <QComboBox>
#include <QStatusBar>
using namespace hnApp;

hnStreetWidget::hnStreetWidget(QWidget *parent)
	: QMainWindow(parent), m_bDoubleStreet(true), showModelComBox(nullptr),caculateRoadWidthBtn(nullptr)
{
	//ui.setupUi(this);
	m_rightPicNeedRotate = false;
	m_pStreetView = NULL;
	m_pStreetViewDouble = NULL;

	m_leftImgLabel = new QLabel;
	m_rightImgLabel = new QLabel;
	m_leftImgLabel->setScaledContents(true);
	m_rightImgLabel->setScaledContents(true);
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
	 brightnessSlider->setRange(-100, 100);
	 brightnessSlider->setValue(0);
	 toolBar->addWidget(brightLabel);
	 toolBar->addWidget(brightnessSlider);
	  
	QWidget* centralWidget = new QWidget;
	centralWidget->setLayout(m_mainLayout);
	setCentralWidget(centralWidget);

	 


	// 绑定消息 

	

	connect(brightnessSlider, &QSlider::valueChanged, this, &hnStreetWidget::slot_updateBrightness);
	this->m_currentFrameIdx = 0;

	connect(m_pStreetView, &hnStreetCameraView::signal_addDisease, this, &hnStreetWidget::signal_addDisease);
	connect(m_pStreetView, &hnStreetCameraView::signal_deleteDisease, this, &hnStreetWidget::signal_deleteDisease);

	connect(m_pStreetViewDouble, &hnStreetCameraView::signal_addDisease, this, &hnStreetWidget::signal_addDisease);
	connect(m_pStreetViewDouble, &hnStreetCameraView::signal_deleteDisease, this, &hnStreetWidget::signal_deleteDisease);
	connect(this, &hnStreetWidget::signal_updateBrightness, m_pStreetViewDouble, &hnStreetCameraView::slot_updatePictureBrightness);
	
}

hnStreetWidget::~hnStreetWidget()
{
}

// 初始化视图
void hnStreetWidget::initView()
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	 
	double leftLength = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->_StreetImgDis;
	double rightLength = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->_StreeRightImgDis;
	int rightStreetCount = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getRightStreetPicturePath().size();
	m_showModelIndex = 0; 
	if (leftLength == rightLength)
	{

		connect(m_pStreetView, SIGNAL(updateShowImg(int)), this, SLOT(updateViewImage(int)));
		 connect(m_pStreetViewDouble, SIGNAL(updateShowImg(int)), this, SLOT(updateViewImage(int))); 
	 
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
					m_leftImgLabel->setVisible(true);
					m_rightImgLabel->setVisible(true);
					m_mainLayout->setStretch(0, 1);
					m_mainLayout->setStretch(1, 1);
					if (leftLength == rightLength)
					{
						connect(m_pStreetView, SIGNAL(updateShowImg(int)), this, SLOT(updateViewImage(int)));
						connect(m_pStreetViewDouble, SIGNAL(updateShowImg(int)), this, SLOT(updateViewImage(int)));

					}

				}
				else if (index == 1)
				{
					//	m_pStreetViewDouble->setVisible(false);	
					m_leftImgLabel->setVisible(true);
					m_rightImgLabel->setVisible(false);
					m_mainLayout->setStretch(0, 1);
					m_mainLayout->setStretch(1, 0); 
					connect(m_pStreetView, SIGNAL(updateShowImg(int)), this, SLOT(updateViewImage(int)));
					connect(m_pStreetViewDouble, SIGNAL(updateShowImg(int)), this, SLOT(updateViewImage(int))); 
				}
				else
				{
					//	m_pStreetView->setVisible(false);
					m_rightImgLabel->setVisible(true);
					m_leftImgLabel->setVisible(false);
					m_mainLayout->setStretch(0, 0);
					m_mainLayout->setStretch(1, 1);
					connect(m_pStreetView, SIGNAL(updateShowImg(int)), this, SLOT(updateViewImage(int)));
					connect(m_pStreetViewDouble, SIGNAL(updateShowImg(int)), this, SLOT(updateViewImage(int))); 
					
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
	m_mainLayout->removeWidget(m_leftImgLabel);
	m_mainLayout->removeWidget(m_rightImgLabel);

	QWidget * statusWidget = new QWidget(this);

	QHBoxLayout *vLayout = new QHBoxLayout(statusWidget);
	vLayout->setContentsMargins(8, 2, 8, 2);
	vLayout->setSpacing(10);

	// leftPictureNameLabel = new QLabel(QStringLiteral("左侧路径"), statusWidget);
	// rightPcitureNameLabel = new QLabel(QStringLiteral("右侧路径"), statusWidget); 
	//leftPictureNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	//rightPcitureNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	//leftPictureNameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	//rightPcitureNameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	int leftPictureSize = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getLeftStreetPicturePath().size();
	int rightPictureSize = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getRightStreetPicturePath().size();
	if (leftPictureSize> 0)
	{ 
		m_mainLayout->addWidget(m_leftImgLabel);
	//	vLayout->addWidget(leftPictureNameLabel,1);
	}

	if (rightPictureSize > 0)
	{
		m_mainLayout->addWidget(m_rightImgLabel);
		//vLayout->addWidget(rightPcitureNameLabel,1);
	}
	//statusBar()->addWidget(statusWidget,1);

	// 重新加载数据
	reloadData();

	//更新景观图片大小，不用定时器的话不会更新
	QTimer::singleShot(100, [this]() {
		this->resizeEvent(nullptr);
	});

}

void hnStreetWidget::clearPix()
{
	if (m_pStreetView)
	{
		m_pStreetView->clear();
	}

	if (m_pStreetViewDouble)
	{
		m_pStreetViewDouble->clear();
	}
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
}

void hnStreetWidget::enterEvent(QEvent * event)
{
	emit signal_enterWidget();
}

// 重新加载数据
void hnStreetWidget::reloadData()
{ 
	hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->leftStreetImgIndex = 0;
	hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->rightStreetImgIndex = 0;

int dis=	hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->_StreetImgDis;
int streetDis = 	hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->_StreeRightImgDis;

int rightStreetCount = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getRightStreetPicturePath().size();
int leftStreetCount = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getLeftStreetPicturePath().size();
	if (m_pStreetView&&leftStreetCount >0)
	{
		m_pStreetView->reloadData(m_rightPicNeedRotate,STREET_LEFT_VIEW);
		m_pStreetView->setStreetInterval(dis);
	}

	if (m_pStreetViewDouble&&rightStreetCount >0)
	{
		m_pStreetViewDouble->reloadData(m_rightPicNeedRotate,STREET_RIGHT_VIEW);
		m_pStreetViewDouble->setStreetInterval(streetDis);
	}
}

// 更新图像
void hnStreetWidget::updateViewImage(int nIndex)
{
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
	int rightStreetCount = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getRightStreetPicturePath().size();
	int leftStreetCount = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getLeftStreetPicturePath().size();
	if (m_pStreetView&&leftStreetCount)
	{
		m_pStreetView->addImage(false, nIndex);
		hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->leftStreetImgIndex--;

	}

	if (m_pStreetViewDouble&&rightStreetCount)
	{
		hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->rightStreetImgIndex--;
		m_pStreetViewDouble->addImage(m_rightPicNeedRotate, nIndex);
	}
	 
	this->m_currentFrameIdx = nIndex;

	emit signal_imageIdxChanged(nIndex);
}

 

void hnStreetWidget::slot_updateBrightness(int value)
{
	//更新亮度
	emit signal_updateBrightness(value);
}

void hnStreetWidget::updateCurViewImage()
{
	int nIndex = this->m_currentFrameIdx; 
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

	this->m_currentFrameIdx = nIndex;

	emit signal_imageIdxChanged(nIndex);
}


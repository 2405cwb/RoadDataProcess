#pragma region 文件说明
/*! @hnContinuouslyBrowsePixWidget.cpp
********************************************************************************
<PRE>
模块名       : hnApp
文件名       : hnIpsShowPixWidget.cpp
相关文件     : hnIpsShowPixWidget.h
文件实现功能 : 显示图片的窗口
作者         : 陈智超
版本         : 1.0.0
--------------------------------------------------------------------------------
备注         :
--------------------------------------------------------------------------------
修改记录 :
日期        版本     修改人              修改内容
2022/10/12	1.0.0	 陈智超				 创建初版
</PRE>
*******************************************************************************/

#pragma endregion



#include "hnContinuouslyBrowsePixWidget.h"
#include <QTimer>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
hnContinuouslyBrowsePixWidget::hnContinuouslyBrowsePixWidget(QWidget *parent)
	: QWidget(parent)
{
	

	xrSetting = HnXRSettings::getInstance();
}

hnContinuouslyBrowsePixWidget::hnContinuouslyBrowsePixWidget(hnBrowsePixWidget *showPixWidget, QWidget *parent)
	: QWidget(parent)
{

	 
	xrSetting = HnXRSettings::getInstance();

	//接受传入的
	this->m_browsePixWidget = showPixWidget;
	
	//初始化
	this->init();

	//qdockwidget添加这个界面之后，这个参数就不管用了
	this->setMouseTracking(true);
}

hnContinuouslyBrowsePixWidget::~hnContinuouslyBrowsePixWidget()
{
}

void hnContinuouslyBrowsePixWidget::setBrowsePixWidget(hnBrowsePixWidget * browsePixWidget,bool show2dToolBar)
{
	//接受传入的
	this->m_browsePixWidget = browsePixWidget;

	
	//初始化
	this->init();

	if (show2dToolBar)
	{
		add2dToolbar(); 
	}
	else
	{
		add3dToolbar();
		//this->setLayout(m_mainLayout);
	}


	//qdockwidget添加这个界面之后，这个参数就不管用了
	this->setMouseTracking(true);
}




void hnContinuouslyBrowsePixWidget::init()
{
	this->setWindowTitle("图像显示");

	// 自动播放固定为每秒前进一张图片，不再使用旧的速度加减逻辑。
	this->m_autoPlay = false;
	if (!m_autoPlayTimer)
	{
		m_autoPlayTimer = new QTimer(this);
		m_autoPlayTimer->setInterval(1000);
		connect(m_autoPlayTimer, &QTimer::timeout, this, [this]()
		{
			if (!m_autoPlay || !stepOneImage())
			{
				stopAutoPlay();
			}
		});
	}
	

	this->m_mainLayout = new QHBoxLayout();
	this->m_mainLayout->addWidget(this->m_browsePixWidget);




	//设置默认鼠标追踪
	this->setMouseTracking(true);

	//初始化信号槽
	this->initSigSlot();
}

//路面图像上方工具栏待完成
void hnContinuouslyBrowsePixWidget::add2dToolbar()
{
	QVBoxLayout *layoutUpDown = new QVBoxLayout();
	QToolBar *tooBar = new QToolBar();

	QHBoxLayout * toolBarLayout = new QHBoxLayout();
	toolBarLayout->setContentsMargins(0, 0, 0, 0);
	toolBarLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
	toolBarLayout->setSpacing(5);
	toolBarLayout->setAlignment(Qt::AlignLeft);

	auto mileLable = new QLabel(QStringLiteral("桩号"));
	mileLable->setMaximumWidth(25);
    mileBlock = new QLineEdit("0"); 
	mileBlock->setFixedWidth(70);


	auto dmiLable = new QLabel(QStringLiteral("里程"));
	dmiLable->setMaximumWidth(25);
	 dmiBlock = new QLineEdit("0");
	dmiBlock->setFixedWidth(70);
	dmiBlock->setReadOnly(true);

	jumpBtn = new QPushButton(QStringLiteral("跳转")); 
	jumpBtn->setFixedWidth(50);
	QPushButton*  lqBtn = new QPushButton(QStringLiteral("沥青"));
	lqBtn->setFixedWidth(50);
	 
	//lqBtn->setStyleSheet("background-color: green;color:white;");
	QPushButton*  snBtn = new QPushButton(QStringLiteral("水泥"));
	snBtn->setFixedWidth(50);

	/*QPushButton*  ssBtnTest = new QPushButton(QStringLiteral("??"));
	ssBtnTest ->setFixedSize(20,20);
	QString sheet = QString("QPushButton{border - radius:25px;color:green;background - position:center;background - repeat:no - repeat;background - color:red;}");
	ssBtnTest->setStyleSheet(sheet);*/
	 
	markBtn = new QPushButton;
	if (xrSetting->diseaseMark)
	{
		markBtn->setText(QStringLiteral("关闭备注"));

	}
	else
	{
		markBtn->setText(QStringLiteral("开启备注"));

	}
	markBtn->setMaximumWidth(80);

	showGpsBtn = new QCheckBox(QStringLiteral("显示高精度定位"));
	showGpsBtn->setMaximumWidth(110);
	showGpsBtn->setChecked(xrSetting->showGpsInfo);
	showGpsBtn->setToolTip(QStringLiteral("仅支持具备高精度定位模块的设备数据!"));



	//diseaseRectShowBtn = new QPushButton;
	//if (xrSetting->diseaseRectShow)
	//{
	//	diseaseRectShowBtn->setText(QStringLiteral("关闭矩形框"));
	//}
	//else
	//{
	//	diseaseRectShowBtn->setText(QStringLiteral("开启矩形框"));
	//}
	//diseaseRectShowBtn->setMaximumWidth(80);


	toolBarLayout->addWidget(mileLable);
	toolBarLayout->addWidget(mileBlock);
	toolBarLayout->addWidget(dmiLable);
	toolBarLayout->addWidget(dmiBlock);

	
	toolBarLayout->addWidget(jumpBtn);

	addAutoPlayControl(toolBarLayout);
	toolBarLayout->addWidget(markBtn);
	toolBarLayout->addWidget(showGpsBtn);

	auto brightnessLabel = new QLabel(QString::fromUtf8("\xE4\xBA\xAE\xE5\xBA\xA6"));
	auto brightnessSlider = new QSlider(Qt::Horizontal);
	brightnessSlider->setRange(-100, 100);
	brightnessSlider->setValue(0);
	brightnessSlider->setFixedWidth(120);
	brightnessSlider->setToolTip(QString::fromUtf8("\xE8\xB0\x83\xE6\x95\xB4\xE5\xBD\x93\xE5\x89\x8D\xE8\xB7\xAF\xE9\x9D\xA2\xE5\xBD\xB1\xE5\x83\x8F\xE4\xBA\xAE\xE5\xBA\xA6\xEF\xBC\x8C\xE5\x8F\x8C\xE5\x87\xBB\xE5\x8F\xAF\xE6\x81\xA2\xE5\xA4\x8D\xE9\xBB\x98\xE8\xAE\xA4"));
	auto brightnessValueLabel = new QLabel(QStringLiteral("0"));
	brightnessValueLabel->setFixedWidth(28);
	auto brightnessResetBtn = new QPushButton(QString::fromUtf8("\xE5\xA4\x8D\xE4\xBD\x8D"));
	brightnessResetBtn->setFixedWidth(42);
	toolBarLayout->addSpacing(8);
	toolBarLayout->addWidget(brightnessLabel);
	toolBarLayout->addWidget(brightnessSlider);
	toolBarLayout->addWidget(brightnessValueLabel);
	toolBarLayout->addWidget(brightnessResetBtn);

//	toolBarLayout->addWidget(diseaseRectShowBtn);

	toolBarLayout->addSpacerItem(new QSpacerItem(20, 10, QSizePolicy::Fixed, QSizePolicy::Minimum));


	
  
	layoutUpDown->addLayout(toolBarLayout);
	layoutUpDown->addLayout(this->m_mainLayout);
	this->setLayout(layoutUpDown);

	connect(brightnessSlider, &QSlider::valueChanged, this,
		[this, brightnessValueLabel](int value)
	{
		brightnessValueLabel->setText(QString::number(value));
		emit signal_imageBrightnessChanged(value);
	});
	connect(brightnessResetBtn, &QPushButton::clicked, brightnessSlider, [brightnessSlider]()
	{
		brightnessSlider->setValue(0);
	});

	connect(this->showGpsBtn, &QCheckBox::stateChanged, this, [&](int state)
	{
		if (state == Qt::Checked)
		{
			xrSetting->showGpsInfo = true;
		}
		else
		{
			xrSetting->showGpsInfo = false;
		}
		xrSetting->writeData();
	});

	connect(jumpBtn, &QPushButton::clicked, this, [this]()
	{
		stopAutoPlay();
		slot_JumpToUserMile();
	});


	connect(this->markBtn, &QPushButton::clicked, [&]() {
	
		if (xrSetting->diseaseMark)
		{
			markBtn->setText(QStringLiteral("开启备注"));
			xrSetting->diseaseMark = false;
		}
		else
		{
			markBtn->setText(QStringLiteral("关闭备注"));
			xrSetting->diseaseMark = true;
		}
		 
	}
	);

	//connect(this->diseaseRectShowBtn, &QPushButton::clicked, [&]() {

	//	if (xrSetting->diseaseRectShow)
	//	{
	//		diseaseRectShowBtn->setText(QStringLiteral("开启矩形框"));
	//		xrSetting->diseaseRectShow = false;
	//	}
	//	else
	//	{
	//		diseaseRectShowBtn->setText(QStringLiteral("关闭矩形框"));
	//		xrSetting->diseaseRectShow = true;
	//	}
	//	xrSetting->writeData();
	//}
	//);
}

void hnContinuouslyBrowsePixWidget::add3dToolbar()
{
	QVBoxLayout *layoutUpDown = new QVBoxLayout();
	QToolBar *tooBar = new QToolBar();

	QHBoxLayout * toolBarLayout = new QHBoxLayout();
	toolBarLayout->setContentsMargins(0, 0, 0, 0);
	toolBarLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
	toolBarLayout->setSpacing(5);
	toolBarLayout->setAlignment(Qt::AlignLeft);
	 
	deepExampleBtn = new QPushButton(QStringLiteral("深度示例"));  
	//deepExampleBtn->setFixedWidth(30); 
	toolBarLayout->addWidget(deepExampleBtn);
	addAutoPlayControl(toolBarLayout);
	auto brightnessLabel = new QLabel(QString::fromUtf8("\xE4\xBA\xAE\xE5\xBA\xA6"));
	auto brightnessSlider = new QSlider(Qt::Horizontal);
	brightnessSlider->setRange(-100, 100);
	brightnessSlider->setValue(0);
	brightnessSlider->setFixedWidth(120);
	auto brightnessValueLabel = new QLabel(QStringLiteral("0"));
	brightnessValueLabel->setFixedWidth(28);
	auto brightnessResetBtn = new QPushButton(QString::fromUtf8("\xE5\xA4\x8D\xE4\xBD\x8D"));
	brightnessResetBtn->setFixedWidth(42);
	toolBarLayout->addSpacing(8);
	toolBarLayout->addWidget(brightnessLabel);
	toolBarLayout->addWidget(brightnessSlider);
	toolBarLayout->addWidget(brightnessValueLabel);
	toolBarLayout->addWidget(brightnessResetBtn);
	toolBarLayout->addSpacerItem(new QSpacerItem(20, 10, QSizePolicy::Fixed, QSizePolicy::Minimum));




	layoutUpDown->addLayout(toolBarLayout);
	layoutUpDown->addLayout(this->m_mainLayout);
	this->setLayout(layoutUpDown); 
	 
	connect(deepExampleBtn, &QPushButton::clicked, this, &hnContinuouslyBrowsePixWidget::slot_Show3dDeepExample);
	connect(brightnessSlider, &QSlider::valueChanged, this,
		[this, brightnessValueLabel](int value)
	{
		brightnessValueLabel->setText(QString::number(value));
		emit signal_imageBrightnessChanged(value);
	});
	connect(brightnessResetBtn, &QPushButton::clicked, brightnessSlider, [brightnessSlider]()
	{
		brightnessSlider->setValue(0);
	});

}

void hnContinuouslyBrowsePixWidget::initSigSlot()
{
	// SDK now owns image browsing. Old scrollbar signals are intentionally unused.
}

hnBrowsePixWidget * hnContinuouslyBrowsePixWidget::getShowPixLabel()
{
	return this->m_browsePixWidget;
}

void hnContinuouslyBrowsePixWidget::slot_setSelectedDiseaseId(int id)
{
	m_browsePixWidget->setSelectedDiseaseId(id);
}

void hnContinuouslyBrowsePixWidget::slot_updateBrowser()
{
	QTimer::singleShot(500, [this]() {
		m_browsePixWidget->update();
	}
	);

}

 
void hnContinuouslyBrowsePixWidget::slot_updateDmiLable(int value)
{
	this->dmiBlock->setText(QString::number( value));
}

void hnContinuouslyBrowsePixWidget::loadPix(const QString & pixDirName)
{
	stopAutoPlay();
	this->m_browsePixWidget->loadPix(pixDirName);
}

void hnContinuouslyBrowsePixWidget::loadPix(const QStringList & pixNames)
{
	stopAutoPlay();
	this->m_browsePixWidget->loadPix(pixNames);
}

void hnContinuouslyBrowsePixWidget::wheelEvent(QWheelEvent * event)
{
	stopAutoPlay();
	QWidget::wheelEvent(event);
}

void hnContinuouslyBrowsePixWidget::keyPressEvent(QKeyEvent *event)
{
	stopAutoPlay();
	QWidget::keyPressEvent(event);

}

void hnContinuouslyBrowsePixWidget::enterEvent(QEvent * event)
{
	emit signal_enterWidget();
}

void hnContinuouslyBrowsePixWidget::addAutoPlayControl(QHBoxLayout* layout)
{
	playBtn = new QPushButton(QStringLiteral("自动播放"), this);
	playBtn->setFixedWidth(72);
	playBtn->setToolTip(QStringLiteral("每秒自动前进一张图片"));
	layout->addWidget(playBtn);
	connect(playBtn, &QPushButton::clicked, this, &hnContinuouslyBrowsePixWidget::toggleAutoPlay);
}

void hnContinuouslyBrowsePixWidget::toggleAutoPlay()
{
	if (m_autoPlay)
	{
		stopAutoPlay();
		return;
	}
	if (!stepOneImage())
	{
		stopAutoPlay();
		return;
	}
	m_autoPlay = true;
	updateAutoPlayButton();
	if (m_autoPlayTimer)
	{
		m_autoPlayTimer->start();
	}
}

void hnContinuouslyBrowsePixWidget::stopAutoPlay()
{
	m_autoPlay = false;
	if (m_autoPlayTimer)
	{
		m_autoPlayTimer->stop();
	}
	updateAutoPlayButton();
}

void hnContinuouslyBrowsePixWidget::updateAutoPlayButton()
{
	if (playBtn)
	{
		playBtn->setText(m_autoPlay ? QStringLiteral("停止播放") : QStringLiteral("自动播放"));
	}
}

int hnContinuouslyBrowsePixWidget::getBrowStep(bool is3d) const
{
	Q_UNUSED(is3d);
	return 1;
}


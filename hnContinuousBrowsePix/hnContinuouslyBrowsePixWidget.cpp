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
#include <QElapsedTimer>
hnContinuouslyBrowsePixWidget::hnContinuouslyBrowsePixWidget(QWidget *parent)
	: QWidget(parent)
{
	
	this->m_playSpeed = 2; 
	xrSetting = HnXRSettings::getInstance();
}

hnContinuouslyBrowsePixWidget::hnContinuouslyBrowsePixWidget(hnBrowsePixWidget *showPixWidget, QWidget *parent)
	: QWidget(parent)
{

	 
	xrSetting = HnXRSettings::getInstance();
	this->m_playSpeed = 2;
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

int hnContinuouslyBrowsePixWidget::getCurrentScrollBarValue()
{
	return this->m_scrollbar->value();
}

void hnContinuouslyBrowsePixWidget::setCurrentScrollBarValue(const int value)
{
	if (value != this->m_scrollbar->value())
	{
		this->m_scrollbar->setValue(value);		
	}	
}

int hnContinuouslyBrowsePixWidget::getMaxScrollBarValue()
{
	return this->m_scrollbar->maximum();
}



void hnContinuouslyBrowsePixWidget::init()
{
	this->setWindowTitle("图像显示");

	m_scrollbar = new CustomScrollBar;
	//设置滚动条为最大值  使图片显示在最底部
	this->m_scrollbar->setValue(100);
	// 自动播放标志
	this->m_autoPlay = false;
	

	this->m_mainLayout = new QHBoxLayout();
	this->m_mainLayout->addWidget(this->m_browsePixWidget);
	this->m_mainLayout->addWidget(this->m_scrollbar);




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

	//playBtn = new QPushButton(QStringLiteral("播放"));
	playBtn = new QPushButton;
	QIcon m_icon_temp = QIcon::fromTheme(QStringLiteral(""),
		  QIcon(QStringLiteral(":/new/prefix1/Icon/Media_16x16.png")));
	  playBtn->setIcon(m_icon_temp);
	playBtn->setFixedWidth(30);

	addSpeedBtn = new QPushButton;
	m_icon_temp = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/new/prefix1/Icon/Add_16x16.png")));
	addSpeedBtn->setIcon(m_icon_temp);
	addSpeedBtn->setFixedWidth(30);
	 

	subBtn = new QPushButton;
	m_icon_temp = QIcon::fromTheme(QStringLiteral(""),
		QIcon(QStringLiteral(":/new/prefix1/Icon/Remove_16x16.png")));
	subBtn->setIcon(m_icon_temp);
	subBtn->setFixedWidth(30);
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

	toolBarLayout->addWidget(playBtn);
	toolBarLayout->addWidget(addSpeedBtn);
	toolBarLayout->addWidget(subBtn);
	toolBarLayout->addWidget(markBtn);
	toolBarLayout->addWidget(showGpsBtn);

	addWheelScrollStepOption(toolBarLayout);
//	toolBarLayout->addWidget(diseaseRectShowBtn);

	toolBarLayout->addSpacerItem(new QSpacerItem(20, 10, QSizePolicy::Fixed, QSizePolicy::Minimum));


	
  
	layoutUpDown->addLayout(toolBarLayout);
	layoutUpDown->addLayout(this->m_mainLayout);
	this->setLayout(layoutUpDown);

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

	connect(this->playBtn, &QPushButton::clicked, [&]() {

		// 这里使用定时器也要使用其他线程
		if (this->m_autoPlay == false)
		{
			this->m_autoPlay = !this->m_autoPlay;
			std::thread scrollEvent(
				[=]() {

				playThePicture();
			}
			);
			scrollEvent.detach();
		}
		else
		{
			this->m_autoPlay = !this->m_autoPlay;;			// 停止标志，修改滚动条的线程会自动退出
		}

		//this->m_autoPlay = !this->m_autoPlay;
		if (this->m_autoPlay)
		{
			//	playBtn->setText(QStringLiteral("停止"));
			QIcon		m_icon_temp = QIcon::fromTheme(QStringLiteral(""),
				QIcon(QStringLiteral(":/new/prefix1/Icon/SelectAll_16x16.png")));
			playBtn->setIcon(m_icon_temp);
		}
		else
		{
			//playBtn->setText(QStringLiteral("播放")); 
			QIcon	m_icon_temp = QIcon::fromTheme(QStringLiteral(""),
				QIcon(QStringLiteral(":/new/prefix1/Icon/Media_16x16.png")));
			playBtn->setIcon(m_icon_temp);
		}
	}
	);
	connect(this->addSpeedBtn, &QPushButton::clicked, [&]() {
		if (this->m_playSpeed < 10)
		{
			this->m_playSpeed += 1;

		}
	}
	);
	connect(this->subBtn, &QPushButton::clicked, [&]() {
		if (this->m_playSpeed > 1)
		{
			this->m_playSpeed--;
		}
		else
		{

		}
	}
	);
	connect(jumpBtn, &QPushButton::clicked, this, &hnContinuouslyBrowsePixWidget::slot_JumpToUserMile);


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
	toolBarLayout->addSpacerItem(new QSpacerItem(20, 10, QSizePolicy::Fixed, QSizePolicy::Minimum));




	layoutUpDown->addLayout(toolBarLayout);
	layoutUpDown->addLayout(this->m_mainLayout);
	this->setLayout(layoutUpDown); 
	 
	connect(deepExampleBtn, &QPushButton::clicked, this, &hnContinuouslyBrowsePixWidget::slot_Show3dDeepExample);

}

void hnContinuouslyBrowsePixWidget::initSigSlot()
{

	//信号槽连接   当滚动条数值变化时，通知label  进行相关操作
	connect(this->m_scrollbar, &QScrollBar::valueChanged,
        [this](int value) {
        QElapsedTimer timer;
        timer.start();
        m_browsePixWidget->slot_updateCurrentScrollBar(value);
        qDebug().noquote() << "[HN_PERF][BrowseScrollValueChanged]"
            << "value=" << value
            << "minimum=" << m_scrollbar->minimum()
            << "maximum=" << m_scrollbar->maximum()
            << "elapsedMs=" << timer.elapsed();

    });

	//信号槽连接   showpixlabel告诉滚动条 滚动条的最大值
	connect(this->m_browsePixWidget, &hnBrowsePixWidget::sig_scrollBarMaxValueChanged, 
		this,&hnContinuouslyBrowsePixWidget::slot_setScrollBarMaxValue);

	//信号槽连接  showpixlabel发送信号，提供滚动条的值，滚动条接受信号 设置数值
	connect(this->m_browsePixWidget, &hnBrowsePixWidget::sig_scrollBarValueChanged,
		m_scrollbar, &QScrollBar::setValue);

	//信号和信号连接   滚动条发送信号 滚动条变化，这里是给联动用的 
	connect(this->m_scrollbar, &QScrollBar::valueChanged, [this](int value) {
		//如果浏览窗口允许联动，就发送信号
		if (m_browsePixWidget->getIsAllowLinked())
		{
			emit signal_scrollValueChanged(value);
		}
	});
	 
}

void hnContinuouslyBrowsePixWidget::addWheelScrollStepOption(QHBoxLayout* toolBarLayout)
{
	if (!toolBarLayout || !xrSetting)
	{
		return;
	}
	wheelOneImageChechBox = new QCheckBox(QStringLiteral("按张翻页"));
	wheelOneImageChechBox->setMaximumWidth(85); 
	wheelOneImageChechBox->setChecked(xrSetting->wheelScrollOneImage);
	toolBarLayout->addWidget(wheelOneImageChechBox);

	connect(wheelOneImageChechBox, &QCheckBox::stateChanged, this, [this](int state)
	{

		xrSetting->wheelScrollOneImage = (state == Qt::Checked);
		xrSetting->writeData();
	});
}

hnBrowsePixWidget * hnContinuouslyBrowsePixWidget::getShowPixLabel()
{
	return this->m_browsePixWidget;
}

void hnContinuouslyBrowsePixWidget::slot_setScrollBarMaxValue(int maxValue)
{
	this->m_scrollbar->setMaximum(maxValue);
}


void hnContinuouslyBrowsePixWidget::slot_updateScrollBarValue(int buttomFrameIdx)
{
	this->m_scrollbar->setValue(this->m_scrollbar->maximum() - buttomFrameIdx * 2);
}

void hnContinuouslyBrowsePixWidget::slot_setSelectedDiseaseId(int id)
{
	m_browsePixWidget->setSelectedDiseaseId(id);
}

void hnContinuouslyBrowsePixWidget::slot_updateBrowser()
{
    qDebug().noquote() << "[HN_PERF][BrowseUpdateScheduled]"
        << "delayMs=500"
        << "scrollValue=" << (m_scrollbar ? m_scrollbar->value() : -1);
    QTimer::singleShot(500, [this]() {
        QElapsedTimer timer;
        timer.start();
        m_browsePixWidget->update();
        qDebug().noquote() << "[HN_PERF][BrowseUpdateTriggered]"
            << "scrollValue=" << (m_scrollbar ? m_scrollbar->value() : -1)
            << "elapsedMs=" << timer.elapsed();
    }
    );

}

 
void hnContinuouslyBrowsePixWidget::slot_updateDmiLable(int value)
{
	this->dmiBlock->setText(QString::number( value));
}

void hnContinuouslyBrowsePixWidget::loadPix(const QString & pixDirName)
{
	this->m_browsePixWidget->loadPix(pixDirName);
}

void hnContinuouslyBrowsePixWidget::loadPix(const QStringList & pixNames)
{
	this->m_browsePixWidget->loadPix(pixNames);
}

void hnContinuouslyBrowsePixWidget::wheelEvent(QWheelEvent * event)
{
    QElapsedTimer timer;
    timer.start();
    if (!m_scrollbar)
    {
        qDebug().noquote() << "[HN_PERF][BrowseWheel]" << "reason=noScrollbar";
        QWidget::wheelEvent(event);
        return;
    }
    const int oldValue = m_scrollbar->value();
    const int step = browseStep();
    if (event->delta() > 0)// 当滚轮远离使用者时
    {

        this->m_scrollbar->setValue(m_scrollbar->value() - step);
    }
    else// 当滚轮向使用者方向旋转时
    {
        this->m_scrollbar->setValue(m_scrollbar->value() + step);
    }
    qDebug().noquote() << "[HN_PERF][BrowseWheel]"
        << "delta=" << event->delta()
        << "step=" << step
        << "oldValue=" << oldValue
        << "newValue=" << m_scrollbar->value()
        << "elapsedMs=" << timer.elapsed();
    event->accept();
}





void hnContinuouslyBrowsePixWidget::keyPressEvent(QKeyEvent *event)
{
	QWidget::keyPressEvent(event);

}

void hnContinuouslyBrowsePixWidget::enterEvent(QEvent * event)
{
	emit signal_enterWidget();
}

void hnContinuouslyBrowsePixWidget::playThePicture()
{
	 
		while (this->m_autoPlay)
		{
			std::this_thread::sleep_for(chrono::milliseconds(500 / this->m_playSpeed));
			this->m_scrollbar->setValue(m_scrollbar->value() - 1);			// 滚动条从下向上滚动
			std::this_thread::sleep_for(chrono::milliseconds(500 / this->m_playSpeed));
		}
		;
}

int hnContinuouslyBrowsePixWidget::getBrowStep(bool is3d) const
{
	if (is3d)
	{
		return 1;
	}
	else
	{
		return (xrSetting && xrSetting->wheelScrollOneImage) ? 2 : 1;

	}
}


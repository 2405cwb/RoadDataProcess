#include "hn3dPixScrollWidget.h"

hn3dPixScrollWidget::hn3dPixScrollWidget(QWidget *parent)
	: hnContinuouslyBrowsePixWidget(parent)
{
	m_p3dImageViewWidget = new hn3dPixWidget;
	this->setBrowsePixWidget(m_p3dImageViewWidget,false);
	initConnect();
}

hn3dPixScrollWidget::~hn3dPixScrollWidget()
{

}

// 加载影像
void hn3dPixScrollWidget::load3dImage()
{
	m_p3dImageViewWidget->load3DImagePictures();
}

void hn3dPixScrollWidget::laodPicRetainScrollBarValue()
{
	//获取当前进度条的值
	int currentScrollBarValue = this->getCurrentScrollBarValue();
	//加载新图片
	this->m_p3dImageViewWidget->load3DImagePictures();
	//设置当前进度条的值
	this->setCurrentScrollBarValue(currentScrollBarValue);
}

hn3dPixWidget * hn3dPixScrollWidget::getPixWidget()
{
	return this->m_p3dImageViewWidget;
}



void hn3dPixScrollWidget::initConnect()
{
	connect(this, &hnContinuouslyBrowsePixWidget::signal_moveMouse, m_browsePixWidget, &hnBrowsePixWidget::slot_moveMouse);
	//connect(this->m_scrollbar, &CustomScrollBar::valueChanged, this, &hn3dPixScrollWidget::slot_BlockValueChanged);
}

void hn3dPixScrollWidget::keyPressEvent(QKeyEvent *event)
{

	auto function = [this]() {
		while (this->m_autoPlay)
		{
			std::this_thread::sleep_for(chrono::milliseconds(500 / this->m_playSpeed));
			this->m_scrollbar->setValue(m_scrollbar->value() - 1);			// 滚动条从下向上滚动
			std::this_thread::sleep_for(chrono::milliseconds(500 / this->m_playSpeed));
		}
	};

	// 按空格键修改滚动条的值
	if (event->key() == Qt::Key_Space)
	{
		// 这里使用定时器也要使用其他线程
		if (this->m_autoPlay == false)
		{
			this->m_autoPlay = !this->m_autoPlay;
			std::thread scrollEvent(function);
			scrollEvent.detach();
		}
		else
		{
			this->m_autoPlay = !this->m_autoPlay;;			// 停止标志，修改滚动条的线程会自动退出
		}
	}

	if (event->key() == Qt::Key_Up || event->key() == Qt::Key_W)
	{
		const bool up = true;
		const int step = getBrowStep(true);
		this->m_scrollbar->setValue(m_scrollbar->value() - step); 

		QTimer::singleShot(0, this, [this, up]
		{
			emit signal_moveMouse(up, false);
		});
		event->accept();
		return;
		

		//QCoreApplication::processEvents();
	}
	else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_S)
	{
		const bool up = false;

		const int step = getBrowStep(true);
		this->m_scrollbar->setValue(m_scrollbar->value() + step);

		QTimer::singleShot(0, this, [this, up]
		{
			emit signal_moveMouse(up, false);
		});
		event->accept();
		return;

	}
	QWidget::keyPressEvent(event);
}

 

void hn3dPixScrollWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button()== Qt::RightButton)
	{
		//弹出右键菜单

	}
}

void hn3dPixScrollWidget::slot_BlockValueChanged(int value)
{
	
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	
	int processValue = value;
	int dmiIndex = this->m_scrollbar->maximum() - processValue;
	//将传进的index转成里程
	auto projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	double roadLenth =8/2;
	if (roadLenth == 0)
	{
		return;
	}
	double curDmi = roadLenth * dmiIndex;

	//将传进的index比转成桩号
	double curMile = hnDataManager::getDataManager()->getCurrentProject()->enclToTrueMile(curDmi);

	//赋值给文本框
	//this->dmiBlock->setText(QString::number((int)curDmi));
	//this->mileBlock->setText(QString::number((int)curMile));

	 
}

void hn3dPixScrollWidget::slot_JumpToUserMile()
{
	////根据桩号进行跳转
	//double encoderMile = this->mileBlock->text().toDouble();
	//if (!hnDataManager::getDataManager()->isOpenProject())
	//{
	//	return;
	//}
	//if (!hnDataManager::getDataManager()->getCurrentProject()->get2DProject() ||
	//	!hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	//{
	//	return;
	//}
	//encoderMile = hnDataManager::getDataManager()->getCurrentProject()->trueMileToEncl(encoderMile);

	//if (encoderMile < 0)
	//{
	//	encoderMile = 0;
	//}
	//double diff2d3d = hnDataManager::getDataManager()->getCurrentProject()->get2d3dMileDiff();
	//encoderMile -= diff2d3d;

	////纯三维
	//const int roadHeight = 8;
 //
	//int maxValue =  this->m_scrollbar->maximum();
	//int jumpToIndex = maxValue - (int)((encoderMile + diff2d3d)/4);
	//this->m_scrollbar->setValue(jumpToIndex); 

	//this->slot_updateDmiLable((int)(encoderMile+ diff2d3d));
}

void hn3dPixScrollWidget::slot_Show3dDeepExample()
{
	double downDeep = -10;
	double upDeep = 10;
	 //显示深度示例
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}

	QDialog dialog;
	dialog.setWindowTitle(QStringLiteral("深度色带"));
	dialog.setFixedSize(200, 400);

	//色带参数
	const int bandWidth = 40;
	const int bandHeight = 300;
	const int bandX = 40;
	const int bandY = 20;

	 

	//创建QPixmap绘制色带
	QPixmap pixmap(dialog.width(),dialog.height());
	pixmap.fill(Qt::white);

	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing);


	QLinearGradient gradient(0, bandY, 0, bandY + bandHeight); 
	gradient.setColorAt(0.0, Qt::red);
	gradient.setColorAt(0.5, Qt::green);
	gradient.setColorAt(1.0, Qt::blue);
	painter.fillRect(bandX, bandY, bandWidth, bandHeight, gradient);

	//绘制刻度
	painter.setPen(Qt::black);
	const int numTicks = 6; //刻度数量
	const int margin = 5;
	for (int i = 0 ; i <= numTicks;++i)
	{
		double tickValue = upDeep - (upDeep - downDeep) * (1.0 - static_cast<double>(i) / (numTicks - 1));

		//计算刻度位置
		int yPos = bandY + bandHeight * (numTicks - 1 - i) / (numTicks - 1);

		painter.drawLine(bandX + bandWidth + margin, yPos, bandX + bandWidth + margin + 10, yPos);

		QString valueText = QString::number(tickValue, 'f', 1);
		painter.drawText(bandX + bandWidth + margin + 15, yPos + 5, valueText);
	}

	//添加标题
	painter.setFont(QFont("Arial", 9));
	painter.drawText(bandX, bandY + bandHeight +30,QStringLiteral("深度范围: %1 至 %2").arg(downDeep).arg(upDeep));

	//结果
	QLabel * label = new QLabel(&dialog);
	label->setPixmap(pixmap);

	QVBoxLayout * layout = new QVBoxLayout(&dialog);
	layout->addWidget(label);
	dialog.exec();
}

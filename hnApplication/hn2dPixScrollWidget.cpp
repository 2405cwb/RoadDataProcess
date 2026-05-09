#include "hn2dPixScrollWidget.h"

hn2dPixScrollWidget::hn2dPixScrollWidget(QWidget *parent)
	:hnContinuouslyBrowsePixWidget(parent)
{
	m_roadDamageBrowserPixWidget = new hn2dPixWidget;
	this->setBrowsePixWidget(m_roadDamageBrowserPixWidget,true); 
	initConnect();
}

void hn2dPixScrollWidget::loadRoadPicture()
{
	 
	this->m_roadDamageBrowserPixWidget->loadRoadPicture();
}

void hn2dPixScrollWidget::initConnect()
{
	connect(this, &hnContinuouslyBrowsePixWidget::signal_moveMouse, m_browsePixWidget, &hnBrowsePixWidget::slot_moveMouse);
	connect(this->m_scrollbar, &CustomScrollBar::valueChanged, this, &hn2dPixScrollWidget::slot_BlockValueChanged);
	connect(this->m_scrollbar, &CustomScrollBar::valueChanged, this, &hn2dPixScrollWidget::slot_roadMileAndDmiChanged);
}

void hn2dPixScrollWidget::keyPressEvent(QKeyEvent *event)
{

	//auto function = [this]() {
	//	while (this->m_autoPlay)
	//	{
	//		std::this_thread::sleep_for(chrono::milliseconds(500 / this->m_playSpeed));
	//		this->m_scrollbar->setValue(m_scrollbar->value() - 1);			// 滚动条从下向上滚动
	//		std::this_thread::sleep_for(chrono::milliseconds(500 / this->m_playSpeed));
	//	}
	//};

	

	// 按空格键修改滚动条的值
	if (event->key() == Qt::Key_Space)
	{
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
	}


	if (event->key() == Qt::Key_Up || event->key() == Qt::Key_W)
	{
		emit signal_moveMouse(true, true);
		this->m_scrollbar->setValue(m_scrollbar->value() - 1);


		//QCoreApplication::processEvents();
	}
	else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_S)
	{
		emit signal_moveMouse(false, true);
		this->m_scrollbar->setValue(m_scrollbar->value() + 1);

		//QCoreApplication::processEvents();

	}
	QWidget::keyPressEvent(event);
}

void hn2dPixScrollWidget::slot_BlockValueChanged(int value)
{
	int processValue = value;

	
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	int dmiIndex = this->m_scrollbar->maximum() - processValue;



	//将传进的index转成里程
	auto projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	double roadLenth = projectSetInfo.dRoadLength;
	if (roadLenth == 0)
	{
		return;
	}
	double curDmi = roadLenth * dmiIndex/2.0;

	//将传进的index比转成桩号
	double curMile = hnDataManager::getDataManager()->getCurrentProject()->enclToTrueMile(curDmi);
	 
	//赋值给文本框
	this->dmiBlock->setText(QString::number((int)curDmi));
	this->mileBlock->setText(QString::number((int)curMile)); 

}

void hn2dPixScrollWidget::slot_JumpToUserMile()
{
 
	//根据桩号进行跳转
	double encoderMile = this->mileBlock->text().toDouble();
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	encoderMile = hnDataManager::getDataManager()->getCurrentProject()->trueMileToEncl(encoderMile);

	if (encoderMile < 0)
	{
		encoderMile = 0;
	} 

	auto projectType = hnDataManager::getDataManager()->getCurrentProject()->getProjectType();
	if (PROJECT_23D_TYPE == projectType ||
		PROJECT_2D_TYPE == projectType)
	{
		//桩号转成 帧号
		auto projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
		int imageNum = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMileVector().size();
		if (encoderMile > imageNum * projectSetInfo.dRoadLength)
		{
			encoderMile = imageNum * projectSetInfo.dRoadLength;
		}
		double roadLenth = projectSetInfo.dRoadLength;
		if (roadLenth == 0)
		{
			return;
		}
		int frameIdx =encoderMile/ roadLenth;
		int jumpToIndex = this->m_scrollbar->maximum() - frameIdx * roadLenth;
		this->m_scrollbar->setValue(jumpToIndex); 
	}
	else
	{
		//纯三维
		const int roadHeight = 8;
		const int frameIdx3d = encoderMile / roadHeight;
	}  
	this->slot_updateDmiLable((int)encoderMile);
}

 
void hn2dPixScrollWidget::slot_Show3dDeepExample()
{

}

void hn2dPixScrollWidget::slot_roadMileAndDmiChanged(int value)
{
	int processValue = value;


	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	int dmiIndex = this->m_scrollbar->maximum() - processValue;



	//将传进的index转成里程
	auto projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	double roadLenth = projectSetInfo.dRoadLength;
	if (roadLenth == 0)
	{
		return;
	}
	double curDmi = roadLenth * dmiIndex / 2.0;

	//将传进的index比转成桩号
	double curMile = hnDataManager::getDataManager()->getCurrentProject()->enclToTrueMile(curDmi);
	emit signal_roadMileAndDmiChanged(curMile, curDmi); 
}

 

hn2dPixWidget * hn2dPixScrollWidget::getPixWidget()
{
	return this->m_roadDamageBrowserPixWidget;
}

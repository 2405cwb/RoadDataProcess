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
	const double currentEncoderMile = this->m_p3dImageViewWidget->currentBottomEncoderMile();
	this->m_p3dImageViewWidget->load3DImagePictures();
	this->m_p3dImageViewWidget->scrollBottomToEncoderMile(currentEncoderMile);
}

hn3dPixWidget * hn3dPixScrollWidget::getPixWidget()
{
	return this->m_p3dImageViewWidget;
}



void hn3dPixScrollWidget::initConnect()
{
	connect(this, &hnContinuouslyBrowsePixWidget::signal_moveMouse, m_browsePixWidget, &hnBrowsePixWidget::slot_moveMouse);
}

void hn3dPixScrollWidget::keyPressEvent(QKeyEvent *event)
{
	QWidget::keyPressEvent(event);
}

 

int hn3dPixScrollWidget::browseStep() const
{
	return getBrowStep(true);
}

bool hn3dPixScrollWidget::is2DView() const
{
	return false;
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
	Q_UNUSED(value);
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

#include "hn2dPixScrollWidget.h"
#include <QTimer>
hn2dPixScrollWidget::hn2dPixScrollWidget(QWidget *parent)
	:hnContinuouslyBrowsePixWidget(parent)
{
	m_roadDamageBrowserPixWidget = new hn2dPixWidget;
	this->setBrowsePixWidget(m_roadDamageBrowserPixWidget,true); 
	initConnect();
}

void hn2dPixScrollWidget::loadRoadPicture()
{
	stopAutoPlay();
	this->m_roadDamageBrowserPixWidget->loadRoadPicture();
}

void hn2dPixScrollWidget::initConnect()
{
	connect(this, &hnContinuouslyBrowsePixWidget::signal_moveMouse, m_browsePixWidget, &hnBrowsePixWidget::slot_moveMouse);
	connect(m_roadDamageBrowserPixWidget, &hn2dPixWidget::signal_sdkBottomEncoderMileChanged,
		this, &hn2dPixScrollWidget::slot_sdkBottomEncoderMileChanged);
	connect(this, &hnContinuouslyBrowsePixWidget::signal_imageBrightnessChanged,
		m_roadDamageBrowserPixWidget, &hn2dPixWidget::setSdkImageBrightness);
}

void hn2dPixScrollWidget::keyPressEvent(QKeyEvent *event)
{
	QWidget::keyPressEvent(event);
}

int hn2dPixScrollWidget::browseStep() const
{
	return getBrowStep(false);
}

bool hn2dPixScrollWidget::is2DView() const
{
	return true;
}

bool hn2dPixScrollWidget::stepOneImage()
{
	return m_roadDamageBrowserPixWidget
		&& m_roadDamageBrowserPixWidget->stepSingleFrame(1);
}

void hn2dPixScrollWidget::slot_BlockValueChanged(int value)
{
	Q_UNUSED(value);
}

void hn2dPixScrollWidget::slot_JumpToUserMile()
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}

	double encoderMile = this->mileBlock->text().toDouble();
	encoderMile = hnDataManager::getDataManager()->getCurrentProject()->trueMileToEncl(encoderMile);
	encoderMile = qMax(0.0, encoderMile);

	m_roadDamageBrowserPixWidget->scrollBottomToEncoderMile(encoderMile);
	this->slot_updateDmiLable((int)encoderMile);
}

void hn2dPixScrollWidget::slot_Show3dDeepExample()
{

}

void hn2dPixScrollWidget::slot_sdkBottomEncoderMileChanged(double encoderMile)
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}

	const double curMile = hnDataManager::getDataManager()->getCurrentProject()->enclToTrueMile(encoderMile);
	this->dmiBlock->setText(QString::number((int)encoderMile));
	this->mileBlock->setText(QString::number((int)curMile));
	emit signal_roadMileAndDmiChanged(curMile, encoderMile);
}

void hn2dPixScrollWidget::slot_roadMileAndDmiChanged(int value)
{
	Q_UNUSED(value);
}

 

hn2dPixWidget * hn2dPixScrollWidget::getPixWidget()
{
	return this->m_roadDamageBrowserPixWidget;
}

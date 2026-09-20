#include "statusBarWidget.h"

statusBarWidget::statusBarWidget(QWidget *parent)
	: QLabel(parent)
{
	this->init();

	this->connectSignalSlot();
}

statusBarWidget::~statusBarWidget()
{
	this->releaseMemory();
}

void statusBarWidget::updateLabelTextSlot(const QString &text)
{
	this->setToolTip(text);
	this->setText(fontMetrics().elidedText(text, Qt::ElideRight, qMax(80, width() - 8)));
	this->update();
}

void statusBarWidget::updateLabelTextSlot()
{
	QString text = QString("%1:%2").arg(QString::fromLocal8Bit("当前路面信息:"))
		.arg(this->m_statusInfo.find(statusType::MOUSE_FRAME_IDX).value());
}

void statusBarWidget::connectSignalSlot()
{

}

void statusBarWidget::init()
{
	this->m_statusInfo.insert(statusType::MOUSE_FRAME_IDX, "");
	this->m_statusInfo.insert(statusType::MOUSE_POS, "1");

	//test
	this->setText(QStringLiteral("就绪 · 打开工程后可浏览影像和检查数据"));
}

void statusBarWidget::releaseMemory()
{

}

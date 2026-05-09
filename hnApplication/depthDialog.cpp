#include "depthDialog.h"

depthDialog::depthDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	connect(ui.okPushButton, &QPushButton::clicked, this, &depthDialog::slot_okPushButtonClicked);
	connect(ui.cancelPushButton, &QPushButton::clicked, this, &depthDialog::slot_cancelPushButtonClicked);
}

depthDialog::~depthDialog()
{
}

void depthDialog::setDepth(double depth)
{
	m_depth = depth;
	//米转成毫米
	ui.depthLineEdit->setText(QString::number(depth * 1000, 'f', 2));
}

double depthDialog::getDepth()
{
	return m_depth;
}

void depthDialog::slot_okPushButtonClicked()
{
	m_depth = ui.depthLineEdit->text().toDouble() / 1000.0f;
	this->accept();
}

void depthDialog::slot_cancelPushButtonClicked()
{
	this->reject();
}

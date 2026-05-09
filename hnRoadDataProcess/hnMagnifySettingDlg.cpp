#include "hnMagnifySettingDlg.h"
#include <QRegExp>
#include <QRegExpValidator>

hnMagnifySettingDlg::hnMagnifySettingDlg(QWidget *parent)
	: QDialog(parent),
	m_pixelSizeLabel(new QLabel(QString::fromLocal8Bit("放大区域边长"))),
	m_pixelSizeLineEdit(new QLineEdit),
	m_offsetLabel(new QLabel(QString::fromLocal8Bit("偏移大小"))),
	m_offsetLineEdit(new QLineEdit),
	m_magnificationLabel(new QLabel(QString::fromLocal8Bit("放大倍数"))),
	m_magnificationLineEdit(new QLineEdit),
	m_okPushButton(new QPushButton(QString::fromLocal8Bit("确定"))),
	m_cancelPushButton(new QPushButton(QString::fromLocal8Bit("取消")))
{
	this->initValidator();
		
	connect(m_okPushButton, &QPushButton::clicked, this, &hnMagnifySettingDlg::slot_onOkPushButtonClicked);
	connect(m_cancelPushButton, &QPushButton::clicked, this, &hnMagnifySettingDlg::slot_onCancelPushButtonClicked);

	QFormLayout *formLayout = new QFormLayout;
	formLayout->addRow(m_pixelSizeLabel, m_pixelSizeLineEdit);
	formLayout->addRow(m_offsetLabel, m_offsetLineEdit);
	formLayout->addRow(m_magnificationLabel, m_magnificationLineEdit);

	QHBoxLayout *okcancelLayout = new QHBoxLayout;
	okcancelLayout->addWidget(m_okPushButton);
	okcancelLayout->addWidget(m_cancelPushButton);
	
	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(formLayout);
	mainLayout->addLayout(okcancelLayout);
	this->setLayout(mainLayout);

	this->setWindowTitle(QString::fromLocal8Bit("放大镜设置"));

}

void hnMagnifySettingDlg::getMagnifySetting(int & pixelSize, int & offset, int & magnifynition)
{
	pixelSize = m_pixelSizeLineEdit->text().toInt();
	offset = m_offsetLineEdit->text().toInt();
	magnifynition = m_magnificationLineEdit->text().toInt();
}

void hnMagnifySettingDlg::setMagnifySetting(const int pixelSize, const int offset, const int magnifynition)
{
	m_pixelSizeLineEdit->setText(QString::number(pixelSize));
	m_offsetLineEdit->setText(QString::number(offset));
	m_magnificationLineEdit->setText(QString::number(magnifynition));
}

void hnMagnifySettingDlg::slot_onOkPushButtonClicked()
{
	this->accept();
}

void hnMagnifySettingDlg::slot_onCancelPushButtonClicked()
{
	this->reject();
}

void hnMagnifySettingDlg::initValidator()
{
	//这个用指针就会生效了
	QIntValidator * pixelValidator = new QIntValidator(this);
	pixelValidator->setRange(50, 300);
	m_pixelSizeLineEdit->setValidator(pixelValidator);

	QIntValidator * offsetValidator = new QIntValidator(this);
	offsetValidator->setRange(0, 300);
	m_offsetLineEdit->setValidator(offsetValidator);

	QIntValidator * magnificationValidator = new QIntValidator(this);
	magnificationValidator->setRange(1, 10);
	m_magnificationLineEdit->setValidator(magnificationValidator);
}

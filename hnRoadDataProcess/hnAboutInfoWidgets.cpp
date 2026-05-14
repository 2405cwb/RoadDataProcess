#include "hnAboutInfoWidgets.h"
#include <QPushButton>
#include <QPixmap>
hnAboutInfoWidgets::hnAboutInfoWidgets(QWidget *parent)
{
	ui.setupUi(this);
	connect(ui.pushButton, &QPushButton::clicked, this, &hnAboutInfoWidgets::okButton_Slot);
	connect(ui.pushButton_2, &QPushButton::clicked, this, &hnAboutInfoWidgets::MessageButton_Slot);
	QPixmap  logo(":/icons/iconsNew/logo_xroe.ico");

	ui.label->setPixmap(logo.scaled(200,200,Qt::KeepAspectRatio,Qt::SmoothTransformation));
	//ui.label->setScaledContents(true);



	ui.label_2->setText( QString::fromLocal8Bit(
		"<html><head/><body><p>二三维内业数据处理软件</p><p>hnRoadDataProcess</p><p>"
		"版本号：V1.8.0</p><p>"
		"版本类型：Official</p></body></html>"));
	ui.label_6->setText(QString::fromLocal8Bit("<html><head/><body><p>CopyRight (C) 2020-2021 武汉汉宁轨道交通技术有限公司 保留所有权力</p></body></html>"));

}

hnAboutInfoWidgets::~hnAboutInfoWidgets()
{
}

void hnAboutInfoWidgets::okButton_Slot()
{
	this->reject();
}

void hnAboutInfoWidgets::MessageButton_Slot()
{
	this->reject();

}

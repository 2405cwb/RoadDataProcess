#pragma region 文件说明
/*! @hnOriginalScalePixShowWidget.cpp
********************************************************************************
<PRE>
模块名       : hnIpsDataProcess
文件名       : hnOriginalScalePixShowWidget.cpp
相关文件     : hnOriginalScalePixShowWidget.h
文件实现功能 : 以原始比例显示图片的界面
作者         : 陈智超
版本         : 1.0.0
--------------------------------------------------------------------------------
备注         :
--------------------------------------------------------------------------------
修改记录 :
日期        版本     修改人              修改内容
2022/10/11	1.0.0	 陈智超				 创建初版
</PRE>
*******************************************************************************/

#pragma endregion


#include "hnOriginalScalePixShowWidget.h"

hnOriginalScalePixShowWidget::hnOriginalScalePixShowWidget(QWidget *parent)
	: QLabel(parent)
{
	//label申请内存
	//this->m_showPixLabel = new QLabel();

	//布局申请内存
	//this->m_mainGridLayout = new QGridLayout();

	//布局添加label
	//this->m_mainGridLayout->addWidget(this->m_showPixLabel);

	//this设置布局
	//this->setLayout(this->m_mainGridLayout);

	//设置自动调节图片大小
	//this->m_showPixLabel->setScaledContents(true);

	//设置策略为忽略  这样就可以随意伸缩了  控件自适应窗口大小
	this->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	//设置鼠标追踪
	this->setMouseTracking(true);

	//设置鼠标指针形状为十字
	this->setCursor(Qt::CrossCursor);
}

hnOriginalScalePixShowWidget::~hnOriginalScalePixShowWidget()
{

}

void hnOriginalScalePixShowWidget::resetWidget()
{
	QImage image;
	this->setPixmap(QPixmap::fromImage(image));
}

void hnOriginalScalePixShowWidget::slot_updatePix(QImage image)
{

	//test
	//qDebug() << "slot_updatePix";
	this->setPixmap(QPixmap::fromImage(image));

	this->m_tmpLabelImage = image;
}


void hnOriginalScalePixShowWidget::resizeEvent(QResizeEvent * event)
{
	//把显示图片的label尺寸发送出去
	emit this->sig_widgetSizeChanged(this->width(), this->height());

	//qDebug() << "this->m_showPixLabel->width():" << this->m_showPixLabel->width();
}




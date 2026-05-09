#pragma once

#pragma region 文件说明
/*! @hnOriginalScalePixShowWidget.h
********************************************************************************
<PRE>
模块名       : hnIpsDataProcess
文件名       : hnOriginalScalePixShowWidget.h
相关文件     : hnOriginalScalePixShowWidget.cpp
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


#include <QWidget>
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QDebug>
#include <QGridLayout>
#include <QMouseEvent>

class hnOriginalScalePixShowWidget : public QLabel
{
	Q_OBJECT

public:
	hnOriginalScalePixShowWidget(QWidget *parent = Q_NULLPTR);
	~hnOriginalScalePixShowWidget();

	/*公共接口*/
public:
	//重置窗口
	void resetWidget();

signals:
	/*
	 *	信号:窗口大小变化
	 *	w:窗口的宽   h:窗口的高
	*/
	void sig_widgetSizeChanged(int w ,int h);


public slots:
	//槽函数  接收image  并在图片上显示
	void slot_updatePix(QImage image);

	//事件重载
protected:
	//窗口大小变换事件
	void resizeEvent(QResizeEvent *event) override;

private:
	//临时储存label image的变量
	QImage m_tmpLabelImage;

};

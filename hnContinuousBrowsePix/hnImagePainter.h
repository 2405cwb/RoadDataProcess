#pragma once

#include <QObject>
#include <QImage>
#include "hncontinuousbrowsepix_global.h"
#include <QWidget>
//往图片上画东西的类
class HNCONTINUOUSBROWSEPIX_EXPORT hnImagePainter : public QObject
{
	Q_OBJECT

public:
	hnImagePainter(QObject *parent = Q_NULLPTR);
	~hnImagePainter();

public:
	/*
	*接口名：drawImageOnAnotherImage
	*接口含义：往一个image上画另一个image				
	*参数一：[QImage const  & const] 要画的image
	*参数二：[QImage & const] painter的画板，就是往它上面画
	*参数三：[double]：画的图片原点x
	*参数四：[double]：画的图片原点y
	*参数五：[double]：图片宽
	*参数六：[double]：图片高
	*参数七：[bool]：是否画一半图片
	*/
	void drawImageOnAnotherImage(QImage const  & const srcImage, QImage & const dstImage, 
		double x, double y, double w, double h, bool isHalf);

	void drawRectOnImage(QImage & image, QRect & rect, const QString & text, const QColor & rectColor, const QColor & textColor);
	void drawRectOnWidget(QWidget*  widget, QRect & rect, const QString & text, const QColor & rectColor, const QColor & textColor);

	void drawRectOnImage(QImage & image, QRect & rect, const QString & text, const QColor & rectColor, const QColor & textColor,int yOffset );

	void drawLineOnImage(QImage & image, QLine & line, const QString & text, const QColor & lineColor, const QColor & textColor);

	void drawTextOnImage(QImage & image, const QString &text, const QColor & textColor, QPoint textPos);

	void setBoardWidth(const int width);

	void setFontPixelSize(const int fontPixelSize);

private:
	int m_boardWidth = 2;
	int m_fontPixelSize = 120;

};

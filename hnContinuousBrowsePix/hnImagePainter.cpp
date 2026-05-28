#include "hnImagePainter.h"
#include <QPainter>

hnImagePainter::hnImagePainter(QObject *parent)
	: QObject(parent)
{
}

hnImagePainter::~hnImagePainter()
{
}



void hnImagePainter::drawImageOnAnotherImage(QImage const  & const srcImage, QImage & const dstImage, double x, double y, double w, double h, bool isHalf)
{
	if (srcImage.isNull())
	{
		return;
	}

	QImage image = srcImage;
	if (isHalf)
	{
		image = image.copy(0, 0, image.width(), image.height() / 2);
		h = h / 2;
	}

	QPainter painter(&dstImage);
	if (!image.isNull())
	{
		painter.drawImage(QRectF(x, y, w, h), image, image.rect());
	}

}


//画矩形 参数1：矩形  参数2：矩形左上角的文字 参数3：矩形边框颜色  参数4：文字颜色
void hnImagePainter::drawRectOnImage(QImage & image, QRect & rect, const QString & text, const QColor & rectColor, const QColor & textColor)
{
	 QPainter painter(&image); 

	painter.setRenderHint(QPainter::Antialiasing); //启用抗锯齿
	painter.setRenderHint(QPainter::TextAntialiasing);
	painter.setWorldMatrixEnabled(false);   //禁用变换矩阵
	QPen rectPen;								//声明画笔
	rectPen.setWidth(m_boardWidth);			//设置宽度
	rectPen.setColor(rectColor);				//设置颜色
	painter.setPen(rectPen);					//画家设置画笔
	painter.drawRect(rect);						//画矩形

	QPen textPen;								//声明画笔
	QFont font = painter.font();
//	font.setBold(true);
	font.setPixelSize(m_fontPixelSize);
	font.setFamily("Microsoft YaHei");
	painter.setFont(font);
	textPen.setColor(textColor);				//设置颜色
	painter.setPen(textPen);					//画家设置画笔
	QFontMetrics fontMetrics(painter.font());
	int height = fontMetrics.height();
	QPoint centerPoint = rect.center();
	QPoint textPoint;
	// 病害靠近右侧时文字会超出界面导致无法看到，这里根据距离值修正位置
	 
	int left_X_int = rect.topLeft().x();
	//如果图像进行了翻转  会导致左角点实际为右角点

	if (rect.width()<0)
	{
		left_X_int = rect.topRight().x();
	}

	if (image.width() - left_X_int < 1300) {
		textPoint.setX(centerPoint.x() - qAbs(0.5*rect.width()) + m_boardWidth - 1300 + (image.width() - left_X_int));
	}
	else
	{
		textPoint.setX(centerPoint.x() - qAbs(0.5*rect.width()) + m_boardWidth);
	}
	textPoint.setY(centerPoint.y() - qAbs(0.5*rect.height())+height );

	
	QStringList lines = text.split('\n');
	 
	for (int i = 0; i < lines.length(); i++)
	{
		QPoint point;
		point.setX(textPoint.x());
		point.setY(textPoint.y() + height*i);
		painter.drawText(point, lines.at(i));			//画文字
	}
	 
}

void hnImagePainter::drawRectOnImage(QImage & image, QRect & rect, const QString & text, const QColor & rectColor, const QColor & textColor, int yOffset)
{
	QPainter painter(&image);
	painter.setRenderHint(QPainter::Antialiasing); //启用抗锯齿

	//禁用变换矩阵
	painter.setWorldMatrixEnabled(false);

	QPen rectPen;								//声明画笔
	rectPen.setWidth(m_boardWidth);			//设置宽度
	rectPen.setColor(rectColor);				//设置颜色
	painter.setPen(rectPen);					//画家设置画笔
	painter.drawRect(rect);						//画矩形

	QPen textPen;								//声明画笔
	QFont font = painter.font();
	//font.setBold(true);
	font.setFamily("Microsoft YaHei");
	font.setPixelSize(m_fontPixelSize);
	painter.setFont(font);
	textPen.setColor(textColor);				//设置颜色
	painter.setPen(textPen);					//画家设置画笔

	QPoint centerPoint = rect.center();
	QPoint textPoint;
	// 病害靠近右侧时文字会超出界面导致无法看到，这里根据距离值修正位置
	if (image.width() - rect.topLeft().x() < 1300) {
		textPoint.setX(centerPoint.x() - qAbs(0.5*rect.width()) + m_boardWidth - 1300 + (image.width() - rect.topLeft().x()));
	}
	else
	{
		textPoint.setX(centerPoint.x() - qAbs(0.5*rect.width()) + m_boardWidth);
	}
	textPoint.setY(centerPoint.y() - qAbs(0.5*rect.height()) - yOffset);

	QFontMetrics fontMetrics(painter.font());
	int height = fontMetrics.height();
	QStringList lines = text.split('\n');
	for (int i = 0; i < lines.length(); i++)
	{
		QPoint point;
		point.setX(textPoint.x());
		point.setY(textPoint.y() + height*i);
		painter.drawText(point, lines.at(i));			//画文字
	}


//	painter.drawText(textPoint, text);			//画文字
}


void hnImagePainter::drawRectOnWidget(QWidget*  widget, QRect & rect, const QString & text, const QColor & rectColor, const QColor & textColor)
{
	QPainter painter(widget);

	painter.setRenderHint(QPainter::Antialiasing); //启用抗锯齿
	painter.setRenderHint(QPainter::TextAntialiasing);
	painter.setWorldMatrixEnabled(false);   //禁用变换矩阵
	QPen rectPen;								//声明画笔
	rectPen.setWidth(m_boardWidth);			//设置宽度
	rectPen.setColor(rectColor);				//设置颜色
	painter.setPen(rectPen);					//画家设置画笔
	painter.drawRect(rect);						//画矩形

	QPen textPen;								//声明画笔
	QFont font = painter.font();
	//	font.setBold(true);
	font.setPixelSize(m_fontPixelSize);
	font.setFamily("Microsoft YaHei");
	painter.setFont(font);
	textPen.setColor(textColor);				//设置颜色
	painter.setPen(textPen);					//画家设置画笔
	QFontMetrics fontMetrics(painter.font());
	int height = fontMetrics.height();
	QPoint centerPoint = rect.center();
	QPoint textPoint;
	// 病害靠近右侧时文字会超出界面导致无法看到，这里根据距离值修正位置

	int left_X_int = rect.topLeft().x();
	//如果图像进行了翻转  会导致左角点实际为右角点

	if (rect.width() < 0)
	{
		left_X_int = rect.topRight().x();
	}

	if (widget->width() - left_X_int < 1300) {
		textPoint.setX(centerPoint.x() - qAbs(0.5*rect.width()) + m_boardWidth - 1300 + (widget->width() - left_X_int));
	}
	else
	{
		textPoint.setX(centerPoint.x() - qAbs(0.5*rect.width()) + m_boardWidth);
	}
	textPoint.setY(centerPoint.y() - qAbs(0.5*rect.height()) + height);


	QStringList lines = text.split('\n');

	for (int i = 0; i < lines.length(); i++)
	{
		QPoint point;
		point.setX(textPoint.x());
		point.setY(textPoint.y() + height*i);
		painter.drawText(point, lines.at(i));			//画文字
	}

}

void hnImagePainter::drawLineOnImage(QImage & image, QLine & line, const QString & text, const QColor & lineColor, const QColor & textColor)
{
	QPainter painter(&image);
	painter.setRenderHint(QPainter::Antialiasing); //启用抗锯齿

												   //禁用变换矩阵
	painter.setWorldMatrixEnabled(false);

	QPen rectPen;								//声明画笔
	rectPen.setWidth(30);						//设置宽度
	rectPen.setColor(lineColor);				//设置颜色
	painter.setPen(rectPen);					//画家设置画笔
	painter.drawLine(line);						//画直线

	QPen textPen;								//声明画笔 
	textPen.setWidth(5);						//设置宽度
	textPen.setColor(textColor);				//设置颜色
	painter.setPen(textPen);					//画家设置画笔
	painter.drawText(line.p1(), text);			//画文字

}

void hnImagePainter::drawTextOnImage(QImage & image, const QString & text, const QColor & textColor, QPoint textPos)
{
	QPainter painter(&image);

	painter.setRenderHint(QPainter::Antialiasing); //启用抗锯齿

												   //禁用变换矩阵
	painter.setWorldMatrixEnabled(false);

	QFont font = painter.font();
	//font.setBold(true);
	font.setPixelSize(m_fontPixelSize);
	//font.setFamily("Microsoft YaHei");
	font.setFamily("SimHei");
	painter.setFont(font);

	QPen textPen;								//声明画笔
	textPen.setWidth(4);						//设置宽度
	textPen.setColor(textColor);				//设置颜色
	painter.setPen(textPen);					//画家设置画笔
	QFontMetrics fontMetrics(painter.font());
	int height = fontMetrics.height();
	QStringList lines = text.split('\n'); 
	for (int i = 0; i < lines.length(); i++)
	{ 
		QPoint point;
		point.setX(textPos.x());
		point.setY(textPos.y() + height*i); 
		painter.setPen(textPen);
		painter.drawText(point, lines.at(i));
	} 
}

void hnImagePainter::setBoardWidth(const int width)
{
	this->m_boardWidth = width;
}

void hnImagePainter::setFontPixelSize(const int fontPixelSize)
{
	this->m_fontPixelSize = fontPixelSize;
}

#pragma once

#include "hnapplication_global.h"
#include <QImage>
#include "opencv2/opencv.hpp"

class HNAPPLICATION_EXPORT hnAdjustImage
{

public:
	hnAdjustImage();

public:
	//获取对比度强度
	double getContrastIntensity();
	//设置对比度强度
	void setContrastIntensity(double intensity);
	//重置对比度强度
	void resetContrastIntensity();

protected:
	//根据现有参数 调整图片
	void adjustImage(QImage &image);

protected:
	//调节图片对比度  alpha 0.0  - 3.0
	QImage adjustContrast(const QImage &image, double alpha);


public:
	//获取亮度
	double getBrightness();
	//设置亮度
	void setBrightness(const double intensity);
	//重置亮度
	void resetBrightness();

protected:
	//调节图片亮度   intensity 0.0 - 3.0
	QImage adjustBrightness(const QImage &image, const double intensity);

protected:
	double m_contrastIntensity;			//对比度强度
	double m_dftContrastIntensity;		//默认对比度强度

	double m_brightnessIntensity;		//亮度强度
	double m_dftBrightnessIntensity;	//默认亮度强度

	double m_sharpenIntensity;			//锐化强度
	double m_dftSharpenIntensity;		//默认锐化强度
};


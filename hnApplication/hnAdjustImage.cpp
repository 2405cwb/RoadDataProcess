#include "hnAdjustImage.h"



hnAdjustImage::hnAdjustImage()
{	
	//初始化对比度的强度
	m_dftContrastIntensity = 1.0;
	m_contrastIntensity = m_dftContrastIntensity;

	//初始化亮度的强度
	m_dftBrightnessIntensity = 1.0;
	m_brightnessIntensity = m_dftBrightnessIntensity;
}

double hnAdjustImage::getContrastIntensity()
{
	return m_contrastIntensity;
}

void hnAdjustImage::setContrastIntensity(double intensity)
{
	m_contrastIntensity = intensity;
}

void hnAdjustImage::resetContrastIntensity()
{
	m_contrastIntensity = m_dftContrastIntensity;
}

void hnAdjustImage::adjustImage(QImage &image)
{
	//调节对比度
	if (m_contrastIntensity != m_dftContrastIntensity)
	{
		this->adjustContrast(image, m_contrastIntensity);
	}

	//调节亮度
	if (m_brightnessIntensity != m_dftBrightnessIntensity)
	{
		this->adjustBrightness(image, m_brightnessIntensity);
	}
}

QImage hnAdjustImage::adjustContrast(const QImage & image, double alpha)
{
	//QImage转mat
	cv::Mat mat(image.height(), image.width(), CV_8UC3, const_cast<uchar*>(image.constBits()), image.bytesPerLine());
	//调整图片的对比度
	cv::Mat adjustedImage = mat;
	cv::multiply(mat, alpha, adjustedImage);

	QImage::Format format = image.format();
	//将修改后的mat转成QImage
	QImage result(adjustedImage.data, adjustedImage.cols, adjustedImage.rows, adjustedImage.step, image.format());

	return result.copy();
}

double hnAdjustImage::getBrightness()
{
	return m_brightnessIntensity;
}

void hnAdjustImage::setBrightness(const double intensity)
{
	m_brightnessIntensity = intensity;
}

void hnAdjustImage::resetBrightness()
{
	m_brightnessIntensity = m_dftBrightnessIntensity;
}

QImage hnAdjustImage::adjustBrightness(const QImage & image, const double intensity)
{
 
	if (image.isNull())
	{
		return QImage();
	}
	cv::Mat mat;
	auto foramt = image.format();
	switch (image.format())
	{
	case QImage::Format_ARGB32:
	case  QImage::Format_RGB32:
	case  QImage::Format_ARGB32_Premultiplied:
		mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
		cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR); //转为标准bgr
		break;
		case  QImage::Format_RGBA8888:
			//3通道
			mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
			cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR); //转为标准bgr
			break;
		case  QImage::Format_Grayscale8:
		case  QImage::Format_Indexed8:
			//单通道
			mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
			break;
		case  QImage::Format::Format_RGB888:
			mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
			cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR); //转为标准bgr
			break;
	default:
		QImage converted = image.convertToFormat(QImage::Format_RGB32);
		mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
		cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR); //转为标准bgr 
		break;
	}

	//调整亮度
	cv::Mat adjusted;
	double realIntensity = (intensity-1.0 )*100;
	 
	mat.convertTo(adjusted, -1, 1.0, realIntensity);

	QImage result;
	int temp = adjusted.channels();
	if (adjusted.channels()==4)
	{
		cv::cvtColor(adjusted, adjusted, cv::COLOR_BGRA2RGBA);
		result = QImage(adjusted.data, adjusted.cols, adjusted.rows, adjusted.step, QImage::Format_RGBA8888).copy();
	}
	else if(adjusted.channels() == 1)
	{
		 
		result = QImage(adjusted.data, adjusted.cols, adjusted.rows, adjusted.step, QImage::Format_Grayscale8).copy();
	}
	else if (adjusted.channels() == 3)
	{

		result = QImage(adjusted.data, adjusted.cols, adjusted.rows, adjusted.step, QImage::Format_RGB888).copy();

	}
	else
	{
		cv::cvtColor(adjusted, adjusted, cv::COLOR_BGR2RGB);
		result = QImage(adjusted.data, adjusted.cols, adjusted.rows, adjusted.step, QImage::Format_RGBA8888).copy();
	}
	return result;

	//if (image.isNull())
	//{
	//	return QImage();
	//}
	//QImage qImage = image;
	//if (qImage.format() != QImage::Format_ARGB32&& qImage.format() != QImage::Format_RGB32)
	//{
	//	qImage = qImage.convertToFormat(QImage::Format_ARGB32);
	//}
	////QImage转mat
	//cv::Mat mat(qImage.height(), qImage.width(), CV_8UC4, const_cast<uchar*>(qImage.constBits()), qImage.bytesPerLine());

	//if (mat.empty())
	//{
	//	return image;
	//}



	////调整图片亮度
	//cv::Mat matBGR;
	//cv::cvtColor(mat, matBGR, cv::COLOR_RGBA2BGR);


	//cv::Mat adjustedImage;
	//matBGR.convertTo(adjustedImage, -1, 1.0, intensity);

	////转换回RGBA
	//cv::Mat matRGBA;
	//cv::cvtColor(adjustedImage, matRGBA, cv::COLOR_BGR2RGBA);




	////将修改后的mat转成QImage
	//QImage result(matRGBA.data, matRGBA.cols, matRGBA.rows, matRGBA.step, QImage::Format_ARGB32);

	//return result.copy();
}

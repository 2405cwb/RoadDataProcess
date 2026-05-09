#include "hn3dImageMode.h"

hn3dImageMode::hn3dImageMode()
{
	//默认设置为灰度图模式
	this->setGrayMode();
}

hn3dImageMode::~hn3dImageMode()
{
}

void hn3dImageMode::setGrayMode()
{
	this->m_3dImageMode = ImageShowMode::Gray;
}

void hn3dImageMode::setRgbMode()
{
	this->m_3dImageMode = ImageShowMode::RGB;
}

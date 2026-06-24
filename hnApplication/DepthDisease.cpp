#include "DepthDisease.h"



DepthDisease::DepthDisease()
{
	m_name = "";
	m_grad = "";
	m_thresholdUp = 0;
	m_thresholdDown = 0;
	m_thresholdAreaUp = -1;
	m_thresholdAreaDown = -1;
}


DepthDisease::~DepthDisease()
{
}

 QString DepthDisease::getName()
{
	 return m_name;
}

 QString DepthDisease::getGrad()
 {
	 return m_grad;
 }

 int DepthDisease::getThreshold()
 {
	 return m_thresholdUp;
 }

void DepthDisease::setName(QString name)
{
	m_name = name;
}

void DepthDisease::setGrad(QString grad)
{
	m_grad = grad;
}

void DepthDisease::setThreshold(int value)
{
	this->m_thresholdUp = value;
}

QString DepthDisease::getFrameType() const
{
	return m_frameType;
}

void DepthDisease::setFrameType(const QString & frameType)
{
	m_frameType = frameType;
}

#include "hnFrameMode.h"

hnFrameMode::hnFrameMode()
{
	m_frameMode = FrameMode::BIG_FRAME;
}

hnFrameMode::~hnFrameMode()
{
}

void hnFrameMode::initFrameMode()
{
	if (nullptr == hnDataManager::getDataManager()->getCurrentProject())
	{
		return;
	}

	//获取绘制方式
	const int drawType = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType;
	
	if (0 == drawType)
	{ 
		m_frameMode = BIG_FRAME;
		m_drawType = 0;
	}
	else if (1 == drawType)
	{
		m_frameMode = LITTLE_FRAME;
		m_drawType = 1;
	}
	else if (2 == drawType)
	{
		//设计模式，默认为面状模式
		m_frameMode = hnFrameMode::DESIGN_FACETS;
		m_drawType = 2;
	}
	else
	{
		m_frameMode = LITTLE_FRAME;
	}
}

void hnFrameMode::setBigFrameMode()
{
	this->m_frameMode = FrameMode::BIG_FRAME;
	m_drawType = 0;
}

void hnFrameMode::setLittleFrameMode()
{
	this->m_frameMode = FrameMode::LITTLE_FRAME;
	m_drawType = 1;
}

void hnFrameMode::setDesignFacetsMode()
{
	this->m_frameMode = FrameMode::DESIGN_FACETS;
	m_drawType = 2;
}

void hnFrameMode::setDesignLineMode()
{
	this->m_frameMode = FrameMode::DESIGN_LINE;
	m_drawType = 3;
}

#pragma once
#include "hnapplication_global.h"
#include "hnDataManager.h"
#include "../hnProject/hnProject.h"
using namespace hnApp;

class HNAPPLICATION_EXPORT hnFrameMode
{
protected:
	enum FrameMode
	{
		BIG_FRAME,		//人工模式
		LITTLE_FRAME,	//自动化模式
		//LITTLE_FRAME_DOUBLE_CLICK,	//自动化模式_双击绘制病害
		DESIGN_FACETS,	//设计模式 面状
		DESIGN_LINE		//设计模式 线状
	};
public:
	hnFrameMode();
	~hnFrameMode();

protected:
	//初始化框选模式
	void initFrameMode();

public:
	//设置人工模式模式
	void setBigFrameMode();

	//设置自动化模式模式
	void setLittleFrameMode();

	//设置面状模式
	void setDesignFacetsMode();

	//设置线状模式
	void setDesignLineMode();

protected:
	FrameMode m_frameMode;

	int m_drawType;
};

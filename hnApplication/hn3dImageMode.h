#pragma once

#include <QObject>
#include "hnapplication_global.h"

//3d显示模式类
class HNAPPLICATION_EXPORT hn3dImageMode 
{
protected:
	enum ImageShowMode
	{
		Gray,	//灰度图
		RGB		//深度图
	};
public:
	hn3dImageMode();
	~hn3dImageMode();

	//设置灰度图模式
	void setGrayMode();

	//设置深度图模式
	void setRgbMode();

protected:
	ImageShowMode m_3dImageMode;
};

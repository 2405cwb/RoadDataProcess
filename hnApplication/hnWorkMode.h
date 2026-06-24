#pragma once
#include "hnapplication_global.h"


//工作模式
class HNAPPLICATION_EXPORT hnWorkMode
{
public:
	//工作模式
	enum WorkMode
	{
		NO_MODE,		//无工作模式
		ADD_MODE,		//添加病害
		DELETE_MODE,	//删除病害
		EDIT_MODE,		//编辑病害
		MOVE,			//移动病害
		MERGE,			//合并
		GET_MILE,		//获取里程，用于二三维开始里程差的校准
		ADD_CTRL_POINT	//添加控制点
	};
public:
	hnWorkMode();
	~hnWorkMode();

public:
	//设置添加病害模式
	void setAddDiseaseMode();

	//设置删除病害模式
	void setDeleteDiseaseMode();

	//设置编辑病害模式
	void setEditMode();

	//设置移动病害模式
	void setMoveMode();

	// 设置添加控制点模式
	void setAddCtrlPointMode();

	//设置模式
	void setMode(WorkMode mode);

	//获取模式
	WorkMode getMode();

protected:
	//工作模式
	WorkMode m_workMode;

	//自动化模式病害绘制 true:以人工模式大面积绘制自动化模式   false:单个自动化模式绘制
	bool littleDrawRectType;

	//线状病害添加模式  true:单击左键增加折线端点	false:自动跟踪鼠标轨迹
	static bool addLineDiseType;
};

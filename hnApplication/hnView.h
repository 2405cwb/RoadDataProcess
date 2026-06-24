#pragma once
#include <QWidget>
#include <QMainWindow>
#include "..\hnCommon\hnRoadTypeDef.h"
#include <windows.h>
#include "hnapplication_global.h"
using namespace hnCommon;

class HNAPPLICATION_EXPORT hnView : public QWidget
{
	Q_OBJECT

public:
	hnView(QWidget *parent);
	~hnView();

	// 设置视图名称
	void setViewName(QString strName);

	// 获取视图名称
	QString getViewName();

	// 刷新视图
	void refreshView();

	// 设置视图类型
	void setViewType(VIEW_TYPE nViewType) { m_nViewType = nViewType; }

	// 获取视图类型
	VIEW_TYPE getViewType();

protected:
    // 视图名称
	QString m_strViewName;

	// 视图类型
	VIEW_TYPE m_nViewType;
};

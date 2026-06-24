#include "hnView.h"

hnView::hnView(QWidget *parent)
	: QWidget(parent), m_strViewName(""),m_nViewType(VIEW_2D_CAMERA_TYPE)
{
}

hnView::~hnView()
{
}

// 设置视图名称
void hnView::setViewName(QString strName)
{
	m_strViewName = strName;
}

// 获取视图名称
QString hnView::getViewName()
{
	return m_strViewName;
}

// 刷新视图
void hnView::refreshView()
{
	update();
}

// 获取点云类型
VIEW_TYPE hnView::getViewType()
{
	return m_nViewType;
}

#include "CustomScrollBar.h"

CustomScrollBar::CustomScrollBar(QWidget *parent)
	: QScrollBar(parent)
{
	ui.setupUi(this);
	m_isLeftButtonDrag = false;
}

CustomScrollBar::~CustomScrollBar()
{
}

bool CustomScrollBar::isLeftButtonDrag()
{
	return m_isLeftButtonDrag;
}

void CustomScrollBar::mousePressEvent(QMouseEvent * event)
{
	if (event->button() == Qt::MouseButton::LeftButton)
	{
		m_isLeftButtonDrag = true;
	}
	QScrollBar::mousePressEvent(event);
}

void CustomScrollBar::mouseReleaseEvent(QMouseEvent * event)
{
	if (event->button() == Qt::MouseButton::LeftButton)
	{
		m_isLeftButtonDrag = false;
	}

	QScrollBar::mouseReleaseEvent(event);
	emit signal_leftouseRelease();
}

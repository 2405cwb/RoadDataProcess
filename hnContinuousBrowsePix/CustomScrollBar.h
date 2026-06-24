#pragma once

#include <QScrollBar>
#include "ui_CustomScrollBar.h"
#include <QDebug>
#include <QMouseEvent>

class CustomScrollBar : public QScrollBar
{
	Q_OBJECT

public:
	CustomScrollBar(QWidget *parent = Q_NULLPTR);
	~CustomScrollBar();

public:
	bool isLeftButtonDrag();

signals:
	void signal_leftouseRelease();

protected:
	void mousePressEvent(QMouseEvent *event) override final;

	void mouseReleaseEvent(QMouseEvent *event) override final;

private:
	bool m_isLeftButtonDrag;

private:
	Ui::CustomScrollBar ui;
};

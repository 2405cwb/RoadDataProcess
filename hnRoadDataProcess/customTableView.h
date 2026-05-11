#pragma once

#include <QTableView>
#include "ui_customTableView.h"
#include <QKeyEvent>

class customTableView : public QTableView
{
	Q_OBJECT

public:
	customTableView(QWidget *parent = Q_NULLPTR);
	~customTableView();

protected:
	void keyPressEvent(QKeyEvent *event);

private:
	 
};

#include "customTableView.h"

customTableView::customTableView(QWidget *parent)
	: QTableView(parent)
{
	 
	this->setContextMenuPolicy(Qt::CustomContextMenu);
}

customTableView::~customTableView()
{
}

void customTableView::keyPressEvent(QKeyEvent * event)
{
	//QTableView::keyPressEvent(event);
	event->ignore();
}

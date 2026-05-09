#include "customTableView.h"

customTableView::customTableView(QWidget *parent)
	: QTableView(parent)
{
	ui.setupUi(this);
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

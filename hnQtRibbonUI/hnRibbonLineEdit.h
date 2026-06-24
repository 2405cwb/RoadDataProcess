#ifndef HNRIBBONLINEEDIT_H
#define HNRIBBONLINEEDIT_H
#include "hnQTRibbonGlobal.h"
#include <QLineEdit>



class HN_QTRIBBON_EXPORT hnRibbonLineEdit : public QLineEdit
{
    Q_OBJECT
public:
	hnRibbonLineEdit(QWidget *parent = Q_NULLPTR);
};

#endif // HNRIBBONLINEEDIT_H

#ifndef HNRIBBONCHECKBOX_H
#define HNRIBBONCHECKBOX_H
#include "hnQTRibbonGlobal.h"
#include <QCheckBox>
class HN_QTRIBBON_EXPORT hnRibbonCheckBox : public QCheckBox
{
    Q_OBJECT
public:
	hnRibbonCheckBox(QWidget *parent = Q_NULLPTR);
};

#endif // HNRIBBONCHECKBOX_H

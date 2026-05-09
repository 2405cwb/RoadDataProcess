#ifndef HNRIBBONPANNELOPTIONBUTTON_H
#define HNRIBBONPANNELOPTIONBUTTON_H
#include <QToolButton>
#include "hnQTRibbonGlobal.h"
class QAction;
class HN_QTRIBBON_EXPORT hnRibbonPannelOptionButton : public QToolButton
{
    Q_OBJECT
public:
	hnRibbonPannelOptionButton(QWidget *parent = Q_NULLPTR);
    //有别于setDefaultAction 此函数只关联action的响应，而不设置text，icon等
    void connectAction(QAction* action);
};

#endif // HNROBBONPANNELOPTIONBUTTON_H

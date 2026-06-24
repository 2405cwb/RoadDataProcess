#include "hnRibbonPannelOptionButton.h"
#include <QAction>
hnRibbonPannelOptionButton::hnRibbonPannelOptionButton(QWidget *parent)
    :QToolButton(parent)
{
    setAutoRaise(true);
    setCheckable(false);
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setFixedSize(16,16);
    setIconSize(QSize(10,10));
    setIcon(QIcon(":/image/resource/ribbonPannelOptionButton.png"));
}

void hnRibbonPannelOptionButton::connectAction(QAction *action)
{
    connect(this,&hnRibbonPannelOptionButton::clicked
            ,action,&QAction::toggle);
}

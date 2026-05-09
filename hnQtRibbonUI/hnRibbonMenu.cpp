#include "hnRibbonMenu.h"
#include <QWidgetAction>
hnRibbonMenu::hnRibbonMenu(QWidget *parent):QMenu(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

hnRibbonMenu::hnRibbonMenu(const QString &title, QWidget *parent):QMenu(title,parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QAction *hnRibbonMenu::addRibbonMenu(hnRibbonMenu *menu)
{
    return QMenu::addMenu(menu);
}

hnRibbonMenu *hnRibbonMenu::addRibbonMenu(const QString &title)
{
	hnRibbonMenu* menu = new hnRibbonMenu(title,this);
    return menu;
}

hnRibbonMenu *hnRibbonMenu::addRibbonMenu(const QIcon &icon, const QString &title)
{
	hnRibbonMenu* menu = new hnRibbonMenu(title,this);
    menu->setIcon(icon);
    return menu;
}

QAction* hnRibbonMenu::addWidget(QWidget *w)
{
    QWidgetAction* action = new QWidgetAction(this);
    action->setDefaultWidget(w);
    addAction(action);
    return action;
}

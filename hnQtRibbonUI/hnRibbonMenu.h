#ifndef HNRIBBONMENU_H
#define HNRIBBONMENU_H
#include "hnQTRibbonGlobal.h"
#include <QMenu>
///
/// \brief 用在ribbon的menu
///
class HN_QTRIBBON_EXPORT hnRibbonMenu : public QMenu
{
    Q_OBJECT
public:
    explicit hnRibbonMenu(QWidget *parent = Q_NULLPTR);
    explicit hnRibbonMenu(const QString &title, QWidget *parent = Q_NULLPTR);
    QAction *addRibbonMenu(hnRibbonMenu *menu);
	hnRibbonMenu *addRibbonMenu(const QString &title);
	hnRibbonMenu *addRibbonMenu(const QIcon &icon, const QString &title);
    QAction *addWidget(QWidget* w);
};

#endif // HNRIBBONMENU_H

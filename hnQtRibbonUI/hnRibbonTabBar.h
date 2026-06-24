#ifndef HNRIBBONTABBAR_H
#define HNRIBBONTABBAR_H
#include "hnQTRibbonGlobal.h"
#include <QTabBar>
#include <QMargins>
class HN_QTRIBBON_EXPORT hnRibbonTabBar : public QTabBar
{
    Q_OBJECT
public:
	hnRibbonTabBar(QWidget *parent = Q_NULLPTR);
    QMargins tabMargin() const;
    void setTabMargin(const QMargins &tabMargin);

private:
    QMargins m_tabMargin;
};

#endif // HNRIBBONTABBAR_H

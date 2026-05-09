#include "hnRibbonTabBar.h"

hnRibbonTabBar::hnRibbonTabBar(QWidget *parent):QTabBar(parent)
  ,m_tabMargin(6,0,0,0)
{
    setExpanding(false);
}

QMargins hnRibbonTabBar::tabMargin() const
{
    return m_tabMargin;
}

void hnRibbonTabBar::setTabMargin(const QMargins &tabMargin)
{
    m_tabMargin = tabMargin;
}

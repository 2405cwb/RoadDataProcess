#include "hnRibbonQuickAccessBar.h"
#include "hnRibbonButtonGroupWidget.h"
#include "hnRibbonSeparatorWidget.h"
class hnRibbonQuickAccessBarPrivate
{
public:
    hnRibbonButtonGroupWidget* groupWidget;
};

hnRibbonQuickAccessBar::hnRibbonQuickAccessBar(QWidget *parent)
    :hnRibbonCtrlContainer(nullptr,parent)
    ,m_d(new hnRibbonQuickAccessBarPrivate)
{
    m_d->groupWidget = new hnRibbonButtonGroupWidget(this);
    setContainerWidget(m_d->groupWidget);
}

hnRibbonQuickAccessBar::~hnRibbonQuickAccessBar()
{
    delete m_d;
}

void hnRibbonQuickAccessBar::addSeparator()
{
    hnRibbonSeparatorWidget* w = new hnRibbonSeparatorWidget(this);
    m_d->groupWidget->addWidget(w);
}

hnRibbonToolButton* hnRibbonQuickAccessBar::addButton(QAction *act)
{
    return m_d->groupWidget->addButton(act);
}

void hnRibbonQuickAccessBar::addWidget(QWidget *w)
{
    m_d->groupWidget->addWidget(w);
}

void hnRibbonQuickAccessBar::initStyleOption(QStyleOption *opt)
{
    opt->initFrom(this);
}

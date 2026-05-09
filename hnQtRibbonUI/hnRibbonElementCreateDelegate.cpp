#include "hnRibbonElementCreateDelegate.h"
#include "hnRibbonBar.h"
#include "hnRibbonApplicationButton.h"
#include "hnRibbonTabBar.h"
#include "hnRibbonCategory.h"
#include "hnRibbonContextCategory.h"
#include "hnRibbonPannel.h"
#include "hnRibbonSeparatorWidget.h"
#include "hnRibbonGallery.h"
#include "hnRibbonGalleryGroup.h"
#include "hnRibbonToolButton.h"
#include "hnRibbonControlButton.h"
#include "hnRibbonButtonGroupWidget.h"
#include "hnRibbonStackedWidget.h"
#include "hnRibbonQuickAccessBar.h"
hnRibbonElementCreateDelegate::hnRibbonElementCreateDelegate()
{

}

hnRibbonElementCreateDelegate::~hnRibbonElementCreateDelegate()
{

}

hnRibbonTabBar *hnRibbonElementCreateDelegate::createRibbonTabBar(QWidget* parent)
{
    return new hnRibbonTabBar(parent);
}

hnRibbonApplicationButton *hnRibbonElementCreateDelegate::createRibbonApplicationButton(QWidget* parent)
{
    return new hnRibbonApplicationButton(parent);
}

hnRibbonCategory *hnRibbonElementCreateDelegate::createRibbonCategory(QWidget* parent)
{
    return new hnRibbonCategory(parent);
}

hnRibbonContextCategory *hnRibbonElementCreateDelegate::createRibbonContextCategory(QWidget *parent)
{
    return new hnRibbonContextCategory(parent);
}

hnRibbonPannel *hnRibbonElementCreateDelegate::createRibbonPannel(QWidget *parent)
{
    return new hnRibbonPannel(parent);
}

hnRibbonSeparatorWidget *hnRibbonElementCreateDelegate::createRibbonSeparatorWidget(int value,QWidget *parent)
{
    return new hnRibbonSeparatorWidget(value,parent);
}

hnRibbonGallery *hnRibbonElementCreateDelegate::createRibbonGallery(QWidget *parent)
{
    return new hnRibbonGallery(parent);
}

hnRibbonGalleryGroup *hnRibbonElementCreateDelegate::createRibbonGalleryGroup(QWidget *parent)
{
    return new hnRibbonGalleryGroup(parent);
}

hnRibbonToolButton *hnRibbonElementCreateDelegate::createRibbonToolButton(QWidget *parent)
{
    return new hnRibbonToolButton(parent);
}

hnRibbonStackedWidget *hnRibbonElementCreateDelegate::createRibbonStackedWidget(hnRibbonBar *parent)
{
    return new hnRibbonStackedWidget(parent);
}

hnRibbonControlButton *hnRibbonElementCreateDelegate::createHidePannelButton(hnRibbonBar *parent)
{
	hnRibbonControlButton* btn = new hnRibbonControlButton(parent);
    btn->setAutoRaise(false);
    btn->setObjectName(QStringLiteral("hnRibbonBarHidePannelButton"));
    btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    btn->setFixedSize(parent->tabBarHeight()-4,parent->tabBarHeight()-4);
    return btn;
}

hnRibbonButtonGroupWidget *hnRibbonElementCreateDelegate::craeteButtonGroupWidget(QWidget *parent)
{
    return new hnRibbonButtonGroupWidget(parent);
}

hnRibbonQuickAccessBar *hnRibbonElementCreateDelegate::createQuickAccessBar(QWidget *parent)
{
    return new hnRibbonQuickAccessBar(parent);
}

#ifndef HNRIBBONELEMENTCREATEDELEGATE_H
#define HNRIBBONELEMENTCREATEDELEGATE_H
#include "hnQTRibbonGlobal.h"
class QWidget;
class hnRibbonBar;
class hnRibbonTabBar;
class hnRibbonApplicationButton;
class hnRibbonCategory;
class hnRibbonContextCategory;
class hnRibbonPannel;
class hnRibbonSeparatorWidget;
class hnRibbonGallery;
class hnRibbonGalleryGroup;
class hnRibbonToolButton;
class hnRibbonControlButton;
class hnRibbonButtonGroupWidget;
class hnRibbonStackedWidget;
class hnRibbonQuickAccessBar;
///
/// \brief hnRibbon的子元素创建的代理，hnRibbon内部创建子元素都通过hnRibbonElementCreateDelegate来创建
/// 如果有些子元素重载，如hnRibbonCategory，可以重载此类的createRibbonCategory,返回重载的类来进行重载
///
class HN_QTRIBBON_EXPORT hnRibbonElementCreateDelegate
{
public:
	hnRibbonElementCreateDelegate();
    virtual ~hnRibbonElementCreateDelegate();
    virtual hnRibbonTabBar* createRibbonTabBar(QWidget* parent);
    virtual hnRibbonApplicationButton* createRibbonApplicationButton(QWidget* parent);
    virtual hnRibbonCategory* createRibbonCategory(QWidget* parent);
    virtual hnRibbonContextCategory* createRibbonContextCategory(QWidget* parent);
    virtual hnRibbonPannel* createRibbonPannel(QWidget* parent);
    virtual hnRibbonSeparatorWidget* createRibbonSeparatorWidget(int value,QWidget* parent);
    virtual hnRibbonGallery* createRibbonGallery(QWidget* parent);
    virtual hnRibbonGalleryGroup* createRibbonGalleryGroup(QWidget* parent);
    virtual hnRibbonToolButton* createRibbonToolButton(QWidget* parent);
    virtual hnRibbonStackedWidget* createRibbonStackedWidget(hnRibbonBar* parent);
    //创建隐藏ribbon的按钮代理函数
    virtual hnRibbonControlButton* createHidePannelButton(hnRibbonBar* parent);
    virtual hnRibbonButtonGroupWidget* craeteButtonGroupWidget(QWidget* parent);
    virtual hnRibbonQuickAccessBar* createQuickAccessBar(QWidget* parent);
};

#endif // HNRIBBONELEMENTCREATEDELEGATE_H

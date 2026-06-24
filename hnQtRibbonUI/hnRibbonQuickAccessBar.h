#ifndef HNRIBBONQUICKACCESSBAR_H
#define HNRIBBONQUICKACCESSBAR_H
#include "hnQTRibbonGlobal.h"
#include "hnRibbonCtrlContainer.h"
class hnRibbonToolButton;
class hnRibbonQuickAccessBarPrivate;
///
/// \brief ribbon左上顶部的快速响应栏
///
class HN_QTRIBBON_EXPORT hnRibbonQuickAccessBar : public hnRibbonCtrlContainer
{
    Q_OBJECT
public:
	hnRibbonQuickAccessBar(QWidget *parent = 0);
    ~hnRibbonQuickAccessBar();
    void addSeparator();
	hnRibbonToolButton *addButton(QAction* act);
    void addWidget(QWidget* w);
protected:
    virtual void initStyleOption(QStyleOption* opt);
private:
	hnRibbonQuickAccessBarPrivate* m_d;
};

#endif // HNRIBBONQUICKACCESSBAR_H

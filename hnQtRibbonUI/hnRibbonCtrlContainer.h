#ifndef HNRIBBONCTROLCONTAINER_H
#define HNRIBBONCTROLCONTAINER_H
#include "hnQTRibbonGlobal.h"
#include <QWidget>
#include <QScopedPointer>
class QStyleOption;
class hnRibbonCtrlContainerPrivate;
class HN_QTRIBBON_EXPORT hnRibbonCtrlContainer : public QWidget
{
    Q_OBJECT
public:
	hnRibbonCtrlContainer(QWidget *containerWidget,QWidget *parent = Q_NULLPTR);
    ~hnRibbonCtrlContainer();
    QSize sizeHint() const Q_DECL_OVERRIDE;
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    QWidget* containerWidget();
    const QWidget* containerWidget() const;
    void setEnableShowIcon(bool b);
    void setEnableShowTitle(bool b);
protected:
    void setContainerWidget(QWidget* w);
    void paintEvent(QPaintEvent *e) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *e ) Q_DECL_OVERRIDE;
    virtual void initStyleOption(QStyleOption* opt) = 0;
private:
	hnRibbonCtrlContainerPrivate*  m_d;
};

#endif // HNRIBBONCTROLCONTAINER_H

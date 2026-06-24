#ifndef HNRIBBONCATEGORY_H
#define HNRIBBONCATEGORY_H
#include "hnQTRibbonGlobal.h"
#include <QWidget>
#include "hnRibbonPannel.h"
#include <QScopedPointer>
class hnRibbonCategoryProxyPrivate;
class hnRibbonCategoryProxy;
class QHBoxLayout;
#define NOT_USE_LAYOUT 1
///
/// \brief 一项ribbon页
///
class HN_QTRIBBON_EXPORT hnRibbonCategory : public QWidget
{
    Q_OBJECT
public:
	hnRibbonCategory(QWidget* parent);
    ~hnRibbonCategory();
	hnRibbonPannel* addPannel(const QString& title);
    void addPannel(hnRibbonPannel* pannel);
    void setBackgroundBrush(const QBrush& brush);
	hnRibbonCategoryProxy* proxy();
    void setProxy(hnRibbonCategoryProxy* proxy);
protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
private:
    QScopedPointer<hnRibbonCategoryProxy> m_proxy;
    QHBoxLayout* m_pannelLayout;
};

///
/// \brief ribbon页的代理类
/// 如果需要修改重绘hnRibbonCategory，可以通过设置hnRibbonCategory::setProxy
///
class HN_QTRIBBON_EXPORT hnRibbonCategoryProxy : public QObject
{
    Q_OBJECT
public:
	hnRibbonCategoryProxy(hnRibbonCategory* parent);
    virtual ~hnRibbonCategoryProxy();

    virtual hnRibbonPannel* addPannel(const QString& title);
    virtual void addPannel(hnRibbonPannel* pannel);
    virtual void setBackgroundBrush(const QBrush& brush);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void paintEvent(QPaintEvent *event);
	hnRibbonCategory* ribbonCategory();
#if NOT_USE_LAYOUT
    virtual void resizePannels(const QSize &categorySize);
protected:
    int buildReduceModePannel(hnRibbonPannel* realPannel, int x, int y);
    static QPoint calcPopupPannelPosition(hnRibbonCategory* category, hnRibbonPannel *pannel, int x);
#endif
private:
	hnRibbonCategoryProxyPrivate* m_d;
};

#endif // HNRIBBONCATEGORY_H

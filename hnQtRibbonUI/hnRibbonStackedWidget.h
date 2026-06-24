#ifndef HNRIBBONSTACKEDWIDGET_H
#define HNRIBBONSTACKEDWIDGET_H
#include <QStackedWidget>
#include "hnQTRibbonGlobal.h"
class hnRibbonStackedWidgetPrivate;
class HN_QTRIBBON_EXPORT hnRibbonStackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
	hnRibbonStackedWidget(QWidget* parent);
    ~hnRibbonStackedWidget();
    void setPopupMode();
    bool isPopupMode() const;
    void setNormalMode();
    bool isNormalMode() const;
    void exec();
protected:
//    void mouseReleaseEvent(QMouseEvent *e);
    void hideEvent(QHideEvent *e);
signals:
    void hidWindow();
private:
	hnRibbonStackedWidgetPrivate* m_d;
};

#endif // HNRIBBONSTACKEDWIDGET_H

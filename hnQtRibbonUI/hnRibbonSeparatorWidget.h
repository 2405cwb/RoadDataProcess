#ifndef HNRIBBONSEPARATORWIDGET_H
#define HNRIBBONSEPARATORWIDGET_H
#include "hnQTRibbonGlobal.h"
#include <QWidget>
#include <QStyleOption>
///
/// \brief 用于显示分割线
///
class HN_QTRIBBON_EXPORT hnRibbonSeparatorWidget : public QWidget
{
    Q_OBJECT
public:
	hnRibbonSeparatorWidget(int height,QWidget* parent = nullptr);
	hnRibbonSeparatorWidget(QWidget* parent = nullptr);
    virtual QSize sizeHint() const Q_DECL_OVERRIDE;
protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
};

#endif // HNRIBBONSEPARATORWIDGET_H

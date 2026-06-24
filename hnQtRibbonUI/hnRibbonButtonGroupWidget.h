#ifndef HNRIBBONBUTTONGROUPWIDGET_H
#define HNRIBBONBUTTONGROUPWIDGET_H
#include "hnQTRibbonGlobal.h"
#include "hnRibbonToolButton.h"
#include <QFrame>
#include <QAbstractButton>
class hnRibbonButtonGroupWidgetPrivate;
///
/// \brief 用于管理一组按钮的控件
///
class HN_QTRIBBON_EXPORT hnRibbonButtonGroupWidget : public QFrame
{
    Q_OBJECT
public:
    hnRibbonButtonGroupWidget(QWidget* parent=Q_NULLPTR);
    ~hnRibbonButtonGroupWidget();
    void addButton(QAbstractButton* btn);
	hnRibbonToolButton* addButton(QAction* action);
    void addWidget(QWidget* w);
    QSize sizeHint() const Q_DECL_OVERRIDE;
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
private:
	hnRibbonButtonGroupWidgetPrivate* m_d;
};

#endif // HNRIBBONBUTTONGROUPWIDGET_H

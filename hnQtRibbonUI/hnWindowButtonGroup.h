#ifndef HNWINDOWBUTTONGROUP_H
#define HNWINDOWBUTTONGROUP_H
#include "hnQTRibbonGlobal.h"
#include <QWidget>
class hnWindowButtonGroupPrivate;
///
/// \brief 窗口的最大最小化按钮
///
class HN_QTRIBBON_EXPORT hnWindowButtonGroup : public QWidget
{
    Q_OBJECT
public:
	hnWindowButtonGroup(QWidget* parent);
    ~hnWindowButtonGroup();
    void setupMinimizeButton(bool on);
    void setupMaximizeButton(bool on);
    void setupCloseButton(bool on);
    void updateWindowFlag();
protected:
    QSize sizeHint();
    virtual bool eventFilter(QObject *watched, QEvent *e);
    virtual void parentResize();
private:
    void updateMaximizeButtonIcon();
protected slots:
    Q_SLOT void closeWindow();
    Q_SLOT void minimizeWindow();
    Q_SLOT void maximizeWindow();
private:
    friend class hnWindowButtonGroupPrivate;
	hnWindowButtonGroupPrivate* m_d;
};

#endif // HNWINDOWBUTTONGROUP_H

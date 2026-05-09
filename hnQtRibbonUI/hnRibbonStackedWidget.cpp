#include "hnRibbonStackedWidget.h"
#include <QEventLoop>
#include <QMouseEvent>
class hnRibbonStackedWidgetPrivate
{
public:
	hnRibbonStackedWidget* Parent;
    QEventLoop* eventLoop;
    hnRibbonStackedWidgetPrivate(hnRibbonStackedWidget* p)
        :Parent(p)
        ,eventLoop(nullptr)
    {

    }
    void init()
    {
        //Parent->setFocusPolicy(Qt::StrongFocus);
    }
};



hnRibbonStackedWidget::hnRibbonStackedWidget(QWidget *parent)
    :QStackedWidget(parent)
    ,m_d(new hnRibbonStackedWidgetPrivate(this))
{
    m_d->init();
    setNormalMode();
}

hnRibbonStackedWidget::~hnRibbonStackedWidget()
{
    if(m_d->eventLoop)
    {
        m_d->eventLoop->exit();
    }
    delete m_d;
}

void hnRibbonStackedWidget::setPopupMode()
{
    setWindowFlags(Qt::Popup|Qt::FramelessWindowHint);
    setFrameShape(QFrame::Panel);
}

bool hnRibbonStackedWidget::isPopupMode() const
{
    return (windowFlags()&Qt::Popup);
}

void hnRibbonStackedWidget::setNormalMode()
{
    if(m_d->eventLoop)
    {
        m_d->eventLoop->exit();
        m_d->eventLoop = nullptr;
    }
    setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
    setFrameShape(QFrame::NoFrame);
}

bool hnRibbonStackedWidget::isNormalMode() const
{
    return !isPopupMode();
}

void hnRibbonStackedWidget::exec()
{
    show();
    if(!isPopupMode())
    {
        m_d->eventLoop = nullptr;
        return;
    }
    QEventLoop event;
    m_d->eventLoop = &event;
    event.exec();
    m_d->eventLoop = nullptr;
}


void hnRibbonStackedWidget::hideEvent(QHideEvent *e)
{
    if(isPopupMode())
    {
        if (m_d->eventLoop)
        {
            m_d->eventLoop->exit();
        }
    }
    setFocus();
    emit hidWindow();
    QStackedWidget::hideEvent(e);
}

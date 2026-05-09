#include "hnWindowButtonGroup.h"
#include <QPushButton>
#include <QResizeEvent>
#include <QStyle>
#include <QDebug>
class hnWindowButtonGroupPrivate
{
public:

    QPushButton* buttonClose;
    QPushButton* buttonMinimize;
    QPushButton* buttonMaximize;
	hnWindowButtonGroupPrivate()
        :buttonClose(nullptr)
        ,buttonMinimize(nullptr)
        ,buttonMaximize(nullptr)
    {

    }

    void setupMinimizeButton(hnWindowButtonGroup* par,bool on)
    {
        if(on)
        {
            if(!buttonMinimize)
            {
                buttonMinimize = new QPushButton(par);
                buttonMinimize->setObjectName(QStringLiteral("hnMinimizeWindowButton"));
                buttonMinimize->setFixedSize(30,20);
                buttonMinimize->setStyleSheet(QString("QPushButton "
                                                   "{ "
					                               " 		color:#444444;"
					                               "        border: 1px solid #416ABD;"
					                               "        border - top - left - radius: 2px;"
					                               "        border - top - right - radius: 2px;"
												   "        background-color: qlineargradient(spread : pad, x1 : 0, y1 : 0, x2 : 0, y2 : 1, stop : 0 #467FBD, stop:0.5 #2A5FAC, stop:0.51 #1A4088, stop:1 #419ACF); "
                                                   "} " 
													"QPushButton::pressed"
													"{ "
														"border - top - left - radius: 2px;"
													    "border - top - right - radius: 2px;"
													"} "

													"QPushButton::checked"
													"{ "
													    "color:#000000;"
													    "border: 1px solid #BAC9DB;"
													     "background: qlineargradient(spread : pad, x1 : 0, y1 : 0, x2 : 0, y2 : 1,stop : 0 #BEE8F5, stop:0.3 #D4E1F1,stop:0.31 #C8D8ED, stop:0.8 #D0E0F0, stop:1 #E3F4FE);"
													     "border - bottom - color: #FFFFFF;"
													"} "

													"QPushButton::hover "
													"{ "
													    "border: 1px solid #ECBC3D;"
													    "color: #000000;"
													"} "

													"QPushButton : !selected"
													"{"
														"margin - top: 0px;"
													"}"));
                QIcon icon = par->style()->standardIcon(QStyle::SP_TitleBarMinButton);
                buttonMinimize->setIcon(icon);
                par->connect(buttonMinimize,&QAbstractButton::clicked
                             ,par,&hnWindowButtonGroup::minimizeWindow);
            }
        }
        else
        {
            if(buttonMinimize)
            {
                delete buttonMinimize;
            }
        }
        updateSize(par);
    }
    void setupMaximizeButton(hnWindowButtonGroup* par,bool on)
    {
        if(on)
        {
            if(!buttonMaximize)
            {
                buttonMaximize = new QPushButton(par);
                buttonMaximize->setObjectName(QStringLiteral("hnMaximizeWindowButton"));
				/*   buttonMaximize->setFixedSize(30,30);
				   buttonMaximize->setStyleSheet(QString("QPushButton "
													  "{ "
													  "    background-color: #E3E3E5; "
													  "    border:none;"
													  "}"
													  "QPushButton:hover {background-color:#C1D1B8}"
													  "QPushButton:pressed {background-color:#A5AF9B}"
													  "QPushButton:focus{outline: none;}"));*/
				buttonMaximize->setFixedSize(30, 20);
				buttonMaximize->setStyleSheet(QString("QPushButton "
					"{ "
					" 		color:#444444;"
					"        border: 1px solid #416ABD;"
					"        border - top - left - radius: 2px;"
					"        border - top - right - radius: 2px;"
					"        background-color: qlineargradient(spread : pad, x1 : 0, y1 : 0, x2 : 0, y2 : 1, stop : 0 #467FBD, stop:0.5 #2A5FAC, stop:0.51 #1A4088, stop:1 #419ACF); "
					"} "
					"QPushButton::pressed"
					"{ "
					"border - top - left - radius: 2px;"
					"border - top - right - radius: 2px;"
					"} "

					"QPushButton::checked"
					"{ "
					"color:#000000;"
					"border: 1px solid #BAC9DB;"
					"background: qlineargradient(spread : pad, x1 : 0, y1 : 0, x2 : 0, y2 : 1,stop : 0 #BEE8F5, stop:0.3 #D4E1F1,stop:0.31 #C8D8ED, stop:0.8 #D0E0F0, stop:1 #E3F4FE);"
					"border - bottom - color: #FFFFFF;"
					"} "

					"QPushButton::hover "
					"{ "
					"border: 1px solid #ECBC3D;"
					"color: #000000;"
					"} "

					"QPushButton : !selected"
					"{"
					"margin - top: 0px;"
					"}"));
                QIcon icon = par->style()->standardIcon(QStyle::SP_TitleBarMaxButton);
                buttonMaximize->setIcon(icon);
                par->connect(buttonMaximize,&QAbstractButton::clicked
                             ,par,&hnWindowButtonGroup::maximizeWindow);
            }
        }
        else
        {
            if(buttonMaximize)
            {
                delete buttonMaximize;
            }
        }
        updateSize(par);
    }
    void setupCloseButton(hnWindowButtonGroup* par,bool on)
    {
        if(on)
        {
            if(!buttonClose)
            {
                buttonClose = new QPushButton(par);
                buttonClose->setObjectName(QStringLiteral("hnCloseWindowButton"));
                buttonClose->setFixedSize(40,30);
                //buttonClose->setFlat(true);
                par->connect(buttonClose,&QAbstractButton::clicked
                             ,par,&hnWindowButtonGroup::closeWindow);
              /*  buttonClose->setStyleSheet(QString("QPushButton "
                                                   "{ "
                                                   "    background-color: #E3E3E5; "
                                                   "    border:none;"
                                                   "}"
                                                   "QPushButton:hover {background-color:#F0604D}"
                                                   "QPushButton:pressed {background-color:#F0604D}"
                                                   "QPushButton:focus{outline: none;}"));*/
				buttonClose->setFixedSize(30, 20);
				buttonClose->setStyleSheet(QString("QPushButton "
					"{ "
					" 		color:#444444;"
					"        border: 1px solid #416ABD;"
					"        border - top - left - radius: 2px;"
					"        border - top - right - radius: 2px;"
					"        background-color: qlineargradient(spread : pad, x1 : 0, y1 : 0, x2 : 0, y2 : 1, stop : 0 #467FBD, stop:0.5 #2A5FAC, stop:0.51 #1A4088, stop:1 #419ACF); "
					"} "
					"QPushButton::pressed"
					"{ "
					"border - top - left - radius: 2px;"
					"border - top - right - radius: 2px;"
					"} "

					"QPushButton::checked"
					"{ "
					"color:#000000;"
					"border: 1px solid #BAC9DB;"
					"background: qlineargradient(spread : pad, x1 : 0, y1 : 0, x2 : 0, y2 : 1,stop : 0 #BEE8F5, stop:0.3 #D4E1F1,stop:0.31 #C8D8ED, stop:0.8 #D0E0F0, stop:1 #E3F4FE);"
					"border - bottom - color: #FFFFFF;"
					"} "

					"QPushButton::hover "
					"{ "
					"border: 1px solid #ECBC3D;"
					"color: #000000;"
					"} "

					"QPushButton : !selected"
					"{"
					"margin - top: 0px;"
					"}"));
                QIcon icon = par->style()->standardIcon(QStyle::SP_TitleBarCloseButton);
                buttonClose->setIcon(icon);
            }
        }
        else
        {
            if(buttonClose)
            {
                delete buttonClose;
            }
        }
        updateSize(par);
    }

    void updateSize(hnWindowButtonGroup* par)
    {
        int span = 0;
        if(buttonClose)
        {
            buttonClose->move(par->width()-buttonClose->width()
                              ,par->y());
            span = buttonClose->width();
        }
        if(buttonMaximize)
        {
            buttonMaximize->move(par->width()-buttonMaximize->width()-span
                              ,par->y());
            span += buttonMaximize->width();
        }
        if(buttonMinimize)
        {
            buttonMinimize->move(par->width()-buttonMinimize->width()-span
                              ,par->y());
        }
    }

    QSize sizeHint()
    {
        int width = 0;
        int height = 0;
        if(buttonClose)
        {
            width += buttonClose->width();
            height = qMax(height,buttonClose->height());
        }
        if(buttonMaximize)
        {
            width += buttonMaximize->width();
            height = qMax(height,buttonMaximize->height());
        }
        if(buttonMinimize)
        {
            width += buttonMinimize->width();
            height = qMax(height,buttonMinimize->height());
        }
        return QSize(width,height);
    }

};

hnWindowButtonGroup::hnWindowButtonGroup(QWidget *parent):QWidget(parent)
  ,m_d(new hnWindowButtonGroupPrivate)
{
    updateWindowFlag();
    if(parent)
        parent->installEventFilter(this);
}

hnWindowButtonGroup::~hnWindowButtonGroup()
{
    delete m_d;
}

void hnWindowButtonGroup::setupMinimizeButton(bool on)
{
    m_d->setupMinimizeButton(this,on);
}

void hnWindowButtonGroup::setupMaximizeButton(bool on)
{
    m_d->setupMaximizeButton(this,on);
    updateMaximizeButtonIcon();
}

void hnWindowButtonGroup::setupCloseButton(bool on)
{
    m_d->setupCloseButton(this,on);
}

void hnWindowButtonGroup::updateWindowFlag()
{
    Qt::WindowFlags flags = parentWidget()->windowFlags();

    setupMinimizeButton(flags & Qt::WindowMinimizeButtonHint);

    setupMaximizeButton(flags & Qt::WindowMaximizeButtonHint);

    setupCloseButton(flags & Qt::WindowCloseButtonHint);

}


QSize hnWindowButtonGroup::sizeHint()
{
    return m_d->sizeHint();
}

bool hnWindowButtonGroup::eventFilter(QObject *watched, QEvent *e)
{
    if(watched == parentWidget())
    {
        switch(e->type())
        {
        case QEvent::Resize:
            parentResize();
            break;
        default:
            break;
        }
    }
    return false;//不截断任何事件
}

void hnWindowButtonGroup::parentResize()
{
    QWidget* par = parentWidget();
    if(par)
    {
        QSize parSize = par->size();
        move(parSize.width() - width()-1,1);
    }
}

void hnWindowButtonGroup::updateMaximizeButtonIcon()
{
    QWidget* par = parentWidget();
    if(par)
    {
        if(par->isMaximized())
        {
            if(m_d->buttonMaximize)
            {
                m_d->buttonMaximize->setIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton));
            }
        }
        else
        {
            if(m_d->buttonMaximize)
            {
                m_d->buttonMaximize->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
            }
        }
    }
}

void hnWindowButtonGroup::closeWindow()
{
    if(parentWidget())
        parentWidget()->close();
}

void hnWindowButtonGroup::minimizeWindow()
{
    if(parentWidget())
        parentWidget()->showMinimized();
}

void hnWindowButtonGroup::maximizeWindow()
{
    QWidget* par = parentWidget();
    if(par)
    {
        if(par->isMaximized())
        {
            par->showNormal();
        }
        else
        {
            par->showMaximized();
        }
        updateMaximizeButtonIcon();
    }
}

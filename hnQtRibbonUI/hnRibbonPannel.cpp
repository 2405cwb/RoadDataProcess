#include "hnRibbonPannel.h"
#include "hnRibbonToolButton.h"
#include <QAction>
#include <QIcon>
#include <QDebug>
#include <QGridLayout>
#include <QFontMetrics>
#include <QPainter>
#include <QApplication>
#include <QDesktopWidget>
#include "hnRibbonPannelOptionButton.h"
#include "hnRibbonSeparatorWidget.h"
#include "hnRibbonGallery.h"
#include "hnRibbonElementManager.h"
#include "hnRibbonMenu.h"

class hnRibbonPannelPrivate
{
public:
	hnRibbonPannelPrivate(hnRibbonPannel* p);
	hnRibbonPannel* Parent;
    QGridLayout* m_gridLayout;
    QPoint m_nextElementPosition;
    int m_row;
	hnRibbonPannelOptionButton* m_optionActionButton;
    int m_titleOptionButtonSpace;///< 标题和项目按钮的间隔
    int m_titleHeight;///< 标题的高度
    int m_titleY;///< 标题栏的y距离
	hnRibbonToolButton* m_defaultReduceButton;///<在pannel无法显示全的时候，显示一个toolbutton用来弹出pannel
};

hnRibbonPannelPrivate::hnRibbonPannelPrivate(hnRibbonPannel *p)
    :Parent(p)
    ,m_nextElementPosition(3,3)
    ,m_row(0)
    ,m_optionActionButton(nullptr)
    ,m_titleOptionButtonSpace(6)
    ,m_titleHeight(17)
    ,m_titleY(68)
    ,m_defaultReduceButton(nullptr)
{
    m_gridLayout = new QGridLayout(Parent);
    m_gridLayout->setSpacing(1);
    m_gridLayout->setContentsMargins(3,2,3,21);
}

hnRibbonPannel::hnRibbonPannel(QWidget *parent):QWidget(parent)
  ,m_d(new hnRibbonPannelPrivate(this))
{
    setFixedHeight(85);
    setMinimumWidth(40);
    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
}

hnRibbonPannel::~hnRibbonPannel()
{
    delete m_d;
}

hnRibbonToolButton *hnRibbonPannel::addLargeAction(QAction *action)
{
	hnRibbonToolButton* btn = RibbonSubElementDelegate->createRibbonToolButton(this);
    btn->setButtonType(hnRibbonToolButton::LargeButton);
    btn->setAutoRaise(true);
    btn->setDefaultAction(action);
    QSize iconSize = maxHightIconSize(action->icon().actualSize(QSize(32,32)),32);
    btn->setIconSize(iconSize);
    if(action->menu())
        btn->setPopupMode(QToolButton::MenuButtonPopup);
    m_d->m_gridLayout->addWidget(btn,0,m_d->m_gridLayout->columnCount(),6,1);
    m_d->m_row = 0;
    addAction(action);
    return btn;
}


hnRibbonToolButton *hnRibbonPannel::addLargeToolButton(const QString& text,const QIcon& icon,QToolButton::ToolButtonPopupMode popMode)
{
	hnRibbonToolButton* btn = RibbonSubElementDelegate->createRibbonToolButton(this);
    btn->setButtonType(hnRibbonToolButton::LargeButton);
    btn->setAutoRaise(true);
    QSize iconSize = maxHightIconSize(icon.actualSize(QSize(32,32)),32);
    btn->setIconSize(iconSize);
    btn->setIcon(icon);
    btn->setPopupMode(popMode);
    btn->setText(text);
    m_d->m_gridLayout->addWidget(btn,0,m_d->m_gridLayout->columnCount(),6,1);
    m_d->m_row = 0;
    return btn;
}


hnRibbonToolButton *hnRibbonPannel::addSmallToolButton(const QString &text, const QIcon &icon, QToolButton::ToolButtonPopupMode popMode)
{
	hnRibbonToolButton* btn = RibbonSubElementDelegate->createRibbonToolButton(this);
    btn->setButtonType(hnRibbonToolButton::SmallButton);
    btn->setAutoRaise(true);
    QSize iconSize = maxHightIconSize(icon.actualSize(QSize(16,16)),16);
    btn->setIconSize(iconSize);
    btn->setIcon(icon);
    btn->setPopupMode(popMode);
    btn->setText(text);
    if(0 == m_d->m_row)
        m_d->m_gridLayout->addWidget(btn,m_d->m_row,m_d->m_gridLayout->columnCount(),2,1);
    else
        m_d->m_gridLayout->addWidget(btn,m_d->m_row,m_d->m_gridLayout->columnCount()-1,2,1);
    m_d->m_row += 2;
    if(m_d->m_row >= 5)//
        m_d->m_row = 0;
    return btn;
}

hnRibbonToolButton* hnRibbonPannel::addSmallAction(QAction *action)
{
	hnRibbonToolButton* btn = RibbonSubElementDelegate->createRibbonToolButton(this);
    btn->setButtonType(hnRibbonToolButton::SmallButton);
    btn->setAutoRaise(true);
    btn->setDefaultAction(action);
    QSize iconSize = maxHightIconSize(action->icon().actualSize(QSize(16,16)),16);
    btn->setIconSize(iconSize);
    if(action->menu())
        btn->setPopupMode(QToolButton::MenuButtonPopup);
    if(0 == m_d->m_row)
        m_d->m_gridLayout->addWidget(btn,m_d->m_row,m_d->m_gridLayout->columnCount(),2,1);
    else
        m_d->m_gridLayout->addWidget(btn,m_d->m_row,m_d->m_gridLayout->columnCount()-1,2,1);
    m_d->m_row += 2;
    if(m_d->m_row >= 5)
        m_d->m_row = 0;
    addAction(action);
    return btn;
}

hnRibbonToolButton *hnRibbonPannel::addMediumAction(QAction *action)
{
	hnRibbonToolButton* btn = RibbonSubElementDelegate->createRibbonToolButton(this);
    btn->setButtonType(hnRibbonToolButton::SmallButton);
    btn->setAutoRaise(true);
    btn->setDefaultAction(action);
    QSize iconSize = maxHightIconSize(action->icon().actualSize(QSize(16,16)),16);
    btn->setIconSize(iconSize);
    if(action->menu())
        btn->setPopupMode(QToolButton::MenuButtonPopup);
    if(0 == m_d->m_row)
    {
        m_d->m_gridLayout->addWidget(btn,m_d->m_row,m_d->m_gridLayout->columnCount(),3,1);
        m_d->m_row = 3;
    }
    else if(3 == m_d->m_row)
    {
        m_d->m_gridLayout->addWidget(btn,m_d->m_row,m_d->m_gridLayout->columnCount()-1,3,1);
        m_d->m_row += 3;
    }
    else
    {
        m_d->m_gridLayout->addWidget(btn,m_d->m_row,m_d->m_gridLayout->columnCount(),3,1);
        m_d->m_row = 0;
    }

    addAction(action);
    return btn;
}

hnRibbonToolButton *hnRibbonPannel::addLargeMenu(hnRibbonMenu *menu)
{
	hnRibbonToolButton* btn = RibbonSubElementDelegate->createRibbonToolButton(this);
    btn->setButtonType(hnRibbonToolButton::LargeButton);
    btn->setAutoRaise(true);
    if(!menu->icon().isNull())
    {
        QSize iconSize = maxHightIconSize(menu->icon().actualSize(QSize(32,32)),32);
        btn->setIconSize(iconSize);
        btn->setIcon(menu->icon());
    }
    btn->setPopupMode(QToolButton::InstantPopup);
    btn->setText(menu->title());
    btn->setMenu(menu);
    m_d->m_gridLayout->addWidget(btn,0,m_d->m_gridLayout->columnCount(),6,1);
    m_d->m_row = 0;
    return btn;
}

hnRibbonToolButton *hnRibbonPannel::addSmallMenu(hnRibbonMenu *menu)
{
	hnRibbonToolButton* btn = RibbonSubElementDelegate->createRibbonToolButton(this);
    btn->setButtonType(hnRibbonToolButton::SmallButton);
    QSize iconSize = maxHightIconSize(menu->icon().actualSize(QSize(16,16)),16);
    btn->setIconSize(iconSize);
    btn->setMenu(menu);
    btn->setPopupMode(QToolButton::InstantPopup);
    if(0 == m_d->m_row)
        m_d->m_gridLayout->addWidget(btn,m_d->m_row,m_d->m_gridLayout->columnCount(),2,1);
    else
        m_d->m_gridLayout->addWidget(btn,m_d->m_row,m_d->m_gridLayout->columnCount()-1,2,1);
    m_d->m_row += 2;
    if(m_d->m_row >= 5)
        m_d->m_row = 0;
    return btn;
}

hnRibbonToolButton *hnRibbonPannel::addLargeActionMenu(QAction *action, hnRibbonMenu *menu)
{
	hnRibbonToolButton* btn = addLargeAction(action);
    btn->setMenu(menu);
    btn->setPopupMode(QToolButton::MenuButtonPopup);
    return btn;
}

hnRibbonGallery *hnRibbonPannel::addGallery()
{
	hnRibbonGallery* gallery = RibbonSubElementDelegate->createRibbonGallery(this);
    m_d->m_gridLayout->addWidget(gallery,0,m_d->m_gridLayout->columnCount(),6,1);
    m_d->m_row = 0;
    setExpanding();
    return gallery;
}

void hnRibbonPannel::addSeparator()
{
#if 0
    QAction*action = new QAction(this);
    action->setSeparator(true);
    addAction(action);
#else
    hnRibbonSeparatorWidget* sep = RibbonSubElementDelegate->createRibbonSeparatorWidget(height() - 10,this);
    m_d->m_gridLayout->addWidget(sep,0,m_d->m_gridLayout->columnCount(),6,1);
    m_d->m_row = 0;
#endif
}

///
/// \brief 向pannel添加窗口，此窗口的所有权归SARibbonPannel所有
/// \param w
///
void hnRibbonPannel::addWidget(QWidget *w)
{
    w->setParent(this);
    int col = m_d->m_gridLayout->columnCount();
    if(0 != m_d->m_row)
    {
        col -= 1;
    }
    m_d->m_gridLayout->addWidget(w,m_d->m_row,col,2,1);
    m_d->m_row += 2;
    if(m_d->m_row >= 5)
        m_d->m_row = 0;
}

void hnRibbonPannel::addWidget(QWidget *w, int row, int rowSpan)
{
    if(this != w->parentWidget())
    {
        w->setParent(this);
    }
    int col = m_d->m_gridLayout->columnCount();
    if(0 != row && 0 != col)
    {
        col -= 1;
    }
    m_d->m_gridLayout->addWidget(w,row,col,rowSpan,1);
    m_d->m_row = row + rowSpan;
    if(m_d->m_row >= 5)
        m_d->m_row = 0;
}

void hnRibbonPannel::addWidget(QWidget *w, int row, int rowSpan, int column, int columnSpan)
{
    w->setParent(this);
    m_d->m_gridLayout->addWidget(w,row,column,rowSpan,columnSpan);
    m_d->m_row = row + rowSpan;
    if(row >= 5)
        m_d->m_row = 0;
}

int hnRibbonPannel::gridLayoutColumnCount() const
{
    return m_d->m_gridLayout->columnCount();
}

void hnRibbonPannel::addOptionAction(QAction *action)
{
    if(nullptr == action)
    {
        if(m_d->m_optionActionButton)
        {
            delete m_d->m_optionActionButton;
            m_d->m_optionActionButton = nullptr;
        }
        return;
    }
    if(nullptr == m_d->m_optionActionButton)
    {
        m_d->m_optionActionButton = new hnRibbonPannelOptionButton(this);
    }
    m_d->m_optionActionButton->connectAction(action);
    repaint();
}


QSize hnRibbonPannel::maxHightIconSize(const QSize &size, int height)
{
    if(size.height() < height)
    {
        return  (size * ((float)height/size.height()));
    }
    return size;
}

void hnRibbonPannel::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    QFont f = font();
    f.setPixelSize(13);
    p.setFont(f);
    if(m_d->m_optionActionButton)
    {
        p.drawText(0,m_d->m_titleY
                   ,width()-m_d->m_optionActionButton->width() - m_d->m_titleOptionButtonSpace
                   ,m_d->m_titleHeight,Qt::AlignCenter,windowTitle());
    }
    else
    {
        p.drawText(0,m_d->m_titleY,width(),m_d->m_titleHeight,Qt::AlignCenter,windowTitle());
    }

    QWidget::paintEvent(event);
}

QSize hnRibbonPannel::sizeHint() const
{
    QSize laySize = layout()->sizeHint();
    QFontMetrics fm = fontMetrics();
    QSize titleSize = fm.size(Qt::TextShowMnemonic,windowTitle());
    int maxWidth = laySize.width();
    if(m_d->m_defaultReduceButton)
    {
        maxWidth = qMax(laySize.width(),titleSize.width()) + m_d->m_titleOptionButtonSpace + m_d->m_defaultReduceButton->width();
    }
    else
    {
        maxWidth = qMax(laySize.width(),titleSize.width());
    }
    return QSize(maxWidth,laySize.height());
}

QSize hnRibbonPannel::minimumSizeHint() const
{
    return layout()->minimumSize();
}

void hnRibbonPannel::setReduce(bool isReduce)
{
    if(isReduce)
    {
        setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    }
    else
    {
        setWindowFlags( Qt::Widget | Qt::FramelessWindowHint);
    }
}

void hnRibbonPannel::setExpanding(bool isExpanding)
{
    setSizePolicy(isExpanding ? QSizePolicy::Expanding : QSizePolicy::Preferred
                                ,QSizePolicy::Fixed);
}

bool hnRibbonPannel::isExpanding() const
{
    QSizePolicy sp = sizePolicy();
    return sp.horizontalPolicy() == QSizePolicy::Expanding;
}




void hnRibbonPannel::resizeEvent(QResizeEvent *event)
{
    if(m_d->m_optionActionButton)
    {
        m_d->m_optionActionButton->move(width()-m_d->m_titleOptionButtonSpace/2 - m_d->m_optionActionButton->width()
                                   ,m_d->m_titleY+(m_d->m_titleHeight-m_d->m_optionActionButton->height())/2);
    }
    return QWidget::resizeEvent(event);
}




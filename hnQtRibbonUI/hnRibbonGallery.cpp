#include "hnRibbonGallery.h"
#include "hnRibbonControlButton.h"
#include <QIcon>
#include <QApplication>
#define ICON_ARROW_UP QIcon(":/image/resource/ArrowUp.png")
#define ICON_ARROW_DOWN QIcon(":/image/resource/ArrowDown.png")
#define ICON_ARROW_MORE QIcon(":/image/resource/ArrowMore.png")
#include "hnRibbonMenu.h"
#include <QResizeEvent>
#include <QDebug>
#include <QVBoxLayout>
#include <QScrollBar>
#include "hnRibbonElementManager.h"

class hnRibbonGalleryPrivate
{
public:
	hnRibbonControlButton* buttonUp;
	hnRibbonControlButton* buttonDown;
	hnRibbonControlButton* buttonMore;
	hnRibbonGallery* Parent;
#if 0
    SARibbonMenu* popupWidget;
#else
    hnRibbonGalleryViewport* popupWidget;
#endif
	hnRibbonGalleryGroup* viewportGroup;
	hnRibbonGalleryPrivate():Parent(nullptr)
    {
    }

    void init(hnRibbonGallery* parent)
    {
        Parent = parent;
        buttonUp = new hnRibbonControlButton(parent);
        buttonDown = new hnRibbonControlButton(parent);
        buttonMore = new hnRibbonControlButton(parent);
        buttonUp->setToolButtonStyle(Qt::ToolButtonIconOnly);
        buttonDown->setToolButtonStyle(Qt::ToolButtonIconOnly);
        buttonMore->setToolButtonStyle(Qt::ToolButtonIconOnly);
        buttonUp->setObjectName(QStringLiteral("hnRibbonGalleryButtonUp"));
        buttonDown->setObjectName(QStringLiteral("hnRibbonGalleryButtonDown"));
        buttonMore->setObjectName(QStringLiteral("hnRibbonGalleryButtonMore"));
        buttonUp->setFixedSize(15,20);
        buttonDown->setFixedSize(15,20);
        buttonMore->setFixedSize(15,20);
        buttonUp->setIcon(ICON_ARROW_UP);
        buttonDown->setIcon(ICON_ARROW_DOWN);
        buttonMore->setIcon(ICON_ARROW_MORE);
        Parent->connect(buttonUp,&QAbstractButton::clicked
                        ,Parent,&hnRibbonGallery::onPageUp);
        Parent->connect(buttonDown,&QAbstractButton::clicked
                        ,Parent,&hnRibbonGallery::onPageDown);
        Parent->connect(buttonMore,&QAbstractButton::clicked
                        ,Parent,&hnRibbonGallery::onShowMoreDetail);
        popupWidget = nullptr;
        viewportGroup = nullptr;
    }

    bool isValid() const
    {
        return Parent != nullptr;
    }

    void createPopupWidget()
    {
        if(nullptr == popupWidget)
        {
#if 0
            popupWidget = new SARibbonMenu(Parent);
#else
            popupWidget = new hnRibbonGalleryViewport(Parent);
#endif
        }
    }

    void setViewPort(hnRibbonGalleryGroup* v)
    {
        if(nullptr == viewportGroup)
        {
            viewportGroup = RibbonSubElementDelegate->createRibbonGalleryGroup(Parent);
        }
        viewportGroup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        viewportGroup->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        viewportGroup->setModel(v->model());
        viewportGroup->setEnableIconText(v->enableIconText());
        viewportGroup->show();
    }
};


//////////////////////////////////////////////


hnRibbonGalleryViewport::hnRibbonGalleryViewport(QWidget *parent):QWidget(parent)
{
    setWindowFlags(Qt::Popup);
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(0);
    m_layout->setMargin(1);
}

void hnRibbonGalleryViewport::addWidget(QWidget *w)
{
    m_layout->addWidget(w);
}

//////////////////////////////////////////////

hnRibbonGallery::hnRibbonGallery(QWidget *parent):QFrame(parent)
  ,m_d(new hnRibbonGalleryPrivate)
{
    m_d->init(this);
    setFrameShape(QFrame::Box);
    //setFrameShape(QFrame::Panel);
    setFixedHeight(60);
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    setMinimumWidth(88);
}

hnRibbonGallery::~hnRibbonGallery()
{
    delete m_d;
}

QSize hnRibbonGallery::sizeHint() const
{
    return QSize(232,60);
}

QSize hnRibbonGallery::minimumSizeHint() const
{
    return QSize(88,60);
}

hnRibbonGalleryGroup *hnRibbonGallery::addGalleryGroup()
{
    hnRibbonGalleryGroup* group = RibbonSubElementDelegate->createRibbonGalleryGroup(this);
    hnRibbonGalleryViewport* viewport = ensureGetPopupViewPort();
    hnRibbonGalleryGroupModel* model = new hnRibbonGalleryGroupModel(this);
    group->setModel(model);
    viewport->addWidget(group);
    if(nullptr == m_d->viewportGroup)
    {
        setCurrentViewGroup(group);
    }
    connect(group,&QAbstractItemView::clicked
            ,this,&hnRibbonGallery::onItemClicked);
    return group;
}

hnRibbonGalleryGroup* hnRibbonGallery::addCategoryActions(const QString &title, QList<QAction *> actions)
{
    hnRibbonGalleryGroup* group = RibbonSubElementDelegate->createRibbonGalleryGroup(this);
    hnRibbonGalleryGroupModel* model = new hnRibbonGalleryGroupModel(this);
    group->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);
    group->setModel(model);
    if(!title.isEmpty())
    {
        group->setGroupTitle(title);
    }
    group->addActionItemList(actions);
    connect(group,&QAbstractItemView::clicked
            ,this,&hnRibbonGallery::onItemClicked);
    hnRibbonGalleryViewport* viewport = ensureGetPopupViewPort();
    viewport->addWidget(group);
    return group;
}

void hnRibbonGallery::setCurrentViewGroup(hnRibbonGalleryGroup *group)
{
    m_d->setViewPort(group);
    QApplication::postEvent(this,new QResizeEvent(size(),size()));
}

hnRibbonGalleryGroup *hnRibbonGallery::currentViewGroup() const
{
    return m_d->viewportGroup;
}

void hnRibbonGallery::onPageDown()
{
    if(m_d->viewportGroup)
    {
        QScrollBar* vscrollBar = m_d->viewportGroup->verticalScrollBar();
        int v = vscrollBar->value();
        v += vscrollBar->singleStep();
        vscrollBar->setValue(v);
    }
}

void hnRibbonGallery::onPageUp()
{
    if(m_d->viewportGroup)
    {
        QScrollBar* vscrollBar = m_d->viewportGroup->verticalScrollBar();
        int v = vscrollBar->value();
        v -= vscrollBar->singleStep();
        vscrollBar->setValue(v);
    }
}

void hnRibbonGallery::onShowMoreDetail()
{
    if(nullptr == m_d->popupWidget)
    {
        return;
    }
    QSize popupMenuSize = m_d->popupWidget->minimumSizeHint();// sizeHint();
    QPoint start = mapToGlobal(QPoint(0,0));
    m_d->popupWidget->setGeometry(start.x(),start.y(),width(),popupMenuSize.height());
    m_d->popupWidget->show();
}

void hnRibbonGallery::onItemClicked(const QModelIndex &index)
{
    QObject* obj = sender();
    hnRibbonGalleryGroup* group = qobject_cast<hnRibbonGalleryGroup*>(obj);

    if(group)
    {
        hnRibbonGalleryGroup* curGroup = currentViewGroup();
        if(nullptr == curGroup)
        {
            setCurrentViewGroup(group);
            curGroup = currentViewGroup();
        }
        if(curGroup->model() != group->model())
        {
            curGroup->setModel(group->model());
        }
        curGroup->scrollTo(index);
        curGroup->setCurrentIndex(index);
        curGroup->repaint();
    }
}

hnRibbonGalleryViewport *hnRibbonGallery::ensureGetPopupViewPort()
{
    if(nullptr == m_d->popupWidget)
    {
        m_d->createPopupWidget();
    }
    return m_d->popupWidget;
}

void hnRibbonGallery::resizeEvent(QResizeEvent *event)
{
    if(!m_d->isValid())
    {
        return;
    }
    const QSize r = event->size();
    int subW = 0;
    m_d->buttonUp->move(r.width() - m_d->buttonUp->width(),0);
    subW = qMax(subW,m_d->buttonUp->width());
    m_d->buttonDown->move(r.width() - m_d->buttonDown->width(),m_d->buttonUp->height());
    subW = qMax(subW,m_d->buttonDown->width());
    m_d->buttonMore->move(r.width() - m_d->buttonMore->width(),m_d->buttonDown->geometry().bottom());
    subW = qMax(subW,m_d->buttonMore->width());
    if(m_d->viewportGroup)
    {
        m_d->viewportGroup->setGeometry(0,0,r.width()-subW,r.height());
    }
    QFrame::resizeEvent(event);
}

void hnRibbonGallery::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

}



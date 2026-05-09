#include "hnRibbonButtonGroupWidget.h"
#include <QHBoxLayout>
#include <QDebug>
#include <QMargins>
#include <QChildEvent>
class hnRibbonButtonGroupWidgetPrivate
{
public:
	hnRibbonButtonGroupWidget* Parent;
	hnRibbonButtonGroupWidgetPrivate(hnRibbonButtonGroupWidget* p)
    {
        Parent = p;
    }
    void init()
    {
        QHBoxLayout *layout = new QHBoxLayout;
        layout->setMargin(0);
        layout->setSpacing(0);
        Parent->setLayout(layout);
        Parent->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);
    }
};

hnRibbonButtonGroupWidget::hnRibbonButtonGroupWidget(QWidget *parent)
    :QFrame(parent)
    ,m_d(new hnRibbonButtonGroupWidgetPrivate(this))
{
    m_d->init();
}

hnRibbonButtonGroupWidget::~hnRibbonButtonGroupWidget()
{
    delete m_d;
}

void hnRibbonButtonGroupWidget::addButton(QAbstractButton *btn)
{
    btn->setParent(this);
    layout()->addWidget(btn);
    layout()->setAlignment(btn,Qt::AlignCenter);
}

hnRibbonToolButton *hnRibbonButtonGroupWidget::addButton(QAction *action)
{
    hnRibbonToolButton* btn = new hnRibbonToolButton(action,this);
    btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    btn->setFixedSize(26,26);
    layout()->addWidget(btn);
    layout()->setAlignment(btn,Qt::AlignCenter);
    return btn;
}

void hnRibbonButtonGroupWidget::addWidget(QWidget *w)
{
    w->setParent(this);
    layout()->addWidget(w);
    layout()->setAlignment(w,Qt::AlignCenter);
}

QSize hnRibbonButtonGroupWidget::sizeHint() const
{
    return layout()->sizeHint();
}

QSize hnRibbonButtonGroupWidget::minimumSizeHint() const
{
    return layout()->minimumSize();
}


#include "hnRibbonGalleryItem.h"
#include "hnRibbonGalleryGroup.h"

hnRibbonGalleryItem::hnRibbonGalleryItem()
    :m_flsgs(Qt::ItemIsEnabled|Qt::ItemIsSelectable)
    ,m_action(nullptr)
{

}

hnRibbonGalleryItem::hnRibbonGalleryItem(const QIcon &icon)
    :m_flsgs(Qt::ItemIsEnabled|Qt::ItemIsSelectable)
    ,m_action(nullptr)
{
    setIcon(icon);
}

hnRibbonGalleryItem::hnRibbonGalleryItem(QAction *act)
    :m_flsgs(Qt::ItemIsEnabled|Qt::ItemIsSelectable)
{
    setAction(act);
}

hnRibbonGalleryItem::~hnRibbonGalleryItem()
{

}

void hnRibbonGalleryItem::setData(int role, const QVariant &data)
{
    m_datas[role] = data;
}

QVariant hnRibbonGalleryItem::data(int role) const
{
    if(m_action)
    {
        switch (role) {
        case Qt::DisplayRole:
            return m_action->text();
        case Qt::ToolTipRole:
            return m_action->toolTip();
        case Qt::DecorationRole:
            return m_action->icon();
        default:
            break;
        }
    }
    return m_datas.value(role);
}

void hnRibbonGalleryItem::setText(const QString &text)
{
    setData(Qt::DisplayRole,text);
}

QString hnRibbonGalleryItem::text() const
{
    if(m_action)
    {
        return m_action->text();
    }
    return data(Qt::DisplayRole).toString();
}

void hnRibbonGalleryItem::setToolTip(const QString &text)
{
    setData(Qt::ToolTipRole, text);
}

QString hnRibbonGalleryItem::toolTip() const
{
    if(m_action)
    {
        return m_action->toolTip();
    }
    return data(Qt::ToolTipRole).toString();
}

void hnRibbonGalleryItem::setIcon(const QIcon &ico)
{
    setData(Qt::DecorationRole, ico);
}

QIcon hnRibbonGalleryItem::icon() const
{
    if(m_action)
    {
        return m_action->icon();
    }
    return qvariant_cast<QIcon>(data(Qt::DecorationRole));
}

bool hnRibbonGalleryItem::isSelectable() const
{
    return (m_flsgs & Qt::ItemIsSelectable);
}

void hnRibbonGalleryItem::setSelectable(bool isSelectable)
{
    if(isSelectable)
    {
        m_flsgs |= Qt::ItemIsSelectable;
    }
    else
    {
        m_flsgs = (m_flsgs & (~Qt::ItemIsSelectable));
    }
}

bool hnRibbonGalleryItem::isEnable() const
{
    if(m_action)
    {
        return m_action->isEnabled();
    }
    return (m_flsgs & Qt::ItemIsEnabled);
}

void hnRibbonGalleryItem::setEnable(bool isEnable)
{
    if(m_action)
    {
        m_action->setEnabled(isEnable);
    }

    if(isEnable)
    {
        m_flsgs |= Qt::ItemIsEnabled;
    }
    else
    {
        m_flsgs = (m_flsgs & (~Qt::ItemIsEnabled));
    }
}

void hnRibbonGalleryItem::setFlags(Qt::ItemFlags flag)
{
    m_flsgs = flag;
    if(m_action)
    {
        m_action->setEnabled(flag & Qt::ItemIsEnabled);
    }
}

Qt::ItemFlags hnRibbonGalleryItem::flags() const
{
    return m_flsgs;
}

void hnRibbonGalleryItem::setAction(QAction *act)
{
    m_action = act;
    if(act->isEnabled())
    {
        m_flsgs |= Qt::ItemIsEnabled;
    }
    else
    {
        m_flsgs = (m_flsgs & (~Qt::ItemIsEnabled));
    }
}

QAction *hnRibbonGalleryItem::action()
{
    return m_action;
}

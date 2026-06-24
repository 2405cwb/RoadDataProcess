#include "hnRibbonGalleryGroup.h"
#include <QPainter>
#include <QDebug>

class hnRibbonGalleryGroupPrivate
{
public:
	hnRibbonGalleryGroup* Parent;
    bool enableIconText;
    QString groupTitle;
    hnRibbonGalleryGroupPrivate(hnRibbonGalleryGroup* p)
        :Parent(p)
        ,enableIconText(false)
    {

    }
};


////////////////////////////////////////

hnRibbonGalleryGroupItemDelegate::hnRibbonGalleryGroupItemDelegate(hnRibbonGalleryGroup *group, QObject *parent)
    :QStyledItemDelegate(parent)
    ,m_group(group)
{

}

void hnRibbonGalleryGroupItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(nullptr == m_group)
    {
        return;
    }
    if(m_group->enableIconText())
    {
        paintIconWithText(painter,option,index);
    }
    else
    {
        paintIconOnly(painter,option,index);
    }
}

void hnRibbonGalleryGroupItemDelegate::paintIconOnly(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyle* style = m_group->style();
    painter->save();
    painter->setClipRect(option.rect);
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, m_group);
    // draw the icon
    QRect iconRect=option.rect;
    iconRect.adjust(3,3,-3,-3);
    QIcon ico = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    ico.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::On);
    painter->restore();
}

void hnRibbonGalleryGroupItemDelegate::paintIconWithText(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter,option,index);
}

QSize hnRibbonGalleryGroupItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //option.rect对应grid size
    Q_UNUSED(index);
    return QSize(option.rect.width(),option.rect.height());
}

//////////////////////////////////////////

hnRibbonGalleryGroupModel::hnRibbonGalleryGroupModel(QObject *parent):QAbstractListModel(parent)
{

}

hnRibbonGalleryGroupModel::~hnRibbonGalleryGroupModel()
{
    clear();
}

int hnRibbonGalleryGroupModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

Qt::ItemFlags hnRibbonGalleryGroupModel::flags(const QModelIndex &index) const
{
    if(!index.isValid()
            ||
            index.row()>=m_items.size())
        return Qt::NoItemFlags;
    return m_items.at(index.row())->flags();
}

QVariant hnRibbonGalleryGroupModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.count())
            return QVariant();
    return m_items.at(index.row())->data(role);
}

QModelIndex hnRibbonGalleryGroupModel::index(int row, int column, const QModelIndex &parent) const
{
    if (hasIndex(row, column, parent))
        return createIndex(row, column, m_items.at(row));
    return QModelIndex();
}

bool hnRibbonGalleryGroupModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_items.count())
        return false;

    m_items.at(index.row())->setData(role, value);
    return true;
}

void hnRibbonGalleryGroupModel::clear()
{
    beginResetModel();
    for (int i = 0; i < m_items.count(); ++i) {
        if (m_items.at(i))
        {
            delete m_items.at(i);
        }
    }
    m_items.clear();
    endResetModel();
}




hnRibbonGalleryItem *hnRibbonGalleryGroupModel::at(int row) const
{
    return m_items.value(row);
}

void hnRibbonGalleryGroupModel::insert(int row, hnRibbonGalleryItem *item)
{
    beginInsertRows(QModelIndex(), row, row);
    m_items.insert(row, item);
    endInsertRows();
}

hnRibbonGalleryItem *hnRibbonGalleryGroupModel::take(int row)
{
    if (row < 0 || row >= m_items.count())
        return 0;

    beginRemoveRows(QModelIndex(), row, row);
	hnRibbonGalleryItem *item = m_items.takeAt(row);
    endRemoveRows();
    return item;
}

void hnRibbonGalleryGroupModel::append(hnRibbonGalleryItem *item)
{
    beginInsertRows(QModelIndex(), m_items.count(), m_items.count()+1);
    m_items.append(item);
    endInsertRows();
}

//////////////////////////
/// \brief SARibbonGalleryGroup::SARibbonGalleryGroup
/// \param w
///////////////////////

hnRibbonGalleryGroup::hnRibbonGalleryGroup(QWidget *w):QListView(w)
  ,m_d(new hnRibbonGalleryGroupPrivate(this))
{
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setSelectionRectVisible(true);
    setUniformItemSizes(true);
    setPreinstallStyle(LargeIconWithText);
    setItemDelegate(new hnRibbonGalleryGroupItemDelegate(this,this));

    connect(this,&QAbstractItemView::clicked
            ,this,&hnRibbonGalleryGroup::onItemClicked);


}

hnRibbonGalleryGroup::~hnRibbonGalleryGroup()
{
    delete m_d;
}
///
/// \brief 设置默认的预设样式
/// \param style
///
void hnRibbonGalleryGroup::setPreinstallStyle(hnRibbonGalleryGroup::PreinstallStyle style)
{
    switch(style)
    {
    case LargeIconWithText:
    {
        setIconSize(QSize(72,36));
        setGridSize(QSize(72,56));
        setEnableIconText(true);
        break;
    }
    case LargeIconOnly:
    {
        setIconSize(QSize(72,56));
        setGridSize(QSize(72,56));
        setEnableIconText(false);
        break;
    }
    default:
    {
        setIconSize(QSize(72,36));
        setGridSize(QSize(72,56));
        setEnableIconText(true);
    }
    }
}

void hnRibbonGalleryGroup::addItem(const QIcon& icon)
{
    if(nullptr == groupModel())
    {
        return;
    }
    addItem(new hnRibbonGalleryItem(icon));
}

void hnRibbonGalleryGroup::addItem(hnRibbonGalleryItem *item)
{
    if(nullptr == groupModel())
    {
        return;
    }
    groupModel()->append(item);
}

void hnRibbonGalleryGroup::addActionItem(QAction *act)
{
    if(nullptr == groupModel())
    {
        return;
    }
    groupModel()->append(new hnRibbonGalleryItem(act));
}

void hnRibbonGalleryGroup::addActionItemList(const QList<QAction *> &acts)
{
    hnRibbonGalleryGroupModel* model = groupModel();
    if(nullptr == model)
    {
        return;
    }
    for(int i=0;i<acts.size();++i)
    {
        model->append(new hnRibbonGalleryItem(acts[i]));
    }
}
///
/// \brief 构建一个model，这个model的父类是SARibbonGalleryGroup，如果要共享model，需要手动处理model的父类
///
void hnRibbonGalleryGroup::setupGroupModel()
{
    setModel(new hnRibbonGalleryGroupModel(this));
}

hnRibbonGalleryGroupModel *hnRibbonGalleryGroup::groupModel()
{
    return qobject_cast<hnRibbonGalleryGroupModel*>(model());
}

void hnRibbonGalleryGroup::setEnableIconText(bool enable)
{
    m_d->enableIconText = enable;
}

bool hnRibbonGalleryGroup::enableIconText() const
{
    return m_d->enableIconText;
}

void hnRibbonGalleryGroup::setGroupTitle(const QString &title)
{
    m_d->groupTitle = title;
    emit groupTitleChanged(m_d->groupTitle);
}

QString hnRibbonGalleryGroup::groupTitle() const
{
    return m_d->groupTitle;
}

void hnRibbonGalleryGroup::onItemClicked(const QModelIndex &index)
{
    if(index.isValid())
    {
        hnRibbonGalleryItem* item = (hnRibbonGalleryItem*)index.internalPointer();
        if(item)
        {
            QAction* act = item->action();
            if(act)
            {
                act->activate(QAction::Trigger);
            }
        }
    }
}






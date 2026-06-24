#ifndef HNRIBBONGALLERYGROUP_H
#define HNRIBBONGALLERYGROUP_H
#include "hnQTRibbonGlobal.h"
#include <QList>
#include "hnRibbonGalleryItem.h"
#include <QListView>
#include <QStyledItemDelegate>
#include "hnRibbonGalleryItem.h"

///
/// \brief SARibbonGalleryGroup对应的显示代理
///
class HN_QTRIBBON_EXPORT hnRibbonGalleryGroupItemDelegate : public QStyledItemDelegate
{
public:
	hnRibbonGalleryGroupItemDelegate(hnRibbonGalleryGroup* group,QObject *parent = Q_NULLPTR);
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    virtual void paintIconOnly(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual void paintIconWithText(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
private:
	hnRibbonGalleryGroup* m_group;
};

///
/// \brief hnRibbonGalleryGroup对应的model
///
class HN_QTRIBBON_EXPORT hnRibbonGalleryGroupModel : public QAbstractListModel
{
    Q_OBJECT
public:
	hnRibbonGalleryGroupModel(QObject *parent = Q_NULLPTR);
    ~hnRibbonGalleryGroupModel();
    virtual int rowCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) Q_DECL_OVERRIDE;
    void clear();
	hnRibbonGalleryItem * at(int row) const;
    void insert(int row, hnRibbonGalleryItem *item);
	hnRibbonGalleryItem * take(int row);
    void append(hnRibbonGalleryItem *item);
private:
    QList<hnRibbonGalleryItem*> m_items;
};



class hnRibbonGalleryGroupPrivate;
///
/// \brief Gallery的组
///
/// 组负责显示管理Gallery Item，
///

class HN_QTRIBBON_EXPORT hnRibbonGalleryGroup : public QListView
{
    Q_OBJECT
    Q_PROPERTY(bool enableIconText READ enableIconText WRITE setEnableIconText)
public:
    ///
    /// \brief 预设样式
    ///
    enum PreinstallStyle
    {
        LargeIconWithText ///< 大图标带文字
        ,LargeIconOnly

    };

	hnRibbonGalleryGroup(QWidget* w = 0);

    virtual ~hnRibbonGalleryGroup();
    void setPreinstallStyle(PreinstallStyle style);
    void addItem(const QIcon& icon);
    void addItem(hnRibbonGalleryItem *item);
    void addActionItem(QAction* act);
    void addActionItemList(const QList<QAction*>& acts);
    //构建一个model，这个model的父类是SARibbonGalleryGroup，如果要共享model，需要手动处理model的父类
    void setupGroupModel();
	hnRibbonGalleryGroupModel* groupModel();
    //是否在Gallery的图标下显示文字
    void setEnableIconText(bool enable);
    bool enableIconText() const;
    void setGroupTitle(const QString& title);
    QString groupTitle() const;
private slots:
    void onItemClicked(const QModelIndex &index);
signals:
    void groupTitleChanged(const QString& title);
private:
	hnRibbonGalleryGroupPrivate* m_d;
};


#endif // HNRIBBONGALLERYGROUP_H

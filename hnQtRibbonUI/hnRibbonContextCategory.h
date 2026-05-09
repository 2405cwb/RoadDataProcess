#ifndef HNRIBBONCONTEXTCATEGORY_H
#define HNRIBBONCONTEXTCATEGORY_H
#include "hnQTRibbonGlobal.h"
#include "hnRibbonCategory.h"
#include <QWidget>
class hnRibbonContextCategoryPrivate;
///
/// \brief 管理上下文标签的类
///
class HN_QTRIBBON_EXPORT hnRibbonContextCategory : public QObject
{
    Q_OBJECT
public:
	hnRibbonContextCategory(QWidget* parent = 0);
    ~hnRibbonContextCategory();
    //上下文目录添加下属目录
	hnRibbonCategory* addCategoryPage(const QString& title);
    //获取上下文标签下管理的标签个数
    int categoryCount() const;
    //设置id
    void setId(const QVariant& id);
    QVariant id() const;
    //设置上下文颜色
    void setContextColor(const QColor color);
    QColor contextColor() const;
    //上下文标签的内容
    QString contextTitle() const;
    void setContextTitle(const QString &contextTitle);
    //获取对应的tab页
	hnRibbonCategory* categoryPage(int index);
signals:
    void categoryPageAdded(hnRibbonCategory* category);
protected:
    QWidget* parentWidget() const;
private:
	hnRibbonContextCategoryPrivate* m_d;
};

#endif // HNRIBBONCONTEXTCATEGORY_H

#include "hnRibbonContextCategory.h"
#include <QList>
#include <QVariant>
#include "hnRibbonElementManager.h"
class hnRibbonCategoryData
{
public:
    hnRibbonCategory* categoryPage;
};

class hnRibbonContextCategoryPrivate
{
public:
    QList<hnRibbonCategoryData> categoryDataList;
    QVariant contextID;
    QColor contextColor;
    QString contextTitle;
};

hnRibbonContextCategory::hnRibbonContextCategory(QWidget *parent):QObject(parent)
  ,m_d(new hnRibbonContextCategoryPrivate)
{
}

hnRibbonContextCategory::~hnRibbonContextCategory()
{
    delete m_d;
}

hnRibbonCategory *hnRibbonContextCategory::addCategoryPage(const QString &title)
{
    hnRibbonCategoryData catData;
    hnRibbonCategory* category = RibbonSubElementDelegate->createRibbonCategory(parentWidget());
    category->setWindowTitle(title);
    catData.categoryPage = category;
    m_d->categoryDataList.append(catData);
    emit categoryPageAdded(category);
    return category;
}

int hnRibbonContextCategory::categoryCount() const
{
    return m_d->categoryDataList.count();
}

void hnRibbonContextCategory::setId(const QVariant &id)
{
    m_d->contextID = id;
}

QVariant hnRibbonContextCategory::id() const
{
    return m_d->contextID;
}

void hnRibbonContextCategory::setContextColor(const QColor color)
{
    m_d->contextColor = color;
}

QColor hnRibbonContextCategory::contextColor() const
{
    return m_d->contextColor;
}

QWidget *hnRibbonContextCategory::parentWidget() const
{
    return qobject_cast<QWidget*>(parent());
}

QString hnRibbonContextCategory::contextTitle() const
{
    return m_d->contextTitle;
}

void hnRibbonContextCategory::setContextTitle(const QString &contextTitle)
{
    m_d->contextTitle = contextTitle;
}
///
/// \brief 获取对应的tab页
/// \param index
/// \return
///
hnRibbonCategory *hnRibbonContextCategory::categoryPage(int index)
{
    return m_d->categoryDataList[index].categoryPage;
}

#ifndef HNRIBBONCATEGORYLAYOUT_H
#define HNRIBBONCATEGORYLAYOUT_H
#include "hnQTRibbonGlobal.h"
#include <QLayout>
#include <QList>
#include <QMap>
#include "hnRibbonCategory.h"
class hnRibbonReduceActionInfo;
class hnRibbonPannel;
class HN_QTRIBBON_EXPORT hnRibbonCategoryLayout : public QLayout
{
public:
	hnRibbonCategoryLayout(hnRibbonCategory *parent);
	hnRibbonCategoryLayout();
    ~hnRibbonCategoryLayout();

	hnRibbonCategory* ribbonCategory();

    virtual void addItem(QLayoutItem *item) Q_DECL_OVERRIDE;
    virtual QLayoutItem *itemAt(int index) const Q_DECL_OVERRIDE;
    virtual QLayoutItem *takeAt(int index) Q_DECL_OVERRIDE;
    virtual int count() const Q_DECL_OVERRIDE;
    
    void setGeometry(const QRect &rect) Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;
    QSize minimumSize() const Q_DECL_OVERRIDE;
    Qt::Orientations expandingDirections() const Q_DECL_OVERRIDE;
    void invalidate() Q_DECL_OVERRIDE;
protected:
    int buildReduceModePannel(hnRibbonPannel* realPannel, int x, int y);
    static QPoint calcPopupPannelPosition(hnRibbonCategory* category, hnRibbonPannel *pannel, int x);
private:
    bool m_isChanged;
    QList<QLayoutItem *> itemList;
    QMap<hnRibbonPannel*, hnRibbonReduceActionInfo> m_pannelReduceInfo;
};

#endif // HNRIBBONCATEGORYLAYOUT_H

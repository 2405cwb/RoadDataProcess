#ifndef HNRIBBONPANNEL_H
#define HNRIBBONPANNEL_H
#include "hnQTRibbonGlobal.h"
#include <QWidget>
#include "hnRibbonToolButton.h"
class hnRibbonMenu;
class hnRibbonGallery;
class QGridLayout;
class hnRibbonPannelOptionButton;
class hnRibbonPannelPrivate;
///
/// \brief The hnRibbonPannel class
///
class HN_QTRIBBON_EXPORT hnRibbonPannel : public QWidget
{
    Q_OBJECT
public:
	hnRibbonPannel(QWidget* parent = 0);
    ~hnRibbonPannel();
    using QWidget::addAction;
	hnRibbonToolButton* addLargeToolButton(const QString& text,const QIcon& icon,QToolButton::ToolButtonPopupMode popMode);
	hnRibbonToolButton* addLargeAction(QAction *action);
	hnRibbonToolButton* addSmallToolButton(const QString& text,const QIcon& icon,QToolButton::ToolButtonPopupMode popMode);
	hnRibbonToolButton *addSmallAction(QAction *action);
	hnRibbonToolButton *addMediumAction(QAction *action);
	hnRibbonToolButton *addLargeMenu(hnRibbonMenu *menu);
	hnRibbonToolButton *addSmallMenu(hnRibbonMenu *menu);
	hnRibbonToolButton* addLargeActionMenu(QAction *action, hnRibbonMenu *menu);
	hnRibbonGallery* addGallery();
    void addSeparator();
    void addWidget(QWidget* w);
    void addWidget(QWidget* w, int row,int rowSpan);
    void addWidget(QWidget* w, int row,int rowSpan,int column,int columnSpan);
    int gridLayoutColumnCount() const;
    void addOptionAction(QAction* action);
    virtual QSize sizeHint() const Q_DECL_OVERRIDE;
    virtual QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    void setReduce(bool isReduce);
    void setExpanding(bool isExpanding = true);
    bool isExpanding() const;
protected:
    static QSize maxHightIconSize(const QSize& size,int height);
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
private:
	hnRibbonPannelPrivate* m_d;
};

#endif // HNRIBBONPANNEL_H

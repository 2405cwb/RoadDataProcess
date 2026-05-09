#ifndef HNRIBBONGALLERY_H
#define HNRIBBONGALLERY_H
#include "hnQTRibbonGlobal.h"
#include <QFrame>
#include "hnRibbonGalleryGroup.h"
class QVBoxLayout;
class hnRibbonGalleryPrivate;
class hnRibbonGalleryViewport;

///
/// \brief Gallery控件
///
class HN_QTRIBBON_EXPORT hnRibbonGallery : public QFrame
{
    Q_OBJECT
public:
	hnRibbonGallery(QWidget* parent = 0);
    virtual ~hnRibbonGallery();
    virtual QSize sizeHint() const Q_DECL_OVERRIDE;
    virtual QSize minimumSizeHint() const Q_DECL_OVERRIDE;
	hnRibbonGalleryGroup* addGalleryGroup();
	hnRibbonGalleryGroup* addCategoryActions(const QString& title,QList<QAction *> actions);
    void setCurrentViewGroup(hnRibbonGalleryGroup* group);
	hnRibbonGalleryGroup* currentViewGroup() const;
protected slots:
    virtual void onPageDown();
    virtual void onPageUp();
    virtual void onShowMoreDetail();
    void onItemClicked(const QModelIndex &index);
private:
	hnRibbonGalleryViewport* ensureGetPopupViewPort();
protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    friend class hnRibbonGalleryPrivate;
	hnRibbonGalleryPrivate* m_d;
};

///
/// \brief hnRibbonGallery的Viewport class
///
class hnRibbonGalleryViewport : public QWidget
{
    Q_OBJECT
public:
	hnRibbonGalleryViewport(QWidget* parent);
    void addWidget(QWidget* w);
private:
    QVBoxLayout* m_layout;
};

#endif // HNRIBBONGALLERY_H

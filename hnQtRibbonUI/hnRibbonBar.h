#ifndef HNRIBBONBAR_H
#define HNRIBBONBAR_H
#include "hnQTRibbonGlobal.h"
#include <QMenuBar>
#include <QVariant>
#include "hnRibbonCategory.h"
#include "hnRibbonContextCategory.h"
#include <QScopedPointer>

class hnRibbonElementCreateDelegate;
class hnRibbonBarPrivate;
class QAbstractButton;
class hnRibbonTabBar;
class hnRibbonButtonGroupWidget;
class hnRibbonQuickAccessBar;
///
/// \brief The hnRibbonBar class
///
class HN_QTRIBBON_EXPORT hnRibbonBar : public QMenuBar
{
    Q_OBJECT
public:
    enum RibbonStyle{
        OfficeStyle ///< 类似office 的ribbon风格
        ,WpsLiteStyle ///< 类似wps的紧凑风格
    };
    enum RibbonMode{
        MinimumRibbonMode ///< 缩小模式
        ,NormalRibbonMode ///< 正常模式
    };

	hnRibbonBar(QWidget* parent);

    //获取applitionButton
    QAbstractButton* applitionButton();
    //设置applitionButton
    void setApplitionButton(QAbstractButton* btn);
    //获取tabbar
	hnRibbonTabBar* ribbonTabBar();
    //添加一个标签
	hnRibbonCategory* addCategoryPage(const QString& title);
    //添加一个上下文标签
	hnRibbonContextCategory* addContextCategory(const QString& title,const QColor& color,const QVariant& id=QVariant());
    //显示一个上下文标签
    void showContextCategory(hnRibbonContextCategory* context);
    //隐藏一个上下文标签
    void hideContextCategory(hnRibbonContextCategory* context);

    //设置上下文标签的显示或隐藏
    void setContextCategoryVisible(hnRibbonContextCategory* context,bool visible);
    //设置为隐藏模式
    void setHideMode(bool isHide);
    //当前Ribbon是否是隐藏模式
    bool isRibbonBarHideMode() const;
    //设置显示隐藏ribbon按钮
    void showHideModeButton(bool isShow = true);
    //是否显示隐藏ribbon按钮
    bool isShowHideModeButton() const;
    //ribbon tab的高度
    int tabBarHeight() const;
    //
    int titleBarHeight() const;
    //激活tabbar右边的按钮群
	hnRibbonButtonGroupWidget* activeTabBarRightButtonGroup();
    //快速响应栏
	hnRibbonQuickAccessBar* quickAccessBar();
    //设置ribbon的风格
    void setRibbonStyle(RibbonStyle v);
    RibbonStyle currentRibbonStyle() const;
    //当前的模式
    RibbonMode currentRibbonMode() const;
    //获取右边不可用区域，只有在wps模式下有用
    int unusableTitleRegion() const;
    void setUnusableTitleRegion(int v);
signals:
    void applitionButtonClicked();
    //
    void currentRibbonTabChanged(int index);
protected:
    bool eventFilter(QObject *obj, QEvent *e);
protected slots:
    void onWindowTitleChanged(const QString &title);
    void onWindowIconChanged(const QIcon &icon);
    void onCategoryWindowTitleChanged(const QString &title);
    void onStackWidgetHided();
    virtual void onCurrentRibbonTabChanged(int index);
    virtual void onCurrentRibbonTabClicked(int index);
    virtual void onCurrentRibbonTabDoubleClicked(int index);
    void onContextsCategoryPageAdded(hnRibbonCategory* category);
private:
    void updateRibbonElementGeometry();
    void updateRibbonElementGeometry(RibbonStyle style);
    void resizeInNormalStyle();
    void resizeInWpsLiteStyle();
    void paintInNormalStyle();
    void paintInWpsLiteStyle();
protected:
    void paintEvent(QPaintEvent* e) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent* e) Q_DECL_OVERRIDE;

    virtual void paintBackground(QPainter& painter);
    virtual void paintWindowTitle(QPainter& painter, const QString &title, const QRect &titleRegion);
    virtual void paintWindowIcon(QPainter& painter, const QIcon &icon);
    virtual void paintContextCategoryTab(QPainter& painter,const QString& title, QRect contextRect, const QColor& color);
private:
    friend class hnRibbonBarPrivate;
	hnRibbonBarPrivate* m_d;
};



#endif // HNRIBBONBAR_H

#ifndef HNRIBBONMAINWINDOW_H
#define HNRIBBONMAINWINDOW_H
#include "hnQTRibbonGlobal.h"
#include <QMainWindow>
class hnRibbonMainWindowPrivate;
class hnRibbonBar;
class hnFramelessHelper;
class hnWindowButtonGroup;
///
/// \brief The hnRibbonMainWindow class
///
class HN_QTRIBBON_EXPORT hnRibbonMainWindow : public QMainWindow
{
    Q_OBJECT
public:
	hnRibbonMainWindow(QWidget* parent = nullptr);
    //
	
    const hnRibbonBar* ribbonBar() const;
	hnRibbonBar* ribbonBar();
    enum RibbonTheme{
        NormalTheme  ///< 普通主题

    };
    void setRibbonTheme(RibbonTheme theme);
    RibbonTheme ribbonTheme() const;
#if 0
    //重写设置stylesheet
    void setStyleSheet(const QString &styleSheet);
    ///
    /// \brief 枚举所有ribbon的元素
    ///
    enum RibbonElement
    {
        RibbonApplicationButton = 0 ///< ApplicationButton
        ,RibbonBar
        ,RibbonCategory
        ,RibbonStackedWidget
        ,RibbonTabBar
        ,RibbonToolButton
        ,RibbonMenu
        ,RibbonPannelOptionButton
        ,RibbonPannel
        ,RibbonControlButton
        ,RibbonGallery
        ,RibbonGalleryGroup
        ,RibbonComboBox
        ,RibbonLineEdit
        ,RibbonSeparatorWidget
        ,RibbonCtrlContainer
        ,RibbonQuickAccessBar
        ,RibbonButtonGroupWidget
    };
    QString ribbonElementStyleSheet(RibbonElement element) const;
    void setRibbonElementStyleSheet(RibbonElement element,const QString& styleSheet);
#endif
protected:
    void loadTheme(const QString &themeFile);
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *obj, QEvent *e);
private:
	hnRibbonMainWindowPrivate* m_d;

	hnFramelessHelper * m_pHelper;

	hnWindowButtonGroup* m_wBGroup;

};

#endif // HNRIBBONMAINWINDOW_H

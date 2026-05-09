#include "hnRibbonMainWindow.h"
#include "hnFramelessHelper.h"
#include "hnWindowButtonGroup.h"
#include "hnRibbonBar.h"
#include <QApplication>
#include <QDebug>
#include <QHash>
#include <QFile>
class hnRibbonMainWindowPrivate
{
public:
	hnRibbonMainWindowPrivate(hnRibbonMainWindow* p);
    void init();
	hnRibbonMainWindow* Parent;
    hnRibbonBar* ribbonBar;
	hnRibbonMainWindow::RibbonTheme currentRibbonTheme;
#if 0
    QHash<hnRibbonMainWindow::RibbonElement,QString> ribbonStyleSheet;
#endif
};

hnRibbonMainWindowPrivate::hnRibbonMainWindowPrivate(hnRibbonMainWindow *p)
    :Parent(p)
    ,currentRibbonTheme(hnRibbonMainWindow::NormalTheme)
{

}

void hnRibbonMainWindowPrivate::init()
{
#if 0
    ribbonStyleSheet[hnRibbonMainWindow::RibbonBar]
            = QString("hnRibbonBar{"
                      " background-color: #E3E6E8;"
                      "}"
                      );

    ribbonStyleSheet[hnRibbonMainWindow::RibbonCategory]
            = QString("hnRibbonCategory{"
                      "background-color: white;"
                      "}"
                      "hnRibbonCategory:focus{outline: none;}"
                      );
    ribbonStyleSheet[hnRibbonMainWindow::RibbonStackedWidget]
            = QString("hnRibbonStackedWidget{"
                      "background-color: white;"
                      "}"
                      "hnRibbonStackedWidget:focus{outline: none;}"
                      );
    ribbonStyleSheet[hnRibbonMainWindow::RibbonApplicationButton]
            = QString("hnRibbonApplicationButton{"
                      "color:white;  "
                      "border: 1px solid #416ABD;  "
                      "border-top-left-radius: 2px;  "
                      "border-top-right-radius: 2px;  "
                      "background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,   "
                      "        stop:0 #467FBD, stop:0.5 #2A5FAC,stop:0.51 #1A4088, stop:1 #419ACF);"
                      "}"
                      "hnRibbonApplicationButton::hover{  "
                      "    background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,   "
                      "        stop:0 #7BB2EB, stop:0.5 #477ECD,stop:0.51 #114ECF, stop:1 #80E1FF);  "
                      "}"
                      "hnRibbonApplicationButton::pressed{  "
                      "    background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,   "
                      "        stop:0 #467BBB, stop:0.5 #2F63AE,stop:0.51 #1C438A, stop:1 #358BC9);  "
                      "}  "
                      "hnRibbonApplicationButton:focus{outline: none;}"
                      );
    ribbonStyleSheet[hnRibbonMainWindow::RibbonTabBar]
            = QString(""
                      "hnRibbonTabBar{"
                      " background: transparent;"
                      "}"
                     "hnRibbonTabBar::tab "
                     "{"
                     " color:#444444;"
                     " border:none;"
                     " background: transparent;"
                     " margin-top: 0px;"
                     " margin-right: 0px;"
                     " margin-left: 6px;"
                     " margin-bottom: 0px;"
                     " min-width:60px;"
                     " max-width:200px;"
                     " min-height:30px;"
                     " max-height:30px;"
                     " padding-left:1px;"
                     " padding-right:1px;"
                     " padding-top:1px;"
                     " padding-bottom:1px;"
                     "}"
                     "hnRibbonTabBar::tab:selected, SARibbonTabBar::tab:hover "
                     "{ "
                     " border-top-left-radius: 2px;"
                     " border-top-right-radius: 2px;"
                     "}"
                     "hnRibbonTabBar::tab:selected{"
                     " color:#000000;"
                     " border: 1px solid #BAC9DB; "
                     " background: white;"
                     " border-bottom-color: #FFFFFF;"
                     "}"
                     "hnRibbonTabBar::tab:hover:!selected"
                     "{"
                     " border: 1px solid #ECBC3D;"
                     " color: #000000;"
                     "}"
                     "hnRibbonTabBar::tab:!selected "
                     "{"
                     " margin-top: 0px;"
                     "}"
                     "");
    ribbonStyleSheet[hnRibbonMainWindow::RibbonLineEdit]
            = QString(
                      "hnRibbonLineEdit {"
                      " border: 1px solid #C0C2C4;"
                      " background: #FFF;"
                      " selection-background-color: #9BBBF7;"
                      " selection-color: #000;"
                      "}"
                      );
    ribbonStyleSheet[hnRibbonMainWindow::RibbonToolButton]
            = QString(""
                      "hnRibbonToolButton{"
                      " border:none;"
                      " color:#444444;"
                      " background-color:transparent;"
                      "}"
                      "hnRibbonToolButton::pressed{"
                      " color:#444444;"
                      " border: 1px solid #FFBF3E;"
                      " background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,stop:0 #FDEDA3, stop:0.1282 #FDD36A,stop:0.8333 #FCD57C, stop:1 #FDFDEB);"
                      "}"
                      "hnRibbonToolButton::checked{"
                      " color:#444444;"
                      " border: 1px solid #f2ca58;"
                      " background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,stop:0 #FDEEB3, stop:0.1282 #FDE38A,stop:0.8333 #FCE58C, stop:1 #FDFDEB);"
                      "}"
                      "hnRibbonToolButton::hover {"
                      " color:#000000;"
                      " border: 1px solid #f2ca58;"
                      " background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,stop:0 #FDEEB3, stop:0.1282 #FDE38A,stop:0.8333 #FCE58C, stop:1 #FDFDEB);"
                      "}"
                      "");
    ribbonStyleSheet[hnRibbonMainWindow::RibbonControlButton]
            = QString(""
                      "hnRibbonControlButton{"
                      "  background-color:transparent;"
                      "  border: none;"
                      "  color:#444444;"
                       "}"
                      "hnRibbonControlButton#SARibbonGalleryButtonUp,#SARibbonGalleryButtonDown,#SARibbonGalleryButtonMore{"
                      "  border: 1px solid #C0C2C4;"
                      "}"
                      "hnRibbonControlButton#SARibbonBarHidePannelButton{"
                      "  border: none;"
                      "}"
                     "hnRibbonControlButton::pressed{"
                     "  border: 1px solid #f2ca58;"
                     "  background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,stop:0 #FDEEB3, stop:0.1282 #FDE38A,stop:0.8333 #FCE58C, stop:1 #FDFDEB);"
                     "}"
                     "hnRibbonControlButton::checked{"
                     "  border: 1px solid #f2ca58;"
                     "  background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,stop:0 #FDEEB3, stop:0.1282 #FDE38A,stop:0.8333 #FCE58C, stop:1 #FDFDEB);"
                     "}"
                     "hnRibbonControlButton::hover {"
                     "  border: 1px solid #f2ca58;"
                     "  background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,stop:0 #FDEEB3, stop:0.1282 #FDE38A,stop:0.8333 #FCE58C, stop:1 #FDFDEB);"
                     " }"
                     "");
    ribbonStyleSheet[hnRibbonMainWindow::RibbonMenu]
            = QString(
                    "hnRibbonMenu { "
                    "      color:#444444;"
                    "    background-color: #FCFCFC;  "
                    "    border: 1px solid #8492A6;  "
                    "}"
                    "hnRibbonMenu::item {  "
                    "    padding: 5px 25px 5px 25px;"
                    "    background-color: transparent;  "
                    "}"
                    "hnRibbonMenu::item:selected {   "
                    "    background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,stop:0 #FEF9F4, stop:0.38 #FDE0BD,stop:0.39 #FFCE69, stop:1 #FFFFE7);"
                    "}"
                    "hnRibbonMenu::item:hover {   "
                    "      color:#000;"
                    "    border: 1px solid #FFB700;"
                    "}  "
                    "hnRibbonMenu::icon{"
                    "margin-left: 5px;"
                    "}"
                      );
    ribbonStyleSheet[hnRibbonMainWindow::RibbonPannelOptionButton]
            = QString(""
                      "hnRibbonPannelOptionButton{"
                      " background-color:transparent;"
                      " color:#444444;"
                      "}"
                      "hnRibbonPannelOptionButton::hover {  "
                      "    background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,stop:0 #FDEEB3, stop:0.1282 #FDE38A,stop:0.8333 #FCE58C, stop:1 #FDFDEB);  "
                      "    border: 0px;"
                      "}"
                      );
    ribbonStyleSheet[hnRibbonMainWindow::RibbonPannel]
            = QString(
                      "hnRibbonPannel {  "
                      "    background-color: #FFFFFF;  "
                      "    border: 0px;"
                      "}"
                      );
    ribbonStyleSheet[hnRibbonMainWindow::RibbonGallery]
            = QString(
                      "hnRibbonGallery {  "
                      " background-color: transparent;"
                      " color: #444444;"
                      " border: 1px solid #C0C2C4;"
                      "}"
                      );
    ribbonStyleSheet[hnRibbonMainWindow::RibbonGalleryGroup]
            = QString(
                      "SARibbonGalleryGroup {  "
                      " show-decoration-selected: 1;"
                      " background-color: transparent;"
                      " color: #444444;"
                      "}"
                      "hnRibbonGalleryGroup::item:selected {  "
                      "    background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,stop:0 #FDEEB3, stop:0.1282 #FDE38A,stop:0.8333 #FCE58C, stop:1 #FDFDEB);  "
                      "}"
                      "hnRibbonGalleryGroup::item:hover {  "
                      "    border: 2px solid #FDEEB3;"
                      "}"
                      );
    ribbonStyleSheet[hnRibbonMainWindow::RibbonComboBox]
            = QString(
                      "hnRibbonComboBox {  "
                      " border: 1px solid #C0C2C4;"
                      //" background: white;"
                      "}"
                      "hnRibbonComboBox:hover{  "
                      "  border: 1px solid #FDEEB3;"
                      "  color : #000;"
                      //" background: white;"
                      "}"
                      "hnRibbonComboBox:editable {"
                      "  color : #000;"
                      "  background: white;"
                      "  selection-background-color: #9BBBF7;"
                      "  selection-color: #000;"
                      "}"
                      "hnRibbonComboBox::drop-down {"
                      "      subcontrol-origin: padding;"
                      "      subcontrol-position: top right;"
                      "      width: 15px;"
                      "      border-left: none;"
                      "      border-top-right-radius: 0px;"
                      "      border-bottom-right-radius: 0px;"
                      "}"
                      "hnRibbonComboBox::drop-down:hover {"
                      "    border: 1px solid #FDEEB3;"
                      "    background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,stop:0 #FDEEB3, stop:0.1282 #FDE38A,stop:0.8333 #FCE58C, stop:1 #FDFDEB);  "
                      "}"
                        "hnRibbonComboBox::down-arrow {"
                        "    image: url(:/image/resource/ArrowDown.png);"
                        "}"
//                    "hnRibbonComboBox QAbstractItemView {"
//                    "   border: 1px solid #C0C2C4;"
//                    "   selection-color:#000;"
//                    "   selection-background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,stop:0 #FEF9F4, stop:0.38 #FDE0BD,stop:0.39 #FFCE69, stop:1 #FFFFE7);"
//                    "}"
                      );
    ribbonStyleSheet[hnRibbonMainWindow::RibbonSeparatorWidget]
            = QString("hnRibbonSeparatorWidget{"
                      " background-color: #E3E6E8;"
                      "}"
                      );
    ribbonStyleSheet[hnRibbonMainWindow::RibbonCtrlContainer]
            = QString("SARibbonCtrlContainer{"
                      " background-color: #E3E6E8;"
                      "}"
                      );
    ribbonStyleSheet[hnRibbonMainWindow::RibbonQuickAccessBar]
            = QString("hnRibbonQuickAccessBar{"
                      " background-color: #E3E6E8;"
                      "}"
                      );
    ribbonStyleSheet[hnRibbonMainWindow::RibbonButtonGroupWidget]
            = QString("hnRibbonButtonGroupWidget{"
                      " background-color: #E3E6E8;"
                      "}"
                      );

    Parent->setStyleSheet("");
#endif
}

hnRibbonMainWindow::hnRibbonMainWindow(QWidget *parent)
    :QMainWindow(parent)
    ,m_d(new hnRibbonMainWindowPrivate(this))
{
	m_pHelper = NULL;
	m_wBGroup = NULL;
    m_d->init();
    setRibbonTheme(ribbonTheme());
    //
    m_d->ribbonBar = new hnRibbonBar(this);
    setMenuWidget(m_d->ribbonBar);
    m_d->ribbonBar->installEventFilter(this);

	if (m_wBGroup)
	{
		delete m_wBGroup;
		m_wBGroup = NULL;
	}

    //
	m_wBGroup = new hnWindowButtonGroup(this);

	if (m_pHelper)
	{
		delete m_pHelper;
		m_pHelper = NULL;
	}

	m_pHelper = new hnFramelessHelper(this);
	m_pHelper->setTitleHeight(m_d->ribbonBar->titleBarHeight());  //设置窗体的标题栏高度
}

const hnRibbonBar *hnRibbonMainWindow::ribbonBar() const
{
    return m_d->ribbonBar;
}

hnRibbonBar *hnRibbonMainWindow::ribbonBar()
{
    return m_d->ribbonBar;
}

void hnRibbonMainWindow::setRibbonTheme(hnRibbonMainWindow::RibbonTheme theme)
{
    switch(ribbonTheme())
    {
    case NormalTheme:
        loadTheme(":/resource/default.qss");
        break;
    default:
        loadTheme(":/resource/default.qss");
        break;
    }
}

hnRibbonMainWindow::RibbonTheme hnRibbonMainWindow::ribbonTheme() const
{
    return m_d->currentRibbonTheme;
}

#if 0
void hnRibbonMainWindow::setStyleSheet(const QString &styleSheet)
{
    QString s = styleSheet;
    for(auto i = m_d->ribbonStyleSheet.begin();i!=m_d->ribbonStyleSheet.end();++i)
    {
        s += i.value();
    }
    qDebug() << s;
    this->QMainWindow::setStyleSheet(s);
}
QString hnRibbonMainWindow::ribbonElementStyleSheet(hnRibbonMainWindow::RibbonElement element) const
{
    return m_d->ribbonStyleSheet.value(element);
}

void hnRibbonMainWindow::setRibbonElementStyleSheet(hnRibbonMainWindow::RibbonElement element, const QString &styleSheet)
{
    m_d->ribbonStyleSheet.insert(element,styleSheet);
}
#endif

void hnRibbonMainWindow::resizeEvent(QResizeEvent *event)
{
    if(m_d->ribbonBar)
    {
        if(m_d->ribbonBar->size().width() != this->size().width())
        {
            m_d->ribbonBar->setFixedWidth(this->size().width());
        }
    }
    QMainWindow::resizeEvent(event);
}

bool hnRibbonMainWindow::eventFilter(QObject *obj, QEvent *e)
{
    if(obj == m_d->ribbonBar)
    {
        switch (e->type())
        {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        case QEvent::Leave:
        case QEvent::HoverMove:
        case QEvent::MouseButtonDblClick:
            QApplication::sendEvent(this,e);
        default:
            break;
        }
    }
    return QMainWindow::eventFilter(obj,e);
}


void hnRibbonMainWindow::loadTheme(const QString& themeFile)
{
	QFile file(themeFile);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return;
	}

	// 设置样式
	setStyleSheet(file.readAll());

	file.close();
}

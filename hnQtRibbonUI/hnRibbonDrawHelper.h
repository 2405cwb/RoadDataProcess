#ifndef HNRIBBONDRAWHELPER_H
#define HNRIBBONDRAWHELPER_H
#include <QIcon>
#include <QStylePainter>
#include <QStyleOption>
#include <QPixmap>
#include "hnQTRibbonGlobal.h"
///
/// \brief 绘图辅助
///
class HN_QTRIBBON_EXPORT hnRibbonDrawHelper
{
public:
	hnRibbonDrawHelper();
    static QPixmap iconToPixmap(const QIcon &icon, QWidget* widget, const QStyleOption *opt, const QSize &icoSize);
    static void drawIcon(const QIcon &icon, QPainter *painter, const QStyleOption *opt
                         , int x, int y, int width, int height);
    static void drawIcon(const QIcon &icon, QPainter *painter, const QStyleOption *opt
                         , const QRect& rect);
    static QSize iconActualSize(const QIcon &icon, const QStyleOption* opt, const QSize& iconSize);

    static void drawText(const QString& text, QStylePainter *painter, const QStyleOption *opt
                         , Qt::Alignment al, int x, int y, int width, int height);
    static void drawText(const QString& text, QStylePainter *painter, const QStyleOption *opt
                         , Qt::Alignment al, const QRect& rect);
};

#endif // HNRIBBONDRAWHELPER_H

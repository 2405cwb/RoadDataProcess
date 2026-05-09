#ifndef HNRIBBONUIGLOBAL_H
#define HNRIBBONUIGLOBAL_H
#include <qglobal.h>
#define HN_QTRIBBON_UI_VERSION 1
#define HN_QTRIBBON_UI_VERSION_STR "0.1"


#ifndef HN_QTRIBBON_UI_NO_EXPORT
    #if defined(HN_QTRIBBON_UI_MAKE_LIB)     // 定义此宏将构建library
    #define HN_QTRIBBON_EXPORT Q_DECL_EXPORT
    #else
    #define HN_QTRIBBON_EXPORT Q_DECL_IMPORT
    #endif
#endif


#ifndef HN_QTRIBBON_EXPORT
#define HN_QTRIBBON_EXPORT
#endif



#endif // HNRIBBONUIGLOBAL_H

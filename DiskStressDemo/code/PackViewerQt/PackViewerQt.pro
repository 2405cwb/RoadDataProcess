QT += widgets
CONFIG += c++11

TEMPLATE = app
TARGET = PackViewerQt

SOURCES += \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    MainWindow.h \
    QtMsvcCompat.h

INCLUDEPATH += \
    ../PackSdk \
    ../PackSdk/qt

LIBS += -L../../bin/Release-x64 -lPackSdk

win32 {
    CONFIG(debug, debug|release): PACKVIEWER_OUTDIR = debug
    else: PACKVIEWER_OUTDIR = release
    QMAKE_POST_LINK += $$quote(cmd /c copy /Y ..\\..\\bin\\Release-x64\\PackSdk.dll $${PACKVIEWER_OUTDIR}\\PackSdk.dll)
}

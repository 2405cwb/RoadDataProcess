QT += core gui widgets sql
CONFIG += console c++11 release
CONFIG -= debug app_bundle
TARGET = disease-import-tests
INCLUDEPATH += ../../hnRoadDataProcess ../../hnQtCommon ../../hnCommon ../../hnDataTable ../../3rd/SQLite/include ../../QXlsx/header ../../hnConfigService
SOURCES += $$PWD/main.cpp ../../hnRoadDataProcess/hn2dDiseaseExchangeService.cpp ../../hnRoadDataProcess/hnOutExcelMile.cpp ../../hnRoadDataProcess/StreetDiseaseManage.cpp
LIBS += -L../../bin/Release-x64 -lhnApplication -lhnProject -lhnDataTable -lhnCommon -lhnConfigService -lhnQtCommon -lQXlsx

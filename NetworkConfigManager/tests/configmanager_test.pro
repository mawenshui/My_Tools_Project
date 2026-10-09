QT += core gui widgets network testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14 testcase console

TARGET = configmanager_test
TEMPLATE = app

INCLUDEPATH += $$PWD/..
DEPENDPATH += $$PWD/..

SOURCES += \
    $$PWD/configmanager_test.cpp \
    $$PWD/../configmanager.cpp \
    $$PWD/../networkinterfacemanager.cpp

HEADERS += \
    $$PWD/../configmanager.h \
    $$PWD/../networkinterfacemanager.h \
    $$PWD/../Logger.h

LIBS += -lIphlpapi -lWs2_32

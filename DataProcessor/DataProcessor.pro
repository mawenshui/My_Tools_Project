QT       += core gui network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

QMAKE_CXXFLAGS += -Wall -Wextra -Werror=return-type #函数应有返回值而没有时报编译错误
CONFIG += resources_big
CONFIG += utf8

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
TARGET = DataProcessor
TEMPLATE = app

INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/
DESTDIR = $$PWD/bin

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    configmanager.cpp \
    customdatasender.cpp \
    main.cpp \
    mainwindow.cpp \
    orderSend/ordersendwidget.cpp \
    orderSend/sendworker.cpp \
    tinyxml/tinystr.cpp \
    tinyxml/tinyxml.cpp \
    tinyxml/tinyxmlerror.cpp \
    tinyxml/tinyxmlparser.cpp \
    udpsender.cpp \
    workerclass.cpp

HEADERS += \
    alldefine.h \
    configmanager.h \
    customdatasender.h \
    mainwindow.h \
    orderSend/ordersendwidget.h \
    orderSend/sendworker.h \
    tinyxml/tinystr.h \
    tinyxml/tinyxml.h \
    udpsender.h \
    workerclass.h

FORMS += \
    mainwindow.ui \
    orderSend/ordersendwidget.ui

RESOURCES += \
    rec.qrc

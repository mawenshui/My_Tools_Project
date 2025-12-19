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

# 按debug|release模式分别设置输出目录
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/bin_debug
    # 在调试版本中添加 DEBUG 宏定义
    DEFINES += DEBUG
} else {
    DESTDIR = $$PWD/bin_release
}

# 在 Windows 下设置输出文件名包含构建类型
win32 {
    CONFIG(debug, debug|release) {
        TARGET = $$join(TARGET,,,_debug)
    } else {
        TARGET = $$join(TARGET,,,_release)
    }
}
# 如果还需要将目标文件 (.obj/.o) 也分开存放
OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.rcc
UI_DIR = $$DESTDIR/.ui

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    configmanager.cpp \
    customdatasender.cpp \
    faultAlarmWidget/faultAlarmWidget.cpp \
    main.cpp \
    mainwindow.cpp \
    threadmanager.cpp \
    orderSend/ordersendwidget.cpp \
    orderSend/sendworker.cpp \
    orderSend/multipliersettingsdialog.cpp \
    orderSend/multiplierhelpdialog.cpp \
    tinyxml/tinystr.cpp \
    tinyxml/tinyxml.cpp \
    tinyxml/tinyxmlerror.cpp \
    tinyxml/tinyxmlparser.cpp \
    udpsender.cpp \
    workerclass.cpp

HEADERS += \
    alldefine.h \
    threadmanager.h \
    configmanager.h \
    customdatasender.h \
    faultAlarmWidget/faultAlarmWidget.h \
    mainwindow.h \
    orderSend/ordersendwidget.h \
    orderSend/sendworker.h \
    orderSend/multipliersettingsdialog.h \
    orderSend/multiplierhelpdialog.h \
    tinyxml/tinystr.h \
    tinyxml/tinyxml.h \
    udpsender.h \
    workerclass.h

FORMS += \
    mainwindow.ui \
    orderSend/ordersendwidget.ui \
    orderSend/multipliersettingsdialog.ui \
    orderSend/multiplierhelpdialog.ui

RESOURCES += \
    rec.qrc

DISTFILES += \
    faultAlarmWidget/FaultAlarmWidget_Specification.md

TEMPLATE = app
QT += core gui sql charts
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
TARGET = diskMonitor

# 设置构建目录结构，避免污染源代码
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/bin
    OBJECTS_DIR = $$PWD/build/debug/obj
    MOC_DIR = $$PWD/build/debug/moc
    RCC_DIR = $$PWD/build/debug/rcc
    UI_DIR = $$PWD/build/debug/ui
} else {
    DESTDIR = $$PWD/build/release/bin
    OBJECTS_DIR = $$PWD/build/release/obj
    MOC_DIR = $$PWD/build/release/moc
    RCC_DIR = $$PWD/build/release/rcc
    UI_DIR = $$PWD/build/release/ui
}

# 确保构建目录存在
mkpath($$DESTDIR)
mkpath($$OBJECTS_DIR)
mkpath($$MOC_DIR)
mkpath($$RCC_DIR)
mkpath($$UI_DIR)

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        processmonitor.cpp \
        historymanager.cpp \
        historydialog.cpp \
        chartmanager.cpp \
        chartdialog.cpp

HEADERS += \
        mainwindow.h \
        processmonitor.h \
        historymanager.h \
        historydialog.h \
        chartmanager.h \
        chartdialog.h

FORMS += \
    mainwindow.ui

# Windows specific libraries
win32 {
    LIBS += -lpsapi -ladvapi32 -lkernel32 -luser32
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

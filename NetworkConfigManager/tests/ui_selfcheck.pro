include(../NetworkConfigManager.pro)
SOURCES = $$files($$PWD/../*.cpp)
SOURCES -= $$PWD/../main.cpp
SOURCES += $$PWD/ui_selfcheck.cpp
HEADERS = $$files($$PWD/../*.h)
FORMS = $$PWD/../mainwindow.ui
RESOURCES = $$PWD/../resources.qrc
INCLUDEPATH += $$PWD/..
QMAKE_CXXFLAGS += -I$$OUT_PWD
TARGET = ui_selfcheck
CONFIG += console
DESTDIR = $$OUT_PWD/bin

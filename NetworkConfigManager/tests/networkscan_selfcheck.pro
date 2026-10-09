QT += widgets network concurrent
CONFIG += c++14 console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = networkscan_selfcheck
SOURCES += networkscan_selfcheck.cpp ../networkscandialog.cpp
HEADERS += ../networkscandialog.h

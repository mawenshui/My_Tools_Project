// main.cpp
#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/app.ico"));
    // 设置中文语言
    QTranslator translator;
    if(translator.load("qt_zh_CN"))
    {
        a.installTranslator(&translator);
    }
    MainWindow w;
    w.show();
    return a.exec();
}

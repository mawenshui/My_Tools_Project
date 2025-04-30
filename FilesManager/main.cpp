#include "mainwindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    // 设置应用程序信息
    QApplication::setApplicationName("File Operation Tool");
    QApplication::setApplicationVersion("1.0");
    MainWindow w;
    w.show();
    return a.exec();
}

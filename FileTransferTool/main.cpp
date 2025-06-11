#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QDir>

/**
 * @brief 应用程序入口点
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 应用程序退出代码
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置应用程序信息
    a.setApplicationName("文件转移工具");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("My Tools");
    
    // 设置应用程序样式
    a.setStyle(QStyleFactory::create("Fusion"));
    
    // 创建并显示主窗口
    MainWindow w;
    w.show();
    
    return a.exec();
}

#include "mainwindow.h"
#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>

/**
 * @brief 应用程序主入口
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 应用程序退出码
 *
 * 主要功能：
 * 1. 创建QApplication实例
 * 2. 创建主窗口
 * 3. 处理启动参数
 * 4. 启动应用程序事件循环
 */
int main(int argc, char *argv[])
{
    //创建Qt应用程序对象，管理应用程序级资源
    QApplication a(argc, argv);
    //创建主窗口实例
    MainWindow w;
    //检查启动参数，如果包含"--minimized"则最小化到托盘
    //这种设计常用于开机自启动等场景
    if (a.arguments().contains("--minimized"))
    {
        //隐藏主窗口（通常配合系统托盘使用）
        w.hide();
        //可以在这里添加日志记录，记录以最小化模式启动
    }
    else
    {
        //正常显示主窗口
        w.show();
        //可以在这里添加窗口初始化后的额外操作
    }
    //启动Qt事件循环，程序将在此处保持运行直到退出
    return a.exec();
    //注：程序退出时，QApplication析构函数会自动清理资源
    //包括删除所有窗口对象和释放Qt相关资源
}

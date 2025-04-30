#include "mainwindow.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QtGlobal>
#include <QLoggingCategory>
#include <memory>

//定义日志分类
Q_LOGGING_CATEGORY(appLog, "[app.main]")

/**
 * @brief 初始化应用程序目录
 * @return 初始化成功返回true，否则返回false
 */
bool initializeAppDirectories()
{
    auto createDir = [](const QString & path) -> bool
    {
        QDir dir(path);
        if (!dir.exists() && !dir.mkpath("."))
        {
            qCCritical(appLog) << "创建目录失败:" << path;
            return false;
        }
        qCInfo(appLog) << "目录初始化完成:" << dir.absolutePath();
        return true;
    };
    //创建配置和日志目录
    const QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    const QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    if (!createDir(configPath) || !createDir(logPath))
    {
        QMessageBox::critical(nullptr, "致命错误",
                              QString("无法初始化必要的目录:\n%1\n%2").arg(configPath, logPath));
        return false;
    }
    return true;
}


/**
 * @brief 应用程序入口
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 应用程序退出码
 */
int main(int argc, char *argv[])
{
    //注册 QMap 类型，以便信号槽可以使用
    qRegisterMetaType<QMap<QString, int >> ("QMap<QString,int>");
    qRegisterMetaType<QMap<QString, double >> ("QMap<QString,double>");
    //配置应用程序信息
    QCoreApplication::setApplicationName("数据处理器");
    QCoreApplication::setOrganizationName("MaWenshui");
    QCoreApplication::setApplicationVersion("1.0.3");
    //启用高DPI缩放
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    //设置应用程序图标
    app.setWindowIcon(QIcon(":/icons/icons/app.png"));
    //设置日志格式
    qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss.zzz} %{category} %{type} %{message}");
    try
    {
        //初始化目录
        if (!initializeAppDirectories())
        {
            return EXIT_FAILURE;
        }
        MainWindow mainWindow;
        //连接应用程序退出信号到清理槽
        QObject::connect(&app, &QCoreApplication::aboutToQuit, [&mainWindow]()
        {
            mainWindow.cleanupBeforeExit();
        });
        //显示主窗口
        mainWindow.show();
        return app.exec();
    }
    catch (const std::exception &e)
    {
        qCCritical(appLog) << "应用程序初始化失败:" << e.what();
        QMessageBox::critical(nullptr, "严重错误",
                              QString("应用程序初始化失败:\n%1").arg(e.what()));
        return EXIT_FAILURE;
    }
    catch (...)
    {
        qCCritical(appLog) << "初始化过程中发生未知异常";
        QMessageBox::critical(nullptr, "严重错误",
                              "初始化过程中发生未知异常");
        return EXIT_FAILURE;
    }
}

#include "mainwindow.h"
#include <QApplication>
#include <windows.h>
#include <DbgHelp.h>
#include <QMessageBox>
#include <exception>
#include <QSharedMemory>
#include "Logger.h"

// 全局异常处理器
LONG WINAPI GlobalExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
    // 生成minidump文件
    HANDLE hDumpFile = CreateFile(L"crash.dmp", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if(hDumpFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ExceptionPointers = pExceptionInfo;
        dumpInfo.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile,
                          MiniDumpNormal, &dumpInfo, nullptr, nullptr);
        CloseHandle(hDumpFile);
    }
    // 记录异常信息
    QString errorMsg = QString("程序发生严重错误，错误代码: 0x%1").arg(pExceptionInfo->ExceptionRecord->ExceptionCode, 8, 16, QLatin1Char('0'));
    Logger::critical(errorMsg);
    // 显示错误对话框
    QMessageBox::critical(nullptr, "程序崩溃",
                          "程序发生严重错误，即将退出。\n"
                          "错误信息已记录到日志文件。\n"
                          "请将crash.dmp文件发送给开发者。");
    return EXCEPTION_EXECUTE_HANDLER;
}

// 全局C++异常处理器
void CppExceptionHandler()
{
    try
    {
        throw;
    }
    catch(const std::exception& e)
    {
        Logger::critical(QString("标准异常: %1").arg(e.what()));
    }
    catch(const QString& e)
    {
        Logger::critical(QString("Qt异常: %1").arg(e));
    }
    catch(...)
    {
        Logger::critical("未知异常");
    }
    QMessageBox::critical(nullptr, "程序异常",
                          "程序发生未处理异常，即将退出。\n"
                          "错误信息已记录到日志文件。");
    exit(1);
}


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
int main(int argc, char* argv[])
{
    // 设置Windows结构化异常处理
    SetUnhandledExceptionFilter(GlobalExceptionHandler);
    // 设置C++异常处理
    std::set_terminate(CppExceptionHandler);
    // 启用内存泄漏检测 (Debug模式下)
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    // 创建Qt应用程序对象
    QApplication a(argc, argv);
    // 配置应用程序属性
    a.setApplicationName("NetworkConfigManager");
    a.setApplicationVersion("2.1.0");
    a.setQuitOnLastWindowClosed(false);
    // 设置Qt消息处理
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext & context, const QString & msg)
    {
        switch(type)
        {
            case QtDebugMsg:
                Logger::debug(msg);
                break;
            case QtInfoMsg:
                Logger::info(msg);
                break;
            case QtWarningMsg:
                Logger::warning(msg);
                break;
            case QtCriticalMsg:
                Logger::critical(msg);
                break;
            case QtFatalMsg:
                Logger::critical(msg);
                QMessageBox::critical(nullptr, "致命错误", msg);
                abort();
        }
    });
    // 配置日志系统
    Logger::instance()->init();
    Logger::setMaxSizeMB(10);  // 设置日志文件最大10MB
    Logger::setBackupCount(5); // 保留5个备份文件
    try
    {
        //创建主窗口实例
        MainWindow w;
        //检查启动参数，如果包含"--minimized"则最小化到托盘
        //这种设计常用于开机自启动等场景
        if(a.arguments().contains("--minimized"))
        {
            //隐藏主窗口（通常配合系统托盘使用）
            w.hide();
            Logger::info("应用程序以最小化模式启动");
        }
        else
        {
            //正常显示主窗口
            w.show();
            Logger::info("应用程序正常启动");
        }
        //启动Qt事件循环，程序将在此处保持运行直到退出
        return a.exec();
    }
    catch(const std::exception& e)
    {
        Logger::critical(QString("主窗口初始化失败: %1").arg(e.what()));
        QMessageBox::critical(nullptr, "初始化错误",
                              QString("无法初始化主窗口:\n%1").arg(e.what()));
        return -1;
    }
    catch(...)
    {
        Logger::critical("未知的主窗口初始化错误");
        QMessageBox::critical(nullptr, "初始化错误",
                              "主窗口初始化过程中发生未知错误");
        return -1;
    }
    //注：程序退出时，QApplication析构函数会自动清理资源
    //包括删除所有窗口对象和释放Qt相关资源
}

#include "sshclientwidget.h"

#include <QApplication>
#include <QTextCodec>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QMutex>

// 全局变量
QFile g_logFile;
QTextStream g_logStream;
QMutex g_logMutex;

// 消息处理函数，将所有qDebug、qInfo、qWarning和qCritical的输出重定向到文件
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // 获取当前时间
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    // 根据消息类型，设置不同的前缀和颜色（用于控制台输出）
    QString prefix;
    QString colorCode;
    switch(type)
    {
        case QtDebugMsg:
            prefix = "[DEBUG]";
            colorCode = "\033[32m"; // 绿色
            break;
        case QtInfoMsg:
            prefix = "[INFO]";
            colorCode = "\033[36m"; // 青色
            break;
        case QtWarningMsg:
            prefix = "[WARNING]";
            colorCode = "\033[33m"; // 黄色
            break;
        case QtCriticalMsg:
            prefix = "[CRITICAL]";
            colorCode = "\033[31m"; // 红色
            break;
        case QtFatalMsg:
            prefix = "[FATAL]";
            colorCode = "\033[35m"; // 紫色
            break;
    }
    // 重置颜色的代码
    const QString resetColorCode = "\033[0m";
    // 格式化日志消息
    QString logMessage = QString("%1 %2 %3")
                         .arg(timestamp)
                         .arg(prefix)
                         .arg(msg);
    // 如果需要，可以添加文件名和行号
    if(context.file && strlen(context.file) > 0)
    {
        logMessage += QString(" (%1:%2, %3)")
                      .arg(context.file)
                      .arg(context.line)
                      .arg(context.function);
    }
    // 使用互斥锁确保线程安全
    QMutexLocker locker(&g_logMutex);
    // 输出到控制台（带颜色）
#ifdef Q_OS_WIN
    // Windows控制台可能不支持ANSI颜色代码，使用普通输出
    fprintf(stderr, "%s\n", qPrintable(logMessage));
#else
    // 在支持ANSI颜色的终端上使用彩色输出
    fprintf(stderr, "%s%s%s\n", qPrintable(colorCode), qPrintable(logMessage), qPrintable(resetColorCode));
#endif
    // 输出到日志文件（不带颜色代码）
    if(g_logFile.isOpen())
    {
        g_logStream << logMessage << "\n";
        g_logStream.flush();
    }
    // 如果是致命错误，终止程序
    if(type == QtFatalMsg)
    {
        abort();
    }
}

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    // 设置应用程序信息
    QApplication::setApplicationName("SSH客户端");
    QApplication::setApplicationVersion("1.0.0");
    // 设置编码，确保中文显示正常
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
#else
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif
    // 初始化日志文件
    QString logDir = QApplication::applicationDirPath() + "/logs";
    QDir dir(logDir);
    if(!dir.exists())
    {
        dir.mkpath(".");
        qDebug() << "创建日志目录：" << logDir;
    }
    // 清理旧日志文件（保留最近30天的日志）
    QStringList filters;
    filters << "*.log";
    QFileInfoList logFiles = dir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);
    QDateTime thirtyDaysAgo = QDateTime::currentDateTime().addDays(-30);
    for(const QFileInfo &fileInfo : logFiles)
    {
        if(fileInfo.lastModified() < thirtyDaysAgo)
        {
            QFile oldLog(fileInfo.absoluteFilePath());
            if(oldLog.remove())
            {
                qDebug() << "删除过期日志文件：" << fileInfo.fileName();
            }
        }
    }
    // 日志文件名：应用程序名称_yyyy-MM-dd_HH-mm-ss.log
    QString logFileName = logDir + "/" + QApplication::applicationName() + "_" +
                          QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".log";
    // 打开日志文件（每次启动创建新文件）
    g_logFile.setFileName(logFileName);
    if(g_logFile.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        g_logStream.setDevice(&g_logFile);
        g_logStream.setCodec("UTF-8");
        // 写入日志文件头部信息
        g_logStream << "------------------------------------------------------------\n";
        g_logStream << "Log session started at " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << "\n";
        g_logStream << "Application: " << QApplication::applicationName() << " v" << QApplication::applicationVersion() << "\n";
        g_logStream << "Operating System: " << QSysInfo::prettyProductName() << " (" << QSysInfo::kernelVersion() << ")\n";
        g_logStream << "Qt Version: " << QT_VERSION_STR << "\n";
        g_logStream << "------------------------------------------------------------\n";
        g_logStream.flush();
        // 安装消息处理函数
        qInstallMessageHandler(messageHandler);
        // 使用标准输出先输出一条消息，然后再通过qDebug输出（这样会同时记录到日志文件）
        fprintf(stderr, "日志系统初始化成功，日志文件：%s\n", qPrintable(logFileName));
        qDebug() << "日志系统初始化成功，日志文件：" << logFileName;
    }
    else
    {
        // 无法打开日志文件时，输出错误信息
        fprintf(stderr, "无法打开日志文件：%s，错误：%s\n",
                qPrintable(logFileName),
                qPrintable(g_logFile.errorString()));
        qWarning() << "无法打开日志文件：" << logFileName << "，错误：" << g_logFile.errorString();
    }
    SSHClientWidget w;
    w.show();
    // 执行应用程序
    int result = a.exec();
    // 程序结束前关闭日志文件
    if(g_logFile.isOpen())
    {
        // 计算会话持续时间
        QDateTime endTime = QDateTime::currentDateTime();
        QDateTime startTime = QDateTime::currentDateTime(); // 默认值，以防无法读取开始时间
        // 尝试从日志文件中读取会话开始时间
        if(g_logFile.isOpen() && g_logFile.size() > 0)
        {
            // 保存当前文件名
            QString fileName = g_logFile.fileName();
            // 关闭当前文件
            g_logStream.setDevice(nullptr);
            g_logFile.close();
            
            // 以读写模式重新打开文件（不需要追加模式，因为每次启动都创建新文件）
            if(g_logFile.open(QIODevice::ReadWrite | QIODevice::Text))
            {
                // 回到文件开始
                g_logFile.seek(0);
                // 查找会话开始行
                QString line;
                QTextStream in(&g_logFile);
                in.setCodec("UTF-8");
                while(!in.atEnd())
                {
                    line = in.readLine();
                    if(line.contains("Log session started at"))
                    {
                        QStringList parts = line.split("at ");
                        if(parts.size() > 1)
                        {
                            startTime = QDateTime::fromString(parts.last().trimmed(), "yyyy-MM-dd hh:mm:ss.zzz");
                            break;
                        }
                    }
                }
                // 移动到文件末尾以便追加写入
                g_logFile.seek(g_logFile.size());
                // 重新设置输出流
                g_logStream.setDevice(&g_logFile);
                g_logStream.setCodec("UTF-8");
            }
            else
            {
                // 如果无法以读写模式打开，则重新以写模式打开（不需要追加模式）
                g_logFile.open(QIODevice::WriteOnly | QIODevice::Text);
                g_logStream.setDevice(&g_logFile);
                g_logStream.setCodec("UTF-8");
                qWarning() << "无法以读写模式打开日志文件，无法获取会话开始时间";
            }
        }
        qint64 sessionDurationSecs = startTime.secsTo(endTime);
        int hours = sessionDurationSecs / 3600;
        int minutes = (sessionDurationSecs % 3600) / 60;
        int seconds = sessionDurationSecs % 60;
        // 写入会话结束信息
        g_logStream << "\n------------------------------------------------------------\n";
        g_logStream << "Log session ended at " << endTime.toString("yyyy-MM-dd hh:mm:ss.zzz") << "\n";
        g_logStream << "Session duration: "
                    << (hours > 0 ? QString("%1 hours, ").arg(hours) : "")
                    << (minutes > 0 ? QString("%1 minutes, ").arg(minutes) : "")
                    << QString("%1 seconds").arg(seconds) << "\n";
        g_logStream << "------------------------------------------------------------\n";
        g_logStream.flush();
        g_logFile.close();
        fprintf(stderr, "日志会话已结束，日志文件已关闭\n");
    }
    return result;
}

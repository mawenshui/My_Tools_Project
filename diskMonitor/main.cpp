#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QMutex>
#include <QTimer>
#include <QMutexLocker>
#include <QMetaType>
#include "mainwindow.h"
#include "processmonitor.h"

// 自定义消息处理函数，将日志输出到文件
void messageOutput(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    // 过滤掉调试消息，只记录警告及以上级别的消息
    if (type == QtDebugMsg) {
        return;
    }
    
    // 过滤掉Qt CSS相关的警告消息
    if (type == QtWarningMsg && (msg.contains("Unknown property") || msg.contains("box-shadow") || msg.contains("transform"))) {
        return;
    }
    
    // 使用静态变量来缓存日志，减少文件I/O操作
    static QString logBuffer;
    static QMutex logMutex;
    static QTimer* flushTimer = nullptr;
    static QString currentLogFile;
    static int currentFileIndex = 1;
    static bool isFirstRun = true;
    static const qint64 MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB
    
    QMutexLocker locker(&logMutex);
    
    QString logDir = QCoreApplication::applicationDirPath() + "/log";
    QDir().mkpath(logDir);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    
    // 如果是第一次运行或需要创建新文件
    if (isFirstRun || currentLogFile.isEmpty()) {
        QString baseFileName = QString("%1/diskMonitor_log_%2")
                              .arg(logDir)
                              .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
        currentLogFile = QString("%1_%2.txt").arg(baseFileName).arg(currentFileIndex, 3, 10, QChar('0'));
        isFirstRun = false;
    }
    
    // 检查当前文件大小，如果超过10MB则创建新文件
    QFileInfo fileInfo(currentLogFile);
    if (fileInfo.exists() && fileInfo.size() >= MAX_FILE_SIZE) {
        currentFileIndex++;
        QString baseFileName = currentLogFile;
        baseFileName = baseFileName.left(baseFileName.lastIndexOf('_'));
        currentLogFile = QString("%1_%2.txt").arg(baseFileName).arg(currentFileIndex, 3, 10, QChar('0'));
    }
    
    QString typeStr;
    switch (type) {
    case QtDebugMsg:
        typeStr = "DEBUG";
        break;
    case QtWarningMsg:
        typeStr = "WARNING";
        break;
    case QtCriticalMsg:
        typeStr = "CRITICAL";
        break;
    case QtFatalMsg:
        typeStr = "FATAL";
        break;
    case QtInfoMsg:
        typeStr = "INFO";
        break;
    }
    
    QString logMessage = QString("[%1] %2: %3\n")
                        .arg(timestamp)
                        .arg(typeStr)
                        .arg(msg);
    
    // 将日志消息添加到缓冲区
    logBuffer += logMessage;
    
    // 如果缓冲区太大或者是关键/致命错误，立即刷新
    if (logBuffer.length() > 1024 || type == QtCriticalMsg || type == QtFatalMsg) {
        QFile file(currentLogFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream stream(&file);
            stream << logBuffer;
            stream.flush();
            file.close();
        }
        logBuffer.clear();
    }
    else {
        // 设置定时器，定期刷新缓冲区
        if (!flushTimer) {
            flushTimer = new QTimer();
            QObject::connect(flushTimer, &QTimer::timeout, [&]() {
                QMutexLocker timerLocker(&logMutex);
                if (!logBuffer.isEmpty() && !currentLogFile.isEmpty()) {
                    QFile file(currentLogFile);
                    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
                        QTextStream stream(&file);
                        stream << logBuffer;
                        stream.flush();
                        file.close();
                    }
                    logBuffer.clear();
                }
            });
            flushTimer->setSingleShot(true);
        }
        flushTimer->start(100); // 100ms后刷新缓冲区
    }
}

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    // 注册自定义类型到Qt元对象系统，用于跨线程信号传递
    qRegisterMetaType<ProcessInfo>("ProcessInfo");
    qRegisterMetaType<DriveProcessStats>("DriveProcessStats");
    qRegisterMetaType<QList<ProcessInfo>>("QList<ProcessInfo>");

    // 安装自定义消息处理器
    qInstallMessageHandler(messageOutput);

    qInfo() << "[系统] 磁盘监控程序启动 - 版本 1.0";

    MainWindow w;
    w.show();

    // 主窗口显示完成

    int result = a.exec();

    qInfo() << "[系统] 程序正常退出";
    return result;
}

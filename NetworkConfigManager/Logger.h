#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDir>
#include <QCoreApplication>
#include <QThread>
#include <QElapsedTimer>

/**
 * @brief 日志记录器类，提供线程安全的日志记录功能
 *
 * 该类实现了单例模式，支持多日志级别，可按日期自动创建日志文件，
 * 并提供了便捷的静态方法用于快速记录日志。
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 日志级别枚举
     */
    enum LogLevel
    {
        DEBUG = 0,      //调试信息
        INFO,           //普通信息
        WARNING,        //警告信息
        ERROR,          //错误信息
        CRITICAL        //严重错误
    };

    /**
     * @brief 获取Logger单例实例
     * @return Logger单例指针
     *
     * 使用双重检查锁定模式实现线程安全的单例
     */
    static Logger* instance()
    {
        static Logger* instance = new Logger();
        return instance;
    }

    /**
     * @brief 初始化日志系统
     * @param logDir 日志目录路径，为空则使用默认目录
     * @param logFilePrefix 日志文件名前缀
     * @return 初始化是否成功
     *
     * 如果目录不存在会自动创建，日志文件按日期命名
     */
    bool init(const QString& logDir = "", const QString& logFilePrefix = "app")
    {
        QMutexLocker locker(&m_mutex);
        // 如果已经初始化成功，直接返回
        if(m_logFile.isOpen())
        {
            return true;
        }
        // 尝试多个可能的日志目录
        QStringList possibleDirs;
        if(!logDir.isEmpty())
        {
            possibleDirs << logDir;
        }
        possibleDirs << QDir::currentPath() + "/logs"
                     << QDir::tempPath() + "/" + QCoreApplication::applicationName() + "/logs";
        QString logFilePath;
        for(const auto& dirPath : possibleDirs)
        {
            QDir dir(dirPath);
            if(!dir.exists() && !dir.mkpath(dirPath))
            {
                continue;
            }
            QString dateStr = QDateTime::currentDateTime().toString("yyyyMMdd");
            logFilePath = dirPath + "/" + logFilePrefix + "_" + dateStr + ".log";
            m_logFile.setFileName(logFilePath);
            QElapsedTimer timer;
            timer.start();
            // 尝试打开文件，最多等待500ms
            while(!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
            {
                if(timer.elapsed() > 500)
                {
                    break;
                }
                QThread::msleep(10);
            }
            if(m_logFile.isOpen())
            {
                m_logStream.setDevice(&m_logFile);
                return true;
            }
        }
        // 所有目录尝试失败，使用控制台输出
        if(!m_consoleFile.isOpen())
        {
            m_consoleFile.open(stdout, QIODevice::WriteOnly);
            qWarning("Failed to initialize log file, falling back to console logging");
            m_logStream.setDevice(&m_consoleFile);
        }
        return false;
    }

    /**
     * @brief 记录日志
     * @param level 日志级别
     * @param message 日志消息
     */
    /**
     * @brief 设置日志文件最大大小(MB)
     * @param maxSizeMB 最大大小(MB)
     */
    static void setMaxSizeMB(qint64 maxSizeMB)
    {
        QMutexLocker locker(&instance()->m_mutex);
        instance()->m_maxSizeBytes = maxSizeMB * 1024 * 1024;
    }

    /**
     * @brief 设置保留的旧日志文件数量
     * @param count 备份数量
     */
    static void setBackupCount(int count)
    {
        QMutexLocker locker(&instance()->m_mutex);
        instance()->m_backupCount = qMax(1, count);
    }

    /**
     * @brief 执行日志轮转
     */
    void rotateLog()
    {
        if(m_logFile.isOpen())
        {
            m_logStream.flush();
            m_logFile.close();
        }
        QString currentPath = m_logFile.fileName();
        QFileInfo fileInfo(currentPath);
        // 删除最旧的备份
        QString oldestBackup = QString("%1.%2").arg(currentPath).arg(m_backupCount);
        if(QFile::exists(oldestBackup))
        {
            QFile::remove(oldestBackup);
        }
        // 重命名现有备份
        for(int i = m_backupCount - 1; i >= 1; --i)
        {
            QString oldName = QString("%1.%2").arg(currentPath).arg(i);
            QString newName = QString("%1.%2").arg(currentPath).arg(i + 1);
            if(QFile::exists(oldName))
            {
                QFile::rename(oldName, newName);
            }
        }
        // 重命名当前日志为第一个备份
        QString firstBackup = QString("%1.1").arg(currentPath);
        if(QFile::exists(currentPath))
        {
            QFile::rename(currentPath, firstBackup);
        }
        // 重新打开日志文件
        if(!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        {
            return;
        }
        m_logStream.setDevice(&m_logFile);
    }

    void log(LogLevel level, const QString& message)
    {
        QMutexLocker locker(&m_mutex);  //加锁保证线程安全
        //检查日志文件是否打开
        if(!m_logFile.isOpen())
        {
            return;
        }
        // 检查日志文件大小
        if(m_logFile.size() >= m_maxSizeBytes)
        {
            rotateLog();
        }
        // 检查日志文件大小
        if(m_logFile.size() >= m_maxSizeBytes)
        {
            rotateLog();
        }
        //将日志级别转换为字符串
        QString levelStr;
        switch(level)
        {
            case DEBUG:
                levelStr = "DEBUG";
                break;
            case INFO:
                levelStr = "INFO";
                break;
            case WARNING:
                levelStr = "WARNING";
                break;
            case ERROR:
                levelStr = "ERROR";
                break;
            case CRITICAL:
                levelStr = "CRITICAL";
                break;
            default:
                levelStr = "UNKNOWN";
                break;
        }
        //格式化日志条目: [时间] [级别] 消息
        QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString logEntry = QString("[%1] [%2] %3\n").arg(timeStr, levelStr, message);
        //写入日志并立即刷新缓冲区
        m_logStream << logEntry;
        m_logStream.flush();
    }

    //以下是一组便捷的静态日志方法，方便直接调用 --------------------------

    /**
     * @brief 记录DEBUG级别日志
     * @param message 日志消息
     */
    static void debug(const QString& message)
    {
        instance()->log(DEBUG, message);
    }

    /**
     * @brief 记录INFO级别日志
     * @param message 日志消息
     */
    static void info(const QString& message)
    {
        instance()->log(INFO, message);
    }

    /**
     * @brief 记录WARNING级别日志
     * @param message 日志消息
     */
    static void warning(const QString& message)
    {
        instance()->log(WARNING, message);
    }

    /**
     * @brief 记录ERROR级别日志
     * @param message 日志消息
     */
    static void error(const QString& message)
    {
        instance()->log(ERROR, message);
    }

    /**
     * @brief 记录CRITICAL级别日志
     * @param message 日志消息
     */
    static void critical(const QString& message)
    {
        instance()->log(CRITICAL, message);
    }

    /**
     * @brief 关闭日志文件
     *
     * 刷新缓冲区并关闭日志文件，通常在程序退出时调用
     */
    void shutdown()
    {
        QMutexLocker locker(&m_mutex);  //加锁保证线程安全
        if(m_logFile.isOpen())
        {
            m_logStream.flush();  //确保所有内容写入文件
            m_logFile.close();
        }
    }

private:
    /**
     * @brief 私有构造函数
     * @param parent 父对象
     */
    explicit Logger(QObject *parent = nullptr) : QObject(parent) {}

    /**
     * @brief 析构函数
     *
     * 自动关闭日志文件
     */
    ~Logger()
    {
        shutdown();
    }

    //禁止拷贝和赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    //成员变量 --------------------------------------------------------
    QFile m_logFile;        //日志文件对象
    QFile m_consoleFile;    //控制台文件对象
    QTextStream m_logStream; //日志文本流
    QMutex m_mutex;         //互斥锁，保证线程安全
    qint64 m_maxSizeBytes = 5 * 1024 * 1024; // 默认5MB
    int m_backupCount = 3;  // 默认保留3个备份
};

#endif //LOGGER_H

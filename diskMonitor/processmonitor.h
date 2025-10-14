#ifndef PROCESSMONITOR_H
#define PROCESSMONITOR_H

#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QHash>
#include <QDateTime>
#include <QEvent>
#include <QChildEvent>
#include <QTimerEvent>

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <psapi.h>
    #include <tlhelp32.h>
    #include <winternl.h>
    #include <ntstatus.h>
#endif

// 进程信息结构体
struct ProcessInfo
{
    QString processName;        // 进程名称
    quint32 processId;         // 进程ID
    QString executablePath;    // 可执行文件路径
    qint64 workingSetSize;     // 工作集大小（内存使用）
    qint64 diskReadBytes;      // 磁盘读取字节数
    qint64 diskWriteBytes;     // 磁盘写入字节数
    qint64 totalDiskBytes;     // 总磁盘使用字节数
    double cpuUsage;           // CPU使用率
    QString userName;          // 用户名
    bool isCriticalProcess;    // 是否为关键系统进程
    QString driveLetter;       // 主要使用的驱动器
    QDateTime lastUpdateTime;  // 最后更新时间
};

// 驱动器进程统计信息
struct DriveProcessStats
{
    QString driveLetter;
    QList<ProcessInfo> processes;
    qint64 totalReadBytes;
    qint64 totalWriteBytes;
    int activeProcessCount;
    int criticalProcessCount;
};

class ProcessMonitor : public QObject
{
    Q_OBJECT

public:
    explicit ProcessMonitor(QObject *parent = nullptr);
    virtual ~ProcessMonitor();

    // 启动和停止监控
    Q_INVOKABLE void startMonitoring(int intervalMs = 10000);
    void stopMonitoring();

    // 获取进程信息
    QList<ProcessInfo> getAllProcesses() const;
    QList<ProcessInfo> getProcessesByDrive(const QString& driveLetter) const;
    DriveProcessStats getDriveStats(const QString& driveLetter) const;

    // 进程操作
    bool terminateProcess(quint32 processId);
    bool openProcessLocation(const QString& processName);
    QString getProcessDescription(quint32 processId);

    // 设置监控参数
    void setMonitoringInterval(int intervalMs);
    Q_INVOKABLE void setDriveFilter(const QStringList& driveLetters);

    // 获取系统信息
    static QStringList getSystemDrives();
    static bool isSystemProcess(const QString& processName);
    static QString formatProcessPriority(int priority);

public slots:
    Q_INVOKABLE void refreshProcessData();
    void updateDriveFilter(const QStringList& drives);

protected:
    // 重写QObject的虚函数
    void connectNotify(const QMetaMethod &) override {}
    void disconnectNotify(const QMetaMethod &) override {}
    void customEvent(QEvent *) override {}
    void childEvent(QChildEvent *) override {}
    void timerEvent(QTimerEvent *) override {}
    bool eventFilter(QObject *, QEvent *) override { return false; }
    bool event(QEvent *e) override { return QObject::event(e); }

signals:
    void processDataUpdated();
    void driveStatsUpdated(const QString& drive, const DriveProcessStats& stats);
    void errorOccurred(const QString& error);
    void processStarted(const ProcessInfo& process);
    void processTerminated(quint32 processId);

public slots:
    void onTimerTimeout();
    Q_INVOKABLE void requestDriveStats(const QString& driveLetter);

private:
    // Windows API 相关函数
#ifdef Q_OS_WIN
    bool enableDebugPrivilege();
    QList<ProcessInfo> enumerateProcesses();
    ProcessInfo getProcessInfo(DWORD processId);
    QString getProcessName(HANDLE hProcess);
    QString getProcessPath(HANDLE hProcess);
    qint64 getProcessDiskUsage(HANDLE hProcess);
    QString getProcessUserName(HANDLE hProcess);
    double calculateCpuUsage(HANDLE hProcess, DWORD processId);
    QString getProcessDriveLetter(const QString& executablePath);
#endif

    // 辅助函数
    void updateProcessList();
    void calculateDriveStats();
    bool isProcessCritical(const QString& processName);
    void cleanupOldData();

    // 成员变量
    QTimer* m_updateTimer;
    QList<ProcessInfo> m_processList;
    QHash<QString, DriveProcessStats> m_driveStats;
    QStringList m_monitoredDrives;
    mutable QMutex m_dataMutex;

    // 性能计数器相关
    QHash<quint32, qint64> m_lastCpuTimes;
    QHash<quint32, qint64> m_lastSystemTime;

    // 配置参数
    int m_updateInterval;
    bool m_isMonitoring;
    bool m_debugPrivilegeEnabled;

    // 系统关键进程列表
    static const QStringList s_criticalProcesses;
};

#endif // PROCESSMONITOR_H

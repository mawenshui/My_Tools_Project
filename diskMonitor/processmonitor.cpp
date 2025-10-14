#include "processmonitor.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QMutexLocker>

#ifdef Q_OS_WIN
    #include <QLibrary>
    #pragma comment(lib, "psapi.lib")
    #pragma comment(lib, "advapi32.lib")
#endif

// 系统关键进程列表
const QStringList ProcessMonitor::s_criticalProcesses =
{
    "System", "smss.exe", "csrss.exe", "wininit.exe", "winlogon.exe",
    "services.exe", "lsass.exe", "svchost.exe", "explorer.exe",
    "dwm.exe", "audiodg.exe", "spoolsv.exe", "taskhost.exe",
    "taskhostw.exe", "RuntimeBroker.exe", "SearchIndexer.exe"
};

ProcessMonitor::ProcessMonitor(QObject *parent)
    : QObject(parent)
    , m_updateTimer(nullptr)
    , m_updateInterval(5000)
    , m_isMonitoring(false)
    , m_debugPrivilegeEnabled(false)
{

#ifdef Q_OS_WIN
    // 尝试启用调试权限
    m_debugPrivilegeEnabled = enableDebugPrivilege();
    if (!m_debugPrivilegeEnabled)
    {
        qWarning() << "Failed to enable debug privilege. Some process information may be limited.";
    }
#endif
}

ProcessMonitor::~ProcessMonitor()
{
    stopMonitoring();
}

void ProcessMonitor::startMonitoring(int intervalMs)
{
    if (m_isMonitoring)
    {
        return;
    }

    // 确保在正确的线程中创建定时器
    if (!m_updateTimer)
    {
        m_updateTimer = new QTimer(this);
        connect(m_updateTimer, &QTimer::timeout, this, &ProcessMonitor::onTimerTimeout);
        // 确保定时器在当前线程中
        m_updateTimer->moveToThread(this->thread());
    }

    m_updateInterval = intervalMs;
    m_updateTimer->setInterval(m_updateInterval);
    m_updateTimer->start();
    m_isMonitoring = true;

    // 立即更新一次
    refreshProcessData();

    qInfo() << "[进程监控] 监控已启动，间隔:" << intervalMs << "ms";
}

void ProcessMonitor::stopMonitoring()
{
    if (!m_isMonitoring)
    {
        return;
    }

    if (m_updateTimer)
    {
        // 使用QMetaObject::invokeMethod确保线程安全
        QMetaObject::invokeMethod(m_updateTimer, "stop", Qt::QueuedConnection);
    }
    m_isMonitoring = false;

    qInfo() << "[进程监控] 监控已停止";
}

void ProcessMonitor::refreshProcessData()
{
    QMutexLocker locker(&m_dataMutex);

    try
    {
        // 减少日志输出频率，避免缓冲区溢出
        static int refreshCount = 0;
        bool shouldLog = (refreshCount % 30 == 0); // 每30次刷新才输出一次日志

        updateProcessList();
        calculateDriveStats();
        if (shouldLog)
        {
            qInfo() << QString("[进程监控] 获取到 %1 个进程，监控 %2 个驱动器")
                    .arg(m_processList.size()).arg(m_monitoredDrives.size());
        }

        cleanupOldData();

        // 只在定时器触发时发送processDataUpdated信号，避免频繁刷新UI
        // emit processDataUpdated();

        // 为每个监控的驱动器发送统计信息
        for (const QString& drive : m_monitoredDrives)
        {
            if (m_driveStats.contains(drive))
            {
                const auto& stats = m_driveStats[drive];
                emit driveStatsUpdated(drive, m_driveStats[drive]);
            }
            else if (shouldLog)
            {
                qWarning() << QString("[进程监控] 驱动器 %1 无统计数据").arg(drive);
            }
        }

        refreshCount++;
    }
    catch (const std::exception& e)
    {
        QString errorMsg = QString::fromStdString(e.what());
        qCritical() << QString("[进程监控] 严重错误：%1").arg(errorMsg);
        emit errorOccurred(errorMsg);
    }
}

void ProcessMonitor::updateProcessList()
{
#ifdef Q_OS_WIN
    m_processList = enumerateProcesses();
    qInfo() << QString("[进程监控] Windows进程枚举完成，获取到 %1 个进程").arg(m_processList.size());
#else
    qInfo() << "[进程监控] 使用非Windows平台模拟数据";
    // 非Windows平台的模拟实现
    m_processList.clear();
    ProcessInfo mockProcess;
    mockProcess.processName = "mock_process";
    mockProcess.processId = 1234;
    mockProcess.executablePath = "/usr/bin/mock";
    mockProcess.workingSetSize = 1024 * 1024; // 1MB
    mockProcess.diskReadBytes = 1024 * 1024;
    mockProcess.diskWriteBytes = 512 * 1024;
    mockProcess.totalDiskBytes = mockProcess.diskReadBytes + mockProcess.diskWriteBytes;
    mockProcess.cpuUsage = 5.0;
    mockProcess.userName = "user";
    mockProcess.isCriticalProcess = false;
    mockProcess.driveLetter = "C:";
    mockProcess.lastUpdateTime = QDateTime::currentDateTime();
    m_processList.append(mockProcess);
    // 模拟数据创建完成
#endif
}

#ifdef Q_OS_WIN
bool ProcessMonitor::enableDebugPrivilege()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return false;
    }

    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid))
    {
        CloseHandle(hToken);
        return false;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bool result = AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0) != 0;
    CloseHandle(hToken);

    return result;
}

QList<ProcessInfo> ProcessMonitor::enumerateProcesses()
{
    QList<ProcessInfo> processes;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        qCritical() << "[进程监控] 错误：无法创建进程快照";
        return processes;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    int totalProcesses = 0;
    int validProcesses = 0;

    if (Process32FirstW(hSnapshot, &pe32))
    {
        do
        {
            totalProcesses++;
            ProcessInfo info = getProcessInfo(pe32.th32ProcessID);
            if (!info.processName.isEmpty())
            {
                validProcesses++;
                processes.append(info);
            }
        }
        while (Process32NextW(hSnapshot, &pe32));
    }

    qInfo() << QString("[进程监控] 枚举完成：总计 %1 个进程，有效 %2 个进程")
            .arg(totalProcesses).arg(validProcesses);

    CloseHandle(hSnapshot);
    return processes;
}

ProcessInfo ProcessMonitor::getProcessInfo(DWORD processId)
{
    ProcessInfo info;
    info.processId = processId;
    info.lastUpdateTime = QDateTime::currentDateTime();

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL)
    {
        // 尝试使用更少的权限
        hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (hProcess == NULL)
        {
            return info;
        }
    }

    // 获取进程名称
    info.processName = getProcessName(hProcess);

    // 获取可执行文件路径
    info.executablePath = getProcessPath(hProcess);

    // 获取内存使用信息
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
        info.workingSetSize = pmc.WorkingSetSize;
    }

    // 获取磁盘使用信息
    info.totalDiskBytes = getProcessDiskUsage(hProcess);
    info.diskReadBytes = info.totalDiskBytes / 2;  // 简化处理
    info.diskWriteBytes = info.totalDiskBytes / 2;

    // 获取CPU使用率
    info.cpuUsage = calculateCpuUsage(hProcess, processId);

    // 获取用户名
    info.userName = getProcessUserName(hProcess);

    // 判断是否为关键进程
    info.isCriticalProcess = isProcessCritical(info.processName);

    // 获取主要使用的驱动器
    info.driveLetter = getProcessDriveLetter(info.executablePath);

    CloseHandle(hProcess);
    return info;
}

QString ProcessMonitor::getProcessName(HANDLE hProcess)
{
    wchar_t processName[MAX_PATH] = L"<unknown>";

    if (GetModuleFileNameExW(hProcess, NULL, processName, MAX_PATH))
    {
        QString fullPath = QString::fromWCharArray(processName);
        return QFileInfo(fullPath).fileName();
    }

    // 备用方法
    HMODULE hMod;
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
    {
        if (GetModuleBaseNameW(hProcess, hMod, processName, sizeof(processName) / sizeof(wchar_t)))
        {
            return QString::fromWCharArray(processName);
        }
    }

    return QString("<unknown>");
}

QString ProcessMonitor::getProcessPath(HANDLE hProcess)
{
    wchar_t processPath[MAX_PATH];

    if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH))
    {
        return QString::fromWCharArray(processPath);
    }

    return QString();
}

qint64 ProcessMonitor::getProcessDiskUsage(HANDLE hProcess)
{
    // 这是一个简化的实现
    // 在实际应用中，需要使用Performance Counters或ETW来获取准确的磁盘I/O信息
    IO_COUNTERS ioCounters;
    if (GetProcessIoCounters(hProcess, &ioCounters))
    {
        return ioCounters.ReadTransferCount + ioCounters.WriteTransferCount;
    }
    return 0;
}

QString ProcessMonitor::getProcessUserName(HANDLE hProcess)
{
    HANDLE hToken;
    if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
    {
        return QString("<unknown>");
    }

    DWORD dwSize = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);

    if (dwSize == 0)
    {
        CloseHandle(hToken);
        return QString("<unknown>");
    }

    PTOKEN_USER pTokenUser = (PTOKEN_USER)malloc(dwSize);
    if (!pTokenUser)
    {
        CloseHandle(hToken);
        return QString("<unknown>");
    }

    QString userName = "<unknown>";
    if (GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize))
    {
        wchar_t szName[256];
        wchar_t szDomain[256];
        DWORD dwNameSize = 256;
        DWORD dwDomainSize = 256;
        SID_NAME_USE sidType;

        if (LookupAccountSidW(NULL, pTokenUser->User.Sid, szName, &dwNameSize,
                              szDomain, &dwDomainSize, &sidType))
        {
            userName = QString::fromWCharArray(szName);
        }
    }

    free(pTokenUser);
    CloseHandle(hToken);
    return userName;
}

double ProcessMonitor::calculateCpuUsage(HANDLE hProcess, DWORD processId)
{
    Q_UNUSED(processId)
    // 简化的CPU使用率计算
    // 在实际应用中需要更复杂的计算方法
    FILETIME creationTime, exitTime, kernelTime, userTime;
    if (GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime))
    {
        ULARGE_INTEGER kernel, user;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;

        qint64 totalTime = kernel.QuadPart + user.QuadPart;

        // 简化处理，返回基于总时间的估算值
        return (double)(totalTime % 10000) / 100.0;
    }
    return 0.0;
}

QString ProcessMonitor::getProcessDriveLetter(const QString& executablePath)
{
    if (executablePath.length() >= 2 && executablePath[1] == ':')
    {
        return executablePath.left(1).toUpper(); // 只返回驱动器字母，不包含冒号
    }
    return "C"; // 默认返回C盘字母
}
#endif

void ProcessMonitor::calculateDriveStats()
{
    m_driveStats.clear();

    // 为每个监控的驱动器计算统计信息
    for (const QString& drive : m_monitoredDrives)
    {
        DriveProcessStats stats;
        stats.driveLetter = drive;
        stats.totalReadBytes = 0;
        stats.totalWriteBytes = 0;
        stats.activeProcessCount = 0;
        stats.criticalProcessCount = 0;

        // 筛选属于该驱动器的进程
        int matchedProcesses = 0;
        for (const ProcessInfo& process : m_processList)
        {
            // 改进的驱动器匹配逻辑
            QString targetDriveLetter = drive.left(1).toUpper();
            bool belongsToDrive = false;
            QString matchReason = "";

            // 首先检查进程的驱动器字母
            if (!process.driveLetter.isEmpty())
            {
                QString processDriveLetter = process.driveLetter.left(1).toUpper();
                belongsToDrive = (processDriveLetter == targetDriveLetter);
                if (belongsToDrive) matchReason = "驱动器字母匹配";
            }
            // 如果进程没有明确的驱动器信息，检查可执行文件路径
            else if (!process.executablePath.isEmpty())
            {
                QString pathDriveLetter = process.executablePath.left(1).toUpper();
                belongsToDrive = (pathDriveLetter == targetDriveLetter);
                if (belongsToDrive) matchReason = "可执行文件路径匹配";
            }
            // 对于没有路径信息的进程，使用更宽松的匹配策略
            else
            {
                // 对于C盘，包含所有系统进程和有活动的进程
                if (targetDriveLetter == "C")
                {
                    belongsToDrive = (process.isCriticalProcess ||
                                      process.diskReadBytes > 0 ||
                                      process.diskWriteBytes > 0 ||
                                      process.cpuUsage > 0 ||
                                      process.workingSetSize > 0);
                    if (belongsToDrive) matchReason = "系统进程默认C盘";
                }
                // 对于其他驱动器，只匹配有明确磁盘活动的进程
                else
                {
                    belongsToDrive = (process.diskReadBytes > 0 || process.diskWriteBytes > 0);
                    if (belongsToDrive) matchReason = "磁盘活动匹配";
                }
            }

            if (belongsToDrive)
            {
                matchedProcesses++;
                stats.processes.append(process);
                stats.totalReadBytes += process.diskReadBytes;
                stats.totalWriteBytes += process.diskWriteBytes;

                if (process.isCriticalProcess)
                {
                    stats.criticalProcessCount++;
                }
            }
        }

        // 设置活跃进程数为实际找到的进程数量
        stats.activeProcessCount = stats.processes.size();

        // 按磁盘使用量排序
        std::sort(stats.processes.begin(), stats.processes.end(),
                  [](const ProcessInfo & a, const ProcessInfo & b)
        {
            return a.totalDiskBytes > b.totalDiskBytes;
        });

        // 只保留前20个进程（增加显示数量）
        if (stats.processes.size() > 20)
        {
            stats.processes = stats.processes.mid(0, 20);
        }

        m_driveStats[drive] = stats;
    }
}

bool ProcessMonitor::isProcessCritical(const QString& processName)
{
    return s_criticalProcesses.contains(processName, Qt::CaseInsensitive);
}

void ProcessMonitor::cleanupOldData()
{
    // 清理超过1小时的旧数据
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-3600);

    m_processList.erase(
        std::remove_if(m_processList.begin(), m_processList.end(),
                       [cutoffTime](const ProcessInfo & process)
    {
        return process.lastUpdateTime < cutoffTime;
    }),
    m_processList.end());
}

QList<ProcessInfo> ProcessMonitor::getAllProcesses() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_processList;
}

QList<ProcessInfo> ProcessMonitor::getProcessesByDrive(const QString& driveLetter) const
{
    QMutexLocker locker(&m_dataMutex);

    if (m_driveStats.contains(driveLetter))
    {
        return m_driveStats[driveLetter].processes;
    }

    return QList<ProcessInfo>();
}

DriveProcessStats ProcessMonitor::getDriveStats(const QString& driveLetter) const
{
    QMutexLocker locker(&m_dataMutex);

    if (m_driveStats.contains(driveLetter))
    {
        return m_driveStats[driveLetter];
    }

    return DriveProcessStats();
}

bool ProcessMonitor::terminateProcess(quint32 processId)
{
#ifdef Q_OS_WIN
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
    if (hProcess == NULL)
    {
        return false;
    }

    bool result = TerminateProcess(hProcess, 0) != 0;
    CloseHandle(hProcess);

    if (result)
    {
        emit processTerminated(processId);
    }

    return result;
#else
    Q_UNUSED(processId)
    return false;
#endif
}

bool ProcessMonitor::openProcessLocation(const QString& processName)
{
    // 查找进程的可执行文件路径
    for (const ProcessInfo& process : m_processList)
    {
        if (process.processName == processName && !process.executablePath.isEmpty())
        {
            QFileInfo fileInfo(process.executablePath);
            if (fileInfo.exists())
            {
                return QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
            }
        }
    }
    return false;
}

QString ProcessMonitor::getProcessDescription(quint32 processId)
{
    for (const ProcessInfo& process : m_processList)
    {
        if (process.processId == processId)
        {
            return QString("进程名: %1\nPID: %2\n路径: %3\n内存: %4\n磁盘使用: %5\nCPU: %6%\n用户: %7")
                   .arg(process.processName)
                   .arg(process.processId)
                   .arg(process.executablePath)
                   .arg(QString::number(process.workingSetSize / 1024 / 1024) + " MB")
                   .arg(QString::number(process.totalDiskBytes / 1024 / 1024) + " MB")
                   .arg(QString::number(process.cpuUsage, 'f', 1))
                   .arg(process.userName);
        }
    }
    return QString("进程信息不可用");
}

void ProcessMonitor::setMonitoringInterval(int intervalMs)
{
    m_updateInterval = intervalMs;
    if (m_isMonitoring && m_updateTimer)
    {
        // 使用QMetaObject::invokeMethod确保线程安全
        QMetaObject::invokeMethod(m_updateTimer, "setInterval", 
                                  Qt::QueuedConnection, Q_ARG(int, intervalMs));
    }
}

void ProcessMonitor::setDriveFilter(const QStringList& driveLetters)
{
    m_monitoredDrives = driveLetters;
    qInfo() << QString("[进程监控] 设置监控驱动器: %1").arg(m_monitoredDrives.join(", "));
}

void ProcessMonitor::updateDriveFilter(const QStringList& drives)
{
    setDriveFilter(drives);
}

QStringList ProcessMonitor::getSystemDrives()
{
    QStringList drives;

#ifdef Q_OS_WIN
    DWORD driveMask = GetLogicalDrives();
    for (int i = 0; i < 26; ++i)
    {
        if (driveMask & (1 << i))
        {
            drives.append(QString("%1:").arg(QChar('A' + i)));
        }
    }
#else
    // 非Windows平台的模拟实现
    drives << "C:" << "D:";
#endif

    return drives;
}

bool ProcessMonitor::isSystemProcess(const QString& processName)
{
    return s_criticalProcesses.contains(processName, Qt::CaseInsensitive);
}

QString ProcessMonitor::formatProcessPriority(int priority)
{
    switch (priority)
    {
        case 4:
            return "空闲";
        case 6:
            return "低于正常";
        case 8:
            return "正常";
        case 10:
            return "高于正常";
        case 13:
            return "高";
        case 24:
            return "实时";
        default:
            return "未知";
    }
}

void ProcessMonitor::onTimerTimeout()
{
    refreshProcessData();
    // 在定时器触发时发送UI更新信号
    emit processDataUpdated();
}

void ProcessMonitor::requestDriveStats(const QString& driveLetter)
{
    // 先强制刷新一次数据
    refreshProcessData();

    QMutexLocker locker(&m_dataMutex);

    if (m_driveStats.contains(driveLetter))
    {
        // 驱动器统计信息已发送
        emit driveStatsUpdated(driveLetter, m_driveStats[driveLetter]);
    }
    else
    {
        // 如果没有数据，创建一个包含所有进程的统计信息作为备选
        DriveProcessStats fallbackStats;
        fallbackStats.driveLetter = driveLetter;
        fallbackStats.totalReadBytes = 0;
        fallbackStats.totalWriteBytes = 0;
        fallbackStats.activeProcessCount = 0;
        fallbackStats.criticalProcessCount = 0;

        // 添加前10个进程作为示例
        int count = 0;
        for (const ProcessInfo& process : m_processList)
        {
            if (count >= 10) break;
            fallbackStats.processes.append(process);
            fallbackStats.totalReadBytes += process.diskReadBytes;
            fallbackStats.totalWriteBytes += process.diskWriteBytes;
            fallbackStats.activeProcessCount++;
            if (process.isCriticalProcess)
            {
                fallbackStats.criticalProcessCount++;
            }
            count++;
        }

        emit driveStatsUpdated(driveLetter, fallbackStats);
    }
}

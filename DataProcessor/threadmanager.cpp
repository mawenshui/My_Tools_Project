#include "threadmanager.h"
#include <QMutexLocker>
#include <QElapsedTimer>

Q_LOGGING_CATEGORY(threadManagerLog, "[app.ThreadManager]")

ThreadManager &ThreadManager::instance()
{
    static ThreadManager instance;
    return instance;
}

void ThreadManager::registerThread(QThread *thread, const QString &name, const std::function<void()> &requestStop)
{
    if (!thread)
    {
        return;
    }
    QMutexLocker locker(&m_mutex);
    for (const auto &entry : m_entries)
    {
        if (entry.thread == thread)
        {
            return;
        }
    }
    ThreadEntry entry;
    entry.thread = thread;
    entry.name = name;
    entry.requestStop = requestStop;
    m_entries.push_back(entry);
    qCInfo(threadManagerLog) << "注册线程:" << name << "地址:" << thread;
}

void ThreadManager::unregisterThread(QThread *thread)
{
    if (!thread)
    {
        return;
    }
    QMutexLocker locker(&m_mutex);
    for (int i = 0; i < m_entries.size(); ++i)
    {
        if (m_entries[i].thread == thread)
        {
            qCInfo(threadManagerLog) << "注销线程:" << m_entries[i].name << "地址:" << thread;
            m_entries.removeAt(i);
            break;
        }
    }
}

void ThreadManager::shutdownAll(int timeoutMs)
{
    QVector<ThreadEntry> entriesCopy;
    {
        QMutexLocker locker(&m_mutex);
        entriesCopy = m_entries;
    }
    if (entriesCopy.isEmpty())
    {
        return;
    }
    qCInfo(threadManagerLog) << "开始关闭所有注册线程, 数量:" << entriesCopy.size();
    for (const auto &entry : entriesCopy)
    {
        QThread *thread = entry.thread;
        if (!thread || !thread->isRunning())
        {
            continue;
        }
        QElapsedTimer timer;
        timer.start();
        int attempts = 0;
        bool exited = false;
        try
        {
            if (entry.requestStop)
            {
                ++attempts;
                entry.requestStop();
            }
        }
        catch (...)
        {
            qCWarning(threadManagerLog) << "请求停止线程时发生异常:" << entry.name;
        }
        if (thread->wait(timeoutMs))
        {
            qint64 elapsed = timer.elapsed();
            qCInfo(threadManagerLog) << "线程正常退出:" << entry.name << "耗时(ms):" << elapsed << "尝试次数:" << attempts;
            exited = true;
        }
        if (exited)
        {
            continue;
        }
        qCWarning(threadManagerLog) << "线程在首次等待内未退出, 重试停止:" << entry.name;
        try
        {
            if (entry.requestStop)
            {
                ++attempts;
                entry.requestStop();
            }
        }
        catch (...)
        {
            qCWarning(threadManagerLog) << "重试停止线程时发生异常:" << entry.name;
        }
        if (thread->wait(timeoutMs))
        {
            qint64 elapsed = timer.elapsed();
            qCInfo(threadManagerLog) << "线程在重试后退出:" << entry.name << "耗时(ms):" << elapsed << "尝试次数:" << attempts;
            continue;
        }
        ++attempts;
        qCWarning(threadManagerLog) << "线程无法在重试后退出, 即将强制终止:" << entry.name;
        thread->terminate();
        thread->wait();
        qint64 elapsed = timer.elapsed();
        qCWarning(threadManagerLog) << "线程已被强制终止:" << entry.name << "耗时(ms):" << elapsed << "尝试次数:" << attempts;
    }
}

bool ThreadManager::hasRunningThreads() const
{
    QMutexLocker locker(&m_mutex);
    for (const auto &entry : m_entries)
    {
        if (entry.thread && entry.thread->isRunning())
        {
            return true;
        }
    }
    return false;
}

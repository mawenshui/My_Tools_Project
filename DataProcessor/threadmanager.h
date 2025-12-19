#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include <QThread>
#include <QMutex>
#include <QVector>
#include <QString>
#include <QLoggingCategory>
#include <functional>

Q_DECLARE_LOGGING_CATEGORY(threadManagerLog)

class ThreadManager
{
public:
    struct ThreadEntry
    {
        QThread *thread = nullptr;
        QString name;
        std::function<void()> requestStop;
    };

    static ThreadManager &instance();

    void registerThread(QThread *thread, const QString &name, const std::function<void()> &requestStop);
    void unregisterThread(QThread *thread);
    void shutdownAll(int timeoutMs);
    bool hasRunningThreads() const;

private:
    ThreadManager() = default;
    ThreadManager(const ThreadManager &) = delete;
    ThreadManager &operator=(const ThreadManager &) = delete;

    mutable QMutex m_mutex;
    QVector<ThreadEntry> m_entries;
};

#endif // THREADMANAGER_H

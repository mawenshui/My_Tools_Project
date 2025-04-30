#ifndef WORKERCLASS_H
#define WORKERCLASS_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QVariantMap>
#include <QDir>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QLoggingCategory>
#include "udpsender.h"
#include <QMutexLocker>
#include <QElapsedTimer>

/**
 * @brief 工作线程类
 * 负责文件处理和 UDP 数据发送，支持暂停、恢复和停止操作。
 */
class WorkerClass : public QObject
{
    Q_OBJECT

public:
    explicit WorkerClass(QObject *parent = nullptr);
    ~WorkerClass();

    /**
     * @brief 配置工作线程参数
     * @param config 配置字典，包含以下键：
     *   - dataDir: 数据目录路径
     *   - addresses: 目标地址列表 (IP:Port)
     *   - sendInterval: 发送间隔(ms)
     *   - order: 文件处理顺序 (顺序/倒序)
     *   - include/excludeTopics: 主题过滤列表
     *   - uniqueMode: 是否过滤重复主题
     */
    void configure(const QVariantMap &config);

    ///开始处理任务
    void startProcessing();

    ///暂停/恢复处理
    ///@param paused true-暂停，false-恢复
    void pauseProcessing(bool paused);

    ///停止处理任务
    void stopProcessing();

    ///获取暂停状态
    bool isPaused();

    ///获取运行状态
    bool isRunning();

    ///设置组播地址
    void setAddrList(const QStringList addrlist);

signals:
    ///日志信号
    void logMessage(const QString &level, const QString &msg);

    ///进度更新 (0-100%)
    void progressUpdated(int percent);

    ///统计信息更新
    ///@param success 成功计数
    ///@param failed 失败计数
    void statsUpdated(int success, int failed);

    ///任务完成信号
    void finished();

private slots:
    ///主处理流程
    void process();

private:
    ///递归收集文件
    QStringList collectFilesRecursive(const QString &path, bool isDesc);

    ///遍历处理文件列表
    void processFiles();

    ///处理单个文件
    ///@param filePath 文件完整路径
    void processFile(const QString &filePath);

    ///主题过滤
    ///@return true-需要发送，false-跳过
    bool filterTopic(const QString &topic);

    ///清理十六进制数据
    ///@return 有效数据返回字节数组，无效返回空
    QByteArray cleanHexData(const QString &data);

    QVariantMap m_config;             ///< 配置参数
    QStringList m_addrlist;           ///需要发送的组播地址
    QScopedPointer<UdpSender> m_udpSender; ///< UDP 发送器实例
    QStringList m_files;               ///< 待处理文件列表
    QSet<QString> m_sentTopics;        ///< 已发送主题记录（唯一模式使用）

    mutable QMutex m_mutex;            ///< 线程互斥锁
    QWaitCondition m_pauseCondition;   ///< 暂停条件变量

    bool m_running = false;            ///< 运行状态标志
    bool m_paused = false;             ///< 暂停状态标志
    int m_successCount = 0;            ///< 成功发送计数
    int m_failedCount = 0;             ///< 发送失败计数
};

#endif //WORKERCLASS_H

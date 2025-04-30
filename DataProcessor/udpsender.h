#ifndef UDPSENDER_H
#define UDPSENDER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QLoggingCategory>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QQueue>

/**
 * @brief UDP 数据发送器类
 * 负责管理 UDP 数据包的发送，支持多线程和批量发送。
 */
class UdpSender : public QObject
{
    Q_OBJECT

public:
    explicit UdpSender(QObject *parent = nullptr);
    ~UdpSender();

    /**
     * @brief 发送数据到指定地址
     * @param address 目标地址（格式：IP:Port）
     * @param data 要发送的数据
     * @param topic 数据主题（用于日志记录）
     * @return 是否成功加入发送队列
     */
    bool send(const QString &address, const QByteArray &data, const QString &topic);

    /**
     * @brief 停止发送并释放资源
     */
    void stop();

    /**
     * @brief 等待发送线程停止
     */
    void waitForStop();

    /**
     * @brief 暂停或恢复发送
     * @param paused true-暂停，false-恢复
     */
    void pause(bool paused);

signals:
    /**
     * @brief 日志信号
     * @param level 日志级别（INFO/WARN/ERROR）
     * @param msg 日志消息
     */
    void logMessage(const QString &level, const QString &msg);

private slots:
    /**
     * @brief 处理发送队列
     */
    void processQueue();

private:
    /**
     * @brief 发送任务结构体
     */
    struct SendTask
    {
        QString address; //目标地址（IP:Port）
        QByteArray data; //要发送的数据
        QString topic;   //数据主题
    };

    /**
     * @brief 将任务加入发送队列
     * @param task 发送任务
     */
    void enqueue(const SendTask &task);

    /**
     * @brief 内部发送逻辑
     * @param address 目标地址
     * @param data 要发送的数据
     * @return 是否发送成功
     */
    bool sendInternal(const QString &address, const QByteArray &data);

    QUdpSocket m_socket;          //UDP 套接字
    QMutex m_dataMutex;           //数据互斥锁
    QQueue<SendTask> m_queue;     //发送队列
    QWaitCondition m_condition;   //条件变量，用于线程同步
    bool m_running = true;        //是否运行中
    QThread m_workerThread;       //工作线程
    bool m_paused = false;        //暂停状态标志
};

#endif //UDPSENDER_H

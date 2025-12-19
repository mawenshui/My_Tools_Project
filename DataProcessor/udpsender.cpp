#include "udpsender.h"
#include <QNetworkInterface>
#include <QThread>
#include <QElapsedTimer>

//定义日志分类
Q_LOGGING_CATEGORY(udpLog, "[app.UdpSender]")

UdpSender::UdpSender(QObject *parent)
    : QObject(parent)
{
    //将对象移动到工作线程
    this->moveToThread(&m_workerThread);
    //连接线程启动信号到处理函数
    connect(&m_workerThread, &QThread::started, this, &UdpSender::processQueue);
    //启动线程
    m_workerThread.start();
    qCDebug(udpLog) << "UDP 发送器初始化完成";
    emit logMessage("DEBUG", "UDP 发送器初始化完成");
}

UdpSender::~UdpSender()
{
    stop();
    waitForStop();
    qCDebug(udpLog) << "UDP 发送器已销毁";
    emit logMessage("DEBUG", "UDP 发送器已销毁");
}

bool UdpSender::send(const QString &address, const QByteArray &data, const QString &topic)
{
    if (address.isEmpty() || data.isEmpty())
    {
        qCWarning(udpLog) << "无效的目标地址或数据";
        emit logMessage("ERROR", "无效的目标地址或数据");
        return false;
    }
    SendTask task{address, data, topic};
    enqueue(task);
    return true;
}

void UdpSender::stop()
{
    QMutexLocker locker(&m_dataMutex);
    if (!m_running)
    {
        qCDebug(udpLog) << "UDP 发送器已停止";
        return;
    }
    qCDebug(udpLog) << "正在停止 UDP 发送器...";
    int pending = m_queue.size();
    if (pending > 0)
    {
        qCWarning(udpLog) << "丢弃未发送的数据包数量:" << pending;
        m_queue.clear();
        emit logMessage("WARN", QString("丢弃未发送的数据包数量: %1").arg(pending));
    }
    //设置停止标志
    m_running = false;
    //唤醒可能等待的线程
    m_condition.wakeAll();
}

void UdpSender::waitForStop()
{
    if (m_workerThread.isRunning())
    {
        qCDebug(udpLog) << "等待 UDP 发送器线程停止...";
        m_workerThread.quit(); //请求线程退出
        if (!m_workerThread.wait(2000))   //等待线程退出，最多等待2秒
        {
            qCWarning(udpLog) << "UDP 发送器线程未退出，强制终止...";
            m_workerThread.terminate(); //强制终止线程
            m_workerThread.wait();      //等待线程终止完成
        }
        qCDebug(udpLog) << "UDP 发送器线程已停止";
    }
}

void UdpSender::pause(bool paused)
{
    QMutexLocker locker(&m_dataMutex);
    m_paused = paused;
    if (!paused)
    {
        m_condition.wakeAll();
    }
    emit logMessage("DEBUG", paused ? "发送已暂停" : "发送已恢复");
}

void UdpSender::processQueue()
{
    QMutexLocker locker(&m_dataMutex);
    QElapsedTimer timer;
    timer.start();
    while (m_running)
    {
        //队列为空或暂停时等待
        while ((m_queue.isEmpty() || m_paused) && m_running)
        {
            m_condition.wait(&m_dataMutex);
        }
        if (!m_running)
        {
            break;
        }
        SendTask task = m_queue.dequeue();
        locker.unlock();
        bool success = sendInternal(task.address, task.data);
        emit logMessage(success ? "INFO" : "ERROR",
                        QString("发送%1 [%2] %3")
                        .arg(success ? "成功" : "失败")
                        .arg(task.topic)
                        .arg(task.address));
        locker.relock();
        //限流处理
        if (timer.elapsed() < 10)
        {
            QThread::usleep(static_cast<unsigned long>(10 - timer.elapsed()));
        }
        timer.restart();
    }
}

void UdpSender::enqueue(const SendTask &task)
{
    QMutexLocker locker(&m_dataMutex);  //自动加锁
    m_queue.enqueue(task);
    m_condition.wakeAll();              //唤醒所有等待线程
}

bool UdpSender::sendInternal(const QString &address, const QByteArray &data)
{
    QStringList parts = address.split(':');
    if (parts.size() != 2)
    {
        qCWarning(udpLog) << "无效的目标地址格式：" << address;
        emit logMessage("ERROR", "无效的目标地址格式：" + address);
        return false;
    }
    QHostAddress ip(parts[0]);
    quint16 port = parts[1].toUShort();
    if (ip.isNull() || port == 0)
    {
        qCWarning(udpLog) << "无效的 IP 或端口：" << address;
        emit logMessage("ERROR", "无效的 IP 或端口：" + address);
        return false;
    }
    qint64 bytesSent = m_socket.writeDatagram(data, ip, port);
    if (bytesSent == -1)
    {
        qCWarning(udpLog) << "数据发送失败：" << m_socket.errorString();
        emit logMessage("ERROR", "数据发送失败：" + m_socket.errorString());
        return false;
    }
    qCInfo(udpLog) << "数据已发送：" << data.toHex().toUpper();
    emit logMessage("INFO", "数据已发送：" + QString(data.toHex().toUpper()));
    return true;
}

//sendworker.h
#ifndef SENDWORKER_H
#define SENDWORKER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QThread>
#include <QDebug>
#include <atomic>
#include "../configmanager.h"

class SendWorker : public QObject
{
    Q_OBJECT
public:
    explicit SendWorker(QObject *parent = nullptr);
    ~SendWorker();

    /**
     * @brief 注入共享的配置管理器
     * @param mgr 外部维护的 ConfigManager 指针（线程安全）
     */
    void setConfigManager(ConfigManager* mgr);

    void requestStop();

public slots:
    /**
     * @brief 发送命令主流程
     * @param address 目标地址
     * @param port 目标端口
     * @param comname 指令显示名称（键）
     * @param commandIdMap 指令名称到主题号映射
     * @param workIdMap 指令名称到工作ID映射
     * @param timeMap 指令名称到反馈超时（秒）映射
     * @param comCrc 是否跳过命令 CRC
     * @param reCrc 是否跳过反馈 CRC
     * @param comReply 是否模拟无应答
     * @param havereturn 是否模拟无反馈
     * @param ReReply 是否模拟反馈无应答
     * @param replycrc 是否跳过应答 CRC
     * @param returnErr 是否模拟反馈错误
     * @param noShake 是否模拟不握手
     * @param isReplyTimeout 是否模拟应答超时
     * @param isReturnTimeout 是否模拟反馈超时
     * @param isReReplyTimeout 是否模拟反馈应答超时
     * @param replyTimeoutMs 应答超时基础值（毫秒），<=0 则使用默认
     * @param returnTimeoutMs 反馈超时基础值（毫秒），<=0 则使用配置（秒->毫秒）
     * @param reReplyTimeoutMs 反馈应答超时基础值（毫秒），<=0 则使用默认
     * @param flowMode 流程模式：0=手动精确，1=快速（默认×0.01），2=正常（默认×0.8）
     */
    void sendCommand(const QString &address, quint16 port,
                     const QString &comname, const QMap<QString, int>& commandIdMap,
                     const QMap<QString, int>& workIdMap, const QMap<QString, double>& timeMap,
                     bool comCrc, bool reCrc, bool comReply, bool havereturn,
                     bool ReReply, bool replycrc, bool returnErr, bool noShake,
                     bool isReplyTimeout, bool isReturnTimeout, bool isReReplyTimeout,
                     int replyTimeoutMs, int returnTimeoutMs, int reReplyTimeoutMs,
                     int flowMode);

signals:
    void logMessage(QString level, QString message);
    void finished();

private:
    QUdpSocket* m_sender;
    ConfigManager* m_configMgr = nullptr;  ///< 共享配置管理器（由外部注入）
    void crcData(unsigned char* data, int len);
    std::atomic<bool> m_stopRequested{false};
    bool sleepInterruptible(int ms);
};

#endif //SENDWORKER_H

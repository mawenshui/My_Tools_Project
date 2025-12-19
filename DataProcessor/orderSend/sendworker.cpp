//sendworker.cpp
#include <QRegularExpression>
#include "sendworker.h"
#include "../alldefine.h"


#define SHAKE_TIME 200 //应答超时时间
#define RE_SHAKE_TIME 500 //反馈应答超时时间

static QRegularExpression g_HexrRegex("【.*?】");  //匹配十六进制ID

SendWorker::SendWorker(QObject *parent) : QObject(parent)
{
    m_sender = new QUdpSocket(this);
}

SendWorker::~SendWorker()
{
    m_sender->deleteLater();
}

void SendWorker::requestStop()
{
    m_stopRequested.store(true);
}

void SendWorker::crcData(unsigned char* data, int len)
{
    unsigned int sum = data[0];
    for(int i = 1; i < len - 2; i++)
    {
        sum += data[i];
    }
    int calCrc = (sum ^ (sum >> 16)) & 0xffff;
    data[len - 2] = calCrc & 0xff;
    data[len - 1] = (calCrc >> 8) & 0xff;
}

void SendWorker::sendCommand(const QString &address, quint16 port,
                             const QString &comname, const QMap<QString, int>& commandIdMap,
                             const QMap<QString, int>& workIdMap, const QMap<QString, double>& timeMap,
                             bool comCrc, bool reCrc, bool comReply, bool havereturn,
                             bool ReReply, bool replycrc, bool returnErr, bool noShake,
                             bool isReplyTimeout, bool isReturnTimeout, bool isReReplyTimeout,
                             int replyTimeoutMs, int returnTimeoutMs, int reReplyTimeoutMs,
                             int flowMode)
{
    m_stopRequested.store(false);
    if(!commandIdMap.contains(comname) || m_stopRequested.load())
    {
        emit logMessage("ERROR", QString("未找到指令: [0x%1]").arg(comname));
        emit finished();
        return;
    }
    QString l_comname = comname;
    emit logMessage("DEBUG", QString("发送线程接收到超时(ms)：应答=%1，反馈=%2，反馈应答=%3，流程模式=%4")
                                   .arg(replyTimeoutMs).arg(returnTimeoutMs).arg(reReplyTimeoutMs)
                                   .arg(flowMode));
    int commandId = commandIdMap.value(comname, 0);
    int workId = workIdMap.value(comname, 0);
    // 配置中的反馈超时（单位：秒），用于作为默认的反馈超时基础
    double timei = timeMap.value(comname, 0.0);
    // 勾选框优先：若任一勾选，则仅对被勾选的类型应用延迟；
    // 若全部未勾选：根据流程模式（快速/正常/手动）计算三种延时
    bool anyChecked = isReplyTimeout || isReturnTimeout || isReReplyTimeout;
    bool applyReplyDelay = anyChecked ? isReplyTimeout : true;
    bool applyReturnDelay = anyChecked ? isReturnTimeout : true;
    bool applyReReplyDelay = anyChecked ? isReReplyTimeout : true;

    // 读取倍率（带回退与安全限制）
    double mulFast = FLOW_FAST_MULTIPLIER;
    double mulNormal = FLOW_NORMAL_MULTIPLIER;
    double mulChecked = TIMEOUT_CHECKED_MULTIPLIER;
    if(m_configMgr)
    {
        mulFast = m_configMgr->get("flowFastMultiplier", FLOW_FAST_MULTIPLIER).toDouble();
        mulNormal = m_configMgr->get("flowNormalMultiplier", FLOW_NORMAL_MULTIPLIER).toDouble();
        mulChecked = m_configMgr->get("timeoutCheckedMultiplier", TIMEOUT_CHECKED_MULTIPLIER).toDouble();
    }
    if(mulFast <= 0) mulFast = FLOW_FAST_MULTIPLIER;
    if(mulNormal <= 0) mulNormal = FLOW_NORMAL_MULTIPLIER;
    if(mulChecked <= 0) mulChecked = TIMEOUT_CHECKED_MULTIPLIER;

    // 线程开始处标注模式与实际延时值（与UI规则保持一致）：
    // 默认基线（来自常量与配置）
    const int defaultReplyMs = SHAKE_TIME;
    const int defaultReturnMs = static_cast<int>(timei * 1000.0);
    const int defaultReReplyMs = RE_SHAKE_TIME;
    // 计算（未勾选时）三种延时的实际值：依据流程模式
    int manualReplyMs = 0;
    int manualReturnMs = 0;
    int manualReReplyMs = 0;
    if(!anyChecked)
    {
        if(flowMode == 1) // 快速：默认×mulFast
        {
            manualReplyMs   = static_cast<int>(defaultReplyMs   * mulFast);
            manualReturnMs  = static_cast<int>(defaultReturnMs  * mulFast);
            manualReReplyMs = static_cast<int>(defaultReReplyMs * mulFast);
        }
        else if(flowMode == 2) // 正常：默认×mulNormal
        {
            manualReplyMs   = static_cast<int>(defaultReplyMs   * mulNormal);
            manualReturnMs  = static_cast<int>(defaultReturnMs  * mulNormal);
            manualReReplyMs = static_cast<int>(defaultReReplyMs * mulNormal);
        }
        else // 手动：使用手动毫秒值（无额外倍率）
        {
            manualReplyMs   = (replyTimeoutMs   > 0) ? replyTimeoutMs   : defaultReplyMs;
            manualReturnMs  = (returnTimeoutMs  > 0) ? returnTimeoutMs  : defaultReturnMs;
            manualReReplyMs = (reReplyTimeoutMs > 0) ? reReplyTimeoutMs : defaultReReplyMs;
        }
    }

    const int effectiveReplyMs   = applyReplyDelay   ? (anyChecked ? static_cast<int>(defaultReplyMs   * mulChecked) : manualReplyMs)   : 0;
    const int effectiveReturnMs  = applyReturnDelay  ? (anyChecked ? static_cast<int>(defaultReturnMs  * mulChecked) : manualReturnMs)  : 0;
    const int effectiveReReplyMs = applyReReplyDelay ? (anyChecked ? static_cast<int>(defaultReReplyMs * mulChecked) : manualReReplyMs) : 0;

    QString replyMode, returnMode, reReplyMode;
    if(anyChecked)
    {
        const QString mStr = QStringLiteral("默认×%1").arg(QString::number(mulChecked, 'g', 3));
        replyMode   = isReplyTimeout   ? mStr : QStringLiteral("未勾选（不延时）");
        returnMode  = isReturnTimeout  ? mStr : QStringLiteral("未勾选（不延时）");
        reReplyMode = isReReplyTimeout ? mStr : QStringLiteral("未勾选（不延时）");
    }
    else
    {
        const QString fastStr   = QStringLiteral("快速流程（%1×)").arg(QString::number(mulFast, 'g', 3));
        const QString normalStr = QStringLiteral("正常流程（%1×)").arg(QString::number(mulNormal, 'g', 3));
        const QString manualStr = QStringLiteral("手动配置（精确）");
        QString flowModeStr = (flowMode==1) ? fastStr : ((flowMode==2) ? normalStr : manualStr);
        replyMode = flowModeStr;
        returnMode = flowModeStr;
        reReplyMode = flowModeStr;
    }

    emit logMessage("DEBUG", QString("线程超时模式与延时：应答=%1/%2ms，反馈=%3/%4ms，反馈应答=%5/%6ms")
                                   .arg(replyMode).arg(effectiveReplyMs)
                                   .arg(returnMode).arg(effectiveReturnMs)
                                   .arg(reReplyMode).arg(effectiveReReplyMs));
    unsigned char sendData[256] = {0};
    int dataLen = 0;
    sendData[0] = noShake ? 0x80 : 0x00;
    sendData[5] = 0x11;
    //根据命令ID构建不同结构的数据包
    switch(commandId)
    {
        case 0xca:
        case 0xc0:
        case 0xcC:
        case 3405:
        case 3406:
        case 3408:
        case 2851:
        case 2859:
        case 2862:
            dataLen = 16;
            sendData[1] = dataLen - 13;
            *((unsigned short*)(sendData + 3)) = commandId;
            *((unsigned short*)(sendData + 12)) = workId;
            break;
        case 0xc2:
        case 3409:
            dataLen = 17;
            sendData[1] = dataLen - 13;
            *((unsigned short*)(sendData + 3)) = commandId;
            *((unsigned short*)(sendData + 12)) = workId;
            break;
        case 2801:
            dataLen = 39;
            sendData[1] = dataLen - 13;
            *((unsigned short*)(sendData + 3)) = commandId;
            break;
        case 2802:
            dataLen = 16;
            sendData[1] = dataLen - 13;
            sendData[12] = 0x31;
            *((unsigned short*)(sendData + 3)) = commandId;
            break;
        case 2803:
        case 3418:
            dataLen = 15;
            sendData[1] = dataLen - 13;
            *((unsigned short*)(sendData + 3)) = commandId;
            break;
        case 2805:
        case 2806:
            dataLen = 38;
            sendData[1] = dataLen - 13;;
            *((unsigned short*)(sendData + 3)) = commandId;
            break;
        case 3302:
        case 2809:
        case 3411:
        case 3420:
        case 3424:
        case 3425:
        case 3601:
        case 3602:
        case 3604:
        case 3605:
        case 3606:
        case 3607:
        case 3608:
        case 3609:
        case 3702:
        case 3361:
        case 3362:
        case 3363:
        case 3364:
            dataLen = 14;
            sendData[1] = dataLen - 13;;
            *((unsigned short*)(sendData + 3)) = commandId;
            break;
        case 3711:
            dataLen = 14;
            sendData[1] = dataLen - 13;;
            sendData[6] = 0x55;
            *((unsigned short*)(sendData + 3)) = commandId;
            break;
        case 13711:
            dataLen = 14;
            sendData[1] = dataLen - 13;;
            sendData[6] = 0x56;
            commandId = 3711;
            *((unsigned short*)(sendData + 3)) = commandId;
            commandId = 13711;
            break;
        case 3701:
            dataLen = 14;
            commandId = 3701;
            sendData[1] = dataLen - 13;;
            sendData[6] = 0x55;
            *((unsigned short*)(sendData + 3)) = commandId;
            break;
        case 13701:
            dataLen = 14;
            commandId = 3701;
            sendData[1] = dataLen - 13;;
            sendData[6] = 0x56;
            *((unsigned short*)(sendData + 3)) = commandId;
            commandId = 13701;
            break;
        case 3410:
            dataLen = 22;
            sendData[1] = dataLen - 13;;
            *((unsigned short*)(sendData + 3)) = commandId;
            break;
        default:
            dataLen = 16;
            emit logMessage("ERROR", QString("未找到主题号: [0x%1]").arg(QString::number(commandId, 16).toUpper()));
            emit finished();
            return;
    }
    if(!comCrc)
    {
        crcData(sendData, dataLen);
    }
    QString data = QByteArray::fromRawData(reinterpret_cast<char*>(sendData), dataLen).toHex(' ').toUpper();
    emit logMessage("INFO", QString("发送指令: [%1]").arg(data));
    qint64 sendlen = m_sender->writeDatagram(QByteArray::fromRawData(reinterpret_cast<char*>(sendData), dataLen), dataLen, QHostAddress(address), port);
    if(sendlen == -1)
    {
        emit logMessage("ERROR", QString("发送失败: [%1]").arg(m_sender->errorString()));
    }
    if (!sleepInterruptible(5))
    {
        emit logMessage("WARN", "发送被中断: 主命令发送后等待阶段");
        emit finished();
        return;
    }
    //处理命令应答（如果未跳过）
    if(!comReply && !noShake)
    {
        if (m_stopRequested.load())
        {
            emit logMessage("WARN", "发送被中断: 进入应答阶段前");
            emit finished();
            return;
        }
        memset(sendData, 0, sizeof(sendData));
        dataLen = 17;
        sendData[1] = dataLen - 13;
        sendData[5] = 0x11;
        *((unsigned short*)(sendData + 3)) = 0x2c24; //应答命令ID
        int temp_commandId;
        if(commandId == 13701 || commandId == 3701)
        {
            temp_commandId = 3701;
        }
        else if(commandId == 13711 || commandId == 3711)
        {
            temp_commandId = 3711;
        }
        else
        {
            temp_commandId = commandId;
        }
        *((unsigned short*)(sendData + 11)) = temp_commandId;
        sendData[13] = 1; //成功标志
        if(!replycrc)
        {
            crcData(sendData, dataLen);
        }
        // 应答延时：统一使用前面计算的 effectiveReplyMs（勾选→默认×1.5，未勾选→按流程模式或手动）
        if(applyReplyDelay)
        {
            int outtime = effectiveReplyMs;
            if (!sleepInterruptible(outtime))
            {
                emit logMessage("WARN", "发送被中断: 应答延时阶段");
                emit finished();
                return;
            }
            emit logMessage("DEBUG", QString("指令应答延时[%1]毫秒").arg(outtime));
        }
        //记录并发送应答
        QString replyData = QByteArray::fromRawData(reinterpret_cast<char*>(sendData), dataLen).toHex(' ').toUpper();
        emit logMessage("INFO", QString("发送应答: [%1]").arg(replyData));
        sendlen = m_sender->writeDatagram(QByteArray::fromRawData(reinterpret_cast<char*>(sendData), dataLen), dataLen, QHostAddress(address), port);
    }
    //处理反馈数据（如果未跳过）
    if(!havereturn)
    {
        if (m_stopRequested.load())
        {
            emit logMessage("WARN", "发送被中断: 进入反馈阶段前");
            emit finished();
            return;
        }
        memset(sendData, 0, sizeof(sendData));
        sendData[5] = 0x11;
        //根据命令ID构建不同的反馈数据结构
        switch(commandId)
        {
            case 0xca:
            case 0xc0:
            case 0xcC:
                dataLen = 20;
                sendData[1] = dataLen - 13;
//               commandId++;
                if(commandId == 0xca)
                {
                    *((unsigned short*)(sendData + 3)) = 0xcb;
                }
                else if(commandId == 0xc0)
                {
                    *((unsigned short*)(sendData + 3)) = 0xc1;
                }
                else if(commandId == 0xcc)
                {
                    *((unsigned short*)(sendData + 3)) = 0xcd;
                }
                else
                {
                    emit logMessage("ERROR", QString("未找到指令[0x%1]的反馈主题号").arg(QString::number(commandId, 16).toUpper()));
                    emit finished();
                    return;
                }
                *((unsigned short*)(sendData + 11)) = workId;
                sendData[16] = 0;
                if(!returnErr)
                {
                    sendData[15] = 0x1;
                }
                else
                {
                    sendData[15] = 0x2;
                }
                break;
            case 0xc2:
                dataLen = 20;
                sendData[1] = dataLen - 13;
                commandId = 0xC3;
                *((unsigned short*)(sendData + 3)) = commandId;
                *((unsigned short*)(sendData + 11)) = workId;
                sendData[16] = 0;
                if(!returnErr)
                {
                    sendData[14] = 0x1;
                }
                else
                {
                    sendData[14] = 0x2;
                }
                break;
            case 2801:
                dataLen = 40;
                sendData[1] = dataLen - 13;
                commandId = 2841;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[13] = 0xAA;
                }
                else
                {
                    sendData[13] = 0x0;
                }
                break;
            case 2802:
                dataLen = 16;
                sendData[1] = dataLen - 13;
                sendData[12] = 0x31;
                commandId = 2842;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[13] = 0xAA;
                }
                else
                {
                    sendData[13] = 0x0;
                }
                break;
            case 2803:
                dataLen = 15;
                sendData[1] = dataLen - 13;
                commandId = 2843;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[13] = 0xAA;
                }
                else
                {
                    sendData[13] = 0x0;
                }
                break;
            case 2805:
                dataLen = 38;
                sendData[1] = dataLen - 13;
                commandId = 2845;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[14] = 0xAA;
                }
                else
                {
                    sendData[14] = 0x0;
                }
                break;
            case 2806:
                dataLen = 29;
                sendData[1] = dataLen - 13;
                commandId = 2846;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[13] = 0xAA;
                }
                else
                {
                    sendData[13] = 0x0;
                }
                break;
            case 2809:
                dataLen = 14;
                sendData[1] = dataLen - 13;
                commandId = 2849;
                *((unsigned short*)(sendData + 3)) = commandId;
                break;
            case 2851:
            case 2859:
            case 2862:
                dataLen = 14;
                sendData[1] = dataLen - 13;
                commandId += 1;
                *((unsigned short*)(sendData + 3)) = commandId;
                break;
            case 3420:
                dataLen = 47;
                sendData[1] = dataLen - 13;
                commandId = 3304;
                *((unsigned short*)(sendData + 3)) = commandId;
                break;
            case 3405:
                dataLen = 46;
                sendData[1] = dataLen - 13;
                commandId = 3305;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[12] = 0xAA;
                }
                else
                {
                    sendData[12] = 0x0;
                }
                break;
            case 3406:
                dataLen = 22;
                sendData[1] = dataLen - 13;
                commandId = 3306;
                *((unsigned short*)(sendData + 3)) = commandId;
                break;
            case 3408:
                dataLen = 15;
                sendData[1] = dataLen - 13;
                commandId = 3308;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[12] = 0xAA;
                    sendData[13] = 0xAA;
                }
                else
                {
                    sendData[12] = 0x0;
                    sendData[13] = 0x0;
                }
                break;
            case 3409:
                dataLen = 27;
                sendData[1] = dataLen - 13;
                commandId = 3309;
                *((unsigned short*)(sendData + 3)) = commandId;
                break;
            case 3410:
                dataLen = 32;
                sendData[1] = dataLen - 13;
                commandId = 3310;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[20] = 17;
                    sendData[29] = 17;
                }
                else
                {
                    sendData[20] = 85;
                    sendData[29] = 85;
                }
                break;
            case 3411:
                dataLen = 24;
                sendData[1] = dataLen - 13;
                commandId = 3311;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[17] = 17;
                }
                else
                {
                    sendData[17] = 85;
                }
                break;
            case 3425:
                dataLen = 19;
                sendData[1] = dataLen - 13;
                commandId = 3326;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[12] = 0xAA;
                }
                else
                {
                    sendData[12] = 0x0;
                }
                break;
            case 3424:
                dataLen = 24;
                sendData[1] = dataLen - 13;
                commandId = 3324;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[17] = 17;
                }
                else
                {
                    sendData[17] = 85;
                }
                break;
            case 3418:
                dataLen = 15;
                sendData[1] = dataLen - 13;
                commandId = 3318;
                *((unsigned short*)(sendData + 3)) = commandId;
                break;
            case 3601:
            case 3602:
            case 3302:
                dataLen = 24;
                sendData[1] = dataLen - 13;
                commandId += 100;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[12] = 0x55;
                }
                else
                {
                    sendData[12] = 0x0;
                }
                break;
            case 3361:
            case 3362:
            case 3363:
            case 3364:
            case 3604:
            case 3605:
                dataLen = 24;
                sendData[1] = dataLen - 13;
                commandId -= 100;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[13] = 0x1;
                }
                else
                {
                    sendData[13] = 0x2;
                }
                break;
            case 3606:
                dataLen = 24;
                sendData[1] = dataLen - 13;
                commandId -= 100;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[14] = 0x1;
                }
                else
                {
                    sendData[14] = 0x2;
                }
                break;
            case 3607:
                dataLen = 24;
                sendData[1] = dataLen - 13;
                commandId -= 100;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[15] = 0x1;
                }
                else
                {
                    sendData[15] = 0x2;
                }
                break;
            case 3608:
            case 3609:
                dataLen = 15;
                sendData[1] = dataLen - 13;
                commandId -= 100;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[12] = 0x1;
                }
                else
                {
                    sendData[12] = 0x2;
                }
                break;
            case 3711:
                dataLen = 24;
                sendData[1] = dataLen - 13;
                commandId = 3802;
                sendData[5] = 0x1;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[15] = 0xAA;
                }
                else
                {
                    sendData[15] = 0x0;
                }
                break;
            case 13711:
                dataLen = 24;
                sendData[1] = dataLen - 13;
                commandId = 3802;
                sendData[5] = 0x2;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[15] = 0xAA;
                }
                else
                {
                    sendData[15] = 0x0;
                }
                break;
            case 3701:
                dataLen = 15;
                sendData[6] = 0x55;
                sendData[1] = dataLen - 13;
                commandId = 3601;
                *((unsigned short*)(sendData + 3)) = commandId;
                break;
            case 13701:
                dataLen = 15;
                sendData[6] = 0x56;
                sendData[1] = dataLen - 13;
                commandId = 3601;
                *((unsigned short*)(sendData + 3)) = commandId;
                break;
            case 3702:
                dataLen = 15;
                sendData[1] = dataLen - 13;
                commandId = 3806;
                *((unsigned short*)(sendData + 3)) = commandId;
                if(!returnErr)
                {
                    sendData[17] = 0xAA;
                }
                else
                {
                    sendData[17] = 0x0;
                }
                break;
            default:
                emit logMessage("ERROR", QString("未找到反馈主题号: [0x%1]").arg(QString::number(commandId, 16).toUpper()));
                emit finished();
                return;
        }
        //计算反馈数据的CRC校验（如果未跳过）
        if(!reCrc)
        {
            crcData(sendData, dataLen);
        }
        // 反馈延时：统一使用前面计算的 effectiveReturnMs（勾选→默认×1.5，未勾选→按流程模式或手动）
        if(applyReturnDelay)
        {
            int outtime = effectiveReturnMs;
            if (!sleepInterruptible(outtime))
            {
                emit logMessage("WARN", "发送被中断: 反馈延时阶段");
                emit finished();
                return;
            }
            emit logMessage("DEBUG", QString("反馈延时[%1]毫秒").arg(outtime));
        }
        //记录并发送反馈
        QString feedbackStr = QByteArray::fromRawData(reinterpret_cast<char*>(sendData), dataLen).toHex(' ').toUpper();
        emit logMessage("INFO", QString("发送反馈: [%1]").arg(feedbackStr));
        sendlen = m_sender->writeDatagram(QByteArray::fromRawData(reinterpret_cast<char*>(sendData), dataLen), dataLen, QHostAddress(address), port);
        //处理反馈应答（如果未跳过）
        if(!ReReply && !noShake)
        {
            if (m_stopRequested.load())
            {
                emit logMessage("WARN", "发送被中断: 进入反馈应答阶段前");
                emit finished();
                return;
            }
            memset(sendData, 0, sizeof(sendData));
            sendData[5] = 0x11;
            dataLen = 17;
            sendData[1] = dataLen - 13;
            *((unsigned short*)(sendData + 3)) = 0x2c24;
            int temp_commandId;
            temp_commandId = commandId;
            *((unsigned short*)(sendData + 11)) = temp_commandId;
            sendData[13] = 1;
            if(!replycrc)
            {
                crcData(sendData, dataLen);
            }
            // 反馈应答延时：统一使用前面计算的 effectiveReReplyMs（勾选→默认×1.5，未勾选→按流程模式或手动）
            if(applyReReplyDelay)
            {
                int outtime = effectiveReReplyMs;
                if (!sleepInterruptible(outtime))
                {
                    emit logMessage("WARN", "发送被中断: 反馈应答延时阶段");
                    emit finished();
                    return;
                }
                emit logMessage("DEBUG", QString("反馈应答延时[%1]毫秒").arg(outtime));
            }
            //记录并发送反馈应答
            QString feedbackReplyStr = QByteArray::fromRawData(reinterpret_cast<char*>(sendData), dataLen).toHex(' ').toUpper();
            emit logMessage("INFO", QString("发送反馈应答: [%1]").arg(feedbackReplyStr));
            sendlen = m_sender->writeDatagram(QByteArray::fromRawData(reinterpret_cast<char*>(sendData), dataLen), dataLen, QHostAddress(address), port);
        }
    }
    emit logMessage("INFO", QString("指令发送完成: [%1]").arg(comname));
    emit finished();
}
void SendWorker::setConfigManager(ConfigManager* mgr)
{
    m_configMgr = mgr;
}

bool SendWorker::sleepInterruptible(int ms)
{
    if (ms <= 0)
    {
        return !m_stopRequested.load();
    }
    int remaining = ms;
    const int step = 50;
    while (remaining > 0)
    {
        if (m_stopRequested.load())
        {
            return false;
        }
        int chunk = remaining > step ? step : remaining;
        QThread::msleep(static_cast<unsigned long>(chunk));
        remaining -= chunk;
    }
    return !m_stopRequested.load();
}

#include "customdatasender.h"
#include <QRegExp> //用于正则表达式处理

//构造函数，初始化成员变量并连接信号槽
CustomDataSender::CustomDataSender(QTextEdit *textEdit,
                                   QLineEdit *addrEdit,
                                   QLineEdit *intervalEdit,
                                   QCheckBox *loopCheck,
                                   QPushButton *startBtn,
                                   QPushButton *pauseBtn,
                                   QPushButton *stopBtn,
                                   QTextEdit *logView,
                                   QObject *parent)
    : QObject(parent),
      m_textEdit(textEdit),      //数据输入框
      m_addrEdit(addrEdit),      //地址输入框
      m_intervalEdit(intervalEdit), //发送间隔输入框
      m_loopCheck(loopCheck),    //循环发送复选框
      m_startBtn(startBtn),      //开始按钮
      m_pauseBtn(pauseBtn),      //暂停按钮
      m_stopBtn(stopBtn),        //停止按钮
      m_logView(logView),        //日志显示框
      m_udpSocket(nullptr),      //初始化为空
      m_timer(new QTimer(this)), //初始化定时器
      m_currentFrameIndex(0)     //当前帧索引初始化为0
{
    //连接按钮点击事件到槽函数
    connect(m_startBtn, &QPushButton::clicked, this, &CustomDataSender::onStartClicked);
    connect(m_pauseBtn, &QPushButton::clicked, this, &CustomDataSender::onPauseClicked);
    connect(m_stopBtn, &QPushButton::clicked, this, &CustomDataSender::onStopClicked);
    connect(m_timer, &QTimer::timeout, this, &CustomDataSender::sendNextFrame); //定时器超时触发发送
    //初始化按钮状态
    m_pauseBtn->setEnabled(false); //暂停按钮默认禁用
    m_stopBtn->setEnabled(false);  //停止按钮默认禁用
    //设置日志显示框不可编辑
    m_logView->setReadOnly(true);
}

//析构函数，清理资源
CustomDataSender::~CustomDataSender()
{
    if (m_udpSocket)
    {
        m_udpSocket->close(); //关闭UDP Socket
        m_udpSocket->deleteLater(); //延迟删除对象
    }
}

//开始按钮点击事件
void CustomDataSender::onStartClicked()
{
    appendLog("尝试启动发送任务");
    //解析地址和端口
    QString addrPort = m_addrEdit->text().trimmed(); //获取地址和端口输入
    QStringList parts = addrPort.split(':'); //按冒号分割
    if (parts.size() != 2)
    {
        appendLog("地址格式错误，应为IP:Port格式", true);
        QMessageBox::critical(nullptr, "错误", "地址格式应为IP:Port");
        return;
    }
    //验证IP地址有效性
    QHostAddress ipCheck(parts[0]);
    if (ipCheck.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol)
    {
        appendLog("无效的IP地址格式", true);
        QMessageBox::critical(nullptr, "错误", "无效的IP地址");
        return;
    }
    //验证组播地址范围（224.0.0.0 - 239.255.255.255）
    bool isMulticast = ipCheck.isMulticast();
    if (isMulticast)
    {
        appendLog(QString("检测到组播地址: %1").arg(parts[0]));
    }
    //验证端口号有效性
    bool ok;
    int port = parts[1].toInt(&ok);
    if (!ok || port < 1 || port > 65535)
    {
        QMessageBox::critical(nullptr, "错误", "无效的端口号");
        return;
    }
    appendLog(QString("解析目标地址：%1:%2").arg(parts[0]).arg(port));
    //解析数据帧
    m_frames = parseFrames();
    if (m_frames.isEmpty())
    {
        appendLog("未检测到有效数据帧", true);
        QMessageBox::critical(nullptr, "错误", "未检测到有效数据帧");
        return;
    }
    appendLog(QString("成功解析 %1 个数据帧").arg(m_frames.size()));
    //初始化目标IP和端口
    m_currentIp = parts[0];
    m_currentPort = port;
    m_currentFrameIndex = 0;
    appendLog("正在初始化UDP连接...");
    //初始化UDP Socket
    if (m_udpSocket)
    {
        m_udpSocket->deleteLater();
    }
    m_udpSocket = new QUdpSocket(this);
    //启动定时器
    m_timer->start(m_intervalEdit->text().toInt());
    //更新按钮状态
    m_startBtn->setEnabled(false);
    m_pauseBtn->setEnabled(true);
    m_stopBtn->setEnabled(true);
}

//暂停按钮点击事件
void CustomDataSender::onPauseClicked()
{
    if (m_timer->isActive())
    {
        m_timer->stop(); //停止定时器
        m_pauseBtn->setText("继续(P)");
    }
    else
    {
        m_timer->start(m_intervalEdit->text().toInt()); //启动定时器
        m_pauseBtn->setText("暂停(P)");
    }
}

//停止按钮点击事件
void CustomDataSender::onStopClicked()
{
    appendLog("用户手动停止发送");
    m_timer->stop(); //停止定时器
    if (m_udpSocket)
    {
        m_udpSocket->close(); //关闭UDP Socket
        m_udpSocket->deleteLater(); //延迟删除对象
        m_udpSocket = nullptr;
    }
    //重置状态
    m_currentFrameIndex = 0;
    m_startBtn->setEnabled(true);
    m_pauseBtn->setEnabled(false);
    m_stopBtn->setEnabled(false);
    m_pauseBtn->setText("暂停(P)");
}

//发送下一帧数据
void CustomDataSender::sendNextFrame()
{
    if (m_currentFrameIndex >= m_frames.size())
    {
        if (m_loopCheck->isChecked())
        {
            appendLog("达到帧列表末尾，循环发送");
            m_currentFrameIndex = 0; //重置帧索引
        }
        else
        {
            appendLog("发送完成，自动停止");
            onStopClicked(); //停止发送
            return;
        }
    }
    if (m_udpSocket)
    {
        QHostAddress targetAddr(m_currentIp); //目标地址
        bool isMulticast = targetAddr.isMulticast(); //判断是否为组播地址
        //设置组播参数
        if (isMulticast)
        {
            //设置TTL（范围1-255）
            m_udpSocket->setSocketOption(QAbstractSocket::MulticastTtlOption, 1);
            //禁用回环（0=关闭，1=开启）
            m_udpSocket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);
        }
        QByteArray frame = m_frames[m_currentFrameIndex]; //获取当前帧数据
        const QString topic = QString("%1%2")
                              .arg(QString::fromLatin1(frame.mid(4, 1).toHex().toUpper()))
                              .arg(QString::fromLatin1(frame.mid(3, 1).toHex().toUpper())); //解析主题号
        qint64 bytesSent = m_udpSocket->writeDatagram(frame, targetAddr, m_currentPort); //发送数据
        //构造日志信息
        QString logPrefix = isMulticast ? "[组播]" : "[单播]";
        if (bytesSent == -1)
        {
            appendLog(QString("%1 发送失败, 主题号[%2]: %3")
                      .arg(logPrefix)
                      .arg(topic)
                      .arg(m_udpSocket->errorString()), true);
        }
        else
        {
            appendLog(QString("%1 发送成功, 主题号[%2], 帧长[%3], 目标[%4:%5]")
                      .arg(logPrefix)
                      .arg(topic)
                      .arg(bytesSent)
                      .arg(m_currentIp)
                      .arg(m_currentPort));
        }
        m_currentFrameIndex++; //更新帧索引
    }
}

//解析输入的十六进制数据帧
QList<QByteArray> CustomDataSender::parseFrames() const
{
    QList<QByteArray> frames;
    const QStringList lines = m_textEdit->toPlainText().split('\n', QString::SkipEmptyParts); //按行分割
    for (const QString &line : lines)
    {
        QByteArray frame = hexStringToByteArray(line.trimmed()); //转换为字节数组
        if (!frame.isEmpty())
        {
            frames.append(frame); //添加到帧列表
        }
    }
    return frames;
}

//将十六进制字符串转换为字节数组
QByteArray CustomDataSender::hexStringToByteArray(const QString &hexStr) const
{
    QString cleanStr = hexStr;
    cleanStr.remove(QRegExp("[^0-9a-fA-F]"));  //移除非十六进制字符
    if (cleanStr.length() % 2 != 0)
    {
        return QByteArray(); //如果长度不是偶数，返回空
    }
    QByteArray byteArray;
    bool conversionOK = true;
    for (int i = 0; i < cleanStr.length(); i += 2)
    {
        const QString byteStr = cleanStr.mid(i, 2); //每两个字符为一字节
        byteArray.append(static_cast<char>(byteStr.toInt(&conversionOK, 16))); //转换为字节
        if (!conversionOK)
        {
            return QByteArray(); //转换失败返回空
        }
    }
    return byteArray;
}

//添加日志到日志显示框
void CustomDataSender::appendLog(const QString &message, bool isError)
{
    if (!m_logView)
    {
        return;
    }
    QString timestamp = QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss.zzz]"); //时间戳
    QString logMsg = timestamp + " " + message; //日志消息
    if (isError)
    {
        m_logView->setTextColor(Qt::red); //错误日志显示为红色
    }
    else
    {
        m_logView->setTextColor(Qt::black); //正常日志显示为黑色
    }
    m_logView->append(logMsg); //添加到日志显示框
}

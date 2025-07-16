#include "sshclient.h"
#include <QDebug>
#include <QFileInfo>
#include <QTimer>

// 连接超时时间（毫秒）
const int CONNECTION_TIMEOUT = 10000;

SSHClient::SSHClient(QObject *parent) : QObject(parent),
    m_process(nullptr),
    m_connectionTimer(nullptr),
    m_connected(false),
    m_networkManager(nullptr),
    m_host(""),
    m_port(22),
    m_username(""),
    m_keyFilePath(""),
    m_keyFilePassword(""),
    m_authMode(KeyFile),
    m_connectionTimeout(CONNECTION_TIMEOUT)
{
    setupProcess();
    // 创建连接超时定时器
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setSingleShot(true);
    connect(m_connectionTimer, &QTimer::timeout, this, &SSHClient::onConnectionTimeout);
    // 创建网络管理器用于连接检测
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &SSHClient::onNetworkReplyFinished);
}

SSHClient::~SSHClient()
{
    disconnect();
    cleanupProcess();
    if(m_networkManager)
    {
        delete m_networkManager;
        m_networkManager = nullptr;
    }
}

void SSHClient::connectToHost(const QString &host, int port, const QString &username,
                              const QString &keyFilePath, const QString &keyFilePassword)
{
    // 保存连接参数
    m_host = host;
    m_port = port;
    m_username = username;
    m_keyFilePath = keyFilePath;
    m_keyFilePassword = keyFilePassword;
    m_authMode = AuthMode::KeyFile;
    // 检查密钥文件是否存在
    QFileInfo keyFile(m_keyFilePath);
    if(!keyFile.exists() || !keyFile.isFile())
    {
        emit connectionError("密钥文件不存在或无效: " + m_keyFilePath);
        return;
    }
    // 确保进程已经设置好
    if(!m_process)
    {
        setupProcess();
    }
    // 如果已经连接，先断开
    if(m_connected)
    {
        disconnect();
    }
    // 构建SSH命令
    QString sshCommand = getSSHCommand();
    // 启动连接超时定时器
    m_connectionTimer->start(m_connectionTimeout);
    // 启动SSH进程
    qDebug() << "执行SSH密钥认证命令：" << sshCommand;
    // 发送命令输出信号，显示在界面上
    emit commandOutput("执行SSH密钥认证命令：" + sshCommand);
    m_process->start(sshCommand);
}

void SSHClient::checkNetworkConnection(const QString &host, int port)
{
    if(m_networkManager)
    {
        // 先尝试ping主机
        // 临时保存当前端口
        int originalPort = m_port;
        // 设置要检测的端口
        m_port = port;
        // 执行ping检测
        pingHost(host);
        // 恢复原始端口
        m_port = originalPort;
    }
    else
    {
        emit networkCheckError("网络管理器未初始化");
    }
}

void SSHClient::disconnect()
{
    if(m_process)
    {
        if(m_process->state() != QProcess::NotRunning)
        {
            // 发送退出命令
            emit commandOutput("$ exit");
            m_process->write("exit\n");
            // 等待进程退出，增加等待时间
            if(!m_process->waitForFinished(5000))
            {
                // 如果进程没有正常退出，先尝试温和终止
                qDebug() << "SSH进程未能正常退出，尝试终止进程...";
                m_process->terminate();
                // 再次等待进程退出
                if(!m_process->waitForFinished(2000))
                {
                    // 如果仍然没有退出，强制终止
                    qDebug() << "SSH进程终止失败，强制关闭进程...";
                    m_process->kill();
                }
            }
            // 确保进程已经完全清理
            m_process->close();
        }
    }
    m_connected = false;
    emit disconnected();
}

void SSHClient::executeCommand(const QString &command)
{
    if(!m_connected || !m_process)
    {
        emit commandError("未连接到服务器，无法执行命令");
        return;
    }
    m_currentCommand = command;
    // 发送命令到SSH进程
    m_process->write((command + "\n").toUtf8());
}

bool SSHClient::isConnected() const
{
    return m_connected;
}

void SSHClient::onProcessStarted()
{
    qDebug() << "SSH进程已启动";
}

void SSHClient::onProcessError(QProcess::ProcessError error)
{
    QString errorMessage;
    switch(error)
    {
        case QProcess::FailedToStart:
            errorMessage = "无法启动SSH进程，请确保已安装SSH客户端";
            break;
        case QProcess::Crashed:
            errorMessage = "SSH进程崩溃";
            break;
        case QProcess::Timedout:
            errorMessage = "SSH进程超时";
            break;
        case QProcess::WriteError:
            errorMessage = "向SSH进程写入数据时出错";
            break;
        case QProcess::ReadError:
            errorMessage = "从SSH进程读取数据时出错";
            break;
        default:
            errorMessage = "SSH进程发生未知错误";
            break;
    }
    qDebug() << "SSH进程错误：" << errorMessage;
    m_connected = false;
    m_connectionTimer->stop();
    emit connectionError(errorMessage);
}

void SSHClient::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString statusText = (exitStatus == QProcess::NormalExit) ? "正常退出" : "崩溃退出";
    qDebug() << "SSH进程已结束，退出代码：" << exitCode << "，退出状态：" << statusText;
    // 停止连接计时器
    m_connectionTimer->stop();
    // 更新连接状态
    bool wasConnected = m_connected;
    m_connected = false;
    // 如果是异常退出且之前是连接状态，发送错误信号
    if(wasConnected && exitStatus == QProcess::CrashExit)
    {
        emit connectionError("SSH进程异常终止，退出代码：" + QString::number(exitCode));
    }
    else
    {
        // 正常断开连接
        emit disconnected();
    }
}

void SSHClient::onProcessReadyReadStandardOutput()
{
    if(!m_process)
    {
        return;
    }
    // 读取标准输出
    QByteArray output = m_process->readAllStandardOutput();
    QString outputStr = QString::fromUtf8(output);
    // 记录所有标准输出到日志
    qDebug() << "SSH标准输出：" << outputStr;
    // 检查是否包含密码提示（由于使用MergedChannels，密码提示现在会出现在标准输出中）
    // 增加更多可能的密码提示检测模式
    if(!m_connected && (
                outputStr.contains("password", Qt::CaseInsensitive) ||
                outputStr.contains("Password", Qt::CaseInsensitive) ||
                outputStr.contains("密码", Qt::CaseInsensitive) ||
                outputStr.contains("passphrase", Qt::CaseInsensitive) ||
                outputStr.contains("'s password:", Qt::CaseInsensitive) ||
                outputStr.contains("password for", Qt::CaseInsensitive) ||
                outputStr.contains("请输入密码", Qt::CaseInsensitive) ||
                outputStr.contains("Enter passphrase", Qt::CaseInsensitive) ||
                outputStr.contains("Enter PIN", Qt::CaseInsensitive) ||
                (outputStr.contains("authentication", Qt::CaseInsensitive) && outputStr.contains("password", Qt::CaseInsensitive))))
    {
        // 根据认证模式发送不同的密码
        qDebug() << "检测到密码提示，发送" << (m_authMode == AuthMode::KeyFile ? "密钥密码" : "账户密码") << "...";
        emit commandOutput(outputStr); // 先输出密码提示
        // 等待一小段时间再发送密码，确保SSH进程已准备好接收
        QTimer::singleShot(100, [this]()
        {
            if(m_process && m_process->state() == QProcess::Running)
            {
                // 根据认证模式选择发送的密码
                // 使用密钥文件密码
                QString passwordToSend = m_keyFilePassword;
                if(!m_keyFilePassword.isEmpty())
                {
                    qDebug() << "发送密钥密码";
                    m_process->write((passwordToSend + "\n").toUtf8());
                }
                else
                {
                    // 密钥没有密码，发送空行
                    qDebug() << "密钥无密码，发送空行";
                    m_process->write("\n");
                }
                qDebug() << "密码已发送";
            }
        });
        return; // 不要继续处理，等待下一次输出
    }
    // 检查是否包含主机密钥确认提示
    if(!m_connected && (
                (outputStr.contains("fingerprint", Qt::CaseInsensitive) && outputStr.contains("yes/no", Qt::CaseInsensitive)) ||
                (outputStr.contains("authenticity", Qt::CaseInsensitive) && outputStr.contains("can't be established", Qt::CaseInsensitive)) ||
                (outputStr.contains("确认", Qt::CaseInsensitive) && outputStr.contains("yes/no", Qt::CaseInsensitive))))
    {
        // 自动确认主机密钥
        qDebug() << "检测到主机密钥确认提示，自动回答yes";
        emit commandOutput(outputStr); // 先输出提示
        m_process->write("yes\n");
        return; // 不要继续处理，等待下一次输出
    }
    // 检查是否是连接成功的输出 - 增加更多可能的成功提示模式
    if(!m_connected && (
                // 常见的命令提示符
                outputStr.contains("$") ||
                outputStr.contains("#") ||
                outputStr.contains(">") ||
                // 登录成功提示
                outputStr.contains("Last login", Qt::CaseInsensitive) ||
                outputStr.contains("Welcome to", Qt::CaseInsensitive) ||
                // 常见的Linux发行版欢迎信息
                outputStr.contains("Ubuntu", Qt::CaseInsensitive) ||
                outputStr.contains("CentOS", Qt::CaseInsensitive) ||
                outputStr.contains("Debian", Qt::CaseInsensitive) ||
                outputStr.contains("Red Hat", Qt::CaseInsensitive) ||
                // 其他可能的成功指示
                (outputStr.contains("login", Qt::CaseInsensitive) && outputStr.contains("success", Qt::CaseInsensitive))))
    {
        m_connected = true;
        m_connectionTimer->stop();
        emit connected();
    }
    // 发送命令输出信号
    if(!outputStr.isEmpty())
    {
        emit commandOutput(outputStr);
    }
}

void SSHClient::onProcessReadyReadStandardError()
{
    if(!m_process)
    {
        return;
    }
    // 读取标准错误
    QByteArray error = m_process->readAllStandardError();
    QString errorStr = QString::fromUtf8(error);
    // 详细记录SSH标准错误输出
    qDebug() << "SSH标准错误输出：" << errorStr;
    // 尽管我们使用MergedChannels，但有些SSH实现可能仍然在标准错误中输出密码提示
    // 检查是否包含密码提示
    if(!m_connected && (
                errorStr.contains("password", Qt::CaseInsensitive) ||
                errorStr.contains("Password", Qt::CaseInsensitive) ||
                errorStr.contains("密码", Qt::CaseInsensitive) ||
                errorStr.contains("passphrase", Qt::CaseInsensitive) ||
                errorStr.contains("'s password:", Qt::CaseInsensitive) ||
                errorStr.contains("password for", Qt::CaseInsensitive) ||
                errorStr.contains("请输入密码", Qt::CaseInsensitive) ||
                (errorStr.contains("authentication", Qt::CaseInsensitive) && errorStr.contains("password", Qt::CaseInsensitive)) ||
                errorStr.contains("Enter passphrase", Qt::CaseInsensitive) ||
                errorStr.contains("Enter PIN", Qt::CaseInsensitive)))
    {
        // 根据认证模式发送不同的密码
        qDebug() << "在标准错误中检测到密码提示，发送" << (m_authMode == AuthMode::KeyFile ? "密钥密码" : "账户密码") << "...";
        emit commandOutput(errorStr); // 先输出密码提示
        // 等待一小段时间再发送密码，确保SSH进程已准备好接收
        QTimer::singleShot(100, [this]()
        {
            if(m_process && m_process->state() == QProcess::Running)
            {
                // 根据认证模式选择发送的密码
                // 使用密钥文件密码
                QString passwordToSend = m_keyFilePassword;
                if(!m_keyFilePassword.isEmpty())
                {
                    qDebug() << "发送密钥密码（从标准错误检测）";
                    m_process->write((passwordToSend + "\n").toUtf8());
                }
                else
                {
                    // 密钥没有密码，发送空行
                    qDebug() << "密钥无密码，发送空行（从标准错误检测）";
                    m_process->write("\n");
                }
                qDebug() << "密码已发送（从标准错误检测）";
            }
        });
        return; // 不要继续处理，等待下一次输出
    }
    // 检查是否包含连接错误
    if(errorStr.contains("Connection refused", Qt::CaseInsensitive) ||
            errorStr.contains("Connection timed out", Qt::CaseInsensitive) ||
            errorStr.contains("No route to host", Qt::CaseInsensitive) ||
            errorStr.contains("Host key verification failed", Qt::CaseInsensitive) ||
            errorStr.contains("Authentication failed", Qt::CaseInsensitive) ||
            errorStr.contains("Permission denied", Qt::CaseInsensitive) ||
            errorStr.contains("Could not resolve hostname", Qt::CaseInsensitive) ||
            errorStr.contains("Bad owner or permissions", Qt::CaseInsensitive) ||
            errorStr.contains("Too many authentication failures", Qt::CaseInsensitive) ||
            errorStr.contains("Operation timed out", Qt::CaseInsensitive) ||
            errorStr.contains("Network is unreachable", Qt::CaseInsensitive) ||
            errorStr.contains("Connection reset by peer", Qt::CaseInsensitive) ||
            errorStr.contains("Broken pipe", Qt::CaseInsensitive))
    {
        // 连接错误
        qDebug() << "检测到连接错误：" << errorStr;
        m_connected = false;
        m_connectionTimer->stop();
        emit connectionError(errorStr.trimmed());
    }
    // 检查是否是密钥相关错误
    else if(errorStr.contains("key", Qt::CaseInsensitive) &&
            (errorStr.contains("error", Qt::CaseInsensitive) ||
             errorStr.contains("invalid", Qt::CaseInsensitive) ||
             errorStr.contains("denied", Qt::CaseInsensitive) ||
             errorStr.contains("wrong", Qt::CaseInsensitive) ||
             errorStr.contains("incorrect", Qt::CaseInsensitive) ||
             errorStr.contains("bad", Qt::CaseInsensitive)))
    {
        // 密钥错误
        qDebug() << "检测到密钥错误：" << errorStr;
        m_connected = false;
        m_connectionTimer->stop();
        emit connectionError("密钥认证失败: " + errorStr.trimmed());
    }
    // 检查是否是警告信息但不影响连接
    else if(errorStr.contains("Warning", Qt::CaseInsensitive) ||
            errorStr.contains("Pseudo-terminal", Qt::CaseInsensitive))
    {
        // 这些通常是警告，不影响连接，只需输出即可
        qDebug() << "SSH警告信息：" << errorStr;
        emit commandOutput(errorStr);
    }
    else if(!errorStr.isEmpty())
    {
        // 发送命令错误信号
        qDebug() << "SSH命令错误：" << errorStr;
        emit commandError(errorStr);
    }
}

void SSHClient::onConnectionTimeout()
{
    if(!m_connected)
    {
        // 连接超时
        if(m_process && m_process->state() != QProcess::NotRunning)
        {
            // 先尝试正常终止进程
            m_process->terminate();
            // 给进程一些时间来终止
            if(!m_process->waitForFinished(2000))
            {
                // 如果进程没有在指定时间内终止，则强制终止
                m_process->kill();
            }
        }
        QString errorMsg = QString("连接超时，请检查以下可能的原因：\n"
                                   "1. 网络连接是否正常\n"
                                   "2. 服务器地址(%1)是否正确\n"
                                   "3. SSH服务是否在端口%2上运行\n"
                                   "4. 防火墙是否允许SSH连接").arg(m_host).arg(m_port);
        emit connectionError(errorMsg);
    }
}

void SSHClient::setupProcess()
{
    // 清理旧进程
    cleanupProcess();
    // 创建新进程
    m_process = new QProcess(this);
    // 设置进程环境
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    m_process->setProcessEnvironment(env);
    // 设置进程启动模式，使用合并通道模式以更好地处理交互式会话
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    // 连接信号和槽
    connect(m_process, &QProcess::started, this, &SSHClient::onProcessStarted);
    connect(m_process, &QProcess::errorOccurred, this, &SSHClient::onProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &SSHClient::onProcessFinished);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &SSHClient::onProcessReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &SSHClient::onProcessReadyReadStandardError);
}

void SSHClient::cleanupProcess()
{
    if(m_process)
    {
        if(m_process->state() != QProcess::NotRunning)
        {
            // 先尝试温和终止进程
            qDebug() << "正在清理SSH进程...";
            m_process->terminate();
            // 等待进程终止
            if(!m_process->waitForFinished(2000))
            {
                // 如果进程没有响应，强制终止
                qDebug() << "SSH进程终止超时，强制关闭进程...";
                m_process->kill();
                m_process->waitForFinished(1000);
            }
            // 确保进程已经完全清理
            m_process->close();
        }
        // 断开所有信号连接，防止删除后的信号触发
        m_process->disconnect();
        delete m_process;
        m_process = nullptr;
    }
}

QString SSHClient::getSSHCommand(const QString &command) const
{
    QString sshCommand;
#ifdef Q_OS_WIN
    // Windows平台使用OpenSSH客户端
    sshCommand = QString("ssh -tt -p %1 -i \"%2\" %3@%4")
                 .arg(m_port)
                 .arg(m_keyFilePath)
                 .arg(m_username)
                 .arg(m_host);
    // 添加选项以自动接受未知主机密钥
    sshCommand += " -o StrictHostKeyChecking=no";
    // 添加选项以禁用主机密钥检查，避免交互式提示
    sshCommand += " -o UserKnownHostsFile=/dev/null";
    // 添加选项以设置连接超时
    sshCommand += " -o ConnectTimeout=10";
    // 添加选项以指定认证方式，优先使用密钥认证
    sshCommand += " -o PreferredAuthentications=publickey,password";
    // 添加选项以禁用批处理模式，确保交互式会话
    sshCommand += " -o BatchMode=no";
#else
    // Linux平台使用系统SSH客户端
    sshCommand = QString("ssh -tt -p %1 -i \"%2\" %3@%4")
                 .arg(m_port)
                 .arg(m_keyFilePath)
                 .arg(m_username)
                 .arg(m_host);
    // 添加选项以自动接受未知主机密钥
    sshCommand += " -o StrictHostKeyChecking=no";
    // 添加选项以禁用主机密钥检查，避免交互式提示
    sshCommand += " -o UserKnownHostsFile=/dev/null";
    // 添加选项以设置连接超时
    sshCommand += " -o ConnectTimeout=10";
    // 添加选项以指定认证方式，优先使用密钥认证
    sshCommand += " -o PreferredAuthentications=publickey,password";
    // 添加选项以禁用批处理模式，确保交互式会话
    sshCommand += " -o BatchMode=no";
#endif
    // 如果有命令，则附加到SSH命令后
    if(!command.isEmpty())
    {
        sshCommand += " \"" + command + "\"";
    }
    return sshCommand;
}

// 判断字符串是否为IP地址
bool SSHClient::isIPAddress(const QString &address)
{
    // 使用正则表达式匹配IPv4地址格式
    QRegExp ipRegex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    return ipRegex.exactMatch(address);
}

void SSHClient::pingHost(const QString &host)
{
    // 判断是否为IP地址
    if(isIPAddress(host))
    {
        qDebug() << "检测到IP地址，使用TCP Socket连接：" << host;
        // 对IP地址使用TCP Socket进行连接测试
        QTcpSocket *socket = new QTcpSocket(this);
        // 设置超时定时器
        QTimer *timeoutTimer = new QTimer(this);
        timeoutTimer->setSingleShot(true);
        // 连接信号槽
        connect(socket, &QTcpSocket::connected, [this, socket, timeoutTimer]()
        {
            qDebug() << "TCP连接成功";
            // 停止超时定时器
            timeoutTimer->stop();
            timeoutTimer->deleteLater();
            emit networkCheckComplete(true);
            // 先断开连接
            socket->disconnectFromHost();
            // 等待断开连接完成
            connect(socket, &QTcpSocket::disconnected, [socket]()
            {
                qDebug() << "TCP连接已断开";
                // 连接断开后再删除socket
                socket->deleteLater();
            });
            // 设置一个安全超时，防止disconnected信号未触发
            QTimer::singleShot(1000, [socket]()
            {
                if(socket->state() != QAbstractSocket::UnconnectedState)
                {
                    qDebug() << "TCP断开连接超时，强制关闭";
                    socket->abort();
                    socket->deleteLater();
                }
            });
        });
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), [this, socket, timeoutTimer](QAbstractSocket::SocketError)
        {
            qCritical() << "TCP连接失败：" << socket->errorString();
            emit networkCheckError(QString("网络连接错误: %1").arg(socket->errorString()));
            emit networkCheckComplete(false);
            // 停止超时定时器
            timeoutTimer->stop();
            timeoutTimer->deleteLater();
            // 安全关闭socket
            socket->abort();
            socket->deleteLater();
        });
        connect(timeoutTimer, &QTimer::timeout, [this, socket, timeoutTimer]()
        {
            // 检查连接状态
            if(socket->state() != QAbstractSocket::ConnectedState)
            {
                qCritical() << "TCP连接超时";
                // 发送错误信号
                emit networkCheckError("网络请求超时");
                emit networkCheckComplete(false);
                // 断开所有连接，防止后续信号触发
                socket->disconnect();
                // 安全关闭socket
                socket->abort();
                socket->deleteLater();
            }
            // 清理定时器
            timeoutTimer->stop();
            timeoutTimer->deleteLater();
        });
        // 开始连接
        socket->connectToHost(host, m_port);
        timeoutTimer->start(5000); // 5秒超时
    }
    else
    {
        qDebug() << "检测到域名，使用HTTP请求：" << "http://" + host;
        // 对非IP地址使用HTTP请求
        QUrl url("http://" + host);
        QNetworkRequest request(url);
        // 设置缓存控制
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        // 发送HEAD请求，只检查连接性，不下载内容
        QNetworkReply *reply = m_networkManager->head(request);
        // 使用定时器处理超时 - 5秒后检查是否需要中止请求
        reply->setProperty("timeoutTime", QDateTime::currentDateTime().addMSecs(5000));
        // 创建检查定时器，每秒检查一次是否超时
        QTimer *checkTimer = new QTimer(this);
        checkTimer->setObjectName("networkTimeoutTimer");
        checkTimer->setInterval(1000); // 每秒检查一次
        connect(checkTimer, SIGNAL(timeout()), this, SLOT(checkNetworkTimeout()));
        checkTimer->start();
        // 存储定时器指针的名称，以便在超时检查中使用
        reply->setProperty("checkTimerName", checkTimer->objectName());
        // 连接错误信号
        connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
                this, &SSHClient::onNetworkReplyError);
    }
}

void SSHClient::onNetworkReplyFinished(QNetworkReply *reply)
{
    // 停止超时检查定时器
    QString timerName = reply->property("checkTimerName").toString();
    if(!timerName.isEmpty())
    {
        QTimer *timer = findChild<QTimer*>(timerName);
        if(timer)
        {
            timer->stop();
            timer->deleteLater();
        }
    }
    if(reply->error() == QNetworkReply::NoError)
    {
        emit networkCheckComplete(true);
    }
    reply->deleteLater();
}

void SSHClient::onNetworkReplyError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error)
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if(reply)
    {
        // 停止超时检查定时器
        QString timerName = reply->property("checkTimerName").toString();
        if(!timerName.isEmpty())
        {
            QTimer *timer = findChild<QTimer*>(timerName);
            if(timer)
            {
                timer->stop();
                timer->deleteLater();
            }
        }
        QString errorString = reply->errorString();
        emit networkCheckError(QString("网络连接错误: %1").arg(errorString));
        emit networkCheckComplete(false);
        reply->deleteLater();
    }
}

void SSHClient::checkNetworkTimeout()
{
    // 查找所有网络请求，检查是否有超时的
    QList<QNetworkReply*> replies = findChildren<QNetworkReply*>();
    QDateTime currentTime = QDateTime::currentDateTime();
    foreach(QNetworkReply *reply, replies)
    {
        if(reply->property("timeoutTime").isValid())
        {
            QDateTime timeoutTime = reply->property("timeoutTime").toDateTime();
            if(currentTime > timeoutTime && reply->isRunning())
            {
                // 请求已超时，中止它
                reply->abort();
                // 停止定时器
                QString timerName = reply->property("checkTimerName").toString();
                if(!timerName.isEmpty())
                {
                    QTimer *timer = findChild<QTimer*>(timerName);
                    if(timer)
                    {
                        timer->stop();
                        timer->deleteLater();
                    }
                }
                // 发出错误信号
                emit networkCheckError("网络请求超时");
                emit networkCheckComplete(false);
            }
        }
    }
}

#include "workerclass.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QElapsedTimer>

//定义日志分类
Q_LOGGING_CATEGORY(workerLog, "[app.WorkerClass]")

WorkerClass::WorkerClass(QObject *parent)
    : QObject(parent)
{
    qCDebug(workerLog) << "工作线程对象已创建";
    emit logMessage("DEBUG", "工作线程对象已创建");
}

WorkerClass::~WorkerClass()
{
    stopProcessing();
    qCDebug(workerLog) << "工作线程对象已销毁";
    emit logMessage("DEBUG", "工作线程对象已销毁");
}

void WorkerClass::configure(const QVariantMap &config)
{
    QMutexLocker locker(&m_mutex);
    m_config = config;
    //验证必要配置参数
    if(!config.contains("dataDir") || m_addrlist.isEmpty())
    {
        qCCritical(workerLog) << "缺失必要配置参数";
        emit logMessage("ERROR", "配置缺失必要参数");
    }
    qCInfo(workerLog) << "工作线程配置完成，数据目录: " << config["dataDir"].toString();
    emit logMessage("DEBUG", "工作线程配置完成，数据目录: " + config["dataDir"].toString());
}

void WorkerClass::startProcessing()
{
    QMutexLocker locker(&m_mutex);
    if(m_running)
    {
        qCWarning(workerLog) << "尝试启动已运行的工作线程";
        emit logMessage("WARN", "工作线程已在运行状态");
        return;
    }
    //初始化运行状态
    m_running = true;
    m_paused = false;
    m_successCount = 0;
    m_failedCount = 0;
    m_files.clear();
    m_sentTopics.clear();
    //初始化 UDP 发送器
    try
    {
        m_udpSender.reset(new UdpSender());
        connect(m_udpSender.data(), &UdpSender::logMessage,
                this, &WorkerClass::logMessage);
    }
    catch(const std::bad_alloc&)
    {
        qCCritical(workerLog) << "UDP 发送器初始化失败（内存不足）";
        emit logMessage("ERROR", "系统资源不足，无法启动");
        m_running = false;
        return;
    }
    qCInfo(workerLog) << "工作线程启动成功";
    emit logMessage("DEBUG", "工作线程启动成功");
    QMetaObject::invokeMethod(this, "process", Qt::QueuedConnection);
}

void WorkerClass::pauseProcessing(bool paused)
{
    QMutexLocker locker(&m_mutex);
    if(m_paused == paused)
    {
        return;
    }
    m_paused = paused;
    if(m_udpSender)
    {
        m_udpSender->pause(paused);
    }
    if(!paused)
    {
        m_pauseCondition.wakeAll();
    }
    emit logMessage("DEBUG", paused ? "处理已暂停" : "处理已恢复");
}

void WorkerClass::stopProcessing()
{
    QMutexLocker locker(&m_mutex);
    if(!m_running)
    {
        qCDebug(workerLog) << "工作线程已停止";
        return;
    }
    qCDebug(workerLog) << "正在停止工作线程...";
    //设置停止标志
    m_running = false;
    m_paused = false;
    //唤醒可能暂停的线程
    m_pauseCondition.wakeAll();
    //释放锁，避免死锁
    locker.unlock();
    //安全停止UdpSender
    if(m_udpSender)
    {
        qCDebug(workerLog) << "正在停止UdpSender...";
        m_udpSender->stop();
        m_udpSender->waitForStop();  //确保等待发送线程结束
        m_udpSender.reset();
        qCDebug(workerLog) << "UdpSender已停止并释放";
    }
    qCDebug(workerLog) << "工作线程已完全停止";
}

bool WorkerClass::isPaused()
{
    QMutexLocker locker(&m_mutex);
    return m_paused;
}

bool WorkerClass::isRunning()
{
    QMutexLocker locker(&m_mutex);
    return m_running;
}

void WorkerClass::setAddrList(const QStringList addrlist)
{
    m_addrlist = addrlist;
}

QStringList WorkerClass::collectFilesRecursive(const QString &path, bool isDesc)
{
    QStringList files;
    QDir currentDir(path);
    //处理当前目录文件（始终正序）
    QStringList currentFiles = currentDir.entryList({"*.txt"}, QDir::Files);
    currentFiles.sort();
    for(const QString &file : currentFiles)
    {
        files.append(currentDir.absoluteFilePath(file));
    }
    //处理子目录（根据 order 参数控制顺序）
    QStringList subDirs = currentDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    subDirs.sort();
    if(isDesc)
    {
        std::reverse(subDirs.begin(), subDirs.end());
    }
    //递归处理子目录
    for(const QString &dir : subDirs)
    {
        files += collectFilesRecursive(currentDir.filePath(dir), isDesc);
    }
    return files;
}

void WorkerClass::process()
{
    QMutexLocker locker(&m_mutex);
    qCInfo(workerLog) << "进入主处理流程";
    //检查配置有效性
    if(!m_config.contains("dataDir") || m_addrlist.isEmpty())
    {
        emit logMessage("ERROR", "配置参数不完整");
        emit finished();
        return;
    }
    QDir dataDir(m_config["dataDir"].toString());
    //递归收集文件
    bool isDesc = m_config["order"].toString().compare("倒序", Qt::CaseInsensitive) == 0;
    m_files = collectFilesRecursive(dataDir.absolutePath(), isDesc);
    if(m_files.isEmpty())
    {
        emit logMessage("WARN", "未找到文本文件: " + dataDir.path());
        emit finished();
        return;
    }
    qCInfo(workerLog) << "已收集文件数:[" << QString::number(m_files.size()) << "] 处理顺序:" << (isDesc ? "倒序" : "正序");
    emit logMessage("DEBUG", "已收集文件数:[" + QString::number(m_files.size()) + "] 处理顺序:" + (isDesc ? "倒序" : "正序"));
    locker.unlock();
    processFiles();
    emit finished();
}

void WorkerClass::processFiles()
{
    const int totalFiles = m_files.size();
    QElapsedTimer timer;
    timer.start();
    for(int i = 0; i < totalFiles; ++i)
    {
        {
            QMutexLocker locker(&m_mutex);
            if(!m_running)
            {
                return;
            }
            //处理暂停状态
            while(m_paused && m_running)
            {
                m_pauseCondition.wait(&m_mutex);
            }
            if(!m_running)
            {
                break;
            }
        } //释放锁，处理文件时不持有锁
        emit logMessage("DEBUG", QString("正在处理第%1个文件：%2").arg(i + 1).arg(m_files[i]));
        processFile(m_files[i]);
        {
            QMutexLocker locker(&m_mutex);
            const int progress = (i + 1) * 100 / totalFiles;
            emit progressUpdated(progress);
            //限流控制
            if(timer.elapsed() < 10)
            {
                QThread::usleep(static_cast<unsigned long>(10 - timer.elapsed()));
            }
            timer.restart();
        }
    }
    emit statsUpdated(m_successCount, m_failedCount);
    qCInfo(workerLog) << "文件处理完成 成功:" << m_successCount
                      << "失败:" << m_failedCount;
    emit logMessage("DEBUG", QString("处理完成 成功: %1 失败: %2")
                    .arg(m_successCount).arg(m_failedCount));
}

void WorkerClass::processFile(const QString &filePath)
{
    QFile file(filePath);
    //打开文件
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const QString error = QString("文件打开失败: %1 (%2)")
                              .arg(filePath).arg(file.errorString());
        qCWarning(workerLog) << error;
        emit logMessage("ERROR", error);
        m_failedCount++;
        return;
    }
    QTextStream stream(&file);
    int lineNum = 0;
    while(!stream.atEnd())
    {
        //检查运行状态
        {
            QMutexLocker locker(&m_mutex);
            if(!m_running)
            {
                break;
            }
            if(m_paused)
            {
                m_pauseCondition.wait(&m_mutex);
            }
        }
        lineNum++;
        const QString line = stream.readLine().trimmed();
        if(line.isEmpty())
        {
            continue;
        }
        //解析数据行
        const QStringList parts = line.split('\t', QString::SkipEmptyParts);
        if(parts.size() < 2)
        {
            const QString warn = QString("%1 第%2行 - 数据格式错误")
                                 .arg(QFileInfo(file).fileName()).arg(lineNum);
            emit logMessage("WARN", warn);
            continue;
        }
        //识别数据格式（支持 [时间戳]\t数据 和 数据\t时间戳 两种格式）
        QString hexDataStr;
        if (parts[0].trimmed().startsWith('['))
        {
            hexDataStr = parts[1];
        }
        else
        {
            hexDataStr = parts[0];
        }

        //数据清洗
        const QByteArray cleanData = cleanHexData(hexDataStr);
        if(cleanData.isEmpty())
        {
            const QString warn = QString("%1 第%2行 - 无效 HEX 数据")
                                 .arg(QFileInfo(file).fileName()).arg(lineNum);
            emit logMessage("WARN", warn);
            continue;
        }
        //提取主题（第 3-4 字节）
        if(cleanData.size() < 4)
        {
            const QString warn = QString("%1 第%2行 - 数据长度不足")
                                 .arg(QFileInfo(file).fileName()).arg(lineNum);
            emit logMessage("WARN", warn);
            m_failedCount++;
            continue;
        }
        //构造主题号（注意字节序）
        const QString topic = QString("%1%2")
                              .arg(QString::fromLatin1(cleanData.mid(4, 1).toHex().toUpper()))
                              .arg(QString::fromLatin1(cleanData.mid(3, 1).toHex().toUpper()));
        //主题过滤
        if(!filterTopic(topic))
        {
            emit logMessage("INFO", "跳过主题:" + topic);
            qCDebug(workerLog) << "跳过主题:" << topic;
            continue;
        }
        //发送到所有目标地址
        const QStringList addresses = m_addrlist;
        for(const QString &addr : addresses)
        {
            if(!m_udpSender->send(addr, cleanData, topic))
            {
                const QString error = QString("发送失败 [%1] [长度：%2] [%3] [%4]").arg(topic).arg(cleanData.size()).arg(addr).arg(QString::fromLatin1(cleanData.toHex().toUpper()));
                emit logMessage("ERROR", error);
                m_failedCount++;
            }
            else
            {
                const QString info = QString("发送成功 [%1] [长度：%2] [%3] [%4]").arg(topic).arg(cleanData.size()).arg(addr).arg(QString::fromLatin1(cleanData.toHex().toUpper()));
                emit logMessage("INFO", info);
                m_successCount++;
            }
        }
        //发送间隔控制
        const int interval = m_config.value("sendInterval", 100).toInt();
        if(interval > 0)
        {
            QThread::msleep(static_cast<unsigned long>(interval));
        }
    }
    file.close();
}

bool WorkerClass::filterTopic(const QString &topic)
{
    QMutexLocker locker(&m_mutex);
    //包含列表检查
    const QStringList include = m_config["includeTopics"].toStringList();
    if(!include.isEmpty() && !include.contains(topic, Qt::CaseInsensitive))
    {
        return false;
    }
    //排除列表检查
    const QStringList exclude = m_config["excludeTopics"].toStringList();
    if(exclude.contains(topic, Qt::CaseInsensitive))
    {
        return false;
    }
    //唯一模式检查
    if(m_config["uniqueMode"].toBool())
    {
        if(m_sentTopics.contains(topic))
        {
            return false;
        }
        m_sentTopics.insert(topic);
    }
    return true;
}

QByteArray WorkerClass::cleanHexData(const QString &data)
{
    //移除所有非十六进制字符
    static QRegularExpression nonHexChar("[^0-9A-Fa-f]");
    QString cleanStr = data;
    cleanStr.remove(nonHexChar);
    //长度校验
    if(cleanStr.isEmpty() || (cleanStr.length() % 2 != 0))
    {
        qCDebug(workerLog) << "无效 HEX 数据:" << data;
        emit logMessage("WARN", "无效 HEX 数据:" + data);
        return QByteArray();
    }
    //转换字节数组
    QByteArray result;
    try
    {
        result = QByteArray::fromHex(cleanStr.toLatin1());
    }
    catch(...)
    {
        qCWarning(workerLog) << "HEX 数据转换异常:" << cleanStr;
        emit logMessage("WARN", "HEX 数据转换异常:" + cleanStr);
        return QByteArray();
    }
    return result;
}

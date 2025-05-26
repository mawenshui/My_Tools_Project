#include "networkinterfacemanager.h"
#include "Logger.h"
#include <QRegularExpression>

NetworkInterfaceManager::NetworkInterfaceManager(QObject *parent)
    : QObject(parent)
{
}

QStringList NetworkInterfaceManager::getNetworkInterfaces()
{
    QString output = executeNetshCommand("interface show interface");
    if(output.isEmpty())
    {
        Logger::warning("Failed to get network interfaces list");
        return QStringList();
    }
    QStringList interfaces;
    // 改进后的正则表达式：严格匹配单行，精确提取网卡名称
    QRegularExpression re(
        "^(已启用|已禁用|Enabled|Disabled)\\s+"      // 管理员状态（忽略）
        "(已连接|已断开连接|Connected|Disconnected)\\s+" // 连接状态（忽略）
        "\\S+\\s+"                                   // 类型（忽略）
        "(\\S+(?:\\s+\\S+)*)(?=\\s*$)",              // 网卡名称（确保匹配到行尾）
        QRegularExpression::MultilineOption
    );
    // 按行分割输出，逐行处理（避免跨行匹配）
    QStringList lines = output.split('\n', QString::SkipEmptyParts);
    for(const QString &line : lines)
    {
        QRegularExpressionMatch match = re.match(line);
        if(match.hasMatch())
        {
            QString interfaceName = match.captured(3).trimmed();
            if(!interfaceName.isEmpty())
            {
                interfaces << interfaceName;
            }
        }
    }
    if(interfaces.isEmpty())
    {
        Logger::warning(QString("No network interfaces found in output: %1").arg(output));
    }
    return interfaces;
}

bool NetworkInterfaceManager::enableInterface(const QString &interfaceName)
{
    QString output = executeNetshCommand(QString("interface set interface \"%1\" enable").arg(interfaceName));
    // 成功情况1：命令执行成功且无输出
    if(output.isEmpty())
    {
        return true;
    }
    // 成功情况2：接口已是启用状态（中文错误提示）
    if(output.contains("已经是启用状态"))
    {
        return true;
    }
    // 其他情况视为失败
    Logger::error(QString("启用网卡 %1 失败: %2").arg(interfaceName).arg(output));
    return false;
}

bool NetworkInterfaceManager::disableInterface(const QString &interfaceName)
{
    QString output = executeNetshCommand(QString("interface set interface \"%1\" disable").arg(interfaceName));
    // 成功情况1：命令执行成功且无输出
    if(output.isEmpty())
    {
        return true;
    }
    // 成功情况2：接口已是禁用状态（中文错误提示）
    if(output.contains("已经是禁用状态"))
    {
        return true;
    }
    // 其他情况视为失败
    Logger::error(QString("禁用网卡 %1 失败: %2").arg(interfaceName).arg(output));
    return false;
}

QString NetworkInterfaceManager::getInterfaceStatus(const QString &interfaceName)
{
    QString adminStatus = getInterfaceAdminStatus(interfaceName);
    QString connStatus = getInterfaceConnStatus(interfaceName);
    return QString("%1|%2").arg(adminStatus).arg(connStatus);
}

QString NetworkInterfaceManager::getInterfaceAdminStatus(const QString &interfaceName)
{
    QString output = executeNetshCommand("interface show interface");
    if(output.isEmpty())
    {
        Logger::warning("Failed to get network interfaces list");
        return "Unknown";
    }
    // 按行分割输出
    QStringList lines = output.split('\n', QString::SkipEmptyParts);
    for(const QString &line : lines)
    {
        // 检查当前行是否包含目标网卡名称
        if(line.contains(interfaceName))
        {
            // 提取管理状态（行首第一个词）
            QStringList parts = line.split(QRegularExpression("\\s+"), QString::SkipEmptyParts);
            if(parts.size() >= 4 && parts[3] == interfaceName)   // 确保至少有4列（状态、连接状态、类型、网卡名）
            {
                QString status = parts[0];
                if(status == "Enabled")
                {
                    return "已启用";
                }
                if(status == "Disabled")
                {
                    return "已禁用";
                }
                return status; // 已经是中文状态
            }
        }
    }
    Logger::warning(QString("无法获取网卡 %1 的管理状态").arg(interfaceName));
    return "Unknown";
}

QString NetworkInterfaceManager::getInterfaceConnStatus(const QString &interfaceName)
{
    QString output = executeNetshCommand("interface show interface");
    if(output.isEmpty())
    {
        Logger::warning("Failed to get network interfaces list");
        return "Unknown";
    }
    // 按行分割输出
    QStringList lines = output.split('\n', QString::SkipEmptyParts);
    for(const QString &line : lines)
    {
        // 检查当前行是否包含目标网卡名称
        if(line.contains(interfaceName))
        {
            // 提取连接状态（行首第二个词）
            QStringList parts = line.split(QRegularExpression("\\s+"), QString::SkipEmptyParts);
            if(parts.size() >= 4 && parts[3] == interfaceName)   // 确保至少有4列（状态、连接状态、类型、网卡名）
            {
                QString status = parts[1];
                if(status == "Connected")
                {
                    return "已连接";
                }
                if(status == "Disconnected")
                {
                    return "已断开连接";
                }
                return status; // 已经是中文状态
            }
        }
    }
    Logger::warning(QString("无法获取网卡 %1 的连接状态").arg(interfaceName));
    return "Unknown";
}

QString NetworkInterfaceManager::executeNetshCommand(const QString &command)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels); // 合并输出和错误流
    process.start("netsh", QStringList() << command.split(" "));
    if(!process.waitForFinished(3000))
    {
        Logger::error(QString("Netsh命令执行超时: %1").arg(command));
        return "ERROR: 命令执行超时";
    }
    QString result = QString::fromLocal8Bit(process.readAll());
    result = result.trimmed(); // 去除首尾空白
    if(process.exitCode() != 0)
    {
        Logger::error(QString("Netsh命令执行失败: %1\n返回码: %2\n输出: %3")
                      .arg(command)
                      .arg(process.exitCode())
                      .arg(result));
    }
    return result;
}

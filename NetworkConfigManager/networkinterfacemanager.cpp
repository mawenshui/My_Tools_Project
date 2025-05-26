#include "NetworkInterfaceManager.h"
#include "Logger.h"
#include <QRegularExpression>

NetworkInterfaceManager::NetworkInterfaceManager(QObject *parent) 
    : QObject(parent)
{
}

QStringList NetworkInterfaceManager::getNetworkInterfaces()
{
    QString output = executeNetshCommand("interface show interface");
    QStringList interfaces;
    
    // 解析netsh输出，示例格式：
    // Admin State    State          Type             Interface Name
    // Enabled        Connected      Dedicated        Ethernet
    QRegularExpression re("(Enabled|Disabled)\\s+\\w+\\s+\\w+\\s+(.+)");
    QRegularExpressionMatchIterator i = re.globalMatch(output);
    
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        interfaces << match.captured(2).trimmed();
    }
    
    return interfaces;
}

bool NetworkInterfaceManager::enableInterface(const QString &interfaceName)
{
    QString output = executeNetshCommand(QString("interface set interface \"%1\" enable").arg(interfaceName));
    return output.contains("Ok.");
}

bool NetworkInterfaceManager::disableInterface(const QString &interfaceName)
{
    QString output = executeNetshCommand(QString("interface set interface \"%1\" disable").arg(interfaceName));
    return output.contains("Ok.");
}

QString NetworkInterfaceManager::getInterfaceStatus(const QString &interfaceName)
{
    QString output = executeNetshCommand("interface show interface");
    QRegularExpression re(QString("(Enabled|Disabled)\\s+\\w+\\s+\\w+\\s+%1").arg(interfaceName));
    QRegularExpressionMatch match = re.match(output);
    
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return "Unknown";
}

QString NetworkInterfaceManager::executeNetshCommand(const QString &command)
{
    QProcess process;
    process.start("netsh", QStringList() << command.split(" "));
    
    if (!process.waitForFinished(3000)) {
        Logger::error(QString("Netsh command timeout: %1").arg(command));
        return "";
    }
    
    if (process.exitCode() != 0) {
        Logger::error(QString("Netsh command failed: %1. Error: %2")
                     .arg(command)
                     .arg(process.readAllStandardError()));
        return "";
    }
    
    return QString::fromLocal8Bit(process.readAllStandardOutput());
}
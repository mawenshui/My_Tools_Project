#include "networkinfocollector.h"
#include "Logger.h"
#include "networkinterfacemanager.h"

NetworkInfoCollector::NetworkInfoCollector(QObject *parent) : QObject(parent)
{
}

bool NetworkInfoCollector::isDhcpEnabled(const QString &output)
{
    QStringList patterns = {"DHCP enabled:\\s*(yes|是|ja|oui)", "DHCP activé"};
    for(const auto &pattern : patterns)
    {
        QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
        if(re.match(output).hasMatch())
        {
            return true;
        }
    }
    return false;
}

QString NetworkInfoCollector::executePowerShellCommand(const QString &command)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    QStringList args;
    args << "-NoProfile" << "-NonInteractive" << "-Command" 
         << "[Console]::OutputEncoding = [System.Text.Encoding]::UTF8; " + command;
    
    process.start("powershell", args);
    if(!process.waitForFinished(4000))
    {
        process.kill();
        Logger::error("PowerShell命令执行超时: " + command);
        return QString();
    }
    
    QByteArray output = process.readAllStandardOutput();
    QString result = QString::fromUtf8(output);
    result.remove('\r');
    result = result.trimmed();
    
    if(process.exitCode() != 0)
    {
        Logger::warning("PowerShell命令执行失败: " + command + " Output: " + result);
    }
    
    return result;
}

bool NetworkInfoCollector::getNetworkConfigViaPowerShell(const QString &rawInterfaceName, QVariantMap &currentConfig)
{
    Logger::debug("尝试使用PowerShell获取网络配置");
    
    QString escapedName = rawInterfaceName;
    escapedName.replace("'", "''");
    QString command = QString(
        "Get-NetIPAddress -InterfaceAlias '%1' -AddressFamily IPv4 | "
        "Where-Object { $_.IPAddress -notlike '127.*' } | "
        "Select-Object IPAddress, PrefixLength, InterfaceAlias | "
        "ConvertTo-Json -Compress"
    ).arg(escapedName);
    
    QString output = executePowerShellCommand(command);
    if(output.isEmpty() || output.contains("error", Qt::CaseInsensitive))
    {
        Logger::debug("PowerShell获取IP失败，尝试下一个方法");
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    if(doc.isNull() || doc.isEmpty())
    {
        Logger::debug("PowerShell返回无效JSON");
        return false;
    }
    
    QJsonObject obj;
    if(doc.isArray())
    {
        QJsonArray arr = doc.array();
        if(arr.isEmpty())
        {
            Logger::debug("PowerShell返回空数组");
            return false;
        }
        obj = arr.first().toObject();
    }
    else
    {
        obj = doc.object();
    }
    
    if(obj.contains("IPAddress"))
    {
        currentConfig["ip"] = obj["IPAddress"].toString();
        Logger::debug(tr("通过PowerShell获取IP地址: %1").arg(currentConfig["ip"].toString()));
    }
    
    // 获取网关
    command = QString(
        "Get-NetRoute -InterfaceAlias '%1' -AddressFamily IPv4 -DestinationPrefix '0.0.0.0/0' | "
        "Select-Object NextHop | "
        "ConvertTo-Json -Compress"
    ).arg(escapedName);
    
    output = executePowerShellCommand(command);
    if(!output.isEmpty() && !output.contains("error", Qt::CaseInsensitive))
    {
        doc = QJsonDocument::fromJson(output.toUtf8());
        if(!doc.isNull())
        {
            if(doc.isArray() && !doc.array().isEmpty())
            {
                obj = doc.array().first().toObject();
            }
            else if(doc.isObject())
            {
                obj = doc.object();
            }
            
            if(obj.contains("NextHop"))
            {
                currentConfig["gateway"] = obj["NextHop"].toString();
                Logger::debug(tr("通过PowerShell获取默认网关: %1").arg(currentConfig["gateway"].toString()));
            }
        }
    }
    
    // 检查DHCP状态
    command = QString(
        "Get-NetIPInterface -InterfaceAlias '%1' -AddressFamily IPv4 | "
        "Select-Object Dhcp | "
        "ConvertTo-Json -Compress"
    ).arg(escapedName);
    
    output = executePowerShellCommand(command);
    if(!output.isEmpty() && !output.contains("error", Qt::CaseInsensitive))
    {
        doc = QJsonDocument::fromJson(output.toUtf8());
        if(!doc.isNull())
        {
            if(doc.isObject())
            {
                obj = doc.object();
            }
            else if(doc.isArray() && !doc.array().isEmpty())
            {
                obj = doc.array().first().toObject();
            }
            
            if(obj.contains("Dhcp"))
            {
                QString dhcpStatus = obj["Dhcp"].toString();
                if(dhcpStatus.toLower() == "enabled")
                {
                    currentConfig["method"] = "dhcp";
                    Logger::debug("通过PowerShell确定为DHCP模式");
                }
                else
                {
                    currentConfig["method"] = "static";
                    Logger::debug("通过PowerShell确定为静态IP模式");
                }
            }
        }
    }
    
    // 通过前缀长度计算子网掩码
    if(obj.contains("PrefixLength"))
    {
        int prefix = obj["PrefixLength"].toInt(-1);
        if(prefix > 0 && prefix <= 32)
        {
            quint32 mask = 0xFFFFFFFF << (32 - prefix);
            QHostAddress subnetMask(mask);
            currentConfig["subnet"] = subnetMask.toString();
            Logger::debug(tr("通过PowerShell获取子网掩码: %1").arg(currentConfig["subnet"].toString()));
        }
    }
    
    return currentConfig.contains("ip") && !currentConfig["ip"].toString().isEmpty();
}

QVariantMap NetworkInfoCollector::getCurrentNetworkConfig(const QString &interfaceName, ConfigManager *configManager)
{
    QString cleanName = configManager->cleanInterfaceName(interfaceName);
    return getCurrentNetworkConfig(interfaceName, cleanName, configManager);
}

QVariantMap NetworkInfoCollector::getCurrentNetworkConfig(const QString &rawInterfaceName, const QString &cleanInterfaceName, ConfigManager *configManager)
{
    Logger::debug("获取当前网卡的网络配置");
    QVariantMap currentConfig;
    currentConfig["interface"] = cleanInterfaceName;
    Logger::debug(tr("查询接口配置: raw=%1, clean=%2").arg(rawInterfaceName).arg(cleanInterfaceName));

    // 先尝试使用 QNetworkInterface 获取IP地址（最可靠）
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    bool foundQNetworkInterface = false;
    for(const QNetworkInterface &currentInterface : interfaces)
    {
        QString sysName = currentInterface.name();
        QString humanName = currentInterface.humanReadableName();
        QString cleanSysName = configManager->cleanInterfaceName(sysName);
        QString cleanHumanName = configManager->cleanInterfaceName(humanName);

        Logger::debug(tr("接口名称匹配: sysName=%1, humanName=%2, cleanSysName=%3, cleanHumanName=%4")
                      .arg(sysName).arg(humanName).arg(cleanSysName).arg(cleanHumanName));

        bool matched = (cleanSysName == cleanInterfaceName) || 
                       (cleanHumanName == cleanInterfaceName) ||
                       (sysName == rawInterfaceName) ||
                       (humanName == rawInterfaceName) ||
                       (sysName.contains(cleanInterfaceName, Qt::CaseInsensitive)) ||
                       (humanName.contains(cleanInterfaceName, Qt::CaseInsensitive));

        if(matched)
        {
            foundQNetworkInterface = true;
            for(const QNetworkAddressEntry &entry : currentInterface.addressEntries())
            {
                if(entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isNull() && entry.ip() != QHostAddress::LocalHost)
                {
                    currentConfig["ip"] = entry.ip().toString();
                    Logger::debug(tr("通过QNetworkInterface获取IP地址: %1").arg(currentConfig["ip"].toString()));
                    if(!entry.netmask().isNull())
                    {
                        currentConfig["subnet"] = entry.netmask().toString();
                        Logger::debug(tr("通过QNetworkInterface获取子网掩码: %1").arg(currentConfig["subnet"].toString()));
                    }
                    break;
                }
            }
            break;
        }
    }

    // 如果 QNetworkInterface 获取到了IP，直接使用netsh获取DHCP状态
    if(foundQNetworkInterface && currentConfig.contains("ip"))
    {
        // 跳过PowerShell，直接使用netsh获取DHCP状态（PowerShell后端在当前环境下持续超时）
        QProcess dhcpProcess;
        dhcpProcess.start("netsh", QStringList() << "interface" << "ip" << "show" << "config" << QString("name=\"%1\"").arg(rawInterfaceName));
        if(!dhcpProcess.waitForFinished(2000))
        {
            dhcpProcess.kill();
            dhcpProcess.waitForFinished(3000);
        }
        QTextCodec *gbkCodec = QTextCodec::codecForName("GBK");
        if(!gbkCodec) gbkCodec = QTextCodec::codecForLocale();
        QString dhcpOutput = gbkCodec->toUnicode(dhcpProcess.readAllStandardOutput());
        bool isDhcp = isDhcpEnabled(dhcpOutput);
        currentConfig["method"] = isDhcp ? "dhcp" : "static";
        Logger::debug(isDhcp ? "通过netsh确定为DHCP模式" : "通过netsh确定为静态IP模式");
        Logger::info("当前网络配置获取完成");
        return currentConfig;
    }

    // 如果 QNetworkInterface 没有获取到IP，尝试使用PowerShell
    if(!foundQNetworkInterface || !currentConfig.contains("ip"))
    {
        if(getNetworkConfigViaPowerShell(rawInterfaceName, currentConfig))
        {
            Logger::info("当前网络配置获取完成");
            return currentConfig;
        }
        
        // 如果PowerShell也失败，尝试用netsh作为最后的备选方案
        Logger::debug("PowerShell也失败，尝试使用netsh");
        QProcess dhcpProcess;
        dhcpProcess.start("netsh", QStringList() << "interface" << "ip" << "show" << "config" << QString("name=\"%1\"").arg(rawInterfaceName));
        if(!dhcpProcess.waitForFinished(2000))
        {
            dhcpProcess.kill();
            dhcpProcess.waitForFinished(3000);
        }
        QTextCodec *gbkCodec = QTextCodec::codecForName("GBK");
        if(!gbkCodec) gbkCodec = QTextCodec::codecForLocale();
        QString dhcpOutput = gbkCodec->toUnicode(dhcpProcess.readAllStandardOutput());
        bool isDhcp = isDhcpEnabled(dhcpOutput);
        Logger::debug(tr("接口 DHCP 状态: %1").arg(isDhcp ? "启用" : "禁用"));

        QProcess ipProcess;
        ipProcess.start("netsh", QStringList() << "interface" << "ip" << "show" << "addresses" << QString("name=\"%1\"").arg(rawInterfaceName));
        if(!ipProcess.waitForFinished(2000))
        {
            ipProcess.kill();
            ipProcess.waitForFinished(3000);
        }
        QString ipOutput = gbkCodec->toUnicode(ipProcess.readAllStandardOutput());

        QRegularExpression ipRegex(R"(IP [Aa]ddress\s*:\s*([0-9.]+))");
        QRegularExpression subnetRegex(R"(Subnet [Pp]refix[^\n]+mask\s+([0-9.]+))");
        QRegularExpression gatewayRegex(R"(Default [Gg]ateway\s*:\s*([0-9.]+))");

        QRegularExpressionMatch ipMatch = ipRegex.match(ipOutput);
        if(ipMatch.hasMatch())
        {
            currentConfig["ip"] = ipMatch.captured(1).trimmed();
            Logger::debug(tr("通过netsh获取IP地址: %1").arg(currentConfig["ip"].toString()));
        }

        QRegularExpressionMatch subnetMatch = subnetRegex.match(ipOutput);
        if(subnetMatch.hasMatch())
        {
            currentConfig["subnet"] = subnetMatch.captured(1).trimmed();
            Logger::debug(tr("通过netsh获取子网掩码: %1").arg(currentConfig["subnet"].toString()));
        }
        else
        {
            QRegularExpression altSubnetRegex(R"(Subnet Mask\s*:\s*([0-9.]+))");
            QRegularExpressionMatch altMatch = altSubnetRegex.match(ipOutput);
            if(altMatch.hasMatch())
            {
                currentConfig["subnet"] = altMatch.captured(1).trimmed();
                Logger::debug(tr("通过netsh获取子网掩码(备用方式): %1").arg(currentConfig["subnet"].toString()));
            }
        }

        QRegularExpressionMatch gatewayMatch = gatewayRegex.match(ipOutput);
        if(gatewayMatch.hasMatch())
        {
            currentConfig["gateway"] = gatewayMatch.captured(1).trimmed();
            Logger::debug(tr("通过netsh获取默认网关: %1").arg(currentConfig["gateway"].toString()));
        }

        currentConfig["method"] = isDhcp ? "dhcp" : "static";
        Logger::debug(isDhcp ? "确定为DHCP模式(通过DHCP状态检测)" : "确定为静态IP模式");
    }
    
    Logger::info("当前网络配置获取完成");
    return currentConfig;
}

QMap<QString, QVariantMap> NetworkInfoCollector::getAllNetworkConfigs(ConfigManager *configManager)
{
    Logger::debug("获取所有网络配置");
    QMap<QString, QVariantMap> allConfigs;
    NetworkInterfaceManager manager;
    QStringList interfaces = manager.getNetworkInterfaces();
    for(const QString &iface : interfaces)
    {
        QString ifaceName = configManager->cleanInterfaceName(iface);
        QVariantMap config = getCurrentNetworkConfig(iface, ifaceName, configManager);
        allConfigs[ifaceName] = config;
    }
    Logger::info("所有网络配置获取完成");
    return allConfigs;
}

bool NetworkInfoCollector::compareConfigs(const QVariantMap &current, const QVariantMap &saved)
{
    if(current.isEmpty() || saved.isEmpty())
    {
        Logger::debug("配置为空，不匹配");
        return false;
    }
    QString currentInterface = ConfigManager::cleanInterfaceName(current["interface"].toString());
    QString savedInterface = ConfigManager::cleanInterfaceName(saved["interface"].toString());
    if(currentInterface != savedInterface)
    {
        Logger::debug(tr("接口不匹配: %1 != %2").arg(currentInterface).arg(savedInterface));
        return false;
    }
    QString currentMethod = current["method"].toString();
    QString savedMethod = saved["method"].toString();
    if(currentMethod != savedMethod)
    {
        Logger::debug(tr("方法不匹配: %1 != %2").arg(currentMethod).arg(savedMethod));
        return false;
    }
    if(currentMethod == "static")
    {
        QString currentIp = current["ip"].toString();
        QString savedIp = saved["ip"].toString();
        if(currentIp != savedIp)
        {
            Logger::debug(tr("IP不匹配: %1 != %2").arg(currentIp).arg(savedIp));
            return false;
        }
        QString currentSubnet = current["subnet"].toString();
        QString savedSubnet = saved["subnet"].toString();
        if(currentSubnet != savedSubnet)
        {
            Logger::debug(tr("子网掩码不匹配: %1 != %2").arg(currentSubnet).arg(savedSubnet));
            return false;
        }
        QString currentGateway = current["gateway"].toString();
        QString savedGateway = saved["gateway"].toString();
        if(currentGateway != savedGateway)
        {
            Logger::debug(tr("网关不匹配: %1 != %2").arg(currentGateway).arg(savedGateway));
            return false;
        }
    }
    Logger::debug("配置匹配");
    return true;
}

bool NetworkInfoCollector::isValidIpAddress(const QString &ip)
{
    QRegularExpression ipRegex(R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    return ipRegex.match(ip).hasMatch();
}

#if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0601
#undef _WIN32_WINNT
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include "networkinterfacemanager.h"
#include "Logger.h"
#include <QRegularExpression>
#include <QTextCodec>
#include <QHostAddress>
#include <QSettings>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

NetworkInterfaceManager::NetworkInterfaceManager(QObject *parent)
    : QObject(parent)
{
}

QVariantMap NetworkInterfaceManager::captureConfig(const QString &interfaceName)
{
    QVariantMap config;
#ifdef _WIN32
    ULONG size = 15000;
    QByteArray buffer(static_cast<int>(size), '\0');
    ULONG result = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr,
                                        reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data()), &size);
    if(result == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(static_cast<int>(size));
        result = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr,
                                      reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data()), &size);
    }
    if(result != NO_ERROR) return config;

    auto ipv4 = [](const SOCKET_ADDRESS &address) -> QString {
        if(!address.lpSockaddr || address.lpSockaddr->sa_family != AF_INET) return QString();
        const auto *socket = reinterpret_cast<const sockaddr_in *>(address.lpSockaddr);
        return QHostAddress(ntohl(socket->sin_addr.s_addr)).toString();
    };
    for(auto *adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data()); adapter; adapter = adapter->Next)
    {
        const QString name = QString::fromWCharArray(adapter->FriendlyName);
        if(name.compare(interfaceName, Qt::CaseInsensitive) != 0) continue;
        config["interface"] = name;
        config["method"] = (adapter->Flags & IP_ADAPTER_DHCP_ENABLED) ? "dhcp" : "static";
        config["custom_dns"] = false;
        for(auto *entry = adapter->FirstUnicastAddress; entry; entry = entry->Next)
        {
            const QString ip = ipv4(entry->Address);
            if(ip.isEmpty() || ip.startsWith("169.254.")) continue;
            config["ip"] = ip;
            const int prefix = entry->OnLinkPrefixLength;
            if(prefix >= 1 && prefix <= 32)
                config["subnet"] = QHostAddress(0xffffffffu << (32 - prefix)).toString();
            break;
        }
        for(auto *entry = adapter->FirstGatewayAddress; entry; entry = entry->Next)
        {
            const QString gateway = ipv4(entry->Address);
            if(!gateway.isEmpty()) { config["gateway"] = gateway; break; }
        }
        int dnsIndex = 0;
        for(auto *entry = adapter->FirstDnsServerAddress; entry && dnsIndex < 2; entry = entry->Next)
        {
            const QString dns = ipv4(entry->Address);
            if(dns.isEmpty()) continue;
            config[dnsIndex++ == 0 ? "primary_dns" : "secondary_dns"] = dns;
        }
        if(config["method"] == "dhcp")
        {
            const QString key = QString("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%1")
                                    .arg(QString::fromLatin1(adapter->AdapterName));
            QSettings settings(key, QSettings::NativeFormat);
            const QStringList manualDns = settings.value("NameServer").toString()
                                      .split(QRegularExpression("[,\\s]+"), QString::SkipEmptyParts);
            config["custom_dns"] = !manualDns.isEmpty();
            if(!manualDns.isEmpty())
            {
                config["primary_dns"] = manualDns.value(0);
                config["secondary_dns"] = manualDns.value(1);
            }
        }
        break;
    }
#else
    Q_UNUSED(interfaceName);
#endif
    return config;
}

QStringList NetworkInterfaceManager::getNetworkInterfaces()
{
    QList<InterfaceDetail> details = getAllInterfaceDetails();
    QStringList interfaces;
    for(const InterfaceDetail &detail : details)
    {
        if(!detail.name.isEmpty())
        {
            interfaces << detail.name;
        }
    }
    if(interfaces.isEmpty())
    {
        Logger::warning("No network interfaces found");
    }
    return interfaces;
}

bool NetworkInterfaceManager::enableInterface(const QString &interfaceName)
{
    QString output = executeNetshCommand(QString("interface set interface \"%1\" enable").arg(interfaceName));
    if(output.isEmpty())
    {
        return true;
    }
    if(output.contains("已经是启用状态"))
    {
        return true;
    }
    Logger::error(QString("启用网卡 %1 失败: %2").arg(interfaceName).arg(output));
    return false;
}

bool NetworkInterfaceManager::disableInterface(const QString &interfaceName)
{
    QString output = executeNetshCommand(QString("interface set interface \"%1\" disable").arg(interfaceName));
    if(output.isEmpty())
    {
        return true;
    }
    if(output.contains("已经是禁用状态"))
    {
        return true;
    }
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
    QList<InterfaceDetail> details = getAllInterfaceDetails();
    for(const InterfaceDetail &detail : details)
    {
        if(detail.name == interfaceName)
        {
            return detail.adminStatus;
        }
    }
    Logger::warning(QString("无法获取网卡 %1 的管理状态").arg(interfaceName));
    return "Unknown";
}

QString NetworkInterfaceManager::getInterfaceConnStatus(const QString &interfaceName)
{
    QList<InterfaceDetail> details = getAllInterfaceDetails();
    for(const InterfaceDetail &detail : details)
    {
        if(detail.name == interfaceName)
        {
            return detail.connStatus;
        }
    }
    Logger::warning(QString("无法获取网卡 %1 的连接状态").arg(interfaceName));
    return "Unknown";
}

QString NetworkInterfaceManager::getPrimaryBackend()
{
    return "netsh";
}

QString NetworkInterfaceManager::executeNetshCommand(const QString &command)
{
    return execProcess("netsh", command.split(" "), 5000);
}

QString NetworkInterfaceManager::executePowerShellCommand(const QString &command)
{
    QString psCommand = QString(
        "[Console]::OutputEncoding = [System.Text.Encoding]::UTF8; "
        "%1"
    ).arg(command);
    return execProcess("powershell", {
        "-NoProfile", "-NonInteractive", "-Command", psCommand
    }, 8000);
}

QString NetworkInterfaceManager::execProcess(const QString &program, const QStringList &args, int timeoutMs)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(program, args);
    if(!process.waitForFinished(timeoutMs))
    {
        process.kill();
        Logger::error(QString("%1 命令执行超时 (timeout=%2ms): %3").arg(program).arg(timeoutMs).arg(args.join(" ")));
        return QString();
    }
    QByteArray raw = process.readAll();
    QTextCodec *codec = QTextCodec::codecForName("GBK");
    if(!codec)
    {
        codec = QTextCodec::codecForLocale();
    }
    QString result = codec->toUnicode(raw);
    result.remove('\r');
    result = result.trimmed();
    if(process.exitCode() != 0)
    {
        Logger::warning(QString("%1 命令失败 (code=%2): %3").arg(program).arg(process.exitCode()).arg(result));
    }
    return result;
}

QList<InterfaceDetail> NetworkInterfaceManager::getInterfaceDetailsViaPowerShell()
{
    QList<InterfaceDetail> details;
    QString output = executePowerShellCommand(
        "Get-NetAdapter | ForEach-Object { "
        "$status = if($_.Status -eq 'Up') { '已连接' } else { '已断开连接' }; "
        "$admin = if($_.AdminStatus -eq 1) { '已启用' } else { '已禁用' }; "
        "Write-Output ('{0}|{1}|{2}' -f $_.Name, $admin, $status)"
        "}"
    );
    if(output.isEmpty())
    {
        Logger::warning("PowerShell Get-NetAdapter returned empty output, falling back to netsh");
        return details;
    }
    QStringList lines = output.split('\n', QString::SkipEmptyParts);
    for(const QString &line : lines)
    {
        QString trimmed = line.trimmed();
        if(trimmed.isEmpty()) continue;
        QStringList parts = trimmed.split('|');
        if(parts.size() >= 3)
        {
            InterfaceDetail d;
            d.name = parts[0].trimmed();
            d.adminStatus = parts[1].trimmed();
            d.connStatus = parts[2].trimmed();
            if(!d.name.isEmpty())
            {
                details.append(d);
            }
        }
    }
    Logger::info(QString("PowerShell: 解析到 %1 个网卡").arg(details.size()));
    return details;
}

QList<InterfaceDetail> NetworkInterfaceManager::getInterfaceDetailsViaNetsh()
{
    QList<InterfaceDetail> details;
    QString output = executeNetshCommand("interface show interface");
    if(output.isEmpty())
    {
        Logger::warning("netsh returned empty output");
        return details;
    }
    QRegularExpression re(
        "^(已启用|已禁用|Enabled|Disabled)\\s+"
        "(已连接|已断开连接|Connected|Disconnected)\\s+"
        "\\S+\\s+"
        "(\\S+(?:\\s+\\S+)*)(?=\\s*$)",
        QRegularExpression::MultilineOption
    );
    QStringList lines = output.split('\n', QString::SkipEmptyParts);
    for(const QString &line : lines)
    {
        QRegularExpressionMatch match = re.match(line);
        if(match.hasMatch())
        {
            InterfaceDetail d;
            d.adminStatus = match.captured(1).trimmed();
            d.connStatus = match.captured(2).trimmed();
            d.name = match.captured(3).trimmed();
            if(!d.name.isEmpty())
            {
                details.append(d);
            }
        }
    }
    Logger::info(QString("netsh: 解析到 %1 个网卡").arg(details.size()));
    return details;
}

QList<InterfaceDetail> NetworkInterfaceManager::getAllInterfaceDetails()
{
    QList<InterfaceDetail> details;
    QString backend = getPrimaryBackend();

    if(backend == "powershell")
    {
        details = getInterfaceDetailsViaPowerShell();
        if(details.isEmpty())
        {
            Logger::warning("PowerShell backend failed, falling back to netsh");
            details = getInterfaceDetailsViaNetsh();
        }
    }
    else
    {
        details = getInterfaceDetailsViaNetsh();
    }
    return details;
}

QString IpHelperWrapper::formatMacAddress(const unsigned char *mac, int len)
{
    QStringList parts;
    for(int i = 0; i < len && mac[i] != 0; ++i)
    {
        parts.append(QString("%1").arg(mac[i], 2, 16, QChar('0')).toUpper());
    }
    return parts.join("-");
}

bool IpHelperWrapper::getAdaptersInfo(QList<IpHelperAdapterInfo> &adapters)
{
#ifdef _WIN32
    adapters.clear();

    PIP_ADAPTER_INFO pAdapterInfo = nullptr;
    ULONG outBufLen = 15000;
    DWORD dwRetVal = 0;

    dwRetVal = GetAdaptersInfo(pAdapterInfo, &outBufLen);
    if(dwRetVal == ERROR_BUFFER_OVERFLOW)
    {
        pAdapterInfo = (IP_ADAPTER_INFO *)malloc(outBufLen);
        if(!pAdapterInfo) return false;
        dwRetVal = GetAdaptersInfo(pAdapterInfo, &outBufLen);
    }

    if(dwRetVal != NO_ERROR)
    {
        if(pAdapterInfo) free(pAdapterInfo);
        return false;
    }

    PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
    while(pAdapter)
    {
        IpHelperAdapterInfo info;
        info.name = QString::fromLocal8Bit(pAdapter->AdapterName);
        info.adapterIndex = pAdapter->Index;

        if(pAdapter->IpAddressList.IpAddress.String)
        {
            info.ipAddress = pAdapter->IpAddressList.IpAddress.String;
        }
        if(pAdapter->IpAddressList.IpMask.String)
        {
            info.subnetMask = pAdapter->IpAddressList.IpMask.String;
        }
        if(pAdapter->GatewayList.IpAddress.String)
        {
            info.gateway = pAdapter->GatewayList.IpAddress.String;
        }
        if(pAdapter->PrimaryWinsServer.IpAddress.String)
        {
            info.primaryDns = pAdapter->PrimaryWinsServer.IpAddress.String;
        }
        info.dhcpEnabled = pAdapter->DhcpEnabled ? "Enabled" : "Disabled";

        if(pAdapter->AddressLength > 0)
        {
            info.macAddress = QByteArray((const char *)pAdapter->Address, pAdapter->AddressLength);
        }

        adapters.append(info);
        pAdapter = pAdapter->Next;
    }

    if(pAdapterInfo) free(pAdapterInfo);
    return true;
#else
    Q_UNUSED(adapters);
    return false;
#endif
}

#include "configmanager.h"
#include "networkinterfacemanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonArray>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>
#include <QDateTime>
#include <QHostAddress>
#include <QTextCodec>
#include <windows.h>
#include <sddl.h>

//常量定义
const QString configFileName = "config.json";        //配置文件名
const QString historyFileName = "config_history.json"; //历史版本文件名
const int interfaceCacheTimeout = 30;                //网络接口缓存时间(秒)

// 子网掩码转前缀长度 (255.255.255.0 → 24)
static int subnetMaskToPrefixLength(const QString &mask)
{
    QStringList octets = mask.split('.');
    int prefix = 0;
    for(const QString &octet : octets)
    {
        int val = octet.toInt();
        for(int i = 7; i >= 0; i--)
        {
            if(val & (1 << i))
            {
                prefix++;
            }
            else
            {
                break;
            }
        }
    }
    return prefix;
}

/**
 * @brief ConfigManager构造函数
 * @param parent 父对象指针
 *
 * 初始化配置管理器，创建配置目录并检查管理员权限
 */
ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent),
      m_isAdmin(false),
      m_isSaving(false)
{
    //确保配置目录存在
    QDir configDir(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/config/"));
    if (!configDir.exists() && !configDir.mkpath("."))
    {
        emit errorOccurred(tr("无法创建配置目录: %1").arg(configDir.path()));
    }
    //设置配置文件完整路径
    m_configFile = QDir::toNativeSeparators(configDir.path() + "/" + configFileName);
    //设置历史版本文件路径
    m_historyFile = QDir::toNativeSeparators(configDir.path() + "/" + historyFileName);
    m_networkBackupFile = QDir::toNativeSeparators(configDir.path() + "/network_backups.json");
    loadHistory();
    loadNetworkBackups();
    //初始检查管理员状态
    m_isAdmin = checkAdminStatus();
}

/**
 * @brief ConfigManager析构函数
 *
 * 确保所有保存操作完成后再销毁对象
 */
ConfigManager::~ConfigManager()
{
    //等待正在进行的保存操作完成
    while (m_isSaving)
    {
        QCoreApplication::processEvents();
    }
}

/**
 * @brief 检查当前是否具有管理员权限
 * @return 是否具有管理员权限
 */
bool ConfigManager::isAdmin() const
{
    return m_isAdmin;
}

/**
 * @brief 检查并更新管理员权限状态
 * @return 当前是否具有管理员权限
 *
 * 使用Windows API检查当前进程是否以管理员权限运行
 */
bool ConfigManager::checkAdminStatus()
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    //使用安全标识符(SID)检查管理员组权限
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0,
                                 &adminGroup))
    {
        if (!CheckTokenMembership(NULL, adminGroup, &isAdmin))
        {
            isAdmin = FALSE;
        }
        FreeSid(adminGroup);
    }
    //如果权限状态变化则发出信号
    if (m_isAdmin != static_cast<bool>(isAdmin))
    {
        m_isAdmin = isAdmin;
        emit adminStatusChanged(m_isAdmin);
    }
    return m_isAdmin;
}

/**
 * @brief 请求提升到管理员权限
 * @return 请求是否成功
 *
 * 使用ShellExecute以管理员身份重新启动程序
 */
bool ConfigManager::requestAdminPrivileges()
{
    wchar_t path[MAX_PATH];
    if (GetModuleFileName(NULL, path, MAX_PATH))
    {
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = L"runas";      //请求提升权限
        sei.lpFile = path;          //当前程序路径
        sei.hwnd = NULL;
        sei.nShow = SW_NORMAL;
        if (ShellExecuteEx(&sei))
        {
            return true;
        }
        else
        {
            DWORD err = GetLastError();
            if (err == ERROR_CANCELLED)
            {
                emit errorOccurred(tr("用户取消了权限请求"));
            }
            else
            {
                emit errorOccurred(tr("请求管理员权限失败 (错误代码: %1)").arg(err));
            }
        }
    }
    return false;
}

/**
 * @brief 从文件加载所有配置
 * @return 加载是否成功
 *
 * 从JSON格式的配置文件中读取所有网络配置
 */
bool ConfigManager::loadConfigs()
{
    QMutexLocker locker(&m_mutex);  //线程安全锁
    QFile file(m_configFile);
    if (!file.exists())
    {
        return true;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        emit errorOccurred(tr("无法打开配置文件: %1\n错误: %2")
                           .arg(m_configFile)
                           .arg(file.errorString()));
        return false;
    }
    //读取并解析JSON文件
    QByteArray data = file.readAll();
    file.close();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        emit errorOccurred(tr("配置文件解析错误: %1")
                           .arg(parseError.errorString()));
        return false;
    }
    if (!doc.isObject())
    {
        emit errorOccurred(tr("配置文件格式无效"));
        return false;
    }
    //清空现有配置并加载新配置
    m_configs.clear();
    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it)
    {
        QVariantMap config = it.value().toObject().toVariantMap();
        if (validateConfig(config))
        {
            m_configs[it.key()] = config;
        }
        else
        {
            qWarning() << "忽略无效配置:" << it.key();
        }
    }
    return true;
}

/**
 * @brief 内部保存配置实现
 * @return 保存是否成功
 *
 * 将当前配置以JSON格式保存到文件
 */
bool ConfigManager::internalSaveConfigs()
{
    QJsonObject obj;
    {
        QMutexLocker locker(&m_mutex);
        for (auto it = m_configs.begin(); it != m_configs.end(); ++it)
        {
            obj[it.key()] = QJsonObject::fromVariantMap(it.value());
        }
    }
    QJsonDocument doc(obj);
    QSaveFile file(m_configFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        emit errorOccurred(tr("无法创建配置文件: %1\n错误: %2")
                           .arg(m_configFile, file.errorString()));
        return false;
    }
    const QByteArray data = doc.toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size() || !file.commit())
    {
        emit errorOccurred(tr("保存配置文件失败: %1").arg(file.errorString()));
        return false;
    }
    return true;
}

bool ConfigManager::saveConfigs()
{
    static QMutex saveMutex;
    QMutexLocker locker(&saveMutex);
    bool result = internalSaveConfigs();
    return result;
}

/**
 * @brief 获取所有配置的副本
 * @return 配置映射表(配置名->配置数据)
 *
 * 线程安全的配置获取方法
 */
QMap<QString, QVariantMap> ConfigManager::configs() const
{
    QMutexLocker locker(&m_mutex);
    return m_configs;
}

/**
 * @brief 添加新配置
 * @param name 配置名称
 * @param config 配置数据
 * @return 添加是否成功
 */
bool ConfigManager::addConfig(const QString &name, const QVariantMap &config)
{
    if (!validateConfig(config))
    {
        return false;
    }
    {
        QMutexLocker locker(&m_mutex);
        if (m_configs.contains(name))
        {
            emit errorOccurred(tr("配置名称已存在: %1").arg(name));
            return false;
        }
        m_configs[name] = config;
    }
    saveConfigVersion(name, config);
    return true;
}

/**
 * @brief 更新现有配置
 * @param oldName 原配置名称
 * @param newName 新配置名称
 * @param config 新配置数据
 * @return 更新是否成功
 */
bool ConfigManager::updateConfig(const QString &oldName, const QString &newName, const QVariantMap &config)
{
    if (!validateConfig(config))
    {
        return false;
    }
    QVariantMap previous;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_configs.contains(oldName))
        {
            emit errorOccurred(tr("配置不存在: %1").arg(oldName));
            return false;
        }
        if (oldName != newName && m_configs.contains(newName))
        {
            emit errorOccurred(tr("配置名称已存在: %1").arg(newName));
            return false;
        }
        previous = m_configs.take(oldName);
        m_configs[newName] = config;
        if(oldName != newName) m_configHistory[newName] = m_configHistory.take(oldName);
    }
    saveConfigVersion(newName, previous);
    saveConfigVersion(newName, config);
    return true;
}

/**
 * @brief 删除指定配置
 * @param name 要删除的配置名称
 * @return 删除是否成功
 */
bool ConfigManager::removeConfig(const QString &name)
{
    QMutexLocker locker(&m_mutex);
    if (!m_configs.contains(name))
    {
        emit errorOccurred(tr("配置不存在: %1").arg(name));
        return false;
    }
    m_configs.remove(name);
    return true;
}

/**
 * @brief 获取网络接口列表
 * @param useCache 是否使用缓存数据
 * @return 网络接口名称列表
 */
QStringList ConfigManager::getNetworkInterfaces(bool useCache)
{
    //使用缓存数据(如果可用且未过期)
    if (useCache && !m_cachedInterfaces.isEmpty() &&
            m_lastInterfaceUpdate.secsTo(QDateTime::currentDateTime()) < interfaceCacheTimeout)
    {
        return m_cachedInterfaces;
    }
    QStringList interfaces;
    const auto details = NetworkInterfaceManager::getAllInterfaceDetails();
    for (const auto &detail : details)
    {
        if (detail.connStatus == "已连接" || detail.connStatus == "Connected")
        {
            interfaces << detail.name;
        }
    }
    //缓存结果并更新缓存时间
    m_cachedInterfaces = interfaces;
    m_lastInterfaceUpdate = QDateTime::currentDateTime();
    //如果没有找到接口，返回默认值
    return interfaces.isEmpty() ? QStringList() << "以太网 (Ethernet)" << "WLAN (WiFi)"
           : interfaces;
}

/**
 * @brief 清理网络接口名称
 * @param rawName 原始接口名称
 * @return 清理后的接口名称
 *
 * 移除名称中的特殊字符和冗余信息
 */
QString ConfigManager::cleanInterfaceName(const QString &rawName)
{
    if (rawName.isEmpty())
    {
        return QString();
    }
    QString cleaned = rawName;
    //移除括号及其内容
    cleaned.replace(QRegularExpression("\$$.*\$$"), "");
    //移除特殊字符但保留中文
    cleaned.remove(QRegularExpression("[^\\p{L}\\p{N}\\s_-]"));
    //移除首尾空格
    cleaned = cleaned.trimmed();
    //处理常见接口名称
    if (cleaned.contains("以太网"))
    {
        return "以太网";
    }
    if (cleaned.contains("WLAN"))
    {
        return "WLAN";
    }
    return cleaned;
}

/**
 * @brief 将显示用网卡名映射回系统真实网卡名
 * @param requestedName 调用方传入的名称
 * @param availableNames 系统可用的真实网卡名列表
 * @return 解析后的真实网卡名；若无匹配则返回原始输入
 */
QString ConfigManager::resolveInterfaceName(const QString &requestedName, const QStringList &availableNames)
{
    const QString normalizedRequested = requestedName.trimmed();
    if(normalizedRequested.isEmpty() || availableNames.isEmpty())
    {
        return normalizedRequested;
    }

    for(const QString &candidate : availableNames)
    {
        if(candidate.compare(normalizedRequested, Qt::CaseInsensitive) == 0)
        {
            return candidate;
        }
    }

    const QString cleanedRequested = cleanInterfaceName(normalizedRequested);
    for(const QString &candidate : availableNames)
    {
        if(cleanInterfaceName(candidate).compare(cleanedRequested, Qt::CaseInsensitive) == 0)
        {
            return candidate;
        }
    }

    return normalizedRequested;
}

/**
 * @brief 验证配置数据有效性
 * @param config 要验证的配置
 * @return 配置是否有效
 */
bool ConfigManager::validateConfig(const QVariantMap &config) const
{
    //基本检查
    if (config.isEmpty())
    {
        emit errorOccurred(tr("配置为空"));
        return false;
    }
    if (config.value("interface").toString().trimmed().isEmpty() || !config.contains("method"))
    {
        emit errorOccurred(tr("配置缺少必要字段"));
        return false;
    }
    QString method = config["method"].toString();
    if (method != "dhcp" && method != "static")
    {
        emit errorOccurred(tr("无效的网络配置方法: %1").arg(method));
        return false;
    }
    //静态IP配置的详细验证
    if (method == "static")
    {
        QString ip = config.value("ip").toString();
        QString subnet = config.value("subnet").toString();
        if (ip.isEmpty() || subnet.isEmpty())
        {
            emit errorOccurred(tr("静态IP配置需要IP地址和子网掩码"));
            return false;
        }
        const auto validIpv4 = [](const QString &value) {
            QHostAddress address;
            return address.setAddress(value) && address.protocol() == QAbstractSocket::IPv4Protocol;
        };
        if (!validIpv4(ip))
        {
            emit errorOccurred(tr("IP地址格式不正确"));
            return false;
        }
        //子网掩码验证
        QHostAddress subnetAddress;
        if (!validIpv4(subnet) || !subnetAddress.setAddress(subnet) ||
            subnetAddress.toIPv4Address() == 0 ||
            ((~subnetAddress.toIPv4Address()) & ((~subnetAddress.toIPv4Address()) + 1)) != 0)
        {
            emit errorOccurred(tr("子网掩码格式不正确"));
            return false;
        }
        //网关验证
        QString gateway = config.value("gateway").toString();
        if (!gateway.isEmpty() && !validIpv4(gateway))
        {
            emit errorOccurred(tr("默认网关格式不正确"));
            return false;
        }
        //DNS验证
        QString dns1 = config.value("primary_dns").toString();
        if (!dns1.isEmpty() && !validIpv4(dns1))
        {
            emit errorOccurred(tr("首选DNS格式不正确"));
            return false;
        }
        QString dns2 = config.value("secondary_dns").toString();
        if (!dns2.isEmpty() && !validIpv4(dns2))
        {
            emit errorOccurred(tr("备用DNS格式不正确"));
            return false;
        }
    }
    return true;
}

bool ConfigManager::loadHistory()
{
    QMutexLocker locker(&m_mutex);
    QFile file(m_historyFile);
    if (!file.exists()) {
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }
    m_configHistory.clear();
    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QMap<QDateTime, QVariantMap> history;
        QJsonObject historyObj = it.value().toObject();
        for (auto histIt = historyObj.begin(); histIt != historyObj.end(); ++histIt) {
            QDateTime timestamp = QDateTime::fromString(histIt.key(), Qt::ISODate);
            history[timestamp] = histIt.value().toObject().toVariantMap();
        }
        m_configHistory[it.key()] = history;
    }
    return true;
}

bool ConfigManager::saveHistory()
{
    QMutexLocker locker(&m_mutex);
    QSaveFile file(m_historyFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QJsonObject obj;
    for (auto it = m_configHistory.begin(); it != m_configHistory.end(); ++it) {
        QJsonObject historyObj;
        for (auto histIt = it.value().begin(); histIt != it.value().end(); ++histIt) {
            historyObj[histIt.key().toString(Qt::ISODateWithMs)] = QJsonObject::fromVariantMap(histIt.value());
        }
        obj[it.key()] = historyObj;
    }
    QJsonDocument doc(obj);
    const QByteArray data = doc.toJson();
    return file.write(data) == data.size() && file.commit();
}

void ConfigManager::saveConfigVersion(const QString &configName, const QVariantMap &config)
{
    {
        QMutexLocker locker(&m_mutex);
        QDateTime now = QDateTime::currentDateTime();
        while(m_configHistory[configName].contains(now)) now = now.addMSecs(1);
        m_configHistory[configName][now] = config;
        while (m_configHistory[configName].size() > m_maxHistoryVersions) {
            m_configHistory[configName].remove(m_configHistory[configName].begin().key());
        }
    }
    saveHistory();
}

QMap<QDateTime, QVariantMap> ConfigManager::getConfigHistory(const QString &configName) const
{
    QMutexLocker locker(&m_mutex);
    return m_configHistory.value(configName);
}

bool ConfigManager::rollbackConfig(const QString &configName, const QDateTime &timestamp)
{
    {
        QMutexLocker locker(&m_mutex);
        if (!m_configHistory.value(configName).contains(timestamp)) return false;
        m_configs[configName] = m_configHistory.value(configName).value(timestamp);
    }
    return saveConfigs();
}

int ConfigManager::getConfigVersionCount(const QString &configName) const
{
    QMutexLocker locker(&m_mutex);
    return m_configHistory.value(configName).size();
}

/**
 * @brief 应用网络配置
 * @param config 要应用的配置
 * @return 应用是否成功
 *
 * 使用netsh命令修改网络接口配置
 */
bool ConfigManager::applyConfig(const QVariantMap &config, bool createBackup)
{
    //权限检查
    if (!isAdmin())
    {
        emit errorOccurred(tr("需要管理员权限来修改网络配置"));
        emit configApplied(false, tr("需要管理员权限"));
        return false;
    }
    //配置验证
    if (!validateConfig(config))
    {
        emit configApplied(false, tr("配置验证失败"));
        return false;
    }
    //处理网络接口名称
    QString rawInterface = config["interface"].toString();
    QString cleanInterface = cleanInterfaceName(rawInterface);
    if (cleanInterface.isEmpty())
    {
        emit errorOccurred(tr("无效的网络接口名称"));
        emit configApplied(false, tr("无效的网络接口"));
        return false;
    }
    if(createBackup)
    {
        QVariantMap previous = NetworkInterfaceManager::captureConfig(rawInterface);
        if(!previous.isEmpty() && validateConfig(previous))
        {
            previous["saved_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
            QMutexLocker locker(&m_mutex);
            const QList<QVariantMap> oldStack = m_networkBackups.value(rawInterface);
            QList<QVariantMap> &stack = m_networkBackups[rawInterface];
            stack.append(previous);
            while(stack.size() > 5) stack.removeFirst();
            if(!saveNetworkBackups())
            {
                if(oldStack.isEmpty()) m_networkBackups.remove(rawInterface);
                else stack = oldStack;
                emit errorOccurred(tr("网络配置备份写入失败，未修改网卡"));
                emit configApplied(false, tr("网络配置备份写入失败"));
                return false;
            }
        }
        else
        {
            emit errorOccurred(tr("无法读取 %1 的完整当前配置，未生成回滚备份").arg(rawInterface));
            emit configApplied(false, tr("无法生成回滚备份"));
            return false;
        }
    }
    QString method = config["method"].toString();
    bool useCustomDns = config["custom_dns"].toBool();
    bool success = true;
    QString message;
    if (method == "dhcp")
    {
        //设置DHCP地址
        QStringList dhcpAddressCmd =
        {
            "interface", "ipv4", "set", "address",
            QString("name=\"%1\"").arg(cleanInterface),
            "source=dhcp"
        };
        QProcess netshProcess;
        netshProcess.start("netsh", dhcpAddressCmd);
        if(!netshProcess.waitForFinished(5000))
        {
            netshProcess.kill();
            netshProcess.waitForFinished(3000);
        }
        if (netshProcess.error() != QProcess::UnknownError || netshProcess.exitStatus() != QProcess::NormalExit || netshProcess.exitCode() != 0)
        {
            // 使用 GBK 编码处理 netsh 输出，兼容中文 Windows
            QTextCodec *gbkCodec = QTextCodec::codecForName("GBK");
            if(!gbkCodec) gbkCodec = QTextCodec::codecForLocale();
            QString errorOutput = gbkCodec->toUnicode(netshProcess.readAllStandardError());
            emit errorOccurred(tr("设置DHCP地址失败: %1").arg(errorOutput));
            success = false;
            message = tr("设置DHCP地址失败: %1").arg(errorOutput);
        }
        else
        {
            QString primaryDns = config["primary_dns"].toString();
            QString secondaryDns = config["secondary_dns"].toString();
            if (useCustomDns && !primaryDns.isEmpty())
            {
                //设置自定义DNS
                QStringList primaryDnsCmd =
                {
                    "interface", "ipv4", "set", "dns",
                    QString("name=\"%1\"").arg(cleanInterface),
                    "static", primaryDns
                };
                netshProcess.start("netsh", primaryDnsCmd);
                if(!netshProcess.waitForFinished(5000))
                {
                    netshProcess.kill();
                    netshProcess.waitForFinished(3000);
                }
                if (netshProcess.error() != QProcess::UnknownError || netshProcess.exitStatus() != QProcess::NormalExit || netshProcess.exitCode() != 0)
                {
                    QTextCodec *gbkCodec = QTextCodec::codecForName("GBK");
                    if(!gbkCodec) gbkCodec = QTextCodec::codecForLocale();
                    QString errorOutput = gbkCodec->toUnicode(netshProcess.readAllStandardError());
                    emit errorOccurred(tr("设置主DNS失败: %1").arg(errorOutput));
                    success = false;
                    message = tr("设置主DNS失败: %1").arg(errorOutput);
                }
                else if (!secondaryDns.isEmpty())
                {
                    //设置备用DNS
                    QStringList secondaryDnsCmd =
                    {
                        "interface", "ipv4", "add", "dns",
                        QString("name=\"%1\"").arg(cleanInterface),
                        secondaryDns, "index=2"
                    };
                    {
                        QProcess proc;
                        proc.start("netsh", secondaryDnsCmd);
                        if(!proc.waitForFinished(5000)) { proc.kill(); proc.waitForFinished(3000); }
                        if (proc.error() != QProcess::UnknownError || proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
                        {
                            emit errorOccurred(tr("设置备用DNS失败"));
                            message = tr("设置备用DNS失败 (主DNS已设置)");
                        }
                    }
                }
            }
            else
            {
                //设置DHCP DNS
                QStringList dhcpDnsCmd =
                {
                    "interface", "ipv4", "set", "dns",
                    QString("name=\"%1\"").arg(cleanInterface),
                    "source=dhcp"
                };
                netshProcess.start("netsh", dhcpDnsCmd);
                if(!netshProcess.waitForFinished(5000))
                {
                    netshProcess.kill();
                    netshProcess.waitForFinished(3000);
                }
                if (netshProcess.error() != QProcess::UnknownError || netshProcess.exitStatus() != QProcess::NormalExit || netshProcess.exitCode() != 0)
                {
                    QTextCodec *gbkCodec = QTextCodec::codecForName("GBK");
                    if(!gbkCodec) gbkCodec = QTextCodec::codecForLocale();
                    QString errorOutput = gbkCodec->toUnicode(netshProcess.readAllStandardError());
                    emit errorOccurred(tr("设置DHCP DNS失败: %1").arg(errorOutput));
                    success = false;
                    message = tr("设置DHCP DNS失败: %1").arg(errorOutput);
                }
            }
        }
    }
    else
    {
        //设置静态IP配置
        QString ip = config["ip"].toString();
        QString subnet = config["subnet"].toString();
        QString gateway = config["gateway"].toString();
        QString primaryDns = config["primary_dns"].toString();
        QString secondaryDns = config["secondary_dns"].toString();
        //设置IP地址和子网掩码
        QStringList addressCmd =
        {
            "interface", "ip", "set", "address",
            QString("name=\"%1\"").arg(cleanInterface),
            "static", ip, subnet
        };
        if (!gateway.isEmpty())
        {
            addressCmd << gateway << "1";  //1表示默认网关的跃点数
        }
        {
            QProcess proc;
            proc.start("netsh", addressCmd);
            if(!proc.waitForFinished(5000)) { proc.kill(); proc.waitForFinished(3000); }
            if (proc.error() != QProcess::UnknownError || proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
            {
                emit errorOccurred(tr("设置静态IP地址失败"));
                success = false;
                message = tr("设置静态IP地址失败");
            }
        }
        //设置DNS
        if (success && !primaryDns.isEmpty())
        {
            QStringList primaryDnsCmd =
            {
                "interface", "ip", "set", "dns",
                QString("name=\"%1\"").arg(cleanInterface),
                "static", primaryDns
            };
            {
                QProcess proc;
                proc.start("netsh", primaryDnsCmd);
                if(!proc.waitForFinished(5000)) { proc.kill(); proc.waitForFinished(3000); }
                if (proc.error() != QProcess::UnknownError || proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
                {
                    emit errorOccurred(tr("设置主DNS失败"));
                    success = false;
                    message = tr("设置主DNS失败");
                }
            }
            //设置备用DNS
            if (success && !secondaryDns.isEmpty())
            {
                QStringList secondaryDnsCmd =
                {
                    "interface", "ip", "add", "dns",
                    QString("name=\"%1\"").arg(cleanInterface),
                    secondaryDns, "index=2"
                };
                {
                    QProcess proc;
                    proc.start("netsh", secondaryDnsCmd);
                    if(!proc.waitForFinished(5000)) { proc.kill(); proc.waitForFinished(3000); }
                    if (proc.error() != QProcess::UnknownError || proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
                    {
                        emit errorOccurred(tr("设置备用DNS失败"));
                        message = tr("设置备用DNS失败 (主DNS已设置)");
                        //不标记为完全失败，因为主DNS已设置
                    }
                }
            }
        }
        else if (success)
        {
            //如果没有指定DNS，则设置为DHCP
            QStringList dhcpDnsCmd =
            {
                "interface", "ip", "set", "dns",
                QString("name=\"%1\"").arg(cleanInterface),
                "source=dhcp"
            };
            {
                QProcess proc;
                proc.start("netsh", dhcpDnsCmd);
                if(!proc.waitForFinished(5000)) { proc.kill(); proc.waitForFinished(3000); }
                if (proc.error() != QProcess::UnknownError || proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
                {
                    emit errorOccurred(tr("设置DHCP DNS失败"));
                    message = tr("设置DHCP DNS失败 (IP地址已设置)");
                    //不标记为完全失败，因为IP地址已设置
                }
            }
        }
    }
    //发送配置应用结果信号
    emit configApplied(success, success ? QString(tr("配置应用成功: %1")).arg(config["method"].toString() == "dhcp" ? cleanInterface + "[自动获取IP]" : cleanInterface + QString("[%1]").arg(config["ip"].toString())) : message);
    if (success)
    {
        //更新接口缓存
        m_cachedInterfaces.clear();
        m_lastInterfaceUpdate = QDateTime();
    }
    return success;
}

int ConfigManager::networkBackupCount(const QString &interfaceName) const
{
    QMutexLocker locker(&m_mutex);
    return m_networkBackups.value(interfaceName).size();
}

bool ConfigManager::restoreLastNetworkBackup(const QString &interfaceName)
{
    QVariantMap previous;
    {
        QMutexLocker locker(&m_mutex);
        const auto stack = m_networkBackups.value(interfaceName);
        if(stack.isEmpty()) return false;
        previous = stack.last();
    }
    if(!applyConfig(previous, false)) return false;
    QMutexLocker locker(&m_mutex);
    auto &stack = m_networkBackups[interfaceName];
    if(!stack.isEmpty() && stack.last() == previous)
    {
        stack.removeLast();
        if(stack.isEmpty()) m_networkBackups.remove(interfaceName);
        return saveNetworkBackups();
    }
    return true;
}

void ConfigManager::loadNetworkBackups()
{
    QFile file(m_networkBackupFile);
    if(!file.open(QIODevice::ReadOnly)) return;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if(!document.isObject()) return;
    const QJsonObject root = document.object();
    for(auto it = root.begin(); it != root.end(); ++it)
    {
        QList<QVariantMap> stack;
        for(const QJsonValue &entry : it.value().toArray())
        {
            const QVariantMap config = entry.toObject().toVariantMap();
            if(validateConfig(config)) stack.append(config);
        }
        while(stack.size() > 5) stack.removeFirst();
        if(!stack.isEmpty()) m_networkBackups[it.key()] = stack;
    }
}

bool ConfigManager::saveNetworkBackups() const
{
    QJsonObject root;
    for(auto it = m_networkBackups.cbegin(); it != m_networkBackups.cend(); ++it)
    {
        QJsonArray stack;
        for(const QVariantMap &config : it.value()) stack.append(QJsonObject::fromVariantMap(config));
        root[it.key()] = stack;
    }
    QSaveFile file(m_networkBackupFile);
    if(!file.open(QIODevice::WriteOnly)) return false;
    if(file.write(QJsonDocument(root).toJson()) < 0) return false;
    return file.commit();
}

/**
 * @brief 执行netsh命令并获取输出
 * @param args 命令参数列表
 * @return 命令输出结果
 */
QString ConfigManager::getNetshOutput(const QStringList &args) const
{
    QProcess process;
    process.start("netsh", args);
    if(!process.waitForFinished(5000))
    {
        process.kill();
        process.waitForFinished(3000);
    }
    QTextCodec *gbkCodec = QTextCodec::codecForName("GBK");
    if(!gbkCodec) gbkCodec = QTextCodec::codecForLocale();
    QString result = gbkCodec->toUnicode(process.readAllStandardOutput());
    result.remove('\r');
    return result;
}




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
#include <QThread>
#include <algorithm>
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
    const QMap<QString, QString> adapters = NetworkInterfaceManager::adapterNamesById();
    for (auto it = obj.begin(); it != obj.end(); ++it)
    {
        QVariantMap config = it.value().toObject().toVariantMap();
        // 旧版本把网卡描述拼在换行后；读取时迁移为真正的友好名。
        const QString savedName = config.value("interface").toString();
        if(savedName.contains('\r') || savedName.contains('\n'))
            config["interface"] = savedName.section('\r', 0, 0).section('\n', 0, 0).trimmed();
        if(config.value("adapter_id").toString().isEmpty())
        {
            const QString matched = resolveInterfaceName(config.value("interface").toString(), adapters.values());
            for(auto adapter = adapters.cbegin(); adapter != adapters.cend(); ++adapter)
                if(adapter.value() == matched) { config["adapter_id"] = adapter.key(); break; }
        }
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
    QVariantMap bound = config;
    const QVariantMap adapter = NetworkInterfaceManager::captureConfig(config.value("interface").toString());
    if(!adapter.isEmpty()) bound["adapter_id"] = adapter.value("adapter_id");
    {
        QMutexLocker locker(&m_mutex);
        if (m_configs.contains(name))
        {
            emit errorOccurred(tr("配置名称已存在: %1").arg(name));
            return false;
        }
        m_configs[name] = bound;
    }
    saveConfigVersion(name, bound);
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
    QVariantMap bound = config;
    const QVariantMap adapter = NetworkInterfaceManager::captureConfig(config.value("interface").toString());
    if(!adapter.isEmpty()) bound["adapter_id"] = adapter.value("adapter_id");
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
        m_configs[newName] = bound;
        if(oldName != newName) m_configHistory[newName] = m_configHistory.take(oldName);
    }
    saveConfigVersion(newName, previous);
    saveConfigVersion(newName, bound);
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
        return QString();
    }

    QString exact;
    for(const QString &candidate : availableNames)
        if(candidate.compare(normalizedRequested, Qt::CaseInsensitive) == 0)
        {
            if(!exact.isEmpty()) return QString();
            exact = candidate;
        }
    if(!exact.isEmpty()) return exact;

    const QString displayName = normalizedRequested.section('\r', 0, 0).section('\n', 0, 0).trimmed();
    QString legacy;
    for(const QString &candidate : availableNames)
        if(candidate.compare(displayName, Qt::CaseInsensitive) == 0)
        {
            if(!legacy.isEmpty()) return QString();
            legacy = candidate;
        }
    if(!legacy.isEmpty()) return legacy;

    return QString();
}

QString ConfigManager::resolvedInterface(const QVariantMap &config, const QMap<QString, QString> &adapters)
{
    const QString id = config.value("adapter_id").toString();
    if(!id.isEmpty())
    {
        for(auto it = adapters.cbegin(); it != adapters.cend(); ++it)
            if(it.key().compare(id, Qt::CaseInsensitive) == 0) return it.value();
        return QString(); // 已绑定的网卡离线时不能误用同名网卡。
    }
    return resolveInterfaceName(config.value("interface").toString(), adapters.values());
}

QStringList ConfigManager::configDifferences(const QVariantMap &actual, const QVariantMap &target)
{
    QStringList differences;
    if(actual.isEmpty()) return QStringList{QStringLiteral("网卡离线或无法读取系统配置")};
    const QString method = target.value("method").toString();
    if(actual.value("method") != method) differences << QStringLiteral("地址模式");
    if(method == "static")
    {
        for(const QString &key : {"ip", "subnet", "gateway"})
            if(actual.value(key).toString() != target.value(key).toString()) differences << key;
        const QString firstDns = target.value("primary_dns").toString().isEmpty()
            ? target.value("secondary_dns").toString() : target.value("primary_dns").toString();
        const QString secondDns = target.value("primary_dns").toString().isEmpty()
            ? QString() : target.value("secondary_dns").toString();
        if(actual.value("primary_dns").toString() != firstDns) differences << QStringLiteral("首选 DNS");
        if(actual.value("secondary_dns").toString() != secondDns) differences << QStringLiteral("备用 DNS");
    }
    else
    {
        if(actual.value("custom_dns").toBool() != target.value("custom_dns").toBool())
            differences << QStringLiteral("DNS 模式");
        if(target.value("custom_dns").toBool())
        {
            const QString firstDns = target.value("primary_dns").toString().isEmpty()
                ? target.value("secondary_dns").toString() : target.value("primary_dns").toString();
            if(actual.value("primary_dns").toString() != firstDns) differences << QStringLiteral("首选 DNS");
            if(!target.value("primary_dns").toString().isEmpty() &&
               actual.value("secondary_dns").toString() != target.value("secondary_dns").toString())
                differences << QStringLiteral("备用 DNS");
        }
    }
    return differences;
}

QString ConfigManager::previewText(const QVariantMap &target)
{
    const QMap<QString, QString> adapters = NetworkInterfaceManager::adapterNamesById();
    const QString name = resolvedInterface(target, adapters);
    if(name.isEmpty()) return QObject::tr("目标网卡离线或名称匹配不唯一；请重新选择网卡。\n不会修改任何网卡。");
    const QVariantMap current = NetworkInterfaceManager::captureConfig(name);
    if(current.isEmpty()) return QObject::tr("无法读取目标网卡状态；不会修改网卡。");
    const auto value = [](const QVariantMap &config, const QString &key) {
        if(!config.contains(key)) return QStringLiteral("未设置");
        const QString text = config.value(key).toString();
        return text.isEmpty() ? QStringLiteral("设为空") : text;
    };
    QStringList lines{QObject::tr("网卡：%1").arg(name)};
    for(const auto &field : {qMakePair(QStringLiteral("method"), QStringLiteral("地址模式")),
                             qMakePair(QStringLiteral("ip"), QStringLiteral("IP 地址")),
                             qMakePair(QStringLiteral("subnet"), QStringLiteral("子网掩码")),
                             qMakePair(QStringLiteral("gateway"), QStringLiteral("默认网关")),
                             qMakePair(QStringLiteral("primary_dns"), QStringLiteral("首选 DNS")),
                             qMakePair(QStringLiteral("secondary_dns"), QStringLiteral("备用 DNS"))})
        lines << QStringLiteral("%1：%2 → %3").arg(field.second, value(current, field.first), value(target, field.first));
    if(target.value("method") == "static")
    {
        QHostAddress ip(target.value("ip").toString()), mask(target.value("subnet").toString());
        QHostAddress gateway(target.value("gateway").toString());
        if(!target.value("gateway").toString().isEmpty() &&
           (ip.toIPv4Address() & mask.toIPv4Address()) !=
           (gateway.toIPv4Address() & mask.toIPv4Address()))
            lines << QObject::tr("注意：网关不在目标 IP 的同一子网；特殊拓扑可继续。");
        for(auto it = adapters.cbegin(); it != adapters.cend(); ++it)
        {
            if(it.value() == name &&
               (target.value("adapter_id").toString().isEmpty() ||
                it.key() == target.value("adapter_id").toString())) continue;
            if(NetworkInterfaceManager::captureConfig(it.key()).value("ip") == target.value("ip"))
                lines << QObject::tr("注意：%1 当前使用相同 IP，可能冲突。").arg(it.value());
        }
    }
    lines << QObject::tr("修改地址或 DNS 可能暂时中断连接。继续前请确认回滚备份可用。");
    return lines.join('\n');
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
    const QRegularExpression unsafeName(QStringLiteral("[\\x00-\\x1f\\x7f\"']"));
    if(config.value("interface").toString().contains(unsafeName) ||
       config.value("adapter_id").toString().contains(unsafeName))
    {
        emit errorOccurred(tr("网卡名称或标识含无效字符"));
        return false;
    }
    QString method = config["method"].toString();
    if (method != "dhcp" && method != "static")
    {
        emit errorOccurred(tr("无效的网络配置方法: %1").arg(method));
        return false;
    }
    if(method == "dhcp" && config.value("custom_dns").toBool() &&
       config.value("primary_dns").toString().isEmpty() &&
       config.value("secondary_dns").toString().isEmpty())
    {
        emit errorOccurred(tr("自定义 DNS 已启用，但未填写 DNS 地址"));
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
    else if(config.value("custom_dns").toBool())
    {
        for(const QString &key : {"primary_dns", "secondary_dns"})
        {
            const QString address = config.value(key).toString();
            QHostAddress parsed;
            if(!address.isEmpty() &&
               (!parsed.setAddress(address) || parsed.protocol() != QAbstractSocket::IPv4Protocol))
            {
                emit errorOccurred(tr("自定义 DNS 地址格式不正确：%1").arg(key));
                return false;
            }
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
bool ConfigManager::runNetshStep(const QString &step, const QStringList &args, int timeoutMs,
                                 QString *error, const QString &program) const
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(program, args);
    bool finished = process.waitForStarted(3000) && process.waitForFinished(timeoutMs);
    if(!finished && process.state() != QProcess::NotRunning)
    {
        process.kill();
        process.waitForFinished(3000);
    }
    QTextCodec *codec = QTextCodec::codecForName("GBK");
    if(!codec) codec = QTextCodec::codecForLocale();
    const QString output = codec->toUnicode(process.readAll()).trimmed().left(400);
    const bool ok = finished && process.error() == QProcess::UnknownError &&
                    process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    const QString detail = ok ? tr("成功") : tr("退出码 %1；%2")
        .arg(process.exitCode()).arg(output.isEmpty() ? process.errorString() : output);
    emit applyStep(step, process.exitCode(), detail);
    if(!ok && error) *error = tr("%1失败：%2").arg(step, detail);
    return ok;
}

bool ConfigManager::applyConfig(const QVariantMap &config, bool createBackup)
{
    if(!isAdmin())
    {
        emit errorOccurred(tr("需要管理员权限来修改网络配置"));
        emit configApplied(false, tr("需要管理员权限"));
        return false;
    }
    if(!validateConfig(config))
    {
        emit configApplied(false, tr("配置验证失败"));
        return false;
    }
    const QMap<QString, QString> adapters = NetworkInterfaceManager::adapterNamesById();
    const QString interfaceName = resolvedInterface(config, adapters);
    if(interfaceName.isEmpty())
    {
        const QString reason = config.value("adapter_id").toString().isEmpty()
            ? tr("旧配置网卡名称无法唯一匹配，请重新选择网卡并更新配置")
            : tr("已绑定的网卡离线或已拔出，未修改其他网卡");
        emit errorOccurred(reason);
        emit configApplied(false, reason);
        return false;
    }
    QVariantMap target = config;
    target["interface"] = interfaceName;
    const QVariantMap current = NetworkInterfaceManager::captureConfig(interfaceName);
    if(current.isEmpty() || !validateConfig(current))
    {
        const QString reason = tr("无法读取 %1 的完整当前配置，未修改网卡").arg(interfaceName);
        emit errorOccurred(reason);
        emit configApplied(false, reason);
        return false;
    }
    target["adapter_id"] = current.value("adapter_id");
    if(!validateConfig(target))
    {
        emit configApplied(false, tr("网卡名称验证失败"));
        return false;
    }
    if(createBackup)
    {
        QVariantMap previous = current;
        previous["saved_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        const QString key = current.value("adapter_id").toString().isEmpty()
            ? interfaceName : current.value("adapter_id").toString();
        QMutexLocker locker(&m_mutex);
        const QList<QVariantMap> oldStack = m_networkBackups.value(key);
        m_networkBackups[key].append(previous);
        if(!saveNetworkBackups())
        {
            if(oldStack.isEmpty()) m_networkBackups.remove(key);
            else m_networkBackups[key] = oldStack;
            const QString reason = tr("网络配置备份写入失败，未修改网卡");
            emit errorOccurred(reason);
            emit configApplied(false, reason);
            return false;
        }
    }

    const QString nameArg = QString("name=\"%1\"").arg(interfaceName);
    const QString method = target.value("method").toString();
    QString error;
    bool success = false;
    if(method == "dhcp")
    {
        success = runNetshStep(tr("设置 DHCP 地址"),
            {"interface", "ipv4", "set", "address", nameArg, "source=dhcp"}, 15000, &error);
        if(success && target.value("custom_dns").toBool() &&
           (!target.value("primary_dns").toString().isEmpty() ||
            !target.value("secondary_dns").toString().isEmpty()))
        {
            const QString first = target.value("primary_dns").toString().isEmpty()
                ? target.value("secondary_dns").toString() : target.value("primary_dns").toString();
            success = runNetshStep(tr("设置首选 DNS"),
                {"interface", "ipv4", "set", "dns", nameArg, "static", first}, 8000, &error);
            if(success && !target.value("primary_dns").toString().isEmpty() &&
               !target.value("secondary_dns").toString().isEmpty())
                success = runNetshStep(tr("设置备用 DNS"),
                    {"interface", "ipv4", "add", "dns", nameArg,
                     target.value("secondary_dns").toString(), "index=2"}, 8000, &error);
        }
        else if(success)
            success = runNetshStep(tr("设置 DHCP DNS"),
                {"interface", "ipv4", "set", "dns", nameArg, "source=dhcp"}, 8000, &error);
    }
    else
    {
        QStringList addressArgs{"interface", "ipv4", "set", "address", nameArg, "static",
                                target.value("ip").toString(), target.value("subnet").toString()};
        if(!target.value("gateway").toString().isEmpty())
            addressArgs << target.value("gateway").toString() << "1";
        success = runNetshStep(tr("设置静态地址"), addressArgs, 15000, &error);
        const QString first = target.value("primary_dns").toString().isEmpty()
            ? target.value("secondary_dns").toString() : target.value("primary_dns").toString();
        if(success && !first.isEmpty())
        {
            success = runNetshStep(tr("设置首选 DNS"),
                {"interface", "ipv4", "set", "dns", nameArg, "static", first}, 8000, &error);
            if(success && !target.value("primary_dns").toString().isEmpty() &&
               !target.value("secondary_dns").toString().isEmpty())
                success = runNetshStep(tr("设置备用 DNS"),
                    {"interface", "ipv4", "add", "dns", nameArg,
                     target.value("secondary_dns").toString(), "index=2"}, 8000, &error);
        }
        else if(success)
            success = runNetshStep(tr("清空 DNS"),
                {"interface", "ipv4", "set", "dnsservers", nameArg, "source=static", "address=none"},
                8000, &error);
    }
    if(success)
    {
        QStringList differences;
        for(int attempt = 0; attempt < 5; ++attempt)
        {
            differences = configDifferences(NetworkInterfaceManager::captureConfig(interfaceName), target);
            if(differences.isEmpty()) break;
            QThread::msleep(500);
        }
        if(!differences.isEmpty())
        {
            success = false;
            error = tr("命令已执行，但以下字段未生效：%1").arg(differences.join("、"));
        }
    }
    else
    {
        const QStringList differences = configDifferences(
            NetworkInterfaceManager::captureConfig(interfaceName), target);
        if(!differences.isEmpty()) error += tr("；未生效：%1").arg(differences.join("、"));
    }
    if(success)
    {
        m_cachedInterfaces.clear();
        m_lastInterfaceUpdate = QDateTime();
    }
    else emit errorOccurred(error);
    emit configApplied(success, success ? tr("配置应用并核验成功：%1").arg(interfaceName) : error);
    return success;
}

int ConfigManager::networkBackupCount(const QString &interfaceName) const
{
    return networkBackups(interfaceName).size();
}

QList<QVariantMap> ConfigManager::networkBackups(const QString &interfaceName) const
{
    const QVariantMap adapter = NetworkInterfaceManager::captureConfig(interfaceName);
    const QString id = adapter.value("adapter_id").toString();
    QMutexLocker locker(&m_mutex);
    if(!id.isEmpty() && m_networkBackups.contains(id)) return m_networkBackups.value(id);
    return m_networkBackups.value(interfaceName);
}

bool ConfigManager::restoreLastNetworkBackup(const QString &interfaceName)
{
    return restoreNetworkBackup(interfaceName, networkBackupCount(interfaceName) - 1);
}

bool ConfigManager::restoreNetworkBackup(const QString &interfaceName, int index)
{
    const QList<QVariantMap> backups = networkBackups(interfaceName);
    if(index < 0 || index >= backups.size()) return false;
    QVariantMap previous = backups.at(index);
    const QVariantMap adapter = NetworkInterfaceManager::captureConfig(interfaceName);
    if(!adapter.isEmpty())
    {
        previous["interface"] = adapter.value("interface");
        previous["adapter_id"] = adapter.value("adapter_id");
    }
    return applyConfig(previous, true);
}

void ConfigManager::loadNetworkBackups()
{
    QFile file(m_networkBackupFile);
    if(!file.open(QIODevice::ReadOnly)) return;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if(!document.isObject()) return;
    const QJsonObject root = document.object();
    const QMap<QString, QString> adapters = NetworkInterfaceManager::adapterNamesById();
    for(auto it = root.begin(); it != root.end(); ++it)
    {
        QString key = it.key();
        if(!adapters.contains(key))
        {
            const QString name = resolveInterfaceName(key, adapters.values());
            for(auto adapter = adapters.cbegin(); adapter != adapters.cend(); ++adapter)
                if(adapter.value() == name) { key = adapter.key(); break; }
        }
        QList<QVariantMap> stack;
        for(const QJsonValue &entry : it.value().toArray())
        {
            QVariantMap config = entry.toObject().toVariantMap();
            const QString oldName = config.value("interface").toString();
            if(oldName.contains('\r') || oldName.contains('\n'))
                config["interface"] = oldName.section('\r', 0, 0).section('\n', 0, 0).trimmed();
            if(adapters.contains(key)) config["adapter_id"] = key;
            if(validateConfig(config)) stack.append(config);
        }
        m_networkBackups[key].append(stack);
    }
    for(auto it = m_networkBackups.begin(); it != m_networkBackups.end(); ++it)
        std::sort(it.value().begin(), it.value().end(), [](const QVariantMap &a, const QVariantMap &b) {
            return a.value("saved_at").toString() < b.value("saved_at").toString();
        });
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
    const QByteArray bytes = QJsonDocument(root).toJson();
    if(file.write(bytes) != bytes.size()) return false;
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




#include "configmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>
#include <QDateTime>
#include <windows.h>
#include <sddl.h>

//常量定义
const QString configFileName = "config.json";        //配置文件名
const int interfaceCacheTimeout = 30;                //网络接口缓存时间(秒)

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
    QFile file(m_configFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        emit errorOccurred(tr("无法打开配置文件: %1\n错误: %2")
                           .arg(m_configFile)
                           .arg(file.errorString()));
        return false;
    }
    //构建JSON对象
    QJsonObject obj;
    {
        QMutexLocker locker(&m_mutex);
        for (auto it = m_configs.begin(); it != m_configs.end(); ++it)
        {
            obj[it.key()] = QJsonObject::fromVariantMap(it.value());
        }
    }
    //写入文件
    QJsonDocument doc(obj);
    qint64 bytesWritten = file.write(doc.toJson(QJsonDocument::Indented));
    if (bytesWritten == -1)
    {
        emit errorOccurred(tr("写入配置文件失败: %1")
                           .arg(file.errorString()));
        file.close();
        return false;
    }
    file.close();
    return true;
}

/**
 * @brief 保存所有配置到文件
 * @return 保存是否成功
 *
 * 线程安全的保存方法，防止重入
 */
bool ConfigManager::saveConfigs()
{
    if (m_isSaving)
    {
        emit errorOccurred(tr("已有保存操作在进行中"));
        return false;
    }
    m_isSaving = true;
    bool result = internalSaveConfigs();
    m_isSaving = false;
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
    QMutexLocker locker(&m_mutex);
    if (m_configs.contains(name))
    {
        emit errorOccurred(tr("配置名称已存在: %1").arg(name));
        return false;
    }
    m_configs[name] = config;
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
    m_configs.remove(oldName);
    m_configs[newName] = config;
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
    //使用netsh命令获取网络接口信息
    QString output = getNetshOutput(QStringList() << "interface" << "show" << "interface");
    QStringList interfaces;
    QStringList lines = output.split('\n', QString::SkipEmptyParts);
    //解析输出结果，只获取已连接的接口
    for (const QString &line : lines)
    {
        if (line.contains("已连接") || line.contains("Connected"))
        {
            QStringList parts = line.split(' ', QString::SkipEmptyParts);
            if (parts.size() >= 4)
            {
                QString interfaceName = parts.last();
                QString interfaceDesc = parts[parts.size() - 2];
                interfaces << QString("%1 (%2)").arg(interfaceName).arg(interfaceDesc);
            }
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
    if (!config.contains("interface") || !config.contains("method"))
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
        //IP地址格式验证
        QRegularExpression ipRegex(R"(^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$)");
        auto ipMatch = ipRegex.match(ip);
        if (!ipMatch.hasMatch())
        {
            emit errorOccurred(tr("IP地址格式不正确"));
            return false;
        }
        //验证IP地址各部分是否在有效范围内
        for (int i = 1; i <= 4; ++i)
        {
            int octet = ipMatch.captured(i).toInt();
            if (octet < 0 || octet > 255)
            {
                emit errorOccurred(tr("IP地址包含无效的数字: %1").arg(octet));
                return false;
            }
        }
        //子网掩码验证
        if (!subnet.isEmpty() && !ipRegex.match(subnet).hasMatch())
        {
            emit errorOccurred(tr("子网掩码格式不正确"));
            return false;
        }
        //网关验证
        QString gateway = config.value("gateway").toString();
        if (!gateway.isEmpty() && !ipRegex.match(gateway).hasMatch())
        {
            emit errorOccurred(tr("默认网关格式不正确"));
            return false;
        }
        //DNS验证
        QString dns1 = config.value("primary_dns").toString();
        if (!dns1.isEmpty() && !ipRegex.match(dns1).hasMatch())
        {
            emit errorOccurred(tr("首选DNS格式不正确"));
            return false;
        }
        QString dns2 = config.value("secondary_dns").toString();
        if (!dns2.isEmpty() && !ipRegex.match(dns2).hasMatch())
        {
            emit errorOccurred(tr("备用DNS格式不正确"));
            return false;
        }
    }
    return true;
}

/**
 * @brief 应用网络配置
 * @param config 要应用的配置
 * @return 应用是否成功
 *
 * 使用netsh命令修改网络接口配置
 */
bool ConfigManager::applyConfig(const QVariantMap &config)
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
    QString method = config["method"].toString();
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
        netshProcess.waitForFinished();
        if (netshProcess.exitCode() != 0)
        {
            QString errorOutput = QString::fromLocal8Bit(netshProcess.readAllStandardError());
            emit errorOccurred(tr("设置DHCP地址失败: %1").arg(errorOutput));
            success = false;
            message = tr("设置DHCP地址失败: %1").arg(errorOutput);
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
            netshProcess.waitForFinished();
            if (netshProcess.exitCode() != 0)
            {
                QString errorOutput = QString::fromLocal8Bit(netshProcess.readAllStandardError());
                emit errorOccurred(tr("设置DHCP DNS失败: %1").arg(errorOutput));
                success = false;
                message = tr("设置DHCP DNS失败: %1").arg(errorOutput);
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
        if (QProcess::execute("netsh", addressCmd) != 0)
        {
            emit errorOccurred(tr("设置静态IP地址失败"));
            success = false;
            message = tr("设置静态IP地址失败");
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
            if (QProcess::execute("netsh", primaryDnsCmd) != 0)
            {
                emit errorOccurred(tr("设置主DNS失败"));
                success = false;
                message = tr("设置主DNS失败");
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
                if (QProcess::execute("netsh", secondaryDnsCmd) != 0)
                {
                    emit errorOccurred(tr("设置备用DNS失败"));
                    message = tr("设置备用DNS失败 (主DNS已设置)");
                    //不标记为完全失败，因为主DNS已设置
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
            if (QProcess::execute("netsh", dhcpDnsCmd) != 0)
            {
                emit errorOccurred(tr("设置DHCP DNS失败"));
                message = tr("设置DHCP DNS失败 (IP地址已设置)");
                //不标记为完全失败，因为IP地址已设置
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

/**
 * @brief 执行netsh命令并获取输出
 * @param args 命令参数列表
 * @return 命令输出结果
 */
QString ConfigManager::getNetshOutput(const QStringList &args) const
{
    QProcess process;
    process.start("netsh", args);
    process.waitForFinished();
    return QString::fromLocal8Bit(process.readAllStandardOutput());
}

#include "configmanager.h"
#include <QLoggingCategory>

//定义日志分类
Q_LOGGING_CATEGORY(configLog, "[app.ConfigManager]")

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent),
      m_configPath(getConfigPath()),
      m_fileWatcher(new QFileSystemWatcher(this))
{
    //初始化默认配置
    initDefaultConfig();
    //加载配置文件
    if (!loadConfig())
    {
        qCWarning(configLog) << "加载配置文件失败，使用默认配置";
    }
    //监控配置文件变化
    watchForChanges();
}

ConfigManager::~ConfigManager()
{
    //保存配置
    if (!saveConfig())
    {
        qCWarning(configLog) << "退出时保存配置文件失败";
    }
}

QString ConfigManager::getConfigPath() const
{
    //获取应用程序所在目录
    QString appDir = QCoreApplication::applicationDirPath();
    //处理可能的符号链接（如果是macOS的.app包）
    QFileInfo appInfo(appDir);
    if (appInfo.isSymLink())
    {
        appDir = appInfo.symLinkTarget();
    }
    //获取上一级目录
    QDir parentDir(appDir);
    if (!parentDir.cdUp())
    {
        qCWarning(configLog) << "无法访问应用程序目录的上一级目录:" << appDir;
        return QString();
    }
    //构建路径（跨平台兼容）
    QString configPath = QDir::cleanPath(parentDir.absolutePath() + QDir::separator() + "config" + QDir::separator() + "config.json");
    //确保目录存在
    QFileInfo configInfo(configPath);
    QDir configDir = configInfo.absoluteDir();
    if (!configDir.exists())
    {
        if (!configDir.mkpath("."))
        {
            qCWarning(configLog) << "无法创建配置目录:" << configDir.absolutePath();
            return QString();
        }
        qCDebug(configLog) << "已创建配置目录:" << configDir.absolutePath();
    }
    return configPath;
}

void ConfigManager::initDefaultConfig()
{
    static const QVector<QPair<QString, QVariant >> defaultConfig =
    {
        {"version", 1},
        {"dataDir", QDir::homePath()},
        {"order", "顺序"},
        {"sendInterval", 100},
        {"addresses", QStringList()},
        {"includeTopics", QStringList()},
        {"excludeTopics", QStringList()},
        {"uniqueMode", false}
    };
    QWriteLocker locker(&m_lock);
    for (const auto &item : defaultConfig)
    {
        if (!m_config.contains(item.first))
        {
            m_config[item.first] = item.second;
        }
    }
}

bool ConfigManager::loadConfig()
{
    QFile file(m_configPath);
    if (!file.exists())
    {
        m_lastError = ConfigError::FileNotFound;
        qCWarning(configLog) << "配置文件未找到:" << m_configPath;
        return false;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        m_lastError = ConfigError::PermissionDenied;
        qCWarning(configLog) << "无法打开配置文件:" << file.errorString();
        return false;
    }
    //读取并解析配置文件
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if (error.error != QJsonParseError::NoError)
    {
        m_lastError = ConfigError::InvalidFormat;
        qCWarning(configLog) << "配置文件解析错误:" << error.errorString();
        return false;
    }
    QVariantMap newConfig = doc.object().toVariantMap();
    QVariantMap oldConfig;
    {
        QWriteLocker locker(&m_lock);
        oldConfig = m_config; //保存旧配置用于比较变更
        m_config = newConfig; //更新为新配置
        //执行版本迁移
        if (!migrateConfig())
        {
            m_lastError = ConfigError::VersionMismatch;
            qCWarning(configLog) << "配置迁移失败";
            return false;
        }
    }
    //比较并发送配置变更信号
    for (auto it = newConfig.constBegin(); it != newConfig.constEnd(); ++it)
    {
        const QString &key = it.key();
        const QVariant &newValue = it.value();
        const QVariant oldValue = oldConfig.value(key);
        if (oldValue != newValue)
        {
            emit configChanged(key, newValue);
            qCDebug(configLog) << "配置变更:" << key << "从" << oldValue << "变为" << newValue;
        }
    }
    //检查被删除的键
    for (auto it = oldConfig.constBegin(); it != oldConfig.constEnd(); ++it)
    {
        const QString &key = it.key();
        if (!newConfig.contains(key))
        {
            emit configChanged(key, QVariant());
            qCDebug(configLog) << "配置项被删除:" << key;
        }
    }
    //最后标记无错误
    QWriteLocker locker(&m_lock);
    m_lastError = ConfigError::NoError;
    emit configReloaded();
    return true;
}

bool ConfigManager::saveConfig()
{
    QWriteLocker locker(&m_lock);
    //确保配置目录存在
    QFileInfo fileInfo(m_configPath);
    if (!QDir().mkpath(fileInfo.absolutePath()))
    {
        m_lastError = ConfigError::DirectoryCreationFailed;
        qCWarning(configLog) << "无法创建配置目录:" << fileInfo.absolutePath();
        return false;
    }
    QFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        m_lastError = ConfigError::WriteFailed;
        qCWarning(configLog) << "无法写入配置文件:" << file.errorString() << "路径:" << m_configPath;
        return false;
    }
    QJsonObject obj;
    for (auto it = m_config.begin(); it != m_config.end(); ++it)
    {
        obj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    qint64 bytesWritten = file.write(QJsonDocument(obj).toJson());
    file.close();
    if (bytesWritten == -1)
    {
        m_lastError = ConfigError::WriteFailed;
        qCWarning(configLog) << "写入配置文件失败:" << file.errorString();
        return false;
    }
    m_lastError = ConfigError::NoError;
    qCDebug(configLog) << "配置已成功保存到:" << m_configPath;
    return true;
}

QVariant ConfigManager::get(const QString &key, const QVariant &defaultValue) const
{
    QReadLocker locker(&m_lock);
    return m_config.value(key, defaultValue);
}

void ConfigManager::set(const QString &key, const QVariant &value)
{
    QWriteLocker locker(&m_lock);
    if (m_config.value(key) != value)
    {
        m_config[key] = value;
        emit configChanged(key, value);
    }
}

QStringList ConfigManager::getAddressList() const
{
    return get("addresses").toStringList();
}

void ConfigManager::setAddressList(const QStringList &addresses)
{
    set("addresses", addresses);
}

QStringList ConfigManager::getIncludeTopics() const
{
    return get("includeTopics").toStringList();
}

QStringList ConfigManager::getExcludeTopics() const
{
    return get("excludeTopics").toStringList();
}

ConfigError ConfigManager::lastError() const
{
    return m_lastError;
}

QString ConfigManager::errorString() const
{
    static const QMap<ConfigError, QString> errorStrings = {
        {ConfigError::NoError, "无错误"},
        {ConfigError::FileNotFound, "配置文件未找到"},
        {ConfigError::PermissionDenied, "文件权限不足"},
        {ConfigError::InvalidFormat, "配置文件格式无效"},
        {ConfigError::WriteFailed, "配置文件写入失败"},
        {ConfigError::VersionMismatch, "配置文件版本不匹配"},
        {ConfigError::InvalidKey, "无效的配置键"},
        {ConfigError::InvalidValue, "无效的配置值"},
        {ConfigError::DirectoryCreationFailed, "目录创建失败"}
    };

    return errorStrings.value(m_lastError, "未知错误");
}

void ConfigManager::watchForChanges()
{
    m_fileWatcher->addPath(m_configPath);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, [this]()
    {
        //文件被删除时重新监控
        if (!QFile::exists(m_configPath))
        {
            m_fileWatcher->addPath(m_configPath);
            return;
        }
        //重新加载配置
        if (loadConfig())
        {
            //重新添加监控（某些系统需手动恢复）
            m_fileWatcher->addPath(m_configPath);
            emit configReloaded();
        }
    });
}

bool ConfigManager::migrateConfig()
{
    int version = m_config.value("version", 1).toInt();
    if (version < 1)
    {
        //示例：版本迁移到 2
        QVariantMap changes;
        changes["newKey"] = "default";
        updateConfig(changes); //通过 updateConfig 触发信号
        return saveConfig();
    }
    return true;
}

QVariantMap ConfigManager::getConfig() const
{
    QReadLocker locker(&m_lock);
    return m_config;
}

void ConfigManager::updateConfig(const QVariantMap &config)
{
    QWriteLocker locker(&m_lock);
    QVariantMap oldConfig = m_config;
    for (auto it = config.begin(); it != config.end(); ++it)
    {
        const QString &key = it.key();
        const QVariant &newValue = it.value();
        QVariant oldValue = oldConfig.value(key);
        if (oldValue != newValue)
        {
            m_config[key] = newValue;
            emit configChanged(key, newValue);
        }
    }
    emit configReloaded();
}

#include "configmanager.h"
#include <QLoggingCategory>
#include <QElapsedTimer>

//定义日志分类
Q_LOGGING_CATEGORY(configLog, "[app.ConfigManager]")

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent),
      m_configPath(getConfigPath()),
      m_fileWatcher(new QFileSystemWatcher(this))
{
    //初始化默认配置
    initDefaultConfig();
    // 延迟到事件循环启动后再加载配置与启动监控，避免应用启动早期阻塞
    qCInfo(configLog) << "ConfigManager 构造完成（未立即加载配置，等待异步初始化）";
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
    qCInfo(configLog) << "配置文件路径解析为:" << configPath;
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
        {"uniqueMode", false},
        // 新增：流程与超时倍率（用于OrderSend模块）
        {"flowFastMultiplier", 0.01},
        {"flowNormalMultiplier", 0.8},
        {"timeoutCheckedMultiplier", 1.5}
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
    qCInfo(configLog) << "开始加载配置文件";
    QElapsedTimer t; t.start();
    QFile file(m_configPath);
    if (!file.exists())
    {
        m_lastError = ConfigError::FileNotFound;
        qCWarning(configLog) << "配置文件未找到:" << m_configPath;
        qCInfo(configLog) << "结束加载配置（不存在）" << "耗时(ms):" << t.elapsed();
        return false;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        m_lastError = ConfigError::PermissionDenied;
        qCWarning(configLog) << "无法打开配置文件:" << file.errorString();
        qCInfo(configLog) << "结束加载配置（打开失败）" << "耗时(ms):" << t.elapsed();
        return false;
    }
    //读取并解析配置文件
    const qint64 size = file.size();
    qCInfo(configLog) << "配置文件大小(bytes):" << size;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if (error.error != QJsonParseError::NoError)
    {
        m_lastError = ConfigError::InvalidFormat;
        qCWarning(configLog) << "配置文件解析错误:" << error.errorString();
        qCInfo(configLog) << "结束加载配置（解析失败）" << "耗时(ms):" << t.elapsed();
        return false;
    }
    QVariantMap parsedConfig = doc.object().toVariantMap();
    qCInfo(configLog) << "解析后的键数量:" << parsedConfig.keys().size();
    // 先将新配置写入（持锁），但不在锁内执行迁移，避免读写锁重入导致的死锁
    QVariantMap oldConfig;
    {
        QWriteLocker locker(&m_lock);
        oldConfig = m_config; //保存旧配置用于比较变更
        m_config = parsedConfig; //更新为新配置（原始）
    }
    // 在不持锁的情况下执行迁移，内部可安全获取写锁
    if (!migrateConfig())
    {
        QWriteLocker locker(&m_lock);
        m_lastError = ConfigError::VersionMismatch;
        qCWarning(configLog) << "配置迁移失败";
        qCInfo(configLog) << "结束加载配置（迁移失败）" << "耗时(ms):" << t.elapsed();
        return false;
    }
    // 迁移可能对 m_config 做了补充，此处读取最终配置用于后续变更比较
    QVariantMap newConfig;
    {
        QReadLocker locker(&m_lock);
        newConfig = m_config;
    }
    //比较并发送配置变更信号
    int changed = 0;
    for (auto it = newConfig.constBegin(); it != newConfig.constEnd(); ++it)
    {
        const QString &key = it.key();
        const QVariant &newValue = it.value();
        const QVariant oldValue = oldConfig.value(key);
        if (oldValue != newValue)
        {
            emit configChanged(key, newValue);
            qCDebug(configLog) << "配置变更:" << key << "从" << oldValue << "变为" << newValue;
            ++changed;
        }
    }
    //检查被删除的键
    int removed = 0;
    for (auto it = oldConfig.constBegin(); it != oldConfig.constEnd(); ++it)
    {
        const QString &key = it.key();
        if (!newConfig.contains(key))
        {
            emit configChanged(key, QVariant());
            qCDebug(configLog) << "配置项被删除:" << key;
            ++removed;
        }
    }
    //最后标记无错误（释放锁后再发信号，避免死锁）
    {
        QWriteLocker locker(&m_lock);
        m_lastError = ConfigError::NoError;
    }
    emit configReloaded();
    qCInfo(configLog) << "加载配置完成" << "变更数:" << changed << "删除数:" << removed << "耗时(ms):" << t.elapsed();
    return true;
}

bool ConfigManager::saveConfig()
{
    qCInfo(configLog) << "开始保存配置文件";
    QElapsedTimer t; t.start();
    QWriteLocker locker(&m_lock);
    //确保配置目录存在
    QFileInfo fileInfo(m_configPath);
    if (!QDir().mkpath(fileInfo.absolutePath()))
    {
        m_lastError = ConfigError::DirectoryCreationFailed;
        qCWarning(configLog) << "无法创建配置目录:" << fileInfo.absolutePath();
        qCInfo(configLog) << "结束保存配置（目录创建失败）" << "耗时(ms):" << t.elapsed();
        return false;
    }
    QFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        m_lastError = ConfigError::WriteFailed;
        qCWarning(configLog) << "无法写入配置文件:" << file.errorString() << "路径:" << m_configPath;
        qCInfo(configLog) << "结束保存配置（打开失败）" << "耗时(ms):" << t.elapsed();
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
        qCInfo(configLog) << "结束保存配置（写入失败）" << "耗时(ms):" << t.elapsed();
        return false;
    }
    m_lastError = ConfigError::NoError;
    qCDebug(configLog) << "配置已成功保存到:" << m_configPath;
    qCInfo(configLog) << "保存配置完成" << "写入字节数:" << bytesWritten << "耗时(ms):" << t.elapsed();
    return true;
}

QVariant ConfigManager::get(const QString &key, const QVariant &defaultValue) const
{
    QReadLocker locker(&m_lock);
    return m_config.value(key, defaultValue);
}

void ConfigManager::initializeAsync()
{
    // 通过队列连接确保在事件循环启动后执行
    QTimer::singleShot(0, this, [this]() { initializeSync(); });
}

void ConfigManager::initializeSync()
{
    qCInfo(configLog) << "ConfigManager 初始化开始：加载配置并启动文件监控";
    //加载配置文件
    if (!loadConfig())
    {
        qCWarning(configLog) << "加载配置文件失败，使用默认配置";
    }
    //监控配置文件变化
    watchForChanges();
    qCInfo(configLog) << "ConfigManager 初始化完成";
}

void ConfigManager::set(const QString &key, const QVariant &value)
{
    // 说明：过去在持有写锁时直接发出 configChanged 信号，
    // 若槽函数中调用 get()/getConfig()（读锁），会导致同线程下的死锁。
    // 修复：在释放锁后再发出信号。
    QVariant v = value;
    bool shouldEmit = false;
    {
        QWriteLocker locker(&m_lock);
        // 针对倍率键进行范围限制（0.001–3.0），避免极端值导致不可用
        if (key == "flowFastMultiplier" || key == "flowNormalMultiplier" || key == "timeoutCheckedMultiplier")
        {
            bool ok = false;
            double d = v.toDouble(&ok);
            if (!ok) d = 1.0;
            if (d < 0.001) d = 0.001;
            if (d > 3.0) d = 3.0;
            v = d;
        }
        if (m_config.value(key) != v)
        {
            m_config[key] = v;
            shouldEmit = true;
        }
    }
    if (shouldEmit)
    {
        emit configChanged(key, v);
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
    qCInfo(configLog) << "开始添加文件监控路径:" << m_configPath;
    m_fileWatcher->addPath(m_configPath);
    qCInfo(configLog) << "文件监控路径添加完成";
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
    // 保障新增倍率键存在（即使版本未变化）
    QVariantMap ensureKeys;
    if (!m_config.contains("flowFastMultiplier")) ensureKeys["flowFastMultiplier"] = 0.01;
    if (!m_config.contains("flowNormalMultiplier")) ensureKeys["flowNormalMultiplier"] = 0.8;
    if (!m_config.contains("timeoutCheckedMultiplier")) ensureKeys["timeoutCheckedMultiplier"] = 1.5;
    if (!ensureKeys.isEmpty())
    {
        updateConfig(ensureKeys);
        saveConfig();
    }
    // 示例旧版本迁移逻辑（保留）
    if (version < 1)
    {
        QVariantMap changes;
        changes["newKey"] = "default";
        updateConfig(changes);
        saveConfig();
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
    // 说明：避免在持锁期间发信号导致潜在死锁（槽可能读配置）。
    // 策略：收集变更，释放锁后统一发信号。
    QList<QPair<QString, QVariant>> changes;
    {
        QWriteLocker locker(&m_lock);
        QVariantMap oldConfig = m_config;
        for (auto it = config.begin(); it != config.end(); ++it)
        {
            const QString &key = it.key();
            QVariant newValue = it.value();
            // 针对倍率键进行范围限制（0.001–3.0）
            if (key == "flowFastMultiplier" || key == "flowNormalMultiplier" || key == "timeoutCheckedMultiplier")
            {
                bool ok = false;
                double d = newValue.toDouble(&ok);
                if (!ok) d = 1.0;
                if (d < 0.001) d = 0.001;
                if (d > 3.0) d = 3.0;
                newValue = d;
            }
            QVariant oldValue = oldConfig.value(key);
            if (oldValue != newValue)
            {
                m_config[key] = newValue;
                changes.append(qMakePair(key, newValue));
            }
        }
    }
    // 发出变更信号（已释放锁）
    for (const auto &chg : changes)
    {
        emit configChanged(chg.first, chg.second);
    }
    emit configReloaded();
}

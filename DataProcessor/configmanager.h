#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QFile>
#include <QReadWriteLock>
#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QLoggingCategory>
#include <QApplication>
#include <QThread>
#include <QTimer>

#include "alldefine.h"

/**
 * @brief 配置管理类
 * 负责加载、保存和监控配置文件的变化。
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager();

    //加载和保存配置
    bool loadConfig();
    bool saveConfig();

    //配置项读写
    QVariant get(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void set(const QString &key, const QVariant &value);

    //获取特定配置项
    QStringList getAddressList() const;
    void setAddressList(const QStringList &addresses);

    QStringList getIncludeTopics() const;
    QStringList getExcludeTopics() const;

    //错误处理
    ConfigError lastError() const;
    QString errorString() const;

    //获取完整配置
    QVariantMap getConfig() const;

    //批量更新配置
    void updateConfig(const QVariantMap &config);

signals:
    void configChanged(const QString &key, const QVariant &value); //配置项变更信号
    void configReloaded(); //配置文件重新加载信号

private:
    //初始化默认配置
    void initDefaultConfig();

    //获取配置文件路径
    QString getConfigPath() const;

    //配置版本迁移
    bool migrateConfig();

    //监控配置文件变化
    void watchForChanges();

    mutable QReadWriteLock m_lock; //读写锁，确保线程安全
    QVariantMap m_config; //配置数据
    QString m_configPath; //配置文件路径
    QFileSystemWatcher *m_fileWatcher; //文件监控
    ConfigError m_lastError = ConfigError::NoError; //最后错误状态
};

#endif //CONFIGMANAGER_H

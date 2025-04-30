#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QRegularExpression>

/**
 * @brief 配置管理类，负责网络配置的加载、保存和应用
 *
 * 该类提供线程安全的网络配置管理功能，支持管理员权限检查，
 * 可管理多个网络接口配置，并提供配置验证功能。
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit ConfigManager(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ConfigManager();

    //权限检查方法组 -----------------------------------------------

    /**
     * @brief 检查当前是否具有管理员权限
     * @return 是否具有管理员权限
     */
    bool isAdmin() const;

    /**
     * @brief 请求提升到管理员权限
     * @return 请求是否成功
     */
    bool requestAdminPrivileges();

    //配置管理方法组 -----------------------------------------------

    /**
     * @brief 从文件加载所有配置
     * @return 加载是否成功
     */
    bool loadConfigs();

    /**
     * @brief 保存所有配置到文件
     * @return 保存是否成功
     */
    bool saveConfigs();

    /**
     * @brief 获取网络接口列表
     * @param useCache 是否使用缓存数据
     * @return 网络接口名称列表
     */
    QStringList getNetworkInterfaces(bool useCache = true);

    //网络配置操作方法组 -------------------------------------------

    /**
     * @brief 应用指定网络配置
     * @param config 配置数据(QVariantMap格式)
     * @return 应用是否成功
     */
    bool applyConfig(const QVariantMap &config);

    //线程安全的配置访问方法组 -------------------------------------

    /**
     * @brief 获取所有配置的副本
     * @return 配置映射表(配置名->配置数据)
     */
    QMap<QString, QVariantMap> configs() const;

    /**
     * @brief 添加新配置
     * @param name 配置名称
     * @param config 配置数据
     * @return 添加是否成功
     */
    bool addConfig(const QString &name, const QVariantMap &config);

    /**
     * @brief 更新现有配置
     * @param oldName 原配置名称
     * @param newName 新配置名称
     * @param config 新配置数据
     * @return 更新是否成功
     */
    bool updateConfig(const QString &oldName, const QString &newName, const QVariantMap &config);

    /**
     * @brief 删除指定配置
     * @param name 要删除的配置名称
     * @return 删除是否成功
     */
    bool removeConfig(const QString &name);

    //工具方法组 --------------------------------------------------

    /**
     * @brief 清理网络接口名称(移除多余字符)
     * @param rawName 原始接口名称
     * @return 清理后的接口名称
     */
    static QString cleanInterfaceName(const QString &rawName);

    /**
     * @brief 验证配置数据有效性
     * @param config 要验证的配置
     * @return 配置是否有效
     */
    bool validateConfig(const QVariantMap &config) const;

signals:
    /**
     * @brief 配置应用完成信号
     * @param success 是否成功
     * @param message 结果消息
     */
    void configApplied(bool success, const QString &message) const;

    /**
     * @brief 错误发生信号
     * @param error 错误信息
     */
    void errorOccurred(const QString &error) const;

    /**
     * @brief 管理员状态变化信号
     * @param isAdmin 当前是否为管理员状态
     */
    void adminStatusChanged(bool isAdmin) const;

private:
    //成员变量 -----------------------------------------------------
    mutable QMutex m_mutex;                //互斥锁(保证线程安全)
    QMap<QString, QVariantMap> m_configs;  //配置存储(配置名->配置数据)
    QString m_configFile;                  //配置文件路径
    QStringList m_cachedInterfaces;        //缓存的网络接口列表
    QDateTime m_lastInterfaceUpdate;       //最后更新接口列表的时间

    bool m_isAdmin;                        //当前是否管理员权限
    bool m_isSaving;                       //是否正在保存配置(防止重入)

    //私有方法 -----------------------------------------------------

    /**
     * @brief 检查并更新管理员状态
     * @return 当前是否管理员权限
     */
    bool checkAdminStatus();

    /**
     * @brief 内部保存配置实现
     * @return 保存是否成功
     */
    bool internalSaveConfigs();

    /**
     * @brief 执行netsh命令并获取输出
     * @param args 命令参数列表
     * @return 命令输出结果
     */
    QString getNetshOutput(const QStringList &args) const;
};

#endif //CONFIGMANAGER_H

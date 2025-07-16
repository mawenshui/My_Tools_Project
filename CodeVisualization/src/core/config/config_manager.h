#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QDir>

/**
 * @brief 配置管理器
 * 
 * 负责应用程序配置的读取、保存和管理
 * 配置文件保存在可执行程序目录下的config文件夹中
 * 
 * @author CodeVisualization Team
 * @date 2024-01-01
 * @version 1.1.0
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取配置管理器单例实例
     * @return 配置管理器实例
     */
    static ConfigManager* instance();
    
    /**
     * @brief 析构函数
     */
    ~ConfigManager();
    
    /**
     * @brief 获取最近项目列表
     * @return 最近项目路径列表
     */
    QStringList getRecentProjects() const;
    
    /**
     * @brief 设置最近项目列表
     * @param projects 最近项目路径列表
     */
    void setRecentProjects(const QStringList &projects);
    
    /**
     * @brief 添加项目到最近项目列表
     * @param projectPath 项目路径
     */
    void addRecentProject(const QString &projectPath);
    
    /**
     * @brief 清空最近项目列表
     */
    void clearRecentProjects();
    
    /**
     * @brief 获取主题设置
     * @return 主题名称
     */
    QString getTheme() const;
    
    /**
     * @brief 设置主题
     * @param theme 主题名称
     */
    void setTheme(const QString &theme);
    
    /**
     * @brief 获取窗口几何信息
     * @return 几何信息字符串
     */
    QString getWindowGeometry() const;
    
    /**
     * @brief 设置窗口几何信息
     * @param geometry 几何信息字符串
     */
    void setWindowGeometry(const QString &geometry);
    
    /**
     * @brief 保存配置到文件
     * @return 是否保存成功
     */
    bool saveConfig();
    
    /**
     * @brief 从文件加载配置
     * @return 是否加载成功
     */
    bool loadConfig();
    
    /**
     * @brief 获取配置文件路径
     * @return 配置文件完整路径
     */
    QString getConfigFilePath() const;
    
    /**
     * @brief 重置为默认配置
     */
    void resetToDefaults();

signals:
    /**
     * @brief 配置变化信号
     */
    void configChanged();
    
    /**
     * @brief 最近项目列表变化信号
     */
    void recentProjectsChanged();

private:
    /**
     * @brief 私有构造函数（单例模式）
     * @param parent 父对象
     */
    explicit ConfigManager(QObject *parent = nullptr);
    
    /**
     * @brief 初始化配置目录
     * @return 是否初始化成功
     */
    bool initializeConfigDirectory();
    
    /**
     * @brief 创建默认配置
     */
    void createDefaultConfig();
    
    /**
     * @brief 验证项目路径是否有效
     * @param path 项目路径
     * @return 是否有效
     */
    bool isValidProjectPath(const QString &path) const;

private:
    static ConfigManager* s_instance;       ///< 单例实例
    
    QString m_configDir;                    ///< 配置目录路径
    QString m_configFilePath;               ///< 配置文件路径
    
    QJsonObject m_config;                   ///< 配置数据
    
    // 配置项
    QStringList m_recentProjects;           ///< 最近项目列表
    QString m_theme;                        ///< 主题设置
    QString m_windowGeometry;               ///< 窗口几何信息
    
    static const int MAX_RECENT_PROJECTS = 10;  ///< 最大最近项目数量
};

#endif // CONFIG_MANAGER_H
#include "config_manager.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

// 静态成员初始化
ConfigManager* ConfigManager::s_instance = nullptr;

ConfigManager* ConfigManager::instance()
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    
    if (!s_instance) {
        s_instance = new ConfigManager();
    }
    return s_instance;
}

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_theme("system")
{
    initializeConfigDirectory();
    loadConfig();
}

ConfigManager::~ConfigManager()
{
    saveConfig();
}

bool ConfigManager::initializeConfigDirectory()
{
    // 获取可执行程序目录
    QString appDir = QCoreApplication::applicationDirPath();
    m_configDir = QDir(appDir).absoluteFilePath("config");
    m_configFilePath = QDir(m_configDir).absoluteFilePath("app_config.json");
    
    // 创建配置目录
    QDir dir;
    if (!dir.exists(m_configDir)) {
        if (!dir.mkpath(m_configDir)) {
            qWarning() << "Failed to create config directory:" << m_configDir;
            return false;
        }
    }
    
    qDebug() << "Config directory initialized:" << m_configDir;
    qDebug() << "Config file path:" << m_configFilePath;
    
    return true;
}

void ConfigManager::createDefaultConfig()
{
    m_config = QJsonObject();
    m_config["version"] = "1.1.0";
    m_config["recentProjects"] = QJsonArray();
    m_config["theme"] = "system";
    m_config["windowGeometry"] = "";
    
    // 重置成员变量
    m_recentProjects.clear();
    m_theme = "system";
    m_windowGeometry.clear();
}

bool ConfigManager::loadConfig()
{
    QFileInfo fileInfo(m_configFilePath);
    if (!fileInfo.exists()) {
        qDebug() << "Config file does not exist, creating default config";
        createDefaultConfig();
        return saveConfig();
    }
    
    QFile file(m_configFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open config file for reading:" << m_configFilePath;
        createDefaultConfig();
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse config file:" << error.errorString();
        createDefaultConfig();
        return false;
    }
    
    m_config = doc.object();
    
    // 加载最近项目列表
    QJsonArray recentArray = m_config["recentProjects"].toArray();
    m_recentProjects.clear();
    for (const QJsonValue &value : recentArray) {
        QString path = value.toString();
        if (isValidProjectPath(path)) {
            m_recentProjects.append(path);
        }
    }
    
    // 加载其他配置
    m_theme = m_config["theme"].toString("system");
    m_windowGeometry = m_config["windowGeometry"].toString();
    
    qDebug() << "Config loaded successfully. Recent projects count:" << m_recentProjects.size();
    
    return true;
}

bool ConfigManager::saveConfig()
{
    // 更新配置对象
    QJsonArray recentArray;
    for (const QString &path : m_recentProjects) {
        recentArray.append(path);
    }
    
    m_config["recentProjects"] = recentArray;
    m_config["theme"] = m_theme;
    m_config["windowGeometry"] = m_windowGeometry;
    m_config["lastSaved"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    // 写入文件
    QFile file(m_configFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open config file for writing:" << m_configFilePath;
        return false;
    }
    
    QJsonDocument doc(m_config);
    qint64 bytesWritten = file.write(doc.toJson());
    file.close();
    
    if (bytesWritten == -1) {
        qWarning() << "Failed to write config file";
        return false;
    }
    
    qDebug() << "Config saved successfully to:" << m_configFilePath;
    
    return true;
}

QStringList ConfigManager::getRecentProjects() const
{
    return m_recentProjects;
}

void ConfigManager::setRecentProjects(const QStringList &projects)
{
    QStringList validProjects;
    for (const QString &path : projects) {
        if (isValidProjectPath(path)) {
            validProjects.append(path);
        }
    }
    
    // 限制数量
    while (validProjects.size() > MAX_RECENT_PROJECTS) {
        validProjects.removeLast();
    }
    
    if (m_recentProjects != validProjects) {
        m_recentProjects = validProjects;
        emit recentProjectsChanged();
        emit configChanged();
    }
}

void ConfigManager::addRecentProject(const QString &projectPath)
{
    if (!isValidProjectPath(projectPath)) {
        return;
    }
    
    // 移除已存在的相同路径
    m_recentProjects.removeAll(projectPath);
    
    // 添加到开头
    m_recentProjects.prepend(projectPath);
    
    // 限制最大数量
    while (m_recentProjects.size() > MAX_RECENT_PROJECTS) {
        m_recentProjects.removeLast();
    }
    
    emit recentProjectsChanged();
    emit configChanged();
    
    // 立即保存
    saveConfig();
}

void ConfigManager::clearRecentProjects()
{
    if (!m_recentProjects.isEmpty()) {
        m_recentProjects.clear();
        emit recentProjectsChanged();
        emit configChanged();
        
        // 立即保存
        saveConfig();
    }
}

QString ConfigManager::getTheme() const
{
    return m_theme;
}

void ConfigManager::setTheme(const QString &theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        emit configChanged();
    }
}

QString ConfigManager::getWindowGeometry() const
{
    return m_windowGeometry;
}

void ConfigManager::setWindowGeometry(const QString &geometry)
{
    if (m_windowGeometry != geometry) {
        m_windowGeometry = geometry;
        emit configChanged();
    }
}

QString ConfigManager::getConfigFilePath() const
{
    return m_configFilePath;
}

void ConfigManager::resetToDefaults()
{
    createDefaultConfig();
    emit recentProjectsChanged();
    emit configChanged();
    saveConfig();
}

bool ConfigManager::isValidProjectPath(const QString &path) const
{
    if (path.isEmpty()) {
        return false;
    }
    
    QFileInfo info(path);
    return info.exists() && info.isDir() && info.isReadable();
}
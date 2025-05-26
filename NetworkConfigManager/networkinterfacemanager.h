#ifndef NETWORKINTERFACEMANAGER_H
#define NETWORKINTERFACEMANAGER_H

#include <QObject>
#include <QStringList>
#include <QProcess>

class NetworkInterfaceManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkInterfaceManager(QObject *parent = nullptr);
    
    // 获取所有网络接口列表
    static QStringList getNetworkInterfaces();
    
    // 启用网络接口
    static bool enableInterface(const QString &interfaceName);
    
    // 禁用网络接口
    static bool disableInterface(const QString &interfaceName);
    
    // 获取接口状态
    static QString getInterfaceStatus(const QString &interfaceName);
    
private:
    // 执行netsh命令
    static QString executeNetshCommand(const QString &command);
};

#endif // NETWORKINTERFACEMANAGER_H
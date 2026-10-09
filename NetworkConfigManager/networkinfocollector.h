#ifndef NETWORKINFOCOLLECTOR_H
#define NETWORKINFOCOLLECTOR_H

#include <QObject>
#include <QVariantMap>
#include <QString>
#include <QProcess>
#include <QTextCodec>
#include <QRegularExpression>
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "configmanager.h"

class NetworkInfoCollector : public QObject
{
    Q_OBJECT

public:
    explicit NetworkInfoCollector(QObject *parent = nullptr);

    QVariantMap getCurrentNetworkConfig(const QString &interfaceName, ConfigManager *configManager);
    QVariantMap getCurrentNetworkConfig(const QString &rawInterfaceName, const QString &cleanInterfaceName, ConfigManager *configManager);
    QMap<QString, QVariantMap> getAllNetworkConfigs(ConfigManager *configManager);
    bool compareConfigs(const QVariantMap &config1, const QVariantMap &config2);
    bool isDhcpEnabled(const QString &output);

private:
    bool isValidIpAddress(const QString &ip);
    QString executePowerShellCommand(const QString &command);
    bool getNetworkConfigViaPowerShell(const QString &rawInterfaceName, QVariantMap &currentConfig);
};

#endif // NETWORKINFOCOLLECTOR_H
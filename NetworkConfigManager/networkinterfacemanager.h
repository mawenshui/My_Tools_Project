#ifndef NETWORKINTERFACEMANAGER_H
#define NETWORKINTERFACEMANAGER_H

#include <QObject>
#include <QStringList>
#include <QProcess>
#include <QList>
#include <QVariantMap>

struct InterfaceDetail {
    QString name;
    QString adminStatus;
    QString connStatus;
};

class NetworkInterfaceManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkInterfaceManager(QObject *parent = nullptr);
    
    static QStringList getNetworkInterfaces();
    static bool enableInterface(const QString &interfaceName);
    static bool disableInterface(const QString &interfaceName);
    static QString getInterfaceStatus(const QString &interfaceName);
    static QString getInterfaceAdminStatus(const QString &interfaceName);
    static QString getInterfaceConnStatus(const QString &interfaceName);
    static QList<InterfaceDetail> getAllInterfaceDetails();
    static QVariantMap captureConfig(const QString &interfaceName);

private:
    static QString executeNetshCommand(const QString &command);
    static QString executePowerShellCommand(const QString &command);

    static QString getPrimaryBackend();

    static QList<InterfaceDetail> getInterfaceDetailsViaNetsh();
    static QList<InterfaceDetail> getInterfaceDetailsViaPowerShell();

    static QString execProcess(const QString &program, const QStringList &args, int timeoutMs);
};

struct IpHelperAdapterInfo {
    QString name;
    QString ipAddress;
    QString subnetMask;
    QString gateway;
    QString primaryDns;
    QString dhcpEnabled;
    quint64 adapterIndex;
    QByteArray macAddress;
};

class IpHelperWrapper {
public:
    static bool getAdaptersInfo(QList<IpHelperAdapterInfo> &adapters);
    static QString formatMacAddress(const unsigned char *mac, int len);
};

#endif

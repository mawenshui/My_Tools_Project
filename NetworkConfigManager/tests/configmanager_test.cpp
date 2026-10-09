#include <QtTest>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include "../configmanager.h"

class ConfigManagerTest : public QObject
{
    Q_OBJECT

private slots:
    /**
     * @brief 显示名应能还原为真实网卡名
     */
    void resolvesCleanDisplayNameToRawInterfaceName();

    /**
     * @brief 精确匹配优先于清洗后的模糊匹配
     */
    void prefersExactMatchOverCleanNameMatch();

    /**
     * @brief 无匹配时应保留原始输入
     */
    void keepsOriginalNameWhenNoMatchExists();
    void validatesEveryIpv4FieldAndSubnetMask();
    void acceptsStaticConfigWithoutGatewayOrDns();
    void loadsNetworkBackupWithoutGatewayOrDns();
    void savesLoadsAndRollsBackProfiles();
    void rejectsNetworkChangesWithoutAdmin();
};

void ConfigManagerTest::resolvesCleanDisplayNameToRawInterfaceName()
{
    const QStringList availableNames = {
        QStringLiteral("WLAN 3"),
        QStringLiteral("蓝牙网络连接")
    };

    QCOMPARE(
        ConfigManager::resolveInterfaceName(QStringLiteral("WLAN"), availableNames),
        QStringLiteral("WLAN 3")
    );
}

void ConfigManagerTest::prefersExactMatchOverCleanNameMatch()
{
    const QStringList availableNames = {
        QStringLiteral("以太网"),
        QStringLiteral("以太网 2")
    };

    QCOMPARE(
        ConfigManager::resolveInterfaceName(QStringLiteral("以太网"), availableNames),
        QStringLiteral("以太网")
    );
}

void ConfigManagerTest::keepsOriginalNameWhenNoMatchExists()
{
    const QStringList availableNames = {
        QStringLiteral("本地连接"),
        QStringLiteral("WLAN 3")
    };

    QCOMPARE(
        ConfigManager::resolveInterfaceName(QStringLiteral("未知网卡"), availableNames),
        QStringLiteral("未知网卡")
    );
}

void ConfigManagerTest::validatesEveryIpv4FieldAndSubnetMask()
{
    ConfigManager manager;
    QVariantMap config{{"interface", "test-adapter"}, {"method", "static"},
                       {"ip", "192.168.1.10"}, {"subnet", "255.255.255.0"},
                       {"gateway", "192.168.1.1"}, {"primary_dns", "1.1.1.1"},
                       {"secondary_dns", "8.8.8.8"}};
    QVERIFY(manager.validateConfig(config));
    const QMap<QString, QString> invalid{{"ip", "999.168.1.10"},
                                         {"subnet", "255.0.255.0"},
                                         {"gateway", "192.168.1.999"},
                                         {"primary_dns", "300.1.1.1"},
                                         {"secondary_dns", "8.8.8.999"}};
    for(auto it = invalid.cbegin(); it != invalid.cend(); ++it)
    {
        QVariantMap candidate = config;
        candidate[it.key()] = it.value();
        QVERIFY2(!manager.validateConfig(candidate), qPrintable(it.key()));
    }
    config["interface"] = " ";
    QVERIFY(!manager.validateConfig(config));
}

void ConfigManagerTest::acceptsStaticConfigWithoutGatewayOrDns()
{
    ConfigManager manager;
    QVariantMap config{{"interface", "test-adapter"}, {"method", "static"},
                       {"ip", "192.0.2.10"}, {"subnet", "255.255.255.0"}};
    QVERIFY(manager.validateConfig(config));
    config["gateway"] = "";
    config["primary_dns"] = "";
    config["secondary_dns"] = "";
    QVERIFY(manager.validateConfig(config));
    config["secondary_dns"] = "9.9.9.9";
    QVERIFY(manager.validateConfig(config));
}

void ConfigManagerTest::loadsNetworkBackupWithoutGatewayOrDns()
{
    const QString path = QCoreApplication::applicationDirPath() + "/config/network_backups.json";
    if(QFile::exists(path)) QSKIP("Backup fixture path already exists");
    QDir().mkpath(QFileInfo(path).absolutePath());
    const QVariantMap config{{"interface", "backup-optional-adapter"}, {"method", "static"},
                             {"ip", "192.0.2.10"}, {"subnet", "255.255.255.0"}};
    QSaveFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    const QByteArray data = QJsonDocument(QJsonObject{{"backup-optional-adapter",
        QJsonArray{QJsonObject::fromVariantMap(config)}}}).toJson();
    QCOMPARE(file.write(data), static_cast<qint64>(data.size()));
    QVERIFY(file.commit());
    ConfigManager reloaded;
    const int count = reloaded.networkBackupCount("backup-optional-adapter");
    QVERIFY(QFile::remove(path));
    QCOMPARE(count, 1);
}

void ConfigManagerTest::savesLoadsAndRollsBackProfiles()
{
    const QVariantMap dhcp{{"interface", "test-adapter"}, {"method", "dhcp"}};
    const QVariantMap fixed{{"interface", "test-adapter"}, {"method", "static"},
                            {"ip", "192.0.2.10"}, {"subnet", "255.255.255.0"}};
    const QString name = "automated-profile-" + QString::number(QDateTime::currentMSecsSinceEpoch());
    ConfigManager manager;
    QVERIFY(manager.addConfig(name, dhcp));
    QVERIFY(manager.updateConfig(name, name, fixed));
    QVERIFY(manager.saveConfigs());
    const auto history = manager.getConfigHistory(name);
    QVERIFY(history.size() >= 2);
    ConfigManager reloaded;
    QVERIFY(reloaded.loadConfigs());
    QCOMPARE(reloaded.configs().value(name).value("ip").toString(), QString("192.0.2.10"));
    QVERIFY(reloaded.getConfigHistory(name).size() >= 2);
    QVERIFY(reloaded.rollbackConfig(name, history.cbegin().key()));
    QCOMPARE(reloaded.configs().value(name).value("method").toString(), QString("dhcp"));
    QVERIFY(reloaded.removeConfig(name));
    QVERIFY(reloaded.saveConfigs());
}

void ConfigManagerTest::rejectsNetworkChangesWithoutAdmin()
{
    ConfigManager manager;
    if(manager.isAdmin()) QSKIP("This check requires a non-elevated process");
    QVERIFY(!manager.applyConfig({{"interface", "test-adapter"}, {"method", "dhcp"}}));
    QCOMPARE(manager.networkBackupCount("test-adapter"), 0);
    QVERIFY(!manager.restoreLastNetworkBackup("test-adapter"));
}

QTEST_MAIN(ConfigManagerTest)

#include "configmanager_test.moc"

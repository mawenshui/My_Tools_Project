#include <QtTest>

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

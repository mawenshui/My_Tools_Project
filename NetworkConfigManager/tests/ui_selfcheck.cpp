#include "../configmanager.h"
#include "../mainwindow.h"
#include "../networkdiagnosticsdialog.h"
#include "../networkinterfacemanager.h"
#include "../networkinfocollector.h"
#include "../networkscandialog.h"
#include "../networktoolsdialog.h"
#include "../networkadapterdialog.h"
#include "../networklabdialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <iostream>
#include <QElapsedTimer>
#include <QFontMetrics>
#include <QHostAddress>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTabWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextEdit>
#include <QThread>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("NetworkConfigManagerSelfcheck");
    MainWindow window;
    auto *ip = window.findChild<QLineEdit *>("ipEdit");
    auto *subnet = window.findChild<QLineEdit *>("subnetEdit");
    auto *gateway = window.findChild<QLineEdit *>("gatewayEdit");
    auto *primaryDns = window.findChild<QLineEdit *>("primaryDnsEdit");
    auto *secondaryDns = window.findChild<QLineEdit *>("secondaryDnsEdit");
    if(!ip || !subnet || !gateway || !primaryDns || !secondaryDns) return 17;
    const auto dotsOnly = [](QLineEdit *edit) {
        return edit && edit->text() == QStringLiteral("...") &&
            QString(edit->displayText()).remove(QChar(0x2060)) == QStringLiteral("...") &&
            !edit->displayText().contains(' ') &&
            QFontMetrics(edit->font()).horizontalAdvance(edit->displayText()) ==
                QFontMetrics(edit->font()).horizontalAdvance(QStringLiteral("..."));
    };
    for(QLineEdit *edit : {ip, subnet, gateway, primaryDns, secondaryDns})
        if(!dotsOnly(edit)) return 18;
    window.findChild<QRadioButton *>("staticRadio")->click();
    auto *interfaceCombo = window.findChild<QComboBox *>("interfaceCombo");
    if(interfaceCombo->currentText().isEmpty()) interfaceCombo->addItem(QStringLiteral("test-adapter"));
    QVariantMap form;
    if(!QMetaObject::invokeMethod(&window, "getCurrentFormConfig", Q_RETURN_ARG(QVariantMap, form)) ||
       !form.value("ip").toString().isEmpty() || !form.value("subnet").toString().isEmpty() ||
       !form.value("gateway").toString().isEmpty() ||
       !form.value("primary_dns").toString().isEmpty() ||
       !form.value("secondary_dns").toString().isEmpty()) return 33;
    ip->setText(QStringLiteral("192.168.4.1"));
    subnet->setText(QStringLiteral("255.255.255.0"));
    if(ip->text() != QStringLiteral("192.168.4.1") ||
       subnet->text() != QStringLiteral("255.255.255.0")) return 35;
    ip->setText(QStringLiteral("192.168.100.200"));
    subnet->setText(QStringLiteral("255.255.255.128"));
    if(!QMetaObject::invokeMethod(&window, "getCurrentFormConfig", Q_RETURN_ARG(QVariantMap, form))) return 19;
    if(form.value("ip").toString() != QStringLiteral("192.168.100.200") ||
       form.value("subnet").toString() != QStringLiteral("255.255.255.128") ||
       !form.value("gateway").toString().isEmpty() ||
       !form.value("primary_dns").toString().isEmpty() ||
       !form.value("secondary_dns").toString().isEmpty()) return 20;
    ConfigManager profileManager;
    NetworkAdapterDialog adapterDialog(&profileManager);
    auto *adapterTabs = adapterDialog.findChild<QTabWidget *>();
    if(!adapterTabs || adapterTabs->count() != 6) return 29;
    auto *addIpv4 = [&adapterDialog]() -> QPushButton * {
        for(auto *button : adapterDialog.findChildren<QPushButton *>())
            if(button->text() == QStringLiteral("添加附加 IPv4")) return button;
        return nullptr;
    }();
    if(!addIpv4) return 36;
    bool addIpv4MaskOk = false;
    QTimer::singleShot(0, [&]() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if(!dialog) return;
        addIpv4MaskOk = dotsOnly(dialog->findChild<QLineEdit *>("newIpAddress"));
        dialog->reject();
    });
    addIpv4->click();
    if(!addIpv4MaskOk) return 37;
    QString scanRange;
    if(!NetworkLabDialog::subnetSummary("192.0.2.4/31", &scanRange).contains(QStringLiteral("可用地址 2")) ||
       scanRange != QStringLiteral("192.0.2.4/31") ||
       !NetworkLabDialog::subnetSummary("2001:db8::1/128").contains(QStringLiteral("地址数量 1"))) return 30;
    NetworkLabDialog lab(QString{});
    if(!dotsOnly(lab.findChild<QLineEdit *>("mtuTarget")) ||
       !dotsOnly(lab.findChild<QLineEdit *>("lanHost"))) return 38;
    auto *labTabs = lab.findChild<QTabWidget *>();
    if(!labTabs || labTabs->count() != 8)
    { std::cerr << "Lab tabs: " << (labTabs ? labTabs->count() : -1) << '\n'; return 31; }
    if(!profileManager.validateConfig(form)) return 21;
    ip->setText(QStringLiteral("invalid"));
    if(!QMetaObject::invokeMethod(&window, "getCurrentFormConfig", Q_RETURN_ARG(QVariantMap, form)) ||
       profileManager.validateConfig(form)) return 34;
    ip->setText(QStringLiteral("192.168.100.200"));
    if(window.styleSheet().isEmpty()) return 1;
    if(!window.styleSheet().contains("qlineargradient") ||
       !window.styleSheet().contains("#fbfdff")) return 23;
    app.processEvents();
    if(window.findChild<QPushButton *>("latencyTestButton")->y() <=
       window.findChild<QPushButton *>("batchNetworkButton")->y()) return 24;
    if(window.minimumWidth() < 980 || window.minimumHeight() < 690) return 26;
    const QString screenshot = qEnvironmentVariable("NETWORKCONFIGMANAGER_SCREENSHOT");
    if(!screenshot.isEmpty() && !window.grab().save(screenshot)) return 25;
    const QStringList buttons{"batchNetworkButton", "restoreNetworkButton", "scanNetworkButton",
                              "networkConnectionsButton", "latencyTestButton", "speedTestButton"};
    for(const QString &name : buttons)
        if(!window.findChild<QPushButton *>(name)) return 2;
    if(!window.findChild<QWidget *>("networkTrafficMonitor")) return 3;

    const auto checkDialog = [&](const QString &buttonName, const char *className, int tab) {
        bool opened = false;
        QTimer::singleShot(300, [&]() {
            for(QWidget *widget : QApplication::topLevelWidgets())
            {
                auto *dialog = qobject_cast<QDialog *>(widget);
                if(!dialog || !dialog->isVisible()) continue;
                opened = QByteArray(dialog->metaObject()->className()) == className;
                if(opened && tab >= 0)
                {
                    auto *tabs = dialog->findChild<QTabWidget *>();
                    opened = tabs && tabs->currentIndex() == tab;
                }
                dialog->reject();
            }
        });
        window.findChild<QPushButton *>(buttonName)->click();
        return opened;
    };
    if(!checkDialog("batchNetworkButton", "NetworkToolsDialog", -1)) return 4;
    if(!checkDialog("scanNetworkButton", "NetworkScanDialog", -1)) return 5;
    if(!checkDialog("latencyTestButton", "NetworkDiagnosticsDialog", 0)) return 6;
    if(!checkDialog("speedTestButton", "NetworkDiagnosticsDialog", 1)) return 7;

    QString routed, unrouted;
    NetworkInfoCollector collector;
    for(const InterfaceDetail &detail : NetworkInterfaceManager::getAllInterfaceDetails())
    {
        const QVariantMap config = NetworkInterfaceManager::captureConfig(detail.name);
        if(!config.isEmpty())
        {
            for(const QString &separator : {QStringLiteral("\n"), QStringLiteral("\r")})
            {
                const QString displayName = detail.name + separator + QStringLiteral(" (专用)");
                const QVariantMap observed = collector.getCurrentNetworkConfig(displayName, &profileManager);
                if(observed.value("method") != config.value("method") ||
                   observed.value("interface") != config.value("interface")) return 27;
                if(config.value("method") == "dhcp" &&
                   !collector.compareConfigs(observed, {{"interface", displayName}, {"method", "dhcp"}}))
                    return 28;
            }
        }
        if(!config.value("ip").toString().isEmpty() &&
           !config.value("gateway").toString().isEmpty() &&
           !config.value("primary_dns").toString().isEmpty()) routed = detail.name;
        else if(!config.value("ip").toString().isEmpty()) unrouted = detail.name;
    }
    if(!routed.isEmpty() && !unrouted.isEmpty())
    {
        NetworkDiagnosticsDialog fallback(unrouted);
        const QVariantMap selectedConfig = NetworkInterfaceManager::captureConfig(
            fallback.findChild<QComboBox *>("latencyInterface")->currentText());
        if(selectedConfig.value("gateway").toString().isEmpty() ||
           selectedConfig.value("primary_dns").toString().isEmpty())
        { std::cerr << "Route fallback: source=" << unrouted.toLocal8Bit().constData()
                    << " selected=" << fallback.findChild<QComboBox *>("latencyInterface")->currentText().toLocal8Bit().constData()
                    << " routed=" << routed.toLocal8Bit().constData() << '\n'; return 11; }
        if(qEnvironmentVariableIsSet("NETWORKCONFIGMANAGER_TEST_LATENCY"))
        {
            QPushButton *detect = nullptr;
            for(QPushButton *button : fallback.findChildren<QPushButton *>())
                if(button->text() == QStringLiteral("开始检测")) detect = button;
            if(!detect) return 13;
            detect->click();
            QElapsedTimer latencyWait;
            latencyWait.start();
            while(latencyWait.elapsed() < 12000 && !detect->isEnabled())
            { app.processEvents(); QThread::msleep(10); }
            const QString result = fallback.findChild<QTextEdit *>("latencyOutput")->toPlainText();
            if(!detect->isEnabled() || result.contains(QStringLiteral("未提供地址")))
            { std::cerr << "Latency result: " << result.toLocal8Bit().constData() << '\n'; return 13; }
        }
    }

    QTcpServer server;
    if(!server.listen(QHostAddress::LocalHost)) return 8;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&]() {
        while(server.hasPendingConnections())
        {
            QTcpSocket *socket = server.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
                socket->readAll();
                QTimer::singleShot(100, socket, [socket]() {
                    const QByteArray payload(128 * 1024, 'x');
                    socket->write("HTTP/1.1 200 OK\r\nContent-Length: 131072\r\nConnection: close\r\n\r\n");
                    socket->write(payload);
                    socket->disconnectFromHost();
                });
            });
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });
    NetworkDiagnosticsDialog diagnostics(QString(), nullptr, true);
    auto *region = diagnostics.findChild<QComboBox *>("speedRegion");
    auto *source = diagnostics.findChild<QComboBox *>("speedSource");
    if(!region || !source) return 15;
    region->setCurrentIndex(1);
    if(source->itemData(0).toString() != QStringLiteral("https://speed.cloudflare.com/__down?bytes=50000000") ||
       source->itemData(1).toString() != QStringLiteral("https://fsn1-speed.hetzner.com/100MB.bin")) return 16;
    region->setCurrentIndex(0);
    auto *url = diagnostics.findChild<QLineEdit *>("speedUrl");
    auto *output = diagnostics.findChildren<QTextEdit *>().last();
    QPushButton *speed = nullptr;
    for(QPushButton *button : diagnostics.findChildren<QPushButton *>())
        if(button->text() == QStringLiteral("开始测速")) speed = button;
    if(!url || !output || !speed) return 9;
    url->setText(QString("http://127.0.0.1:%1/test").arg(server.serverPort()));
    speed->click();
    QElapsedTimer wait;
    wait.start();
    while(wait.elapsed() < 3500)
    {
        app.processEvents();
        QThread::msleep(10);
    }
    if(speed->text() == QStringLiteral("停止测速")) speed->click();
    if(!output->toPlainText().contains(QStringLiteral("平均")) ||
       !output->toPlainText().contains(QStringLiteral("已下载")))
    { std::cerr << "Speed result: " << output->toPlainText().toLocal8Bit().constData() << '\n'; return 10; }
    speed->click();
    wait.restart();
    while(wait.elapsed() < 1000) { app.processEvents(); QThread::msleep(10); }
    if(speed->text() == QStringLiteral("停止测速")) speed->click();
    if(!output->toPlainText().contains(QStringLiteral("平均")))
    { std::cerr << "Second speed result: " << output->toPlainText().toLocal8Bit().constData() << '\n'; return 14; }
    if(qEnvironmentVariableIsSet("NETWORKCONFIGMANAGER_TEST_HTTPS"))
    {
        const QString testUrl = qEnvironmentVariable("NETWORKCONFIGMANAGER_TEST_URL",
            QStringLiteral("https://mirrors.aliyun.com/ubuntu/ls-lR.gz"));
        url->setText(testUrl);
        speed->click();
        wait.restart();
        while(wait.elapsed() < 7000 && speed->text() == QStringLiteral("停止测速"))
        { app.processEvents(); QThread::msleep(10); }
        if(speed->text() == QStringLiteral("停止测速")) speed->click();
        const auto transferred = QRegularExpression(QStringLiteral("已下载 ([0-9.]+) MiB"))
                                     .match(output->toPlainText());
        if(!transferred.hasMatch() || transferred.captured(1).toDouble() <= 0.0)
        { std::cerr << "HTTPS result: " << output->toPlainText().toLocal8Bit().constData() << '\n'; return 12; }
    }
    lab.findChild<QLineEdit *>("httpDiagnosticUrl")
       ->setText(QString("http://127.0.0.1:%1/test").arg(server.serverPort()));
    lab.findChild<QPushButton *>("httpDiagnosticButton")->click();
    wait.restart();
    while(wait.elapsed() < 3000 && !lab.findChild<QTextEdit *>("httpOutput")
          ->toPlainText().contains(QStringLiteral("状态：HTTP/1.1 200")))
    { app.processEvents(); QThread::msleep(10); }
    if(!lab.findChild<QTextEdit *>("httpOutput")->toPlainText().contains(QStringLiteral("状态：HTTP/1.1 200")))
    { std::cerr << "HTTP diagnostic: " << lab.findChild<QTextEdit *>("httpOutput")->toPlainText().toLocal8Bit().constData() << '\n'; return 32; }
    if(qEnvironmentVariableIsSet("NETWORKCONFIGMANAGER_TEST_REPORT") && !routed.isEmpty())
    {
        lab.findChild<QComboBox *>("labAdapter")->setCurrentText(routed);
        lab.findChild<QPushButton *>("captureReportButton")->click();
        wait.restart();
        auto *preview = lab.findChild<QTextEdit *>("reportPreview");
        while(wait.elapsed() < 30000 && !preview->toPlainText().contains(QStringLiteral("\"snapshot\"")))
        { app.processEvents(); QThread::msleep(20); }
        if(!preview->toPlainText().contains(QStringLiteral("\"snapshot\"")))
        { std::cerr << "Report capture: " << preview->toPlainText().toLocal8Bit().constData() << '\n'; return 33; }
        lab.findChild<QCheckBox *>("reportRedact")->click();
        if(!preview->toPlainText().contains(QStringLiteral("[IP]"))) return 34;
    }
    if(qEnvironmentVariableIsSet("NETWORKCONFIGMANAGER_TEST_LAN") && !routed.isEmpty())
    {
        const QString localIp = NetworkInterfaceManager::captureConfig(routed).value("ip").toString();
        NetworkLabDialog receiver(routed);
        lab.findChild<QComboBox *>("labAdapter")->setCurrentText(routed);
        const int port = 52011;
        receiver.findChild<QSpinBox *>("lanPort")->setValue(port);
        lab.findChild<QSpinBox *>("lanPort")->setValue(port);
        receiver.findChild<QPushButton *>("lanListen")->click();
        lab.findChild<QLineEdit *>("lanHost")->setText(localIp);
        lab.findChild<QSpinBox *>("lanSeconds")->setValue(1);
        lab.findChild<QPushButton *>("lanSend")->click();
        wait.restart();
        while(wait.elapsed() < 3500 && !receiver.findChild<QTextEdit *>("lanOutput")
              ->toPlainText().contains(QStringLiteral("接收")))
        { app.processEvents(); QThread::msleep(10); }
        if(!receiver.findChild<QTextEdit *>("lanOutput")->toPlainText().contains(QStringLiteral("接收")))
        { std::cerr << "LAN result: " << receiver.findChild<QTextEdit *>("lanOutput")->toPlainText().toLocal8Bit().constData() << '\n'; return 35; }
    }
    return 0;
}

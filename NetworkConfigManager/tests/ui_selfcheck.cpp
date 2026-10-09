#include "../configmanager.h"
#include "../mainwindow.h"
#include "../networkdiagnosticsdialog.h"
#include "../networkinterfacemanager.h"
#include "../networkscandialog.h"
#include "../networktoolsdialog.h"

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <iostream>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
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
    auto *gateway = window.findChild<QLineEdit *>("gatewayEdit");
    auto *primaryDns = window.findChild<QLineEdit *>("primaryDnsEdit");
    auto *secondaryDns = window.findChild<QLineEdit *>("secondaryDnsEdit");
    if(!gateway || !primaryDns || !secondaryDns) return 17;
    if(gateway->text() != QStringLiteral("...") ||
       primaryDns->text() != QStringLiteral("...") ||
       secondaryDns->text() != QStringLiteral("...")) return 18;
    window.findChild<QRadioButton *>("staticRadio")->click();
    auto *interfaceCombo = window.findChild<QComboBox *>("interfaceCombo");
    if(interfaceCombo->currentText().isEmpty()) interfaceCombo->addItem(QStringLiteral("test-adapter"));
    window.findChild<QLineEdit *>("ipEdit")->setText(QStringLiteral("192.168.100.200"));
    window.findChild<QLineEdit *>("subnetEdit")->setText(QStringLiteral("255.255.255.128"));
    QVariantMap form;
    if(!QMetaObject::invokeMethod(&window, "getCurrentFormConfig", Q_RETURN_ARG(QVariantMap, form))) return 19;
    if(!form.value("gateway").toString().isEmpty() ||
       !form.value("primary_dns").toString().isEmpty() ||
       !form.value("secondary_dns").toString().isEmpty()) return 20;
    ConfigManager profileManager;
    if(!profileManager.validateConfig(form)) return 21;
    if(window.styleSheet().isEmpty()) return 1;
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
    for(const InterfaceDetail &detail : NetworkInterfaceManager::getAllInterfaceDetails())
    {
        const QVariantMap config = NetworkInterfaceManager::captureConfig(detail.name);
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
    auto *url = diagnostics.findChild<QLineEdit *>();
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
    if(!output->toPlainText().contains(QStringLiteral("平均"))) return 14;
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
        if(!transferred.hasMatch() || transferred.captured(1).toDouble() <= 0.1)
        { std::cerr << "HTTPS result: " << output->toPlainText().toLocal8Bit().constData() << '\n'; return 12; }
    }
    return 0;
}

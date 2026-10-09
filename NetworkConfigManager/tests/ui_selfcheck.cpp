#include "../mainwindow.h"
#include "../networkdiagnosticsdialog.h"
#include "../networkscandialog.h"
#include "../networktoolsdialog.h"

#include <QApplication>
#include <QDialog>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QLineEdit>
#include <QPushButton>
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
       !output->toPlainText().contains(QStringLiteral("已下载"))) return 10;
    return 0;
}

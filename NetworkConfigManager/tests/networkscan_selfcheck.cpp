#include <QApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QTableWidget>
#include <QTcpServer>
#include <QThread>
#include "../networkscandialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QString error;
    if(NetworkScanDialog::parseHosts("192.168.1.1-3", &error).size() != 3) return 1;
    if(NetworkScanDialog::parseHosts("192.168.1.0/30", &error).size() != 4) return 2;
    if(!NetworkScanDialog::parseHosts("192.168.1.0/20", &error).isEmpty()) return 3;
    if(NetworkScanDialog::parsePorts("22,80-81,80", &error) != QList<int>({22, 80, 81})) return 4;
    if(NetworkScanDialog::parsePorts("1-1024,1-1024", &error).size() != 1024) return 5;
    if(!NetworkScanDialog::parsePorts("1-1025", &error).isEmpty()) return 6;
    if(NetworkScanDialog::subnetCidr("192.168.10.72", "255.255.255.0", &error) != "192.168.10.0/24") return 13;
    if(NetworkScanDialog::subnetCidr("10.1.9.2", "255.255.248.0", &error) != "10.1.8.0/21") return 14;
    if(!NetworkScanDialog::subnetCidr("10.1.9.2", "255.255.0.0", &error).isEmpty() || !error.contains("2048")) return 15;
    if(!NetworkScanDialog::subnetCidr("192.168.1.2", "255.0.255.0", &error).isEmpty()) return 16;
    QTcpServer server;
    if(!server.listen(QHostAddress::LocalHost)) return 7;
    NetworkScanDialog dialog;
    const auto inputs = dialog.findChildren<QLineEdit *>();
    if(inputs.size() != 2) return 8;
    inputs[1]->setText(QString::number(server.serverPort()));
    const auto start = dialog.findChild<QPushButton *>();
    const auto progress = dialog.findChild<QProgressBar *>("scanProgress");
    const auto results = dialog.findChild<QTableWidget *>();
    if(!start || !results || !progress) return 9;
    if(!dialog.autoScan("127.0.0.1", "255.255.255.255")) return 17;
    if(inputs[0]->text() != "127.0.0.1/32") return 18;
    if(progress->isHidden() || progress->maximum() != 1) return 11;
    QElapsedTimer wait;
    wait.start();
    while(wait.elapsed() < 6000 && results->rowCount() == 0)
    {
        app.processEvents();
        QThread::msleep(10);
    }
    if(results->rowCount() != 1 || results->item(0, 0)->text() != "127.0.0.1" ||
       results->item(0, 3)->text() != QString::number(server.serverPort())) return 10;
    if(progress->value() != 1) return 12;
    return 0;
}

#include <QApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QLineEdit>
#include <QPushButton>
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
    QTcpServer server;
    if(!server.listen(QHostAddress::LocalHost)) return 7;
    NetworkScanDialog dialog;
    const auto inputs = dialog.findChildren<QLineEdit *>();
    if(inputs.size() != 2) return 8;
    inputs[0]->setText("127.0.0.1");
    inputs[1]->setText(QString::number(server.serverPort()));
    const auto start = dialog.findChild<QPushButton *>();
    const auto results = dialog.findChild<QTableWidget *>();
    if(!start || !results) return 9;
    start->click();
    QElapsedTimer wait;
    wait.start();
    while(wait.elapsed() < 6000 && results->rowCount() == 0)
    {
        app.processEvents();
        QThread::msleep(10);
    }
    if(results->rowCount() != 1 || results->item(0, 0)->text() != "127.0.0.1" ||
       results->item(0, 3)->text() != QString::number(server.serverPort())) return 10;
    return 0;
}

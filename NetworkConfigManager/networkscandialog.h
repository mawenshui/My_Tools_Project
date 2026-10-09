#ifndef NETWORKSCANDIALOG_H
#define NETWORKSCANDIALOG_H

#include <QDialog>
#include <QFutureWatcher>
#include <QStringList>
#include <QList>

class QLineEdit;
class QTableWidget;
class QPushButton;
class QLabel;

struct ScanResult
{
    QString ip;
    QString mac;
    QString hostname;
    QStringList ports;
    bool responds = false;
};

class NetworkScanDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NetworkScanDialog(QWidget *parent = nullptr);
    ~NetworkScanDialog() override;
    static QStringList parseHosts(const QString &range, QString *error);
    static QList<int> parsePorts(const QString &input, QString *error);

private:
    void startScan();
    void addResult(int index);
    QLineEdit *m_range;
    QLineEdit *m_ports;
    QTableWidget *m_results;
    QPushButton *m_start;
    QLabel *m_status;
    QFutureWatcher<ScanResult> m_watcher;
};

#endif

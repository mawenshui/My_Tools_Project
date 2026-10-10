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
class QProgressBar;

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
    static QString subnetCidr(const QString &ip, const QString &mask, QString *error);
    bool autoScan(const QString &ip, const QString &mask);

private:
    void startScan();
    void addResult(int index);
    QLineEdit *m_range;
    QLineEdit *m_ports;
    QTableWidget *m_results;
    QPushButton *m_start;
    QLabel *m_status;
    QProgressBar *m_progress;
    int m_completed = 0;
    QFutureWatcher<ScanResult> m_watcher;
};

#endif

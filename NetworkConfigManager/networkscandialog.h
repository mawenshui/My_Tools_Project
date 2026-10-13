#ifndef NETWORKSCANDIALOG_H
#define NETWORKSCANDIALOG_H

#include <QDialog>
#include <QFutureWatcher>
#include <QStringList>
#include <QList>
#include <QHash>

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
    QStringList portStates;
    QStringList sources;
    bool responds = false;
    QString firstSeen;
    QString lastSeen;
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
    void setRange(const QString &range);

private:
    void startScan();
    void addResult(int index);
    QLineEdit *m_range;
    QLineEdit *m_ports;
    QLineEdit *m_filter;
    QTableWidget *m_results;
    QPushButton *m_start;
    QLabel *m_status;
    QProgressBar *m_progress;
    int m_concurrency = 8;
    int m_timeout = 350;
    int m_delay = 0;
    QHash<QString, QString> m_firstSeen;
    int m_completed = 0;
    QFutureWatcher<ScanResult> m_watcher;
};

#endif

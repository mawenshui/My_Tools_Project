#ifndef NETWORKLABDIALOG_H
#define NETWORKLABDIALOG_H

#include <QDialog>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QProcess>
#include <QVector>
#include <functional>

class QAbstractSocket;
class QCheckBox;
class QComboBox;
class QDnsLookup;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTextEdit;
class QTimer;
class QUdpSocket;

class NetworkLabDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NetworkLabDialog(const QString &adapterName, QWidget *parent = nullptr);
    ~NetworkLabDialog() override;
    static QString subnetSummary(const QString &cidr, QString *range = nullptr);

private:
    void runCommand(const QString &program, const QStringList &args,
                    std::function<void(const QString &, int)> done);
    void runPing();
    void finishPing(const QString &output, int code);
    void runTrace();
    void runMtu();
    void nextHealth();
    void runDns();
    void startLan();
    void stopLan();
    void readLan();
    void runHttp();
    void finishHttp(const QString &error = QString());
    void captureReport();
    void showReport(const QJsonObject &report);
    void saveReport();
    void compareReports();
    QString sourceIp() const;

    QComboBox *m_adapter;
    QProcess m_command;
    std::function<void(const QString &, int)> m_commandDone;
    QString m_commandOutput;
    QTextEdit *m_activeOutput = nullptr;

    QLineEdit *m_pingTarget;
    QSpinBox *m_pingInterval;
    QTextEdit *m_pingOutput;
    QPushButton *m_pingButton;
    QPushButton *m_pauseButton;
    QTimer *m_pingTimer;
    bool m_pingActive = false;
    bool m_pingPaused = false;
    int m_pingSent = 0;
    int m_pingLost = 0;
    int m_pingUnreachable = 0;
    QVector<int> m_pingTimes;

    QLineEdit *m_traceTarget;
    QSpinBox *m_traceHops;
    QSpinBox *m_traceTimeout;
    QCheckBox *m_traceContinuous;
    QTextEdit *m_traceOutput;
    QPushButton *m_traceButton;
    bool m_traceActive = false;

    QLineEdit *m_dnsName;
    QLineEdit *m_dnsServer;
    QComboBox *m_dnsType;
    QTextEdit *m_dnsOutput;
    QDnsLookup *m_dnsLookup = nullptr;

    QLineEdit *m_cidr;
    QTextEdit *m_subnetOutput;
    QLineEdit *m_mtuTarget;
    QTextEdit *m_mtuOutput;
    QPushButton *m_mtuButton;
    bool m_mtuActive = false;
    int m_mtuLow = 575;
    int m_mtuHigh = 1472;

    QLineEdit *m_healthTarget;
    QTextEdit *m_healthOutput;
    QVector<QPair<QString, QString>> m_healthSteps;
    int m_healthIndex = 0;

    QLineEdit *m_lanHost;
    QSpinBox *m_lanPort;
    QSpinBox *m_lanSeconds;
    QSpinBox *m_lanRate;
    QTextEdit *m_lanOutput;
    QUdpSocket *m_udp;
    QTimer *m_lanTimer;
    QElapsedTimer m_lanClock;
    quint32 m_lanSequence = 0;
    quint32 m_lanReceived = 0;
    quint32 m_lanLost = 0;
    qint64 m_lanBytes = 0;
    bool m_lanSending = false;

    QLineEdit *m_httpUrl;
    QTextEdit *m_httpOutput;
    QAbstractSocket *m_httpSocket = nullptr;
    QElapsedTimer m_httpClock;
    qint64 m_httpDnsMs = 0;
    qint64 m_httpConnectMs = 0;
    qint64 m_httpTlsMs = 0;
    QString m_httpBuffer;
    QString m_httpHost;
    QString m_httpPath;
    bool m_httpSecure = false;

    QTextEdit *m_reportPreview;
    QCheckBox *m_redact;
    QComboBox *m_compareA;
    QComboBox *m_compareB;
    QJsonObject m_report;
};

#endif

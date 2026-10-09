#ifndef NETWORKDIAGNOSTICSDIALOG_H
#define NETWORKDIAGNOSTICSDIALOG_H

#include <QDialog>
#include <QElapsedTimer>
#include <QList>
#include <QVector>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QThread;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class NetworkDiagnosticsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NetworkDiagnosticsDialog(const QString &interfaceName, QWidget *parent = nullptr,
                                      bool showSpeed = false);
    ~NetworkDiagnosticsDialog() override;

private:
    void startLatency();
    void startSpeed();
    void stopSpeed();
    void issueDownload();
    void sampleSpeed();

    QComboBox *m_interface;
    QComboBox *m_region;
    QComboBox *m_source;
    QLineEdit *m_url;
    QTextEdit *m_latencyOutput;
    QTextEdit *m_speedOutput;
    QPushButton *m_latencyButton;
    QPushButton *m_speedButton;
    QThread *m_latencyThread = nullptr;
    QNetworkAccessManager *m_network;
    QList<QNetworkReply *> m_replies;
    QTimer *m_sampleTimer;
    QElapsedTimer m_speedClock;
    QVector<double> m_samples;
    qint64 m_bytes = 0;
    qint64 m_lastSampleBytes = 0;
    qint64 m_lastSampleMs = 0;
    bool m_running = false;
    QString m_speedError;
};

#endif

#ifndef DDSIMULATORWIDGET_H
#define DDSIMULATORWIDGET_H

#include <QObject>
#include <QWidget>
#include <QThread>
#include <QMutex>
#include <QString>

class QLineEdit;
class QPushButton;
class QSpinBox;
class QProgressBar;
class QTextEdit;

class DDSendWorker : public QObject
{
    Q_OBJECT
public:
    explicit DDSendWorker(QObject *parent = nullptr);

signals:
    void logMessage(const QString &level, const QString &message);
    void progressChanged(int value);
    void workingStateChanged(bool running);
    void finished();

public slots:
    void startWork(const QString &rootDir,
                   const QString &patternText,
                   int chunkSize,
                   int intervalMs,
                   const QString &targetIp,
                   quint16 targetPort);
    void setPaused(bool paused);
    void stopWork();

private:
    bool m_running;
    bool m_paused;
    bool m_stopRequested;
    QMutex m_stateMutex;
};

class DDSimulatorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DDSimulatorWidget(QWidget *parent = nullptr);
    ~DDSimulatorWidget();

signals:
    void requestStartWork(const QString &rootDir,
                          const QString &patternText,
                          int chunkSize,
                          int intervalMs,
                          const QString &targetIp,
                          quint16 targetPort);
    void requestPause(bool paused);
    void requestStop();

private slots:
    void onBrowseFolder();
    void onStartClicked();
    void onPauseClicked();
    void onStopClicked();
    void onLogMessage(const QString &level, const QString &message);
    void onProgressChanged(int value);
    void onWorkingStateChanged(bool running);
    void onWorkerFinished();

private:
    void setupUi();
    void appendLog(const QString &level, const QString &message);
    void updateButtonState(bool running, bool paused);
    bool validateInput(QString &error) const;

private:
    QLineEdit *m_folderEdit;
    QLineEdit *m_patternEdit;
    QLineEdit *m_ipEdit;
    QSpinBox *m_portSpin;
    QSpinBox *m_chunkSizeSpin;
    QSpinBox *m_intervalSpin;
    QPushButton *m_browseButton;
    QPushButton *m_startButton;
    QPushButton *m_pauseButton;
    QPushButton *m_stopButton;
    QProgressBar *m_progressBar;
    QTextEdit *m_logView;
    QThread m_workerThread;
    DDSendWorker *m_worker;
    bool m_running;
    bool m_paused;
};

#endif // DDSIMULATORWIDGET_H

#include "ddsimulatorwidget.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QDateTime>
#include <QHostAddress>
#include <QUdpSocket>
#include <QThread>
#include <QMutexLocker>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QMessageBox>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <algorithm>

DDSendWorker::DDSendWorker(QObject *parent)
    : QObject(parent),
      m_running(false),
      m_paused(false),
      m_stopRequested(false)
{
}

void DDSendWorker::startWork(const QString &rootDir,
                             const QString &patternText,
                             int chunkSize,
                             int intervalMs,
                             const QString &targetIp,
                             quint16 targetPort)
{
    {
        QMutexLocker locker(&m_stateMutex);
        if (m_running)
        {
            emit logMessage("WARN", QStringLiteral("已有任务正在发送，忽略本次开始请求"));
            return;
        }
        m_running = true;
        m_paused = false;
        m_stopRequested = false;
    }
    emit workingStateChanged(true);
    emit progressChanged(0);

    QStringList filters;
    const QStringList raw = patternText.split(QRegularExpression("[;,\\s]+"), QString::SkipEmptyParts);
    for (const QString &item : raw)
    {
        const QString p = item.trimmed();
        if (!p.isEmpty())
        {
            filters << p;
        }
    }
    if (filters.isEmpty())
    {
        filters << "*";
    }

    QStringList files;
    QDirIterator it(rootDir, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        files << it.next();
    }
    std::sort(files.begin(), files.end(), [](const QString &a, const QString &b)
    {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });

    if (files.isEmpty())
    {
        emit logMessage("WARN", QStringLiteral("未找到匹配文件"));
        {
            QMutexLocker locker(&m_stateMutex);
            m_running = false;
            m_paused = false;
            m_stopRequested = false;
        }
        emit progressChanged(100);
        emit workingStateChanged(false);
        emit finished();
        return;
    }

    qint64 totalBytes = 0;
    for (const QString &path : files)
    {
        totalBytes += QFileInfo(path).size();
    }
    if (totalBytes <= 0)
    {
        totalBytes = 1;
    }

    QUdpSocket socket;
    QHostAddress address(targetIp);
    qint64 sentBytes = 0;
    int sentFiles = 0;

    emit logMessage("INFO", QStringLiteral("开始发送，文件数: %1").arg(files.size()));
    for (const QString &path : files)
    {
        {
            QMutexLocker locker(&m_stateMutex);
            if (m_stopRequested)
            {
                break;
            }
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
        {
            emit logMessage("ERROR", QStringLiteral("打开失败: %1").arg(path));
            continue;
        }

        const qint64 currentFileSize = qMax(static_cast<qint64>(1), file.size());
        qint64 currentFileSentBytes = 0;
        QElapsedTimer fileProgressLogTimer;
        fileProgressLogTimer.start();
        qint64 lastProgressLogMs = 0;
        emit logMessage("INFO", QStringLiteral("发送文件: %1").arg(path));
        while (!file.atEnd())
        {
            bool shouldStop = false;
            bool paused = false;
            {
                QMutexLocker locker(&m_stateMutex);
                shouldStop = m_stopRequested;
                paused = m_paused;
            }
            if (shouldStop)
            {
                break;
            }
            if (paused)
            {
                QThread::msleep(50);
                continue;
            }

            const QByteArray payload = file.read(chunkSize);
            if (payload.isEmpty())
            {
                break;
            }
            const qint64 bytes = socket.writeDatagram(payload, address, targetPort);
            if (bytes < 0)
            {
                emit logMessage("ERROR", QStringLiteral("发送失败: %1").arg(socket.errorString()));
            }
            else
            {
                sentBytes += payload.size();
                currentFileSentBytes += payload.size();
                const int value = qBound(0, static_cast<int>(sentBytes * 100 / totalBytes), 100);
                emit progressChanged(value);
                const qint64 elapsedMs = fileProgressLogTimer.elapsed();
                if (elapsedMs - lastProgressLogMs >= 1000)
                {
                    const int currentFilePercent = qBound(0, static_cast<int>(currentFileSentBytes * 100 / currentFileSize), 100);
                    emit logMessage("INFO",
                                    QStringLiteral("文件发送进度: %1% (%2/%3 字节)")
                                        .arg(currentFilePercent)
                                        .arg(currentFileSentBytes)
                                        .arg(currentFileSize));
                    lastProgressLogMs = elapsedMs;
                }
            }
            if (intervalMs > 0)
            {
                QThread::msleep(static_cast<unsigned long>(intervalMs));
            }
        }
        const int finalFilePercent = qBound(0, static_cast<int>(currentFileSentBytes * 100 / currentFileSize), 100);
        emit logMessage("INFO",
                        QStringLiteral("文件发送进度: %1% (%2/%3 字节)")
                            .arg(finalFilePercent)
                            .arg(currentFileSentBytes)
                            .arg(currentFileSize));
        file.close();
        ++sentFiles;
    }

    const bool stoppedByUser = [&]()
    {
        QMutexLocker locker(&m_stateMutex);
        return m_stopRequested;
    }();

    if (stoppedByUser)
    {
        emit logMessage("INFO", QStringLiteral("发送已停止"));
    }
    else
    {
        emit progressChanged(100);
        emit logMessage("INFO", QStringLiteral("发送完成，已处理文件数: %1").arg(sentFiles));
    }

    {
        QMutexLocker locker(&m_stateMutex);
        m_running = false;
        m_paused = false;
        m_stopRequested = false;
    }
    emit workingStateChanged(false);
    emit finished();
}

void DDSendWorker::setPaused(bool paused)
{
    QMutexLocker locker(&m_stateMutex);
    if (!m_running)
    {
        return;
    }
    m_paused = paused;
}

void DDSendWorker::stopWork()
{
    QMutexLocker locker(&m_stateMutex);
    if (!m_running)
    {
        return;
    }
    m_stopRequested = true;
}

DDSimulatorWidget::DDSimulatorWidget(QWidget *parent)
    : QWidget(parent),
      m_folderEdit(nullptr),
      m_patternEdit(nullptr),
      m_ipEdit(nullptr),
      m_portSpin(nullptr),
      m_chunkSizeSpin(nullptr),
      m_intervalSpin(nullptr),
      m_browseButton(nullptr),
      m_startButton(nullptr),
      m_pauseButton(nullptr),
      m_stopButton(nullptr),
      m_progressBar(nullptr),
      m_logView(nullptr),
      m_worker(new DDSendWorker),
      m_running(false),
      m_paused(false)
{
    setupUi();
    m_worker->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &DDSimulatorWidget::requestStartWork, m_worker, &DDSendWorker::startWork, Qt::QueuedConnection);
    connect(this, &DDSimulatorWidget::requestPause, m_worker, &DDSendWorker::setPaused, Qt::DirectConnection);
    connect(this, &DDSimulatorWidget::requestStop, m_worker, &DDSendWorker::stopWork, Qt::DirectConnection);
    connect(m_worker, &DDSendWorker::logMessage, this, &DDSimulatorWidget::onLogMessage, Qt::QueuedConnection);
    connect(m_worker, &DDSendWorker::progressChanged, this, &DDSimulatorWidget::onProgressChanged, Qt::QueuedConnection);
    connect(m_worker, &DDSendWorker::workingStateChanged, this, &DDSimulatorWidget::onWorkingStateChanged, Qt::QueuedConnection);
    connect(m_worker, &DDSendWorker::finished, this, &DDSimulatorWidget::onWorkerFinished, Qt::QueuedConnection);
    m_workerThread.start();
    updateButtonState(false, false);
}

DDSimulatorWidget::~DDSimulatorWidget()
{
    emit requestStop();
    m_workerThread.quit();
    m_workerThread.wait(2000);
}

void DDSimulatorWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    auto *formLayout = new QFormLayout();
    auto *folderLayout = new QHBoxLayout();
    m_folderEdit = new QLineEdit(this);
    m_browseButton = new QPushButton(QStringLiteral("选择文件夹"), this);
    folderLayout->addWidget(m_folderEdit);
    folderLayout->addWidget(m_browseButton);
    formLayout->addRow(QStringLiteral("目录"), folderLayout);

    m_patternEdit = new QLineEdit(this);
    m_patternEdit->setPlaceholderText(QStringLiteral("*.dat;*.bin，留空表示全部文件"));
    formLayout->addRow(QStringLiteral("文件过滤"), m_patternEdit);

    m_ipEdit = new QLineEdit(QStringLiteral("192.168.4.11"), this);
    formLayout->addRow(QStringLiteral("目标IP"), m_ipEdit);

    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(2027);
    formLayout->addRow(QStringLiteral("目标端口"), m_portSpin);

    m_chunkSizeSpin = new QSpinBox(this);
    m_chunkSizeSpin->setRange(1, 65535);
    m_chunkSizeSpin->setValue(672);
    formLayout->addRow(QStringLiteral("分块字节数"), m_chunkSizeSpin);

    m_intervalSpin = new QSpinBox(this);
    m_intervalSpin->setRange(0, 2000);
    m_intervalSpin->setValue(1);
    formLayout->addRow(QStringLiteral("发送间隔(ms)"), m_intervalSpin);

    mainLayout->addLayout(formLayout);

    auto *buttonLayout = new QHBoxLayout();
    m_startButton = new QPushButton(QStringLiteral("开始发送"), this);
    m_pauseButton = new QPushButton(QStringLiteral("暂停发送"), this);
    m_stopButton = new QPushButton(QStringLiteral("停止发送"), this);
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addWidget(m_stopButton);
    mainLayout->addLayout(buttonLayout);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    mainLayout->addWidget(m_progressBar);

    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    mainLayout->addWidget(m_logView, 1);

    connect(m_browseButton, &QPushButton::clicked, this, &DDSimulatorWidget::onBrowseFolder);
    connect(m_startButton, &QPushButton::clicked, this, &DDSimulatorWidget::onStartClicked);
    connect(m_pauseButton, &QPushButton::clicked, this, &DDSimulatorWidget::onPauseClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &DDSimulatorWidget::onStopClicked);
}

void DDSimulatorWidget::onBrowseFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择数据目录"), m_folderEdit->text());
    if (!dir.isEmpty())
    {
        m_folderEdit->setText(dir);
    }
}

void DDSimulatorWidget::onStartClicked()
{
    QString error;
    if (!validateInput(error))
    {
        QMessageBox::warning(this, QStringLiteral("参数错误"), error);
        return;
    }
    m_progressBar->setValue(0);
    m_logView->clear();
    m_running = true;
    m_paused = false;
    updateButtonState(true, false);
    emit requestStartWork(m_folderEdit->text().trimmed(),
                          m_patternEdit->text().trimmed(),
                          m_chunkSizeSpin->value(),
                          m_intervalSpin->value(),
                          m_ipEdit->text().trimmed(),
                          static_cast<quint16>(m_portSpin->value()));
}

void DDSimulatorWidget::onPauseClicked()
{
    if (!m_running)
    {
        return;
    }
    m_paused = !m_paused;
    m_worker->setPaused(m_paused);
    appendLog("INFO", m_paused ? QStringLiteral("已暂停发送") : QStringLiteral("已继续发送"));
    updateButtonState(true, m_paused);
}

void DDSimulatorWidget::onStopClicked()
{
    m_worker->stopWork();
    appendLog("INFO", QStringLiteral("已收到停止请求，正在停止发送..."));
}

void DDSimulatorWidget::onLogMessage(const QString &level, const QString &message)
{
    appendLog(level, message);
}

void DDSimulatorWidget::onProgressChanged(int value)
{
    m_progressBar->setValue(value);
}

void DDSimulatorWidget::onWorkingStateChanged(bool running)
{
    m_running = running;
    if (!running)
    {
        m_paused = false;
    }
    updateButtonState(m_running, m_paused);
}

void DDSimulatorWidget::onWorkerFinished()
{
    m_running = false;
    m_paused = false;
    updateButtonState(false, false);
}

void DDSimulatorWidget::appendLog(const QString &level, const QString &message)
{
    const QString line = QStringLiteral("[%1] [%2] %3")
                             .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"),
                                  level,
                                  message);
    m_logView->append(line);
}

void DDSimulatorWidget::updateButtonState(bool running, bool paused)
{
    m_startButton->setEnabled(!running);
    m_pauseButton->setEnabled(running);
    m_stopButton->setEnabled(running);
    m_pauseButton->setText(paused ? QStringLiteral("继续发送") : QStringLiteral("暂停发送"));
    m_browseButton->setEnabled(!running);
    m_folderEdit->setEnabled(!running);
    m_patternEdit->setEnabled(!running);
    m_ipEdit->setEnabled(!running);
    m_portSpin->setEnabled(!running);
    m_chunkSizeSpin->setEnabled(!running);
    m_intervalSpin->setEnabled(!running);
}

bool DDSimulatorWidget::validateInput(QString &error) const
{
    const QString dir = m_folderEdit->text().trimmed();
    if (dir.isEmpty())
    {
        error = QStringLiteral("请选择目录");
        return false;
    }
    QDir checkDir(dir);
    if (!checkDir.exists())
    {
        error = QStringLiteral("目录不存在");
        return false;
    }
    QHostAddress ip;
    if (!ip.setAddress(m_ipEdit->text().trimmed()))
    {
        error = QStringLiteral("目标IP无效");
        return false;
    }
    return true;
}

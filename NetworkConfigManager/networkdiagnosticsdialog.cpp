#include "networkdiagnosticsdialog.h"
#include "networkinterfacemanager.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QTabWidget>
#include <QTcpSocket>
#include <QTextEdit>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QMetaObject>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#endif

namespace {
QString probe(const QString &label, const QString &target, const QString &source, quint16 fallbackPort)
{
    if(target.isEmpty()) return QObject::tr("%1：当前网卡未提供地址").arg(label);
    QList<qint64> times;
    int lost = 0;
    for(int index = 0; index < 3; ++index)
    {
        QElapsedTimer clock;
        clock.start();
        QProcess ping;
        QStringList args{"-n", "1", "-w", "700"};
        if(!source.isEmpty()) args << "-S" << source;
        args << target;
        ping.start("ping", args);
        bool success = ping.waitForFinished(1200) && ping.exitCode() == 0;
        if(!success)
        {
            if(ping.state() != QProcess::NotRunning) { ping.kill(); ping.waitForFinished(); }
            QTcpSocket socket;
            if(!source.isEmpty()) socket.bind(QHostAddress(source));
            clock.restart();
            socket.connectToHost(target, fallbackPort);
            success = socket.waitForConnected(700);
            socket.abort();
        }
        if(success) times << clock.elapsed();
        else ++lost;
    }
    if(times.isEmpty()) return QObject::tr("%1 (%2)：丢包 100%").arg(label, target);
    qint64 total = 0;
    for(qint64 value : times) total += value;
    const auto minmax = std::minmax_element(times.cbegin(), times.cend());
    return QObject::tr("%1 (%2)：最小 %3 ms / 平均 %4 ms / 最大 %5 ms，丢包 %6%")
            .arg(label, target).arg(*minmax.first).arg(total / times.size())
            .arg(*minmax.second).arg(lost * 100 / 3);
}

#ifdef _WIN32
QString downloadOnce(const QUrl &url, std::atomic<bool> &running, std::atomic<qint64> &bytes,
                     const std::atomic<int> &runId, int currentRun)
{
    const QString host = QString::fromLatin1(QUrl::toAce(url.host()));
    QString path = url.path(QUrl::FullyEncoded);
    if(path.isEmpty()) path = "/";
    if(url.hasQuery()) path += "?" + url.query(QUrl::FullyEncoded);
    const HINTERNET session = WinHttpOpen(L"NetworkConfigManager/2.2.2", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if(!session) return QObject::tr("WinHTTP 初始化失败（%1）").arg(GetLastError());
    WinHttpSetTimeouts(session, 3000, 3000, 3000, 3000);
    const HINTERNET connection = WinHttpConnect(session, reinterpret_cast<LPCWSTR>(host.utf16()),
                                                 static_cast<INTERNET_PORT>(url.port(url.scheme() == "https" ? 443 : 80)), 0);
    if(!connection) { const DWORD error = GetLastError(); WinHttpCloseHandle(session); return QObject::tr("连接失败（%1）").arg(error); }
    const HINTERNET request = WinHttpOpenRequest(connection, L"GET", reinterpret_cast<LPCWSTR>(path.utf16()),
                                                nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                url.scheme() == "https" ? WINHTTP_FLAG_SECURE : 0);
    if(!request) { const DWORD error = GetLastError(); WinHttpCloseHandle(connection); WinHttpCloseHandle(session); return QObject::tr("请求失败（%1）").arg(error); }
    QString error;
    if(!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
       !WinHttpReceiveResponse(request, nullptr))
        error = QObject::tr("下载连接失败（%1）").arg(GetLastError());
    else
    {
        DWORD status = 0, size = sizeof(status);
        if(WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                               WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX) && status >= 400)
            error = QObject::tr("HTTP %1").arg(status);
        char buffer[64 * 1024];
        while(error.isEmpty() && running && runId == currentRun && bytes < 100ll * 1024 * 1024)
        {
            DWORD received = 0;
            if(!WinHttpReadData(request, buffer, sizeof(buffer), &received))
            { error = QObject::tr("下载中断（%1）").arg(GetLastError()); break; }
            if(!received) break;
            if(runId == currentRun) bytes += received;
        }
    }
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return error;
}
#endif
}

NetworkDiagnosticsDialog::NetworkDiagnosticsDialog(const QString &interfaceName, QWidget *parent,
                                                   bool showSpeed)
    : QDialog(parent), m_interface(new QComboBox(this)),
      m_region(new QComboBox(this)), m_source(new QComboBox(this)), m_url(new QLineEdit(this)),
      m_latencyOutput(new QTextEdit(this)), m_speedOutput(new QTextEdit(this)),
      m_latencyButton(new QPushButton(tr("开始检测"), this)),
      m_speedButton(new QPushButton(tr("开始测速"), this)),
      m_sampleTimer(new QTimer(this))
{
    setWindowTitle(tr("网络诊断"));
    resize(650, 450);
    auto *layout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget(this);
    layout->addWidget(tabs);

    auto *latencyPage = new QWidget(tabs);
    auto *latencyLayout = new QVBoxLayout(latencyPage);
    m_interface->setObjectName("latencyInterface");
    for(const InterfaceDetail &detail : NetworkInterfaceManager::getAllInterfaceDetails())
        if(!detail.name.isEmpty()) m_interface->addItem(detail.name);
    const int selected = m_interface->findText(interfaceName);
    if(selected >= 0) m_interface->setCurrentIndex(selected);
    latencyLayout->addWidget(new QLabel(tr("目标来自当前选中网卡的网关、DNS，并检测公网 DNS。"), latencyPage));
    latencyLayout->addWidget(m_interface);
    const QVariantMap selectedConfig = NetworkInterfaceManager::captureConfig(m_interface->currentText());
    auto *interfaceHint = new QLabel(latencyPage);
    if(selectedConfig.value("gateway").toString().isEmpty() ||
       selectedConfig.value("primary_dns").toString().isEmpty())
    {
        for(int index = 0; index < m_interface->count(); ++index)
        {
            const QVariantMap candidate = NetworkInterfaceManager::captureConfig(m_interface->itemText(index));
            if(candidate.value("ip").toString().isEmpty() ||
               candidate.value("gateway").toString().isEmpty() ||
               candidate.value("primary_dns").toString().isEmpty()) continue;
            m_interface->setCurrentIndex(index);
            if(selected >= 0 && index != selected)
                interfaceHint->setText(tr("已自动选择有网关和 DNS 的网卡：%1（原选中：%2）")
                                       .arg(m_interface->currentText(), interfaceName));
            break;
        }
    }
    interfaceHint->setVisible(!interfaceHint->text().isEmpty());
    latencyLayout->addWidget(interfaceHint);
    connect(m_interface, QOverload<int>::of(&QComboBox::currentIndexChanged), interfaceHint,
            [interfaceHint](int) { interfaceHint->hide(); });
    latencyLayout->addWidget(m_latencyButton);
    m_latencyOutput->setObjectName("latencyOutput");
    m_latencyOutput->setReadOnly(true);
    latencyLayout->addWidget(m_latencyOutput);
    tabs->addTab(latencyPage, tr("延迟检测"));

    auto *speedPage = new QWidget(tabs);
    auto *speedLayout = new QVBoxLayout(speedPage);
    auto *selection = new QHBoxLayout;
    m_region->setObjectName("speedRegion");
    m_source->setObjectName("speedSource");
    m_region->addItems({tr("国内"), tr("国外"), tr("内网"), tr("自定义")});
    selection->addWidget(m_region);
    selection->addWidget(m_source);
    speedLayout->addLayout(selection);
    m_url->setPlaceholderText(tr("http(s)://... 下载文件地址"));
    speedLayout->addWidget(m_url);
    speedLayout->addWidget(new QLabel(tr("主动测速：最多 4 路并发（Hetzner 单路）、10 秒、100 MiB；数据不保存。"), speedPage));
    speedLayout->addWidget(m_speedButton);
    m_speedOutput->setReadOnly(true);
    speedLayout->addWidget(m_speedOutput);
    tabs->addTab(speedPage, tr("下载测速"));
    tabs->setCurrentIndex(showSpeed ? 1 : 0);

    connect(m_latencyButton, &QPushButton::clicked, this, &NetworkDiagnosticsDialog::startLatency);
    connect(m_region, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int region) {
        m_source->clear();
        if(region == 0)
        {
            m_source->addItem(tr("阿里云"), "https://mirrors.aliyun.com/ubuntu/ls-lR.gz");
            m_source->addItem(tr("腾讯云"), "https://mirrors.cloud.tencent.com/ubuntu/ls-lR.gz");
            m_source->addItem(tr("中科大"), "https://mirrors.ustc.edu.cn/ubuntu/ls-lR.gz");
        }
        else if(region == 1)
        {
            m_source->addItem("Cloudflare", "https://speed.cloudflare.com/__down?bytes=50000000");
            m_source->addItem("Hetzner FSN1", "https://fsn1-speed.hetzner.com/100MB.bin");
        }
        else m_source->addItem(tr("填写地址"), QString());
        m_url->setText(m_source->currentData().toString());
    });
    connect(m_source, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if(!m_source->currentData().toString().isEmpty()) m_url->setText(m_source->currentData().toString());
    });
    m_region->setCurrentIndex(1);
    m_region->setCurrentIndex(0);
    connect(m_speedButton, &QPushButton::clicked, this, [this]() {
        if(m_running) stopSpeed(); else startSpeed();
    });
    m_sampleTimer->setInterval(250);
    connect(m_sampleTimer, &QTimer::timeout, this, &NetworkDiagnosticsDialog::sampleSpeed);
}

NetworkDiagnosticsDialog::~NetworkDiagnosticsDialog()
{
    stopSpeed();
    for(QThread *download : m_downloads) { download->wait(); delete download; }
    if(m_latencyThread) m_latencyThread->wait();
}

void NetworkDiagnosticsDialog::startLatency()
{
    if(m_latencyThread) return;
    const QVariantMap config = NetworkInterfaceManager::captureConfig(m_interface->currentText());
    if(config.isEmpty()) { QMessageBox::warning(this, tr("延迟检测"), tr("无法读取当前网卡配置。")); return; }
    const QString source = config.value("ip").toString();
    const QString gateway = config.value("gateway").toString();
    const QString dns = config.value("primary_dns").toString();
    m_latencyButton->setEnabled(false);
    m_latencyOutput->setPlainText(tr("检测中..."));
    m_latencyThread = QThread::create([this, source, gateway, dns]() {
        const QString output = QStringList{
            probe(QObject::tr("网关"), gateway, source, 80),
            probe(QObject::tr("首选 DNS"), dns, source, 53),
            probe(QObject::tr("公网 DNS"), "223.5.5.5", source, 53)
        }.join("\n");
        QMetaObject::invokeMethod(this, [this, output]() {
            m_latencyOutput->setPlainText(output);
            m_latencyButton->setEnabled(true);
        }, Qt::QueuedConnection);
    });
    connect(m_latencyThread, &QThread::finished, m_latencyThread, &QObject::deleteLater);
    connect(m_latencyThread, &QThread::finished, this, [this]() {
        m_latencyThread = nullptr;
    });
    m_latencyThread->start();
}

void NetworkDiagnosticsDialog::startSpeed()
{
    const QUrl url = QUrl::fromUserInput(m_url->text().trimmed());
    if(!url.isValid() || (url.scheme() != "http" && url.scheme() != "https") || url.host().isEmpty())
    { QMessageBox::warning(this, tr("下载测速"), tr("请输入有效的 HTTP 或 HTTPS 地址。")); return; }
    m_running = true;
    ++m_runId;
    m_speedError.clear();
    m_bytes = m_lastSampleBytes = m_lastSampleMs = 0;
    m_samples.clear();
    m_speedClock.start();
    m_speedButton->setText(tr("停止测速"));
    m_speedOutput->setPlainText(tr("正在下载并采样..."));
    m_sampleTimer->start();
    const int connections = url.host().endsWith("-speed.hetzner.com") ? 1 : 4;
    for(int index = 0; index < connections; ++index) issueDownload();
}

void NetworkDiagnosticsDialog::issueDownload()
{
    if(!m_running) return;
    const QUrl url = QUrl::fromUserInput(m_url->text().trimmed());
    const int currentRun = m_runId;
    auto *download = QThread::create([this, url, currentRun]() {
#ifdef _WIN32
        while(m_running && m_runId == currentRun && m_speedClock.elapsed() < 10000 &&
              m_bytes < 100ll * 1024 * 1024)
        {
            const QString error = downloadOnce(url, m_running, m_bytes, m_runId, currentRun);
            if(error.isEmpty()) continue;
            QMetaObject::invokeMethod(this, [this, error, currentRun]() {
                if(!m_running || m_runId != currentRun) return;
                m_speedError = error;
                stopSpeed();
            }, Qt::QueuedConnection);
            break;
        }
#else
        Q_UNUSED(url);
#endif
    });
    m_downloads << download;
    connect(download, &QThread::finished, this, [this, download]() {
        m_downloads.removeOne(download);
        download->deleteLater();
    });
    download->start();
}

void NetworkDiagnosticsDialog::sampleSpeed()
{
    const qint64 elapsed = m_speedClock.elapsed();
    if(m_bytes >= 100ll * 1024 * 1024) { stopSpeed(); return; }
    if(elapsed >= 10000) { stopSpeed(); return; }
    if(elapsed < 3000) { m_lastSampleBytes = m_bytes; m_lastSampleMs = elapsed; return; }
    const qint64 duration = elapsed - m_lastSampleMs;
    if(duration > 0)
    {
        const double mibPerSec = (m_bytes - m_lastSampleBytes) * 1000.0 / duration / (1024.0 * 1024.0);
        m_samples << mibPerSec;
        m_speedOutput->setPlainText(tr("当前 %1 MiB/s (%2 Mbps)\n已下载 %3 MiB")
            .arg(mibPerSec, 0, 'f', 2).arg(mibPerSec * 8.388608, 0, 'f', 1)
            .arg(m_bytes / (1024.0 * 1024.0), 0, 'f', 1));
    }
    m_lastSampleBytes = m_bytes;
    m_lastSampleMs = elapsed;
}

void NetworkDiagnosticsDialog::stopSpeed()
{
    if(!m_running) return;
    m_running = false;
    ++m_runId;
    m_sampleTimer->stop();
    m_speedButton->setText(tr("开始测速"));
    if(m_samples.isEmpty())
    {
        if(m_bytes > 0)
        {
            const double mibPerSec = m_bytes * 1000.0 / std::max<qint64>(1, m_speedClock.elapsed()) / (1024.0 * 1024.0);
            m_speedOutput->setPlainText(tr("平均 %1 MiB/s (%2 Mbps)\n已下载 %3 MiB%4")
                .arg(mibPerSec, 0, 'f', 2).arg(mibPerSec * 8.388608, 0, 'f', 1)
                .arg(m_bytes / (1024.0 * 1024.0), 0, 'f', 1)
                .arg(m_speedError.isEmpty() ? QString() : "\n" + m_speedError));
        }
        else m_speedOutput->setPlainText(m_speedError.isEmpty() ? tr("测速已停止，未收到下载数据。") : m_speedError);
        return;
    }
    std::sort(m_samples.begin(), m_samples.end());
    const double p90 = m_samples.at((m_samples.size() - 1) * 9 / 10);
    double total = 0;
    for(double sample : m_samples) total += sample;
    const double average = total / m_samples.size();
    m_speedOutput->setPlainText(tr("平均 %1 MiB/s (%2 Mbps)\n90 分位 %3 MiB/s\n已下载 %4 MiB%5")
        .arg(average, 0, 'f', 2).arg(average * 8.388608, 0, 'f', 1)
        .arg(p90, 0, 'f', 2).arg(m_bytes / (1024.0 * 1024.0), 0, 'f', 1)
        .arg(m_speedError.isEmpty() ? QString() : "\n" + m_speedError));
}

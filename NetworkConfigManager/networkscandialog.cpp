#include "networkscandialog.h"

#include <QtConcurrent>
#include <QClipboard>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QHostInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMutex>
#include <QMutexLocker>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QSaveFile>
#include <QSemaphore>
#include <QSharedPointer>
#include <QSpinBox>
#include <QTableWidget>
#include <QTcpSocket>
#include <QTimer>
#include <QEventLoop>
#include <QVBoxLayout>
#include <QThread>
#include <algorithm>

namespace {
bool ipv4Number(const QString &text, quint32 *value)
{
    const QStringList parts = text.split('.');
    if(parts.size() != 4) return false;
    quint32 number = 0;
    for(const QString &part : parts)
    {
        if(part.isEmpty() || part.size() > 3 || !QRegularExpression("^[0-9]+$").match(part).hasMatch()) return false;
        bool ok = false;
        const uint octet = part.toUInt(&ok);
        if(!ok || octet > 255) return false;
        number = (number << 8) | octet;
    }
    *value = number;
    return true;
}

ScanResult scanHost(const QString &ip, const QList<int> &ports, int timeout)
{
    ScanResult result;
    result.ip = ip;
    QProcess ping;
    ping.start("ping", QStringList() << "-n" << "1" << "-w" << QString::number(timeout) << ip);
    if(ping.waitForFinished(timeout + 1000)) result.responds = ping.exitCode() == 0;
    else { ping.kill(); ping.waitForFinished(); }
    if(result.responds) result.sources << "ICMP";

    QEventLoop loop;
    QList<QTcpSocket *> sockets;
    QSet<int> resolved;
    for(int port : ports)
    {
        auto *socket = new QTcpSocket;
        QElapsedTimer clock; clock.start();
        sockets.append(socket);
        QObject::connect(socket, &QTcpSocket::connected, &loop, [socket, port, clock, &result, &resolved]() {
            if(resolved.contains(port)) return;
            resolved.insert(port);
            result.ports << QString::number(port);
            result.portStates << QString::number(port) + " 开放 " + QString::number(clock.elapsed()) + " ms";
            if(!result.sources.contains("TCP")) result.sources << "TCP";
            socket->abort();
        });
        QObject::connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
                         &loop, [socket, port, clock, &result, &resolved](QAbstractSocket::SocketError error) {
            if(resolved.contains(port)) return;
            resolved.insert(port);
            result.portStates << QString::number(port) + (error == QAbstractSocket::ConnectionRefusedError
                                 ? " 拒绝 " : " 超时/错误 ") + QString::number(clock.elapsed()) + " ms";
            socket->abort();
        });
        socket->connectToHost(ip, static_cast<quint16>(port));
    }
    QTimer::singleShot(timeout, &loop, &QEventLoop::quit);
    loop.exec();
    for(auto *socket : sockets) { socket->abort(); delete socket; }
    for(int port : ports)
        if(!resolved.contains(port)) result.portStates << QString::number(port) + " 超时 " + QString::number(timeout) + " ms";
    QProcess arp;
    arp.start("arp", QStringList() << "-a" << ip);
    if(arp.waitForFinished(1200))
    {
        const QString output = QString::fromLocal8Bit(arp.readAllStandardOutput());
        const auto match = QRegularExpression("([0-9a-fA-F]{2}(?:-[0-9a-fA-F]{2}){5})").match(output);
        if(match.hasMatch()) { result.mac = match.captured(1).toUpper(); result.sources << "ARP"; }
    }
    else { arp.kill(); arp.waitForFinished(); }
    if(!result.responds && result.ports.isEmpty() && result.mac.isEmpty() && ports.size() > 1) return result;
    const QHostInfo host = QHostInfo::fromName(ip);
    if(host.error() == QHostInfo::NoError && host.hostName() != ip) result.hostname = host.hostName();
    result.lastSeen = QDateTime::currentDateTime().toString(Qt::ISODate);
    return result;
}

struct HostScanner
{
    typedef ScanResult result_type;
    struct Pacer { QMutex mutex; qint64 next = 0; };
    HostScanner(const QList<int> &selectedPorts, int selectedTimeout, int selectedDelay,
                const QSharedPointer<QSemaphore> &selectedLimit,
                const QSharedPointer<Pacer> &selectedPacer)
        : ports(selectedPorts), timeout(selectedTimeout), delay(selectedDelay),
          limit(selectedLimit), pacer(selectedPacer) {}
    ScanResult operator()(const QString &host) const
    {
        limit->acquire();
        if(delay)
        {
            QMutexLocker lock(&pacer->mutex);
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            if(pacer->next > now) QThread::msleep(static_cast<unsigned long>(pacer->next - now));
            pacer->next = QDateTime::currentMSecsSinceEpoch() + delay;
        }
        const ScanResult result = scanHost(host, ports, timeout);
        limit->release();
        return result;
    }
    QList<int> ports;
    int timeout;
    int delay;
    QSharedPointer<QSemaphore> limit;
    QSharedPointer<Pacer> pacer;
};
}

QStringList NetworkScanDialog::parseHosts(const QString &range, QString *error)
{
    const QString input = range.trimmed();
    quint32 first = 0, last = 0;
    if(input.contains('/'))
    {
        const QStringList parts = input.split('/');
        bool ok = false;
        const int prefix = parts.value(1).toInt(&ok);
        if(parts.size() != 2 || !ipv4Number(parts.first(), &first) || !ok || prefix < 0 || prefix > 32)
        { if(error) *error = QObject::tr("无效的 CIDR 范围"); return {}; }
        const quint32 mask = prefix == 0 ? 0 : 0xffffffffu << (32 - prefix);
        first &= mask;
        last = first | ~mask;
    }
    else if(input.contains('-'))
    {
        const QStringList parts = input.split('-');
        if(parts.size() != 2 || !ipv4Number(parts.first(), &first))
        { if(error) *error = QObject::tr("无效的 IP 范围"); return {}; }
        if(parts.last().contains('.'))
        {
            if(!ipv4Number(parts.last(), &last))
            { if(error) *error = QObject::tr("无效的结束 IP"); return {}; }
        }
        else
        {
            bool ok = false;
            const uint octet = parts.last().toUInt(&ok);
            if(!ok || octet > 255)
            { if(error) *error = QObject::tr("无效的结束 IP"); return {}; }
            last = (first & 0xffffff00u) | octet;
        }
    }
    else if(ipv4Number(input, &first)) last = first;
    else { if(error) *error = QObject::tr("请输入 IPv4 地址、区间或 CIDR"); return {}; }
    if(last < first || static_cast<quint64>(last) - first + 1 > 2048)
    { if(error) *error = QObject::tr("扫描范围须递增，且不超过 2048 个地址"); return {}; }
    QStringList hosts;
    for(quint64 address = first; address <= last; ++address)
        hosts << QHostAddress(static_cast<quint32>(address)).toString();
    return hosts;
}

QList<int> NetworkScanDialog::parsePorts(const QString &input, QString *error)
{
    QSet<int> values;
    for(const QString &part : input.split(',', QString::SkipEmptyParts))
    {
        const QStringList bounds = part.trimmed().split('-');
        bool startOk = false, endOk = false;
        const int start = bounds.first().toInt(&startOk);
        const int end = bounds.size() == 1 ? start : bounds.last().toInt(&endOk);
        if(bounds.size() == 1) endOk = startOk;
        if(bounds.size() > 2 || !startOk || !endOk || start < 1 || end > 65535 || end < start)
        { if(error) *error = QObject::tr("端口格式无效或超过 1024 个"); return {}; }
        for(int port = start; port <= end; ++port)
        {
            values.insert(port);
            if(values.size() > 1024)
            { if(error) *error = QObject::tr("端口格式无效或超过 1024 个"); return {}; }
        }
    }
    if(values.isEmpty()) { if(error) *error = QObject::tr("请输入端口"); return {}; }
    QList<int> ports = values.values();
    std::sort(ports.begin(), ports.end());
    return ports;
}

QString NetworkScanDialog::subnetCidr(const QString &ip, const QString &mask, QString *error)
{
    quint32 address = 0, netmask = 0;
    if(!ipv4Number(ip, &address) || !ipv4Number(mask, &netmask) || !netmask ||
       ((~netmask & (~netmask + 1)) != 0))
    { if(error) *error = QObject::tr("当前网卡未提供有效的 IPv4 地址和子网掩码"); return {}; }
    int prefix = 0;
    for(quint32 bits = netmask; bits & 0x80000000u; bits <<= 1) ++prefix;
    if(prefix < 21)
    { if(error) *error = QObject::tr("当前网段超过 2048 个地址，请手动输入较小范围"); return {}; }
    return QHostAddress(address & netmask).toString() + "/" + QString::number(prefix);
}

bool NetworkScanDialog::autoScan(const QString &ip, const QString &mask)
{
    QString error;
    const QString range = subnetCidr(ip, mask, &error);
    if(range.isEmpty()) { m_status->setText(error); return false; }
    m_range->setText(range);
    startScan();
    return true;
}

void NetworkScanDialog::setRange(const QString &range)
{
    m_range->setText(range);
}

NetworkScanDialog::NetworkScanDialog(QWidget *parent)
    : QDialog(parent), m_range(new QLineEdit(this)), m_ports(new QLineEdit(this)),
      m_filter(new QLineEdit(this)),
      m_results(new QTableWidget(this)), m_start(new QPushButton(tr("开始扫描"), this)),
      m_status(new QLabel(this)), m_progress(new QProgressBar(this))
{
    setWindowTitle(tr("网段扫描"));
    resize(800, 520);
    auto *layout = new QVBoxLayout(this);
    auto *inputs = new QHBoxLayout;
    m_range->setPlaceholderText(tr("192.168.1.0/24 或 192.168.1.1-100"));
    m_range->setObjectName("scanRange");
    m_ports->setText("22,80,443,445,3389,8080");
    m_ports->setObjectName("scanPorts");
    m_ports->setToolTip(tr("仅检测此处列出的 TCP 端口；可用逗号或范围指定，最多 1024 个"));
    auto *presets = new QComboBox(this);
    presets->addItem(tr("常见端口"), "22,80,443,445,3389,8080");
    presets->addItem(tr("Web"), "80,443,8080,8443");
    presets->addItem(tr("远程管理"), "22,135,139,445,3389,5900");
    presets->addItem(tr("自定义"), QString());
    connect(presets, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, presets](int index) { if(index < 3) m_ports->setText(presets->itemData(index).toString()); });
    inputs->addWidget(m_range, 2);
    inputs->addWidget(m_ports, 1);
    inputs->addWidget(presets);
    inputs->addWidget(m_start);
    layout->addLayout(inputs);
    auto *controls = new QHBoxLayout;
    auto *concurrency = new QSpinBox(this);
    concurrency->setRange(1, 32);
    concurrency->setValue(8);
    concurrency->setPrefix(tr("并发 "));
    auto *timeout = new QSpinBox(this);
    timeout->setRange(100, 3000);
    timeout->setValue(350);
    timeout->setSuffix(" ms");
    auto *delay = new QSpinBox(this);
    delay->setRange(0, 1000);
    delay->setPrefix(tr("间隔 "));
    delay->setSuffix(" ms");
    m_filter->setPlaceholderText(tr("过滤 IP、主机名、端口或来源"));
    controls->addWidget(concurrency);
    controls->addWidget(timeout);
    controls->addWidget(delay);
    controls->addWidget(m_filter, 1);
    layout->addLayout(controls);
    connect(concurrency, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int value) { m_concurrency = value; });
    connect(timeout, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int value) { m_timeout = value; });
    connect(delay, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int value) { m_delay = value; });
    m_results->setColumnCount(7);
    m_results->setHorizontalHeaderLabels({tr("IP"), tr("MAC"), tr("主机名"), tr("TCP 状态"),
                                          tr("发现来源"), tr("首次发现"), tr("最近发现")});
    m_results->setSortingEnabled(true);
    m_results->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_results, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &) {
        if(auto *item = m_results->currentItem()) QGuiApplication::clipboard()->setText(item->text());
    });
    layout->addWidget(m_status);
    m_progress->setObjectName("scanProgress");
    m_progress->setTextVisible(true);
    m_progress->setFormat(tr("扫描进度 %p%"));
    m_progress->hide();
    layout->addWidget(m_progress);
    layout->addWidget(m_results);
    connect(m_filter, &QLineEdit::textChanged, this, [this](const QString &text) {
        for(int row = 0; row < m_results->rowCount(); ++row)
        {
            QString joined;
            for(int column = 0; column < m_results->columnCount(); ++column)
                if(m_results->item(row, column)) joined += m_results->item(row, column)->text() + ' ';
            m_results->setRowHidden(row, !joined.contains(text, Qt::CaseInsensitive));
        }
    });
    connect(m_start, &QPushButton::clicked, this, &NetworkScanDialog::startScan);
    connect(&m_watcher, &QFutureWatcher<ScanResult>::resultReadyAt, this, &NetworkScanDialog::addResult);
    connect(&m_watcher, &QFutureWatcher<ScanResult>::finished, this, [this]() {
        m_start->setText(tr("开始扫描"));
        const bool cancelled = m_watcher.isCanceled();
        m_status->setText(cancelled ? tr("扫描已停止：完成 %1/%2，发现 %3 台设备")
                                         .arg(m_completed).arg(m_progress->maximum()).arg(m_results->rowCount())
                                    : tr("扫描完成：发现 %1 台设备").arg(m_results->rowCount()));
        m_progress->setValue(cancelled ? m_completed : m_progress->maximum());
        m_status->setStyleSheet(QString());
        QJsonArray rows;
        for(int row = 0; row < m_results->rowCount(); ++row)
        {
            QJsonObject record;
            const QStringList keys{"ip", "mac", "hostname", "tcp", "sources", "firstSeen", "lastSeen"};
            for(int col = 0; col < keys.size(); ++col)
                record.insert(keys[col], m_results->item(row, col)->text());
            rows.append(record);
        }
        QDir().mkpath(QCoreApplication::applicationDirPath() + "/config");
        QSaveFile file(QCoreApplication::applicationDirPath() + "/config/last_scan.json");
        if(file.open(QIODevice::WriteOnly))
        { file.write(QJsonDocument(rows).toJson()); file.commit(); }
        QJsonObject firstSeen;
        for(auto it = m_firstSeen.cbegin(); it != m_firstSeen.cend(); ++it) firstSeen.insert(it.key(), it.value());
        QSaveFile historyFile(QCoreApplication::applicationDirPath() + "/config/scan_first_seen.json");
        if(historyFile.open(QIODevice::WriteOnly))
        { historyFile.write(QJsonDocument(firstSeen).toJson()); historyFile.commit(); }
    });
    QFile historyFile(QCoreApplication::applicationDirPath() + "/config/scan_first_seen.json");
    if(historyFile.open(QIODevice::ReadOnly))
    {
        const QJsonObject saved = QJsonDocument::fromJson(historyFile.readAll()).object();
        for(auto it = saved.begin(); it != saved.end(); ++it) m_firstSeen.insert(it.key(), it.value().toString());
    }
}

NetworkScanDialog::~NetworkScanDialog()
{
    m_watcher.cancel();
    m_watcher.waitForFinished();
}

void NetworkScanDialog::startScan()
{
    if(m_watcher.isRunning()) { m_watcher.cancel(); m_status->setText(tr("正在停止扫描...")); return; }
    QString error;
    const QStringList hosts = parseHosts(m_range->text(), &error);
    if(hosts.isEmpty()) { QMessageBox::warning(this, tr("网段扫描"), error); return; }
    const QList<int> ports = parsePorts(m_ports->text(), &error);
    if(ports.isEmpty()) { QMessageBox::warning(this, tr("网段扫描"), error); return; }
    m_results->setRowCount(0);
    m_completed = 0;
    m_start->setText(tr("停止扫描"));
    m_status->setText(tr("● 正在扫描：0/%1，发现 0 台设备").arg(hosts.size()));
    m_status->setStyleSheet("font-weight: 600;");
    m_progress->setRange(0, hosts.size());
    m_progress->setValue(0);
    m_progress->show();
    m_watcher.setFuture(QtConcurrent::mapped(hosts, HostScanner(ports, m_timeout, m_delay,
                                               QSharedPointer<QSemaphore>::create(m_concurrency),
                                               QSharedPointer<HostScanner::Pacer>::create())));
}

void NetworkScanDialog::addResult(int index)
{
    const ScanResult result = m_watcher.resultAt(index);
    ++m_completed;
    m_progress->setValue(m_completed);
    m_status->setText(tr("● 正在扫描：%1/%2，发现 %3 台设备")
                      .arg(m_completed).arg(m_progress->maximum())
                      .arg(m_results->rowCount() + (!result.lastSeen.isEmpty() ? 1 : 0)));
    if(result.lastSeen.isEmpty()) return;
    m_results->setSortingEnabled(false);
    const int row = m_results->rowCount();
    m_results->insertRow(row);
    const QString firstSeen = m_firstSeen.value(result.ip, result.lastSeen);
    m_firstSeen.insert(result.ip, firstSeen);
    const QStringList fields{result.ip, result.mac, result.hostname, result.portStates.join(", "),
                             result.sources.join("/"), firstSeen, result.lastSeen};
    for(int column = 0; column < fields.size(); ++column)
        m_results->setItem(row, column, new QTableWidgetItem(fields.at(column)));
    m_results->setRowHidden(row, !fields.join(' ').contains(m_filter->text(), Qt::CaseInsensitive));
    m_results->setSortingEnabled(true);
}

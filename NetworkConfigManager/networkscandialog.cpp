#include "networkscandialog.h"

#include <QtConcurrent>
#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QHostInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QTableWidget>
#include <QTcpSocket>
#include <QTimer>
#include <QEventLoop>
#include <QVBoxLayout>
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

    QEventLoop loop;
    QList<QTcpSocket *> sockets;
    for(int port : ports)
    {
        auto *socket = new QTcpSocket;
        sockets.append(socket);
        QObject::connect(socket, &QTcpSocket::connected, &loop, [socket, port, &result]() {
            result.ports << QString::number(port);
            socket->abort();
        });
        socket->connectToHost(ip, static_cast<quint16>(port));
    }
    QTimer::singleShot(timeout, &loop, &QEventLoop::quit);
    loop.exec();
    for(auto *socket : sockets) { socket->abort(); delete socket; }
    if(!result.responds && result.ports.isEmpty()) return result;

    const QHostInfo host = QHostInfo::fromName(ip);
    if(host.error() == QHostInfo::NoError && host.hostName() != ip) result.hostname = host.hostName();
    QProcess arp;
    arp.start("arp", QStringList() << "-a" << ip);
    if(arp.waitForFinished(1200))
    {
        const QString output = QString::fromLocal8Bit(arp.readAllStandardOutput());
        const auto match = QRegularExpression("([0-9a-fA-F]{2}(?:-[0-9a-fA-F]{2}){5})").match(output);
        if(match.hasMatch()) result.mac = match.captured(1).toUpper();
    }
    else { arp.kill(); arp.waitForFinished(); }
    return result;
}

struct HostScanner
{
    typedef ScanResult result_type;
    explicit HostScanner(const QList<int> &selectedPorts) : ports(selectedPorts) {}
    ScanResult operator()(const QString &host) const { return scanHost(host, ports, 350); }
    QList<int> ports;
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

NetworkScanDialog::NetworkScanDialog(QWidget *parent)
    : QDialog(parent), m_range(new QLineEdit(this)), m_ports(new QLineEdit(this)),
      m_results(new QTableWidget(this)), m_start(new QPushButton(tr("开始扫描"), this)),
      m_status(new QLabel(this)), m_progress(new QProgressBar(this))
{
    setWindowTitle(tr("网段扫描"));
    resize(800, 520);
    auto *layout = new QVBoxLayout(this);
    auto *inputs = new QHBoxLayout;
    m_range->setPlaceholderText(tr("192.168.1.0/24 或 192.168.1.1-100"));
    m_ports->setText("22,80,443,445,3389,8080");
    m_ports->setToolTip(tr("仅检测此处列出的 TCP 端口；可用逗号或范围指定，最多 1024 个"));
    inputs->addWidget(m_range, 2);
    inputs->addWidget(m_ports, 1);
    inputs->addWidget(m_start);
    layout->addLayout(inputs);
    m_results->setColumnCount(4);
    m_results->setHorizontalHeaderLabels({tr("IP"), tr("MAC"), tr("主机名"), tr("开放 TCP 端口")});
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
    });
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
    m_watcher.setFuture(QtConcurrent::mapped(hosts, HostScanner(ports)));
}

void NetworkScanDialog::addResult(int index)
{
    const ScanResult result = m_watcher.resultAt(index);
    ++m_completed;
    m_progress->setValue(m_completed);
    m_status->setText(tr("● 正在扫描：%1/%2，发现 %3 台设备")
                      .arg(m_completed).arg(m_progress->maximum())
                      .arg(m_results->rowCount() + (result.responds || !result.ports.isEmpty() ? 1 : 0)));
    if(!result.responds && result.ports.isEmpty()) return;
    m_results->setSortingEnabled(false);
    const int row = m_results->rowCount();
    m_results->insertRow(row);
    const QStringList fields{result.ip, result.mac, result.hostname, result.ports.join(", ")};
    for(int column = 0; column < fields.size(); ++column)
        m_results->setItem(row, column, new QTableWidgetItem(fields.at(column)));
    m_results->setSortingEnabled(true);
}

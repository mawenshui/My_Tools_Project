#include "networklabdialog.h"
#include "networkadapterdialog.h"
#include "networkinterfacemanager.h"
#include "networkscandialog.h"
#include "ipv4input.h"

#include <QAbstractSocket>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QtEndian>
#include <QDir>
#include <QDnsLookup>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHostAddress>
#include <QHostInfo>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSpinBox>
#include <QSslCertificate>
#include <QSslSocket>
#include <QTabWidget>
#include <QTcpSocket>
#include <QTextEdit>
#include <QTimer>
#include <QUdpSocket>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>

namespace {
QTextEdit *output(QWidget *parent, const char *name)
{
    auto *box = new QTextEdit(parent);
    box->setReadOnly(true);
    box->setObjectName(name);
    return box;
}
QString historyPath()
{
    const QString dir = QCoreApplication::applicationDirPath() + "/config";
    QDir().mkpath(dir);
    return dir + "/diagnostic_reports.json";
}
QJsonArray history()
{
    QFile file(historyPath());
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).array() : QJsonArray{};
}
QString redactText(QString text)
{
    text.replace(QRegularExpression("\\b(?:[0-9]{1,3}\\.){3}[0-9]{1,3}\\b"), "[IP]");
    text.replace(QRegularExpression("\\b[0-9a-fA-F]{2}(?:[:-][0-9a-fA-F]{2}){5}\\b"), "[MAC]");
    text.replace(QRegularExpression("(?i)(?:[0-9a-f]{1,4}:){3,}[0-9a-f]{0,4}|[0-9a-f]*::[0-9a-f:]*"), "[IP]");
    text.replace(QRegularExpression("(?i)(SSID|主机名|hostname|Name)\\s*[=:]\\s*[^,\\r\\n]+"), "\\1=[REDACTED]");
    const QString localHost = QHostInfo::localHostName();
    if(!localHost.isEmpty()) text.replace(localHost, "[HOST]", Qt::CaseInsensitive);
    return text;
}
QJsonValue scrub(const QJsonValue &value, const QString &key = QString())
{
    if(value.isObject())
    {
        QJsonObject result;
        const QJsonObject source = value.toObject();
        for(auto it = source.begin(); it != source.end(); ++it)
            result.insert(it.key(), scrub(it.value(), it.key()));
        return result;
    }
    if(value.isArray())
    {
        QJsonArray result;
        for(const QJsonValue &item : value.toArray()) result.append(scrub(item));
        return result;
    }
    if(value.isString())
    {
        if(QRegularExpression("^(ssid|hostname|name)$", QRegularExpression::CaseInsensitiveOption).match(key).hasMatch())
            return "[REDACTED]";
        return redactText(value.toString());
    }
    return value;
}
QJsonObject redactedReport(const QJsonObject &report)
{
    QString content = QString::fromUtf8(QJsonDocument(scrub(report).toObject()).toJson());
    for(const QJsonValue &item : report.value("scan").toArray())
    {
        const QString host = item.toObject().value("hostname").toString();
        if(host.size() >= 3) content.replace(host, "[HOST]", Qt::CaseInsensitive);
    }
    return QJsonDocument::fromJson(content.toUtf8()).object();
}
}

NetworkLabDialog::NetworkLabDialog(const QString &adapterName, QWidget *parent)
    : QDialog(parent), m_adapter(new QComboBox(this)), m_pingTarget(new QLineEdit(this)),
      m_pingInterval(new QSpinBox(this)), m_pingOutput(output(this, "pingOutput")),
      m_pingButton(new QPushButton(tr("开始 Ping"), this)),
      m_pauseButton(new QPushButton(tr("暂停"), this)), m_pingTimer(new QTimer(this)),
      m_traceTarget(new QLineEdit(this)), m_traceHops(new QSpinBox(this)),
      m_traceTimeout(new QSpinBox(this)), m_traceContinuous(new QCheckBox(tr("连续追踪"), this)),
      m_traceOutput(output(this, "traceOutput")), m_traceButton(new QPushButton(tr("开始追踪"), this)),
      m_dnsName(new QLineEdit(this)), m_dnsServer(new QLineEdit(this)), m_dnsType(new QComboBox(this)),
      m_dnsOutput(output(this, "dnsOutput")), m_cidr(new QLineEdit(this)),
      m_subnetOutput(output(this, "subnetOutput")), m_mtuTarget(new QLineEdit(this)),
      m_mtuOutput(output(this, "mtuOutput")), m_mtuButton(new QPushButton(tr("探测 MTU"), this)),
      m_healthTarget(new QLineEdit(this)), m_healthOutput(output(this, "healthOutput")),
      m_lanHost(new QLineEdit(this)), m_lanPort(new QSpinBox(this)),
      m_lanSeconds(new QSpinBox(this)), m_lanRate(new QSpinBox(this)),
      m_lanOutput(output(this, "lanOutput")), m_udp(new QUdpSocket(this)), m_lanTimer(new QTimer(this)),
      m_httpUrl(new QLineEdit(this)), m_httpOutput(output(this, "httpOutput")),
      m_reportPreview(output(this, "reportPreview")), m_redact(new QCheckBox(tr("遮盖 IP、MAC、SSID、主机名"), this)),
      m_compareA(new QComboBox(this)), m_compareB(new QComboBox(this))
{
    setWindowTitle(tr("诊断、局域网与报告"));
    resize(900, 650);
    auto *root = new QVBoxLayout(this);
    auto *header = new QHBoxLayout;
    header->addWidget(new QLabel(tr("网卡"), this));
    for(const InterfaceDetail &detail : NetworkInterfaceManager::getAllInterfaceDetails())
        if(!detail.name.isEmpty()) m_adapter->addItem(detail.name);
    m_adapter->setObjectName("labAdapter");
    m_adapter->setCurrentIndex(std::max(0, m_adapter->findText(adapterName)));
    header->addWidget(m_adapter, 1);
    root->addLayout(header);
    auto *tabs = new QTabWidget(this);
    root->addWidget(tabs);

    auto *pingPage = new QWidget(tabs);
    auto *pingLayout = new QVBoxLayout(pingPage);
    auto *pingRow = new QHBoxLayout;
    m_pingTarget->setPlaceholderText(tr("目标 IP 或主机名"));
    m_pingInterval->setRange(200, 10000);
    m_pingInterval->setValue(1000);
    m_pingInterval->setSuffix(" ms");
    pingRow->addWidget(m_pingTarget, 1);
    pingRow->addWidget(m_pingInterval);
    pingRow->addWidget(m_pingButton);
    pingRow->addWidget(m_pauseButton);
    auto *pingExport = new QPushButton(tr("导出 TXT"), pingPage);
    pingRow->addWidget(pingExport);
    pingLayout->addLayout(pingRow);
    pingLayout->addWidget(m_pingOutput);
    tabs->addTab(pingPage, tr("连续 Ping"));
    connect(m_pingButton, &QPushButton::clicked, this, [this]() {
        if(m_pingActive) { m_pingActive = false; m_pingTimer->stop(); if(m_command.state() != QProcess::NotRunning) m_command.kill(); m_pingButton->setText(tr("开始 Ping")); return; }
        if(m_pingTarget->text().trimmed().isEmpty()) return;
        m_pingActive = true; m_pingPaused = false; m_pingSent = m_pingLost = m_pingUnreachable = 0;
        m_pingTimes.clear(); m_pingOutput->clear(); m_pingButton->setText(tr("停止 Ping"));
        runPing();
    });
    connect(m_pauseButton, &QPushButton::clicked, this, [this]() {
        m_pingPaused = !m_pingPaused;
        m_pauseButton->setText(m_pingPaused ? tr("继续") : tr("暂停"));
        if(!m_pingPaused && m_pingActive && m_command.state() == QProcess::NotRunning) runPing();
    });
    connect(m_pingTimer, &QTimer::timeout, this, &NetworkLabDialog::runPing);
    m_pingTimer->setSingleShot(true);
    connect(pingExport, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getSaveFileName(this, tr("导出 Ping"), "ping.txt", "Text (*.txt)");
        if(path.isEmpty()) return;
        QSaveFile file(path);
        if(file.open(QIODevice::WriteOnly)) { file.write(m_pingOutput->toPlainText().toUtf8()); file.commit(); }
    });

    auto *tracePage = new QWidget(tabs);
    auto *traceLayout = new QVBoxLayout(tracePage);
    auto *traceRow = new QHBoxLayout;
    m_traceTarget->setPlaceholderText(tr("IPv4 / IPv6 目标"));
    m_traceHops->setRange(1, 64); m_traceHops->setValue(20); m_traceHops->setPrefix(tr("跳数 "));
    m_traceTimeout->setRange(100, 3000); m_traceTimeout->setValue(700); m_traceTimeout->setSuffix(" ms");
    traceRow->addWidget(m_traceTarget, 1); traceRow->addWidget(m_traceHops);
    traceRow->addWidget(m_traceTimeout); traceRow->addWidget(m_traceContinuous); traceRow->addWidget(m_traceButton);
    traceLayout->addLayout(traceRow); traceLayout->addWidget(m_traceOutput);
    tabs->addTab(tracePage, tr("路由追踪"));
    connect(m_traceButton, &QPushButton::clicked, this, [this]() {
        if(m_traceActive) { m_traceActive = false; if(m_command.state() != QProcess::NotRunning) m_command.kill(); m_traceButton->setText(tr("开始追踪")); return; }
        if(m_traceTarget->text().trimmed().isEmpty()) return;
        m_traceActive = true; m_traceOutput->clear(); m_traceButton->setText(tr("停止追踪")); runTrace();
    });

    auto *dnsPage = new QWidget(tabs);
    auto *dnsLayout = new QVBoxLayout(dnsPage);
    auto *dnsRow = new QHBoxLayout;
    m_dnsName->setPlaceholderText(tr("域名或 PTR 地址"));
    m_dnsServer->setPlaceholderText(tr("留空使用系统 DNS，或填写解析器 IP"));
    m_dnsType->addItems({"A", "AAAA", "CNAME", "MX", "NS", "TXT", "PTR"});
    auto *dnsButton = new QPushButton(tr("查询"), dnsPage);
    dnsRow->addWidget(m_dnsName, 2); dnsRow->addWidget(m_dnsServer, 2);
    dnsRow->addWidget(m_dnsType); dnsRow->addWidget(dnsButton);
    dnsLayout->addLayout(dnsRow); dnsLayout->addWidget(m_dnsOutput);
    tabs->addTab(dnsPage, tr("DNS 查询"));
    connect(dnsButton, &QPushButton::clicked, this, &NetworkLabDialog::runDns);

    auto *subnetPage = new QWidget(tabs);
    auto *subnetLayout = new QVBoxLayout(subnetPage);
    auto *subnetRow = new QHBoxLayout;
    m_cidr->setPlaceholderText(tr("IPv4 / IPv6 CIDR，例如 192.168.1.0/24 或 fd00::/64"));
    auto *calculate = new QPushButton(tr("计算"), subnetPage);
    auto *scan = new QPushButton(tr("填入扫描器"), subnetPage);
    subnetRow->addWidget(m_cidr, 1); subnetRow->addWidget(calculate); subnetRow->addWidget(scan);
    subnetLayout->addLayout(subnetRow); subnetLayout->addWidget(m_subnetOutput);
    auto *mtuRow = new QHBoxLayout;
    configureIpv4Input(m_mtuTarget);
    m_mtuTarget->setToolTip(tr("路径 MTU：IPv4 目标"));
    m_mtuTarget->setObjectName("mtuTarget");
    mtuRow->addWidget(m_mtuTarget, 1); mtuRow->addWidget(m_mtuButton);
    subnetLayout->addLayout(mtuRow); subnetLayout->addWidget(m_mtuOutput);
    tabs->addTab(subnetPage, tr("子网与 MTU"));
    connect(calculate, &QPushButton::clicked, this, [this]() { m_subnetOutput->setPlainText(subnetSummary(m_cidr->text())); });
    connect(scan, &QPushButton::clicked, this, [this]() {
        QString range;
        const QString summary = subnetSummary(m_cidr->text(), &range);
        if(range.isEmpty()) { QMessageBox::information(this, tr("扫描范围"), summary); return; }
        NetworkScanDialog dialog(this); dialog.setRange(range); dialog.exec();
    });
    connect(m_mtuButton, &QPushButton::clicked, this, [this]() {
        if(m_mtuActive) { m_mtuActive = false; m_command.kill(); m_mtuButton->setText(tr("探测 MTU")); return; }
        QHostAddress address;
        if(!address.setAddress(m_mtuTarget->text().trimmed()) || address.protocol() != QAbstractSocket::IPv4Protocol)
        { QMessageBox::warning(this, tr("路径 MTU"), tr("请填写 IPv4 目标地址。")); return; }
        m_mtuActive = true; m_mtuLow = 575; m_mtuHigh = 1472; m_mtuOutput->clear();
        m_mtuButton->setText(tr("停止探测")); runMtu();
    });

    auto *healthPage = new QWidget(tabs);
    auto *healthLayout = new QVBoxLayout(healthPage);
    auto *healthRow = new QHBoxLayout;
    m_healthTarget->setPlaceholderText(tr("可选：用户指定目标"));
    auto *healthButton = new QPushButton(tr("检查"), healthPage);
    healthRow->addWidget(m_healthTarget, 1); healthRow->addWidget(healthButton);
    healthLayout->addLayout(healthRow); healthLayout->addWidget(m_healthOutput);
    tabs->addTab(healthPage, tr("本地健康"));
    connect(healthButton, &QPushButton::clicked, this, [this]() {
        const QVariantMap config = NetworkInterfaceManager::captureConfig(m_adapter->currentText());
        QString link = tr("未知");
        for(const InterfaceDetail &detail : NetworkInterfaceManager::getAllInterfaceDetails())
            if(detail.name == m_adapter->currentText())
            { link = detail.adminStatus + " / " + detail.connStatus; break; }
        m_healthOutput->setPlainText(tr("%1\n链路：%2（%3）\n地址：%4；网关：%5；DNS：%6")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate), m_adapter->currentText(), link,
                 config.value("ip").toString(), config.value("gateway").toString(),
                 config.value("primary_dns").toString()));
        m_healthSteps.clear();
        if(!config.value("gateway").toString().isEmpty()) m_healthSteps.append({tr("网关可达"), config.value("gateway").toString()});
        if(!config.value("primary_dns").toString().isEmpty()) m_healthSteps.append({"DNS", config.value("primary_dns").toString()});
        if(!m_healthTarget->text().trimmed().isEmpty()) m_healthSteps.append({tr("指定目标"), m_healthTarget->text().trimmed()});
        m_healthIndex = 0; nextHealth();
    });

    auto *lanPage = new QWidget(tabs);
    auto *lanLayout = new QVBoxLayout(lanPage);
    auto *lanRow = new QHBoxLayout;
    configureIpv4Input(m_lanHost);
    m_lanHost->setToolTip(tr("另一台局域网主机 IP"));
    m_lanHost->setObjectName("lanHost");
    m_lanPort->setRange(1024, 65535); m_lanPort->setValue(52011); m_lanPort->setPrefix(tr("端口 "));
    m_lanPort->setObjectName("lanPort");
    m_lanSeconds->setRange(1, 30); m_lanSeconds->setValue(5); m_lanSeconds->setSuffix(" s");
    m_lanSeconds->setObjectName("lanSeconds");
    m_lanRate->setRange(1, 500); m_lanRate->setValue(100); m_lanRate->setSuffix(tr(" 包/秒"));
    auto *listen = new QPushButton(tr("开始监听"), lanPage);
    listen->setObjectName("lanListen");
    auto *send = new QPushButton(tr("向主机发送"), lanPage);
    send->setObjectName("lanSend");
    auto *stop = new QPushButton(tr("停止"), lanPage);
    lanRow->addWidget(m_lanHost, 2); lanRow->addWidget(m_lanPort);
    lanRow->addWidget(m_lanSeconds); lanRow->addWidget(m_lanRate);
    lanRow->addWidget(listen); lanRow->addWidget(send); lanRow->addWidget(stop);
    lanLayout->addLayout(lanRow); lanLayout->addWidget(m_lanOutput);
    tabs->addTab(lanPage, tr("LAN 吞吐"));
    connect(listen, &QPushButton::clicked, this, [this]() {
        stopLan();
        const QHostAddress local(sourceIp());
        if(local.protocol() != QAbstractSocket::IPv4Protocol)
        { m_lanOutput->setPlainText(tr("所选网卡没有 IPv4 地址。")); return; }
        if(!m_udp->bind(local, static_cast<quint16>(m_lanPort->value())))
        { m_lanOutput->setPlainText(m_udp->errorString()); return; }
        m_lanReceived = m_lanLost = m_lanSequence = 0; m_lanBytes = 0; m_lanClock.start();
        m_lanOutput->setPlainText(tr("监听 %1:%2；请在另一台机器输入本机局域网 IP。")
                                  .arg(local.toString()).arg(m_lanPort->value()));
    });
    connect(send, &QPushButton::clicked, this, &NetworkLabDialog::startLan);
    connect(stop, &QPushButton::clicked, this, &NetworkLabDialog::stopLan);
    connect(m_udp, &QUdpSocket::readyRead, this, &NetworkLabDialog::readLan);
    connect(m_lanTimer, &QTimer::timeout, this, [this]() {
        if(!m_lanSending) return;
        if(m_lanClock.elapsed() >= m_lanSeconds->value() * 1000 || m_lanBytes >= 50ll * 1024 * 1024)
        { stopLan(); return; }
        QByteArray packet(1200, 0);
        qToBigEndian(++m_lanSequence, reinterpret_cast<uchar *>(packet.data()));
        const QHostAddress host(m_lanHost->text().trimmed());
        if(m_udp->writeDatagram(packet, host, static_cast<quint16>(m_lanPort->value())) >= 0) m_lanBytes += packet.size();
    });

    auto *httpPage = new QWidget(tabs);
    auto *httpLayout = new QVBoxLayout(httpPage);
    auto *httpRow = new QHBoxLayout;
    m_httpUrl->setPlaceholderText(tr("http(s)://用户指定目标"));
    m_httpUrl->setObjectName("httpDiagnosticUrl");
    auto *httpButton = new QPushButton(tr("诊断 HEAD"), httpPage);
    httpButton->setObjectName("httpDiagnosticButton");
    httpRow->addWidget(m_httpUrl, 1); httpRow->addWidget(httpButton);
    httpLayout->addLayout(httpRow); httpLayout->addWidget(m_httpOutput);
    tabs->addTab(httpPage, tr("HTTP(S)"));
    connect(httpButton, &QPushButton::clicked, this, &NetworkLabDialog::runHttp);

    auto *reportPage = new QWidget(tabs);
    auto *reportLayout = new QVBoxLayout(reportPage);
    auto *reportRow = new QHBoxLayout;
    auto *capture = new QPushButton(tr("采集快照"), reportPage);
    capture->setObjectName("captureReportButton");
    auto *save = new QPushButton(tr("导出"), reportPage);
    auto *compare = new QPushButton(tr("比较"), reportPage);
    reportRow->addWidget(capture); reportRow->addWidget(m_redact); reportRow->addWidget(save);
    m_redact->setObjectName("reportRedact");
    reportRow->addWidget(m_compareA); reportRow->addWidget(m_compareB); reportRow->addWidget(compare);
    reportLayout->addLayout(reportRow); reportLayout->addWidget(m_reportPreview);
    tabs->addTab(reportPage, tr("本地报告"));
    connect(capture, &QPushButton::clicked, this, &NetworkLabDialog::captureReport);
    connect(save, &QPushButton::clicked, this, &NetworkLabDialog::saveReport);
    connect(compare, &QPushButton::clicked, this, &NetworkLabDialog::compareReports);
    connect(m_redact, &QCheckBox::toggled, this, [this]() { showReport(m_report); });
    for(const QJsonValue &item : history())
    {
        const QString label = item.toObject().value("time").toString() + " " + item.toObject().value("adapter").toString();
        m_compareA->addItem(label); m_compareB->addItem(label);
    }
    if(m_compareB->count() > 1) m_compareB->setCurrentIndex(m_compareB->count() - 1);

    connect(&m_command, &QProcess::readyReadStandardOutput, this, [this]() {
        const QString chunk = QString::fromLocal8Bit(m_command.readAllStandardOutput());
        m_commandOutput += chunk;
        if(m_activeOutput) m_activeOutput->insertPlainText(chunk);
    });
    connect(&m_command, &QProcess::readyReadStandardError, this, [this]() {
        m_commandOutput += QString::fromLocal8Bit(m_command.readAllStandardError());
    });
    connect(&m_command, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus) {
        m_commandOutput += QString::fromLocal8Bit(m_command.readAllStandardOutput());
        m_commandOutput += QString::fromLocal8Bit(m_command.readAllStandardError());
        auto done = m_commandDone; m_commandDone = {}; m_activeOutput = nullptr;
        if(done) done(m_commandOutput, code);
    });
}

NetworkLabDialog::~NetworkLabDialog()
{
    m_pingActive = m_traceActive = m_mtuActive = false;
    m_commandDone = {};
    m_pingTimer->stop(); m_lanTimer->stop();
    if(m_command.state() != QProcess::NotRunning) { m_command.kill(); m_command.waitForFinished(1000); }
    if(m_dnsLookup) m_dnsLookup->abort();
    if(m_httpSocket) { m_httpSocket->disconnect(this); m_httpSocket->abort(); }
    for(QProcess *process : findChildren<QProcess *>())
        if(process->state() != QProcess::NotRunning)
        { process->disconnect(this); process->kill(); process->waitForFinished(1000); }
}

QString NetworkLabDialog::sourceIp() const
{
    return NetworkInterfaceManager::captureConfig(m_adapter->currentText()).value("ip").toString();
}

void NetworkLabDialog::runCommand(const QString &program, const QStringList &args,
                                  std::function<void(const QString &, int)> done)
{
    if(m_command.state() != QProcess::NotRunning)
    { m_activeOutput = nullptr; done(tr("另一项探测正在运行。"), -1); return; }
    m_commandDone = done;
    m_commandOutput.clear();
    m_command.start(program, args);
    if(!m_command.waitForStarted(100))
    {
        m_commandDone = {};
        done(m_command.errorString(), -1);
    }
}

void NetworkLabDialog::runPing()
{
    if(!m_pingActive || m_pingPaused || m_command.state() != QProcess::NotRunning) return;
    QStringList args{"-n", "1", "-w", "1000"};
    const QString source = sourceIp();
    if(!source.isEmpty()) args << "-S" << source;
    args << m_pingTarget->text().trimmed();
    runCommand("ping", args, [this](const QString &result, int code) { finishPing(result, code); });
}

void NetworkLabDialog::finishPing(const QString &result, int code)
{
    if(!m_pingActive) return;
    ++m_pingSent;
    const bool ok = code == 0 && result.contains(QRegularExpression("TTL=", QRegularExpression::CaseInsensitiveOption));
    QString state;
    if(ok)
    {
        const auto match = QRegularExpression("(?:time|时间)[=<]?[ ]*([0-9]+)",
                         QRegularExpression::CaseInsensitiveOption).match(result);
        const int milliseconds = match.hasMatch() ? match.captured(1).toInt() : 0;
        m_pingTimes << milliseconds;
        state = tr("%1 ms").arg(milliseconds);
    }
    else if(result.contains(QRegularExpression("unreachable|不可达", QRegularExpression::CaseInsensitiveOption)))
    { ++m_pingUnreachable; state = tr("目标不可达"); }
    else { ++m_pingLost; state = tr("超时/未响应"); }
    int minimum = 0, maximum = 0, average = 0, jitter = 0;
    if(!m_pingTimes.isEmpty())
    {
        minimum = *std::min_element(m_pingTimes.cbegin(), m_pingTimes.cend());
        maximum = *std::max_element(m_pingTimes.cbegin(), m_pingTimes.cend());
        int sum = 0, delta = 0;
        for(int i = 0; i < m_pingTimes.size(); ++i)
        { sum += m_pingTimes[i]; if(i) delta += std::abs(m_pingTimes[i] - m_pingTimes[i-1]); }
        average = sum / m_pingTimes.size();
        if(m_pingTimes.size() > 1) jitter = delta / (m_pingTimes.size() - 1);
    }
    m_pingOutput->append(tr("%1  #%2 %3 | 丢包 %4/%5（超时 %6、不可达 %7）；min/avg/max %8/%9/%10 ms；抖动 %11 ms")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(m_pingSent).arg(state)
        .arg(m_pingLost + m_pingUnreachable).arg(m_pingSent).arg(m_pingLost).arg(m_pingUnreachable)
        .arg(minimum).arg(average).arg(maximum).arg(jitter));
    if(m_pingActive && !m_pingPaused) m_pingTimer->start(m_pingInterval->value());
}

void NetworkLabDialog::runTrace()
{
    if(!m_traceActive) return;
    m_traceOutput->append(tr("追踪开始：%1").arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
    m_activeOutput = m_traceOutput;
    const QString target = m_traceTarget->text().trimmed();
    const QHostAddress parsed(target);
    QStringList args{"-d", "-h", QString::number(m_traceHops->value()),
                     "-w", QString::number(m_traceTimeout->value())};
    if(parsed.protocol() == QAbstractSocket::IPv4Protocol) args << "-4";
    else if(parsed.protocol() == QAbstractSocket::IPv6Protocol) args << "-6";
    args << target;
    runCommand("tracert", args, [this](const QString &result, int code) {
        const QStringList lines = result.split(QRegularExpression("[\\r\\n]+"), QString::SkipEmptyParts);
        int hops = 0, timeouts = 0;
        QStringList hopStats;
        for(const QString &line : lines)
            if(QRegularExpression("^\\s*[0-9]+\\s").match(line).hasMatch())
            {
                ++hops; const int missed = line.count('*'); timeouts += missed;
                hopStats << tr("第 %1 跳：未响应 %2/3；RTT 见原始行").arg(hops).arg(missed);
            }
        m_traceOutput->append(hopStats.join('\n'));
        m_traceOutput->append(tr("跳数 %1；未响应探针 %2；退出码 %3。中间跳未回应不表示目标不可达。")
                              .arg(hops).arg(timeouts).arg(code));
        if(m_traceActive && m_traceContinuous->isChecked())
            QTimer::singleShot(1000, this, [this]() { if(m_traceActive) runTrace(); });
        else { m_traceActive = false; m_traceButton->setText(tr("开始追踪")); }
    });
}

void NetworkLabDialog::runDns()
{
    if(m_dnsLookup) { m_dnsLookup->abort(); m_dnsLookup->deleteLater(); m_dnsLookup = nullptr; }
    QString name = m_dnsName->text().trimmed();
    if(name.isEmpty()) return;
    const QString type = m_dnsType->currentText();
    if(type == "PTR")
    {
        QHostAddress address(name);
        if(address.protocol() == QAbstractSocket::IPv4Protocol)
        {
            QStringList octets = name.split('.'); std::reverse(octets.begin(), octets.end());
            name = octets.join('.') + ".in-addr.arpa";
        }
        else if(address.protocol() == QAbstractSocket::IPv6Protocol)
        {
            const Q_IPV6ADDR bytes = address.toIPv6Address();
            QStringList nibbles;
            for(int i = 15; i >= 0; --i)
            { nibbles << QString::number(bytes[i] & 0xf, 16) << QString::number(bytes[i] >> 4, 16); }
            name = nibbles.join('.') + ".ip6.arpa";
        }
    }
    const QHostAddress resolver(m_dnsServer->text().trimmed());
    if(!m_dnsServer->text().trimmed().isEmpty() && resolver.isNull())
    { m_dnsOutput->setPlainText(tr("解析器必须为 IP 地址。")); return; }
    QDnsLookup::Type query = QDnsLookup::A;
    if(type == "AAAA") query = QDnsLookup::AAAA;
    else if(type == "CNAME") query = QDnsLookup::CNAME;
    else if(type == "MX") query = QDnsLookup::MX;
    else if(type == "NS") query = QDnsLookup::NS;
    else if(type == "TXT") query = QDnsLookup::TXT;
    else if(type == "PTR") query = QDnsLookup::PTR;
    m_dnsLookup = new QDnsLookup(query, name, this);
    if(!resolver.isNull()) m_dnsLookup->setNameserver(resolver);
    QDnsLookup *lookup = m_dnsLookup;
    auto *clock = new QElapsedTimer;
    clock->start();
    m_dnsOutput->setPlainText(tr("查询中..."));
    connect(lookup, &QDnsLookup::finished, this, [this, lookup, clock, type]() {
        QStringList lines{tr("类型 %1；服务器 %2；耗时 %3 ms")
            .arg(type, m_dnsServer->text().trimmed().isEmpty() ? tr("系统 DNS") : m_dnsServer->text().trimmed())
            .arg(clock->elapsed())};
        delete clock;
        if(lookup->error() != QDnsLookup::NoError) lines << tr("错误：%1").arg(lookup->errorString());
        for(const auto &record : lookup->hostAddressRecords())
            lines << tr("%1  TTL %2  %3").arg(record.name()).arg(record.timeToLive()).arg(record.value().toString());
        for(const auto &record : lookup->canonicalNameRecords())
            lines << tr("%1  TTL %2  %3").arg(record.name()).arg(record.timeToLive()).arg(record.value());
        for(const auto &record : lookup->mailExchangeRecords())
            lines << tr("%1  TTL %2  优先级 %3  %4").arg(record.name()).arg(record.timeToLive())
                     .arg(record.preference()).arg(record.exchange());
        for(const auto &record : lookup->nameServerRecords())
            lines << tr("%1  TTL %2  %3").arg(record.name()).arg(record.timeToLive()).arg(record.value());
        for(const auto &record : lookup->textRecords())
        {
            QStringList values; for(const QByteArray &value : record.values()) values << QString::fromUtf8(value);
            lines << tr("%1  TTL %2  %3").arg(record.name()).arg(record.timeToLive()).arg(values.join(" "));
        }
        for(const auto &record : lookup->pointerRecords())
            lines << tr("%1  TTL %2  %3").arg(record.name()).arg(record.timeToLive()).arg(record.value());
        if(lines.size() == 1) lines << tr("无记录");
        m_dnsOutput->setPlainText(lines.join('\n'));
        lookup->deleteLater(); if(m_dnsLookup == lookup) m_dnsLookup = nullptr;
    });
    lookup->lookup();
}

QString NetworkLabDialog::subnetSummary(const QString &cidr, QString *range)
{
    if(range) range->clear();
    const QStringList parts = cidr.trimmed().split('/');
    QHostAddress address;
    bool prefixOk = false;
    const int prefix = parts.value(1).toInt(&prefixOk);
    if(parts.size() != 2 || !address.setAddress(parts[0]) || !prefixOk)
        return QObject::tr("请输入有效 IPv4/IPv6 CIDR。");
    if(address.protocol() == QAbstractSocket::IPv4Protocol)
    {
        if(prefix < 0 || prefix > 32) return QObject::tr("IPv4 前缀须为 0–32。");
        const quint32 mask = prefix == 0 ? 0 : 0xffffffffu << (32 - prefix);
        const quint32 network = address.toIPv4Address() & mask;
        const quint32 broadcast = network | ~mask;
        const quint64 count = static_cast<quint64>(broadcast) - network + 1;
        const quint32 first = prefix < 31 ? network + 1 : network;
        const quint32 last = prefix < 31 ? broadcast - 1 : broadcast;
        if(range && count <= 2048) *range = QHostAddress(network).toString() + '/' + QString::number(prefix);
        return QObject::tr("网络 %1/%2\n掩码 %3\n广播 %4\n可用范围 %5–%6\n可用地址 %7；总地址 %8%9")
            .arg(QHostAddress(network).toString()).arg(prefix).arg(QHostAddress(mask).toString())
            .arg(QHostAddress(broadcast).toString()).arg(QHostAddress(first).toString())
            .arg(QHostAddress(last).toString()).arg(prefix < 31 ? count - 2 : count).arg(count)
            .arg(count > 2048 ? QObject::tr("；超过扫描上限 2048") : QString());
    }
    if(address.protocol() != QAbstractSocket::IPv6Protocol || prefix < 0 || prefix > 128)
        return QObject::tr("IPv6 前缀须为 0–128。");
    Q_IPV6ADDR network = address.toIPv6Address();
    Q_IPV6ADDR last = network;
    for(int i = 0; i < 16; ++i)
    {
        const int bits = qBound(0, prefix - i * 8, 8);
        const quint8 mask = bits == 0 ? 0 : static_cast<quint8>(0xff << (8 - bits));
        network[i] &= mask;
        last[i] = network[i] | static_cast<quint8>(~mask);
    }
    const QString count = prefix == 128 ? "1" : QString("2^%1").arg(128 - prefix);
    return QObject::tr("网络 %1/%2\n范围 %3–%4\n地址数量 %5\nIPv6 无广播地址；扫描器目前只接受 IPv4 范围。")
        .arg(QHostAddress(network).toString()).arg(prefix).arg(QHostAddress(network).toString())
        .arg(QHostAddress(last).toString()).arg(count);
}

void NetworkLabDialog::runMtu()
{
    if(!m_mtuActive) return;
    if(m_mtuLow >= m_mtuHigh)
    {
        m_mtuOutput->append(tr("最大确认载荷 %1 字节；估计 IPv4 MTU %2 字节（不含额外封装）。")
                            .arg(m_mtuLow).arg(m_mtuLow + 28));
        m_mtuActive = false; m_mtuButton->setText(tr("探测 MTU")); return;
    }
    const int size = (m_mtuLow + m_mtuHigh + 1) / 2;
    runCommand("ping", {"-f", "-l", QString::number(size), "-n", "1", "-w", "1000",
                        m_mtuTarget->text().trimmed()},
        [this, size](const QString &text, int code) {
            if(!m_mtuActive) return;
            if(code == 0 && text.contains(QRegularExpression("TTL=", QRegularExpression::CaseInsensitiveOption)))
            { m_mtuLow = size; m_mtuOutput->append(tr("载荷 %1：成功").arg(size)); }
            else if(text.contains(QRegularExpression("fragment|分段|拆分|需要分片", QRegularExpression::CaseInsensitiveOption)))
            { m_mtuHigh = size - 1; m_mtuOutput->append(tr("载荷 %1：需要分片").arg(size)); }
            else
            {
                m_mtuOutput->append(tr("载荷 %1：超时/ICMP 受限；MTU 不确定，已停止。\n%2")
                                    .arg(size).arg(text.trimmed()));
                m_mtuActive = false; m_mtuButton->setText(tr("探测 MTU")); return;
            }
            QTimer::singleShot(0, this, &NetworkLabDialog::runMtu);
        });
}

void NetworkLabDialog::nextHealth()
{
    if(m_healthIndex >= m_healthSteps.size())
    { m_healthOutput->append(tr("检查结束。未提供的网关、DNS 或目标已跳过。")); return; }
    const auto step = m_healthSteps.at(m_healthIndex++);
    if(step.first == "DNS")
    {
        const QHostAddress server(step.second);
        QStringList octets = sourceIp().split('.');
        if(server.isNull() || octets.size() != 4)
        { m_healthOutput->append(tr("DNS：缺少有效的服务器或本地 IPv4 地址。")); nextHealth(); return; }
        std::reverse(octets.begin(), octets.end());
        auto *lookup = new QDnsLookup(QDnsLookup::PTR, octets.join('.') + ".in-addr.arpa", this);
        lookup->setNameserver(server);
        auto *clock = new QElapsedTimer; clock->start();
        connect(lookup, &QDnsLookup::finished, this, [this, lookup, clock, step]() {
            const bool answered = lookup->error() == QDnsLookup::NoError ||
                                  lookup->error() == QDnsLookup::NotFoundError;
            m_healthOutput->append(tr("DNS %1：%2，%3 ms；证据：%4")
                .arg(step.second, answered ? tr("有响应") : tr("未确认有响应"))
                .arg(clock->elapsed()).arg(lookup->errorString()));
            delete clock; lookup->deleteLater(); nextHealth();
        });
        QTimer::singleShot(2500, lookup, [lookup]() { if(!lookup->isFinished()) lookup->abort(); });
        lookup->lookup();
        return;
    }
    QStringList args{"-n", "1", "-w", "1000"};
    const QString source = sourceIp();
    if(!source.isEmpty()) args << "-S" << source;
    args << step.second;
    auto *clock = new QElapsedTimer; clock->start();
    runCommand("ping", args, [this, step, clock](const QString &text, int code) {
        m_healthOutput->append(tr("%1：%2，%3 ms；证据：%4")
            .arg(step.first, code == 0 ? tr("可达") : tr("未确认可达"))
            .arg(clock->elapsed()).arg(text.trimmed().split(QRegularExpression("[\\r\\n]+"), QString::SkipEmptyParts).last()));
        delete clock;
        QTimer::singleShot(0, this, &NetworkLabDialog::nextHealth);
    });
}

void NetworkLabDialog::startLan()
{
    const QHostAddress target(m_lanHost->text().trimmed());
    if(target.protocol() != QAbstractSocket::IPv4Protocol ||
       !(target.isInSubnet(QHostAddress("10.0.0.0"), 8) ||
         target.isInSubnet(QHostAddress("172.16.0.0"), 12) ||
         target.isInSubnet(QHostAddress("192.168.0.0"), 16) ||
         target.isInSubnet(QHostAddress("169.254.0.0"), 16) || target == QHostAddress::LocalHost))
    { m_lanOutput->setPlainText(tr("仅允许局域网或本机 IPv4 目标。")); return; }
    stopLan();
    if(!m_udp->bind(QHostAddress(QHostAddress::AnyIPv4), static_cast<quint16>(0)))
    { m_lanOutput->setPlainText(m_udp->errorString()); return; }
    m_lanSequence = 0; m_lanBytes = 0; m_lanSending = true; m_lanClock.start();
    m_lanOutput->setPlainText(tr("向 %1:%2 发送 UDP；最长 %3 秒，最多 50 MiB。")
                              .arg(target.toString()).arg(m_lanPort->value()).arg(m_lanSeconds->value()));
    m_lanTimer->start(std::max(2, 1000 / m_lanRate->value()));
}

void NetworkLabDialog::stopLan()
{
    m_lanTimer->stop();
    if(m_lanSending)
    {
        m_lanSending = false;
        const QHostAddress target(m_lanHost->text().trimmed());
        m_udp->writeDatagram("NCM_END", target, static_cast<quint16>(m_lanPort->value()));
        const double seconds = std::max(0.001, m_lanClock.elapsed() / 1000.0);
        m_lanOutput->append(tr("已发送 %1 包 / %2 字节；发送速率 %3 Mbit/s。等待接收端统计...")
                            .arg(m_lanSequence).arg(m_lanBytes).arg(m_lanBytes * 8.0 / seconds / 1e6, 0, 'f', 2));
        QTimer::singleShot(2000, this, [this]() { if(!m_lanSending) m_udp->close(); });
    }
    else m_udp->close();
}

void NetworkLabDialog::readLan()
{
    while(m_udp->hasPendingDatagrams())
    {
        QByteArray data(static_cast<int>(m_udp->pendingDatagramSize()), 0);
        QHostAddress peer; quint16 port = 0;
        m_udp->readDatagram(data.data(), data.size(), &peer, &port);
        if(data.startsWith("NCM_RESULT "))
        { m_lanOutput->append(QString::fromUtf8(data.mid(11))); continue; }
        if(data == "NCM_END")
        {
            const double seconds = std::max(0.001, m_lanClock.elapsed() / 1000.0);
            const QString summary = tr("接收 %1 包 / %2 字节；缺号 %3；有效吞吐 %4 Mbit/s。UDP 缺号包括网络丢包和乱序。")
                .arg(m_lanReceived).arg(m_lanBytes).arg(m_lanLost)
                .arg(m_lanBytes * 8.0 / seconds / 1e6, 0, 'f', 2);
            m_lanOutput->append(summary);
            m_udp->writeDatagram("NCM_RESULT " + summary.toUtf8(), peer, port);
            continue;
        }
        if(data.size() != 1200) continue;
        const quint32 sequence = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(data.constData()));
        if(m_lanReceived && sequence > m_lanSequence + 1) m_lanLost += sequence - m_lanSequence - 1;
        m_lanSequence = std::max(m_lanSequence, sequence);
        ++m_lanReceived; m_lanBytes += data.size();
        if(m_lanReceived % 100 == 0)
            m_lanOutput->setPlainText(tr("监听接收：%1 包，%2 字节，当前缺号 %3")
                                      .arg(m_lanReceived).arg(m_lanBytes).arg(m_lanLost));
    }
}

void NetworkLabDialog::runHttp()
{
    if(m_httpSocket) { m_httpSocket->abort(); m_httpSocket->deleteLater(); m_httpSocket = nullptr; }
    const QUrl url = QUrl::fromUserInput(m_httpUrl->text().trimmed());
    if(!url.isValid() || (url.scheme() != "http" && url.scheme() != "https") || url.host().isEmpty())
    { m_httpOutput->setPlainText(tr("请输入有效 HTTP(S) URL。")); return; }
    m_httpSecure = url.scheme() == "https";
    m_httpHost = url.host();
    m_httpPath = url.path(QUrl::FullyEncoded);
    if(m_httpPath.isEmpty()) m_httpPath = "/";
    if(url.hasQuery()) m_httpPath += '?' + url.query(QUrl::FullyEncoded);
    m_httpBuffer.clear(); m_httpDnsMs = m_httpConnectMs = m_httpTlsMs = 0;
    m_httpOutput->setPlainText(tr("解析 %1...").arg(m_httpHost));
    m_httpClock.start();
    QHostInfo::lookupHost(m_httpHost, this, [this, url](const QHostInfo &info) {
        if(info.error() != QHostInfo::NoError || info.addresses().isEmpty())
        { finishHttp(tr("DNS 失败：%1").arg(info.errorString())); return; }
        m_httpDnsMs = m_httpClock.elapsed();
        m_httpOutput->append(tr("DNS %1 ms：%2").arg(m_httpDnsMs)
                             .arg(info.addresses().first().toString()));
        m_httpSocket = m_httpSecure ? static_cast<QAbstractSocket *>(new QSslSocket(this))
                                   : static_cast<QAbstractSocket *>(new QTcpSocket(this));
        QAbstractSocket *socket = m_httpSocket;
        const QByteArray request = "HEAD " + m_httpPath.toUtf8() + " HTTP/1.1\r\nHost: " +
            m_httpHost.toUtf8() + "\r\nUser-Agent: NetworkConfigManager/2.5.5\r\nConnection: close\r\n\r\n";
        connect(socket, &QAbstractSocket::connected, this, [this, socket, request]() {
            if(socket != m_httpSocket) return;
            m_httpConnectMs = m_httpClock.elapsed() - m_httpDnsMs;
            if(!m_httpSecure) socket->write(request);
        });
        if(m_httpSecure)
        {
            auto *ssl = static_cast<QSslSocket *>(socket);
            connect(ssl, &QSslSocket::encrypted, this, [this, socket, request]() {
                if(socket != m_httpSocket) return;
                m_httpTlsMs = m_httpClock.elapsed() - m_httpDnsMs - m_httpConnectMs;
                socket->write(request);
            });
            connect(ssl, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), this,
                    [this, socket](const QList<QSslError> &errors) {
                if(socket == m_httpSocket && !errors.isEmpty()) finishHttp(tr("TLS：%1").arg(errors.first().errorString()));
            });
            ssl->connectToHostEncrypted(m_httpHost, static_cast<quint16>(url.port(443)));
        }
        else socket->connectToHost(m_httpHost, static_cast<quint16>(url.port(80)));
        connect(socket, &QIODevice::readyRead, this, [this, socket]() {
            if(socket != m_httpSocket) return;
            m_httpBuffer += QString::fromLatin1(socket->read(32768));
            if(m_httpBuffer.size() > 32768) { finishHttp(tr("响应头超过 32 KiB。")); return; }
            if(m_httpBuffer.contains("\r\n\r\n")) finishHttp();
        });
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this,
                [this, socket](QAbstractSocket::SocketError) {
            if(socket == m_httpSocket) finishHttp(socket->errorString());
        });
        QTimer::singleShot(10000, this, [this, socket]() {
            if(socket == m_httpSocket) finishHttp(tr("10 秒超时。"));
        });
    });
}

void NetworkLabDialog::finishHttp(const QString &error)
{
    QStringList lines = m_httpOutput->toPlainText().split('\n');
    if(!error.isEmpty()) lines << tr("错误：%1").arg(error);
    else
    {
        const QStringList headers = m_httpBuffer.section("\r\n\r\n", 0, 0).split("\r\n");
        lines << tr("连接 %1 ms；TLS %2 ms；首字节 %3 ms")
                 .arg(m_httpConnectMs).arg(m_httpTlsMs).arg(m_httpClock.elapsed());
        if(!headers.isEmpty()) lines << tr("状态：%1").arg(headers.first());
        for(const QString &header : headers)
            if(header.startsWith("Location:", Qt::CaseInsensitive)) lines << tr("重定向：%1").arg(header.mid(9).trimmed());
        if(m_httpSecure && m_httpSocket)
        {
            const QSslCertificate cert = static_cast<QSslSocket *>(m_httpSocket)->peerCertificate();
            if(!cert.isNull())
                lines << tr("证书：%1；签发者 %2；有效期至 %3；SHA256 %4")
                    .arg(cert.subjectInfo(QSslCertificate::CommonName).join(", "),
                         cert.issuerInfo(QSslCertificate::CommonName).join(", "),
                         cert.expiryDate().toString(Qt::ISODate),
                         QString::fromLatin1(cert.digest(QCryptographicHash::Sha256).toHex()));
        }
    }
    m_httpOutput->setPlainText(lines.join('\n'));
    QAbstractSocket *socket = m_httpSocket;
    m_httpSocket = nullptr;
    if(socket) { socket->abort(); socket->deleteLater(); }
}

void NetworkLabDialog::captureReport()
{
    if(m_adapter->currentText().isEmpty()) return;
    const QString adapterId = NetworkInterfaceManager::captureConfig(m_adapter->currentText())
                                  .value("adapter_id").toString();
    if(adapterId.isEmpty())
    { m_reportPreview->setPlainText(tr("无法定位网卡 GUID。")); return; }
    auto *process = new QProcess(this);
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert("NCM_ADAPTER_ID", adapterId);
    process->setProcessEnvironment(environment);
    const QString script = QStringLiteral("[Console]::OutputEncoding = [Text.Encoding]::UTF8\n") +
                           NetworkAdapterDialog::snapshotScript();
    const QByteArray bytes(reinterpret_cast<const char *>(script.utf16()), script.size() * 2);
    m_reportPreview->setPlainText(tr("正在采集网卡、路由和 DNS 快照..."));
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, process](int code, QProcess::ExitStatus) {
        const QByteArray raw = process->readAllStandardOutput();
        const QJsonDocument doc = QJsonDocument::fromJson(raw);
        if(code != 0 || !doc.isObject())
        {
            m_reportPreview->setPlainText(tr("快照失败：%1")
                .arg(QString::fromUtf8(process->readAllStandardError()).trimmed()));
            process->deleteLater(); return;
        }
        QJsonObject report{
            {"time", QDateTime::currentDateTime().toString(Qt::ISODate)},
            {"adapter", m_adapter->currentText()},
            {"snapshot", doc.object()},
            {"ping", m_pingOutput->toPlainText()}, {"trace", m_traceOutput->toPlainText()},
            {"dns", m_dnsOutput->toPlainText()}, {"mtu", m_mtuOutput->toPlainText()},
            {"health", m_healthOutput->toPlainText()}, {"lan", m_lanOutput->toPlainText()},
            {"http", m_httpOutput->toPlainText()}
        };
        QFile scan(QCoreApplication::applicationDirPath() + "/config/last_scan.json");
        if(scan.open(QIODevice::ReadOnly)) report.insert("scan", QJsonDocument::fromJson(scan.readAll()).array());
        m_report = report;
        QFile existing(historyPath());
        if(existing.open(QIODevice::ReadOnly) && existing.size() > 0)
        {
            QJsonParseError parseError;
            const QJsonDocument previous = QJsonDocument::fromJson(existing.readAll(), &parseError);
            if(parseError.error != QJsonParseError::NoError || !previous.isArray())
            { m_reportPreview->setPlainText(tr("报告历史格式错误，未覆盖原文件。")); process->deleteLater(); return; }
        }
        existing.close();
        QJsonArray saved = history(); saved.append(report);
        while(saved.size() > 50) saved.removeAt(0);
        QSaveFile file(historyPath());
        const QByteArray data = QJsonDocument(saved).toJson();
        if(!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
        { m_reportPreview->setPlainText(tr("历史保存失败：%1").arg(file.errorString())); process->deleteLater(); return; }
        m_compareA->clear(); m_compareB->clear();
        for(const QJsonValue &item : saved)
        {
            const QString label = item.toObject().value("time").toString() + " " +
                                  item.toObject().value("adapter").toString();
            m_compareA->addItem(label); m_compareB->addItem(label);
        }
        m_compareB->setCurrentIndex(m_compareB->count() - 1);
        showReport(report);
        process->deleteLater();
    });
    QTimer::singleShot(30000, process, [process]() {
        if(process->state() != QProcess::NotRunning) process->kill();
    });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if(error == QProcess::FailedToStart)
        { m_reportPreview->setPlainText(tr("快照失败：%1").arg(process->errorString())); process->deleteLater(); }
    });
    process->start("powershell.exe", {"-NoProfile", "-NonInteractive", "-EncodedCommand",
                   QString::fromLatin1(bytes.toBase64())});
}

void NetworkLabDialog::showReport(const QJsonObject &report)
{
    const QJsonObject view = m_redact->isChecked() ? redactedReport(report) : report;
    m_reportPreview->setPlainText(QString::fromUtf8(QJsonDocument(view).toJson(QJsonDocument::Indented)));
}

void NetworkLabDialog::saveReport()
{
    if(m_report.isEmpty()) return;
    const QString path = QFileDialog::getSaveFileName(this, tr("导出本地报告"), "network-report.json",
                                                    "JSON (*.json);;Text (*.txt);;CSV (*.csv)");
    if(path.isEmpty()) return;
    const QJsonObject report = m_redact->isChecked() ? redactedReport(m_report) : m_report;
    QByteArray data;
    if(path.endsWith(".csv", Qt::CaseInsensitive))
    {
        data = "section,value\r\n";
        for(auto it = report.begin(); it != report.end(); ++it)
        {
            QString value = it.value().isString() ? it.value().toString()
                : QString::fromUtf8(QJsonDocument(QJsonArray{it.value()}).toJson(QJsonDocument::Compact));
            value.replace('"', "\"\"");
            data += '"' + it.key().toUtf8() + "\",\"" + value.toUtf8() + "\"\r\n";
        }
    }
    else if(path.endsWith(".txt", Qt::CaseInsensitive))
    {
        QStringList sections{tr("时间：%1").arg(report.value("time").toString()),
                             tr("网卡：%1").arg(report.value("adapter").toString())};
        for(auto it = report.begin(); it != report.end(); ++it)
        {
            if(it.key() == "time" || it.key() == "adapter") continue;
            const QString value = it.value().isString() ? it.value().toString()
                : QString::fromUtf8(QJsonDocument(QJsonArray{it.value()}).toJson(QJsonDocument::Indented));
            sections << "\n[" + it.key() + "]\n" + value;
        }
        data = sections.join('\n').toUtf8();
    }
    else data = QJsonDocument(report).toJson(QJsonDocument::Indented);
    QSaveFile file(path);
    if(!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
        QMessageBox::warning(this, tr("导出报告"), tr("无法保存报告。"));
}

void NetworkLabDialog::compareReports()
{
    const QJsonArray saved = history();
    const int first = m_compareA->currentIndex(), second = m_compareB->currentIndex();
    if(first < 0 || second < 0 || first >= saved.size() || second >= saved.size()) return;
    const QJsonObject left = saved[first].toObject(), right = saved[second].toObject();
    const QJsonObject before = left.value("snapshot").toObject();
    const QJsonObject after = right.value("snapshot").toObject();
    QStringList changes{tr("比较 %1 → %2").arg(left.value("time").toString(), right.value("time").toString())};
    for(auto it = after.begin(); it != after.end(); ++it)
        if(before.value(it.key()) != it.value())
        {
            const QJsonValue oldItem = m_redact->isChecked() ? scrub(before.value(it.key()), it.key()) : before.value(it.key());
            const QJsonValue newItem = m_redact->isChecked() ? scrub(it.value(), it.key()) : it.value();
            QString oldValue = QString::fromUtf8(QJsonDocument(QJsonArray{oldItem}).toJson(QJsonDocument::Compact));
            QString newValue = QString::fromUtf8(QJsonDocument(QJsonArray{newItem}).toJson(QJsonDocument::Compact));
            changes << it.key() + ": " + oldValue + " → " + newValue;
        }
    if(changes.size() == 1) changes << tr("网卡快照无变化。" );
    m_reportPreview->setPlainText(changes.join('\n'));
}

#include "networkadapterdialog.h"
#include "configmanager.h"
#include "networkinterfacemanager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSharedPointer>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace {
QString adapterLookup()
{
    return QStringLiteral(R"PS(
$ErrorActionPreference = 'Stop'
$id = $env:NCM_ADAPTER_ID.Trim('{}')
$a = Get-NetAdapter -IncludeHidden | Where-Object { $_.InterfaceGuid.ToString().Trim('{}') -ieq $id } | Select-Object -First 1
if (-not $a) { throw '网卡已离线或系统不支持 NetTCPIP 管理命令' }
$idx = [int]$a.ifIndex
)PS");
}

QString snapshotScript()
{
    return adapterLookup() + QStringLiteral(R"PS(
$v4 = @(Get-NetIPAddress -InterfaceIndex $idx -AddressFamily IPv4 -ErrorAction SilentlyContinue | ForEach-Object {
    @{ Address=$_.IPAddress; Prefix=[int]$_.PrefixLength; Origin=[string]$_.PrefixOrigin; Skip=[bool]$_.SkipAsSource }
})
$v6 = @(Get-NetIPAddress -InterfaceIndex $idx -AddressFamily IPv6 -ErrorAction SilentlyContinue | ForEach-Object {
    @{ Address=$_.IPAddress; Prefix=[int]$_.PrefixLength; Origin=[string]$_.PrefixOrigin; Skip=[bool]$_.SkipAsSource }
})
$routes = @(foreach ($store in @('ActiveStore', 'PersistentStore')) {
    Get-NetRoute -InterfaceIndex $idx -PolicyStore $store -ErrorAction SilentlyContinue | ForEach-Object {
        @{ Prefix=$_.DestinationPrefix; Gateway=$_.NextHop; Metric=[int]$_.RouteMetric;
           Protocol=[string]$_.Protocol; Store=$store; Family=[string]$_.AddressFamily }
    }
})
$if4 = Get-NetIPInterface -InterfaceIndex $idx -AddressFamily IPv4 -ErrorAction SilentlyContinue
$if6 = Get-NetIPInterface -InterfaceIndex $idx -AddressFamily IPv6 -ErrorAction SilentlyContinue
$dns4 = @(Get-DnsClientServerAddress -InterfaceIndex $idx -AddressFamily IPv4 -ErrorAction SilentlyContinue |
          ForEach-Object { $_.ServerAddresses })
$dns6 = @(Get-DnsClientServerAddress -InterfaceIndex $idx -AddressFamily IPv6 -ErrorAction SilentlyContinue |
          ForEach-Object { $_.ServerAddresses })
$guid = '{' + $a.InterfaceGuid.ToString().Trim('{}') + '}'
$dns4Registry = Get-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces\$guid" -ErrorAction SilentlyContinue
$dns6Registry = Get-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip6\Parameters\Interfaces\$guid" -ErrorAction SilentlyContinue
$stats = Get-NetAdapterStatistics -Name $a.Name -ErrorAction SilentlyContinue
@{
    Name=$a.Name; Id=$a.InterfaceGuid.ToString(); Mac=$a.MacAddress; Speed=$a.LinkSpeed;
    Status=[string]$a.Status; Description=$a.InterfaceDescription;
    Driver=$a.DriverVersion; Updated=(Get-Date).ToString('o'); Source='Windows NetAdapter/NetTCPIP/DnsClient';
    IPv4=$v4; IPv6=$v6; Routes=$routes;
    Interface4=@{ Dhcp=[string]$if4.Dhcp; Mtu=[int]$if4.NlMtu;
                  Metric=[int]$if4.InterfaceMetric; AutoMetric=[string]$if4.AutomaticMetric };
    Interface6=@{ Dhcp=[string]$if6.Dhcp; RouterDiscovery=[string]$if6.RouterDiscovery;
                  Mtu=[int]$if6.NlMtu; Metric=[int]$if6.InterfaceMetric;
                  AutoMetric=[string]$if6.AutomaticMetric };
    Dns4=$dns4; Dns6=$dns6;
    DnsMode4=$(if ($dns4Registry.NameServer) { '手动' } else { '自动' });
    DnsMode6=$(if ($dns6Registry.NameServer) { '手动' } else { '自动' });
    Sent=[long]$stats.SentBytes; Received=[long]$stats.ReceivedBytes
} | ConvertTo-Json -Depth 6 -Compress
)PS");
}

QJsonArray array(const QJsonValue &value)
{
    if(value.isArray()) return value.toArray();
    return value.isObject() ? QJsonArray{value} : QJsonArray{};
}

bool validIp(const QString &value, QAbstractSocket::NetworkLayerProtocol family)
{
    QHostAddress address;
    return address.setAddress(value) && address.protocol() == family;
}

bool validPrefix(const QString &text)
{
    const QStringList parts = text.split('/');
    if(parts.size() != 2) return false;
    QHostAddress address;
    if(!address.setAddress(parts[0])) return false;
    bool ok = false;
    const int prefix = parts[1].toInt(&ok);
    return ok && prefix >= 0 && prefix <= (address.protocol() == QAbstractSocket::IPv4Protocol ? 32 : 128);
}
}

QString NetworkAdapterDialog::snapshotScript()
{
    return ::snapshotScript();
}

NetworkAdapterDialog::NetworkAdapterDialog(ConfigManager *manager, QWidget *parent)
    : QDialog(parent), m_manager(manager), m_adapter(new QComboBox(this)),
      m_details(new QPlainTextEdit(this)), m_dns(new QPlainTextEdit(this)),
      m_wifi(new QPlainTextEdit(this)), m_addresses(new QTableWidget(this)),
      m_routes(new QTableWidget(this)), m_cancel(new QPushButton(tr("取消当前操作"), this)),
      m_timeout(new QTimer(this))
{
    setWindowTitle(tr("网卡、地址与路由"));
    resize(900, 650);
    m_backupFile = QCoreApplication::applicationDirPath() + "/config/network_tool_backups.json";
    auto *layout = new QVBoxLayout(this);
    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel(tr("网卡"), this));
    top->addWidget(m_adapter, 1);
    auto *reload = new QPushButton(tr("刷新"), this);
    auto *undo = new QPushButton(tr("恢复最近修改前值"), this);
    top->addWidget(reload);
    top->addWidget(undo);
    top->addWidget(m_cancel);
    layout->addLayout(top);
    auto *tabs = new QTabWidget(this);
    m_details->setReadOnly(true);
    tabs->addTab(m_details, tr("网卡详情"));

    auto *addressPage = new QWidget(this);
    auto *addressLayout = new QVBoxLayout(addressPage);
    m_addresses->setColumnCount(5);
    m_addresses->setHorizontalHeaderLabels({tr("地址族"), tr("地址"), tr("前缀"), tr("来源"), tr("附加地址")});
    m_addresses->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_addresses->setEditTriggers(QAbstractItemView::NoEditTriggers);
    addressLayout->addWidget(m_addresses);
    auto *addressButtons = new QHBoxLayout;
    auto *add4 = new QPushButton(tr("添加附加 IPv4"), this);
    auto *add6 = new QPushButton(tr("添加静态 IPv6"), this);
    auto *removeAddressButton = new QPushButton(tr("删除选中手动地址"), this);
    addressButtons->addWidget(add4);
    addressButtons->addWidget(add6);
    addressButtons->addWidget(removeAddressButton);
    addressLayout->addLayout(addressButtons);
    tabs->addTab(addressPage, tr("IPv4 / IPv6 地址"));

    auto *routePage = new QWidget(this);
    auto *routeLayout = new QVBoxLayout(routePage);
    auto *filter = new QLineEdit(this);
    filter->setPlaceholderText(tr("按 IPv4/IPv6、目标、网关或来源过滤"));
    routeLayout->addWidget(filter);
    m_routes->setColumnCount(6);
    m_routes->setHorizontalHeaderLabels({tr("地址族"), tr("目标网段"), tr("下一跳"), tr("跃点"), tr("来源"), tr("存储")});
    m_routes->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_routes->setEditTriggers(QAbstractItemView::NoEditTriggers);
    routeLayout->addWidget(m_routes);
    auto *routeButtons = new QHBoxLayout;
    auto *addRouteButton = new QPushButton(tr("新增静态路由"), this);
    auto *deleteRouteButton = new QPushButton(tr("删除选中静态路由"), this);
    routeButtons->addWidget(addRouteButton);
    routeButtons->addWidget(deleteRouteButton);
    routeLayout->addLayout(routeButtons);
    tabs->addTab(routePage, tr("路由表"));

    auto *dnsPage = new QWidget(this);
    auto *dnsLayout = new QVBoxLayout(dnsPage);
    m_dns->setReadOnly(true);
    dnsLayout->addWidget(m_dns);
    auto *showCache = new QPushButton(tr("查看本机 DNS 缓存"), this);
    auto *clearCache = new QPushButton(tr("清空本机 DNS 缓存"), this);
    auto *dnsButtons = new QHBoxLayout;
    dnsButtons->addWidget(showCache);
    dnsButtons->addWidget(clearCache);
    dnsLayout->addLayout(dnsButtons);
    tabs->addTab(dnsPage, tr("DNS"));

    auto *mtuPage = new QWidget(this);
    auto *mtuLayout = new QVBoxLayout(mtuPage);
    mtuLayout->addWidget(new QLabel(tr("当前 IPv4/IPv6 MTU 与接口跃点数见“网卡详情”。IPv4 最小 576，IPv6 最小 1280；最大值受设备限制。"), this));
    auto *changeMtuButton = new QPushButton(tr("修改 MTU / 接口跃点数"), this);
    auto *restoreMtuButton = new QPushButton(tr("恢复保存的修改前值"), this);
    mtuLayout->addWidget(changeMtuButton);
    mtuLayout->addWidget(restoreMtuButton);
    mtuLayout->addStretch();
    tabs->addTab(mtuPage, tr("MTU 与跃点"));

    auto *wifiPage = new QWidget(this);
    auto *wifiLayout = new QVBoxLayout(wifiPage);
    m_wifi->setReadOnly(true);
    wifiLayout->addWidget(m_wifi);
    auto *scanWifi = new QPushButton(tr("刷新无线状态与附近接入点"), this);
    wifiLayout->addWidget(scanWifi);
    tabs->addTab(wifiPage, tr("Wi-Fi 环境"));
    layout->addWidget(tabs);
    m_cancel->setEnabled(false);
    m_timeout->setSingleShot(true);

    connect(reload, &QPushButton::clicked, this, &NetworkAdapterDialog::refresh);
    connect(undo, &QPushButton::clicked, this, &NetworkAdapterDialog::undoLast);
    connect(restoreMtuButton, &QPushButton::clicked, this, &NetworkAdapterDialog::undoLast);
    connect(m_cancel, &QPushButton::clicked, this, [this]() { if(m_process) m_process->kill(); });
    connect(m_adapter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { if(!m_process) refresh(); });
    connect(add4, &QPushButton::clicked, this, [this]() { addAddress(false); });
    connect(add6, &QPushButton::clicked, this, [this]() { addAddress(true); });
    connect(removeAddressButton, &QPushButton::clicked, this, [this]() {
        const int row = m_addresses->currentRow();
        if(row >= 0) removeAddress(m_addresses->item(row, 0)->text() == "IPv6");
    });
    connect(addRouteButton, &QPushButton::clicked, this, &NetworkAdapterDialog::addRoute);
    connect(deleteRouteButton, &QPushButton::clicked, this, &NetworkAdapterDialog::removeRoute);
    connect(changeMtuButton, &QPushButton::clicked, this, &NetworkAdapterDialog::changeMtu);
    connect(clearCache, &QPushButton::clicked, this, &NetworkAdapterDialog::flushDns);
    connect(showCache, &QPushButton::clicked, this, [this]() {
        runScript(QStringLiteral("ipconfig /displaydns"), {}, [this](bool ok, const QString &output) {
            m_dns->setPlainText(ok ? output : tr("读取 DNS 缓存失败：%1").arg(output));
        });
    });
    connect(scanWifi, &QPushButton::clicked, this, &NetworkAdapterDialog::refreshWifi);
    connect(filter, &QLineEdit::textChanged, this, [this](const QString &text) {
        for(int row = 0; row < m_routes->rowCount(); ++row)
        {
            bool match = text.isEmpty();
            for(int column = 0; column < m_routes->columnCount() && !match; ++column)
                match = m_routes->item(row, column)->text().contains(text, Qt::CaseInsensitive);
            m_routes->setRowHidden(row, !match);
        }
    });
    const auto adapters = NetworkInterfaceManager::adapterNamesById();
    for(auto it = adapters.cbegin(); it != adapters.cend(); ++it)
        m_adapter->addItem(it.value(), it.key());
    if(m_adapter->count()) refresh();
    else m_details->setPlainText(tr("未找到网卡。"));
}

NetworkAdapterDialog::~NetworkAdapterDialog()
{
    if(m_process) { m_process->kill(); m_process->waitForFinished(1000); }
}

void NetworkAdapterDialog::runScript(const QString &script, const QMap<QString, QString> &values, Completion done)
{
    if(m_process)
    {
        QMessageBox::information(this, tr("操作进行中"), tr("请等待当前操作完成或取消。"));
        return;
    }
    auto *process = new QProcess(this);
    m_process = process;
    m_cancel->setEnabled(true);
    m_adapter->setEnabled(false);
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert("NCM_ADAPTER_ID", m_adapter->currentData().toString());
    for(auto it = values.cbegin(); it != values.cend(); ++it) environment.insert(it.key(), it.value());
    process->setProcessEnvironment(environment);
    const QString command = QStringLiteral("[Console]::OutputEncoding = [Text.Encoding]::UTF8\n") + script;
    const QByteArray bytes(reinterpret_cast<const char *>(command.utf16()), command.size() * 2);
    const QString encoded = QString::fromLatin1(bytes.toBase64());
    auto finished = QSharedPointer<bool>::create(false);
    auto complete = [this, process, done, finished](bool ok) {
        if(*finished) return;
        *finished = true;
        m_timeout->stop();
        QObject::disconnect(m_timeout, nullptr, process, nullptr);
        const QString out = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
        const QString err = QString::fromUtf8(process->readAllStandardError()).trimmed();
        m_process = nullptr;
        m_cancel->setEnabled(false);
        m_adapter->setEnabled(true);
        process->deleteLater();
        done(ok, ok ? out : (err.isEmpty() ? out : err));
    };
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [process, complete](int code, QProcess::ExitStatus status) {
        complete(status == QProcess::NormalExit && code == 0);
    });
    connect(process, &QProcess::errorOccurred, this, [process, complete](QProcess::ProcessError error) {
        if(error == QProcess::FailedToStart) complete(false);
    });
    connect(m_timeout, &QTimer::timeout, process, [process]() { process->kill(); });
    m_timeout->start(30000);
    process->start("powershell.exe", {"-NoProfile", "-NonInteractive", "-EncodedCommand", encoded});
}

void NetworkAdapterDialog::refresh()
{
    if(m_adapter->currentIndex() < 0 || m_process) return;
    runScript(snapshotScript(), {}, [this](bool ok, const QString &output) {
        const QJsonDocument document = QJsonDocument::fromJson(output.toUtf8());
        if(ok && document.isObject())
        {
            m_snapshot = document.object();
            showSnapshot(m_snapshot);
            return;
        }
        m_snapshot = {};
        const QVariantMap fallback = NetworkInterfaceManager::captureConfig(m_adapter->currentData().toString());
        m_details->setPlainText(tr("高级信息读取失败：%1\n\n基础网卡信息（Windows IP Helper）：\n%2")
            .arg(output, QString::fromUtf8(QJsonDocument::fromVariant(fallback).toJson(QJsonDocument::Indented))));
        m_addresses->setRowCount(0);
        m_routes->setRowCount(0);
        m_dns->clear();
    });
}

void NetworkAdapterDialog::showSnapshot(const QJsonObject &s)
{
    const QJsonObject i4 = s.value("Interface4").toObject(), i6 = s.value("Interface6").toObject();
    const auto text = [](const QJsonValue &value) { return value.toVariant().toString(); };
    QString summary = tr("来源：%1\n刷新：%2\n名称：%3\nGUID：%4\n状态：%5\nMAC：%6\n链路速率：%7\n驱动：%8（%9）\n发送：%10 B；接收：%11 B\n\nIPv4 DHCP：%12；MTU：%13；接口跃点：%14（自动：%15）\nIPv6 DHCPv6：%16；路由发现/SLAAC：%17；MTU：%18；接口跃点：%19（自动：%20）");
    for(const QString &field : QStringList{text(s.value("Source")), text(s.value("Updated")),
        text(s.value("Name")), text(s.value("Id")), text(s.value("Status")), text(s.value("Mac")),
        text(s.value("Speed")), text(s.value("Description")), text(s.value("Driver")),
        text(s.value("Sent")), text(s.value("Received")), text(i4.value("Dhcp")), text(i4.value("Mtu")),
        text(i4.value("Metric")), text(i4.value("AutoMetric")), text(i6.value("Dhcp")),
        text(i6.value("RouterDiscovery")), text(i6.value("Mtu")), text(i6.value("Metric")),
        text(i6.value("AutoMetric"))}) summary = summary.arg(field);
    for(const QString &family : {QStringLiteral("IPv4"), QStringLiteral("IPv6")})
    {
        QStringList addresses, gateways;
        for(const QJsonValue &value : array(s.value(family)))
        {
            const QJsonObject address = value.toObject();
            addresses << QStringLiteral("%1/%2（%3）").arg(text(address.value("Address")),
                text(address.value("Prefix")), text(address.value("Origin")));
        }
        for(const QJsonValue &value : array(s.value("Routes")))
        {
            const QJsonObject route = value.toObject();
            if(route.value("Family") == family && route.value("Prefix") ==
               (family == "IPv4" ? QStringLiteral("0.0.0.0/0") : QStringLiteral("::/0")))
                gateways << text(route.value("Gateway"));
        }
        gateways.removeDuplicates();
        summary += tr("\n%1 地址：%2\n%1 网关：%3").arg(family,
            addresses.isEmpty() ? tr("无") : addresses.join("、"),
            gateways.isEmpty() ? tr("无") : gateways.join("、"));
    }
    m_details->setPlainText(summary);
    m_addresses->setRowCount(0);
    for(const QString &family : {QStringLiteral("IPv4"), QStringLiteral("IPv6")})
        for(const QJsonValue &entry : array(s.value(family)))
        {
            const QJsonObject address = entry.toObject();
            const int row = m_addresses->rowCount();
            m_addresses->insertRow(row);
            const QStringList cells{family, text(address.value("Address")), text(address.value("Prefix")),
                                    text(address.value("Origin")), address.value("Skip").toBool() ? tr("是") : tr("否")};
            for(int column = 0; column < cells.size(); ++column)
                m_addresses->setItem(row, column, new QTableWidgetItem(cells.at(column)));
            m_addresses->item(row, 0)->setData(Qt::UserRole, address.toVariantMap());
        }
    m_routes->setRowCount(0);
    for(const QJsonValue &entry : array(s.value("Routes")))
    {
        const QJsonObject route = entry.toObject();
        const int row = m_routes->rowCount();
        m_routes->insertRow(row);
        const QStringList cells{text(route.value("Family")), text(route.value("Prefix")),
                                text(route.value("Gateway")), text(route.value("Metric")),
                                text(route.value("Protocol")), text(route.value("Store"))};
        for(int column = 0; column < cells.size(); ++column)
            m_routes->setItem(row, column, new QTableWidgetItem(cells.at(column)));
        m_routes->item(row, 0)->setData(Qt::UserRole, route.toVariantMap());
    }
    const auto dnsLines = [](const QJsonArray &servers) {
        QStringList result;
        for(const QJsonValue &server : servers) result << server.toString();
        return result.isEmpty() ? QStringLiteral("自动或未设置") : result.join(" → ");
    };
    m_dns->setPlainText(tr("来源：Windows DnsClient 与本机注册表；刷新：%1\nIPv4 DNS（%2）顺序：%3\nIPv6 DNS（%4）顺序：%5\n\n使用“查看本机 DNS 缓存”可读取当前缓存；清空缓存需单独确认。")
        .arg(text(s.value("Updated")), text(s.value("DnsMode4")), dnsLines(array(s.value("Dns4"))),
             text(s.value("DnsMode6")), dnsLines(array(s.value("Dns6")))));
}

void NetworkAdapterDialog::refreshWifi()
{
    runScript(QStringLiteral("netsh wlan show interfaces; netsh wlan show networks mode=bssid"), {},
              [this](bool ok, const QString &output) {
        if(!ok || output.isEmpty())
        {
            m_wifi->setPlainText(tr("无法读取 Wi-Fi 信息：%1\n请检查无线网卡、WLAN 服务和系统位置权限。")
                                 .arg(output));
            return;
        }
        QMap<QString, int> channels;
        const QRegularExpression regex(QStringLiteral("(?:Channel|信道)\\s*:\\s*(\\d+)"),
                                       QRegularExpression::CaseInsensitiveOption);
        for(const QString &line : output.split('\n'))
        {
            const auto match = regex.match(line);
            if(match.hasMatch()) ++channels[match.captured(1)];
        }
        QStringList counts;
        for(auto it = channels.cbegin(); it != channels.cend(); ++it)
            counts << tr("信道 %1：%2 个 BSSID").arg(it.key()).arg(it.value());
        m_wifi->setPlainText(tr("本机 netsh wlan；刷新：%1\n按信道聚合：%2\n\n%3")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate),
                 counts.isEmpty() ? tr("未发现接入点或系统未授权扫描") : counts.join("；"), output));
    });
}

bool NetworkAdapterDialog::saveUndo(const QString &title, const QJsonObject &before, const QJsonObject &undo)
{
    QJsonArray history;
    QFile existing(m_backupFile);
    if(existing.open(QIODevice::ReadOnly))
    {
        const QJsonDocument document = QJsonDocument::fromJson(existing.readAll());
        if(!document.isArray()) return false;
        history = document.array();
    }
    history.append(QJsonObject{{"time", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
                               {"title", title}, {"adapter_id", m_adapter->currentData().toString()},
                               {"before", before}, {"undo", undo}});
    while(history.size() > 30) history.removeAt(0);
    if(!QDir().mkpath(QFileInfo(m_backupFile).absolutePath())) return false;
    QSaveFile file(m_backupFile);
    if(!file.open(QIODevice::WriteOnly)) return false;
    const QByteArray data = QJsonDocument(history).toJson();
    return file.write(data) == data.size() && file.commit();
}

void NetworkAdapterDialog::mutate(const QString &title, const QString &script,
                                  const QMap<QString, QString> &values, const QJsonObject &undo)
{
    if(m_process || m_adapter->currentIndex() < 0) return;
    if(QMessageBox::warning(this, tr("修改前预览"),
          tr("网卡：%1\n操作：%2\n可能短暂断网。先保存完整地址、路由、DNS、MTU 快照，再执行并复核。继续？")
            .arg(m_adapter->currentText(), title),
          QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
    if(!m_manager->isAdmin())
    {
        if(QMessageBox::question(this, tr("管理员权限"), tr("修改网卡需要管理员权限。现在重新启动应用？"),
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
            m_manager->requestAdminPrivileges();
        return;
    }
    runScript(snapshotScript(), {}, [this, title, script, values, undo](bool ok, const QString &output) {
        const QJsonDocument document = QJsonDocument::fromJson(output.toUtf8());
        if(!ok || !document.isObject())
        {
            QMessageBox::warning(this, tr("未修改网卡"), tr("无法读取修改前快照：%1").arg(output));
            return;
        }
        QJsonObject prepared = undo;
        const QJsonObject before = document.object();
        if(prepared.value("kind") == "address_add" && !prepared.value("gateway").toString().isEmpty())
        {
            bool existed = false;
            for(const QJsonValue &route : array(before.value("Routes")))
            {
                const QJsonObject r = route.toObject();
                if(r.value("Prefix") == "::/0" && r.value("Gateway") == prepared.value("gateway")) existed = true;
            }
            prepared["gateway_existed"] = existed;
        }
        if(!saveUndo(title, before, prepared))
        {
            QMessageBox::warning(this, tr("未修改网卡"), tr("无法保存本地回滚快照。"));
            return;
        }
        runScript(adapterLookup() + script, values, [this, title, before, prepared](bool changed, const QString &result) {
            runScript(snapshotScript(), {}, [this, title, before, prepared, changed, result](bool read, const QString &output) {
                const QJsonDocument document = QJsonDocument::fromJson(output.toUtf8());
                QStringList notes;
                if(read && document.isObject())
                {
                    m_snapshot = document.object();
                    showSnapshot(m_snapshot);
                    const QString kind = prepared.value("kind").toString();
                    const QString family = prepared.value("family").toString();
                    for(const QString &name : {QStringLiteral("IPv4"), QStringLiteral("IPv6")})
                    {
                        if(kind.startsWith("address_") && family == name) continue;
                        if(QJsonDocument(array(before.value(name))).toJson(QJsonDocument::Compact) !=
                           QJsonDocument(array(m_snapshot.value(name))).toJson(QJsonDocument::Compact))
                            notes << tr("%1 地址列表也发生变化，请核对。").arg(name);
                    }
                }
                else notes << tr("修改后无法重新读取完整快照：%1").arg(output);
                const QString message = tr("%1\n%2\n%3").arg(title, result, notes.join("\n"));
                QMessageBox::information(this, changed && notes.isEmpty() ? tr("修改完成") : tr("需复核"), message);
            });
        });
    });
}

void NetworkAdapterDialog::addAddress(bool ipv6)
{
    QDialog dialog(this);
    dialog.setWindowTitle(ipv6 ? tr("添加静态 IPv6") : tr("添加附加 IPv4"));
    auto *layout = new QFormLayout(&dialog);
    auto *address = new QLineEdit(&dialog);
    address->setObjectName("newIpAddress");
    if(!ipv6) address->setInputMask(QStringLiteral("000.000.000.000;") + QChar(0x2060));
    auto *prefix = new QSpinBox(&dialog);
    prefix->setRange(1, ipv6 ? 128 : 32);
    prefix->setValue(ipv6 ? 64 : 24);
    auto *gateway = new QLineEdit(&dialog);
    auto *dns = new QLineEdit(&dialog);
    layout->addRow(tr("地址"), address);
    layout->addRow(tr("前缀长度"), prefix);
    if(ipv6)
    {
        layout->addRow(tr("IPv6 网关（可选）"), gateway);
        layout->addRow(tr("IPv6 DNS（可选）"), dns);
    }
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if(dialog.exec() != QDialog::Accepted) return;
    const auto family = ipv6 ? QAbstractSocket::IPv6Protocol : QAbstractSocket::IPv4Protocol;
    if(!validIp(address->text(), family) ||
       (!gateway->text().isEmpty() && !validIp(gateway->text(), QAbstractSocket::IPv6Protocol)) ||
       (!dns->text().isEmpty() && !validIp(dns->text(), QAbstractSocket::IPv6Protocol)))
    {
        QMessageBox::warning(this, tr("地址无效"), tr("地址、网关或 DNS 格式不正确。"));
        return;
    }
    const QMap<QString, QString> values{{"NCM_ADDRESS", address->text()},
                                       {"NCM_PREFIX", QString::number(prefix->value())},
                                       {"NCM_GATEWAY", gateway->text()}, {"NCM_DNS", dns->text()}};
    const QString script = ipv6 ? QStringLiteral(R"PS(
$address = $env:NCM_ADDRESS; $prefix = [int]$env:NCM_PREFIX
if (Get-NetIPAddress -InterfaceIndex $idx -AddressFamily IPv6 -IPAddress $address -ErrorAction SilentlyContinue) { throw '地址已存在' }
New-NetIPAddress -InterfaceIndex $idx -AddressFamily IPv6 -IPAddress $address -PrefixLength $prefix -ErrorAction Stop | Out-Null
if ($env:NCM_GATEWAY) {
    if (-not (Get-NetRoute -InterfaceIndex $idx -AddressFamily IPv6 -DestinationPrefix '::/0' -ErrorAction SilentlyContinue |
               Where-Object NextHop -eq $env:NCM_GATEWAY)) {
        New-NetRoute -InterfaceIndex $idx -DestinationPrefix '::/0' -NextHop $env:NCM_GATEWAY -PolicyStore PersistentStore -ErrorAction Stop | Out-Null
    }
}
if ($env:NCM_DNS) {
    & netsh interface ipv6 set dnsservers "name=$idx" source=static "address=$env:NCM_DNS" validate=no | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'IPv6 DNS 设置失败' }
}
if (-not (Get-NetIPAddress -InterfaceIndex $idx -AddressFamily IPv6 -IPAddress $address -ErrorAction SilentlyContinue)) { throw 'IPv6 地址未生效' }
if ($env:NCM_GATEWAY -and -not (Get-NetRoute -InterfaceIndex $idx -AddressFamily IPv6 -DestinationPrefix '::/0' -NextHop $env:NCM_GATEWAY -ErrorAction SilentlyContinue)) { throw 'IPv6 网关未生效' }
if ($env:NCM_DNS -and -not ((Get-DnsClientServerAddress -InterfaceIndex $idx -AddressFamily IPv6 -ErrorAction Stop).ServerAddresses -contains $env:NCM_DNS)) { throw 'IPv6 DNS 未生效' }
'IPv6 地址已核验'
)PS") : QStringLiteral(R"PS(
$address = $env:NCM_ADDRESS; $prefix = [int]$env:NCM_PREFIX
if (Get-NetIPAddress -InterfaceIndex $idx -AddressFamily IPv4 -IPAddress $address -ErrorAction SilentlyContinue) { throw '地址已存在' }
New-NetIPAddress -InterfaceIndex $idx -IPAddress $address -PrefixLength $prefix -SkipAsSource $true -ErrorAction Stop | Out-Null
$actual = Get-NetIPAddress -InterfaceIndex $idx -AddressFamily IPv4 -IPAddress $address -ErrorAction SilentlyContinue
if (-not $actual -or -not $actual.SkipAsSource) { throw '附加 IPv4 地址未生效' }
'附加 IPv4 地址已核验；主地址未覆盖'
)PS");
    mutate(tr("添加 %1/%2%3").arg(address->text()).arg(prefix->value())
           .arg(ipv6 ? tr("；网关 %1；DNS %2").arg(gateway->text(), dns->text()) : QString()),
           script, values, QJsonObject{{"kind", "address_add"}, {"family", ipv6 ? "IPv6" : "IPv4"},
                                       {"address", address->text()}, {"prefix", prefix->value()},
                                       {"gateway", gateway->text()}, {"dns", dns->text()}});
}

void NetworkAdapterDialog::removeAddress(bool ipv6)
{
    const int row = m_addresses->currentRow();
    if(row < 0) return;
    const QJsonObject selected = QJsonObject::fromVariantMap(m_addresses->item(row, 0)->data(Qt::UserRole).toMap());
    if(selected.value("Origin").toString() != "Manual" || (!ipv6 && !selected.value("Skip").toBool()))
    {
        QMessageBox::warning(this, tr("禁止删除"), tr("只能删除手动添加的地址；IPv4 主地址及自动地址需通过原配置入口管理。"));
        return;
    }
    const QString address = selected.value("Address").toString();
    if(!validIp(address, ipv6 ? QAbstractSocket::IPv6Protocol : QAbstractSocket::IPv4Protocol)) return;
    const QString script = QStringLiteral(R"PS(
$family = $env:NCM_FAMILY; $address = $env:NCM_ADDRESS
$current = Get-NetIPAddress -InterfaceIndex $idx -AddressFamily $family -IPAddress $address -ErrorAction Stop
if ($current.PrefixOrigin -ne 'Manual' -or ($family -eq 'IPv4' -and -not $current.SkipAsSource)) { throw '系统地址或主地址不可删除' }
$current | Remove-NetIPAddress -Confirm:$false -ErrorAction Stop
if (Get-NetIPAddress -InterfaceIndex $idx -AddressFamily $family -IPAddress $address -ErrorAction SilentlyContinue) { throw '地址仍存在' }
'地址删除已核验'
)PS");
    mutate(tr("删除 %1 手动地址 %2/%3").arg(ipv6 ? "IPv6" : "IPv4", address)
           .arg(selected.value("Prefix").toInt()), script,
           {{"NCM_FAMILY", ipv6 ? "IPv6" : "IPv4"}, {"NCM_ADDRESS", address}},
           QJsonObject{{"kind", "address_remove"}, {"family", ipv6 ? "IPv6" : "IPv4"},
                       {"address", address}, {"prefix", selected.value("Prefix")},
                       {"skip", selected.value("Skip")}});
}

void NetworkAdapterDialog::addRoute()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("新增静态路由"));
    auto *layout = new QFormLayout(&dialog);
    auto *prefix = new QLineEdit(&dialog);
    prefix->setPlaceholderText("192.168.10.0/24 或 2001:db8::/64");
    auto *gateway = new QLineEdit(&dialog);
    auto *metric = new QSpinBox(&dialog);
    metric->setRange(1, 9999);
    metric->setValue(10);
    auto *persistent = new QCheckBox(tr("重启后保留"), &dialog);
    persistent->setChecked(true);
    layout->addRow(tr("目标网段"), prefix);
    layout->addRow(tr("下一跳"), gateway);
    layout->addRow(tr("路由跃点"), metric);
    layout->addRow(persistent);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if(dialog.exec() != QDialog::Accepted) return;
    QHostAddress destination(prefix->text().section('/', 0, 0));
    if(!validPrefix(prefix->text()) || !validIp(gateway->text(), destination.protocol()))
    {
        QMessageBox::warning(this, tr("路由无效"), tr("目标网段或下一跳无效，且两者必须属于同一地址族。"));
        return;
    }
    const QString store = persistent->isChecked() ? "PersistentStore" : "ActiveStore";
    const QString script = QStringLiteral(R"PS(
$prefix = $env:NCM_PREFIX; $gateway = $env:NCM_GATEWAY; $store = $env:NCM_STORE
if (Get-NetRoute -InterfaceIndex $idx -DestinationPrefix $prefix -NextHop $gateway -PolicyStore $store -ErrorAction SilentlyContinue) { throw '静态路由已存在' }
New-NetRoute -InterfaceIndex $idx -DestinationPrefix $prefix -NextHop $gateway -RouteMetric ([int]$env:NCM_METRIC) -PolicyStore $store -ErrorAction Stop | Out-Null
if (-not (Get-NetRoute -InterfaceIndex $idx -DestinationPrefix $prefix -NextHop $gateway -PolicyStore $store -ErrorAction SilentlyContinue)) { throw '路由未生效' }
'路由已核验'
)PS");
    mutate(tr("添加 %1 → %2，跃点 %3，%4")
           .arg(prefix->text(), gateway->text()).arg(metric->value()).arg(store), script,
           {{"NCM_PREFIX", prefix->text()}, {"NCM_GATEWAY", gateway->text()},
            {"NCM_METRIC", QString::number(metric->value())}, {"NCM_STORE", store}},
           QJsonObject{{"kind", "route_add"}, {"prefix", prefix->text()},
                       {"gateway", gateway->text()}, {"metric", metric->value()}, {"store", store}});
}

void NetworkAdapterDialog::removeRoute()
{
    const int row = m_routes->currentRow();
    if(row < 0) return;
    const QJsonObject selected = QJsonObject::fromVariantMap(m_routes->item(row, 0)->data(Qt::UserRole).toMap());
    const QString prefix = selected.value("Prefix").toString();
    const QString gateway = selected.value("Gateway").toString();
    const QString store = selected.value("Store").toString();
    const QHostAddress destination(prefix.section('/', 0, 0));
    if(selected.value("Protocol").toString() != "NetMgmt" || !validPrefix(prefix) ||
       !validIp(gateway, destination.protocol()) ||
       (store != "ActiveStore" && store != "PersistentStore"))
    {
        QMessageBox::warning(this, tr("禁止删除"), tr("仅允许删除已识别的静态路由；动态与系统路由受保护。"));
        return;
    }
    const QString script = QStringLiteral(R"PS(
$route = Get-NetRoute -InterfaceIndex $idx -DestinationPrefix $env:NCM_PREFIX -NextHop $env:NCM_GATEWAY -PolicyStore $env:NCM_STORE -ErrorAction Stop |
    Where-Object Protocol -eq 'NetMgmt' | Select-Object -First 1
if (-not $route) { throw '路由来源已变化，拒绝删除' }
$route | Remove-NetRoute -Confirm:$false -ErrorAction Stop
if (Get-NetRoute -InterfaceIndex $idx -DestinationPrefix $env:NCM_PREFIX -NextHop $env:NCM_GATEWAY -PolicyStore $env:NCM_STORE -ErrorAction SilentlyContinue) { throw '路由仍存在' }
'静态路由删除已核验'
)PS");
    mutate(tr("删除静态路由 %1 → %2（%3）").arg(prefix, gateway, store), script,
           {{"NCM_PREFIX", prefix}, {"NCM_GATEWAY", gateway}, {"NCM_STORE", store}},
           QJsonObject{{"kind", "route_remove"}, {"prefix", prefix}, {"gateway", gateway},
                       {"metric", selected.value("Metric")}, {"store", store}});
}

void NetworkAdapterDialog::changeMtu()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("修改 MTU 与接口跃点数"));
    auto *layout = new QFormLayout(&dialog);
    auto *family = new QComboBox(&dialog);
    family->addItems({"IPv4", "IPv6"});
    auto *mtu = new QSpinBox(&dialog);
    mtu->setRange(576, 9000);
    auto *metric = new QSpinBox(&dialog);
    metric->setRange(1, 9999);
    const QJsonObject current = m_snapshot.value("Interface4").toObject();
    mtu->setValue(qMax(576, current.value("Mtu").toInt(1500)));
    metric->setValue(qMax(1, current.value("Metric").toInt(25)));
    connect(family, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog,
            [this, mtu, metric, family](int) {
        const QJsonObject value = m_snapshot.value(family->currentText() == "IPv6" ? "Interface6" : "Interface4").toObject();
        mtu->setMinimum(family->currentText() == "IPv6" ? 1280 : 576);
        mtu->setValue(qMax(mtu->minimum(), value.value("Mtu").toInt(1500)));
        metric->setValue(qMax(1, value.value("Metric").toInt(25)));
    });
    layout->addRow(tr("地址族"), family);
    layout->addRow(tr("MTU"), mtu);
    layout->addRow(tr("接口跃点数"), metric);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if(dialog.exec() != QDialog::Accepted) return;
    const QString script = QStringLiteral(R"PS(
$family = $env:NCM_FAMILY; $mtu = [int]$env:NCM_MTU; $metric = [int]$env:NCM_METRIC
Set-NetIPInterface -InterfaceIndex $idx -AddressFamily $family -NlMtuBytes $mtu -InterfaceMetric $metric -AutomaticMetric Disabled -ErrorAction Stop
$actual = Get-NetIPInterface -InterfaceIndex $idx -AddressFamily $family -ErrorAction Stop
if ($actual.NlMtu -ne $mtu -or $actual.InterfaceMetric -ne $metric) { throw 'MTU 或接口跃点数未生效' }
'MTU 与接口跃点数已核验'
)PS");
    mutate(tr("设置 %1 MTU=%2、接口跃点=%3").arg(family->currentText())
           .arg(mtu->value()).arg(metric->value()), script,
           {{"NCM_FAMILY", family->currentText()}, {"NCM_MTU", QString::number(mtu->value())},
            {"NCM_METRIC", QString::number(metric->value())}},
           QJsonObject{{"kind", "interface"}, {"family", family->currentText()}});
}

void NetworkAdapterDialog::flushDns()
{
    if(QMessageBox::question(this, tr("清空 DNS 缓存"),
          tr("这会清空本机 DNS 缓存，不修改网卡地址或 DNS 服务器。继续？"),
          QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
    if(!m_manager->isAdmin())
    {
        m_manager->requestAdminPrivileges();
        return;
    }
    runScript(QStringLiteral("ipconfig /flushdns; if ($LASTEXITCODE -ne 0) { throw '清空 DNS 缓存失败' }"), {},
              [this](bool ok, const QString &output) {
        QMessageBox::information(this, tr("DNS 缓存"), ok ? tr("已清空本机 DNS 缓存。")
                                    : tr("清空失败：%1").arg(output));
    });
}

void NetworkAdapterDialog::undoLast()
{
    if(m_process || m_adapter->currentIndex() < 0) return;
    QFile file(m_backupFile);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this, tr("恢复"), tr("没有本地修改快照。"));
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if(!document.isArray())
    {
        QMessageBox::warning(this, tr("恢复"), tr("快照文件无效。"));
        return;
    }
    QJsonArray history = document.array();
    int index = -1;
    for(int i = history.size() - 1; i >= 0; --i)
        if(history.at(i).toObject().value("adapter_id") == m_adapter->currentData().toString())
        { index = i; break; }
    if(index < 0)
    {
        QMessageBox::information(this, tr("恢复"), tr("此网卡没有本地修改快照。"));
        return;
    }
    const QJsonObject record = history.at(index).toObject();
    const QJsonObject undo = record.value("undo").toObject();
    const QJsonObject before = record.value("before").toObject();
    const QString kind = undo.value("kind").toString();
    QString script;
    QMap<QString, QString> values;
    if(kind == "address_add" || kind == "address_remove")
    {
        const QString family = undo.value("family").toString();
        const QString address = undo.value("address").toString();
        const int prefix = undo.value("prefix").toInt();
        if((family != "IPv4" && family != "IPv6") ||
           !validIp(address, family == "IPv4" ? QAbstractSocket::IPv4Protocol : QAbstractSocket::IPv6Protocol) ||
           prefix < 1 || prefix > (family == "IPv4" ? 32 : 128)) return;
        values = {{"NCM_FAMILY", family}, {"NCM_ADDRESS", address}, {"NCM_PREFIX", QString::number(prefix)}};
        if(kind == "address_add")
        {
            const QString gateway = undo.value("gateway").toString();
            if(!gateway.isEmpty() && !validIp(gateway, QAbstractSocket::IPv6Protocol)) return;
            values["NCM_GATEWAY"] = gateway;
            values["NCM_GATEWAY_ADDED"] = undo.value("gateway_existed").toBool() ? "0" : "1";
            QStringList dns;
            for(const QJsonValue &server : array(before.value("Dns6")))
            {
                if(!validIp(server.toString(), QAbstractSocket::IPv6Protocol)) return;
                dns << server.toString();
            }
            values["NCM_DNS_BEFORE"] = dns.join(',');
            values["NCM_DNS_CHANGED"] = undo.value("dns").toString().isEmpty() ? "0" : "1";
            script = QStringLiteral(R"PS(
$current = Get-NetIPAddress -InterfaceIndex $idx -AddressFamily $env:NCM_FAMILY -IPAddress $env:NCM_ADDRESS -ErrorAction SilentlyContinue
if ($current -and $current.PrefixOrigin -eq 'Manual' -and ($env:NCM_FAMILY -eq 'IPv6' -or $current.SkipAsSource)) {
    $current | Remove-NetIPAddress -Confirm:$false -ErrorAction Stop
}
if ($env:NCM_GATEWAY -and $env:NCM_GATEWAY_ADDED -eq '1') {
    Get-NetRoute -InterfaceIndex $idx -AddressFamily IPv6 -DestinationPrefix '::/0' -NextHop $env:NCM_GATEWAY -PolicyStore PersistentStore -ErrorAction SilentlyContinue |
        Where-Object Protocol -eq 'NetMgmt' | Remove-NetRoute -Confirm:$false -ErrorAction Stop
}
if ($env:NCM_DNS_CHANGED -eq '1') {
    $old = @($env:NCM_DNS_BEFORE.Split(',') | Where-Object { $_ })
    if ($old.Count -eq 0) {
        & netsh interface ipv6 set dnsservers "name=$idx" source=dhcp | Out-Null
    } else {
        & netsh interface ipv6 set dnsservers "name=$idx" source=static "address=$($old[0])" validate=no | Out-Null
        for ($n = 1; $n -lt $old.Count; $n++) {
            & netsh interface ipv6 add dnsservers "name=$idx" "address=$($old[$n])" "index=$($n + 1)" validate=no | Out-Null
        }
    }
    if ($LASTEXITCODE -ne 0) { throw '恢复 IPv6 DNS 失败' }
}
'地址修改前值已恢复'
)PS");
        }
        else
        {
            values["NCM_SKIP"] = undo.value("skip").toBool() ? "true" : "false";
            script = QStringLiteral(R"PS(
if (-not (Get-NetIPAddress -InterfaceIndex $idx -AddressFamily $env:NCM_FAMILY -IPAddress $env:NCM_ADDRESS -ErrorAction SilentlyContinue)) {
    New-NetIPAddress -InterfaceIndex $idx -AddressFamily $env:NCM_FAMILY -IPAddress $env:NCM_ADDRESS -PrefixLength ([int]$env:NCM_PREFIX) -SkipAsSource ([bool]::Parse($env:NCM_SKIP)) -ErrorAction Stop | Out-Null
}
if (-not (Get-NetIPAddress -InterfaceIndex $idx -AddressFamily $env:NCM_FAMILY -IPAddress $env:NCM_ADDRESS -ErrorAction SilentlyContinue)) { throw '地址恢复未生效' }
'手动地址已恢复'
)PS");
        }
    }
    else if(kind == "route_add" || kind == "route_remove")
    {
        const QString prefix = undo.value("prefix").toString(), gateway = undo.value("gateway").toString();
        const QString store = undo.value("store").toString();
        const QHostAddress destination(prefix.section('/', 0, 0));
        const int metric = undo.value("metric").toInt();
        if(!validPrefix(prefix) || !validIp(gateway, destination.protocol()) ||
           (store != "ActiveStore" && store != "PersistentStore") || metric < 1 || metric > 9999) return;
        values = {{"NCM_PREFIX", prefix}, {"NCM_GATEWAY", gateway}, {"NCM_STORE", store},
                  {"NCM_METRIC", QString::number(metric)}};
        script = kind == "route_add" ? QStringLiteral(R"PS(
Get-NetRoute -InterfaceIndex $idx -DestinationPrefix $env:NCM_PREFIX -NextHop $env:NCM_GATEWAY -PolicyStore $env:NCM_STORE -ErrorAction SilentlyContinue |
    Where-Object Protocol -eq 'NetMgmt' | Remove-NetRoute -Confirm:$false -ErrorAction Stop
'新增路由已撤销'
)PS") : QStringLiteral(R"PS(
if (-not (Get-NetRoute -InterfaceIndex $idx -DestinationPrefix $env:NCM_PREFIX -NextHop $env:NCM_GATEWAY -PolicyStore $env:NCM_STORE -ErrorAction SilentlyContinue)) {
    New-NetRoute -InterfaceIndex $idx -DestinationPrefix $env:NCM_PREFIX -NextHop $env:NCM_GATEWAY -RouteMetric ([int]$env:NCM_METRIC) -PolicyStore $env:NCM_STORE -ErrorAction Stop | Out-Null
}
if (-not (Get-NetRoute -InterfaceIndex $idx -DestinationPrefix $env:NCM_PREFIX -NextHop $env:NCM_GATEWAY -PolicyStore $env:NCM_STORE -ErrorAction SilentlyContinue)) { throw '路由恢复未生效' }
'静态路由已恢复'
)PS");
    }
    else if(kind == "interface")
    {
        const QString family = undo.value("family").toString();
        if(family != "IPv4" && family != "IPv6") return;
        const QJsonObject previous = before.value(family == "IPv4" ? "Interface4" : "Interface6").toObject();
        const int mtu = previous.value("Mtu").toInt(), metric = previous.value("Metric").toInt();
        const QString automatic = previous.value("AutoMetric").toString();
        if(mtu < (family == "IPv4" ? 576 : 1280) || mtu > 9000 || metric < 1 || metric > 9999 ||
           (automatic != "Enabled" && automatic != "Disabled")) return;
        values = {{"NCM_FAMILY", family}, {"NCM_MTU", QString::number(mtu)},
                  {"NCM_METRIC", QString::number(metric)}, {"NCM_AUTO", automatic}};
        script = QStringLiteral(R"PS(
Set-NetIPInterface -InterfaceIndex $idx -AddressFamily $env:NCM_FAMILY -NlMtuBytes ([int]$env:NCM_MTU) -InterfaceMetric ([int]$env:NCM_METRIC) -AutomaticMetric $env:NCM_AUTO -ErrorAction Stop
$actual = Get-NetIPInterface -InterfaceIndex $idx -AddressFamily $env:NCM_FAMILY -ErrorAction Stop
if ($actual.NlMtu -ne [int]$env:NCM_MTU) { throw 'MTU 恢复未生效' }
'MTU 与接口跃点数已恢复保存值'
)PS");
    }
    else return;
    if(QMessageBox::warning(this, tr("恢复修改前值"),
          tr("快照时间：%1\n网卡：%2\n原操作：%3\n只撤销该操作涉及的字段。继续？")
              .arg(record.value("time").toString(), m_adapter->currentText(), record.value("title").toString()),
          QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
    if(!m_manager->isAdmin())
    {
        m_manager->requestAdminPrivileges();
        return;
    }
    runScript(adapterLookup() + script, values, [this, history, index](bool ok, const QString &output) mutable {
        if(ok)
        {
            history.removeAt(index);
            QSaveFile file(m_backupFile);
            if(file.open(QIODevice::WriteOnly))
            {
                const QByteArray data = QJsonDocument(history).toJson();
                if(file.write(data) != data.size() || !file.commit())
                    QMessageBox::warning(this, tr("恢复记录"), tr("系统已恢复，但无法更新本地回滚记录。"));
            }
        }
        QMessageBox::information(this, tr("恢复修改前值"), ok ? tr("已恢复并核验：%1").arg(output)
                                          : tr("恢复失败：%1").arg(output));
        refresh();
    });
}

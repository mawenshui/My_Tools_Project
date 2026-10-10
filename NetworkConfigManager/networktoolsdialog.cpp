#include "networktoolsdialog.h"
#include "configmanager.h"
#include "networkinterfacemanager.h"
#include "networkscandialog.h"
#include "networkdiagnosticsdialog.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>

NetworkToolsDialog::NetworkToolsDialog(ConfigManager *manager, QWidget *parent)
    : QDialog(parent), m_manager(manager), m_profiles(new QComboBox(this)),
      m_interfaces(new QListWidget(this))
{
    setWindowTitle(tr("网络工具"));
    resize(560, 420);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("选择网卡和已保存的配置；静态配置批量应用可能造成 IP 冲突。"), this));
    layout->addWidget(m_interfaces);
    layout->addWidget(m_profiles);

    auto *buttons = new QHBoxLayout;
    auto *apply = new QPushButton(tr("批量应用配置"), this);
    auto *dhcp = new QPushButton(tr("批量恢复 DHCP"), this);
    auto *restore = new QPushButton(tr("回滚最近备份"), this);
    buttons->addWidget(apply);
    buttons->addWidget(dhcp);
    buttons->addWidget(restore);
    layout->addLayout(buttons);

    auto *systemButtons = new QHBoxLayout;
    auto *refresh = new QPushButton(tr("刷新网卡"), this);
    auto *connections = new QPushButton(tr("打开网络连接"), this);
    auto *scan = new QPushButton(tr("扫描所选网段"), this);
    auto *diagnostics = new QPushButton(tr("延迟与测速"), this);
    systemButtons->addWidget(refresh);
    systemButtons->addWidget(scan);
    systemButtons->addWidget(diagnostics);
    systemButtons->addWidget(connections);
    layout->addLayout(systemButtons);

    connect(apply, &QPushButton::clicked, this, [this]() { runBatch(false); });
    connect(dhcp, &QPushButton::clicked, this, [this]() { runBatch(true); });
    connect(restore, &QPushButton::clicked, this, &NetworkToolsDialog::restoreSelected);
    connect(refresh, &QPushButton::clicked, this, &NetworkToolsDialog::refreshInterfaces);
    connect(scan, &QPushButton::clicked, this, [this]() {
        NetworkScanDialog dialog(this);
        const QString name = checkedInterfaces().value(0);
        if(!name.isEmpty())
        {
            const QVariantMap config = NetworkInterfaceManager::captureConfig(name);
            dialog.autoScan(config.value("ip").toString(), config.value("subnet").toString());
        }
        dialog.exec();
    });
    connect(diagnostics, &QPushButton::clicked, this, [this]() {
        NetworkDiagnosticsDialog dialog(checkedInterfaces().value(0), this);
        dialog.exec();
    });
    connect(connections, &QPushButton::clicked, this, [this]() {
        if(!QProcess::startDetached("control.exe", QStringList() << "ncpa.cpl"))
            QMessageBox::warning(this, tr("网络连接"), tr("无法打开系统网络连接窗口。"));
    });

    const auto profiles = m_manager->configs();
    for(auto it = profiles.cbegin(); it != profiles.cend(); ++it)
        m_profiles->addItem(it.key(), it.value());
    refreshInterfaces();
}

void NetworkToolsDialog::refreshInterfaces()
{
    m_interfaces->clear();
    for(const InterfaceDetail &detail : NetworkInterfaceManager::getAllInterfaceDetails())
    {
        if(detail.name.isEmpty()) continue;
        auto *item = new QListWidgetItem(QString("%1  (%2 / %3)")
                                         .arg(detail.name, detail.adminStatus, detail.connStatus), m_interfaces);
        item->setData(Qt::UserRole, detail.name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

QStringList NetworkToolsDialog::checkedInterfaces() const
{
    QStringList result;
    for(int index = 0; index < m_interfaces->count(); ++index)
    {
        const auto *item = m_interfaces->item(index);
        if(item->checkState() == Qt::Checked) result << item->data(Qt::UserRole).toString();
    }
    return result;
}

void NetworkToolsDialog::runBatch(bool dhcp)
{
    const QStringList interfaces = checkedInterfaces();
    if(interfaces.isEmpty())
    {
        QMessageBox::information(this, tr("批量操作"), tr("请先勾选网卡。"));
        return;
    }
    QVariantMap templateConfig = dhcp ? QVariantMap{{"method", "dhcp"}, {"custom_dns", false}}
                                       : m_profiles->currentData().toMap();
    if(templateConfig.isEmpty())
    {
        QMessageBox::information(this, tr("批量操作"), tr("请先选择已保存的配置。"));
        return;
    }
    if(interfaces.size() > 1 && templateConfig.value("method").toString() == "static" &&
       QMessageBox::warning(this, tr("IP 冲突风险"),
                            tr("相同静态 IP 将应用到 %1 块网卡，可能造成地址冲突。继续？").arg(interfaces.size()),
                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    QProgressDialog progress(tr("正在修改网卡配置..."), tr("取消后续操作"), 0, interfaces.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    QStringList failed;
    int processed = 0;
    for(int index = 0; index < interfaces.size(); ++index)
    {
        progress.setValue(index);
        QCoreApplication::processEvents();
        if(progress.wasCanceled()) break;
        QVariantMap config = templateConfig;
        config["interface"] = interfaces.at(index);
        if(!m_manager->applyConfig(config)) failed << interfaces.at(index);
        ++processed;
    }
    progress.setValue(interfaces.size());
    if(failed.isEmpty())
    {
        QMessageBox::information(this, tr("批量操作"),
                                 tr("已处理 %1/%2 块网卡。").arg(processed).arg(interfaces.size()));
        return;
    }
    const auto choice = QMessageBox::warning(this, tr("批量操作"),
                         tr("以下网卡应用失败：%1\n是否回滚这些网卡的最近备份？").arg(failed.join(", ")),
                         QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if(choice == QMessageBox::Yes)
    {
        QStringList restoreFailed;
        for(const QString &name : failed)
            if(!m_manager->restoreLastNetworkBackup(name)) restoreFailed << name;
        if(!restoreFailed.isEmpty())
            QMessageBox::warning(this, tr("回滚"), tr("未恢复：%1").arg(restoreFailed.join(", ")));
    }
}

void NetworkToolsDialog::restoreSelected()
{
    const QStringList interfaces = checkedInterfaces();
    if(interfaces.isEmpty())
    {
        QMessageBox::information(this, tr("回滚"), tr("请先勾选网卡。"));
        return;
    }
    if(QMessageBox::question(this, tr("回滚"), tr("恢复所选网卡的最近备份？")) != QMessageBox::Yes)
        return;
    QStringList failed;
    for(const QString &name : interfaces)
        if(!m_manager->restoreLastNetworkBackup(name)) failed << name;
    QMessageBox::information(this, tr("回滚"), failed.isEmpty() ? tr("已恢复所选网卡。")
                             : tr("未恢复：%1").arg(failed.join(", ")));
}

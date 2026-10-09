#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QMenu>
#include <QCloseEvent>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QDir>
#include <QPainter>
#include <QTextCodec>
#include <QFileDialog>
#include <QFile>
#include <QThread>
#include <QMetaObject>
#include <QMenuBar>
#include <QGridLayout>
#include <QGroupBox>
#include <QProcess>
#include "networktoolsdialog.h"
#include "networkscandialog.h"
#include "networkdiagnosticsdialog.h"
#include "networktrafficmonitor.h"

/**
 * @brief MainWindow构造函数
 * @param parent 父窗口指针
 *
 * 初始化主窗口，包括：
 * 1. 检查单实例运行
 * 2. 初始化UI组件
 * 3. 加载应用程序配置
 * 4. 初始化系统托盘和悬浮窗
 */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_configManager(new ConfigManager(this)),  //初始化配置管理器
    m_networkInfoCollector(new NetworkInfoCollector(this)),  //初始化网络信息收集器
    m_quickMenu(nullptr),                     //快速菜单初始为空
    m_floatWindow(new FloatWindow(this)),     //创建悬浮窗
    m_trayIcon(new QSystemTrayIcon(this)),    //创建系统托盘图标
    m_floatVisible(true),                     //默认显示悬浮窗
    m_autostart(false)                        //默认不启用开机自启动
{
    ui->setupUi(this);
    QAction *networkToolsAction = menuBar()->addAction(tr("网络工具"));
    connect(networkToolsAction, &QAction::triggered, this, [this]() {
        NetworkToolsDialog dialog(m_configManager, this);
        dialog.setStyleSheet(styleSheet());
        dialog.exec();
    });
    initSettings(posConfigPath);
    //确保先初始化菜单
    Logger::debug("初始化快速应用配置菜单");
    m_quickMenu = new QMenu("快速应用配置", this);
    //2. 初始化UI组件
    Logger::info("开始初始化UI组件");
    setupUi();
    //3. 初始化配置和网络接口
    Logger::info("开始初始化应用程序配置");
    initializeApplication();
    //4. 初始化系统托盘和悬浮窗
    Logger::info("初始化系统托盘和悬浮窗");
    //初始化系统托盘
    setupTrayIcon();
    //5. 最后建立连接
    Logger::debug("建立信号槽连接");
    setupConnections();
    //初始化悬浮窗位置
    loadFloatWindowPosition();
    if(m_floatVisible)
    {
        Logger::debug("显示悬浮窗");
        m_floatWindow->setBackgroundPixmap(QPixmap(":/images/images/float_icon.png"));
        m_floatWindow->show();
    }
    loadAndApplyStyleSheet(":/styles/styles/drak_theme.qss");
    Logger::info("主窗口初始化完成");
    show(); // 确保主窗口显示
}

/**
 * @brief 加载并应用样式表
 * @param styleSheetPath 样式表文件路径
 */
void MainWindow::loadAndApplyStyleSheet(const QString &styleSheetPath)
{
    QFile file(styleSheetPath);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString styleSheet = file.readAll();
        this->setStyleSheet(styleSheet);
        file.close();
    }
    else
    {
        Logger::critical("加载样式表失败");
        return;
    }
}

/**
 * @brief MainWindow析构函数
 *
 * 清理资源，保存窗口状态
 */
MainWindow::~MainWindow()
{
    Logger::info("开始销毁主窗口");
    //安全删除菜单
    if(m_quickMenu)
    {
        Logger::debug("删除快速菜单");
        m_quickMenu->deleteLater();
    }
    Logger::debug("保存窗口状态");
    saveWindowState();
    delete ui;
    Logger::info("主窗口销毁完成");
}

void MainWindow::saveFloatWindowPosition()
{
    if(!m_settings)
    {
        return;
    }
    if(m_floatWindow && m_floatWindow->isVisible())
    {
        m_settings->beginGroup("FloatWindow");
        m_settings->setValue("pos", m_floatWindow->pos());
        m_settings->endGroup();
        m_settings->sync(); //立即写入磁盘
    }
}

void MainWindow::loadFloatWindowPosition()
{
    if(!m_settings)
    {
        return;
    }
    m_settings->beginGroup("FloatWindow");
    if(m_settings->contains("pos"))
    {
        QPoint pos = m_settings->value("pos").toPoint();
        QRect floatRect(pos, m_floatWindow->size());
        bool isVisible = false;
        for(QScreen *screen : qApp->screens())
        {
            QRect screenGeo = screen->availableGeometry();
            if(screenGeo.intersects(floatRect))
            {
                isVisible = true;
                break;
            }
        }
        if(!isVisible)
        {
            QScreen *primaryScreen = qApp->primaryScreen();
            QRect screenGeo = primaryScreen->availableGeometry();
            pos.setX(screenGeo.left() + 50);
            pos.setY(screenGeo.top() + screenGeo.height() - m_floatWindow->height() - 50);
        }
        m_floatWindow->move(pos);
    }
    m_settings->endGroup();
}

void MainWindow::initSettings(const QString &posConfigPath)
{
    //确定最终配置文件路径
    QString configFileName;
    if(posConfigPath.isEmpty())
    {
        configFileName = "config.ini";
    }
    else
    {
        //处理用户输入的路径，确保以config.ini结尾
        configFileName = posConfigPath.endsWith(".ini", Qt::CaseInsensitive)
                         ? posConfigPath
                         : posConfigPath + "/config.ini";
    }
    //构建完整路径（确保在应用程序目录下）
    m_posConfigPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/" + configFileName);
    //确保配置目录存在
    QFileInfo fileInfo(m_posConfigPath);
    if(!fileInfo.dir().exists())
    {
        if(!QDir().mkpath(fileInfo.absolutePath()))
        {
            Logger::warning("无法创建配置目录:" + fileInfo.absolutePath());
            //回退到临时目录
            m_posConfigPath = QDir::toNativeSeparators(
                                  QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/config.ini"
                              );
            Logger::warning("已回退到临时目录:" + m_posConfigPath);
        }
    }
    //初始化QSettings（如果文件不存在会自动创建）
    m_settings = new QSettings(m_posConfigPath, QSettings::IniFormat, this);
    //立即创建空文件（如果不存在）
    if(!QFile::exists(m_posConfigPath))
    {
        QFile file(m_posConfigPath);
        if(file.open(QIODevice::WriteOnly))
        {
            file.close();
            Logger::info("已创建新的配置文件:" + m_posConfigPath);
        }
        else
        {
            Logger::critical("无法创建配置文件:" + m_posConfigPath + "错误:" + file.errorString());
        }
    }
    //验证文件可写性
    if(!QFileInfo(m_posConfigPath).isWritable())
    {
        Logger::critical("配置文件不可写:" + m_posConfigPath);
    }
    Logger::info("使用的配置文件路径:" + QDir::toNativeSeparators(m_posConfigPath));
}

/**
 * @brief 初始化UI组件
 *
 * 设置UI初始状态，恢复窗口几何状态
 */
void MainWindow::setupUi()
{
    Logger::debug("设置UI初始状态");
    //初始状态
    ui->dhcpRadio->setChecked(true);
    onIpMethodToggled(true);
    //设置图标
    setWindowIcon(QIcon(":/images/images/icon.png"));
    //恢复窗口状态
    Logger::debug("恢复窗口状态");
    restoreWindowState();
    m_statusIndicator = new QLabel(this);
    m_statusIndicator->setFixedSize(16, 16);
    m_historyListWidget = ui->historyListWidget;
    m_statusIndicator->setToolTip("配置状态");
    QPixmap grayIndicator(16, 16);
    grayIndicator.fill(Qt::transparent);
    {
        QPainter p(&grayIndicator);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor(128, 128, 128));
        p.setPen(Qt::NoPen);
        p.drawEllipse(2, 2, 12, 12);
    }
    m_statusIndicator->setPixmap(grayIndicator);
    m_defaultIndicator = grayIndicator;
    ui->statusBar->addPermanentWidget(m_statusIndicator);

    auto *toolsGroup = new QGroupBox(tr("网络工具"), ui->centralWidget);
    toolsGroup->setObjectName("networkToolsGroup");
    toolsGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto *toolsLayout = new QGridLayout(toolsGroup);
    auto *batchButton = new QPushButton(tr("批量配置"), toolsGroup);
    auto *restoreButton = new QPushButton(toolsGroup);
    auto *scanButton = new QPushButton(tr("网段扫描"), toolsGroup);
    auto *connectionsButton = new QPushButton(tr("网络连接"), toolsGroup);
    auto *latencyButton = new QPushButton(tr("延迟检测"), toolsGroup);
    auto *speedButton = new QPushButton(tr("下载测速"), toolsGroup);
    auto *trafficMonitor = new NetworkTrafficMonitor(toolsGroup);
    batchButton->setObjectName("batchNetworkButton");
    restoreButton->setObjectName("restoreNetworkButton");
    scanButton->setObjectName("scanNetworkButton");
    connectionsButton->setObjectName("networkConnectionsButton");
    latencyButton->setObjectName("latencyTestButton");
    speedButton->setObjectName("speedTestButton");
    trafficMonitor->setObjectName("networkTrafficMonitor");
    batchButton->setToolTip(tr("批量应用已保存配置或恢复 DHCP"));
    toolsLayout->addWidget(batchButton, 0, 0);
    toolsLayout->addWidget(restoreButton, 0, 1);
    toolsLayout->addWidget(scanButton, 0, 2);
    toolsLayout->addWidget(latencyButton, 0, 3);
    toolsLayout->addWidget(speedButton, 0, 4);
    toolsLayout->addWidget(connectionsButton, 0, 5);
    toolsLayout->addWidget(trafficMonitor, 0, 6);
    for(int column = 0; column < 6; ++column) toolsLayout->setColumnStretch(column, 1);
    toolsLayout->setColumnStretch(6, 2);
    ui->verticalLayout->insertWidget(1, toolsGroup);

    const auto updateBackupButton = [this, restoreButton]() {
        const int count = m_configManager->networkBackupCount(ui->interfaceCombo->currentText());
        restoreButton->setText(tr("回滚备份 (%1)").arg(count));
        restoreButton->setEnabled(count > 0);
        restoreButton->setToolTip(tr("应用配置前自动备份；恢复当前网卡的最近一次备份"));
    };
    connect(ui->interfaceCombo, &QComboBox::currentTextChanged, this, updateBackupButton);
    connect(m_configManager, &ConfigManager::configApplied, this, updateBackupButton);
    connect(batchButton, &QPushButton::clicked, this, [this, updateBackupButton]() {
        NetworkToolsDialog dialog(m_configManager, this);
        dialog.setStyleSheet(styleSheet());
        dialog.exec();
        updateBackupButton();
    });
    connect(restoreButton, &QPushButton::clicked, this, [this, updateBackupButton]() {
        const QString name = ui->interfaceCombo->currentText();
        if(QMessageBox::question(this, tr("回滚备份"), tr("恢复 %1 的最近一次网络配置备份？").arg(name)) != QMessageBox::Yes)
            return;
        if(m_configManager->restoreLastNetworkBackup(name))
            ui->statusBar->showMessage(tr("已恢复 %1 的网络配置").arg(name), 3000);
        else QMessageBox::warning(this, tr("回滚备份"), tr("无法恢复 %1 的最近备份。").arg(name));
        updateBackupButton();
    });
    connect(scanButton, &QPushButton::clicked, this, [this]() {
        NetworkScanDialog dialog(this);
        dialog.setStyleSheet(styleSheet());
        dialog.exec();
    });
    const auto showDiagnostics = [this](bool speed) {
        NetworkDiagnosticsDialog dialog(ui->interfaceCombo->currentText(), this, speed);
        dialog.setStyleSheet(styleSheet());
        dialog.exec();
    };
    connect(latencyButton, &QPushButton::clicked, this, [showDiagnostics]() { showDiagnostics(false); });
    connect(speedButton, &QPushButton::clicked, this, [showDiagnostics]() { showDiagnostics(true); });
    connect(connectionsButton, &QPushButton::clicked, this, [this]() {
        if(!QProcess::startDetached("control.exe", QStringList() << "ncpa.cpl"))
            QMessageBox::warning(this, tr("网络连接"), tr("无法打开系统网络连接窗口。"));
    });
    connect(ui->interfaceCombo, &QComboBox::currentTextChanged,
            trafficMonitor, &NetworkTrafficMonitor::setInterfaceName);
    trafficMonitor->setInterfaceName(ui->interfaceCombo->currentText());
    updateBackupButton();
    //初始化状态栏
    ui->statusBar->showMessage("就绪", 2000);

    //初始化加载遮罩
    m_loadingOverlay = new QWidget(this);
    m_loadingOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 0.6);");
    m_loadingOverlay->setGeometry(rect());
    m_loadingOverlay->setVisible(false);
    m_loadingOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    QVBoxLayout *loadingLayout = new QVBoxLayout(m_loadingOverlay);
    loadingLayout->setAlignment(Qt::AlignCenter);
    loadingLayout->setSpacing(16);

    m_loadingIcon = new QLabel(m_loadingOverlay);
    m_loadingIcon->setFixedSize(64, 64);
    loadingLayout->addWidget(m_loadingIcon, 0, Qt::AlignCenter);

    m_loadingText = new QLabel("正在执行操作...", m_loadingOverlay);
    m_loadingText->setStyleSheet("color: white; font-size: 14px;");
    m_loadingText->setAlignment(Qt::AlignCenter);
    loadingLayout->addWidget(m_loadingText);

    m_progressBar = new QProgressBar(m_loadingOverlay);
    m_progressBar->setFixedWidth(200);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 1px solid #3ddbff;"
        "    border-radius: 4px;"
        "    background-color: rgba(255, 255, 255, 0.1);"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #3ddbff;"
        "    border-radius: 4px;"
        "}"
    );
    m_progressBar->setVisible(false);
    loadingLayout->addWidget(m_progressBar);

    m_stepIndicator = new QLabel("", m_loadingOverlay);
    m_stepIndicator->setStyleSheet("color: #888888; font-size: 12px;");
    m_stepIndicator->setAlignment(Qt::AlignCenter);
    loadingLayout->addWidget(m_stepIndicator);

    m_loadingTimer.reset(new QTimer(this));
    m_loadingAngle = 0;
    connect(m_loadingTimer.data(), &QTimer::timeout, this, [this]() {
        m_loadingAngle += 15;
        if (m_loadingAngle >= 360) {
            m_loadingAngle = 0;
        }
        updateLoadingIcon();
    });
    m_loadingTimer->setInterval(30);

    m_loadingOverlay->raise();

    //初始化网络配置监控定时器（每30秒检查一次）
    m_networkMonitorTimer.reset(new QTimer(this));
    connect(m_networkMonitorTimer.data(), &QTimer::timeout, this, &MainWindow::checkNetworkChanges);
    m_networkMonitorTimer->setInterval(30000);
    m_networkMonitorTimer->start();

    Logger::info("UI初始化完成");
}

void MainWindow::addHistoryItem(const QString &configName, ConfigResult result, const QString &message)
{
    addHistoryItem(configName, result, message, QVariantMap(), QVariantMap());
}

void MainWindow::addHistoryItem(const QString &configName, ConfigResult result, const QString &message,
                                const QVariantMap &beforeConfig, const QVariantMap &afterConfig)
{
    HistoryItem item;
    item.timestamp = QDateTime::currentDateTime();
    item.configName = configName;
    item.result = result;
    item.message = message;
    item.beforeConfig = beforeConfig;
    item.afterConfig = afterConfig;

    m_configHistory.prepend(item);

    // 只保留最近20条记录
    if (m_configHistory.size() > 20)
    {
        m_configHistory.removeLast();
    }

    updateHistoryDisplay();

    // 写入日志
    Logger::info(tr("配置历史: %1 [%2] %3").arg(
        item.timestamp.toString("yyyy-MM-dd HH:mm:ss"),
        configName,
        message
    ));
}

QString MainWindow::formatConfigDiff(const QVariantMap &before, const QVariantMap &after)
{
    if (before.isEmpty() && after.isEmpty()) {
        return QString();
    }

    QString diffText;
    QStringList keys = before.keys();
    keys.append(after.keys());
    keys.removeDuplicates();

    for (const QString &key : keys) {
        QString beforeValue = before.value(key).toString();
        QString afterValue = after.value(key).toString();

        if (beforeValue != afterValue) {
            if (beforeValue.isEmpty()) {
                diffText += QString("+ %1: %2\n").arg(key).arg(afterValue);
            } else if (afterValue.isEmpty()) {
                diffText += QString("- %1: %2\n").arg(key).arg(beforeValue);
            } else {
                diffText += QString("~ %1: %2 -> %3\n").arg(key).arg(beforeValue).arg(afterValue);
            }
        }
    }

    return diffText.trimmed();
}

void MainWindow::updateHistoryDisplay()
{
    if (!m_historyListWidget)
        return;

    m_historyListWidget->clear();
    for (const HistoryItem &item : m_configHistory)
    {
        QString statusText;
        QColor statusColor;
        switch (item.result)
        {
        case ConfigResult::Success:
            statusText = "✓ 成功";
            statusColor = QColor(0, 180, 0);
            break;
        case ConfigResult::Failure:
            statusText = "✗ 失败";
            statusColor = QColor(180, 0, 0);
            break;
        case ConfigResult::Unchanged:
            statusText = "⊙ 未变更";
            statusColor = QColor(128, 128, 128);
            break;
        }

        QString diffText = formatConfigDiff(item.beforeConfig, item.afterConfig);
        
        QString itemText = QString("[%1] %2 - %3\n  %4").arg(
            item.timestamp.toString("HH:mm:ss"),
            statusText,
            item.configName,
            item.message
        );

        if (!diffText.isEmpty()) {
            itemText += "\n  --- 变更详情 ---\n" + diffText;
        }

        QListWidgetItem *listItem = new QListWidgetItem(itemText);
        listItem->setForeground(statusColor);
        listItem->setToolTip(itemText);
        m_historyListWidget->addItem(listItem);
    }
}

void MainWindow::showConfigResult(ConfigResult result, const QString& message)
{
    QPixmap originalPixmap = m_floatWindow->getBackgroundPixmap();
    QString bgImage;
    QIcon trayIcon;
    QSystemTrayIcon::MessageIcon trayMsgIcon;

    switch(result)
    {
    case ConfigResult::Success:
        bgImage = ":/images/images/indicator_green.png";
        trayIcon = QIcon(":/images/images/icon_green.png");
        trayMsgIcon = QSystemTrayIcon::Information;
        break;
    case ConfigResult::Failure:
        bgImage = ":/images/images/indicator_red.png";
        trayIcon = QIcon(":/images/images/icon_red.png");
        trayMsgIcon = QSystemTrayIcon::Critical;
        break;
    case ConfigResult::Unchanged:
    default:
        bgImage = ":/images/images/indicator_gray.png";
        trayIcon = QIcon(":/images/images/icon.png");
        trayMsgIcon = QSystemTrayIcon::NoIcon;
        break;
    }

    m_floatWindow->clearBackgroundPixmap();
    m_floatWindow->setBackgroundPixmap(QPixmap(bgImage));
    QPixmap indicatorPixmap(bgImage);
    m_statusIndicator->setPixmap(indicatorPixmap.scaled(16, 16, Qt::KeepAspectRatio));
    QPropertyAnimation* animation = new QPropertyAnimation(m_floatWindow, "windowOpacity");
    animation->setDuration(300);
    animation->setKeyValueAt(0, 1.0);
    animation->setKeyValueAt(0.5, 0.5);
    animation->setKeyValueAt(1, 1.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    ui->statusBar->showMessage(message, 3000);
    if(m_trayIcon)
    {
        m_trayIcon->setIcon(trayIcon);
        if(m_trayIcon->isVisible() && m_trayIcon->supportsMessages())
        {
            m_trayIcon->showMessage(tr("网络配置"), message, trayMsgIcon, 3000);
        }
    }

    // 添加历史记录
    addHistoryItem(m_currentConfig, result, message);
    QTimer::singleShot(1500, this, [this, originalPixmap]()
    {
        m_floatWindow->clearBackgroundPixmap();
        m_floatWindow->setBackgroundPixmap(originalPixmap);
        m_statusIndicator->setPixmap(m_defaultIndicator);
        if(m_trayIcon)
        {
            m_trayIcon->setIcon(QIcon(":/images/images/icon.png"));
        }
    });
}

void MainWindow::updateLoadingIcon()
{
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(32, 32);
    painter.rotate(m_loadingAngle);

    //绘制旋转圆环
    int radius = 24;
    QRectF ringRect(-radius, -radius, radius * 2, radius * 2);
    QPen pen(QColor(61, 219, 255), 4);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(ringRect, 90 * 16, -m_loadingAngle * 16);

    //绘制中心圆点
    painter.setBrush(QColor(61, 219, 255));
    painter.drawEllipse(-4, -4, 8, 8);

    m_loadingIcon->setPixmap(pixmap);
}

void MainWindow::showLoading(const QString &text)
{
    showLoading(text, -1, -1, -1);
}

void MainWindow::showLoading(const QString &text, int progress, int currentStep, int totalSteps)
{
    if (m_loadingOverlay && m_loadingText) {
        m_loadingText->setText(text);
        m_loadingAngle = 0;
        updateLoadingIcon();
        m_loadingOverlay->setGeometry(rect());
        m_loadingOverlay->setVisible(true);
        
        if (progress >= 0 && m_progressBar) {
            m_progressBar->setVisible(true);
            m_progressBar->setValue(progress);
        } else if (m_progressBar) {
            m_progressBar->setVisible(false);
        }
        
        if (currentStep > 0 && totalSteps > 0 && m_stepIndicator) {
            m_stepIndicator->setVisible(true);
            m_stepIndicator->setText(QString("步骤 %1 / %2").arg(currentStep).arg(totalSteps));
        } else if (m_stepIndicator) {
            m_stepIndicator->setVisible(false);
        }
        
        if (!m_loadingTimer.isNull()) {
            m_loadingTimer->start();
        }

        if (m_floatWindow) {
            m_floatWindow->startLoading();
        }
    }
}

void MainWindow::hideLoading()
{
    if (m_loadingOverlay) {
        m_loadingOverlay->setVisible(false);
        if (!m_loadingTimer.isNull()) {
            m_loadingTimer->stop();
        }
    }

    if (m_floatWindow) {
        m_floatWindow->stopLoading();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (m_loadingOverlay) {
        m_loadingOverlay->setGeometry(rect());
    }
}

/**
 * @brief 建立信号槽连接
 *
 * 连接所有UI组件和业务逻辑的信号槽
 */
void MainWindow::setupConnections()
{
    Logger::debug("开始建立信号槽连接");
    //配置列表
    connect(ui->configList, &QListWidget::itemClicked, this, &MainWindow::onConfigSelected);
    //网络接口
    connect(ui->interfaceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onInterfaceChanged);
    //IP方法
    connect(ui->dhcpRadio, &QRadioButton::toggled, this, &MainWindow::onIpMethodToggled);
    //自定义DNS复选框
    connect(ui->customDnsCheckBox, &QCheckBox::toggled, this, &MainWindow::onCustomDnsToggled);
    //按钮
    connect(ui->addButton, &QPushButton::clicked, this, &MainWindow::onAddConfig);
    connect(ui->updateButton, &QPushButton::clicked, this, &MainWindow::onUpdateConfig);
    connect(ui->deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteConfig);
    connect(ui->applyButton, &QPushButton::clicked, this, &MainWindow::onApplyConfig);
    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::updateInterfaces);
    //网卡管理信号槽
    connect(ui->enableInterfaceBtn, &QPushButton::clicked, this, static_cast<void (MainWindow::*)()>(&MainWindow::onEnableInterface));
    connect(ui->disableInterfaceBtn, &QPushButton::clicked, this, static_cast<void (MainWindow::*)()>(&MainWindow::onDisableInterface));
    connect(ui->refreshInterfacesBtn, &QPushButton::clicked, this, &MainWindow::refreshNetworkInterfaces);
    //实时输入验证
    connect(ui->ipEdit, &QLineEdit::textChanged, this, &MainWindow::onInputTextChanged);
    connect(ui->subnetEdit, &QLineEdit::textChanged, this, &MainWindow::onInputTextChanged);
    connect(ui->gatewayEdit, &QLineEdit::textChanged, this, &MainWindow::onInputTextChanged);
    connect(ui->primaryDnsEdit, &QLineEdit::textChanged, this, &MainWindow::onInputTextChanged);
    connect(ui->secondaryDnsEdit, &QLineEdit::textChanged, this, &MainWindow::onInputTextChanged);
    //导入导出
    connect(ui->importButton, &QPushButton::clicked, this, &MainWindow::onImportConfig);
    connect(ui->exportButton, &QPushButton::clicked, this, &MainWindow::onExportConfig);
    //键盘快捷键
    QShortcut *shortcutAdd = new QShortcut(QKeySequence(tr("Ctrl+N")), this);
    connect(shortcutAdd, &QShortcut::activated, this, &MainWindow::onAddConfig);
    QShortcut *shortcutApply = new QShortcut(QKeySequence(tr("Ctrl+Return")), this);
    connect(shortcutApply, &QShortcut::activated, this, &MainWindow::onApplyConfig);
    QShortcut *shortcutDelete = new QShortcut(QKeySequence(tr("Ctrl+D")), this);
    connect(shortcutDelete, &QShortcut::activated, this, &MainWindow::onDeleteConfig);
    QShortcut *shortcutRefresh = new QShortcut(QKeySequence(tr("Ctrl+R")), this);
    connect(shortcutRefresh, &QShortcut::activated, this, &MainWindow::updateInterfaces);
    QShortcut *shortcutHide = new QShortcut(QKeySequence(tr("Esc")), this);
    connect(shortcutHide, &QShortcut::activated, this, &MainWindow::hide);
    //配置管理器的信号
    connect(m_configManager, &ConfigManager::configApplied, this, [this](bool success, const QString & message)
    {
        if(success)
        {
            Logger::info(tr("%1").arg(message));
            ui->statusBar->showMessage(tr("%1").arg(message), 2000);
            updateQuickMenu();
            m_floatWindow->update();
        }
        else
        {
            Logger::error(tr("配置应用失败: %1").arg(message));
            QMessageBox::warning(this, "应用失败", message);
        }
    });
    connect(m_configManager, &ConfigManager::errorOccurred, this, [this](const QString & error)
    {
        Logger::error(tr("配置管理器错误: %1").arg(error));
        QMessageBox::critical(this, "错误", error);
    });
    connect(m_configManager, &ConfigManager::adminStatusChanged, this, [this](bool isAdmin)
    {
        ui->applyButton->setEnabled(isAdmin);
        ui->addButton->setEnabled(isAdmin);
        ui->updateButton->setEnabled(isAdmin);
        ui->deleteButton->setEnabled(isAdmin);
        if(isAdmin)
        {
            Logger::info("已获取管理员权限");
            ui->statusBar->showMessage("已获取管理员权限", 2000);
        }
        else
        {
            Logger::warning("管理员权限已丢失");
        }
    });
    //浮动窗口
    connect(m_floatWindow, &FloatWindow::doubleClicked, this, &MainWindow::showNormal);
    connect(m_floatWindow, &FloatWindow::showContextMenu, this, &MainWindow::showFloatWindowMenu);
    //在初始化悬浮窗时连接信号
    connect(m_floatWindow, &QWidget::windowTitleChanged, [this]()
    {
        saveFloatWindowPosition();
    });
    //在MainWindow构造函数中
    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::saveFloatWindowPosition);
    Logger::info("信号槽连接建立完成");
}

/**
 * @brief 初始化应用程序
 *
 * 加载配置，更新网络接口，检查开机自启动设置
 */
void MainWindow::initializeApplication()
{
    Logger::info("开始初始化应用程序");
    //加载配置
    if(!m_configManager->loadConfigs())
    {
        Logger::warning("加载配置文件失败，将使用空配置");
        QMessageBox::warning(this, "警告", "加载配置文件失败，将使用空配置");
    }
    else
    {
        Logger::info("配置文件加载成功");
    }
    //更新网络接口
    Logger::debug("更新网络接口列表");
    updateInterfaces();
    //更新所有网络接口及状态
    refreshNetworkInterfaces();
    //检查开机启动
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    m_autostart = settings.contains(APP_NAME);
    Logger::info(tr("开机自启动状态: %1").arg(m_autostart ? "已启用" : "已禁用"));
    //初始化浮动窗口
    QSettings windowSettings;
    QPoint floatWindowPos = windowSettings.value("floatWindowPos", QPoint(100, 100)).toPoint();
    m_floatWindow->move(floatWindowPos);
    m_floatVisible = windowSettings.value("floatWindowVisible", true).toBool();
    if(m_floatVisible)
    {
        Logger::debug("显示浮动窗口");
        m_floatWindow->show();
    }
    Logger::info("应用程序初始化完成");
}

/**
 * @brief 恢复窗口状态
 *
 * 从注册表/QSettings中恢复窗口几何状态和位置
 */
void MainWindow::restoreWindowState()
{
    Logger::debug("恢复窗口状态");
    QSettings settings;
    restoreGeometry(settings.value("windowGeometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    m_floatVisible = settings.value("floatWindowVisible", true).toBool();
    Logger::info("窗口状态已恢复");
}

/**
 * @brief 保存窗口状态
 *
 * 将窗口几何状态和位置保存到注册表/QSettings
 */
void MainWindow::saveWindowState()
{
    Logger::debug("保存窗口状态");
    QSettings settings;
    settings.setValue("windowGeometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("floatWindowVisible", m_floatVisible);
    settings.setValue("floatWindowPos", m_floatWindow->pos());
    Logger::info("窗口状态已保存");
}

/**
 * @brief 显示管理员权限警告
 *
 * 当需要管理员权限时提示用户，并提供获取权限的选项
 */
void MainWindow::showAdminWarning()
{
    Logger::warning("检测到需要管理员权限");
    int ret = QMessageBox::question(this, "权限提示",
                                    "需要管理员权限才能修改网络配置。\n"
                                    "是否立即获取权限？(否则部分功能将受限)",
                                    QMessageBox::Yes | QMessageBox::No);
    if(ret == QMessageBox::Yes)
    {
        Logger::info("用户选择获取管理员权限");
        if(!m_configManager->requestAdminPrivileges())
        {
            Logger::error("获取管理员权限失败");
            QMessageBox::warning(this, "警告", "获取管理员权限失败，部分功能将受限");
            disableAdminFunctions();
        }
    }
    else
    {
        Logger::info("用户选择不获取管理员权限");
        disableAdminFunctions();
    }
}

/**
 * @brief 禁用管理员功能
 *
 * 当没有管理员权限时，禁用相关功能按钮
 */
void MainWindow::disableAdminFunctions()
{
    Logger::warning("禁用管理员功能");
    ui->applyButton->setEnabled(false);
    ui->addButton->setEnabled(false);
    ui->updateButton->setEnabled(false);
    ui->deleteButton->setEnabled(false);
    ui->statusBar->showMessage("无管理员权限，部分功能受限", 2000);
}

/**
 * @brief 处理配置项选择
 * @param item 被选择的列表项
 *
 * 当用户选择配置列表中的项时，加载对应配置到表单
 */
void MainWindow::onConfigSelected(QListWidgetItem *item)
{
    if(!item)
    {
        Logger::debug("未选择有效配置项");
        return;
    }
    QString name = item->text();
    m_currentConfig = name;
    Logger::debug(QString("选择配置: %1").arg(name));
    QVariantMap config = m_configManager->configs().value(name);
    if(config.isEmpty())
    {
        Logger::warning(tr("无效配置: %1").arg(name));
        ui->statusBar->showMessage("无效配置: " + name, 2000);
        return;
    }
    //更新界面显示
    int index = ui->interfaceCombo->findText(config["interface"].toString());
    if(index >= 0)
    {
        ui->interfaceCombo->setCurrentIndex(index);
    }
    if(config["method"].toString() == "dhcp")
    {
        ui->dhcpRadio->setChecked(true);
        Logger::debug("配置使用DHCP模式");
    }
    else
    {
        ui->staticRadio->setChecked(true);
        Logger::debug("配置使用静态IP模式");
    }
    ui->customDnsCheckBox->setChecked(config["custom_dns"].toBool());
    ui->ipEdit->setText(config["ip"].toString());
    ui->subnetEdit->setText(config["subnet"].toString());
    ui->gatewayEdit->setText(config["gateway"].toString());
    ui->primaryDnsEdit->setText(config["primary_dns"].toString());
    ui->secondaryDnsEdit->setText(config["secondary_dns"].toString());
    ui->statusBar->showMessage("已选择配置: " + name, 2000);
    Logger::info(tr("已加载配置: %1").arg(name));
}

/**
 * @brief 处理网络接口变更
 * @param index 新选择的接口索引
 *
 * 当用户选择不同的网络接口时更新当前接口
 */
void MainWindow::onInterfaceChanged(int index)
{
    if(index >= 0)
    {
        m_currentInterface = ui->interfaceCombo->itemText(index);
        Logger::debug(tr("选择网络接口: %1").arg(m_currentInterface));
        ui->statusBar->showMessage("已选择接口: " + m_currentInterface, 2000);
    }
    else
    {
        Logger::warning("无效的网络接口索引");
    }
}

/**
 * @brief 更新网络接口列表
 *
 * 从系统获取可用网络接口并更新下拉框
 */
void MainWindow::updateInterfaces()
{
    Logger::debug("更新网络接口列表");
    ui->interfaceCombo->clear();
    NetworkInterfaceManager manager;
    QStringList interfaces = manager.getNetworkInterfaces();
    if(interfaces.isEmpty())
    {
        Logger::warning("未找到可用的网络接口");
    }
    else
    {
        Logger::info(tr("找到 %1 个网络接口").arg(interfaces.size()));
    }
    ui->interfaceCombo->addItems(interfaces);
    if(!interfaces.isEmpty())
    {
        updateConfigList();
    }
    ui->statusBar->showMessage("网络接口列表已更新", 2000);
    Logger::info("网络接口列表更新完成");
}

/**
 * @brief 处理IP方法切换
 * @param checked 是否选中DHCP模式
 *
 * 根据选择的IP方法(DHCP/静态IP)启用/禁用相关输入框
 */
void MainWindow::onIpMethodToggled(bool checked)
{
    Q_UNUSED(checked);
    bool isDhcp = ui->dhcpRadio->isChecked();
    Logger::debug(tr("IP方法切换: %1").arg(isDhcp ? "DHCP" : "静态IP"));
    ui->ipEdit->setEnabled(!isDhcp);
    ui->subnetEdit->setEnabled(!isDhcp);
    ui->gatewayEdit->setEnabled(!isDhcp);
    ui->customDnsCheckBox->setEnabled(isDhcp);
    bool useCustomDns = isDhcp && ui->customDnsCheckBox->isChecked();
    ui->primaryDnsEdit->setEnabled(!isDhcp || useCustomDns);
    ui->secondaryDnsEdit->setEnabled(!isDhcp || useCustomDns);
}

/**
 * @brief 处理自定义DNS复选框切换
 * @param checked 是否使用自定义DNS
 */
void MainWindow::onCustomDnsToggled(bool checked)
{
    Q_UNUSED(checked);
    bool isDhcp = ui->dhcpRadio->isChecked();
    bool useCustomDns = isDhcp && ui->customDnsCheckBox->isChecked();
    ui->primaryDnsEdit->setEnabled(!isDhcp || useCustomDns);
    ui->secondaryDnsEdit->setEnabled(!isDhcp || useCustomDns);
}

/**
 * @brief 添加新配置
 *
 * 获取用户输入并添加新的网络配置
 */
void MainWindow::onAddConfig()
{
    Logger::info("开始添加新配置");
    bool ok;
    QString name = QInputDialog::getText(this, "添加配置", "请输入配置名称:",
                                         QLineEdit::Normal, "", &ok);
    if(!ok || name.isEmpty())
    {
        Logger::debug("用户取消添加配置或输入为空");
        return;
    }
    if(m_configManager->configs().contains(name))
    {
        Logger::warning(tr("配置名称已存在: %1").arg(name));
        QMessageBox::critical(this, "错误", "该配置名称已存在！");
        return;
    }
    if(ui->interfaceCombo->currentIndex() < 0)
    {
        Logger::warning("未选择网络接口");
        QMessageBox::critical(this, "错误", "请先选择网络接口！");
        return;
    }
    QString currentInterface = ui->interfaceCombo->currentText();
    QString displayName = QString("[%1] %2").arg(currentInterface.split(' ').first()).arg(name);
    QVariantMap config = getCurrentFormConfig();
    if(!m_configManager->validateConfig(config))
    {
        Logger::warning("IP配置验证失败");
        return;
    }
    if(m_configManager->addConfig(displayName, config))
    {
        if(m_configManager->saveConfigs())
        {
            Logger::info(tr("配置添加成功: %1").arg(displayName));
            updateConfigList(); //更新所有配置列表
            m_currentConfig = displayName;
            ui->statusBar->showMessage("已添加配置: " + displayName, 2000);
            QMessageBox::information(this, "成功", tr("配置 '%1' 已添加！").arg(displayName));
        }
        else
        {
            Logger::error("保存配置文件失败");
            QMessageBox::critical(this, "错误", "保存配置失败，请检查文件权限");
        }
    }
    else
    {
        Logger::error("添加配置失败");
    }
}

/**
 * @brief 更新配置
 *
 * 更新当前选中的网络配置
 */
void MainWindow::onUpdateConfig()
{
    Logger::info("开始更新配置");
    if(m_currentConfig.isEmpty())
    {
        Logger::warning("未选择要更新的配置");
        QMessageBox::critical(this, "错误", "请先选择一个配置！");
        return;
    }
    if(ui->interfaceCombo->currentIndex() < 0)
    {
        Logger::warning("未选择网络接口");
        QMessageBox::critical(this, "错误", "请先选择网络接口！");
        return;
    }
    QString currentInterface = ui->interfaceCombo->currentText();
    QString oldName = m_currentConfig;
    //更新配置名称以包含接口前缀
    QString newName;
    if(oldName.contains("] "))
    {
        newName = QString("[%1] %2").arg(currentInterface.split(' ').first())
                  .arg(oldName.split("] ").last());
    }
    else
    {
        newName = QString("[%1] %2").arg(currentInterface.split(' ').first()).arg(oldName);
    }
    QVariantMap config = getCurrentFormConfig();
    if(!m_configManager->validateConfig(config))
    {
        Logger::warning("IP配置验证失败");
        return;
    }
    if(m_configManager->updateConfig(oldName, newName, config))
    {
        if(m_configManager->saveConfigs())
        {
            Logger::info(tr("配置更新成功: %1 -> %2").arg(oldName).arg(newName));
            updateConfigList(); //更新所有配置列表
            m_currentConfig = newName;
            ui->statusBar->showMessage("已更新配置: " + newName, 2000);
            QMessageBox::information(this, "成功", tr("配置 '%1' 已更新！").arg(newName));
        }
        else
        {
            Logger::error("保存配置文件失败");
            QMessageBox::critical(this, "错误", "保存配置失败，请检查文件权限");
        }
    }
    else
    {
        Logger::error("更新配置失败");
    }
}

/**
 * @brief 删除配置
 *
 * 删除当前选中的网络配置
 */
void MainWindow::onDeleteConfig()
{
    Logger::info("开始删除配置");
    if(m_currentConfig.isEmpty())
    {
        Logger::warning("未选择要删除的配置");
        QMessageBox::critical(this, "错误", "请先选择一个配置！");
        return;
    }
    if(QMessageBox::question(this, "确认",
                             tr("确定要删除配置 '%1' 吗？").arg(m_currentConfig))
            == QMessageBox::Yes)
    {
        Logger::debug(tr("用户确认删除配置: %1").arg(m_currentConfig));
        QString currentInterface = m_configManager->configs().value(m_currentConfig)["interface"].toString();
        if(m_configManager->removeConfig(m_currentConfig))
        {
            if(m_configManager->saveConfigs())
            {
                Logger::info(tr("配置删除成功: %1").arg(m_currentConfig));
                m_currentConfig.clear();
                updateConfigList(); //更新所有配置列表
                clearFields();
                ui->statusBar->showMessage("配置已删除", 2000);
                QMessageBox::information(this, "成功", "配置已删除！");
            }
            else
            {
                Logger::error("保存配置文件失败");
                QMessageBox::critical(this, "错误", "保存配置失败，请检查文件权限");
            }
        }
        else
        {
            Logger::error("删除配置失败");
        }
    }
    else
    {
        Logger::debug("用户取消删除配置");
    }
}

void MainWindow::onEnableCompleted(bool success, const QString &interface, const QString &message)
{
    hideLoading();
    if (success)
    {
        showConfigResult(ConfigResult::Success, message);
    }
    else
    {
        showConfigResult(ConfigResult::Failure, message);
    }
    refreshNetworkInterfaces();
}

void MainWindow::onDisableCompleted(bool success, const QString &interface, const QString &message)
{
    hideLoading();
    if (success)
    {
        showConfigResult(ConfigResult::Success, message);
    }
    else
    {
        showConfigResult(ConfigResult::Failure, message);
    }
    refreshNetworkInterfaces();
}

/**
 * @brief 应用配置
 *
 * 将当前选中的网络配置应用到系统
 */
void MainWindow::onApplyConfig()
{
    Logger::info("开始应用配置");
    try
    {
        if(m_currentConfig.isEmpty())
        {
            Logger::warning("未选择要应用的配置");
            showConfigResult(ConfigResult::Failure, "请先选择一个配置");
            return;
        }
        QVariantMap config = m_configManager->configs().value(m_currentConfig);
        if(config.isEmpty())
        {
            Logger::error("无效的配置");
            showConfigResult(ConfigResult::Failure, "无效的配置");
            return;
        }
        showLoading("正在应用配置...");

        QThread *thread = QThread::create([this, config]() {
            QString interfaceName = config["interface"].toString();
            QString adminStatus = NetworkInterfaceManager::getInterfaceAdminStatus(interfaceName);
            if(adminStatus == "已禁用")
            {
                Logger::warning(tr("网卡 %1 已被禁用，无法应用配置").arg(interfaceName));
                QMetaObject::invokeMethod(this, [this, interfaceName]() {
                    hideLoading();
                    showConfigResult(ConfigResult::Failure, tr("网卡 %1 已被禁用").arg(interfaceName));
                });
                return;
            }
            if(m_networkInfoCollector->compareConfigs(m_networkInfoCollector->getCurrentNetworkConfig(interfaceName, m_configManager), config))
            {
                Logger::info("配置未变更，跳过应用");
                QMetaObject::invokeMethod(this, [this]() {
                    hideLoading();
                    showConfigResult(ConfigResult::Unchanged, "配置未变更");
                });
                return;
            }
            Logger::debug(tr("应用配置: %1").arg(config["interface"].toString()));

            bool success = m_configManager->applyConfig(config);

            QMetaObject::invokeMethod(this, [this, success]() {
                hideLoading();

                if(success)
                {
                    Logger::info("配置应用成功");
                    showConfigResult(ConfigResult::Success, "配置应用成功");
                    QTimer::singleShot(500, this, [this]()
                    {
                        updateQuickMenu();
                        m_floatWindow->update();
                    });
                }
                else
                {
                    Logger::error("应用配置失败");
                    showConfigResult(ConfigResult::Failure, "应用配置失败");
                }
            });
        });

        connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        thread->start();
    }
    catch(const std::exception& e)
    {
        hideLoading();
        Logger::critical(QString("配置应用异常: %1").arg(e.what()));
        showConfigResult(ConfigResult::Failure, QString("配置应用过程中发生异常: %1").arg(e.what()));
    }
    catch(...)
    {
        hideLoading();
        Logger::critical("未知的配置应用异常");
        showConfigResult(ConfigResult::Failure, "配置应用过程中发生未知异常");
    }
}

/**
 * @brief 获取当前表单配置
 * @return 包含当前表单数据的QVariantMap
 *
 * 从UI表单中收集当前配置数据
 */
QVariantMap MainWindow::getCurrentFormConfig() const
{
    const auto optionalAddress = [](QLineEdit *edit) {
        const QString value = edit->text().trimmed();
        return value == QStringLiteral("...") ? QString() : value;
    };
    QVariantMap config;
    config["interface"] = ui->interfaceCombo->currentText();
    config["method"] = ui->dhcpRadio->isChecked() ? "dhcp" : "static";
    config["custom_dns"] = ui->customDnsCheckBox->isChecked();
    config["ip"] = ui->ipEdit->text();
    config["subnet"] = ui->subnetEdit->text();
    config["gateway"] = optionalAddress(ui->gatewayEdit);
    config["primary_dns"] = optionalAddress(ui->primaryDnsEdit);
    config["secondary_dns"] = optionalAddress(ui->secondaryDnsEdit);
    Logger::debug(tr("获取当前表单配置: 方法=%1, IP=%2").arg(config["method"].toString()).arg(config["ip"].toString()));
    return config;
}

/**
 * @brief 更新配置列表
 *
 * 从配置管理器中获取所有配置并更新UI中的列表显示
 */
void MainWindow::updateConfigList()
{
    Logger::debug("更新所有配置列表");
    ui->configList->clear();
    QMap<QString, QVariantMap> configs = m_configManager->configs();
    //遍历所有配置并添加到列表中
    for(auto it = configs.begin(); it != configs.end(); ++it)
    {
        ui->configList->addItem(it.key());
    }
    Logger::info(tr("显示 %1 个配置").arg(configs.size()));
    //更新快捷菜单
    updateQuickMenu();
}

/**
 * @brief 清空表单字段
 *
 * 重置所有输入字段到初始状态
 */
void MainWindow::clearFields()
{
    Logger::debug("清空表单字段");
    ui->ipEdit->clear();
    ui->subnetEdit->clear();
    ui->gatewayEdit->clear();
    ui->primaryDnsEdit->clear();
    ui->secondaryDnsEdit->clear();
    ui->dhcpRadio->setChecked(true);
}

/**
 * @brief 初始化系统托盘图标
 *
 * 创建系统托盘图标及其上下文菜单
 */
void MainWindow::setupTrayIcon()
{
    Logger::info("初始化系统托盘图标");
    m_trayIcon->setIcon(QIcon(":/images/images/icon.png"));
    m_trayIcon->setToolTip(MAIN_WINDOW_TITLE);
    //先确保创建菜单对象
    if(!m_quickMenu)
    {
        Logger::debug("创建快速应用配置菜单");
        m_quickMenu = new QMenu("快速应用配置", this);
    }
    QMenu *trayMenu = new QMenu(this);
    //添加"显示主窗口"动作
    QAction *showAction = trayMenu->addAction(tr("显示主窗口"));
    connect(showAction, &QAction::triggered, this, &MainWindow::showNormal);
    //添加"显示/隐藏悬浮球"动作
    QAction *floatAction = trayMenu->addAction(tr("显示/隐藏悬浮球"));
    floatAction->setCheckable(true);
    floatAction->setChecked(m_floatVisible);
    connect(floatAction, &QAction::toggled, this, &MainWindow::toggleFloatWindow);
    //初始化快速配置子菜单
    m_quickMenu = trayMenu->addMenu("快速应用配置");
    updateQuickMenu();
    //添加网卡管理子菜单
    QMenu *interfaceMenu = trayMenu->addMenu("网卡管理");
    //获取所有网络接口（一次调用获取全部状态，避免每个接口单独调用netsh）
    QList<InterfaceDetail> allDetails = NetworkInterfaceManager::getAllInterfaceDetails();
    for(const InterfaceDetail &detail : allDetails)
    {
        if(detail.name.isEmpty()) continue;
        const QString rawInterface = detail.name;
        const QString cleanInterface = m_configManager->cleanInterfaceName(rawInterface);
        QMenu *ifaceMenu = interfaceMenu->addMenu(cleanInterface);
        QString adminStatus = detail.adminStatus;
        QAction *enableAction = ifaceMenu->addAction(tr("启用网卡"));
        enableAction->setCheckable(true);
        enableAction->setChecked(adminStatus == tr("已启用"));
        connect(enableAction, &QAction::triggered, this, [this, rawInterface]()
        {
            onEnableInterface(rawInterface);
        });
        QAction *disableAction = ifaceMenu->addAction(tr("禁用网卡"));
        disableAction->setCheckable(true);
        disableAction->setChecked(adminStatus == "已禁用");
        connect(disableAction, &QAction::triggered, this, [this, rawInterface]()
        {
            onDisableInterface(rawInterface);
        });
    }
    //添加"主题切换"子菜单
    QMenu *themeMenu = trayMenu->addMenu("主题切换");
    QAction *darkThemeAction = themeMenu->addAction(tr("暗色主题"));
    connect(darkThemeAction, &QAction::triggered, this, [this]()
    {
        loadAndApplyStyleSheet(":/styles/styles/drak_theme.qss");
    });
    QAction *lightThemeAction = themeMenu->addAction(tr("亮色主题"));
    connect(lightThemeAction, &QAction::triggered, this, [this]()
    {
        // 尝试从资源加载亮色主题，否则从文件加载
        loadAndApplyStyleSheet(":/styles/styles/light_theme.qss");
    });
    themeMenu->addSeparator();
    //添加"开机自启动"动作
    QAction *autostartAction = trayMenu->addAction(tr("开机自启动"));
    autostartAction->setCheckable(true);
    autostartAction->setChecked(m_autostart);
    connect(autostartAction, &QAction::toggled, this, &MainWindow::toggleAutostart);
    //添加"导出日志"动作
    QAction *exportLogAction = trayMenu->addAction(tr("导出日志"));
    connect(exportLogAction, &QAction::triggered, this, &MainWindow::onExportLogs);
    trayMenu->addSeparator();
    //添加"退出"动作
    QAction *quitAction = trayMenu->addAction(tr("退出"));
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    m_trayIcon->setContextMenu(trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    m_trayIcon->show();
    Logger::info("系统托盘图标初始化完成");
}

/**
 * @brief 切换悬浮窗显示状态
 * @param visible 是否显示悬浮窗
 *
 * 根据参数显示或隐藏悬浮窗，并保存状态到设置
 */
void MainWindow::toggleFloatWindow(bool visible)
{
    Logger::info(tr("设置悬浮窗可见性: %1").arg(visible ? "显示" : "隐藏"));
    m_floatVisible = visible;
    QSettings().setValue("floatWindowVisible", m_floatVisible);
    if(m_floatVisible)
    {
        m_floatWindow->show();
    }
    else
    {
        m_floatWindow->hide();
    }
}

/**
 * @brief 切换开机自启动设置
 * @param enabled 是否启用开机自启动
 *
 * 修改注册表实现开机自启动功能
 */
void MainWindow::toggleAutostart(bool enabled)
{
    Logger::info(tr("设置开机自启动: %1").arg(enabled ? "启用" : "禁用"));
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if(enabled)
    {
        //获取应用程序路径并添加最小化启动参数
        QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        settings.setValue(APP_NAME, tr("\"%1\" --minimized").arg(appPath));
    }
    else
    {
        settings.remove(APP_NAME);
    }
    m_autostart = enabled;
    ui->statusBar->showMessage(enabled ? "已启用开机启动" : "已禁用开机启动", 2000);
}

/**
 * @brief 实时验证输入
 *
 * 当IP输入框内容变化时验证输入并改变边框颜色
 */
void MainWindow::onInputTextChanged()
{
    QRegularExpression ipRegex(R"(^(\d{1,3}\.){3}\d{1,3}$)");
    QList<QLineEdit*> ipEdits = {ui->ipEdit, ui->subnetEdit, ui->gatewayEdit, ui->primaryDnsEdit, ui->secondaryDnsEdit};

    for(QLineEdit* edit : ipEdits)
    {
        if(edit->isEnabled())
        {
            QString text = edit->text().trimmed();
            if(text.isEmpty() || text == QStringLiteral("..."))
            {
                edit->setStyleSheet(""); // 恢复默认
                continue;
            }

            QRegularExpressionMatch match = ipRegex.match(text);
            if(match.hasMatch())
            {
                // 简单的颜色边框
                edit->setStyleSheet("QLineEdit { border: 2px solid #2ecc71; }");
            }
            else
            {
                // 红色边框
                edit->setStyleSheet("QLineEdit { border: 2px solid #e74c3c; }");
            }
        }
    }
}

/**
 * @brief 导入配置
 *
 * 从JSON文件导入配置到配置管理器
 */
void MainWindow::onImportConfig()
{
    Logger::info("开始导入配置");
    QString filePath = QFileDialog::getOpenFileName(this, "选择配置文件", "", "JSON Files (*.json);;All Files (*)");
    if(filePath.isEmpty())
    {
        Logger::debug("用户取消导入");
        return;
    }

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        Logger::error("无法打开导入文件: " + filePath);
        QMessageBox::critical(this, "错误", "无法打开文件！");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if(doc.isNull() || !doc.isObject())
    {
        Logger::error("无效的JSON文件: " + filePath);
        QMessageBox::critical(this, "错误", "无效的配置文件！");
        return;
    }

    QJsonObject json = doc.object();
    int importedCount = 0;
    int skippedCount = 0;

    for(auto it = json.begin(); it != json.end(); ++it)
    {
        QString key = it.key();
        QVariantMap config = it.value().toObject().toVariantMap();

        if(m_configManager->configs().contains(key))
        {
            skippedCount++;
            continue;
        }
        else
        {
            m_configManager->addConfig(key, config);
            importedCount++;
        }
    }

    if(m_configManager->saveConfigs())
    {
        updateConfigList();
        QString msg = tr("导入成功！已导入 %1 个配置").arg(importedCount);
        if(skippedCount > 0)
        {
            msg += tr("，%1 个已存在的配置被跳过").arg(skippedCount);
        }
        Logger::info(msg);
        ui->statusBar->showMessage(msg, 3000);
        QMessageBox::information(this, "成功", msg);
    }
}

/**
 * @brief 导出配置
 *
 * 将当前配置管理器中的配置导出到JSON文件
 */
void MainWindow::onExportConfig()
{
    Logger::info("开始导出配置");
    QString filePath = QFileDialog::getSaveFileName(this, "保存配置文件", "", "JSON Files (*.json);;All Files (*)");
    if(filePath.isEmpty())
    {
        Logger::debug("用户取消导出");
        return;
    }

    if(!filePath.endsWith(".json", Qt::CaseInsensitive))
    {
        filePath += ".json";
    }

    QJsonDocument doc = QJsonDocument::fromVariant(QVariant::fromValue(m_configManager->configs()));
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        Logger::error("无法打开导出文件: " + filePath);
        QMessageBox::critical(this, "错误", "无法保存文件！");
        return;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    QString msg = tr("导出成功！已保存到: ") + filePath;
    Logger::info(msg);
    ui->statusBar->showMessage(msg, 3000);
    QMessageBox::information(this, "成功", msg);
}

/**
 * @brief 构建接口配置菜单（公共方法）
 * @param menu 目标菜单指针
 * @param includeManageActions 是否包含管理动作（启用/禁用网卡）
 */
void MainWindow::buildInterfaceConfigMenu(QMenu *menu, bool includeManageActions)
{
    // 一次调用获取全部接口详情，并使用缓存获取网络配置，避免多次netsh阻塞
    QList<InterfaceDetail> allDetails = NetworkInterfaceManager::getAllInterfaceDetails();
    QMap<QString, QVariantMap> configs = m_configManager->configs();

    for(const InterfaceDetail &detail : allDetails)
    {
        if(detail.name.isEmpty()) continue;
        const QString rawInterface = detail.name;
        const QString cleanInterface = m_configManager->cleanInterfaceName(rawInterface);
        // 使用缓存获取当前配置（缓存有效时直接返回，无效时自动刷新一次）
        QVariantMap currentConfig = getCachedInterfaceConfig(cleanInterface);
        QMenu *interfaceMenu = menu->addMenu(cleanInterface);

        QString configTitle = (currentConfig["method"].toString() == "dhcp")
            ? "DHCP自动获取"
            : QString("静态IP: %1").arg(currentConfig["ip"].toString());
        QAction *currentAction = interfaceMenu->addAction(configTitle);
        currentAction->setCheckable(true);
        currentAction->setChecked(true);
        currentAction->setEnabled(false);
        interfaceMenu->addSeparator();

        bool hasConfigs = false;
        for(auto it = configs.begin(); it != configs.end(); ++it)
        {
            QString configInterface = m_configManager->cleanInterfaceName(it.value()["interface"].toString());
            if(configInterface == cleanInterface)
            {
                QAction *action = interfaceMenu->addAction(it.key());
                action->setCheckable(true);
                action->setChecked(m_networkInfoCollector->compareConfigs(currentConfig, it.value()));
                connect(action, &QAction::triggered, [this, config = it.value()]()
                {
                    Logger::info(tr("从菜单应用配置: %1").arg(config["interface"].toString()));
                    QVariantMap currentConfig = m_networkInfoCollector->getCurrentNetworkConfig(config["interface"].toString(), m_configManager);
                    if(m_networkInfoCollector->compareConfigs(currentConfig, config))
                    {
                        Logger::info("配置与当前相同，无需重复应用");
                        showConfigResult(ConfigResult::Unchanged, "配置与当前相同，无需重复应用");
                        return;
                    }
                    if(m_configManager->applyConfig(config))
                    {
                        QString msg = tr("应用配置: %1").arg(
                            config["method"].toString() == "dhcp"
                                ? config["interface"].toString() + "[自动获取IP]"
                                : config["interface"].toString() + tr("[%1]").arg(config["ip"].toString())
                        );
                        ui->statusBar->showMessage(msg, 2000);
                        QTimer::singleShot(500, this, [this]()
                        {
                            updateQuickMenu();
                            m_floatWindow->update();
                        });
                        showConfigResult(ConfigResult::Success, msg);
                    }
                });
                hasConfigs = true;
            }
        }
        if(!hasConfigs)
        {
            QAction *noConfigAction = interfaceMenu->addAction(tr("无保存的配置"));
            noConfigAction->setEnabled(false);
        }

        if(includeManageActions)
        {
            interfaceMenu->addSeparator();
            QString adminStatus = detail.adminStatus;
            QAction *enableAction = interfaceMenu->addAction(tr("启用网卡"));
            enableAction->setCheckable(true);
            enableAction->setChecked(adminStatus == tr("已启用"));
            connect(enableAction, &QAction::triggered, this, [this, rawInterface]()
            {
                int index = ui->networkInterfaceCombo->findData(rawInterface);
                if(index >= 0)
                {
                    ui->networkInterfaceCombo->setCurrentIndex(index);
                }
                onEnableInterface(rawInterface);
            });
            QAction *disableAction = interfaceMenu->addAction(tr("禁用网卡"));
            disableAction->setCheckable(true);
            disableAction->setChecked(adminStatus == tr("已禁用"));
            connect(disableAction, &QAction::triggered, this, [this, rawInterface]()
            {
                int index = ui->networkInterfaceCombo->findData(rawInterface);
                if(index >= 0)
                {
                    ui->networkInterfaceCombo->setCurrentIndex(index);
                }
                onDisableInterface(rawInterface);
            });
        }
    }
}

/**
 * @brief 更新快速应用菜单
 *
 * 根据当前网络接口和保存的配置更新快速应用菜单
 */
void MainWindow::updateQuickMenu()
{
    if(!m_quickMenu)
    {
        Logger::warning("快速菜单未初始化");
        return;
    }
    Logger::debug("更新快速应用菜单");
    m_quickMenu->clear();
    NetworkInterfaceManager manager;
    QStringList interfaces = manager.getNetworkInterfaces();
    if(interfaces.isEmpty())
    {
        Logger::debug(tr("无可用网络接口"));
        QAction *noInterfaceAction = m_quickMenu->addAction(tr("无可用网络接口"));
        noInterfaceAction->setEnabled(false);
        return;
    }
    buildInterfaceConfigMenu(m_quickMenu, false);
    QMap<QString, QVariantMap> configs = m_configManager->configs();
    if(configs.size() > 8)
    {
        m_quickMenu->addSeparator();
        QAction *moreAction = m_quickMenu->addAction(tr("更多配置..."));
        connect(moreAction, &QAction::triggered, this, &MainWindow::showNormal);
    }
    Logger::info("快速应用菜单更新完成");
}

/**
 * @brief 显示悬浮窗上下文菜单
 * @param pos 菜单显示位置
 *
 * 在悬浮窗上显示包含网络配置选项的上下文菜单
 */
void MainWindow::showFloatWindowMenu(const QPoint &pos)
{
    Logger::debug("显示悬浮窗上下文菜单");
    QMenu menu;
    buildInterfaceConfigMenu(&menu, true);
    menu.addSeparator();
    QAction *topAction = menu.addAction(tr("置顶显示"));
    topAction->setCheckable(true);
    topAction->setChecked(m_floatWindow->windowFlags() & Qt::WindowStaysOnTopHint);
    connect(topAction, &QAction::toggled, [this](bool checked)
    {
        Logger::info(tr("设置悬浮窗置顶: %1").arg(checked ? "是" : "否"));
        Qt::WindowFlags flags = m_floatWindow->windowFlags();
        if(checked)
        {
            flags |= Qt::WindowStaysOnTopHint;
        }
        else
        {
            flags &= ~Qt::WindowStaysOnTopHint;
        }
        m_floatWindow->setWindowFlags(flags);
        m_floatWindow->show();
    });
    menu.addSeparator();
    menu.addAction("显示主窗口", this, &MainWindow::showNormal);
    menu.addAction("退出", qApp, &QCoreApplication::quit);
    menu.exec(pos);
    Logger::debug("悬浮窗上下文菜单已显示");
}

/**
 * @brief 处理托盘图标激活事件
 * @param reason 激活原因
 *
 * 响应托盘图标的点击事件，如双击显示主窗口
 */
void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::DoubleClick)
    {
        Logger::debug("双击托盘图标，显示主窗口");
        showNormal();
        activateWindow();
    }
}

void MainWindow::refreshNetworkInterfaces()
{
    Logger::debug("刷新网卡列表");
    // 一次调用获取全部接口详情，避免每个接口单独触发netsh（N×5秒→1×5秒）
    NetworkInterfaceManager manager;
    QList<InterfaceDetail> allDetails = manager.getAllInterfaceDetails();
    QStringList interfaces;
    for(const InterfaceDetail &detail : allDetails)
    {
        if(!detail.name.isEmpty())
            interfaces << detail.name;
    }
    QString current = ui->networkInterfaceCombo->currentText();
    ui->networkInterfaceCombo->clear();
    foreach(const InterfaceDetail &detail, allDetails)
    {
        if(detail.name.isEmpty()) continue;
        QString adminStatus = detail.adminStatus;
        QString connStatus = detail.connStatus;
        QString cleanInterface = m_configManager->cleanInterfaceName(detail.name);
        QVariantMap currentConfig = m_networkInfoCollector->getCurrentNetworkConfig(detail.name, cleanInterface, m_configManager);
        QString ipInfo = currentConfig["ip"].toString();
        if(ipInfo.isEmpty())
        {
            ipInfo = "无IP";
        }
        QString displayText = QString("%1 [%2|%3] %4").arg(detail.name).arg(adminStatus).arg(connStatus).arg(ipInfo);
        ui->networkInterfaceCombo->addItem(displayText, detail.name);
    }
    // 恢复之前选中的网卡
    int index = ui->networkInterfaceCombo->findData(current);
    if(index >= 0)
    {
        ui->networkInterfaceCombo->setCurrentIndex(index);
    }
    updateInterfaceControls();
    ui->statusBar->showMessage(tr("网卡列表已刷新，共 %1 个网卡").arg(interfaces.size()), 2000);
}

void MainWindow::onEnableInterface(const QString &interfaceName)
{
    QString interface = interfaceName.isEmpty() ? ui->networkInterfaceCombo->currentData().toString() : interfaceName;
    if(!interface.isEmpty())
    {
        QStringList availableNames;
        const QList<InterfaceDetail> details = NetworkInterfaceManager::getAllInterfaceDetails();
        for(const InterfaceDetail &detail : details)
        {
            if(!detail.name.isEmpty())
            {
                availableNames << detail.name;
            }
        }
        interface = ConfigManager::resolveInterfaceName(interface, availableNames);
    }
    if(interface.isEmpty())
    {
        showConfigResult(ConfigResult::Failure, tr("请选择要启用的网卡"));
        return;
    }
    NetworkInterfaceManager manager;
    QString status = manager.getInterfaceAdminStatus(interface);
    if(status.contains("已启用"))
    {
        QMessageBox::information(this, tr("提示"), tr("网卡 %1 已经是启用状态").arg(interface));
        return;
    }

    showLoading(tr("正在启用网卡 %1...").arg(interface));

    QThread* thread = new QThread();
    QObject* worker = new QObject();
    worker->moveToThread(thread);

    connect(thread, &QThread::started, this, [this, interface, worker]() {
        NetworkInterfaceManager mgr;
        bool result = mgr.enableInterface(interface);
        QString status = mgr.getInterfaceAdminStatus(interface);

        QString message = result ? tr("网卡 %1 已启用").arg(interface) : tr("启用网卡 %1 失败").arg(interface);
        QMetaObject::invokeMethod(this, "onEnableCompleted", Qt::QueuedConnection,
            Q_ARG(bool, result), Q_ARG(QString, interface), Q_ARG(QString, message));
    });

    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void MainWindow::onEnableInterface()
{
    onEnableInterface("");
}

void MainWindow::onDisableInterface(const QString &interfaceName)
{
    QString interface = interfaceName.isEmpty() ? ui->networkInterfaceCombo->currentData().toString() : interfaceName;
    if(!interface.isEmpty())
    {
        QStringList availableNames;
        const QList<InterfaceDetail> details = NetworkInterfaceManager::getAllInterfaceDetails();
        for(const InterfaceDetail &detail : details)
        {
            if(!detail.name.isEmpty())
            {
                availableNames << detail.name;
            }
        }
        interface = ConfigManager::resolveInterfaceName(interface, availableNames);
    }
    if(interface.isEmpty())
    {
        QMessageBox::warning(this, tr("警告"), tr("请选择要禁用的网卡"));
        return;
    }
    NetworkInterfaceManager manager;
    QString status = manager.getInterfaceAdminStatus(interface);
    if(status.contains("已禁用"))
    {
        QMessageBox::information(this, tr("提示"), tr("网卡 %1 已经是禁用状态").arg(interface));
        return;
    }

    showLoading(tr("正在禁用网卡 %1...").arg(interface));

    QThread* thread = new QThread();
    QObject* worker = new QObject();
    worker->moveToThread(thread);

    connect(thread, &QThread::started, this, [this, interface, worker]() {
        NetworkInterfaceManager mgr;
        bool result = mgr.disableInterface(interface);
        QString status = mgr.getInterfaceAdminStatus(interface);

        QString message = result ? tr("网卡 %1 已禁用").arg(interface) : tr("禁用网卡 %1 失败").arg(interface);
        QMetaObject::invokeMethod(this, "onDisableCompleted", Qt::QueuedConnection,
            Q_ARG(bool, result), Q_ARG(QString, interface), Q_ARG(QString, message));
    });

    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void MainWindow::onDisableInterface()
{
    onDisableInterface("");
}

void MainWindow::updateInterfaceControls()
{
    if(ui->networkInterfaceCombo->count() == 0)
    {
        ui->enableInterfaceBtn->setEnabled(false);
        ui->disableInterfaceBtn->setEnabled(false);
        Logger::debug("无可用网卡，禁用操作按钮");
        return;
    }
    QString interface = ui->networkInterfaceCombo->currentData().toString();
    NetworkInterfaceManager manager;
    QString status = manager.getInterfaceStatus(interface);
    bool canEnable = (status.contains("已禁用"));
    bool canDisable = (status.contains("已启用"));
    ui->enableInterfaceBtn->setEnabled(canEnable);
    ui->disableInterfaceBtn->setEnabled(canDisable);
    Logger::debug(tr("更新网卡控制状态: %1, 可启用: %2, 可禁用: %3")
                  .arg(interface).arg(canEnable).arg(canDisable));
    // 更新按钮提示文本
    ui->enableInterfaceBtn->setToolTip(canEnable ? tr("启用网卡 %1").arg(interface) : "");
    ui->disableInterfaceBtn->setToolTip(canDisable ? tr("禁用网卡 %1").arg(interface) : "");
}

/**
 * @brief 处理窗口关闭事件
 * @param event 关闭事件
 *
 * 重写关闭事件实现最小化到托盘功能
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    Logger::info("主窗口关闭事件");
    saveWindowState();
    hide();
    Logger::debug("主窗口已隐藏，程序在后台运行");
    event->ignore();
}

void MainWindow::on_networkInterfaceCombo_currentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    updateInterfaceControls();
}

void MainWindow::checkNetworkChanges()
{
    if (m_currentInterface.isEmpty()) {
        return;
    }

    QString cleanInterface = m_configManager->cleanInterfaceName(m_currentInterface);
    QVariantMap config = m_networkInfoCollector->getCurrentNetworkConfig(m_currentInterface, cleanInterface, m_configManager);
    QString currentIP = config["ip"].toString();
    
    if (currentIP.isEmpty()) {
        currentIP = "无IP";
    }

    if (currentIP != m_lastIPAddress) {
        Logger::info(tr("检测到网络配置变化: %1 -> %2").arg(m_lastIPAddress).arg(currentIP));
        m_lastIPAddress = currentIP;
        
        refreshNetworkInterfaces();
        
        if (isVisible()) {
            ui->statusBar->showMessage(tr("网络配置已更新"), 3000);
        }
    }
}

void MainWindow::onExportLogs()
{
    Logger::info("开始导出日志");
    
    QString targetDir = QFileDialog::getExistingDirectory(this, tr("选择导出目录"));
    if (targetDir.isEmpty()) {
        Logger::debug("用户取消导出日志");
        return;
    }

    bool success = Logger::exportLogs(targetDir);
    if (success) {
        Logger::info(tr("日志导出成功: %1").arg(targetDir));
        QMessageBox::information(this, tr("成功"), tr("日志文件已导出到:\n%1").arg(targetDir));
    } else {
        Logger::error("日志导出失败");
        QMessageBox::critical(this, tr("错误"), tr("无法导出日志文件！"));
    }
}

bool MainWindow::isMenuCacheValid() const
{
    if (m_menuCache.isEmpty()) {
        return false;
    }
    return m_menuCacheTime.msecsTo(QDateTime::currentDateTime()) < m_menuCacheTimeout;
}

void MainWindow::invalidateMenuCache()
{
    m_menuCache.clear();
    m_menuCacheTime = QDateTime();
    Logger::debug("菜单缓存已失效");
}

void MainWindow::updateMenuCache()
{
    Logger::debug("更新菜单缓存");
    QVariantMap cache;
    
    NetworkInterfaceManager manager;
    QStringList interfaces = manager.getNetworkInterfaces();
    
    for (const QString &interfaceName : interfaces) {
        QString cleanInterface = m_configManager->cleanInterfaceName(interfaceName);
        QVariantMap config = m_networkInfoCollector->getCurrentNetworkConfig(interfaceName, cleanInterface, m_configManager);
        cache[cleanInterface] = config;
    }
    
    m_menuCache = cache;
    m_menuCacheTime = QDateTime::currentDateTime();
    Logger::debug(tr("菜单缓存更新完成，包含 %1 个接口").arg(interfaces.size()));
}

QVariantMap MainWindow::getCachedInterfaceConfig(const QString &interfaceName)
{
    if (!isMenuCacheValid()) {
        updateMenuCache();
    }
    return m_menuCache.value(interfaceName).toMap();
}

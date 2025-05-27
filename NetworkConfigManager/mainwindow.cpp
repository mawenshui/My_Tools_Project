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
#include <QRegExp>

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
    m_quickMenu(nullptr),                     //快速菜单初始为空
    m_floatWindow(new FloatWindow(this)),     //创建悬浮窗
    m_trayIcon(new QSystemTrayIcon(this)),    //创建系统托盘图标
    //使用应用程序名称+进程ID作为单实例锁的唯一标识
    m_singleInstanceLock(APP_NAME + QString::number(QCoreApplication::applicationPid())),
    m_floatVisible(true),                     //默认显示悬浮窗
    m_autostart(false)                        //默认不启用开机自启动
{
//    //初始化日志系统
//    QDir logsDir(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/logs/"));
//    if(!logsDir.exists() && !logsDir.mkpath("."))
//    {
//        Logger::error(tr("无法创建日志文件夹: %1").arg(logsDir.path()));
//        QMessageBox::warning(this, "错误", tr("无法创建日志文件夹: ") + logsDir.path());
//        Logger::instance()->init();
//    }
//    else
//    {
//        Logger::info(tr("日志目录初始化成功: %1").arg(logsDir.path()));
//    }
//    Logger::instance()->init(logsDir.path(), "MainWindow");
    ui->setupUi(this);
    //1. 检查单实例运行
    Logger::debug("开始检查单实例运行");
    if(!checkSingleInstance())
    {
        Logger::critical("单实例检查失败，程序将退出");
        return;
    }
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
    if(!m_quickMenu)
    {
        Logger::debug("删除快速菜单");
        m_quickMenu->deleteLater();
    }
    Logger::debug("保存窗口状态");
    saveWindowState();
    delete ui;
    Logger::info("主窗口销毁完成");
}

/**
 * @brief 检查单实例运行
 * @return 是否通过单实例检查
 *
 * 使用共享内存实现单实例检查，确保同一时间只有一个程序实例运行
 */
bool MainWindow::checkSingleInstance()
{
    Logger::debug("检查程序是否已运行");
    //尝试附加到现有共享内存
    if(m_singleInstanceLock.attach())
    {
        Logger::warning("检测到程序已经在运行中");
        QMessageBox::critical(nullptr, "错误", "程序已经在运行中");
        qApp->quit();
        return false;
    }
    //创建新的共享内存块
    if(!m_singleInstanceLock.create(1))
    {
        Logger::error("无法创建单实例锁");
        QMessageBox::critical(nullptr, "错误", "无法创建单实例锁");
        return false;
    }
    Logger::info("单实例检查通过");
    return true;
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
        //边界检查
        QRect screenGeo = QApplication::primaryScreen()->availableGeometry();
        if(!screenGeo.contains(pos))
        {
            pos.setX(qBound(screenGeo.left(), pos.x(), screenGeo.right() - 100));
            pos.setY(qBound(screenGeo.top(), pos.y(), screenGeo.bottom() - 50));
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
    // 初始化状态指示灯
    m_statusIndicator = new QLabel(this);
    m_statusIndicator->setFixedSize(16, 16);
    m_statusIndicator->setToolTip("配置状态");
    QPixmap indicatorPixmap(":/images/images/indicator_gray.png");
    m_statusIndicator->setPixmap(indicatorPixmap.scaled(16, 16, Qt::KeepAspectRatio));
    ui->statusBar->addPermanentWidget(m_statusIndicator);
    //初始化状态栏
    ui->statusBar->showMessage("就绪", 2000);
    Logger::info("UI初始化完成");
}

void MainWindow::showConfigResult(bool success, const QString& message)
{
    // 更新状态指示灯
    QString iconPath = success ? ":/images/images/indicator_green.png"
                       : ":/images/images/indicator_red.png";
    QPixmap indicatorPixmap(iconPath);
    m_statusIndicator->setPixmap(indicatorPixmap.scaled(16, 16, Qt::KeepAspectRatio));
    // 显示动画效果
    QPropertyAnimation* animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(500);
    animation->setKeyValueAt(0, 1.0);
    animation->setKeyValueAt(0.5, 0.7);
    animation->setKeyValueAt(1, 1.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    // 更新状态栏消息
    ui->statusBar->showMessage(message, 5000);
    // 更新托盘图标状态
    if(m_trayIcon)
    {
        QIcon trayIcon(success ? ":/images/images/icon_green.png"
                       : ":/images/images/icon_red.png");
        m_trayIcon->setIcon(trayIcon);
        // 显示气泡通知
        m_trayIcon->showMessage("网络配置",
                                message,
                                success ? QSystemTrayIcon::Information
                                : QSystemTrayIcon::Warning,
                                3000);
    }
    // 5秒后恢复默认状态
    QTimer::singleShot(5000, this, [this]()
    {
        QPixmap indicatorPixmap(":/images/images/indicator_gray.png");
        m_statusIndicator->setPixmap(indicatorPixmap.scaled(16, 16, Qt::KeepAspectRatio));
        if(m_trayIcon)
        {
            m_trayIcon->setIcon(QIcon(":/images/images/icon.png"));
        }
    });
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
    ui->primaryDnsEdit->setEnabled(!isDhcp);
    ui->secondaryDnsEdit->setEnabled(!isDhcp);
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
    if(!validateIpConfig(config))
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
    if(!validateIpConfig(config))
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
            showConfigResult(false, "请先选择一个配置");
            return;
        }
        QVariantMap config = m_configManager->configs().value(m_currentConfig);
        if(config.isEmpty())
        {
            Logger::error("无效的配置");
            showConfigResult(false, "无效的配置");
            return;
        }
        QString interfaceName = config["interface"].toString();
        if(compareConfigs(getCurrentNetworkConfig(interfaceName), config))
        {
            Logger::info("配置未变更，跳过应用");
            showConfigResult(true, "配置未变更");
            return;
        }
        Logger::debug(tr("应用配置: %1").arg(m_currentConfig));
        // 应用配置
        if(!m_configManager->applyConfig(config))
        {
            Logger::error("应用配置失败");
            showConfigResult(false, "应用配置失败");
            return;
        }
        Logger::info("配置应用成功");
        showConfigResult(true, "配置应用成功");
        // 延迟确保网络配置更新后再刷新菜单
        QTimer::singleShot(500, this, [this]()
        {
            updateQuickMenu();
            m_floatWindow->update();
        });
    }
    catch(const std::exception& e)
    {
        Logger::critical(QString("配置应用异常: %1").arg(e.what()));
        showConfigResult(false, QString("配置应用过程中发生异常: %1").arg(e.what()));
    }
    catch(...)
    {
        Logger::critical("未知的配置应用异常");
        showConfigResult(false, "配置应用过程中发生未知异常");
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
    QVariantMap config;
    config["interface"] = ui->interfaceCombo->currentText();
    config["method"] = ui->dhcpRadio->isChecked() ? "dhcp" : "static";
    config["ip"] = ui->ipEdit->text();
    config["subnet"] = ui->subnetEdit->text();
    config["gateway"] = ui->gatewayEdit->text();
    config["primary_dns"] = ui->primaryDnsEdit->text();
    config["secondary_dns"] = ui->secondaryDnsEdit->text();
    Logger::debug(tr("获取当前表单配置: 方法=%1, IP=%2").arg(config["method"].toString()).arg(config["ip"].toString()));
    return config;
}

/**
 * @brief 验证IP配置
 * @param config 要验证的配置
 * @return 配置是否有效
 *
 * 验证静态IP配置的各项参数是否符合要求
 */
bool MainWindow::validateIpConfig(const QVariantMap &config)
{
    if(config["method"].toString() == "static")
    {
        Logger::debug("验证静态IP配置");
        //验证IP地址
        if(config["ip"].toString().isEmpty())
        {
            Logger::warning("IP地址不能为空");
            QMessageBox::critical(this, "错误", "IP地址不能为空");
            return false;
        }
        //验证子网掩码
        if(config["subnet"].toString().isEmpty())
        {
            Logger::warning("子网掩码不能为空");
            QMessageBox::critical(this, "错误", "子网掩码不能为空");
            return false;
        }
        //验证IP和子网掩码格式
        QRegExp ipRegex("^(\\d{1,3}\\.){3}\\d{1,3}$");
        if(!ipRegex.exactMatch(config["ip"].toString()))
        {
            Logger::warning(tr("IP地址格式不正确: %1").arg(config["ip"].toString()));
            QMessageBox::critical(this, "错误", "IP地址格式不正确");
            return false;
        }
        if(!ipRegex.exactMatch(config["subnet"].toString()))
        {
            Logger::warning(tr("子网掩码格式不正确: %1").arg(config["subnet"].toString()));
            QMessageBox::critical(this, "错误", "子网掩码格式不正确");
            return false;
        }
        //如果有网关，验证网关格式
        if(!config["gateway"].toString().isEmpty() && !ipRegex.exactMatch(config["gateway"].toString()))
        {
            Logger::warning(tr("默认网关格式不正确: %1").arg(config["gateway"].toString()));
            QMessageBox::critical(this, "错误", "默认网关格式不正确");
            return false;
        }
        //验证DNS格式
        if(!config["primary_dns"].toString().isEmpty() && !ipRegex.exactMatch(config["primary_dns"].toString()))
        {
            Logger::warning(tr("首选DNS格式不正确: %1").arg(config["primary_dns"].toString()));
            QMessageBox::critical(this, "错误", "首选DNS格式不正确");
            return false;
        }
        if(!config["secondary_dns"].toString().isEmpty() && !ipRegex.exactMatch(config["secondary_dns"].toString()))
        {
            Logger::warning(tr("备用DNS格式不正确: %1").arg(config["secondary_dns"].toString()));
            QMessageBox::critical(this, "错误", "备用DNS格式不正确");
            return false;
        }
    }
    else
    {
        Logger::debug("DHCP配置无需验证");
    }
    Logger::info("IP配置验证通过");
    return true;
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
    QAction *showAction = trayMenu->addAction("显示主窗口");
    connect(showAction, &QAction::triggered, this, &MainWindow::showNormal);
    //添加"显示/隐藏悬浮球"动作
    QAction *floatAction = trayMenu->addAction("显示/隐藏悬浮球");
    floatAction->setCheckable(true);
    floatAction->setChecked(m_floatVisible);
    connect(floatAction, &QAction::toggled, this, &MainWindow::toggleFloatWindow);
    //初始化快速配置子菜单
    m_quickMenu = trayMenu->addMenu("快速应用配置");
    updateQuickMenu();
    //添加网卡管理子菜单
    QMenu *interfaceMenu = trayMenu->addMenu("网卡管理");
    //获取所有网络接口
    NetworkInterfaceManager manager;
    QStringList interfaces = manager.getNetworkInterfaces();
    for(const QString &interface : interfaces)
    {
        QString cleanInterface = interface.split(" (").first().trimmed();
        QMenu *ifaceMenu = interfaceMenu->addMenu(cleanInterface);
        QString adminStatus = manager.getInterfaceAdminStatus(cleanInterface);
        QAction *enableAction = ifaceMenu->addAction("启用网卡");
        enableAction->setCheckable(true);
        enableAction->setChecked(adminStatus == "已启用");
        connect(enableAction, &QAction::triggered, this, [this, cleanInterface]()
        {
            onEnableInterface(cleanInterface);
        });
        QAction *disableAction = ifaceMenu->addAction("禁用网卡");
        disableAction->setCheckable(true);
        disableAction->setChecked(adminStatus == "已禁用");
        connect(disableAction, &QAction::triggered, this, [this, cleanInterface]()
        {
            onDisableInterface(cleanInterface);
        });
    }
    //添加"开机自启动"动作
    QAction *autostartAction = trayMenu->addAction("开机自启动");
    autostartAction->setCheckable(true);
    autostartAction->setChecked(m_autostart);
    connect(autostartAction, &QAction::toggled, this, &MainWindow::toggleAutostart);
    trayMenu->addSeparator();
    //添加"退出"动作
    QAction *quitAction = trayMenu->addAction("退出");
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
    //获取所有网络接口
    NetworkInterfaceManager manager;
    QStringList interfaces = manager.getNetworkInterfaces();
    if(interfaces.isEmpty())
    {
        Logger::debug("无可用网络接口");
        QAction *noInterfaceAction = m_quickMenu->addAction("无可用网络接口");
        noInterfaceAction->setEnabled(false);
        return;
    }
    //获取所有保存的配置
    QMap<QString, QVariantMap> configs = m_configManager->configs();
    //为每个接口创建子菜单
    for(const QString &currentInterface : interfaces)
    {
        QString cleanInterface = currentInterface.split(" (").first().trimmed();
        //获取当前接口的配置
        QVariantMap currentConfig = getCurrentNetworkConfig(cleanInterface);
        //创建接口子菜单
        QMenu *interfaceMenu = m_quickMenu->addMenu(cleanInterface);
        //添加"当前配置"项
        QString configTitle;
        if(currentConfig["method"].toString() == "dhcp")
        {
            configTitle = "DHCP自动获取";
        }
        else
        {
            configTitle = QString("静态IP: %1").arg(currentConfig["ip"].toString());
        }
        QAction *currentAction = interfaceMenu->addAction(configTitle);
        currentAction->setCheckable(true);
        currentAction->setChecked(true);
        currentAction->setEnabled(false); //当前配置不可点击
        interfaceMenu->addSeparator();
        //添加该接口的所有保存的配置
        bool hasConfigs = false;
        for(auto it = configs.begin(); it != configs.end(); ++it)
        {
            QString configInterface = m_configManager->cleanInterfaceName(it.value()["interface"].toString());
            if(configInterface == cleanInterface)
            {
                QAction *action = interfaceMenu->addAction(it.key());
                action->setCheckable(true);
                action->setChecked(compareConfigs(currentConfig, it.value()));
                connect(action, &QAction::triggered, [this, config = it.value()]()
                {
                    Logger::info(tr("从快速菜单应用配置: %1").arg(config["interface"].toString()));
                    if(m_configManager->applyConfig(config))
                    {
                        ui->statusBar->showMessage(tr("从快速菜单应用配置: %1").arg(config["method"].toString() == "dhcp" ? config["interface"].toString() + "[自动获取IP]" : config["interface"].toString() + tr("[%1]").arg(config["ip"].toString())), 2000);
                        //更新菜单状态
                        QTimer::singleShot(500, this, &MainWindow::updateQuickMenu);
                    }
                });
                hasConfigs = true;
            }
        }
        if(!hasConfigs)
        {
            QAction *noConfigAction = interfaceMenu->addAction("无保存的配置");
            noConfigAction->setEnabled(false);
        }
    }
    //如果配置较多，添加"更多配置"选项
    if(configs.size() > 8)
    {
        m_quickMenu->addSeparator();
        QAction *moreAction = m_quickMenu->addAction("更多配置...");
        connect(moreAction, &QAction::triggered, this, &MainWindow::showNormal);
    }
    Logger::info("快速应用菜单更新完成");
}

/**
 * @brief 检查DHCP是否启用
 * @param dhcpOutput netsh命令输出
 * @return 是否启用DHCP
 *
 * 从netsh命令输出中解析DHCP状态
 */
bool MainWindow::isDhcpEnabled(const QString &output)
{
    QStringList patterns = {"DHCP enabled:\\s*(yes|是|ja|oui)", "DHCP activé"};
    for(const auto &pattern : patterns)
    {
        QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
        if(re.match(output).hasMatch())
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief 获取当前网络配置
 * @param InterfaceName 网络接口名称
 * @return 包含当前配置的QVariantMap
 *
 * 通过执行netsh命令获取指定接口的当前网络配置
 */
QVariantMap MainWindow::getCurrentNetworkConfig(const QString &InterfaceName)
{
    Logger::debug("获取当前网卡的网络配置");
    QVariantMap currentConfig;
    QString normalizedInterface = InterfaceName.split('\r').first().trimmed();
    if(normalizedInterface.contains("("))
    {
        normalizedInterface = normalizedInterface.split("(").first().trimmed();
    }
    currentConfig["interface"] = normalizedInterface;
    Logger::debug(tr("查询接口配置: %1").arg(normalizedInterface));
    //检测DHCP状态
    QProcess dhcpProcess;
    dhcpProcess.start("netsh", QStringList() << "interface" << "ip" << "show" << "config" << QString("name=\"%1\"").arg(normalizedInterface));
    dhcpProcess.waitForFinished();
    QString dhcpOutput = QString::fromLocal8Bit(dhcpProcess.readAllStandardOutput());
    bool isDhcp = isDhcpEnabled(dhcpOutput);
    Logger::debug(tr("接口 DHCP 状态: %1").arg(isDhcp ? "启用" : "禁用"));
    //获取IP配置信息
    QProcess ipProcess;
    ipProcess.start("netsh", QStringList() << "interface" << "ip" << "show" << "addresses" << QString("name=\"%1\"").arg(normalizedInterface));
    ipProcess.waitForFinished();
    QString ipOutput = QString::fromLocal8Bit(ipProcess.readAllStandardOutput());
    //使用正则表达式匹配IP配置信息
    QRegularExpression ipRegex(R"(IP [Aa]ddress\s*:\s*([0-9.]+))");
    QRegularExpression subnetRegex(R"(Subnet [Pp]refix[^\n]+mask\s+([0-9.]+))");
    QRegularExpression gatewayRegex(R"(Default [Gg]ateway\s*:\s*([0-9.]+))");
    //匹配IP地址
    QRegularExpressionMatch ipMatch = ipRegex.match(ipOutput);
    if(ipMatch.hasMatch())
    {
        currentConfig["ip"] = ipMatch.captured(1).trimmed();
        Logger::debug(tr("获取IP地址: %1").arg(currentConfig["ip"].toString()));
    }
    //匹配子网掩码
    QRegularExpressionMatch subnetMatch = subnetRegex.match(ipOutput);
    if(subnetMatch.hasMatch())
    {
        currentConfig["subnet"] = subnetMatch.captured(1).trimmed();
        Logger::debug(tr("获取子网掩码: %1").arg(currentConfig["subnet"].toString()));
    }
    else
    {
        //备用匹配方式
        QRegularExpression altSubnetRegex(R"(Subnet Mask\s*:\s*([0-9.]+))");
        QRegularExpressionMatch altMatch = altSubnetRegex.match(ipOutput);
        if(altMatch.hasMatch())
        {
            currentConfig["subnet"] = altMatch.captured(1).trimmed();
            Logger::debug(tr("获取子网掩码(备用方式): %1").arg(currentConfig["subnet"].toString()));
        }
    }
    //匹配默认网关
    QRegularExpressionMatch gatewayMatch = gatewayRegex.match(ipOutput);
    if(gatewayMatch.hasMatch())
    {
        currentConfig["gateway"] = gatewayMatch.captured(1).trimmed();
        Logger::debug(tr("获取默认网关: %1").arg(currentConfig["gateway"].toString()));
    }
    //如果通过netsh未获取到子网掩码，尝试使用QNetworkInterface
    if(currentConfig["subnet"].toString().isEmpty())
    {
        Logger::debug("尝试通过QNetworkInterface获取子网掩码");
        QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
        for(const QNetworkInterface &currentInterface : interfaces)
        {
            QString ifaceName = currentInterface.name();
            if(ifaceName.contains("("))
            {
                ifaceName = ifaceName.split("(").first().trimmed();
            }
            if(ifaceName == normalizedInterface)
            {
                for(const QNetworkAddressEntry &entry : currentInterface.addressEntries())
                {
                    if(entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isNull())
                    {
                        if(currentConfig["ip"].toString().isEmpty())
                        {
                            currentConfig["ip"] = entry.ip().toString();
                            Logger::debug(tr("通过QNetworkInterface获取IP地址: %1").arg(currentConfig["ip"].toString()));
                        }
                        if(currentConfig["subnet"].toString().isEmpty())
                        {
                            currentConfig["subnet"] = entry.netmask().toString();
                            Logger::debug(tr("通过QNetworkInterface获取子网掩码: %1").arg(currentConfig["subnet"].toString()));
                        }
                        break;
                    }
                }
                break;
            }
        }
    }
    //确定IP获取方式
    if(isDhcp)
    {
        currentConfig["method"] = "dhcp";
        Logger::debug("确定为DHCP模式(通过DHCP状态检测)");
    }
    else
    {
        currentConfig["method"] = "static";
        Logger::debug("确定为静态IP模式");
    }
    Logger::info("当前网络配置获取完成");
    return currentConfig;
}

/**
 * @brief 获取所有网络接口的配置
 * @return 包含所有接口配置的QVariantList
 *
 * 通过执行netsh命令获取所有网络接口的当前配置
 */
QVariantList MainWindow::getAllNetworkConfigs()
{
    Logger::debug("获取所有网络配置");
    QVariantList allConfigs;
    //获取所有网络接口
    NetworkInterfaceManager manager;
    QStringList interfaces = manager.getNetworkInterfaces();
    for(const QString &iface : interfaces)
    {
        QVariantMap currentConfig;
        QString ifaceName = iface.split(" (").first().trimmed(); //提取接口名称
        currentConfig["interface"] = ifaceName;
        Logger::debug(tr("查询接口配置: %1").arg(ifaceName));
        //检测DHCP状态
        QProcess dhcpProcess;
        dhcpProcess.start("netsh", QStringList() << "interface" << "ip" << "show" << "config" << QString("name=\"%1\"").arg(ifaceName));
        dhcpProcess.waitForFinished();
        QString dhcpOutput = QString::fromLocal8Bit(dhcpProcess.readAllStandardOutput());
        bool isDhcp = isDhcpEnabled(dhcpOutput);
        Logger::debug(tr("接口 DHCP 状态: %1").arg(isDhcp ? "启用" : "禁用"));
        //获取IP配置信息
        QProcess ipProcess;
        ipProcess.start("netsh", QStringList() << "interface" << "ip" << "show" << "addresses" << QString("name=\"%1\"").arg(ifaceName));
        ipProcess.waitForFinished();
        QString ipOutput = QString::fromLocal8Bit(ipProcess.readAllStandardOutput());
        //使用正则表达式匹配IP配置信息
        QRegularExpression ipRegex(R"(IP [Aa]ddress\s*:\s*([0-9.]+))");
        QRegularExpression subnetRegex(R"(Subnet [Pp]refix[^\n]+mask\s+([0-9.]+))");
        QRegularExpression gatewayRegex(R"(Default [Gg]ateway\s*:\s*([0-9.]+))");
        //匹配IP地址
        QRegularExpressionMatch ipMatch = ipRegex.match(ipOutput);
        if(ipMatch.hasMatch())
        {
            currentConfig["ip"] = ipMatch.captured(1).trimmed();
            Logger::debug(tr("获取IP地址: %1").arg(currentConfig["ip"].toString()));
        }
        //匹配子网掩码
        QRegularExpressionMatch subnetMatch = subnetRegex.match(ipOutput);
        if(subnetMatch.hasMatch())
        {
            currentConfig["subnet"] = subnetMatch.captured(1).trimmed();
            Logger::debug(tr("获取子网掩码: %1").arg(currentConfig["subnet"].toString()));
        }
        //匹配默认网关
        QRegularExpressionMatch gatewayMatch = gatewayRegex.match(ipOutput);
        if(gatewayMatch.hasMatch())
        {
            currentConfig["gateway"] = gatewayMatch.captured(1).trimmed();
            Logger::debug(tr("获取默认网关: %1").arg(currentConfig["gateway"].toString()));
        }
        //确定配置类型
        currentConfig["method"] = isDhcp ? "dhcp" : "static";
        Logger::debug(tr("确定为[%1]模式").arg(isDhcp ? "DHCP" : "静态IP"));
        //将当前配置添加到结果列表
        allConfigs.append(currentConfig);
    }
    Logger::info("所有网络配置获取完成");
    return allConfigs;
}

/**
 * @brief 比较两个配置是否相同
 * @param current 当前配置
 * @param saved 保存的配置
 * @return 配置是否相同
 *
 * 比较两个网络配置的各项参数是否一致
 */
bool MainWindow::compareConfigs(const QVariantMap &current, const QVariantMap &saved)
{
    if(current.isEmpty() || saved.isEmpty())
    {
        Logger::debug("配置为空，不匹配");
        return false;
    }
    //清理接口名称进行比较
    QString currentInterface = ConfigManager::cleanInterfaceName(current["interface"].toString());
    QString savedInterface = ConfigManager::cleanInterfaceName(saved["interface"].toString());
    if(currentInterface != savedInterface)
    {
        Logger::debug(tr("接口不匹配: %1 != %2").arg(currentInterface).arg(savedInterface));
        return false;
    }
    //比较IP获取方法
    QString currentMethod = current["method"].toString();
    QString savedMethod = saved["method"].toString();
    if(currentMethod != savedMethod)
    {
        Logger::debug(tr("方法不匹配: %1 != %2").arg(currentMethod).arg(savedMethod));
        return false;
    }
    //如果是DHCP配置，只需比较接口和方法
    if(savedMethod == "dhcp")
    {
        Logger::debug("比较DHCP配置");
        return currentMethod == "dhcp";
    }
    //静态IP配置需要比较所有关键字段
    Logger::debug("比较静态IP配置");
    try
    {
        bool ipMatch = current["ip"].toString() == saved["ip"].toString();
        bool subnetMatch = current["subnet"].toString() == saved["subnet"].toString();
        bool gatewayMatch = current["gateway"].toString() == saved["gateway"].toString();
        Logger::debug(tr("比较结果 - IP: %1, 子网: %2, 网关: %3")
                      .arg(ipMatch ? "匹配" : "不匹配")
                      .arg(subnetMatch ? "匹配" : "不匹配")
                      .arg(gatewayMatch ? "匹配" : "不匹配"));
        return ipMatch && subnetMatch && gatewayMatch;
    }
    catch(...)
    {
        Logger::error("配置比较异常");
        return false;
    }
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
    //获取所有网络接口
    NetworkInterfaceManager manager;
    QStringList interfaces = manager.getNetworkInterfaces();
    QMenu menu;
    //为每个接口创建子菜单
    for(const QString &currentInterface : interfaces)
    {
        QString cleanInterface = currentInterface.split(" (").first().trimmed();
        //获取当前接口的配置
        QVariantMap currentConfig = getCurrentNetworkConfig(cleanInterface);
        //创建接口子菜单
        QMenu *interfaceMenu = menu.addMenu(cleanInterface);
        //添加"当前配置"项
        QString configTitle;
        if(currentConfig["method"].toString() == "dhcp")
        {
            configTitle = "DHCP自动获取";
        }
        else
        {
            configTitle = QString("静态IP: %1").arg(currentConfig["ip"].toString());
        }
        QAction *currentAction = interfaceMenu->addAction(configTitle);
        currentAction->setCheckable(true);
        currentAction->setChecked(true);
        currentAction->setEnabled(false); //当前配置不可点击
        interfaceMenu->addSeparator();
        //添加该接口的所有保存的配置
        bool hasConfigs = false;
        QMap<QString, QVariantMap> configs = m_configManager->configs();
        for(auto it = configs.begin(); it != configs.end(); ++it)
        {
            QString configInterface = m_configManager->cleanInterfaceName(it.value()["interface"].toString());
            if(configInterface == cleanInterface)
            {
                QAction *action = interfaceMenu->addAction(it.key());
                action->setCheckable(true);
                action->setChecked(compareConfigs(currentConfig, it.value()));
                connect(action, &QAction::triggered, [this, config = it.value()]()
                {
                    Logger::info(tr("从悬浮窗菜单应用配置: %1").arg(config["interface"].toString()));
                    // 先检查是否与当前配置相同
                    QVariantMap currentConfig = getCurrentNetworkConfig(config["interface"].toString());
                    if(compareConfigs(currentConfig, config))
                    {
                        Logger::info("配置与当前相同，无需重复应用");
                        return;
                    }
                    if(m_configManager->applyConfig(config))
                    {
                        ui->statusBar->showMessage(tr("从悬浮窗菜单应用配置: %1").arg(config["method"].toString() == "dhcp" ? config["interface"].toString() + "[自动获取IP]" : config["interface"].toString() + tr("[%1]").arg(config["ip"].toString())), 2000);
                        // 延迟500ms确保网络配置更新后再刷新菜单
                        QTimer::singleShot(500, this, [this]()
                        {
                            updateQuickMenu();
                            m_floatWindow->update();
                        });
                    }
                });
                hasConfigs = true;
            }
        }
        if(!hasConfigs)
        {
            QAction *noConfigAction = interfaceMenu->addAction("无保存的配置");
            noConfigAction->setEnabled(false);
        }
        // 添加网卡管理动作
        interfaceMenu->addSeparator();
        QString adminStatus = manager.getInterfaceAdminStatus(cleanInterface);
        QAction *enableAction = interfaceMenu->addAction("启用网卡");
        enableAction->setCheckable(true);
        enableAction->setChecked(adminStatus == "已启用");
        connect(enableAction, &QAction::triggered, this, [this, cleanInterface]()
        {
            ui->networkInterfaceCombo->setCurrentText(cleanInterface);
            onEnableInterface(cleanInterface);
        });
        QAction *disableAction = interfaceMenu->addAction("禁用网卡");
        disableAction->setCheckable(true);
        disableAction->setChecked(adminStatus == "已禁用");
        connect(disableAction, &QAction::triggered, this, [this, cleanInterface]()
        {
            ui->networkInterfaceCombo->setCurrentText(cleanInterface);
            onDisableInterface(cleanInterface);
        });
    }
    menu.addSeparator();
    //添加"置顶显示"选项
    QAction *topAction = menu.addAction("置顶显示");
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
    NetworkInterfaceManager manager;
    QStringList interfaces = manager.getNetworkInterfaces();
    QString current = ui->networkInterfaceCombo->currentText();
    ui->networkInterfaceCombo->clear();
    foreach(const QString &interface, interfaces)
    {
        QString adminStatus = manager.getInterfaceAdminStatus(interface);
        QString connStatus = manager.getInterfaceConnStatus(interface);
        ui->networkInterfaceCombo->addItem(QString("%1 [%2|%3]").arg(interface).arg(adminStatus).arg(connStatus), interface);
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
    if(interface.isEmpty())
    {
        QMessageBox::warning(this, tr("警告"), tr("请选择要启用的网卡"));
        return;
    }
    NetworkInterfaceManager manager;
    QString status = manager.getInterfaceAdminStatus(interface);
    if(status.contains("已启用"))
    {
        QMessageBox::information(this, tr("提示"), tr("网卡 %1 已经是启用状态").arg(interface));
        return; // 已经是启用状态
    }
    if(manager.enableInterface(interface))
    {
        ui->statusBar->showMessage(tr("网卡 %1 已启用").arg(interface), 2000);
        refreshNetworkInterfaces(); // 刷新列表显示新状态
    }
    else
    {
        QMessageBox::critical(this, tr("错误"), tr("启用网卡 %1 失败").arg(interface));
    }
}

void MainWindow::onEnableInterface()
{
    onEnableInterface("");
}

void MainWindow::onDisableInterface(const QString &interfaceName)
{
    QString interface = interfaceName.isEmpty() ? ui->networkInterfaceCombo->currentData().toString() : interfaceName;
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
        return; // 已经是禁用状态
    }
    if(manager.disableInterface(interface))
    {
        ui->statusBar->showMessage(tr("网卡 %1 已禁用").arg(interface), 2000);
        refreshNetworkInterfaces(); // 刷新列表显示新状态
    }
    else
    {
        QMessageBox::critical(this, tr("错误"), tr("禁用网卡 %1 失败").arg(interface));
    }
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

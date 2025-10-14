#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "historydialog.h"
#include "chartdialog.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

// ProcessTreeWidgetItem 实现
ProcessTreeWidgetItem::ProcessTreeWidgetItem(QTreeWidget *parent)
    : QTreeWidgetItem(parent)
{
}

/**
 * @brief 重写比较操作符，实现正确的数值排序
 * @param other 要比较的另一个项目
 * @return 如果当前项目应该排在other之前返回true
 */
/**
 * @brief 重写比较操作符，实现正确的数值排序
 * @param other 要比较的另一个项目
 * @return 如果当前项目应该排在other之前返回true
 */
bool ProcessTreeWidgetItem::operator<(const QTreeWidgetItem &other) const
{
    int column = treeWidget()->sortColumn();
    
    // 对于数值列，使用UserRole中存储的数值进行比较
     switch (column) {
         case 1: // PID
         case 2: // 磁盘读取
         case 3: // 磁盘写入
         case 4: // 总磁盘使用
         case 6: // 内存使用
         case 8: // 重要性（数值排序）
         {
             QVariant thisData = data(column, Qt::UserRole);
             QVariant otherData = other.data(column, Qt::UserRole);
             
             if (thisData.isValid() && otherData.isValid()) {
                 return thisData.toULongLong() < otherData.toULongLong();
             }
             break;
         }
         case 5: // CPU使用率（double类型）
         {
             QVariant thisData = data(column, Qt::UserRole);
             QVariant otherData = other.data(column, Qt::UserRole);
             
             if (thisData.isValid() && otherData.isValid()) {
                 return thisData.toDouble() < otherData.toDouble();
             }
             break;
         }
         default:
             // 对于文本列（进程名称、用户），使用默认的字符串比较
             break;
     }
    
    // 对于文本列或没有数值数据的情况，使用默认的字符串比较
    return QTreeWidgetItem::operator<(other);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , refreshTimer(new QTimer(this))
    , driveScrollArea(nullptr)
    , driveContainer(nullptr)
    , driveLayout(nullptr)
    , processTreeWidget(nullptr)
    , refreshIntervalCombo(nullptr)
    , statusLabel(nullptr)
    , refreshButton(nullptr)
    , mainSplitter(nullptr)
    , processMonitor(nullptr)
    , processContextMenu(nullptr)
    , terminateProcessAction(nullptr)
    , showProcessDetailsAction(nullptr)
    , openProcessLocationAction(nullptr)
    , historyManager(nullptr)
    , historyDialog(nullptr)
    , chartDialog(nullptr)
    , trayIcon(nullptr)
    , trayMenu(nullptr)
    , settings(nullptr)
    , minimizeToTray(true)
    , startMinimized(false)
    , autoStart(false)
    , closeToTray(true)
{
    qInfo() << "[主窗口] 初始化磁盘监控程序";
    
    ui->setupUi(this);
    
    // 初始化设置 - 配置文件保存到程序目录下的config文件夹
    QString configDir = QApplication::applicationDirPath() + "/config";
    QDir().mkpath(configDir); // 确保config目录存在
    QString configFile = configDir + "/settings.ini";
    settings = new QSettings(configFile, QSettings::IniFormat, this);
    loadSettings();
    
    // 设置系统托盘
    setupSystemTray();
    
    // 初始化颜色方案
    lowUsageColor = QColor(76, 175, 80);      // 绿色
    mediumUsageColor = QColor(255, 193, 7);   // 黄色
    highUsageColor = QColor(255, 152, 0);     // 橙色
    criticalUsageColor = QColor(244, 67, 54); // 红色
    
    setupUI();
    setupMenuBar();
    setupStatusBar();
    initializeDriveMonitoring();
    initializeProcessMonitor();
    initializeHistoryManager();
    
    qInfo() << "[主窗口] 初始化完成";
    
    // 连接定时器
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::updateDiskInfo);
    
    // 设置初始刷新间隔为5秒
    refreshTimer->start(5000);
    
    // 立即更新一次
    updateDiskInfo();
}

MainWindow::~MainWindow()
{
    if (refreshTimer) {
        refreshTimer->stop();
    }
    
    if (processMonitor && monitorThread) {
        // 停止监控
        QMetaObject::invokeMethod(processMonitor, "stopMonitoring", Qt::QueuedConnection);
        
        // 等待线程结束
        monitorThread->quit();
        monitorThread->wait(3000); // 最多等待3秒
        
        if (monitorThread->isRunning()) {
            monitorThread->terminate();
            monitorThread->wait(1000);
        }
    }
    
    // 清理历史数据管理器
    if (historyManager) {
        historyManager->stopRecording();
        delete historyManager;
        historyManager = nullptr;
    }
    
    // 清理历史数据对话框
    if (historyDialog) {
        delete historyDialog;
        historyDialog = nullptr;
    }
    
    delete ui;
}

void MainWindow::setupUI()
{
    // 获取UI中的组件
    driveScrollArea = ui->driveScrollArea;
    processTreeWidget = ui->processTreeWidget;
    mainSplitter = ui->mainSplitter;
    
    // 检查关键UI组件是否有效
    if (!driveScrollArea || !processTreeWidget || !mainSplitter) {
        qCritical() << "[主窗口] 错误：关键UI组件为空指针";
        return;
    }
    
    // 设置分割器比例
    mainSplitter->setSizes({400, 600});
    
    // 连接分割器状态变化信号
    connect(mainSplitter, &QSplitter::splitterMoved, this, [this]() {
        if (settings) {
            settings->setValue("UI/splitterSizes", QVariant::fromValue(mainSplitter->sizes()));
            settings->sync();
        }
    });
    
    // 设置进程信息标签的默认样式
    if (ui->processInfoLabel) {
        ui->processInfoLabel->setText("💡 请选择一个磁盘查看相关进程信息");
        ui->processInfoLabel->setStyleSheet(
            "QLabel {"
            "    color: #6c757d;"
            "    font-style: italic;"
            "    font-size: 10pt;"
            "    padding: 8px 12px;"
            "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "                                stop: 0 rgba(108, 117, 125, 0.1),"
            "                                stop: 1 rgba(108, 117, 125, 0.05));"
            "    border: 1px solid rgba(108, 117, 125, 0.2);"
            "    border-radius: 6px;"
            "    margin: 4px 0;"
            "}"
        );
    }
    
    // 设置进程树控件的列标题 - 使用更清晰的描述
    QStringList headers;
    headers << "📋 进程名称" << "🆔 进程ID" << "📖 磁盘读取速率" << "📝 磁盘写入速率" << "💾 磁盘总使用量" << "⚡ CPU占用率" << "🧠 内存占用" << "👤 所属用户" << "⭐ 系统重要性";
    processTreeWidget->setHeaderLabels(headers);
    
    // 设置列的工具提示，帮助用户理解每列的含义
    processTreeWidget->headerItem()->setToolTip(0, "当前运行的进程程序名称");
    processTreeWidget->headerItem()->setToolTip(1, "进程的唯一标识符(Process ID)");
    processTreeWidget->headerItem()->setToolTip(2, "进程每秒从磁盘读取的数据量");
    processTreeWidget->headerItem()->setToolTip(3, "进程每秒向磁盘写入的数据量");
    processTreeWidget->headerItem()->setToolTip(4, "进程累计使用的磁盘IO总量");
    processTreeWidget->headerItem()->setToolTip(5, "进程当前占用的CPU处理器资源百分比");
    processTreeWidget->headerItem()->setToolTip(6, "进程当前占用的系统内存大小");
    processTreeWidget->headerItem()->setToolTip(7, "运行此进程的用户账户");
    processTreeWidget->headerItem()->setToolTip(8, "进程对系统稳定性的重要程度");
    
    // 设置列宽 - 根据内容调整合适的宽度
    processTreeWidget->header()->resizeSection(0, 180);  // 进程名称 - 增加宽度显示完整名称
    processTreeWidget->header()->resizeSection(1, 85);   // 进程ID
    processTreeWidget->header()->resizeSection(2, 120);  // 磁盘读取速率
    processTreeWidget->header()->resizeSection(3, 120);  // 磁盘写入速率
    processTreeWidget->header()->resizeSection(4, 140);  // 磁盘总使用量
    processTreeWidget->header()->resizeSection(5, 100);  // CPU占用率
    processTreeWidget->header()->resizeSection(6, 120);  // 内存占用
    processTreeWidget->header()->resizeSection(7, 100);  // 所属用户
    processTreeWidget->header()->resizeSection(8, 110);  // 系统重要性
    
    // 设置表头样式 - 优化对齐和调整模式
     processTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
     processTreeWidget->header()->setStretchLastSection(true);  // 最后一列自动拉伸
     processTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Interactive);  // 进程名称可调整
     processTreeWidget->header()->setSectionResizeMode(1, QHeaderView::Fixed);       // PID固定宽度
     processTreeWidget->header()->setSectionResizeMode(7, QHeaderView::Interactive); // 用户列可调整
     processTreeWidget->header()->setSectionResizeMode(8, QHeaderView::Stretch);     // 重要性列自适应
    
    // 启用排序功能并设置默认排序
    processTreeWidget->setSortingEnabled(true);
    processTreeWidget->sortByColumn(4, Qt::DescendingOrder); // 默认按总磁盘使用量降序排列
    
    // 设置右键菜单
    processTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(processTreeWidget, &QTreeWidget::customContextMenuRequested,
            this, &MainWindow::showProcessContextMenu);
    
    // 连接进程树双击事件
    connect(processTreeWidget, &QTreeWidget::itemDoubleClicked, 
            this, &MainWindow::onProcessItemDoubleClicked);
    
    // 连接列宽变化信号，自动保存列宽设置
    connect(processTreeWidget->header(), &QHeaderView::sectionResized, 
            this, [this](int logicalIndex, int oldSize, int newSize) {
        Q_UNUSED(oldSize)
        if (settings) {
            QString key = QString("ProcessList/column%1Width").arg(logicalIndex);
            settings->setValue(key, newSize);
            settings->sync();
        }
    });
    
    // 创建右键菜单
    processContextMenu = new QMenu(this);
    terminateProcessAction = processContextMenu->addAction("终止进程");
    showProcessDetailsAction = processContextMenu->addAction("显示详细信息");
    openProcessLocationAction = processContextMenu->addAction("打开文件位置");
    
    connect(terminateProcessAction, &QAction::triggered, this, &MainWindow::terminateSelectedProcess);
    connect(showProcessDetailsAction, &QAction::triggered, this, &MainWindow::showProcessDetails);
    connect(openProcessLocationAction, &QAction::triggered, this, &MainWindow::openSelectedProcessLocation);
    
    // 创建驱动器容器
    driveContainer = new QWidget();
    if (!driveContainer) {
        qCritical() << "[主窗口] 错误：无法创建驱动器容器";
        return;
    }
    
    driveLayout = new QVBoxLayout(driveContainer);
    if (!driveLayout) {
        qCritical() << "[主窗口] 错误：无法创建驱动器布局";
        return;
    }
    
    driveLayout->setSpacing(8);
    driveLayout->setContentsMargins(4, 4, 4, 4);
    driveScrollArea->setWidget(driveContainer);
}

void MainWindow::setupMenuBar()
{
    // 连接菜单动作
    connect(ui->actionRefresh, &QAction::triggered, this, &MainWindow::refreshData);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::quitApplication);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    
    // 连接数据菜单动作
    connect(ui->actionViewHistory, &QAction::triggered, this, &MainWindow::showHistoryDialog);
    connect(ui->actionExportCSV, &QAction::triggered, this, &MainWindow::exportHistoryCSV);
    connect(ui->actionExportExcel, &QAction::triggered, this, &MainWindow::exportHistoryExcel);
    
    // 连接图表菜单动作（如果UI文件中已定义）
    // 这些动作需要在UI文件中定义，或者在代码中动态创建
    // connect(ui->actionShowCharts, &QAction::triggered, this, &MainWindow::showChartDialog);
    // connect(ui->actionRealTimeChart, &QAction::triggered, this, &MainWindow::showRealTimeChart);
    // connect(ui->actionHistoryTrendChart, &QAction::triggered, this, &MainWindow::showHistoryTrendChart);
    // connect(ui->actionMultiDriveChart, &QAction::triggered, this, &MainWindow::showMultiDriveChart);
    
    // 连接设置菜单动作
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::showSettings);
    connect(ui->actionAutoStart, &QAction::triggered, this, &MainWindow::toggleAutoStart);
    connect(ui->actionMinimizeToTray, &QAction::triggered, this, &MainWindow::toggleMinimizeToTray);
    connect(ui->actionStartMinimized, &QAction::triggered, this, &MainWindow::toggleStartMinimized);
    connect(ui->actionCloseToTray, &QAction::triggered, this, &MainWindow::toggleCloseToTray);
    
    // 设置菜单项的初始状态
    ui->actionAutoStart->setChecked(autoStart);
    ui->actionMinimizeToTray->setChecked(minimizeToTray);
    ui->actionStartMinimized->setChecked(startMinimized);
    ui->actionCloseToTray->setChecked(closeToTray);
    
    // 添加工具栏控件
    refreshButton = new QPushButton("刷新");
    refreshButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshData);
    
    refreshIntervalCombo = new QComboBox();
    refreshIntervalCombo->addItem("1秒", 1000);
    refreshIntervalCombo->addItem("3秒", 3000);
    refreshIntervalCombo->addItem("5秒", 5000);
    refreshIntervalCombo->addItem("10秒", 10000);
    refreshIntervalCombo->addItem("30秒", 30000);
    refreshIntervalCombo->setCurrentIndex(2); // 默认5秒
    connect(refreshIntervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onRefreshIntervalChanged);
    
    // 添加设置按钮
    QPushButton *settingsButton = new QPushButton("设置");
    settingsButton->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::showSettings);
    
    // 添加图表按钮
    QPushButton *chartButton = new QPushButton("图表");
    chartButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    chartButton->setToolTip("打开图表查看器");
    connect(chartButton, &QPushButton::clicked, this, &MainWindow::showChartDialog);
    
    ui->toolBar->addWidget(refreshButton);
    ui->toolBar->addSeparator();
    ui->toolBar->addWidget(new QLabel("刷新间隔:"));
    ui->toolBar->addWidget(refreshIntervalCombo);
    ui->toolBar->addSeparator();
    ui->toolBar->addWidget(chartButton);
    ui->toolBar->addWidget(settingsButton);
}

void MainWindow::setupStatusBar()
{
    // 创建状态标签，使用更清晰的图标和文字
    statusLabel = new QLabel("🟢 监控状态：正常运行");
    statusLabel->setStyleSheet(
        "QLabel {"
        "    color: #28a745;"
        "    font-weight: 600;"
        "    padding: 4px 12px;"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 rgba(40, 167, 69, 0.15),"
        "                                stop: 1 rgba(40, 167, 69, 0.05));"
        "    border: 1px solid rgba(40, 167, 69, 0.3);"
        "    border-radius: 6px;"
        "    margin: 2px;"
        "    font-size: 9pt;"
        "}"
    );
    statusBar()->addWidget(statusLabel);
     
     // 添加分隔符
     QFrame *separator1 = new QFrame();
     separator1->setFrameShape(QFrame::VLine);
     separator1->setFrameShadow(QFrame::Sunken);
     separator1->setStyleSheet("color: #dee2e6;");
     statusBar()->addPermanentWidget(separator1);
     
     // 添加刷新间隔选择 - 使用更清晰的标签
     QLabel *intervalLabel = new QLabel("⏱️ 数据刷新频率:");
     intervalLabel->setStyleSheet(
         "QLabel {"
         "    color: #495057;"
         "    font-weight: 600;"
         "    margin-right: 6px;"
         "    font-size: 9pt;"
         "}"
     );
     
     refreshIntervalCombo = new QComboBox();
     refreshIntervalCombo->addItem("⚡ 实时 (1秒)", 1000);
     refreshIntervalCombo->addItem("🚀 快速 (3秒)", 3000);
     refreshIntervalCombo->addItem("⭐ 标准 (5秒)", 5000);
     refreshIntervalCombo->addItem("🔄 节能 (10秒)", 10000);
     refreshIntervalCombo->addItem("⏳ 省电 (30秒)", 30000);
     refreshIntervalCombo->setCurrentIndex(2); // 默认5秒
     refreshIntervalCombo->setMinimumWidth(120);
     refreshIntervalCombo->setToolTip("选择磁盘监控数据的刷新频率\n频率越高，数据越实时，但会消耗更多系统资源");
     
     connect(refreshIntervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
             this, &MainWindow::onRefreshIntervalChanged);
     
     statusBar()->addPermanentWidget(intervalLabel);
     statusBar()->addPermanentWidget(refreshIntervalCombo);
     
     // 添加分隔符
     QFrame *separator2 = new QFrame();
     separator2->setFrameShape(QFrame::VLine);
     separator2->setFrameShadow(QFrame::Sunken);
     separator2->setStyleSheet("color: #dee2e6;");
     statusBar()->addPermanentWidget(separator2);
     
     // 添加手动刷新按钮 - 使用更明确的文字
     refreshButton = new QPushButton("🔄 立即刷新");
     refreshButton->setMinimumWidth(90);
     refreshButton->setMaximumWidth(110);
     refreshButton->setToolTip("立即刷新所有磁盘和进程数据");
     refreshButton->setStyleSheet(
         "QPushButton {"
         "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
         "                                stop: 0 #28a745, stop: 1 #1e7e34);"
         "    border: none;"
         "    color: white;"
         "    padding: 6px 12px;"
         "    border-radius: 6px;"
         "    font-weight: 600;"
         "    font-size: 9pt;"
         "    margin: 2px;"
         "}"
         "QPushButton:hover {"
         "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
         "                                stop: 0 #1e7e34, stop: 1 #155724);"
         "}"
         "QPushButton:pressed {"
         "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
         "                                stop: 0 #155724, stop: 1 #0d4017);"
         "}"
     );
     
     connect(refreshButton, &QPushButton::clicked, this, &MainWindow::updateDiskInfo);
     statusBar()->addPermanentWidget(refreshButton);
     
     // 添加版本信息和系统状态
     QLabel *versionLabel = new QLabel("💾 磁盘监控器 v1.0 | 实时监控系统");
     versionLabel->setStyleSheet(
         "QLabel {"
         "    color: #6c757d;"
         "    font-weight: 500;"
         "    font-size: 8pt;"
         "    padding: 2px 8px;"
         "    font-style: italic;"
         "}"
     );
     statusBar()->addPermanentWidget(versionLabel);
}

void MainWindow::initializeDriveMonitoring()
{
    // 获取所有可用驱动器
    QList<QStorageInfo> drives = QStorageInfo::mountedVolumes();
    
    driveInfoList.clear();
    
    for (const QStorageInfo &storage : drives) {
        if (storage.isValid() && storage.isReady()) {
            DriveInfo driveInfo;
            driveInfo.driveLetter = storage.rootPath();
            driveInfo.displayName = storage.displayName().isEmpty() ? 
                                   storage.rootPath() : storage.displayName();
            driveInfo.totalSpace = storage.bytesTotal();
            driveInfo.usedSpace = storage.bytesTotal() - storage.bytesAvailable();
            driveInfo.freeSpace = storage.bytesAvailable();
            driveInfo.usagePercentage = driveInfo.totalSpace > 0 ? 
                                       (double)driveInfo.usedSpace / driveInfo.totalSpace * 100.0 : 0.0;
            driveInfo.isMonitored = true;
            driveInfo.processCount = 0;
            driveInfo.criticalProcessCount = 0;
            
            driveInfoList.append(driveInfo);
        }
    }
    
    qInfo() << QString("[主窗口] 初始化完成，监控 %1 个驱动器").arg(driveInfoList.size());
}

void MainWindow::updateDiskInfo()
{
    statusLabel->setText("🔄 正在刷新磁盘数据...");
    statusLabel->setStyleSheet(
        "QLabel {"
        "    color: #ffc107;"
        "    font-weight: 600;"
        "    padding: 4px 12px;"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 rgba(255, 193, 7, 0.15),"
        "                                stop: 1 rgba(255, 193, 7, 0.05));"
        "    border: 1px solid rgba(255, 193, 7, 0.3);"
        "    border-radius: 6px;"
        "    margin: 2px;"
        "    font-size: 9pt;"
        "}"
    );
    
    // 更新驱动器信息
    int updatedCount = 0;
    for (DriveInfo &driveInfo : driveInfoList) {
        QStorageInfo storage(driveInfo.driveLetter);
        if (storage.isValid() && storage.isReady()) {
            driveInfo.totalSpace = storage.bytesTotal();
            driveInfo.usedSpace = storage.bytesTotal() - storage.bytesAvailable();
            driveInfo.freeSpace = storage.bytesAvailable();
            driveInfo.usagePercentage = driveInfo.totalSpace > 0 ? 
                                       (double)driveInfo.usedSpace / driveInfo.totalSpace * 100.0 : 0.0;
            
            // 扫描进程
            scanDriveProcesses(driveInfo);
            
            // 计算颜色
            driveInfo.statusColor = calculateDriveColor(driveInfo);
            updatedCount++;
        }
    }
    
    updateDriveDisplay();
    
    // 恢复正常状态显示
    statusLabel->setText(QString("✅ 数据更新完成 | 最后刷新：%1 | 已监控 %2 个驱动器")
                        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                        .arg(updatedCount));
    statusLabel->setStyleSheet(
        "QLabel {"
        "    color: #28a745;"
        "    font-weight: 600;"
        "    padding: 4px 12px;"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 rgba(40, 167, 69, 0.15),"
        "                                stop: 1 rgba(40, 167, 69, 0.05));"
        "    border: 1px solid rgba(40, 167, 69, 0.3);"
        "    border-radius: 6px;"
        "    margin: 2px;"
        "    font-size: 9pt;"
        "}"
    );
    
    // 发射信号，触发历史数据记录
    emit driveDataUpdated();
}

void MainWindow::updateDriveDisplay()
{
    // 检查driveLayout是否有效
    if (!driveLayout) {
        qCritical() << "[主窗口] 错误：driveLayout为空指针";
        return;
    }
    
    // 清除现有的驱动器显示
    QLayoutItem *child;
    while ((child = driveLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    
    // 为每个驱动器创建显示组件
    for (const DriveInfo &driveInfo : driveInfoList) {
        if (driveInfo.isMonitored) {
            QWidget *driveWidget = createDriveWidget(driveInfo);
            driveLayout->addWidget(driveWidget);
        }
    }
    
    driveLayout->addStretch();
}

QWidget* MainWindow::createDriveWidget(const DriveInfo& driveInfo)
{
    QFrame *frame = new QFrame();
    frame->setFrameStyle(QFrame::NoFrame);
    frame->setContentsMargins(12, 12, 12, 12);
    
    // 设置现代化的背景样式
    QString gradientColor1, gradientColor2, borderColor, shadowColor;
    
    if (driveInfo.usagePercentage < 50) {
        gradientColor1 = "rgba(76, 175, 80, 0.1)";
        gradientColor2 = "rgba(76, 175, 80, 0.05)";
        borderColor = "#4CAF50";
        shadowColor = "rgba(76, 175, 80, 0.2)";
    } else if (driveInfo.usagePercentage < 75) {
        gradientColor1 = "rgba(255, 193, 7, 0.1)";
        gradientColor2 = "rgba(255, 193, 7, 0.05)";
        borderColor = "#FFC107";
        shadowColor = "rgba(255, 193, 7, 0.2)";
    } else if (driveInfo.usagePercentage < 90) {
        gradientColor1 = "rgba(255, 152, 0, 0.1)";
        gradientColor2 = "rgba(255, 152, 0, 0.05)";
        borderColor = "#FF9800";
        shadowColor = "rgba(255, 152, 0, 0.2)";
    } else {
        gradientColor1 = "rgba(244, 67, 54, 0.1)";
        gradientColor2 = "rgba(244, 67, 54, 0.05)";
        borderColor = "#F44336";
        shadowColor = "rgba(244, 67, 54, 0.2)";
    }
    
    // 检查是否为当前选中的磁盘
    bool isSelected = (driveInfo.driveLetter.left(1).toUpper() == currentSelectedDrive.toUpper());
    
    QString styleSheet;
    if (isSelected) {
        // 选中状态的样式 - 更加突出
        styleSheet = QString(
            "QFrame {"
            "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "                                stop: 0 rgba(0, 123, 255, 0.25),"
            "                                stop: 1 rgba(0, 123, 255, 0.15));"
            "    border: 3px solid #007bff;"
            "    border-radius: 12px;"
            "    margin: 4px;"
            "    box-shadow: 0 4px 12px rgba(0, 123, 255, 0.3);"
            "}"
            "QFrame:hover {"
            "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "                                stop: 0 rgba(0, 123, 255, 0.35),"
            "                                stop: 1 rgba(0, 123, 255, 0.25));"
            "    border-color: #0056b3;"
            "    transform: translateY(-1px);"
            "}"
        );
    } else {
        // 未选中状态的样式
        styleSheet = QString(
            "QFrame {"
            "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "                                stop: 0 %1, stop: 1 %2);"
            "    border: 2px solid %3;"
            "    border-radius: 12px;"
            "    margin: 4px;"
            "}"
            "QFrame:hover {"
            "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "                                stop: 0 rgba(255, 255, 255, 0.9),"
            "                                stop: 1 %1);"
            "    border-color: %3;"
            "    transform: translateY(-2px);"
            "}"
        ).arg(gradientColor1, gradientColor2, borderColor);
    }
    
    frame->setStyleSheet(styleSheet);
    
    QVBoxLayout *layout = new QVBoxLayout(frame);
    layout->setSpacing(8);
    layout->setContentsMargins(4, 4, 4, 4);
    
    // 驱动器标题
    QHBoxLayout *titleLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel(QString("%1 (%2)")
                                   .arg(driveInfo.displayName)
                                   .arg(driveInfo.driveLetter));
    titleLabel->setFont(QFont(titleLabel->font().family(), 11, QFont::DemiBold));
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #2c3e50;"
        "    font-weight: 600;"
        "    padding: 2px 0;"
        "}"
    );
    
    // 添加驱动器类型图标（使用Unicode符号）
    QLabel *iconLabel = new QLabel();
    QString iconText = "💾"; // 磁盘图标
    if (driveInfo.driveLetter.contains("C:")) {
        iconText = "🖥️"; // 系统盘图标
    } else if (driveInfo.displayName.contains("USB") || driveInfo.displayName.contains("移动")) {
        iconText = "🔌"; // USB图标
    }
    iconLabel->setText(iconText);
    iconLabel->setStyleSheet("font-size: 16px; margin-right: 4px;");
    
    titleLayout->addWidget(iconLabel);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    
    // 使用情况进度条
    QProgressBar *progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue((int)driveInfo.usagePercentage);
    progressBar->setTextVisible(true);
    progressBar->setFormat(QString("%1%").arg(QString::number(driveInfo.usagePercentage, 'f', 1)));
    progressBar->setFixedHeight(20);
    
    // 根据使用率设置现代化进度条样式
    QString progressColor, progressBg;
    if (driveInfo.usagePercentage < 50) {
        progressColor = "#4CAF50";
        progressBg = "rgba(76, 175, 80, 0.1)";
    } else if (driveInfo.usagePercentage < 75) {
        progressColor = "#FFC107";
        progressBg = "rgba(255, 193, 7, 0.1)";
    } else if (driveInfo.usagePercentage < 90) {
        progressColor = "#FF9800";
        progressBg = "rgba(255, 152, 0, 0.1)";
    } else {
        progressColor = "#F44336";
        progressBg = "rgba(244, 67, 54, 0.1)";
    }
    
    QString progressStyle = QString(
        "QProgressBar {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 10px;"
        "    background: %1;"
        "    text-align: center;"
        "    font-weight: 600;"
        "    font-size: 9pt;"
        "    color: #495057;"
        "}"
        "QProgressBar::chunk {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 %2, stop: 1 %2);"
        "    border-radius: 9px;"
        "    margin: 1px;"
        "}"
    ).arg(progressBg, progressColor);
    
    progressBar->setStyleSheet(progressStyle);
    
    // 空间信息 - 使用更清晰和美观的布局
    QVBoxLayout *spaceInfoLayout = new QVBoxLayout();
    
    // 第一行：已用空间和总容量
    QHBoxLayout *spaceLayout1 = new QHBoxLayout();
    
    QLabel *usedLabel = new QLabel(QString("📊 已使用：%1").arg(formatBytes(driveInfo.usedSpace)));
    usedLabel->setStyleSheet(
        "QLabel {"
        "    color: #495057;"
        "    font-size: 9pt;"
        "    font-weight: 600;"
        "    padding: 3px 8px;"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 rgba(108, 117, 125, 0.15),"
        "                                stop: 1 rgba(108, 117, 125, 0.05));"
        "    border: 1px solid rgba(108, 117, 125, 0.2);"
        "    border-radius: 5px;"
        "}"
    );
    
    QLabel *totalLabel = new QLabel(QString("💽 总容量：%1").arg(formatBytes(driveInfo.totalSpace)));
    totalLabel->setStyleSheet(
        "QLabel {"
        "    color: #495057;"
        "    font-size: 9pt;"
        "    font-weight: 600;"
        "    padding: 3px 8px;"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 rgba(73, 80, 87, 0.15),"
        "                                stop: 1 rgba(73, 80, 87, 0.05));"
        "    border: 1px solid rgba(73, 80, 87, 0.2);"
        "    border-radius: 5px;"
        "}"
    );
    
    spaceLayout1->addWidget(usedLabel);
    spaceLayout1->addWidget(totalLabel);
    spaceLayout1->addStretch();
    
    // 第二行：可用空间和使用率状态
    QHBoxLayout *spaceLayout2 = new QHBoxLayout();
    
    QLabel *freeLabel = new QLabel(QString("💾 可用空间：%1").arg(formatBytes(driveInfo.freeSpace)));
    QString freeColor = driveInfo.usagePercentage > 90 ? "#dc3545" : "#28a745";
    QString freeBgColor = driveInfo.usagePercentage > 90 ? "rgba(220, 53, 69, 0.1)" : "rgba(40, 167, 69, 0.1)";
    QString freeBorderColor = driveInfo.usagePercentage > 90 ? "rgba(220, 53, 69, 0.3)" : "rgba(40, 167, 69, 0.3)";
    freeLabel->setStyleSheet(QString(
        "QLabel {"
        "    color: %1;"
        "    font-size: 9pt;"
        "    font-weight: 600;"
        "    padding: 3px 8px;"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 %2, stop: 1 rgba(255, 255, 255, 0.1));"
        "    border: 1px solid %3;"
        "    border-radius: 5px;"
        "}"
    ).arg(freeColor, freeBgColor, freeBorderColor));
    
    // 添加磁盘健康状态指示
    QLabel *statusLabel = new QLabel();
    QString statusText, statusColor, statusBg;
    if (driveInfo.usagePercentage < 70) {
        statusText = "✅ 空间充足";
        statusColor = "#28a745";
        statusBg = "rgba(40, 167, 69, 0.1)";
    } else if (driveInfo.usagePercentage < 85) {
        statusText = "⚠️ 空间紧张";
        statusColor = "#ffc107";
        statusBg = "rgba(255, 193, 7, 0.1)";
    } else if (driveInfo.usagePercentage < 95) {
        statusText = "🔶 空间不足";
        statusColor = "#fd7e14";
        statusBg = "rgba(253, 126, 20, 0.1)";
    } else {
        statusText = "🔴 空间严重不足";
        statusColor = "#dc3545";
        statusBg = "rgba(220, 53, 69, 0.1)";
    }
    
    statusLabel->setText(statusText);
    statusLabel->setStyleSheet(QString(
        "QLabel {"
        "    color: %1;"
        "    font-size: 9pt;"
        "    font-weight: 600;"
        "    padding: 3px 8px;"
        "    background: %2;"
        "    border: 1px solid %1;"
        "    border-radius: 5px;"
        "}"
    ).arg(statusColor, statusBg));
    
    spaceLayout2->addWidget(freeLabel);
    spaceLayout2->addWidget(statusLabel);
    spaceLayout2->addStretch();
    
    spaceInfoLayout->addLayout(spaceLayout1);
    spaceInfoLayout->addLayout(spaceLayout2);
    
    // 进程信息 - 使用更详细和美观的样式
    QHBoxLayout *processLayout = new QHBoxLayout();
    
    QLabel *processIconLabel = new QLabel("⚙️");
    processIconLabel->setStyleSheet("font-size: 14px; margin-right: 6px;");
    
    QLabel *processCountLabel = new QLabel(QString("活跃进程：%1 个").arg(driveInfo.processCount));
    processCountLabel->setStyleSheet(
        "QLabel {"
        "    color: #0056b3;"
        "    font-size: 9pt;"
        "    font-weight: 600;"
        "    padding: 3px 8px;"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 rgba(0, 123, 255, 0.15),"
        "                                stop: 1 rgba(0, 123, 255, 0.05));"
        "    border: 1px solid rgba(0, 123, 255, 0.3);"
        "    border-radius: 5px;"
        "}"
    );
    
    QLabel *criticalLabel = new QLabel(QString("系统关键：%1 个").arg(driveInfo.criticalProcessCount));
    QString criticalColor = driveInfo.criticalProcessCount > 0 ? "#dc3545" : "#6c757d";
    QString criticalBg = driveInfo.criticalProcessCount > 0 ? "rgba(220, 53, 69, 0.1)" : "rgba(108, 117, 125, 0.1)";
    QString criticalBorder = driveInfo.criticalProcessCount > 0 ? "rgba(220, 53, 69, 0.3)" : "rgba(108, 117, 125, 0.2)";
    criticalLabel->setStyleSheet(QString(
        "QLabel {"
        "    color: %1;"
        "    font-size: 9pt;"
        "    font-weight: 600;"
        "    padding: 3px 8px;"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 %2, stop: 1 rgba(255, 255, 255, 0.1));"
        "    border: 1px solid %3;"
        "    border-radius: 5px;"
        "}"
    ).arg(criticalColor, criticalBg, criticalBorder));
    
    processLayout->addWidget(processIconLabel);
     processLayout->addWidget(processCountLabel);
     processLayout->addWidget(criticalLabel);
     processLayout->addStretch();
     
     // 组装所有布局
     layout->addLayout(titleLayout);
     layout->addWidget(progressBar);
     layout->addLayout(spaceInfoLayout);
     layout->addLayout(processLayout);
     
     // 添加点击事件处理
     frame->setProperty("driveLetter", driveInfo.driveLetter.left(1).toUpper());
     frame->installEventFilter(this);
     frame->setCursor(Qt::PointingHandCursor);
     frame->setToolTip(QString("点击查看驱动器 %1 的详细进程信息\n\n"
                              "💽 总容量：%2\n"
                              "📊 已使用：%3 (%4%)\n"
                              "💾 可用空间：%5\n"
                              "⚙️ 活跃进程：%6 个\n"
                              "🔴 系统关键进程：%7 个")
                      .arg(driveInfo.driveLetter)
                      .arg(formatBytes(driveInfo.totalSpace))
                      .arg(formatBytes(driveInfo.usedSpace))
                      .arg(QString::number(driveInfo.usagePercentage, 'f', 1))
                      .arg(formatBytes(driveInfo.freeSpace))
                      .arg(driveInfo.processCount)
                      .arg(driveInfo.criticalProcessCount));
     
     return frame;
    
    // 添加一些间距
    layout->addStretch();
    
    // 添加点击事件 - 使用自定义Frame类
    frame->setProperty("driveLetter", driveInfo.driveLetter);
    frame->installEventFilter(this);
    
    return frame;
}

void MainWindow::scanDriveProcesses(DriveInfo& driveInfo)
{
    if (!processMonitor) {
        return;
    }
    
    // 获取驱动器统计信息
    DriveProcessStats stats = processMonitor->getDriveStats(driveInfo.driveLetter);
    
    // 更新驱动器信息
    driveInfo.topProcesses = stats.processes;
    driveInfo.totalDiskReadBytes = stats.totalReadBytes;
    driveInfo.totalDiskWriteBytes = stats.totalWriteBytes;
    driveInfo.processCount = stats.activeProcessCount;
    driveInfo.criticalProcessCount = stats.criticalProcessCount;
}

QColor MainWindow::calculateDriveColor(const DriveInfo& driveInfo)
{
    // 基于使用率和进程重要性计算颜色
    double usageWeight = driveInfo.usagePercentage / 100.0;
    double processWeight = driveInfo.processCount > 0 ? 
                          (double)driveInfo.criticalProcessCount / driveInfo.processCount : 0.0;
    
    double combinedWeight = (usageWeight * 0.7) + (processWeight * 0.3);
    
    if (combinedWeight < 0.3) {
        return lowUsageColor;
    } else if (combinedWeight < 0.6) {
        return mediumUsageColor;
    } else if (combinedWeight < 0.8) {
        return highUsageColor;
    } else {
        return criticalUsageColor;
    }
}

QString MainWindow::formatBytes(qint64 bytes)
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    const qint64 TB = GB * 1024;
    
    if (bytes >= TB) {
        return QString("%1 TB").arg(QString::number((double)bytes / TB, 'f', 2));
    } else if (bytes >= GB) {
        return QString("%1 GB").arg(QString::number((double)bytes / GB, 'f', 2));
    } else if (bytes >= MB) {
        return QString("%1 MB").arg(QString::number((double)bytes / MB, 'f', 2));
    } else if (bytes >= KB) {
        return QString("%1 KB").arg(QString::number((double)bytes / KB, 'f', 2));
    } else {
        return QString("%1 B").arg(bytes);
    }
}

void MainWindow::updateProcessList(const QString &driveLetter)
{
    if (!processTreeWidget || !processMonitor) {
        qWarning() << "[主窗口] 关键组件为空，无法更新进程列表";
        return;
    }

    // 临时禁用排序以提高性能
    bool sortingWasEnabled = processTreeWidget->isSortingEnabled();
    processTreeWidget->setSortingEnabled(false);
    
    processTreeWidget->clear();
    currentSelectedDrive = driveLetter;
    
    // 更新进程信息标签，显示当前选中的磁盘
    if (ui->processInfoLabel) {
        QString driveDisplayName = QString("驱动器 %1:").arg(driveLetter.toUpper());
        
        // 查找对应的驱动器信息以获取更详细的显示名称
        for (const auto& driveInfo : driveInfoList) {
            if (driveInfo.driveLetter.left(1).toUpper() == driveLetter.toUpper()) {
                driveDisplayName = QString("🔍 当前选中：%1 (%2:)").arg(driveInfo.displayName).arg(driveLetter.toUpper());
                break;
            }
        }
        
        ui->processInfoLabel->setText(driveDisplayName);
        ui->processInfoLabel->setStyleSheet(
            "QLabel {"
            "    color: #007bff;"
            "    font-weight: 700;"
            "    font-size: 11pt;"
            "    padding: 8px 12px;"
            "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "                                stop: 0 rgba(0, 123, 255, 0.15),"
            "                                stop: 1 rgba(0, 123, 255, 0.05));"
            "    border: 2px solid rgba(0, 123, 255, 0.3);"
            "    border-radius: 8px;"
            "    margin: 4px 0;"
            "}"
        );
    }
    
    // 显示加载提示
    ProcessTreeWidgetItem *loadingItem = new ProcessTreeWidgetItem(processTreeWidget);
    loadingItem->setText(0, "正在加载进程信息...");
    loadingItem->setText(1, "");
    loadingItem->setText(2, "");
    loadingItem->setText(3, "");
    loadingItem->setText(4, "");
    loadingItem->setText(5, "");
    loadingItem->setText(6, "");
    loadingItem->setText(7, "");
    loadingItem->setText(8, "");

    // 通过信号槽异步请求驱动器统计信息
    QMetaObject::invokeMethod(processMonitor, "requestDriveStats", 
                              Qt::QueuedConnection, Q_ARG(QString, driveLetter));
    
    // 刷新磁盘显示以更新选中状态
    updateDriveDisplay();
    
    // 重新启用排序功能
    if (sortingWasEnabled) {
        processTreeWidget->setSortingEnabled(true);
        // 保持当前的排序状态
        int currentSortColumn = processTreeWidget->header()->sortIndicatorSection();
        Qt::SortOrder currentSortOrder = processTreeWidget->header()->sortIndicatorOrder();
        if (currentSortColumn >= 0) {
            processTreeWidget->sortByColumn(currentSortColumn, currentSortOrder);
        }
    }
}

void MainWindow::updateProcessListWithStats(const QString &driveLetter, const DriveProcessStats &stats)
{
    if (currentSelectedDrive != driveLetter || !processTreeWidget) {
        return;
    }
    
    // 临时禁用排序以提高性能
    bool sortingWasEnabled = processTreeWidget->isSortingEnabled();
    processTreeWidget->setSortingEnabled(false);
    
    // 更新进程信息标签，确保显示当前选中的磁盘
    if (ui->processInfoLabel) {
        QString driveDisplayName = QString("驱动器 %1:").arg(driveLetter.toUpper());
        
        // 查找对应的驱动器信息以获取更详细的显示名称
        for (const auto& driveInfo : driveInfoList) {
            if (driveInfo.driveLetter.left(1).toUpper() == driveLetter.toUpper()) {
                driveDisplayName = QString("🔍 当前选中：%1 (%2:) - 共 %3 个进程")
                    .arg(driveInfo.displayName)
                    .arg(driveLetter.toUpper())
                    .arg(stats.processes.size());
                break;
            }
        }
        
        ui->processInfoLabel->setText(driveDisplayName);
        ui->processInfoLabel->setStyleSheet(
            "QLabel {"
            "    color: #007bff;"
            "    font-weight: 700;"
            "    font-size: 11pt;"
            "    padding: 8px 12px;"
            "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "                                stop: 0 rgba(0, 123, 255, 0.15),"
            "                                stop: 1 rgba(0, 123, 255, 0.05));"
            "    border: 2px solid rgba(0, 123, 255, 0.3);"
            "    border-radius: 8px;"
            "    margin: 4px 0;"
            "}"
        );
    }
    
    // 清空之前的内容（包括加载提示）
    processTreeWidget->clear();
    
    // 如果没有进程数据，显示提示信息
    if (stats.processes.isEmpty()) {
        ProcessTreeWidgetItem *noDataItem = new ProcessTreeWidgetItem(processTreeWidget);
        noDataItem->setText(0, QString("驱动器 %1 暂无进程数据").arg(driveLetter));
        noDataItem->setText(1, "可能原因：权限不足或驱动器无活动进程");
        noDataItem->setText(2, "");
        noDataItem->setText(3, "");
        noDataItem->setText(4, "");
        noDataItem->setText(5, "");
        noDataItem->setText(6, "");
        noDataItem->setText(7, "");
        noDataItem->setText(8, "");
        
        // 设置提示文字颜色
        noDataItem->setForeground(0, QBrush(QColor(128, 128, 128)));
        noDataItem->setForeground(1, QBrush(QColor(128, 128, 128)));
        return;
    }
    
    // 添加进程信息到树控件
    for (const auto &process : stats.processes) {
        ProcessTreeWidgetItem *item = new ProcessTreeWidgetItem(processTreeWidget);
        item->setText(0, process.processName);  // 进程名称
        item->setText(1, QString::number(process.processId)); // PID
        item->setText(2, formatDiskUsage(process.diskReadBytes)); // 磁盘读取
        item->setText(3, formatDiskUsage(process.diskWriteBytes)); // 磁盘写入
        item->setText(4, formatDiskUsage(process.diskReadBytes + process.diskWriteBytes)); // 总磁盘使用
        item->setText(5, QString("%1%").arg(process.cpuUsage, 0, 'f', 1)); // CPU使用率
        item->setText(6, formatBytes(process.workingSetSize)); // 内存使用
        item->setText(7, process.userName); // 用户
        
        // 设置数值数据用于正确排序（使用UserRole存储原始数值）
        item->setData(1, Qt::UserRole, static_cast<qulonglong>(process.processId)); // PID数值
        item->setData(2, Qt::UserRole, static_cast<qulonglong>(process.diskReadBytes)); // 磁盘读取数值
        item->setData(3, Qt::UserRole, static_cast<qulonglong>(process.diskWriteBytes)); // 磁盘写入数值
        item->setData(4, Qt::UserRole, static_cast<qulonglong>(process.diskReadBytes + process.diskWriteBytes)); // 总磁盘使用数值
        item->setData(5, Qt::UserRole, process.cpuUsage); // CPU使用率数值
        item->setData(6, Qt::UserRole, static_cast<qulonglong>(process.workingSetSize)); // 内存使用数值
        
        // 设置重要程度和颜色
        QString importance;
        QColor textColor = Qt::black;
        int importanceLevel = 0; // 用于排序的数值：0=普通，1=高，2=系统
        
        if (process.isCriticalProcess) {
            importance = "系统";
            importanceLevel = 2;
            textColor = QColor(255, 100, 100); // 红色表示系统进程
        } else if (process.cpuUsage > 50.0 || (process.diskReadBytes + process.diskWriteBytes) > 50 * 1024 * 1024) {
            importance = "高";
            importanceLevel = 1;
            textColor = QColor(255, 140, 0); // 橙色表示高负载进程
        } else {
            importance = "普通";
            importanceLevel = 0;
        }
        
        item->setText(8, importance);
        item->setData(8, Qt::UserRole, importanceLevel); // 设置重要性数值用于排序
        item->setForeground(0, QBrush(textColor));
        
        // 存储进程信息用于右键菜单
        item->setData(0, Qt::UserRole, QVariant::fromValue(process));
    }
    
    // 调整列宽
    for (int i = 0; i < processTreeWidget->columnCount(); ++i) {
        processTreeWidget->resizeColumnToContents(i);
    }
    
    // 重新启用排序功能
    if (sortingWasEnabled) {
        processTreeWidget->setSortingEnabled(true);
        // 保持当前的排序状态
        int currentSortColumn = processTreeWidget->header()->sortIndicatorSection();
        Qt::SortOrder currentSortOrder = processTreeWidget->header()->sortIndicatorOrder();
        if (currentSortColumn >= 0) {
            processTreeWidget->sortByColumn(currentSortColumn, currentSortOrder);
        }
    }
}

void MainWindow::onDriveSelectionChanged()
{
    // 处理驱动器选择变化
}

void MainWindow::onRefreshIntervalChanged()
{
    int interval = refreshIntervalCombo->currentData().toInt();
    refreshTimer->setInterval(interval);
    statusLabel->setText(QString("刷新间隔已设置为 %1").arg(refreshIntervalCombo->currentText()));
    
    // 保存刷新间隔设置
    settings->setValue("Monitor/refreshInterval", interval);
    settings->sync();
    qInfo() << QString("[主窗口] 刷新间隔已更改为 %1 毫秒").arg(interval);
}

void MainWindow::onProcessItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column)
    if (item) {
        ProcessInfo process = item->data(0, Qt::UserRole).value<ProcessInfo>();
        openProcessLocation(process.executablePath);
    }
}

void MainWindow::openProcessLocation(const QString& processPath)
{
    if (processPath.isEmpty()) {
        QMessageBox::information(this, "提示", "无法获取进程的可执行文件路径。");
        return;
    }
    
    QFileInfo fileInfo(processPath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "错误", "进程的可执行文件不存在或已被删除。");
        return;
    }
    
    // 在Windows资源管理器中选中文件
#ifdef Q_OS_WIN
    QString command = QString("explorer.exe /select,\"%1\"").arg(QDir::toNativeSeparators(processPath));
    QProcess::startDetached(command);
#else
    // 非Windows系统，打开包含文件的目录
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
#endif
}

void MainWindow::refreshData()
{
    updateDiskInfo();
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "关于磁盘监控器", 
                      "磁盘监控器 v1.0\n\n"
                      "一个实时监控磁盘使用情况的Qt应用程序\n\n"
                      "功能特性:\n"
                      "• 实时监控磁盘使用率\n"
                      "• 显示相关进程信息\n"
                      "• 根据使用情况智能着色\n"
                      "• 可自定义刷新间隔\n\n"
                      "开发者: Qt开发团队");
}

void MainWindow::showSettings()
{
    QDialog settingsDialog(this);
    settingsDialog.setWindowTitle("设置");
    settingsDialog.setModal(true);
    settingsDialog.resize(450, 500);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(&settingsDialog);
    
    // 驱动器监控设置组
    QGroupBox *driveGroup = new QGroupBox("驱动器监控设置");
    QVBoxLayout *driveLayout = new QVBoxLayout(driveGroup);
    
    QLabel *driveLabel = new QLabel("选择要监控的驱动器:");
    driveLayout->addWidget(driveLabel);
    
    // 创建驱动器复选框列表
    QList<QCheckBox*> driveCheckBoxes;
    for (auto &driveInfo : driveInfoList) {
        QCheckBox *checkBox = new QCheckBox(QString("%1 (%2)")
                                           .arg(driveInfo.displayName)
                                           .arg(driveInfo.driveLetter));
        checkBox->setChecked(driveInfo.isMonitored);
        checkBox->setProperty("driveLetter", driveInfo.driveLetter);
        driveCheckBoxes.append(checkBox);
        driveLayout->addWidget(checkBox);
    }
    
    mainLayout->addWidget(driveGroup);
    
    // 数据保留期限设置组
    QGroupBox *retentionGroup = new QGroupBox("数据保留期限设置");
    QVBoxLayout *retentionLayout = new QVBoxLayout(retentionGroup);
    
    QLabel *retentionLabel = new QLabel("选择历史数据保留期限:");
    retentionLayout->addWidget(retentionLabel);
    
    QComboBox *retentionComboBox = new QComboBox();
    retentionComboBox->addItem("一周", static_cast<int>(RetentionPolicy::OneWeek));
    retentionComboBox->addItem("一个月", static_cast<int>(RetentionPolicy::OneMonth));
    retentionComboBox->addItem("三个月", static_cast<int>(RetentionPolicy::ThreeMonths));
    retentionComboBox->addItem("六个月", static_cast<int>(RetentionPolicy::SixMonths));
    retentionComboBox->addItem("一年", static_cast<int>(RetentionPolicy::OneYear));
    retentionComboBox->addItem("永久保留", static_cast<int>(RetentionPolicy::Forever));
    
    // 设置当前选中的保留策略
    if (historyManager) {
        RetentionPolicy currentPolicy = historyManager->getRetentionPolicy();
        for (int i = 0; i < retentionComboBox->count(); ++i) {
            if (retentionComboBox->itemData(i).toInt() == static_cast<int>(currentPolicy)) {
                retentionComboBox->setCurrentIndex(i);
                break;
            }
        }
    }
    
    retentionLayout->addWidget(retentionComboBox);
    
    // 添加数据库大小信息
    QLabel *dbSizeLabel = new QLabel();
    if (historyManager) {
        qint64 dbSize = historyManager->getDatabaseSize();
        QString sizeText = QString("当前数据库大小: %1").arg(formatBytes(dbSize));
        dbSizeLabel->setText(sizeText);
    } else {
        dbSizeLabel->setText("数据库大小: 未知");
    }
    dbSizeLabel->setStyleSheet("QLabel { color: #666666; font-style: italic; }");
    retentionLayout->addWidget(dbSizeLabel);
    
    // 添加立即清理按钮
    QPushButton *cleanupButton = new QPushButton("立即清理过期数据");
    cleanupButton->setToolTip("根据当前保留策略清理过期的历史数据");
    connect(cleanupButton, &QPushButton::clicked, [&]() {
        if (historyManager) {
            bool success = historyManager->cleanupOldData();
            if (success) {
                QMessageBox::information(&settingsDialog, "成功", "过期数据清理完成！");
                // 更新数据库大小显示
                qint64 newSize = historyManager->getDatabaseSize();
                QString newSizeText = QString("当前数据库大小: %1").arg(formatBytes(newSize));
                dbSizeLabel->setText(newSizeText);
            } else {
                QMessageBox::warning(&settingsDialog, "错误", 
                    QString("清理失败: %1").arg(historyManager->getLastError()));
            }
        }
    });
    retentionLayout->addWidget(cleanupButton);
    
    mainLayout->addWidget(retentionGroup);
    
    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("确定");
    QPushButton *cancelButton = new QPushButton("取消");
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 连接按钮信号
    connect(okButton, &QPushButton::clicked, [&]() {
        // 更新驱动器监控状态
        for (QCheckBox *checkBox : driveCheckBoxes) {
            QString driveLetter = checkBox->property("driveLetter").toString();
            bool isChecked = checkBox->isChecked();
            
            for (auto &driveInfo : driveInfoList) {
                if (driveInfo.driveLetter == driveLetter) {
                    driveInfo.isMonitored = isChecked;
                    break;
                }
            }
        }
        
        // 更新进程监控器的驱动器过滤器
        QStringList monitoredDrives;
        for (const auto &driveInfo : driveInfoList) {
            if (driveInfo.isMonitored) {
                monitoredDrives.append(driveInfo.driveLetter);
            }
        }
        
        if (processMonitor) {
            QMetaObject::invokeMethod(processMonitor, "setDriveFilter", 
                                      Qt::QueuedConnection, Q_ARG(QStringList, monitoredDrives));
        } else {
            qWarning() << "[主窗口] 进程监控器对象为空";
        }
        
        // 保存数据保留期限设置
        if (historyManager) {
            int selectedPolicy = retentionComboBox->currentData().toInt();
            historyManager->setRetentionPolicy(static_cast<RetentionPolicy>(selectedPolicy));
        }
        
        // 刷新显示
        updateDriveDisplay();
        
        settingsDialog.accept();
    });
    
    connect(cancelButton, &QPushButton::clicked, &settingsDialog, &QDialog::reject);
    
    settingsDialog.exec();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame *frame = qobject_cast<QFrame*>(obj);
        if (frame) {
            if (frame->property("driveLetter").isValid()) {
                QString driveLetter = frame->property("driveLetter").toString();
                updateProcessList(driveLetter);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// 初始化进程监控器
void MainWindow::initializeProcessMonitor()
{
    // 创建工作线程
    monitorThread = new QThread(this);
    processMonitor = new ProcessMonitor();
    
    // 将ProcessMonitor移到工作线程
    processMonitor->moveToThread(monitorThread);
    
    // 连接信号槽 - 使用Qt::QueuedConnection确保跨线程安全
    connect(processMonitor, &ProcessMonitor::processDataUpdated,
            this, &MainWindow::onProcessDataUpdated, Qt::QueuedConnection);
    connect(processMonitor, &ProcessMonitor::driveStatsUpdated,
            this, &MainWindow::onDriveStatsUpdated, Qt::QueuedConnection);
    connect(processMonitor, &ProcessMonitor::errorOccurred,
            this, &MainWindow::onProcessMonitorError, Qt::QueuedConnection);
    connect(processMonitor, &ProcessMonitor::processStarted,
            this, &MainWindow::onProcessStarted, Qt::QueuedConnection);
    connect(processMonitor, &ProcessMonitor::processTerminated,
            this, &MainWindow::onProcessTerminated, Qt::QueuedConnection);
    
    // 连接线程生命周期信号
    connect(monitorThread, &QThread::started, [this]() {
        // 设置要监控的驱动器
        QStringList driveLetters;
        for (const auto& driveInfo : driveInfoList) {
            if (driveInfo.isMonitored) {
                // 提取驱动器字母（去掉路径分隔符）
                QString driveLetter = driveInfo.driveLetter.left(1).toUpper();
                if (!driveLetter.isEmpty() && !driveLetters.contains(driveLetter)) {
                    driveLetters.append(driveLetter);
                }
            }
        }
        
        QMetaObject::invokeMethod(processMonitor, "setDriveFilter", 
                                  Qt::QueuedConnection, Q_ARG(QStringList, driveLetters));
        
        QMetaObject::invokeMethod(processMonitor, "startMonitoring", Qt::QueuedConnection);
    });
    
    connect(monitorThread, &QThread::finished, processMonitor, &QObject::deleteLater);
    
    // 启动工作线程
    monitorThread->start();
}

// 进程数据更新槽函数
void MainWindow::onProcessDataUpdated()
{
    // 如果当前有选中的驱动器，更新进程列表
    if (!currentSelectedDrive.isEmpty()) {
        updateProcessList(currentSelectedDrive);
    }
}

// 驱动器统计更新槽函数
void MainWindow::onDriveStatsUpdated(const QString &driveLetter, const DriveProcessStats &stats)
{
    // 静默处理，减少日志输出
    // 更新对应驱动器的统计信息
    bool driveUpdated = false;
    for (auto &driveInfo : driveInfoList) {
        QString driveInfoLetter = driveInfo.driveLetter.left(1).toUpper();
        if (driveInfoLetter == driveLetter.toUpper()) {
            driveInfo.topProcesses = stats.processes;
            driveInfo.totalDiskReadBytes = stats.totalReadBytes;
            driveInfo.totalDiskWriteBytes = stats.totalWriteBytes;
            driveInfo.processCount = stats.processes.size();
            
            // 计算关键进程数量
             driveInfo.criticalProcessCount = 0;
             for (const auto &process : stats.processes) {
                 if (process.isCriticalProcess) {
                     driveInfo.criticalProcessCount++;
                 }
             }
            driveUpdated = true;
            // 驱动器统计信息已更新
            break;
        }
    }
    
    // 如果驱动器信息已更新，刷新驱动器显示界面
    if (driveUpdated) {
        updateDriveDisplay();
    }
    
    // 如果是当前选中的驱动器，直接更新进程列表显示
    if (currentSelectedDrive == driveLetter) {
        updateProcessListWithStats(driveLetter, stats);
    }
}

// 进程监控错误槽函数
void MainWindow::onProcessMonitorError(const QString &error)
{
    if (statusLabel) {
        statusLabel->setText(QString("❌ 进程监控错误：%1").arg(error));
        statusLabel->setStyleSheet(
            "QLabel {"
            "    color: #dc3545;"
            "    font-weight: 600;"
            "    padding: 4px 12px;"
            "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "                                stop: 0 rgba(220, 53, 69, 0.15),"
            "                                stop: 1 rgba(220, 53, 69, 0.05));"
            "    border: 1px solid rgba(220, 53, 69, 0.3);"
            "    border-radius: 6px;"
            "    margin: 2px;"
            "    font-size: 9pt;"
            "}"
        );
    }
    // 错误信息已在状态栏显示，无需重复记录
}

// 进程启动槽函数
void MainWindow::onProcessStarted(const ProcessInfo &process)
{
    if (statusLabel) {
        statusLabel->setText(QString("🚀 新进程启动：%1 (PID: %2)").arg(process.processName).arg(process.processId));
        statusLabel->setStyleSheet(
            "QLabel {"
            "    color: #17a2b8;"
            "    font-weight: 600;"
            "    padding: 4px 12px;"
            "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "                                stop: 0 rgba(23, 162, 184, 0.15),"
            "                                stop: 1 rgba(23, 162, 184, 0.05));"
            "    border: 1px solid rgba(23, 162, 184, 0.3);"
            "    border-radius: 6px;"
            "    margin: 2px;"
            "    font-size: 9pt;"
            "}"
        );
    }
}

// 进程终止槽函数
void MainWindow::onProcessTerminated(quint32 pid)
{
    if (statusLabel) {
        statusLabel->setText(QString("⚠️ 进程已终止：PID %1").arg(pid));
        statusLabel->setStyleSheet(
            "QLabel {"
            "    color: #fd7e14;"
            "    font-weight: 600;"
            "    padding: 4px 12px;"
            "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "                                stop: 0 rgba(253, 126, 20, 0.15),"
            "                                stop: 1 rgba(253, 126, 20, 0.05));"
            "    border: 1px solid rgba(253, 126, 20, 0.3);"
            "    border-radius: 6px;"
            "    margin: 2px;"
            "    font-size: 9pt;"
            "}"
        );
    }
}

// 显示进程右键菜单
void MainWindow::showProcessContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = processTreeWidget->itemAt(pos);
    if (item && processContextMenu) {
        processContextMenu->exec(processTreeWidget->mapToGlobal(pos));
    }
}

// 终止选中的进程
void MainWindow::terminateSelectedProcess()
{
    QTreeWidgetItem *item = processTreeWidget->currentItem();
    if (!item) {
        return;
    }
    
    ProcessInfo process = item->data(0, Qt::UserRole).value<ProcessInfo>();
    
    // 确认对话框
    int ret = QMessageBox::warning(this, "确认终止进程",
                                   QString("确定要终止进程 %1 (PID: %2) 吗？\n\n警告：终止系统进程可能导致系统不稳定！")
                                   .arg(process.processName).arg(process.processId),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (processMonitor && processMonitor->terminateProcess(process.processId)) {
            statusLabel->setText(QString("已终止进程: %1 (PID: %2)").arg(process.processName).arg(process.processId));
        } else {
            QMessageBox::critical(this, "错误", "无法终止进程，可能权限不足或进程已不存在。");
        }
    }
}

// 显示进程详细信息
void MainWindow::showProcessDetails()
{
    QTreeWidgetItem *item = processTreeWidget->currentItem();
    if (!item) {
        QMessageBox::information(this, "💡 提示", "请先选择一个进程查看详细信息");
        return;
    }
    
    ProcessInfo process = item->data(0, Qt::UserRole).value<ProcessInfo>();
    
    // 创建自定义对话框
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(QString("📋 进程详细信息 - %1").arg(process.processName));
    dialog->setFixedSize(500, 450);
    dialog->setModal(true);
    
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);
    
    // 标题区域
    QHBoxLayout *titleLayout = new QHBoxLayout();
    QLabel *iconLabel = new QLabel("🔧");
    iconLabel->setStyleSheet("font-size: 24px;");
    iconLabel->setFixedSize(32, 32);
    iconLabel->setAlignment(Qt::AlignCenter);
    
    QLabel *titleLabel = new QLabel(QString("<h2 style='color: #2c3e50; margin: 0;'>%1</h2>").arg(process.processName));
    titleLabel->setWordWrap(true);
    
    titleLayout->addWidget(iconLabel);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    
    // 详细信息区域
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    
    QWidget *infoWidget = new QWidget();
    QVBoxLayout *infoLayout = new QVBoxLayout(infoWidget);
    infoLayout->setSpacing(8);
    
    // 添加详细信息项
    auto addInfoItem = [&](const QString &icon, const QString &label, const QString &value, const QString &color = "#2c3e50") {
        QFrame *itemFrame = new QFrame();
        itemFrame->setStyleSheet(
            "QFrame {"
            "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "                                stop: 0 rgba(52, 152, 219, 0.05),"
            "                                stop: 1 rgba(52, 152, 219, 0.02));"
            "    border: 1px solid rgba(52, 152, 219, 0.15);"
            "    border-radius: 6px;"
            "    padding: 8px;"
            "    margin: 2px;"
            "}"
        );
        
        QHBoxLayout *itemLayout = new QHBoxLayout(itemFrame);
        itemLayout->setContentsMargins(8, 6, 8, 6);
        
        QLabel *iconLbl = new QLabel(icon);
        iconLbl->setFixedWidth(25);
        iconLbl->setAlignment(Qt::AlignCenter);
        iconLbl->setStyleSheet("font-size: 14px;");
        
        QLabel *labelLbl = new QLabel(QString("<b>%1:</b>").arg(label));
        labelLbl->setFixedWidth(100);
        labelLbl->setStyleSheet(QString("color: %1; font-weight: 600; font-size: 10pt;").arg(color));
        
        QLabel *valueLbl = new QLabel(value);
        valueLbl->setStyleSheet(QString("color: %1; font-weight: 500; font-size: 10pt;").arg(color));
        valueLbl->setWordWrap(true);
        valueLbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        
        itemLayout->addWidget(iconLbl);
        itemLayout->addWidget(labelLbl);
        itemLayout->addWidget(valueLbl, 1);
        
        infoLayout->addWidget(itemFrame);
    };
    
    // 添加各项信息
    addInfoItem("🆔", "进程ID", QString::number(process.processId), "#3498db");
    addInfoItem("📁", "可执行文件", process.executablePath.isEmpty() ? "未知" : process.executablePath, "#8e44ad");
    addInfoItem("👤", "运行用户", process.userName.isEmpty() ? "未知" : process.userName, "#f39c12");
    
    QString cpuColor = process.cpuUsage > 50.0 ? "#e74c3c" : (process.cpuUsage > 20.0 ? "#f39c12" : "#27ae60");
    addInfoItem("⚡", "CPU使用率", QString("%1%").arg(process.cpuUsage, 0, 'f', 2), cpuColor);
    
    addInfoItem("💾", "内存使用", formatBytes(process.workingSetSize), "#9b59b6");
    addInfoItem("📖", "磁盘读取", formatDiskUsage(process.diskReadBytes), "#16a085");
    addInfoItem("📝", "磁盘写入", formatDiskUsage(process.diskWriteBytes), "#e67e22");
    
    QString criticalColor = process.isCriticalProcess ? "#e74c3c" : "#27ae60";
    QString criticalText = process.isCriticalProcess ? "系统关键进程" : "普通进程";
    addInfoItem("⭐", "重要程度", criticalText, criticalColor);
    
    addInfoItem("🕒", "最后更新", process.lastUpdateTime.toString("yyyy-MM-dd hh:mm:ss"), "#34495e");
    
    scrollArea->setWidget(infoWidget);
    
    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *locationBtn = new QPushButton("📂 打开文件位置");
    locationBtn->setFixedSize(120, 35);
    locationBtn->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 #27ae60,"
        "                                stop: 1 #229954);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    font-weight: 600;"
        "    font-size: 9pt;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 #229954,"
        "                                stop: 1 #1e8449);"
        "}"
        "QPushButton:pressed {"
        "    background: #1e8449;"
        "}"
        "QPushButton:disabled {"
        "    background: #bdc3c7;"
        "    color: #7f8c8d;"
        "}"
    );
    
    if (process.executablePath.isEmpty()) {
        locationBtn->setEnabled(false);
        locationBtn->setToolTip("无法获取进程文件路径");
    } else {
        connect(locationBtn, &QPushButton::clicked, [=]() {
            openProcessLocation(process.executablePath);
        });
    }
    
    QPushButton *closeBtn = new QPushButton("关闭");
    closeBtn->setFixedSize(80, 35);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 #3498db,"
        "                                stop: 1 #2980b9);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    font-weight: 600;"
        "    font-size: 10pt;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 #2980b9,"
        "                                stop: 1 #21618c);"
        "}"
        "QPushButton:pressed {"
        "    background: #21618c;"
        "}"
    );
    
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    
    buttonLayout->addWidget(locationBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    
    // 组装布局
    layout->addLayout(titleLayout);
    layout->addWidget(scrollArea, 1);
    layout->addLayout(buttonLayout);
    
    dialog->exec();
    dialog->deleteLater();
}

// 打开选中进程的文件位置
void MainWindow::openSelectedProcessLocation()
{
    QTreeWidgetItem *item = processTreeWidget->currentItem();
    if (!item) {
        return;
    }
    
    ProcessInfo process = item->data(0, Qt::UserRole).value<ProcessInfo>();
    openProcessLocation(process.executablePath);
}

// 格式化磁盘使用量（包含速率信息）
QString MainWindow::formatDiskUsage(qint64 bytes)
{
    if (bytes == 0) {
        return "0 B";
    }
    
    // 如果是速率信息，添加 /s 后缀
    return formatBytes(bytes);
}

// 获取进程图标
QIcon MainWindow::getProcessIcon(const QString &executablePath)
{
    Q_UNUSED(executablePath)
    // 简单实现，返回默认图标
    // 在实际应用中，可以使用Windows API获取文件图标
    return QIcon();
}

// 初始化历史数据管理器
void MainWindow::initializeHistoryManager()
{
    qInfo() << "[历史管理] 初始化历史数据管理器";
    
    historyManager = new HistoryManager(this);
    
    // 初始化数据库
    if (!historyManager->initializeDatabase()) {
        qWarning() << "[历史管理] 数据库初始化失败:" << historyManager->getLastError();
        return;
    }
    
    // 连接信号槽
    connect(historyManager, &HistoryManager::dataRecorded, 
            this, [](const HistoryRecord &record) {
        qDebug() << "[历史管理] 记录数据:" << record.driveLetter 
                 << "使用率:" << record.usagePercentage << "%";
    });
    
    connect(historyManager, &HistoryManager::errorOccurred,
            this, [](const QString &error) {
        qWarning() << "[历史管理] 错误:" << error;
    });
    
    connect(historyManager, &HistoryManager::recordingStarted,
            this, []() {
        qInfo() << "[历史管理] 开始记录历史数据";
    });
    
    connect(historyManager, &HistoryManager::recordingStopped,
            this, []() {
        qInfo() << "[历史管理] 停止记录历史数据";
    });
    
    connect(historyManager, &HistoryManager::cleanupCompleted,
            this, [](int removedRecords) {
        qInfo() << "[历史管理] 清理完成，删除了" << removedRecords << "条记录";
    });
    
    // 连接到磁盘数据更新信号，自动记录历史数据
    connect(this, &MainWindow::driveDataUpdated,
            this, &MainWindow::recordDriveHistory);
    
    // 设置默认配置
    historyManager->setRecordingInterval(300); // 5分钟记录一次
    historyManager->setRetentionPolicy(RetentionPolicy::OneMonth); // 保留一个月
    historyManager->setAutoCleanup(true);
    
    // 启动记录
    historyManager->startRecording();
    
    qInfo() << "[历史管理] 历史数据管理器初始化完成";
}

// 记录驱动器历史数据
void MainWindow::recordDriveHistory()
{
    if (!historyManager || !historyManager->isDatabaseReady()) {
        return;
    }
    
    QList<QStorageInfo> drives = QStorageInfo::mountedVolumes();
    QList<HistoryRecord> records;
    
    for (const QStorageInfo &drive : drives) {
        if (!drive.isValid() || drive.isReadOnly()) {
            continue;
        }
        
        QString driveLetter = drive.rootPath().left(1).toUpper();
        if (driveLetter.isEmpty()) {
            continue;
        }
        
        HistoryRecord record;
        record.timestamp = QDateTime::currentDateTime();
        record.driveLetter = driveLetter;
        record.displayName = drive.displayName().isEmpty() ? 
                            QString("本地磁盘 (%1:)").arg(driveLetter) : 
                            drive.displayName();
        record.totalSpace = drive.bytesTotal();
        record.usedSpace = drive.bytesTotal() - drive.bytesAvailable();
        record.freeSpace = drive.bytesAvailable();
        record.usagePercentage = record.totalSpace > 0 ? 
                                (double(record.usedSpace) / record.totalSpace) * 100.0 : 0.0;
        
        // 获取进程统计信息（如果可用）
        if (processMonitor) {
            DriveProcessStats stats = processMonitor->getDriveStats(driveLetter);
            record.processCount = stats.activeProcessCount;
            record.criticalProcessCount = stats.criticalProcessCount;
            record.totalDiskReadBytes = stats.totalReadBytes;
            record.totalDiskWriteBytes = stats.totalWriteBytes;
        } else {
            record.processCount = 0;
            record.criticalProcessCount = 0;
            record.totalDiskReadBytes = 0;
            record.totalDiskWriteBytes = 0;
        }
        
        records.append(record);
    }
    
    // 批量记录数据
    if (!records.isEmpty()) {
        historyManager->recordDriveDataBatch(records);
    }
}

// 导出历史数据
void MainWindow::exportHistoryData(const QString &driveLetter, 
                                  const QDateTime &startTime, 
                                  const QDateTime &endTime,
                                  ExportFormat format)
{
    if (!historyManager || !historyManager->isDatabaseReady()) {
        QMessageBox::warning(this, "导出失败", "历史数据管理器未就绪");
        return;
    }
    
    QString defaultFileName = QString("磁盘历史数据_%1_%2_%3")
                             .arg(driveLetter)
                             .arg(startTime.toString("yyyyMMdd"))
                             .arg(endTime.toString("yyyyMMdd"));
    
    QString filter;
    QString extension;
    
    switch (format) {
    case ExportFormat::CSV:
        filter = "CSV文件 (*.csv)";
        extension = ".csv";
        break;
    case ExportFormat::Excel:
        filter = "Excel文件 (*.xlsx)";
        extension = ".xlsx";
        break;
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this, "导出历史数据", 
        defaultFileName + extension, 
        filter
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    bool success = false;
    switch (format) {
    case ExportFormat::CSV:
        success = historyManager->exportToCSV(fileName, driveLetter, startTime, endTime);
        break;
    case ExportFormat::Excel:
        success = historyManager->exportToExcel(fileName, driveLetter, startTime, endTime);
        break;
    }
    
    if (success) {
        QMessageBox::information(this, "导出成功", 
                               QString("历史数据已成功导出到:\n%1").arg(fileName));
    } else {
        QMessageBox::warning(this, "导出失败", 
                           QString("导出失败: %1").arg(historyManager->getLastError()));
    }
}

void MainWindow::showHistoryDialog()
{
    if (!historyManager || !historyManager->isDatabaseReady()) {
        QMessageBox::warning(this, "警告", "历史数据管理器未就绪或数据库连接失败！");
        return;
    }
    
    if (!historyDialog) {
        historyDialog = new HistoryDialog(historyManager, this);
    }
    
    historyDialog->show();
    historyDialog->raise();
    historyDialog->activateWindow();
}

void MainWindow::exportHistoryCSV()
{
    if (!historyManager || !historyManager->isDatabaseReady()) {
        QMessageBox::warning(this, "警告", "历史数据管理器未就绪或数据库连接失败！");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, 
        "导出CSV文件", 
        QString("磁盘历史数据_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "CSV文件 (*.csv)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // 导出所有驱动器的数据，时间范围为最近7天
    QDateTime endTime = QDateTime::currentDateTime();
    QDateTime startTime = endTime.addDays(-7);
    
    bool success = historyManager->exportToCSV(fileName, "", startTime, endTime);
    
    if (success) {
        QMessageBox::information(this, "成功", "CSV文件导出成功！");
    } else {
        QMessageBox::critical(this, "错误", 
            QString("CSV文件导出失败: %1").arg(historyManager->getLastError()));
    }
}

void MainWindow::exportHistoryExcel()
{
    if (!historyManager || !historyManager->isDatabaseReady()) {
        QMessageBox::warning(this, "警告", "历史数据管理器未就绪或数据库连接失败！");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, 
        "导出Excel文件", 
        QString("磁盘历史数据_%1.xlsx").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "Excel文件 (*.xlsx);;CSV文件 (*.csv)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // 导出所有驱动器的数据，时间范围为最近7天
    QDateTime endTime = QDateTime::currentDateTime();
    QDateTime startTime = endTime.addDays(-7);
    
    bool success = historyManager->exportToExcel(fileName, "", startTime, endTime);
    
    if (success) {
        QMessageBox::information(this, "成功", "Excel文件导出成功！");
    } else {
        QMessageBox::critical(this, "错误", 
            QString("Excel文件导出失败: %1").arg(historyManager->getLastError()));
    }
}

// 系统托盘设置
void MainWindow::setupSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "[主窗口] 系统托盘不可用";
        return;
    }
    
    // 创建托盘图标
    trayIcon = new QSystemTrayIcon(this);
    // 使用系统标准图标，避免资源文件缺失问题
    QIcon trayIconImage = style()->standardIcon(QStyle::SP_DriveHDIcon);
    if (trayIconImage.isNull()) {
        // 如果标准图标不可用，使用应用程序图标
        trayIconImage = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    }
    trayIcon->setIcon(trayIconImage);
    trayIcon->setToolTip("磁盘监控器");
    
    // 创建托盘菜单
    createTrayMenu();
    
    // 连接托盘图标信号
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
    
    // 显示托盘图标
    trayIcon->show();
    
    qInfo() << "[主窗口] 系统托盘设置完成";
}

// 创建托盘菜单
void MainWindow::createTrayMenu()
{
    trayMenu = new QMenu(this);
    
    // 主窗口控制
    showAction = new QAction("显示主窗口", this);
    connect(showAction, &QAction::triggered, this, &MainWindow::showWindow);
    
    hideAction = new QAction("隐藏窗口", this);
    connect(hideAction, &QAction::triggered, this, &MainWindow::hideWindow);
    
    // 功能菜单
    QAction *settingsAction = new QAction("设置", this);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
    
    QAction *historyAction = new QAction("历史记录", this);
    connect(historyAction, &QAction::triggered, this, &MainWindow::showHistoryDialog);
    
    QAction *chartAction = new QAction("图表查看", this);
    connect(chartAction, &QAction::triggered, this, &MainWindow::showChartDialog);
    
    // 快速操作
    QAction *refreshAction = new QAction("立即刷新", this);
    connect(refreshAction, &QAction::triggered, this, [this]() {
        if (processMonitor) {
            processMonitor->refreshProcessData();
        }
    });
    
    quitAction = new QAction("退出程序", this);
    connect(quitAction, &QAction::triggered, this, &MainWindow::quitApplication);
    
    // 构建菜单
    trayMenu->addAction(showAction);
    trayMenu->addAction(hideAction);
    trayMenu->addSeparator();
    trayMenu->addAction(refreshAction);
    trayMenu->addAction(settingsAction);
    trayMenu->addAction(historyAction);
    trayMenu->addAction(chartAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);
    
    trayIcon->setContextMenu(trayMenu);
}

// 托盘图标激活处理
void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        if (isVisible()) {
            hideWindow();
        } else {
            showWindow();
        }
        break;
    default:
        break;
    }
}

// 显示窗口
void MainWindow::showWindow()
{
    show();
    raise();
    activateWindow();
    if (isMinimized()) {
        showNormal();
    }
}

// 隐藏窗口
void MainWindow::hideWindow()
{
    hide();
}

// 退出应用程序
void MainWindow::quitApplication()
{
    saveSettings();
    QApplication::quit();
}

// 加载设置
void MainWindow::loadSettings()
{
    // 系统托盘和启动设置
    autoStart = settings->value("System/autoStart", false).toBool();
    minimizeToTray = settings->value("System/minimizeToTray", true).toBool();
    startMinimized = settings->value("System/startMinimized", false).toBool();
    closeToTray = settings->value("System/closeToTray", true).toBool();
    
    // 窗口设置
    QSize defaultSize(1200, 800);
    QPoint defaultPos(100, 100);
    resize(settings->value("Window/size", defaultSize).toSize());
    move(settings->value("Window/position", defaultPos).toPoint());
    bool isMaximized = settings->value("Window/maximized", false).toBool();
    if (isMaximized) {
        showMaximized();
    }
    
    // 刷新间隔设置
    int refreshInterval = settings->value("Monitor/refreshInterval", 5000).toInt();
    if (refreshTimer) {
        refreshTimer->setInterval(refreshInterval);
    }
    
    // 设置刷新间隔组合框的选中项（延迟应用，确保UI已初始化）
    QTimer::singleShot(200, [this, refreshInterval]() {
        if (refreshIntervalCombo) {
            for (int i = 0; i < refreshIntervalCombo->count(); ++i) {
                if (refreshIntervalCombo->itemData(i).toInt() == refreshInterval) {
                    refreshIntervalCombo->setCurrentIndex(i);
                    break;
                }
            }
        }
    });
    
    // 颜色设置
    lowUsageColor = QColor(settings->value("Colors/lowUsage", "#4CAF50").toString());
    mediumUsageColor = QColor(settings->value("Colors/mediumUsage", "#FFC107").toString());
    highUsageColor = QColor(settings->value("Colors/highUsage", "#FF9800").toString());
    criticalUsageColor = QColor(settings->value("Colors/criticalUsage", "#F44336").toString());
    
    // 监控设置
    QStringList monitoredDrives = settings->value("Monitor/driveFilter", QStringList()).toStringList();
    
    // 数据保留策略
    int retentionPolicy = settings->value("Data/retentionPolicy", static_cast<int>(RetentionPolicy::OneMonth)).toInt();
    if (historyManager) {
        historyManager->setRetentionPolicy(static_cast<RetentionPolicy>(retentionPolicy));
    }
    
    // 界面显示设置（预留用于未来功能扩展）
    // bool showAllDrives = settings->value("Display/showAllDrives", true).toBool();
    // bool hideEmptyDrives = settings->value("Display/hideEmptyDrives", false).toBool();
    
    // 进程列表列宽设置
    QList<int> columnWidths = {
        settings->value("ProcessList/column0Width", 180).toInt(),
        settings->value("ProcessList/column1Width", 85).toInt(),
        settings->value("ProcessList/column2Width", 120).toInt(),
        settings->value("ProcessList/column3Width", 120).toInt(),
        settings->value("ProcessList/column4Width", 140).toInt(),
        settings->value("ProcessList/column5Width", 100).toInt(),
        settings->value("ProcessList/column6Width", 120).toInt(),
        settings->value("ProcessList/column7Width", 100).toInt(),
        settings->value("ProcessList/column8Width", 110).toInt()
    };
    
    // 应用进程列表列宽（延迟应用，确保UI已初始化）
    QTimer::singleShot(100, [this, columnWidths]() {
        if (ui->processTreeWidget && ui->processTreeWidget->header()) {
            for (int i = 0; i < columnWidths.size() && i < ui->processTreeWidget->columnCount(); ++i) {
                ui->processTreeWidget->header()->resizeSection(i, columnWidths[i]);
            }
        }
    });
    
    // 分割器状态（延迟应用，确保UI已初始化）
    QTimer::singleShot(150, [this]() {
        QList<int> defaultSizes;
        defaultSizes << 400 << 600;
        QList<int> splitterSizes = settings->value("UI/splitterSizes", QVariant::fromValue(defaultSizes)).value<QList<int>>();
        if (mainSplitter && !splitterSizes.isEmpty()) {
            mainSplitter->setSizes(splitterSizes);
        }
    });
    
    // 如果设置了启动时最小化，则隐藏到托盘
    if (startMinimized) {
        QTimer::singleShot(100, this, &MainWindow::hideWindow);
    }
    
    qInfo() << "[主窗口] 设置加载完成";
}

// 保存设置
void MainWindow::saveSettings()
{
    // 系统托盘和启动设置
    settings->setValue("System/autoStart", autoStart);
    settings->setValue("System/minimizeToTray", minimizeToTray);
    settings->setValue("System/startMinimized", startMinimized);
    settings->setValue("System/closeToTray", closeToTray);
    
    // 窗口设置
    settings->setValue("Window/size", size());
    settings->setValue("Window/position", pos());
    settings->setValue("Window/maximized", isMaximized());
    
    // 刷新间隔设置
    if (refreshTimer) {
        settings->setValue("Monitor/refreshInterval", refreshTimer->interval());
    }
    
    // 颜色设置
    settings->setValue("Colors/lowUsage", lowUsageColor.name());
    settings->setValue("Colors/mediumUsage", mediumUsageColor.name());
    settings->setValue("Colors/highUsage", highUsageColor.name());
    settings->setValue("Colors/criticalUsage", criticalUsageColor.name());
    
    // 监控设置
    QStringList monitoredDrives;
    for (const auto &driveInfo : driveInfoList) {
        if (driveInfo.isMonitored) {
            monitoredDrives.append(driveInfo.driveLetter);
        }
    }
    settings->setValue("Monitor/driveFilter", monitoredDrives);
    
    // 数据保留策略
    if (historyManager) {
        settings->setValue("Data/retentionPolicy", static_cast<int>(historyManager->getRetentionPolicy()));
    }
    
    // 界面显示设置
    settings->setValue("Display/showAllDrives", ui->actionShowAllDrives->isChecked());
    settings->setValue("Display/hideEmptyDrives", ui->actionHideEmptyDrives->isChecked());
    
    // 进程列表列宽设置
    if (ui->processTreeWidget && ui->processTreeWidget->header()) {
        for (int i = 0; i < ui->processTreeWidget->columnCount(); ++i) {
            settings->setValue(QString("ProcessList/column%1Width").arg(i), 
                             ui->processTreeWidget->header()->sectionSize(i));
        }
    }
    
    settings->sync();
    
    qInfo() << "[主窗口] 设置保存完成";
}

// 设置开机自启动
void MainWindow::setupAutoStart(bool enabled)
{
    QString appName = "DiskMonitor";
    QString appPath = QApplication::applicationFilePath();
    
#ifdef Q_OS_WIN
    QSettings autoStartSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    
    if (enabled) {
        autoStartSettings.setValue(appName, QDir::toNativeSeparators(appPath));
        qInfo() << "[主窗口] 开机自启动已启用";
    } else {
        autoStartSettings.remove(appName);
        qInfo() << "[主窗口] 开机自启动已禁用";
    }
#endif
}

// 检查是否已设置开机自启动
bool MainWindow::isAutoStartEnabled()
{
#ifdef Q_OS_WIN
    QString appName = "DiskMonitor";
    QSettings autoStartSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    return autoStartSettings.contains(appName);
#else
    return false;
#endif
}

// 切换开机自启动
void MainWindow::toggleAutoStart(bool enabled)
{
    autoStart = enabled;
    setupAutoStart(enabled);
    saveSettings();
}

// 切换最小化到托盘
void MainWindow::toggleMinimizeToTray(bool enabled)
{
    minimizeToTray = enabled;
    saveSettings();
}

// 切换启动时最小化
void MainWindow::toggleStartMinimized(bool enabled)
{
    startMinimized = enabled;
    saveSettings();
}

// 切换关闭时最小化到托盘
void MainWindow::toggleCloseToTray(bool enabled)
{
    closeToTray = enabled;
    saveSettings();
}

// 窗口关闭事件处理
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (closeToTray && trayIcon && trayIcon->isVisible()) {
        // 最小化到托盘而不是退出
        hide();
        event->ignore();
        
        // 显示提示信息（仅第一次）
        static bool firstTime = true;
        if (firstTime) {
            trayIcon->showMessage("磁盘监控器", "程序已最小化到系统托盘", QSystemTrayIcon::Information, 2000);
            firstTime = false;
        }
    } else {
        // 正常退出
        saveSettings();
        event->accept();
    }
}

// 窗口状态改变事件处理
void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && minimizeToTray && trayIcon && trayIcon->isVisible()) {
            // 最小化时隐藏到托盘
            hide();
            event->ignore();
        }
    }
}

// 窗口大小改变事件处理
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (settings && !isMaximized() && !isMinimized()) {
        settings->setValue("Window/size", size());
        settings->sync();
    }
}

// 窗口位置改变事件处理
void MainWindow::moveEvent(QMoveEvent *event)
{
    QMainWindow::moveEvent(event);
    if (settings && !isMaximized() && !isMinimized()) {
        settings->setValue("Window/position", pos());
        settings->sync();
    }
}

// 图表相关槽函数实现
void MainWindow::showChartDialog()
{
    if (!historyManager || !historyManager->isDatabaseReady()) {
        QMessageBox::warning(this, "警告", "历史数据管理器未就绪或数据库连接失败！");
        return;
    }
    
    if (!chartDialog) {
        chartDialog = new ChartDialog(historyManager, this);
        
        // 连接图表对话框的信号
        connect(chartDialog, &ChartDialog::chartDialogClosed, [this]() {
            qDebug() << "[主窗口] 图表对话框已关闭";
        });
        
        connect(chartDialog, &ChartDialog::chartTypeSelected, 
                [this](ChartType type, const QString &driveLetter) {
            qDebug() << "[主窗口] 选择图表类型:" << static_cast<int>(type) << "驱动器:" << driveLetter;
        });
    }
    
    // 显示图表对话框
    chartDialog->show();
    chartDialog->raise();
    chartDialog->activateWindow();
    
    qInfo() << "[主窗口] 显示图表对话框";
}

void MainWindow::showRealTimeChart()
{
    if (!historyManager || !historyManager->isDatabaseReady()) {
        QMessageBox::warning(this, "警告", "历史数据管理器未就绪或数据库连接失败！");
        return;
    }
    
    if (!chartDialog) {
        chartDialog = new ChartDialog(historyManager, this);
    }
    
    // 显示实时使用率图表
    QString selectedDrive = currentSelectedDrive.isEmpty() ? "C:" : currentSelectedDrive;
    chartDialog->showChart(ChartType::RealTimeUsage, selectedDrive);
    
    qInfo() << "[主窗口] 显示实时图表 - 驱动器:" << selectedDrive;
}

void MainWindow::showHistoryTrendChart()
{
    if (!historyManager || !historyManager->isDatabaseReady()) {
        QMessageBox::warning(this, "警告", "历史数据管理器未就绪或数据库连接失败！");
        return;
    }
    
    if (!chartDialog) {
        chartDialog = new ChartDialog(historyManager, this);
    }
    
    // 显示历史趋势图表
    QString selectedDrive = currentSelectedDrive.isEmpty() ? "C:" : currentSelectedDrive;
    chartDialog->showChart(ChartType::HistoryTrend, selectedDrive);
    
    qInfo() << "[主窗口] 显示历史趋势图表 - 驱动器:" << selectedDrive;
}

void MainWindow::showMultiDriveChart()
{
    if (!historyManager || !historyManager->isDatabaseReady()) {
        QMessageBox::warning(this, "警告", "历史数据管理器未就绪或数据库连接失败！");
        return;
    }
    
    if (!chartDialog) {
        chartDialog = new ChartDialog(historyManager, this);
    }
    
    // 显示多驱动器对比图表
    chartDialog->showChart(ChartType::MultiDriveCompare, "");
    
    qInfo() << "[主窗口] 显示多驱动器对比图表";
}


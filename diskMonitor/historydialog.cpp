#include "historydialog.h"

HistoryDialog::HistoryDialog(HistoryManager *historyManager, QWidget *parent)
    : QDialog(parent)
    , historyManager(historyManager)
    , autoRefreshEnabled(false)
    , isRefreshing(false)
{
    setWindowTitle("历史数据查看器");
    setWindowIcon(QIcon(":/icons/history.png"));
    resize(1000, 600);
    setModal(false);
    
    // 初始化定时器
    autoRefreshTimer = new QTimer(this);
    autoRefreshTimer->setInterval(AUTO_REFRESH_INTERVAL);
    connect(autoRefreshTimer, &QTimer::timeout, this, &HistoryDialog::onAutoRefreshTimer);
    
    // 初始化防抖定时器
    debounceTimer = new QTimer(this);
    debounceTimer->setSingleShot(true);
    debounceTimer->setInterval(DEBOUNCE_DELAY);
    connect(debounceTimer, &QTimer::timeout, this, &HistoryDialog::performRefresh);
    
    setupUI();
    populateDriveComboBox();
    loadHistoryData();
}

HistoryDialog::~HistoryDialog()
{
    if (autoRefreshTimer->isActive()) {
        autoRefreshTimer->stop();
    }
}

void HistoryDialog::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    setupControls();
    setupTable();
    
    // 状态标签
    statusLabel = new QLabel("准备就绪", this);
    statusLabel->setStyleSheet("QLabel { color: #666666; font-style: italic; padding: 5px; }");
    
    mainLayout->addWidget(filterGroupBox);
    mainLayout->addWidget(dataGroupBox);
    mainLayout->addWidget(statusLabel);
}

void HistoryDialog::setupControls()
{
    filterGroupBox = new QGroupBox("筛选条件", this);
    controlLayout = new QHBoxLayout(filterGroupBox);
    
    // 驱动器选择
    QLabel *driveLabel = new QLabel("驱动器:", this);
    driveComboBox = new QComboBox(this);
    driveComboBox->setMinimumWidth(100);
    connect(driveComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HistoryDialog::onDriveSelectionChanged);
    
    // 时间范围
    QLabel *startLabel = new QLabel("开始时间:", this);
    startTimeEdit = new QDateTimeEdit(this);
    startTimeEdit->setDateTime(QDateTime::currentDateTime().addDays(-7));
    startTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
    startTimeEdit->setCalendarPopup(true);
    connect(startTimeEdit, &QDateTimeEdit::dateTimeChanged,
            this, &HistoryDialog::onTimeRangeChanged);
    
    QLabel *endLabel = new QLabel("结束时间:", this);
    endTimeEdit = new QDateTimeEdit(this);
    endTimeEdit->setDateTime(QDateTime::currentDateTime());
    endTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
    endTimeEdit->setCalendarPopup(true);
    connect(endTimeEdit, &QDateTimeEdit::dateTimeChanged,
            this, &HistoryDialog::onTimeRangeChanged);
    
    // 按钮
    refreshButton = new QPushButton("刷新数据", this);
    refreshButton->setIcon(QIcon(":/icons/refresh.png"));
    connect(refreshButton, &QPushButton::clicked, this, &HistoryDialog::onRefreshData);
    
    autoRefreshButton = new QPushButton("自动刷新", this);
    autoRefreshButton->setCheckable(true);
    autoRefreshButton->setIcon(QIcon(":/icons/auto.png"));
    connect(autoRefreshButton, &QPushButton::toggled, this, &HistoryDialog::onAutoRefreshToggled);
    
    exportCSVButton = new QPushButton("导出CSV", this);
    exportCSVButton->setIcon(QIcon(":/icons/export.png"));
    connect(exportCSVButton, &QPushButton::clicked, this, &HistoryDialog::onExportCSV);
    
    exportExcelButton = new QPushButton("导出Excel", this);
    exportExcelButton->setIcon(QIcon(":/icons/excel.png"));
    connect(exportExcelButton, &QPushButton::clicked, this, &HistoryDialog::onExportExcel);
    
    // 布局
    controlLayout->addWidget(driveLabel);
    controlLayout->addWidget(driveComboBox);
    controlLayout->addWidget(startLabel);
    controlLayout->addWidget(startTimeEdit);
    controlLayout->addWidget(endLabel);
    controlLayout->addWidget(endTimeEdit);
    controlLayout->addStretch();
    controlLayout->addWidget(refreshButton);
    controlLayout->addWidget(autoRefreshButton);
    controlLayout->addWidget(exportCSVButton);
    controlLayout->addWidget(exportExcelButton);
}

void HistoryDialog::setupTable()
{
    dataGroupBox = new QGroupBox("历史数据", this);
    QVBoxLayout *tableLayout = new QVBoxLayout(dataGroupBox);
    
    historyTable = new QTableWidget(this);
    historyTable->setAlternatingRowColors(true);
    historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    historyTable->setSortingEnabled(true);
    historyTable->setShowGrid(true);
    historyTable->setGridStyle(Qt::SolidLine);
    
    // 设置列
    QStringList headers;
    headers << "时间" << "驱动器" << "显示名称" << "总空间" << "已用空间" 
            << "可用空间" << "使用率(%)" << "活跃进程" << "关键进程" 
            << "读取字节" << "写入字节";
    
    historyTable->setColumnCount(headers.size());
    historyTable->setHorizontalHeaderLabels(headers);
    
    // 设置列宽
    QHeaderView *header = historyTable->horizontalHeader();
    header->setStretchLastSection(true);
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents); // 时间
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents); // 驱动器
    header->setSectionResizeMode(2, QHeaderView::Stretch);          // 显示名称
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents); // 总空间
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents); // 已用空间
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents); // 可用空间
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents); // 使用率
    header->setSectionResizeMode(7, QHeaderView::ResizeToContents); // 活跃进程
    header->setSectionResizeMode(8, QHeaderView::ResizeToContents); // 关键进程
    header->setSectionResizeMode(9, QHeaderView::ResizeToContents); // 读取字节
    header->setSectionResizeMode(10, QHeaderView::ResizeToContents);// 写入字节
    
    tableLayout->addWidget(historyTable);
}

void HistoryDialog::populateDriveComboBox()
{
    driveComboBox->clear();
    driveComboBox->addItem("所有驱动器", QString());
    
    // 获取所有可用的驱动器
    QFileInfoList drives = QDir::drives();
    for (const QFileInfo &drive : drives) {
        QString driveDisplay = drive.absolutePath().left(2); // 例如 "C:" 用于显示
        QString driveLetter = drive.absolutePath().left(1).toUpper(); // 例如 "C" 用于查询数据库
        driveComboBox->addItem(driveDisplay, driveLetter);
    }
}

void HistoryDialog::loadHistoryData()
{
    if (!historyManager || !historyManager->isDatabaseReady()) {
        QMessageBox::warning(this, "警告", "历史数据管理器未就绪或数据库连接失败！");
        return;
    }
    
    // 设置刷新状态
    isRefreshing = true;
    refreshButton->setEnabled(false);
    refreshButton->setText("刷新中...");
    
    QString selectedDrive = driveComboBox->currentData().toString();
    QDateTime startTime = startTimeEdit->dateTime();
    QDateTime endTime = endTimeEdit->dateTime();
    
    // 验证时间范围
    if (startTime >= endTime) {
        QMessageBox::warning(this, "警告", "开始时间必须早于结束时间！");
        return;
    }
    
    // 显示进度对话框
    QProgressDialog progress("正在加载历史数据...", "取消", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    QApplication::processEvents();
    
    try {
        if (selectedDrive.isEmpty()) {
            currentData = historyManager->getAllDrivesData(startTime, endTime);
        } else {
            currentData = historyManager->getHistoryData(selectedDrive, startTime, endTime);
        }
        
        // 限制显示记录数
        if (currentData.size() > MAX_DISPLAY_RECORDS) {
            QMessageBox::information(this, "提示", 
                QString("查询到 %1 条记录，仅显示最新的 %2 条记录。")
                .arg(currentData.size()).arg(MAX_DISPLAY_RECORDS));
            
            // 保留最新的记录
            std::sort(currentData.begin(), currentData.end(), 
                     [](const HistoryRecord &a, const HistoryRecord &b) {
                         return a.timestamp > b.timestamp;
                     });
            currentData = currentData.mid(0, MAX_DISPLAY_RECORDS);
        }
        
        formatTableData();
        updateStatusLabel();
        
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "错误", QString("加载历史数据时发生错误: %1").arg(e.what()));
    }
    
    progress.close();
    
    // 恢复刷新状态
    isRefreshing = false;
    refreshButton->setEnabled(true);
    refreshButton->setText("刷新数据");
}

void HistoryDialog::formatTableData()
{
    historyTable->setRowCount(currentData.size());
    historyTable->setSortingEnabled(false);
    
    for (int row = 0; row < currentData.size(); ++row) {
        const HistoryRecord &record = currentData[row];
        
        // 时间
        QTableWidgetItem *timeItem = new QTableWidgetItem(formatDateTime(record.timestamp));
        timeItem->setData(Qt::UserRole, record.timestamp);
        historyTable->setItem(row, 0, timeItem);
        
        // 驱动器
        historyTable->setItem(row, 1, new QTableWidgetItem(record.driveLetter));
        
        // 显示名称
        historyTable->setItem(row, 2, new QTableWidgetItem(record.displayName));
        
        // 总空间
        historyTable->setItem(row, 3, new QTableWidgetItem(formatBytes(record.totalSpace)));
        
        // 已用空间
        historyTable->setItem(row, 4, new QTableWidgetItem(formatBytes(record.usedSpace)));
        
        // 可用空间
        historyTable->setItem(row, 5, new QTableWidgetItem(formatBytes(record.freeSpace)));
        
        // 使用率
        QTableWidgetItem *usageItem = new QTableWidgetItem(QString::number(record.usagePercentage, 'f', 2));
        usageItem->setData(Qt::UserRole, record.usagePercentage);
        // 根据使用率设置颜色
        if (record.usagePercentage >= 90.0) {
            usageItem->setBackground(QColor(255, 200, 200)); // 红色
        } else if (record.usagePercentage >= 80.0) {
            usageItem->setBackground(QColor(255, 255, 200)); // 黄色
        } else {
            usageItem->setBackground(QColor(200, 255, 200)); // 绿色
        }
        historyTable->setItem(row, 6, usageItem);
        
        // 活跃进程数
        historyTable->setItem(row, 7, new QTableWidgetItem(QString::number(record.processCount)));
        
        // 关键进程数
        historyTable->setItem(row, 8, new QTableWidgetItem(QString::number(record.criticalProcessCount)));
        
        // 读取字节数
        historyTable->setItem(row, 9, new QTableWidgetItem(formatBytes(record.totalDiskReadBytes)));
        
        // 写入字节数
        historyTable->setItem(row, 10, new QTableWidgetItem(formatBytes(record.totalDiskWriteBytes)));
    }
    
    historyTable->setSortingEnabled(true);
    historyTable->sortByColumn(0, Qt::DescendingOrder); // 按时间降序排列
}

void HistoryDialog::updateStatusLabel()
{
    QString status = QString("共 %1 条记录").arg(currentData.size());
    
    if (!currentData.isEmpty()) {
        QDateTime earliest = currentData.first().timestamp;
        QDateTime latest = currentData.first().timestamp;
        
        for (const HistoryRecord &record : currentData) {
            if (record.timestamp < earliest) earliest = record.timestamp;
            if (record.timestamp > latest) latest = record.timestamp;
        }
        
        status += QString(" | 时间范围: %1 至 %2")
                 .arg(formatDateTime(earliest))
                 .arg(formatDateTime(latest));
    }
    
    statusLabel->setText(status);
}

QString HistoryDialog::formatBytes(qint64 bytes) const
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    const qint64 TB = GB * 1024;
    
    if (bytes >= TB) {
        return QString::number(bytes / (double)TB, 'f', 2) + " TB";
    } else if (bytes >= GB) {
        return QString::number(bytes / (double)GB, 'f', 2) + " GB";
    } else if (bytes >= MB) {
        return QString::number(bytes / (double)MB, 'f', 2) + " MB";
    } else if (bytes >= KB) {
        return QString::number(bytes / (double)KB, 'f', 2) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
}

QString HistoryDialog::formatDateTime(const QDateTime &dateTime) const
{
    return dateTime.toString("yyyy-MM-dd hh:mm:ss");
}

void HistoryDialog::onRefreshData()
{
    // 如果正在刷新，忽略请求
    if (isRefreshing) {
        statusLabel->setText("正在刷新中，请稍候...");
        statusLabel->setStyleSheet("QLabel { color: #ff9800; font-style: italic; padding: 5px; }");
        return;
    }
    
    // 停止之前的防抖定时器
    if (debounceTimer->isActive()) {
        debounceTimer->stop();
    }
    
    // 启动防抖定时器
    debounceTimer->start();
    statusLabel->setText("准备刷新数据...");
    statusLabel->setStyleSheet("QLabel { color: #2196f3; font-style: italic; padding: 5px; }");
}

void HistoryDialog::performRefresh()
{
    loadHistoryData();
}

void HistoryDialog::onExportCSV()
{
    if (currentData.isEmpty()) {
        QMessageBox::information(this, "提示", "没有数据可以导出！");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, 
        "导出CSV文件", 
        QString("磁盘历史数据_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "CSV文件 (*.csv)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QString selectedDrive = driveComboBox->currentData().toString();
    QDateTime startTime = startTimeEdit->dateTime();
    QDateTime endTime = endTimeEdit->dateTime();
    
    bool success;
    if (selectedDrive.isEmpty()) {
        // 导出所有驱动器的数据需要特殊处理
        success = historyManager->exportToCSV(fileName, "", startTime, endTime);
    } else {
        success = historyManager->exportToCSV(fileName, selectedDrive, startTime, endTime);
    }
    
    if (success) {
        QMessageBox::information(this, "成功", "CSV文件导出成功！");
    } else {
        QMessageBox::critical(this, "错误", 
            QString("CSV文件导出失败: %1").arg(historyManager->getLastError()));
    }
}

void HistoryDialog::onExportExcel()
{
    if (currentData.isEmpty()) {
        QMessageBox::information(this, "提示", "没有数据可以导出！");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, 
        "导出Excel文件", 
        QString("磁盘历史数据_%1.xlsx").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "Excel文件 (*.xlsx);;CSV文件 (*.csv)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QString selectedDrive = driveComboBox->currentData().toString();
    QDateTime startTime = startTimeEdit->dateTime();
    QDateTime endTime = endTimeEdit->dateTime();
    
    bool success;
    if (selectedDrive.isEmpty()) {
        success = historyManager->exportToExcel(fileName, "", startTime, endTime);
    } else {
        success = historyManager->exportToExcel(fileName, selectedDrive, startTime, endTime);
    }
    
    if (success) {
        QMessageBox::information(this, "成功", "Excel文件导出成功！");
    } else {
        QMessageBox::critical(this, "错误", 
            QString("Excel文件导出失败: %1").arg(historyManager->getLastError()));
    }
}

void HistoryDialog::onDriveSelectionChanged()
{
    loadHistoryData();
}

void HistoryDialog::onTimeRangeChanged()
{
    // 延迟加载，避免频繁刷新
    QTimer::singleShot(500, this, &HistoryDialog::loadHistoryData);
}

void HistoryDialog::onAutoRefreshToggled(bool enabled)
{
    autoRefreshEnabled = enabled;
    
    if (enabled) {
        autoRefreshTimer->start();
        autoRefreshButton->setText("停止自动刷新");
        autoRefreshButton->setStyleSheet("QPushButton { background-color: #dc3545; }");
    } else {
        autoRefreshTimer->stop();
        autoRefreshButton->setText("自动刷新");
        autoRefreshButton->setStyleSheet("");
    }
}

void HistoryDialog::onAutoRefreshTimer()
{
    if (autoRefreshEnabled) {
        loadHistoryData();
    }
}
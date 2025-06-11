#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>

// MainWindow 实现
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_isCutOperation(false)
    , m_currentDirectory(QDir::currentPath())
{
    ui->setupUi(this);
    initUI();
    initFileManagementUI();
}

MainWindow::~MainWindow()
{
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
    delete ui;
}

void MainWindow::initUI()
{
    setWindowTitle("文件转移工具");
    setMinimumSize(800, 600);
    
    // 创建中央窗口部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // 源文件夹组
    QGroupBox *sourceGroup = new QGroupBox("源文件夹", this);
    QVBoxLayout *sourceLayout = new QVBoxLayout(sourceGroup);
    
    // 源文件夹列表
    m_sourceListWidget = new QListWidget(this);
    m_sourceListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    sourceLayout->addWidget(m_sourceListWidget);
    
    // 源文件夹按钮布局
    QHBoxLayout *sourceBtnLayout = new QHBoxLayout();
    m_addFolderBtn = new QPushButton("添加文件夹", this);
    m_removeFolderBtn = new QPushButton("移除选中", this);
    sourceBtnLayout->addWidget(m_addFolderBtn);
    sourceBtnLayout->addWidget(m_removeFolderBtn);
    sourceBtnLayout->addStretch();
    sourceLayout->addLayout(sourceBtnLayout);
    
    mainLayout->addWidget(sourceGroup);
    
    // 目标目录组
    QGroupBox *targetGroup = new QGroupBox("目标目录", this);
    QHBoxLayout *targetLayout = new QHBoxLayout(targetGroup);
    
    m_targetPathEdit = new QLineEdit(this);
    m_targetPathEdit->setPlaceholderText("请选择目标目录...");
    m_selectTargetBtn = new QPushButton("选择目录", this);
    
    targetLayout->addWidget(m_targetPathEdit);
    targetLayout->addWidget(m_selectTargetBtn);
    
    mainLayout->addWidget(targetGroup);
    
    // 转移选项组
    QGroupBox *optionsGroup = new QGroupBox("转移选项", this);
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);
    
    // 转移模式选择
    QLabel *modeLabel = new QLabel("转移模式:", this);
    optionsLayout->addWidget(modeLabel);
    
    m_keepStructureRadio = new QRadioButton("保持文件夹结构", this);
    m_keepStructureRadio->setChecked(true); // 默认选中
    m_keepStructureRadio->setToolTip("将源文件夹及其子文件夹完整复制到目标目录");
    optionsLayout->addWidget(m_keepStructureRadio);
    
    m_flattenFilesRadio = new QRadioButton("只提取文件(扁平化)", this);
    m_flattenFilesRadio->setToolTip("只复制文件，忽略文件夹结构，所有文件放在目标目录根目录下");
    optionsLayout->addWidget(m_flattenFilesRadio);
    
    // 覆盖选项
    m_overwriteCheckBox = new QCheckBox("覆盖已存在的文件", this);
    m_overwriteCheckBox->setToolTip("如果目标位置已存在同名文件，是否覆盖");
    optionsLayout->addWidget(m_overwriteCheckBox);
    
    mainLayout->addWidget(optionsGroup);
    
    // 筛选选项组
    QGroupBox *filterGroup = new QGroupBox("筛选选项", this);
    QVBoxLayout *filterLayout = new QVBoxLayout(filterGroup);
    
    // 创建筛选选项卡
    m_filterTabWidget = new QTabWidget(this);
    
    // 文件类型筛选选项卡
    QWidget *fileTypeTab = new QWidget();
    QFormLayout *fileTypeLayout = new QFormLayout(fileTypeTab);
    
    m_enableFileTypeFilter = new QCheckBox("启用文件类型筛选", this);
    fileTypeLayout->addRow(m_enableFileTypeFilter);
    
    m_fileTypeCombo = new QComboBox(this);
    m_fileTypeCombo->addItems({"图片文件", "文档文件", "视频文件", "音频文件", "压缩文件", "可执行文件", "自定义"});
    m_fileTypeCombo->setEnabled(false);
    fileTypeLayout->addRow("文件类型:", m_fileTypeCombo);
    
    // 添加显示文件类型后缀的标签
    m_fileTypeExtensionsLabel = new QLabel(this);
    m_fileTypeExtensionsLabel->setWordWrap(true);
    m_fileTypeExtensionsLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; padding: 5px; background-color: #f5f5f5; border: 1px solid #ddd; border-radius: 3px; }");
    m_fileTypeExtensionsLabel->setText("包含后缀: .jpg, .jpeg, .png, .gif, .bmp, .tiff, .webp, .svg");
    fileTypeLayout->addRow("包含后缀:", m_fileTypeExtensionsLabel);
    
    m_customExtensions = new QLineEdit(this);
    m_customExtensions->setPlaceholderText("输入文件扩展名，用逗号分隔，如: .txt,.doc,.pdf");
    m_customExtensions->setEnabled(false);
    fileTypeLayout->addRow("自定义扩展名:", m_customExtensions);
    
    m_filterTabWidget->addTab(fileTypeTab, "文件类型");
    
    // 文件大小筛选选项卡
    QWidget *sizeTab = new QWidget();
    QFormLayout *sizeLayout = new QFormLayout(sizeTab);
    
    m_enableSizeFilter = new QCheckBox("启用文件大小筛选", this);
    sizeLayout->addRow(m_enableSizeFilter);
    
    QHBoxLayout *minSizeLayout = new QHBoxLayout();
    m_minSizeSpinBox = new QSpinBox(this);
    m_minSizeSpinBox->setRange(0, 999999);
    m_minSizeSpinBox->setEnabled(false);
    m_sizeUnitCombo = new QComboBox(this);
    m_sizeUnitCombo->addItems({"字节", "KB", "MB", "GB"});
    m_sizeUnitCombo->setCurrentText("KB");
    m_sizeUnitCombo->setEnabled(false);
    minSizeLayout->addWidget(m_minSizeSpinBox);
    minSizeLayout->addWidget(m_sizeUnitCombo);
    sizeLayout->addRow("最小大小:", minSizeLayout);
    
    m_maxSizeSpinBox = new QSpinBox(this);
    m_maxSizeSpinBox->setRange(0, 999999);
    m_maxSizeSpinBox->setValue(1000);
    m_maxSizeSpinBox->setEnabled(false);
    sizeLayout->addRow("最大大小:", m_maxSizeSpinBox);
    
    m_filterTabWidget->addTab(sizeTab, "文件大小");
    
    // 修改时间筛选选项卡
    QWidget *timeTab = new QWidget();
    QFormLayout *timeLayout = new QFormLayout(timeTab);
    
    m_enableTimeFilter = new QCheckBox("启用修改时间筛选", this);
    timeLayout->addRow(m_enableTimeFilter);
    
    m_timeRangeCombo = new QComboBox(this);
    m_timeRangeCombo->addItems({"最近1天", "最近7天", "最近30天", "自定义范围"});
    m_timeRangeCombo->setEnabled(false);
    timeLayout->addRow("时间范围:", m_timeRangeCombo);
    
    m_startDateTime = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-7), this);
    m_startDateTime->setCalendarPopup(true);
    m_startDateTime->setEnabled(false);
    timeLayout->addRow("开始时间:", m_startDateTime);
    
    m_endDateTime = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_endDateTime->setCalendarPopup(true);
    m_endDateTime->setEnabled(false);
    timeLayout->addRow("结束时间:", m_endDateTime);
    
    m_filterTabWidget->addTab(timeTab, "修改时间");
    
    // 文件名模式匹配选项卡
    QWidget *nameTab = new QWidget();
    QFormLayout *nameLayout = new QFormLayout(nameTab);
    
    m_enableNameFilter = new QCheckBox("启用文件名筛选", this);
    nameLayout->addRow(m_enableNameFilter);
    
    m_patternTypeCombo = new QComboBox(this);
    m_patternTypeCombo->addItems({"通配符", "正则表达式"});
    m_patternTypeCombo->setEnabled(false);
    nameLayout->addRow("匹配类型:", m_patternTypeCombo);
    
    m_namePatternEdit = new QLineEdit(this);
    m_namePatternEdit->setPlaceholderText("输入匹配模式，如: *.txt 或 [0-9]+\.jpg");
    m_namePatternEdit->setEnabled(false);
    nameLayout->addRow("匹配模式:", m_namePatternEdit);
    
    m_filterTabWidget->addTab(nameTab, "文件名");
    
    // 排除列表选项卡
    QWidget *excludeTab = new QWidget();
    QVBoxLayout *excludeLayout = new QVBoxLayout(excludeTab);
    
    m_enableExcludeFilter = new QCheckBox("启用排除列表", this);
    excludeLayout->addWidget(m_enableExcludeFilter);
    
    QLabel *excludeLabel = new QLabel("排除的文件/文件夹（每行一个）:", this);
    excludeLayout->addWidget(excludeLabel);
    
    m_excludeListEdit = new QTextEdit(this);
    m_excludeListEdit->setMaximumHeight(100);
    m_excludeListEdit->setPlaceholderText("输入要排除的文件或文件夹名称，每行一个\n例如:\n.git\n*.tmp\ntemp/");
    m_excludeListEdit->setEnabled(false);
    excludeLayout->addWidget(m_excludeListEdit);
    
    QHBoxLayout *excludeBtnLayout = new QHBoxLayout();
    m_addExcludeBtn = new QPushButton("添加常用排除项", this);
    m_clearExcludeBtn = new QPushButton("清空列表", this);
    m_addExcludeBtn->setEnabled(false);
    m_clearExcludeBtn->setEnabled(false);
    excludeBtnLayout->addWidget(m_addExcludeBtn);
    excludeBtnLayout->addWidget(m_clearExcludeBtn);
    excludeBtnLayout->addStretch();
    excludeLayout->addLayout(excludeBtnLayout);
    
    m_filterTabWidget->addTab(excludeTab, "排除列表");
    
    filterLayout->addWidget(m_filterTabWidget);
    mainLayout->addWidget(filterGroup);
    
    // 连接筛选相关信号槽
    connect(m_enableFileTypeFilter, &QCheckBox::toggled, [this](bool enabled) {
        m_fileTypeCombo->setEnabled(enabled);
        m_customExtensions->setEnabled(enabled && m_fileTypeCombo->currentText() == "自定义");
    });
    
    connect(m_fileTypeCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged), [this](const QString &text) {
        m_customExtensions->setEnabled(m_enableFileTypeFilter->isChecked() && text == "自定义");
        updateFileTypeExtensionsDisplay(text);
    });
    
    connect(m_enableSizeFilter, &QCheckBox::toggled, [this](bool enabled) {
        m_minSizeSpinBox->setEnabled(enabled);
        m_maxSizeSpinBox->setEnabled(enabled);
        m_sizeUnitCombo->setEnabled(enabled);
    });
    
    connect(m_enableTimeFilter, &QCheckBox::toggled, [this](bool enabled) {
        m_timeRangeCombo->setEnabled(enabled);
        bool customRange = enabled && m_timeRangeCombo->currentText() == "自定义范围";
        m_startDateTime->setEnabled(customRange);
        m_endDateTime->setEnabled(customRange);
    });
    
    connect(m_timeRangeCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged), [this](const QString &text) {
        bool customRange = m_enableTimeFilter->isChecked() && text == "自定义范围";
        m_startDateTime->setEnabled(customRange);
        m_endDateTime->setEnabled(customRange);
    });
    
    connect(m_enableNameFilter, &QCheckBox::toggled, [this](bool enabled) {
        m_patternTypeCombo->setEnabled(enabled);
        m_namePatternEdit->setEnabled(enabled);
    });
    
    connect(m_enableExcludeFilter, &QCheckBox::toggled, [this](bool enabled) {
        m_excludeListEdit->setEnabled(enabled);
        m_addExcludeBtn->setEnabled(enabled);
        m_clearExcludeBtn->setEnabled(enabled);
    });
    
    connect(m_addExcludeBtn, &QPushButton::clicked, [this]() {
        QString commonExcludes = ".git\n.svn\n.DS_Store\nThumbs.db\n*.tmp\n*.temp\n*.log\n*.bak";
        if (!m_excludeListEdit->toPlainText().isEmpty()) {
            m_excludeListEdit->append("\n" + commonExcludes);
        } else {
            m_excludeListEdit->setPlainText(commonExcludes);
        }
    });
    
    connect(m_clearExcludeBtn, &QPushButton::clicked, [this]() {
        m_excludeListEdit->clear();
    });
    
    // 操作按钮
    QHBoxLayout *actionLayout = new QHBoxLayout();
    m_startTransferBtn = new QPushButton("开始转移", this);
    m_startTransferBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
    actionLayout->addStretch();
    actionLayout->addWidget(m_startTransferBtn);
    actionLayout->addStretch();
    
    mainLayout->addLayout(actionLayout);
    
    // 进度条和状态
    QGroupBox *progressGroup = new QGroupBox("转移进度", this);
    QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    progressLayout->addWidget(m_progressBar);
    
    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setStyleSheet("QLabel { color: #666; }");
    progressLayout->addWidget(m_statusLabel);
    
    mainLayout->addWidget(progressGroup);
    
    // 连接信号槽
    connect(m_addFolderBtn, &QPushButton::clicked, this, &MainWindow::addSourceFolders);
    connect(m_removeFolderBtn, &QPushButton::clicked, this, &MainWindow::removeSelectedFolders);
    connect(m_selectTargetBtn, &QPushButton::clicked, this, &MainWindow::selectTargetDirectory);
    connect(m_startTransferBtn, &QPushButton::clicked, this, &MainWindow::startTransfer);
}

void MainWindow::addSourceFolders()
{
    QString folder = QFileDialog::getExistingDirectory(this, "选择源文件夹", "", 
                                                      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!folder.isEmpty()) {
        // 检查是否已经添加过
        bool exists = false;
        for (int i = 0; i < m_sourceListWidget->count(); ++i) {
            if (m_sourceListWidget->item(i)->text() == folder) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            m_sourceListWidget->addItem(folder);
        }
    }
}

void MainWindow::removeSelectedFolders()
{
    QList<QListWidgetItem*> selectedItems = m_sourceListWidget->selectedItems();
    for (QListWidgetItem *item : selectedItems) {
        delete m_sourceListWidget->takeItem(m_sourceListWidget->row(item));
    }
}

void MainWindow::selectTargetDirectory()
{
    QString targetDir = QFileDialog::getExistingDirectory(this, "选择目标目录", "", 
                                                         QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!targetDir.isEmpty()) {
        m_targetPathEdit->setText(targetDir);
    }
}

void MainWindow::startTransfer()
{
    // 检查输入
    if (m_sourceListWidget->count() == 0) {
        QMessageBox::warning(this, "警告", "请至少添加一个源文件夹！");
        return;
    }
    
    if (m_targetPathEdit->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择目标目录！");
        return;
    }
    
    // 获取源路径列表
    QStringList sourcePaths;
    for (int i = 0; i < m_sourceListWidget->count(); ++i) {
        sourcePaths.append(m_sourceListWidget->item(i)->text());
    }
    
    // 获取转移选项
    TransferMode mode = m_keepStructureRadio->isChecked() ? TransferMode::KeepStructure : TransferMode::FlattenFiles;
    bool overwrite = m_overwriteCheckBox->isChecked();
    
    // 收集筛选选项
    FilterOptions filterOptions;
    
    // 文件类型筛选
    filterOptions.enableFileTypeFilter = m_enableFileTypeFilter->isChecked();
    if (filterOptions.enableFileTypeFilter) {
        QString fileType = m_fileTypeCombo->currentText();
        if (fileType == "自定义") {
            QString customExt = m_customExtensions->text().trimmed();
            if (!customExt.isEmpty()) {
                QStringList extensions = customExt.split(',', QString::SkipEmptyParts);
                for (QString &ext : extensions) {
                    ext = ext.trimmed();
                    if (!ext.startsWith('.')) {
                        ext = '.' + ext;
                    }
                }
                filterOptions.allowedExtensions = extensions;
            }
        } else {
            filterOptions.allowedExtensions = getFileTypeExtensions(fileType);
        }
    }
    
    // 文件大小筛选
    filterOptions.enableSizeFilter = m_enableSizeFilter->isChecked();
    if (filterOptions.enableSizeFilter) {
        QString unit = m_sizeUnitCombo->currentText();
        filterOptions.minSize = convertSizeToBytes(m_minSizeSpinBox->value(), unit);
        filterOptions.maxSize = convertSizeToBytes(m_maxSizeSpinBox->value(), unit);
    }
    
    // 修改时间筛选
    filterOptions.enableTimeFilter = m_enableTimeFilter->isChecked();
    if (filterOptions.enableTimeFilter) {
        QString timeRange = m_timeRangeCombo->currentText();
        if (timeRange == "自定义范围") {
            filterOptions.startTime = m_startDateTime->dateTime();
            filterOptions.endTime = m_endDateTime->dateTime();
        } else {
            QDateTime now = QDateTime::currentDateTime();
            filterOptions.endTime = now;
            if (timeRange == "最近1天") {
                filterOptions.startTime = now.addDays(-1);
            } else if (timeRange == "最近7天") {
                filterOptions.startTime = now.addDays(-7);
            } else if (timeRange == "最近30天") {
                filterOptions.startTime = now.addDays(-30);
            }
        }
    }
    
    // 文件名模式匹配
    filterOptions.enableNameFilter = m_enableNameFilter->isChecked();
    if (filterOptions.enableNameFilter) {
        filterOptions.namePattern = m_namePatternEdit->text().trimmed();
        filterOptions.useRegex = (m_patternTypeCombo->currentText() == "正则表达式");
    }
    
    // 排除列表
    filterOptions.enableExcludeFilter = m_enableExcludeFilter->isChecked();
    if (filterOptions.enableExcludeFilter) {
        QString excludeText = m_excludeListEdit->toPlainText().trimmed();
        if (!excludeText.isEmpty()) {
            QStringList excludeItems = excludeText.split('\n', QString::SkipEmptyParts);
            for (QString &item : excludeItems) {
                item = item.trimmed();
                if (!item.isEmpty()) {
                    filterOptions.excludeList.append(item);
                }
            }
        }
    }
    
    // 创建工作线程
    m_workerThread = new QThread(this);
    m_worker = new FileTransferWorker(sourcePaths, m_targetPathEdit->text(), mode, overwrite, filterOptions);
    m_worker->moveToThread(m_workerThread);
    
    // 连接信号槽
    connect(m_workerThread, &QThread::started, m_worker, &FileTransferWorker::startTransfer);
    connect(m_worker, &FileTransferWorker::progressUpdated, this, &MainWindow::updateProgress);
    connect(m_worker, &FileTransferWorker::transferFinished, this, &MainWindow::onTransferFinished);
    connect(m_worker, &FileTransferWorker::transferFinished, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_worker, &FileTransferWorker::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QThread::deleteLater);
    
    // 设置UI状态
    setUIEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText("正在准备转移...");
    
    // 启动线程
    m_workerThread->start();
}

void MainWindow::updateProgress(int current, int total, const QString &fileName)
{
    m_progressBar->setMaximum(total);
    m_progressBar->setValue(current);
    m_statusLabel->setText(QString("正在转移: %1 (%2/%3)").arg(fileName).arg(current).arg(total));
}

void MainWindow::onTransferFinished(bool success, const QString &message)
{
    setUIEnabled(true);
    m_progressBar->setVisible(false);
    
    if (success) {
        m_statusLabel->setText("转移完成: " + message);
        QMessageBox::information(this, "成功", message);
    } else {
        m_statusLabel->setText("转移失败: " + message);
        QMessageBox::critical(this, "错误", message);
    }
    
    m_workerThread = nullptr;
    m_worker = nullptr;
}

void MainWindow::setUIEnabled(bool enabled)
{
    m_addFolderBtn->setEnabled(enabled);
    m_removeFolderBtn->setEnabled(enabled);
    m_selectTargetBtn->setEnabled(enabled);
    m_startTransferBtn->setEnabled(enabled);
    m_sourceListWidget->setEnabled(enabled);
    m_targetPathEdit->setEnabled(enabled);
}

QStringList MainWindow::getFileTypeExtensions(const QString &fileType)
{
    if (fileType == "图片文件") {
        return {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".tiff", ".webp", ".svg", ".ico"};
    } else if (fileType == "文档文件") {
        return {".txt", ".doc", ".docx", ".pdf", ".xls", ".xlsx", ".ppt", ".pptx", ".rtf", ".odt", ".ods", ".odp"};
    } else if (fileType == "视频文件") {
        return {".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm", ".m4v", ".3gp", ".mpg", ".mpeg"};
    } else if (fileType == "音频文件") {
        return {".mp3", ".wav", ".flac", ".aac", ".ogg", ".wma", ".m4a", ".opus", ".aiff"};
    } else if (fileType == "压缩文件") {
        return {".zip", ".rar", ".7z", ".tar", ".gz", ".bz2", ".xz", ".tar.gz", ".tar.bz2", ".tar.xz"};
    } else if (fileType == "可执行文件") {
        return {".exe", ".msi", ".bat", ".cmd", ".com", ".scr", ".dll", ".sys"};
    }
    return {};
}

qint64 MainWindow::convertSizeToBytes(int size, const QString &unit)
{
    qint64 bytes = size;
    if (unit == "KB") {
        bytes *= 1024;
    } else if (unit == "MB") {
        bytes *= 1024 * 1024;
    } else if (unit == "GB") {
        bytes *= 1024 * 1024 * 1024;
    }
    return bytes;
}

void MainWindow::updateFileTypeExtensionsDisplay(const QString &fileType)
{
    if (fileType == "自定义") {
        m_fileTypeExtensionsLabel->setText("请在下方输入自定义扩展名");
        m_fileTypeExtensionsLabel->setStyleSheet("QLabel { color: #888; font-size: 11px; padding: 5px; background-color: #f9f9f9; border: 1px solid #ddd; border-radius: 3px; }");
    } else {
        QStringList extensions = getFileTypeExtensions(fileType);
        if (!extensions.isEmpty()) {
            QString extensionsText = "包含后缀: " + extensions.join(", ");
            m_fileTypeExtensionsLabel->setText(extensionsText);
            m_fileTypeExtensionsLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; padding: 5px; background-color: #f5f5f5; border: 1px solid #ddd; border-radius: 3px; }");
        } else {
            m_fileTypeExtensionsLabel->setText("未知文件类型");
            m_fileTypeExtensionsLabel->setStyleSheet("QLabel { color: #999; font-size: 11px; padding: 5px; background-color: #f9f9f9; border: 1px solid #ddd; border-radius: 3px; }");
        }
    }
}

// 文件管理功能实现
void MainWindow::initFileManagementUI()
{
    // 创建文件管理选项卡
    QWidget *fileManagementTab = new QWidget();
    QVBoxLayout *fileManagementLayout = new QVBoxLayout(fileManagementTab);
    
    // 当前路径显示和导航
    QHBoxLayout *pathLayout = new QHBoxLayout();
    QLabel *pathLabel = new QLabel("当前路径:", this);
    
    // 添加盘符选择下拉框
    QLabel *driveLabel = new QLabel("盘符:", this);
    m_driveComboBox = new QComboBox(this);
    
    // 获取所有可用的盘符
    QFileInfoList drives = QDir::drives();
    for (const QFileInfo &drive : drives) {
        QString drivePath = drive.absolutePath();
        m_driveComboBox->addItem(drivePath, drivePath);
    }
    
    // 设置当前盘符为当前目录所在的盘符
    QString currentDrive = QDir(m_currentDirectory).rootPath();
    int driveIndex = m_driveComboBox->findData(currentDrive);
    if (driveIndex >= 0) {
        m_driveComboBox->setCurrentIndex(driveIndex);
    }
    
    m_currentPathEdit = new QLineEdit(this);
    m_currentPathEdit->setText(m_currentDirectory);
    m_currentPathEdit->setReadOnly(true);
    m_upDirBtn = new QPushButton("上级目录", this);
    m_refreshBtn = new QPushButton("刷新", this);
    
    pathLayout->addWidget(driveLabel);
    pathLayout->addWidget(m_driveComboBox);
    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(m_currentPathEdit);
    pathLayout->addWidget(m_upDirBtn);
    pathLayout->addWidget(m_refreshBtn);
    
    fileManagementLayout->addLayout(pathLayout);
    
    // 文件列表
    m_fileListWidget = new QListWidget(this);
    m_fileListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    fileManagementLayout->addWidget(m_fileListWidget);
    
    // 文件操作按钮
    QHBoxLayout *fileOpsLayout = new QHBoxLayout();
    m_copyBtn = new QPushButton("复制", this);
    m_cutBtn = new QPushButton("剪切", this);
    m_pasteBtn = new QPushButton("粘贴", this);
    m_deleteBtn = new QPushButton("删除", this);
    m_renameBtn = new QPushButton("重命名", this);
    m_findBtn = new QPushButton("查找", this);
    m_propertiesBtn = new QPushButton("属性", this);
    
    // 设置按钮样式
    QString buttonStyle = "QPushButton { padding: 6px 12px; margin: 2px; }";
    m_copyBtn->setStyleSheet(buttonStyle);
    m_cutBtn->setStyleSheet(buttonStyle);
    m_pasteBtn->setStyleSheet(buttonStyle + "QPushButton:disabled { color: #999; }");
    m_deleteBtn->setStyleSheet(buttonStyle + "QPushButton { background-color: #f44336; color: white; } QPushButton:hover { background-color: #d32f2f; }");
    m_renameBtn->setStyleSheet(buttonStyle);
    m_findBtn->setStyleSheet(buttonStyle);
    m_propertiesBtn->setStyleSheet(buttonStyle);
    
    fileOpsLayout->addWidget(m_copyBtn);
    fileOpsLayout->addWidget(m_cutBtn);
    fileOpsLayout->addWidget(m_pasteBtn);
    fileOpsLayout->addWidget(m_deleteBtn);
    fileOpsLayout->addWidget(m_renameBtn);
    fileOpsLayout->addWidget(m_findBtn);
    fileOpsLayout->addWidget(m_propertiesBtn);
    fileOpsLayout->addStretch();
    
    fileManagementLayout->addLayout(fileOpsLayout);
    
    // 将文件管理选项卡添加到筛选选项卡组件中
    m_filterTabWidget->addTab(fileManagementTab, "文件管理");
    
    // 连接信号槽
    connect(m_driveComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        if (index >= 0) {
            QString selectedDrive = m_driveComboBox->itemData(index).toString();
            m_currentDirectory = selectedDrive;
            loadDirectoryContent(m_currentDirectory);
        }
    });
    
    connect(m_upDirBtn, &QPushButton::clicked, [this]() {
        QDir dir(m_currentDirectory);
        if (dir.cdUp()) {
            m_currentDirectory = dir.absolutePath();
            loadDirectoryContent(m_currentDirectory);
            
            // 更新盘符选择框
            QString currentDrive = QDir(m_currentDirectory).rootPath();
            int driveIndex = m_driveComboBox->findData(currentDrive);
            if (driveIndex >= 0 && driveIndex != m_driveComboBox->currentIndex()) {
                m_driveComboBox->setCurrentIndex(driveIndex);
            }
        }
    });
    
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshFileList);
    connect(m_fileListWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::onFileListDoubleClicked);
    connect(m_fileListWidget, &QListWidget::customContextMenuRequested, this, &MainWindow::showFileListContextMenu);
    connect(m_fileListWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::updateFileManagementButtons);
    
    connect(m_copyBtn, &QPushButton::clicked, this, &MainWindow::copySelectedItems);
    connect(m_cutBtn, &QPushButton::clicked, this, &MainWindow::cutSelectedItems);
    connect(m_pasteBtn, &QPushButton::clicked, this, &MainWindow::pasteItems);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainWindow::deleteSelectedItems);
    connect(m_renameBtn, &QPushButton::clicked, this, &MainWindow::renameSelectedItem);
    connect(m_findBtn, &QPushButton::clicked, this, &MainWindow::findFiles);
    connect(m_propertiesBtn, &QPushButton::clicked, this, &MainWindow::showProperties);
    
    // 初始化文件列表
    loadDirectoryContent(m_currentDirectory);
    updateFileManagementButtons();
}

void MainWindow::loadDirectoryContent(const QString &dirPath)
{
    m_fileListWidget->clear();
    m_currentDirectory = dirPath;
    m_currentPathEdit->setText(dirPath);
    
    // 更新盘符选择框
    QString currentDrive = QDir(dirPath).rootPath();
    int driveIndex = m_driveComboBox->findData(currentDrive);
    if (driveIndex >= 0 && driveIndex != m_driveComboBox->currentIndex()) {
        // 临时断开信号连接，避免递归调用
        m_driveComboBox->blockSignals(true);
        m_driveComboBox->setCurrentIndex(driveIndex);
        m_driveComboBox->blockSignals(false);
    }
    
    QDir dir(dirPath);
    if (!dir.exists()) {
        return;
    }
    
    // 添加文件夹
    QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &dirName : dirs) {
        QListWidgetItem *item = new QListWidgetItem(m_fileListWidget);
        QString fullPath = dir.absoluteFilePath(dirName);
        item->setText(QString("📁 %1").arg(dirName));
        item->setData(Qt::UserRole, fullPath);
        item->setData(Qt::UserRole + 1, "directory");
        item->setToolTip(fullPath);
    }
    
    // 添加文件
    QStringList files = dir.entryList(QDir::Files, QDir::Name);
    for (const QString &fileName : files) {
        QListWidgetItem *item = new QListWidgetItem(m_fileListWidget);
        QString fullPath = dir.absoluteFilePath(fileName);
        QFileInfo fileInfo(fullPath);
        
        QString icon = getFileIconType(fullPath);
        QString sizeStr = formatFileSize(fileInfo.size());
        QString displayText = QString("%1 %2 (%3)").arg(icon).arg(fileName).arg(sizeStr);
        
        item->setText(displayText);
        item->setData(Qt::UserRole, fullPath);
        item->setData(Qt::UserRole + 1, "file");
        item->setToolTip(QString("%1\n大小: %2\n修改时间: %3")
                        .arg(fullPath)
                        .arg(sizeStr)
                        .arg(fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss")));
    }
}

void MainWindow::copySelectedItems()
{
    m_clipboardPaths = getSelectedFilePaths();
    m_isCutOperation = false;
    updateFileManagementButtons();
    
    if (!m_clipboardPaths.isEmpty()) {
        m_statusLabel->setText(QString("已复制 %1 个项目到剪贴板").arg(m_clipboardPaths.size()));
    }
}

void MainWindow::cutSelectedItems()
{
    m_clipboardPaths = getSelectedFilePaths();
    m_isCutOperation = true;
    updateFileManagementButtons();
    
    if (!m_clipboardPaths.isEmpty()) {
        m_statusLabel->setText(QString("已剪切 %1 个项目到剪贴板").arg(m_clipboardPaths.size()));
    }
}

void MainWindow::pasteItems()
{
    if (m_clipboardPaths.isEmpty()) {
        QMessageBox::information(this, "提示", "剪贴板为空！");
        return;
    }
    
    bool success = performFileCopy(m_clipboardPaths, m_currentDirectory, m_isCutOperation);
    
    if (success) {
        if (m_isCutOperation) {
            m_clipboardPaths.clear();
            m_isCutOperation = false;
        }
        refreshFileList();
        m_statusLabel->setText("粘贴操作完成");
    } else {
        QMessageBox::critical(this, "错误", "粘贴操作失败！");
    }
    
    updateFileManagementButtons();
}

void MainWindow::deleteSelectedItems()
{
    QStringList selectedPaths = getSelectedFilePaths();
    if (selectedPaths.isEmpty()) {
        QMessageBox::information(this, "提示", "请选择要删除的文件或文件夹！");
        return;
    }
    
    int ret = QMessageBox::question(this, "确认删除", 
                                   QString("确定要删除选中的 %1 个项目吗？\n\n注意：此操作不可撤销！")
                                   .arg(selectedPaths.size()),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    int successCount = 0;
    int failCount = 0;
    
    for (const QString &path : selectedPaths) {
        QFileInfo fileInfo(path);
        bool success = false;
        
        if (fileInfo.isDir()) {
            QDir dir(path);
            success = dir.removeRecursively();
        } else {
            QFile file(path);
            success = file.remove();
        }
        
        if (success) {
            successCount++;
        } else {
            failCount++;
        }
    }
    
    refreshFileList();
    
    if (failCount == 0) {
        m_statusLabel->setText(QString("成功删除 %1 个项目").arg(successCount));
    } else {
        QMessageBox::warning(this, "删除结果", 
                           QString("删除完成：成功 %1 个，失败 %2 个")
                           .arg(successCount).arg(failCount));
    }
}

void MainWindow::renameSelectedItem()
{
    QStringList selectedPaths = getSelectedFilePaths();
    if (selectedPaths.isEmpty()) {
        QMessageBox::information(this, "提示", "请选择要重命名的文件或文件夹！");
        return;
    }
    
    if (selectedPaths.size() > 1) {
        QMessageBox::information(this, "提示", "一次只能重命名一个项目！");
        return;
    }
    
    QString oldPath = selectedPaths.first();
    QFileInfo fileInfo(oldPath);
    QString oldName = fileInfo.fileName();
    
    bool ok;
    QString newName = QInputDialog::getText(this, "重命名", 
                                          QString("请输入新名称："),
                                          QLineEdit::Normal, oldName, &ok);
    
    if (!ok || newName.isEmpty() || newName == oldName) {
        return;
    }
    
    QString newPath = fileInfo.dir().absoluteFilePath(newName);
    
    if (QFileInfo::exists(newPath)) {
        QMessageBox::warning(this, "错误", "目标名称已存在！");
        return;
    }
    
    QDir dir;
    bool success = dir.rename(oldPath, newPath);
    
    if (success) {
        refreshFileList();
        m_statusLabel->setText(QString("重命名成功：%1 -> %2").arg(oldName).arg(newName));
    } else {
        QMessageBox::critical(this, "错误", "重命名失败！");
    }
}

void MainWindow::findFiles()
{
    bool ok;
    QString searchPattern = QInputDialog::getText(this, "查找文件", 
                                                "请输入搜索模式（支持通配符，如 *.txt）：",
                                                QLineEdit::Normal, "", &ok);
    
    if (!ok || searchPattern.isEmpty()) {
        return;
    }
    
    m_fileListWidget->clear();
    
    QDir dir(m_currentDirectory);
    QStringList nameFilters;
    nameFilters << searchPattern;
    
    // 搜索文件
    QFileInfoList files = dir.entryInfoList(nameFilters, QDir::Files, QDir::Name);
    for (const QFileInfo &fileInfo : files) {
        QListWidgetItem *item = new QListWidgetItem(m_fileListWidget);
        QString icon = getFileIconType(fileInfo.absoluteFilePath());
        QString sizeStr = formatFileSize(fileInfo.size());
        QString displayText = QString("%1 %2 (%3)").arg(icon).arg(fileInfo.fileName()).arg(sizeStr);
        
        item->setText(displayText);
        item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
        item->setData(Qt::UserRole + 1, "file");
        item->setToolTip(QString("%1\n大小: %2\n修改时间: %3")
                        .arg(fileInfo.absoluteFilePath())
                        .arg(sizeStr)
                        .arg(fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss")));
    }
    
    m_statusLabel->setText(QString("找到 %1 个匹配的文件").arg(files.size()));
}

void MainWindow::showProperties()
{
    QStringList selectedPaths = getSelectedFilePaths();
    if (selectedPaths.isEmpty()) {
        QMessageBox::information(this, "提示", "请选择要查看属性的文件或文件夹！");
        return;
    }
    
    QString path = selectedPaths.first();
    QFileInfo fileInfo(path);
    
    QString properties;
    properties += QString("名称: %1\n").arg(fileInfo.fileName());
    properties += QString("路径: %1\n").arg(fileInfo.absolutePath());
    properties += QString("类型: %1\n").arg(fileInfo.isDir() ? "文件夹" : "文件");
    
    if (fileInfo.isFile()) {
        properties += QString("大小: %1 (%2 字节)\n")
                     .arg(formatFileSize(fileInfo.size()))
                     .arg(fileInfo.size());
    }
    
    properties += QString("创建时间: %1\n").arg(fileInfo.birthTime().toString("yyyy-MM-dd hh:mm:ss"));
    properties += QString("修改时间: %1\n").arg(fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
    properties += QString("访问时间: %1\n").arg(fileInfo.lastRead().toString("yyyy-MM-dd hh:mm:ss"));
    
    // 权限信息
    QFile::Permissions permissions = fileInfo.permissions();
    QString permStr = "权限: ";
    permStr += (permissions & QFile::ReadOwner) ? "r" : "-";
    permStr += (permissions & QFile::WriteOwner) ? "w" : "-";
    permStr += (permissions & QFile::ExeOwner) ? "x" : "-";
    properties += permStr + "\n";
    
    properties += QString("只读: %1\n").arg(fileInfo.isReadable() && !fileInfo.isWritable() ? "是" : "否");
    properties += QString("隐藏: %1\n").arg(fileInfo.isHidden() ? "是" : "否");
    
    QMessageBox::information(this, "文件属性", properties);
}

void MainWindow::refreshFileList()
{
    loadDirectoryContent(m_currentDirectory);
    updateFileManagementButtons();
    m_statusLabel->setText("文件列表已刷新");
}

void MainWindow::onFileListDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    QString path = item->data(Qt::UserRole).toString();
    QString type = item->data(Qt::UserRole + 1).toString();
    
    if (type == "directory") {
        // 进入文件夹
        loadDirectoryContent(path);
    } else {
        // 打开文件
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void MainWindow::showFileListContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_fileListWidget->itemAt(pos);
    
    QMenu contextMenu(this);
    
    if (item) {
        contextMenu.addAction("打开", [this, item]() {
            onFileListDoubleClicked(item);
        });
        contextMenu.addSeparator();
        contextMenu.addAction("复制", this, &MainWindow::copySelectedItems);
        contextMenu.addAction("剪切", this, &MainWindow::cutSelectedItems);
        
        if (!m_clipboardPaths.isEmpty()) {
            contextMenu.addAction("粘贴", this, &MainWindow::pasteItems);
        }
        
        contextMenu.addSeparator();
        contextMenu.addAction("删除", this, &MainWindow::deleteSelectedItems);
        contextMenu.addAction("重命名", this, &MainWindow::renameSelectedItem);
        contextMenu.addSeparator();
        contextMenu.addAction("属性", this, &MainWindow::showProperties);
    } else {
        if (!m_clipboardPaths.isEmpty()) {
            contextMenu.addAction("粘贴", this, &MainWindow::pasteItems);
        }
        contextMenu.addAction("刷新", this, &MainWindow::refreshFileList);
    }
    
    contextMenu.exec(m_fileListWidget->mapToGlobal(pos));
}

void MainWindow::updateFileManagementButtons()
{
    QStringList selectedPaths = getSelectedFilePaths();
    bool hasSelection = !selectedPaths.isEmpty();
    bool hasClipboard = !m_clipboardPaths.isEmpty();
    
    m_copyBtn->setEnabled(hasSelection);
    m_cutBtn->setEnabled(hasSelection);
    m_pasteBtn->setEnabled(hasClipboard);
    m_deleteBtn->setEnabled(hasSelection);
    m_renameBtn->setEnabled(selectedPaths.size() == 1);
    m_propertiesBtn->setEnabled(selectedPaths.size() == 1);
}

bool MainWindow::performFileCopy(const QStringList &sourcePaths, const QString &targetDir, bool isCut)
{
    if (sourcePaths.isEmpty() || targetDir.isEmpty()) {
        return false;
    }
    
    QDir target(targetDir);
    if (!target.exists()) {
        return false;
    }
    
    bool allSuccess = true;
    
    for (const QString &sourcePath : sourcePaths) {
        QFileInfo sourceInfo(sourcePath);
        QString targetPath = target.absoluteFilePath(sourceInfo.fileName());
        
        // 检查目标是否已存在
        if (QFileInfo::exists(targetPath)) {
            int ret = QMessageBox::question(this, "文件已存在", 
                                          QString("目标位置已存在文件 '%1'，是否覆盖？")
                                          .arg(sourceInfo.fileName()),
                                          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                          QMessageBox::No);
            
            if (ret == QMessageBox::Cancel) {
                return false;
            } else if (ret == QMessageBox::No) {
                continue;
            }
            
            // 删除目标文件/文件夹
            if (QFileInfo(targetPath).isDir()) {
                QDir(targetPath).removeRecursively();
            } else {
                QFile::remove(targetPath);
            }
        }
        
        bool success = false;
        
        if (sourceInfo.isDir()) {
            // 复制文件夹
            success = copyDirectoryRecursively(sourcePath, targetPath);
        } else {
            // 复制文件
            success = QFile::copy(sourcePath, targetPath);
        }
        
        if (success && isCut) {
            // 剪切操作：删除源文件
            if (sourceInfo.isDir()) {
                QDir(sourcePath).removeRecursively();
            } else {
                QFile::remove(sourcePath);
            }
        }
        
        if (!success) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

bool MainWindow::copyDirectoryRecursively(const QString &sourceDir, const QString &targetDir)
{
    QDir source(sourceDir);
    QDir target(targetDir);
    
    if (!target.exists()) {
        if (!target.mkpath(".")) {
            return false;
        }
    }
    
    // 复制文件
    QStringList files = source.entryList(QDir::Files);
    for (const QString &fileName : files) {
        QString sourcePath = source.absoluteFilePath(fileName);
        QString targetPath = target.absoluteFilePath(fileName);
        
        if (!QFile::copy(sourcePath, targetPath)) {
            return false;
        }
    }
    
    // 递归复制子文件夹
    QStringList dirs = source.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &dirName : dirs) {
        QString sourcePath = source.absoluteFilePath(dirName);
        QString targetPath = target.absoluteFilePath(dirName);
        
        if (!copyDirectoryRecursively(sourcePath, targetPath)) {
            return false;
        }
    }
    
    return true;
}

QStringList MainWindow::getSelectedFilePaths()
{
    QStringList paths;
    QList<QListWidgetItem*> selectedItems = m_fileListWidget->selectedItems();
    
    for (QListWidgetItem *item : selectedItems) {
        QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty()) {
            paths.append(path);
        }
    }
    
    return paths;
}

QString MainWindow::formatFileSize(qint64 bytes)
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    
    if (bytes >= GB) {
        return QString("%1 GB").arg(QString::number(bytes / (double)GB, 'f', 2));
    } else if (bytes >= MB) {
        return QString("%1 MB").arg(QString::number(bytes / (double)MB, 'f', 2));
    } else if (bytes >= KB) {
        return QString("%1 KB").arg(QString::number(bytes / (double)KB, 'f', 2));
    } else {
        return QString("%1 B").arg(bytes);
    }
}

QString MainWindow::getFileIconType(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    
    // 图片文件
    if (QStringList({"jpg", "jpeg", "png", "gif", "bmp", "tiff", "webp", "svg", "ico"}).contains(suffix)) {
        return "🖼️";
    }
    // 文档文件
    else if (QStringList({"txt", "doc", "docx", "pdf", "rtf", "odt"}).contains(suffix)) {
        return "📄";
    }
    // 表格文件
    else if (QStringList({"xls", "xlsx", "csv", "ods"}).contains(suffix)) {
        return "📊";
    }
    // 演示文件
    else if (QStringList({"ppt", "pptx", "odp"}).contains(suffix)) {
        return "📽️";
    }
    // 视频文件
    else if (QStringList({"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "m4v", "3gp", "mpg", "mpeg"}).contains(suffix)) {
        return "🎬";
    }
    // 音频文件
    else if (QStringList({"mp3", "wav", "flac", "aac", "ogg", "wma", "m4a", "opus", "aiff"}).contains(suffix)) {
        return "🎵";
    }
    // 压缩文件
    else if (QStringList({"zip", "rar", "7z", "tar", "gz", "bz2", "xz"}).contains(suffix)) {
        return "📦";
    }
    // 可执行文件
    else if (QStringList({"exe", "msi", "bat", "cmd", "com", "scr"}).contains(suffix)) {
        return "⚙️";
    }
    // 代码文件
    else if (QStringList({"cpp", "h", "c", "py", "java", "js", "html", "css", "php", "rb", "go", "rs"}).contains(suffix)) {
        return "💻";
    }
    // 默认文件图标
    else {
        return "📋";
    }
}


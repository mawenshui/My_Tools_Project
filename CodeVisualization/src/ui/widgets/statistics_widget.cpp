#include "statistics_widget.h"
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>
#include <QtCore/QLocale>
#include <QtGui/QFont>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtCore/QBuffer>
#include <QtCharts/QChart>
#include <QtWidgets/QGraphicsScene>

StatisticsWidget::StatisticsWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_tabWidget(nullptr)
    , m_overviewTab(nullptr)
    , m_overviewLayout(nullptr)
    , m_projectPathLabel(nullptr)
    , m_analysisTimeLabel(nullptr)
    , m_totalFilesLabel(nullptr)
    , m_totalLinesLabel(nullptr)
    , m_codeLinesLabel(nullptr)
    , m_commentLinesLabel(nullptr)
    , m_blankLinesLabel(nullptr)
    , m_languageCountLabel(nullptr)
    , m_detailedTableTab(nullptr)
    , m_detailedTable(nullptr)
    , m_fileListTab(nullptr)
    , m_fileTree(nullptr)
    , m_languageStatsTab(nullptr)
    , m_languageTable(nullptr)
    , m_toolBar(nullptr)
    , m_exportCSVButton(nullptr)
    , m_exportHTMLButton(nullptr)
    , m_exportMarkdownButton(nullptr)
    , m_refreshButton(nullptr)
    , m_progressBar(nullptr)
    , m_progressLabel(nullptr)
    , m_currentDisplayMode(Overview)
    , m_csvExporter(nullptr)
    , m_markdownExporter(nullptr)
{
    // 初始化CSV导出器
    m_csvExporter = new CSVExporter(this);
    
    // 连接CSV导出器信号
    connect(m_csvExporter, &CSVExporter::progressUpdated,
            this, &StatisticsWidget::onCSVExportProgress);
    connect(m_csvExporter, &CSVExporter::exportCompleted,
            this, &StatisticsWidget::onCSVExportCompleted);
    
    // 初始化Markdown导出器
    m_markdownExporter = new MarkdownExporter(this);
    
    // 连接Markdown导出器信号
    connect(m_markdownExporter, &MarkdownExporter::progressUpdated,
            this, &StatisticsWidget::onMarkdownExportProgress);
    connect(m_markdownExporter, &MarkdownExporter::exportCompleted,
            this, &StatisticsWidget::onMarkdownExportCompleted);
    
    initializeUI();
}

StatisticsWidget::~StatisticsWidget()
{
    if (m_csvExporter) {
        delete m_csvExporter;
        m_csvExporter = nullptr;
    }
    
    if (m_markdownExporter) {
        delete m_markdownExporter;
        m_markdownExporter = nullptr;
    }
}

void StatisticsWidget::initializeUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(5);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    
    // 创建工具栏
    m_mainLayout->addWidget(createToolBar());
    
    // 创建标签页组件
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createOverviewTab(), tr("概览"));
    m_tabWidget->addTab(createDetailedTableTab(), tr("详细表格"));
    m_tabWidget->addTab(createFileListTab(), tr("文件列表"));
    m_tabWidget->addTab(createLanguageStatsTab(), tr("语言统计"));
    
    m_mainLayout->addWidget(m_tabWidget, 1);
    
    // 连接信号
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &StatisticsWidget::onTabChanged);
    
    setLayout(m_mainLayout);
}

QWidget* StatisticsWidget::createToolBar()
{
    m_toolBar = new QWidget(this);
    QHBoxLayout *toolLayout = new QHBoxLayout(m_toolBar);
    toolLayout->setContentsMargins(5, 5, 5, 5);
    
    // 导出按钮
    m_exportCSVButton = new QPushButton(tr("导出CSV"), this);
    m_exportHTMLButton = new QPushButton(tr("导出HTML"), this);
    m_exportMarkdownButton = new QPushButton(tr("导出Markdown"), this);
    m_refreshButton = new QPushButton(tr("刷新"), this);
    
    // 进度显示
    m_progressLabel = new QLabel(tr("就绪"), this);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(200);
    
    toolLayout->addWidget(m_exportCSVButton);
    toolLayout->addWidget(m_exportHTMLButton);
    toolLayout->addWidget(m_exportMarkdownButton);
    toolLayout->addWidget(m_refreshButton);
    toolLayout->addStretch();
    toolLayout->addWidget(m_progressLabel);
    toolLayout->addWidget(m_progressBar);
    
    // 连接信号
    connect(m_exportCSVButton, &QPushButton::clicked, this, &StatisticsWidget::onExportCSVClicked);
    connect(m_exportHTMLButton, &QPushButton::clicked, this, &StatisticsWidget::onExportHTMLClicked);
    connect(m_exportMarkdownButton, &QPushButton::clicked, this, &StatisticsWidget::onExportMarkdownClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &StatisticsWidget::onRefreshClicked);
    
    return m_toolBar;
}

QWidget* StatisticsWidget::createOverviewTab()
{
    m_overviewTab = new QWidget(this);
    m_overviewLayout = new QGridLayout(m_overviewTab);
    m_overviewLayout->setSpacing(10);
    m_overviewLayout->setContentsMargins(10, 10, 10, 10);
    
    // 项目信息
    QGroupBox *projectGroup = new QGroupBox(tr("项目信息"), this);
    QVBoxLayout *projectLayout = new QVBoxLayout(projectGroup);
    
    m_projectPathLabel = new QLabel(tr("未选择项目"), this);
    m_projectPathLabel->setWordWrap(true);
    m_analysisTimeLabel = new QLabel(tr("分析时间: 无"), this);
    
    projectLayout->addWidget(m_projectPathLabel);
    projectLayout->addWidget(m_analysisTimeLabel);
    
    // 统计信息卡片
    QWidget *statsWidget = new QWidget(this);
    QGridLayout *statsLayout = new QGridLayout(statsWidget);
    
    // 创建信息卡片（稍后在updateOverviewDisplay中填充）
    
    m_overviewLayout->addWidget(projectGroup, 0, 0, 1, 2);
    m_overviewLayout->addWidget(statsWidget, 1, 0, 1, 2);
    m_overviewLayout->setRowStretch(2, 1);
    
    return m_overviewTab;
}

QWidget* StatisticsWidget::createDetailedTableTab()
{
    m_detailedTableTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(m_detailedTableTab);
    layout->setContentsMargins(5, 5, 5, 5);
    
    m_detailedTable = new QTableWidget(this);
    m_detailedTable->setAlternatingRowColors(true);
    m_detailedTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_detailedTable->setSortingEnabled(true);
    m_detailedTable->horizontalHeader()->setStretchLastSection(true);
    
    // 设置列标题
    QStringList headers;
    headers << tr("文件名") << tr("语言") << tr("总行数")
            << tr("代码行数") << tr("注释行数") << tr("空白行数")
            << tr("文件大小") << tr("代码密度") << tr("注释密度");
    
    m_detailedTable->setColumnCount(headers.size());
    m_detailedTable->setHorizontalHeaderLabels(headers);
    
    layout->addWidget(m_detailedTable);
    
    // 连接信号
    connect(m_detailedTable, &QTableWidget::itemSelectionChanged, 
            this, &StatisticsWidget::onFileTableItemSelectionChanged);
    
    return m_detailedTableTab;
}

QWidget* StatisticsWidget::createFileListTab()
{
    m_fileListTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(m_fileListTab);
    layout->setContentsMargins(5, 5, 5, 5);
    
    m_fileTree = new QTreeWidget(this);
    m_fileTree->setAlternatingRowColors(true);
    m_fileTree->setRootIsDecorated(true);
    
    // 设置列标题
    QStringList headers;
    headers << tr("文件/目录") << tr("行数") << tr("语言") << tr("大小");
    
    m_fileTree->setHeaderLabels(headers);
    m_fileTree->header()->setStretchLastSection(false);
    m_fileTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    
    layout->addWidget(m_fileTree);
    
    // 连接信号
    connect(m_fileTree, &QTreeWidget::itemSelectionChanged, 
            this, &StatisticsWidget::onFileTreeItemSelectionChanged);
    
    return m_fileListTab;
}

QWidget* StatisticsWidget::createLanguageStatsTab()
{
    m_languageStatsTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(m_languageStatsTab);
    layout->setContentsMargins(5, 5, 5, 5);
    
    m_languageTable = new QTableWidget(this);
    m_languageTable->setAlternatingRowColors(true);
    m_languageTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_languageTable->setSortingEnabled(true);
    m_languageTable->horizontalHeader()->setStretchLastSection(true);
    
    // 设置列标题
    QStringList headers;
    headers << tr("语言") << tr("文件数") << tr("总行数")
            << tr("代码行数") << tr("百分比");
    
    m_languageTable->setColumnCount(headers.size());
    m_languageTable->setHorizontalHeaderLabels(headers);
    
    layout->addWidget(m_languageTable);
    
    return m_languageStatsTab;
}

void StatisticsWidget::setAnalysisResult(const AnalysisResult &result)
{
    m_analysisResult = result;
    refreshDisplay();
}

void StatisticsWidget::clearContent()
{
    m_analysisResult.clear();
    
    // 清空所有显示
    if (m_projectPathLabel) {
        m_projectPathLabel->setText(tr("未选择项目"));
    }
    if (m_analysisTimeLabel) {
        m_analysisTimeLabel->setText(tr("分析时间: 无"));
    }
    if (m_detailedTable) {
        m_detailedTable->setRowCount(0);
    }
    if (m_fileTree) {
        m_fileTree->clear();
    }
    if (m_languageTable) {
        m_languageTable->setRowCount(0);
    }
    
    m_progressLabel->setText(tr("就绪"));
    m_progressBar->setVisible(false);
}

StatisticsWidget::DisplayMode StatisticsWidget::getCurrentDisplayMode() const
{
    return m_currentDisplayMode;
}

void StatisticsWidget::setDisplayMode(DisplayMode mode)
{
    m_currentDisplayMode = mode;
    m_tabWidget->setCurrentIndex(static_cast<int>(mode));
}

const AnalysisResult& StatisticsWidget::getAnalysisResult() const
{
    return m_analysisResult;
}

void StatisticsWidget::refreshDisplay()
{
    updateOverviewDisplay();
    updateDetailedTableDisplay();
    updateFileListDisplay();
    updateLanguageStatsDisplay();
}

void StatisticsWidget::updateProgress(int current, int total, const QString &message)
{
    if (total > 0) {
        m_progressBar->setVisible(true);
        m_progressBar->setMaximum(total);
        m_progressBar->setValue(current);
        m_progressLabel->setText(message);
    } else {
        m_progressBar->setVisible(false);
        m_progressLabel->setText(message);
    }
}

void StatisticsWidget::updateOverviewDisplay()
{
    if (m_analysisResult.isEmpty()) {
        return;
    }
    
    // 更新项目信息
    QString projectPath = m_analysisResult.getProjectPath();
    m_projectPathLabel->setText(tr("项目: %1").arg(projectPath));
    
    // 更新分析时间
    qint64 duration = m_analysisResult.getAnalysisDuration();
    QString timeText = tr("分析完成，用时 %1").arg(formatDuration(duration));
    if (m_analysisResult.getAnalysisEndTime().isValid()) {
        timeText += tr("，时间 %1").arg(m_analysisResult.getAnalysisEndTime().toString("yyyy-MM-dd hh:mm:ss"));
    }
    m_analysisTimeLabel->setText(timeText);
    
    // 清除旧的统计卡片
    QWidget *statsWidget = m_overviewLayout->itemAtPosition(1, 0)->widget();
    QGridLayout *statsLayout = qobject_cast<QGridLayout*>(statsWidget->layout());
    if (!statsLayout) {
        statsLayout = new QGridLayout(statsWidget);
    }
    
    // 清除旧的组件
    QLayoutItem *item;
    while ((item = statsLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    // 创建新的统计卡片
    int row = 0, col = 0;
    const int maxCols = 3;
    
    // 总文件数
    statsLayout->addWidget(createInfoCard(
        tr("总文件数"), 
        formatNumber(m_analysisResult.getTotalFiles()),
        tr("个文件已分析")
    ), row, col++);
    
    // 总行数
    if (col >= maxCols) { row++; col = 0; }
    statsLayout->addWidget(createInfoCard(
        tr("总行数"), 
        formatNumber(m_analysisResult.getTotalLines()),
        tr("行代码")
    ), row, col++);
    
    // 代码行数
    if (col >= maxCols) { row++; col = 0; }
    statsLayout->addWidget(createInfoCard(
        tr("代码行数"), 
        formatNumber(m_analysisResult.getTotalCodeLines()),
        formatPercentage(m_analysisResult.getTotalLines() > 0 ? 
            (double)m_analysisResult.getTotalCodeLines() / m_analysisResult.getTotalLines() * 100 : 0)
    ), row, col++);
    
    // 注释行数
    if (col >= maxCols) { row++; col = 0; }
    statsLayout->addWidget(createInfoCard(
        tr("注释行数"), 
        formatNumber(m_analysisResult.getTotalCommentLines()),
        formatPercentage(m_analysisResult.getTotalLines() > 0 ? 
            (double)m_analysisResult.getTotalCommentLines() / m_analysisResult.getTotalLines() * 100 : 0)
    ), row, col++);
    
    // 空白行数
    if (col >= maxCols) { row++; col = 0; }
    statsLayout->addWidget(createInfoCard(
        tr("空白行数"), 
        formatNumber(m_analysisResult.getTotalBlankLines()),
        formatPercentage(m_analysisResult.getTotalLines() > 0 ? 
            (double)m_analysisResult.getTotalBlankLines() / m_analysisResult.getTotalLines() * 100 : 0)
    ), row, col++);
    
    // 语言数量
    if (col >= maxCols) { row++; col = 0; }
    statsLayout->addWidget(createInfoCard(
        tr("编程语言"), 
        formatNumber(m_analysisResult.getLanguageCount()),
        tr("种编程语言")
    ), row, col++);
    
    // 设置拉伸
    statsLayout->setRowStretch(row + 1, 1);
}

void StatisticsWidget::updateDetailedTableDisplay()
{
    m_detailedTable->setRowCount(0);
    
    if (m_analysisResult.isEmpty()) {
        return;
    }
    
    const QVector<FileStatistics> &fileStats = m_analysisResult.getFileStatistics();
    m_detailedTable->setRowCount(fileStats.size());
    
    for (int i = 0; i < fileStats.size(); ++i) {
        const FileStatistics &stats = fileStats[i];
        
        // 文件名
        QTableWidgetItem *nameItem = new QTableWidgetItem(stats.fileName);
        nameItem->setData(Qt::UserRole, stats.filePath);
        m_detailedTable->setItem(i, 0, nameItem);
        
        // 语言
        m_detailedTable->setItem(i, 1, new QTableWidgetItem(stats.language));
        
        // 总行数
        QTableWidgetItem *totalItem = new QTableWidgetItem();
        totalItem->setData(Qt::DisplayRole, stats.totalLines);
        m_detailedTable->setItem(i, 2, totalItem);
        
        // 代码行数
        QTableWidgetItem *codeItem = new QTableWidgetItem();
        codeItem->setData(Qt::DisplayRole, stats.codeLines);
        m_detailedTable->setItem(i, 3, codeItem);
        
        // 注释行数
        QTableWidgetItem *commentItem = new QTableWidgetItem();
        commentItem->setData(Qt::DisplayRole, stats.commentLines);
        m_detailedTable->setItem(i, 4, commentItem);
        
        // 空白行数
        QTableWidgetItem *blankItem = new QTableWidgetItem();
        blankItem->setData(Qt::DisplayRole, stats.blankLines);
        m_detailedTable->setItem(i, 5, blankItem);
        
        // 文件大小
        m_detailedTable->setItem(i, 6, new QTableWidgetItem(formatFileSize(stats.fileSize)));
        
        // 代码密度
        m_detailedTable->setItem(i, 7, new QTableWidgetItem(formatPercentage(stats.getCodeDensity())));
        
        // 注释密度
        m_detailedTable->setItem(i, 8, new QTableWidgetItem(formatPercentage(stats.getCommentDensity())));
    }
    
    // 调整列宽
    m_detailedTable->resizeColumnsToContents();
}

void StatisticsWidget::updateFileListDisplay()
{
    m_fileTree->clear();
    
    if (m_analysisResult.isEmpty()) {
        return;
    }
    
    // 构建文件树结构
    QMap<QString, QTreeWidgetItem*> dirItems;
    QString projectPath = m_analysisResult.getProjectPath();
    
    const QVector<FileStatistics> &fileStats = m_analysisResult.getFileStatistics();
    
    for (const FileStatistics &stats : fileStats) {
        QString relativePath = QDir(projectPath).relativeFilePath(stats.filePath);
        QStringList pathParts = relativePath.split('/', QString::SkipEmptyParts);
        
        QTreeWidgetItem *parentItem = nullptr;
        QString currentPath;
        
        // 创建目录结构
        for (int i = 0; i < pathParts.size() - 1; ++i) {
            currentPath += pathParts[i] + "/";
            
            if (!dirItems.contains(currentPath)) {
                QTreeWidgetItem *dirItem = new QTreeWidgetItem();
                dirItem->setText(0, pathParts[i]);
                dirItem->setData(0, Qt::UserRole, "directory");
                
                if (parentItem) {
                    parentItem->addChild(dirItem);
                } else {
                    m_fileTree->addTopLevelItem(dirItem);
                }
                
                dirItems[currentPath] = dirItem;
            }
            
            parentItem = dirItems[currentPath];
        }
        
        // 创建文件项
        QTreeWidgetItem *fileItem = new QTreeWidgetItem();
        fileItem->setText(0, stats.fileName);
        fileItem->setText(1, formatNumber(stats.totalLines));
        fileItem->setText(2, stats.language);
        fileItem->setText(3, formatFileSize(stats.fileSize));
        fileItem->setData(0, Qt::UserRole, stats.filePath);
        
        if (parentItem) {
            parentItem->addChild(fileItem);
        } else {
            m_fileTree->addTopLevelItem(fileItem);
        }
    }
    
    // 展开所有项
    m_fileTree->expandAll();
    
    // 调整列宽
    for (int i = 0; i < m_fileTree->columnCount(); ++i) {
        m_fileTree->resizeColumnToContents(i);
    }
}

void StatisticsWidget::updateLanguageStatsDisplay()
{
    m_languageTable->setRowCount(0);
    
    if (m_analysisResult.isEmpty()) {
        return;
    }
    
    QMap<QString, int> languageStats = m_analysisResult.getLanguageStatistics();
    QMap<QString, int> languageFiles;
    QMap<QString, int> languageTotal;
    
    // 统计每种语言的文件数和总行数
    const QVector<FileStatistics> &fileStats = m_analysisResult.getFileStatistics();
    for (const FileStatistics &stats : fileStats) {
        languageFiles[stats.language]++;
        languageTotal[stats.language] += stats.totalLines;
    }
    
    m_languageTable->setRowCount(languageStats.size());
    
    int row = 0;
    int totalCodeLines = m_analysisResult.getTotalCodeLines();
    
    for (auto it = languageStats.begin(); it != languageStats.end(); ++it, ++row) {
        QString language = it.key();
        int codeLines = it.value();
        
        // 语言名称
        m_languageTable->setItem(row, 0, new QTableWidgetItem(language));
        
        // 文件数
        QTableWidgetItem *filesItem = new QTableWidgetItem();
        filesItem->setData(Qt::DisplayRole, languageFiles[language]);
        m_languageTable->setItem(row, 1, filesItem);
        
        // 总行数
        QTableWidgetItem *totalItem = new QTableWidgetItem();
        totalItem->setData(Qt::DisplayRole, languageTotal[language]);
        m_languageTable->setItem(row, 2, totalItem);
        
        // 代码行数
        QTableWidgetItem *codeItem = new QTableWidgetItem();
        codeItem->setData(Qt::DisplayRole, codeLines);
        m_languageTable->setItem(row, 3, codeItem);
        
        // 百分比
        double percentage = totalCodeLines > 0 ? (double)codeLines / totalCodeLines * 100 : 0;
        m_languageTable->setItem(row, 4, new QTableWidgetItem(formatPercentage(percentage)));
    }
    
    // 调整列宽
    m_languageTable->resizeColumnsToContents();
}

QWidget* StatisticsWidget::createInfoCard(const QString &title, const QString &value, const QString &description)
{
    QGroupBox *card = new QGroupBox(this);
    card->setFixedHeight(80);
    card->setStyleSheet(
        "QGroupBox {"
        "    border: 1px solid #ddd;"
        "    border-radius: 6px;"
        "    margin: 5px;"
        "    padding: 5px;"
        "    background-color: #f9f9f9;"
        "}"
    );
    
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(2);
    
    // 标题
    QLabel *titleLabel = new QLabel(title, this);
    titleLabel->setStyleSheet("font-weight: bold; color: #333; font-size: 12px;");
    
    // 值
    QLabel *valueLabel = new QLabel(value, this);
    valueLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2196F3;");
    
    // 描述
    QLabel *descLabel = new QLabel(description, this);
    descLabel->setStyleSheet("font-size: 10px; color: #666;");
    
    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    if (!description.isEmpty()) {
        layout->addWidget(descLabel);
    }
    
    return card;
}

QString StatisticsWidget::formatFileSize(qint64 bytes) const
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    
    if (bytes >= GB) {
        return QString("%1 GB").arg(QString::number(bytes / (double)GB, 'f', 2));
    } else if (bytes >= MB) {
        return QString("%1 MB").arg(QString::number(bytes / (double)MB, 'f', 2));
    } else if (bytes >= KB) {
        return QString("%1 KB").arg(QString::number(bytes / (double)KB, 'f', 1));
    } else {
        return QString("%1 B").arg(bytes);
    }
}

QString StatisticsWidget::formatNumber(int number) const
{
    return QLocale().toString(number);
}

QString StatisticsWidget::formatPercentage(double value) const
{
    return QString("%1%").arg(QString::number(value, 'f', 1));
}

QString StatisticsWidget::formatDuration(qint64 milliseconds) const
{
    if (milliseconds < 1000) {
        return tr("%1 毫秒").arg(milliseconds);
    } else if (milliseconds < 60000) {
        return tr("%1 秒").arg(QString::number(milliseconds / 1000.0, 'f', 1));
    } else {
        int minutes = milliseconds / 60000;
        int seconds = (milliseconds % 60000) / 1000;
        return tr("%1分 %2秒").arg(minutes).arg(seconds);
    }
}

void StatisticsWidget::onTabChanged(int index)
{
    m_currentDisplayMode = static_cast<DisplayMode>(index);
}

void StatisticsWidget::onFileTableItemSelectionChanged()
{
    QList<QTableWidgetItem*> selectedItems = m_detailedTable->selectedItems();
    if (!selectedItems.isEmpty()) {
        QTableWidgetItem *nameItem = m_detailedTable->item(selectedItems.first()->row(), 0);
        if (nameItem) {
            QString filePath = nameItem->data(Qt::UserRole).toString();
            emit fileSelected(filePath);
        }
    }
}

void StatisticsWidget::onFileTreeItemSelectionChanged()
{
    QList<QTreeWidgetItem*> selectedItems = m_fileTree->selectedItems();
    if (!selectedItems.isEmpty()) {
        QTreeWidgetItem *item = selectedItems.first();
        QString filePath = item->data(0, Qt::UserRole).toString();
        if (!filePath.isEmpty() && filePath != "directory") {
            emit fileSelected(filePath);
        }
    }
}

void StatisticsWidget::onExportCSVClicked()
{
    // 使用新的CSV导出对话框
    exportToCSV();
}

bool StatisticsWidget::exportToCSV()
{
    // 创建CSV导出对话框
    CSVExportDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // 获取用户选择
        CSVExportDialog::ExportContents contents = dialog.getSelectedContents();
        CSVExportDialog::ExportMode mode = dialog.getExportMode();
        QString exportPath = dialog.getExportPath();
        QString filePrefix = dialog.getFilePrefix();
        QString encoding = dialog.getSelectedEncoding();
        
        // 设置分析结果和编码到导出器
        m_csvExporter->setAnalysisResult(m_analysisResult);
        m_csvExporter->setEncoding(encoding);
        
        // 转换内容标志
        CSVExporter::ExportContents exporterContents = CSVExporter::ExportContents();
        if (contents & CSVExportDialog::Overview) {
            exporterContents |= CSVExporter::Overview;
        }
        if (contents & CSVExportDialog::DetailedTable) {
            exporterContents |= CSVExporter::DetailedTable;
        }
        if (contents & CSVExportDialog::FileList) {
            exporterContents |= CSVExporter::FileList;
        }
        if (contents & CSVExportDialog::LanguageStats) {
            exporterContents |= CSVExporter::LanguageStats;
        }
        
        // 执行导出
        bool success = false;
        if (mode == CSVExportDialog::SingleFile) {
            success = m_csvExporter->exportToSingleFile(exportPath, exporterContents);
        } else {
            success = m_csvExporter->exportToMultipleFiles(exportPath, filePrefix, exporterContents);
        }
        
        return success;
    }
    
    return false;
}

void StatisticsWidget::onCSVExportProgress(int current, int total, const QString &message)
{
    // 更新进度显示
    updateProgress(current, total, message);
}

void StatisticsWidget::onCSVExportCompleted(bool success, const QString &message)
{
    // 隐藏进度条
    m_progressBar->setVisible(false);
    m_progressLabel->setVisible(false);
    
    // 显示结果消息
    if (success) {
        QMessageBox::information(this, tr("导出成功"), message);
    } else {
        QMessageBox::warning(this, tr("导出失败"), message);
    }
}

void StatisticsWidget::onExportHTMLClicked()
{
    showHTMLPreviewDialog();
}

void StatisticsWidget::showHTMLPreviewDialog()
{
    // 请求图表数据
    emit chartDataRequested();
    
    // 创建HTML预览对话框
    HtmlPreviewDialog dialog(m_analysisResult, m_chartData, this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // 获取导出设置
        HtmlPreviewDialog::ExportSettings settings = dialog.getExportSettings();
        
        // 选择保存文件
        QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("导出为HTML"),
            QString("%1_%2.html")
                .arg(settings.reportTitle.isEmpty() ? "code_statistics" : settings.reportTitle)
                .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
            tr("HTML文件 (*.html)")
        );
        
        if (!fileName.isEmpty()) {
            // 使用预览对话框生成的HTML内容进行导出
            QString htmlContent = dialog.generateHTMLContent();
            
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream.setCodec("UTF-8");
                stream << htmlContent;
                file.close();
                
                QMessageBox::information(this, tr("导出成功"),
                                    tr("报告已导出到 %1").arg(fileName));
            } else {
                QMessageBox::warning(this, tr("导出失败"),
                                tr("导出报告到 %1 失败").arg(fileName));
            }
        }
    }
}

void StatisticsWidget::onRefreshClicked()
{
    refreshDisplay();
}

bool StatisticsWidget::exportToCSV(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << generateCSVContent();
    
    return true;
}

bool StatisticsWidget::exportToHTML(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << generateHTMLReport();
    
    return true;
}

QString StatisticsWidget::generateCSVContent() const
{
    QString csv;
    
    // CSV 头部
    csv += "文件名,编程语言,总行数,代码行数,注释行数,空白行数,文件大小,代码密度,注释密度\n";
    
    // 数据行
    const QVector<FileStatistics> &fileStats = m_analysisResult.getFileStatistics();
    for (const FileStatistics &stats : fileStats) {
        csv += QString("%1,%2,%3,%4,%5,%6,%7,%8,%9\n")
               .arg(stats.fileName)
               .arg(stats.language)
               .arg(stats.totalLines)
               .arg(stats.codeLines)
               .arg(stats.commentLines)
               .arg(stats.blankLines)
               .arg(stats.fileSize)
               .arg(stats.getCodeDensity(), 0, 'f', 2)
               .arg(stats.getCommentDensity(), 0, 'f', 2);
    }
    
    return csv;
}

QString StatisticsWidget::generateHTMLReport() const
{
    QString html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>代码统计分析报告</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background-color: #f5f5f5; padding: 20px; border-radius: 5px; }
        .stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 20px 0; }
        .stat-card { background-color: #fff; border: 1px solid #ddd; border-radius: 5px; padding: 15px; text-align: center; }
        .stat-value { font-size: 24px; font-weight: bold; color: #2196F3; }
        .stat-label { font-size: 14px; color: #666; }
        .charts-section { margin: 30px 0; }
        .chart-container { text-align: center; margin: 30px 0; page-break-inside: avoid; }
        .chart-image { max-width: 100%; height: auto; border: 2px solid #ddd; border-radius: 8px; }
        .chart-title { font-size: 18px; font-weight: bold; margin: 15px 0; color: #333; }
        table { width: 100%; border-collapse: collapse; margin: 20px 0; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f5f5f5; }
        tr:nth-child(even) { background-color: #f9f9f9; }
    </style>
</head>
<body>
)";
    
    // 添加报告内容
    html += QString(R"(
    <div class="header">
        <h1>代码统计分析报告</h1>
        <p><strong>项目路径:</strong> %1</p>
        <p><strong>生成时间:</strong> %2</p>
    </div>
)")
    .arg(m_analysisResult.getProjectPath())
    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    
    // 添加统计概览
    html += R"(
    <div class="stats-grid">
)";
    
    html += QString(R"(
        <div class="stat-card">
            <div class="stat-value">%1</div>
            <div class="stat-label">总文件数</div>
        </div>
)")
    .arg(formatNumber(m_analysisResult.getTotalFiles()));
    
    html += QString(R"(
        <div class="stat-card">
            <div class="stat-value">%1</div>
            <div class="stat-label">总行数</div>
        </div>
)")
    .arg(formatNumber(m_analysisResult.getTotalLines()));
    
    html += QString(R"(
        <div class="stat-card">
            <div class="stat-value">%1</div>
            <div class="stat-label">代码行数</div>
        </div>
)")
    .arg(formatNumber(m_analysisResult.getTotalCodeLines()));
    
    html += QString(R"(
        <div class="stat-card">
            <div class="stat-value">%1</div>
            <div class="stat-label">编程语言</div>
        </div>
)")
    .arg(formatNumber(m_analysisResult.getLanguageCount()));
    
    html += R"(
    </div>
)";
    
    // 添加图表部分
    if (!m_chartData.isEmpty()) {
        html += R"(
    <div class="charts-section">
        <h2>数据图表</h2>
)";
        
        for (int i = 0; i < m_chartData.size(); ++i) {
            QChart* chart = m_chartData[i];
            if (chart) {
                QString base64Image = chartToBase64(chart);
                if (!base64Image.isEmpty()) {
                    QString chartTitle = chart->title();
                    if (chartTitle.isEmpty()) {
                        chartTitle = QString("图表 %1").arg(i + 1);
                    }
                    
                    html += QString(R"(
        <div class="chart-container">
            <div class="chart-title">%1</div>
            <img class="chart-image" src="data:image/png;base64,%2" alt="%3" />
        </div>
)")
                    .arg(chartTitle)
                    .arg(base64Image)
                    .arg(chartTitle);
                }
            }
        }
        
        html += R"(
    </div>
)";
    }
    
    // 添加详细表格
    html += R"(
    <h2>文件详细信息</h2>
    <table>
        <tr>
            <th>文件名</th>
            <th>编程语言</th>
            <th>总行数</th>
            <th>代码行数</th>
            <th>注释行数</th>
            <th>空白行数</th>
            <th>文件大小</th>
        </tr>
)";
    
    const QVector<FileStatistics> &fileStats = m_analysisResult.getFileStatistics();
    for (const FileStatistics &stats : fileStats) {
        html += QString(R"(
        <tr>
            <td>%1</td>
            <td>%2</td>
            <td>%3</td>
            <td>%4</td>
            <td>%5</td>
            <td>%6</td>
            <td>%7</td>
        </tr>
)")
        .arg(stats.fileName)
        .arg(stats.language)
        .arg(stats.totalLines)
        .arg(stats.codeLines)
        .arg(stats.commentLines)
        .arg(stats.blankLines)
        .arg(formatFileSize(stats.fileSize));
    }
    
    html += R"(
    </table>
</body>
</html>
)";
    
    return html;
}

QString StatisticsWidget::chartToBase64(QChart* chart) const
{
    if (!chart) {
        return QString();
    }
    
    try {
        // 设置图表大小
        QSize size(800, 600);
        
        // 创建像素图
        QPixmap pixmap(size);
        pixmap.fill(Qt::white);
        
        // 创建画家并渲染图表
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 设置图表场景矩形
        chart->resize(size);
        chart->scene()->setSceneRect(QRectF(QPointF(0, 0), size));
        
        // 渲染图表场景
        chart->scene()->render(&painter, QRectF(0, 0, size.width(), size.height()));
        painter.end();
        
        // 转换为Base64
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        if (!pixmap.save(&buffer, "PNG")) {
            return QString();
        }
        
        QByteArray base64Data = buffer.data().toBase64();
        return QString::fromLatin1(base64Data);
        
    } catch (...) {
        return QString();
    }
}

void StatisticsWidget::receiveChartData(const QList<QChart*> &charts)
{
    m_chartData = charts;
}

void StatisticsWidget::setChartGenerationOptions(const ChartGenerationOptions &options)
{
    m_chartOptions = options;
}

ChartGenerationOptions StatisticsWidget::getChartGenerationOptions() const
{
    return m_chartOptions;
}

void StatisticsWidget::onExportMarkdownClicked()
{
    // 使用新的Markdown导出对话框
    exportToMarkdown();
}

bool StatisticsWidget::exportToMarkdown()
{
    // 创建Markdown导出对话框
    MarkdownExportDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // 获取用户选择
        MarkdownExportDialog::ExportContents contents = dialog.getSelectedContents();
        MarkdownExportDialog::ExportMode mode = dialog.getExportMode();
        QString exportPath = dialog.getExportPath();
        QString filePrefix = dialog.getFilePrefix();
        
        // 设置分析结果到导出器
        m_markdownExporter->setAnalysisResult(m_analysisResult);
        
        // 转换导出内容类型
        MarkdownExporter::ExportContents exporterContents = static_cast<MarkdownExporter::ExportContents>(0);
        if (contents & MarkdownExportDialog::Overview)
            exporterContents |= MarkdownExporter::Overview;
        if (contents & MarkdownExportDialog::DetailedTable)
            exporterContents |= MarkdownExporter::DetailedTable;
        if (contents & MarkdownExportDialog::FileList)
            exporterContents |= MarkdownExporter::FileList;
        if (contents & MarkdownExportDialog::LanguageStats)
            exporterContents |= MarkdownExporter::LanguageStats;
        
        // 执行导出
        bool success = false;
        if (mode == MarkdownExportDialog::SingleFile) {
            success = m_markdownExporter->exportToSingleFile(exportPath, exporterContents);
        } else {
            success = m_markdownExporter->exportToMultipleFiles(exportPath, filePrefix, exporterContents);
        }
        
        return success;
    }
    
    return false;
}

bool StatisticsWidget::exportToMarkdown(const QString &filePath) const
{
    if (filePath.isEmpty()) {
        return false;
    }
    
    // 设置分析结果到导出器
    m_markdownExporter->setAnalysisResult(m_analysisResult);
    
    // 导出所有内容到单个文件
    MarkdownExporter::ExportContents allContents = 
        MarkdownExporter::Overview | MarkdownExporter::DetailedTable | 
        MarkdownExporter::FileList | MarkdownExporter::LanguageStats;
    
    return m_markdownExporter->exportToSingleFile(filePath, allContents);
}

void StatisticsWidget::onMarkdownExportProgress(int current, int total, const QString &message)
{
    updateProgress(current, total, message);
}

void StatisticsWidget::onMarkdownExportCompleted(bool success, const QString &message)
{
    m_progressBar->setVisible(false);
    m_progressLabel->setText(tr("就绪"));
    
    if (success) {
        QMessageBox::information(this, tr("导出成功"), message);
    } else {
        QMessageBox::warning(this, tr("导出失败"), message);
    }
}
#include "html_preview_dialog.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtGui/QPainter>
#include <QtCharts/QChartView>
#include <QtCore/QBuffer>
#include <QtCore/QDateTime>

HtmlPreviewDialog::HtmlPreviewDialog(const AnalysisResult &analysisResult, 
                                   const QList<QChart*> &chartData,
                                   QWidget *parent)
    : QDialog(parent)
    , m_analysisResult(analysisResult)
    , m_chartData(chartData)
{
    setWindowTitle(tr("HTML导出预览"));
    setModal(true);
    resize(1200, 800);
    
    initializeUI();
    updatePreview();
}

HtmlPreviewDialog::~HtmlPreviewDialog()
{
}

void HtmlPreviewDialog::initializeUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_splitter = new QSplitter(Qt::Horizontal, this);
    
    // 创建设置面板和预览面板
    m_settingsPanel = createSettingsPanel();
    m_previewPanel = createPreviewPanel();
    
    m_splitter->addWidget(m_settingsPanel);
    m_splitter->addWidget(m_previewPanel);
    m_splitter->setSizes({300, 900});
    
    m_mainLayout->addWidget(m_splitter);
    
    // 创建按钮布局
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch();
    
    m_exportButton = new QPushButton(tr("导出HTML"), this);
    m_cancelButton = new QPushButton(tr("取消"), this);
    
    m_exportButton->setDefault(true);
    m_exportButton->setMinimumSize(100, 30);
    m_cancelButton->setMinimumSize(100, 30);
    
    m_buttonLayout->addWidget(m_exportButton);
    m_buttonLayout->addWidget(m_cancelButton);
    
    m_mainLayout->addLayout(m_buttonLayout);
    
    // 连接信号
    connect(m_exportButton, &QPushButton::clicked, this, &HtmlPreviewDialog::onExportClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &HtmlPreviewDialog::onCancelClicked);
}

QWidget* HtmlPreviewDialog::createSettingsPanel()
{
    QWidget *panel = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 报告标题设置
    m_titleGroup = new QGroupBox(tr("报告设置"), panel);
    QVBoxLayout *titleLayout = new QVBoxLayout(m_titleGroup);
    
    QLabel *titleLabel = new QLabel(tr("报告标题:"), m_titleGroup);
    m_titleEdit = new QLineEdit(m_settings.reportTitle, m_titleGroup);
    
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(m_titleEdit);
    
    // 内容选择
    m_contentGroup = new QGroupBox(tr("导出内容"), panel);
    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentGroup);
    
    m_overviewCheck = new QCheckBox(tr("概览统计"), m_contentGroup);
    m_languageStatsCheck = new QCheckBox(tr("语言统计"), m_contentGroup);
    m_fileListCheck = new QCheckBox(tr("文件列表"), m_contentGroup);
    m_detailedTableCheck = new QCheckBox(tr("详细表格"), m_contentGroup);
    m_chartsCheck = new QCheckBox(tr("数据图表"), m_contentGroup);
    
    // 设置默认选中状态
    m_overviewCheck->setChecked(m_settings.includeOverview);
    m_languageStatsCheck->setChecked(m_settings.includeLanguageStats);
    m_fileListCheck->setChecked(m_settings.includeFileList);
    m_detailedTableCheck->setChecked(m_settings.includeDetailedTable);
    m_chartsCheck->setChecked(m_settings.includeCharts);
    
    contentLayout->addWidget(m_overviewCheck);
    contentLayout->addWidget(m_languageStatsCheck);
    contentLayout->addWidget(m_fileListCheck);
    contentLayout->addWidget(m_detailedTableCheck);
    contentLayout->addWidget(m_chartsCheck);
    
    // 图表选择
    m_chartGroup = new QGroupBox(tr("图表类型"), panel);
    QVBoxLayout *chartLayout = new QVBoxLayout(m_chartGroup);
    
    m_pieChartCheck = new QCheckBox(tr("饼图 (语言分布)"), m_chartGroup);
    m_barChartCheck = new QCheckBox(tr("柱状图 (代码行数)"), m_chartGroup);
    m_lineChartCheck = new QCheckBox(tr("折线图 (趋势分析)"), m_chartGroup);
    m_areaChartCheck = new QCheckBox(tr("面积图 (累积统计)"), m_chartGroup);
    
    // 设置默认选中状态
    m_pieChartCheck->setChecked(m_settings.includePieChart);
    m_barChartCheck->setChecked(m_settings.includeBarChart);
    m_lineChartCheck->setChecked(m_settings.includeLineChart);
    m_areaChartCheck->setChecked(m_settings.includeAreaChart);
    
    chartLayout->addWidget(m_pieChartCheck);
    chartLayout->addWidget(m_barChartCheck);
    chartLayout->addWidget(m_lineChartCheck);
    chartLayout->addWidget(m_areaChartCheck);
    
    // 根据图表复选框状态启用/禁用图表类型选择
    m_chartGroup->setEnabled(m_chartsCheck->isChecked());
    
    layout->addWidget(m_titleGroup);
    layout->addWidget(m_contentGroup);
    layout->addWidget(m_chartGroup);
    layout->addStretch();
    
    // 连接信号
    connect(m_titleEdit, &QLineEdit::textChanged, this, &HtmlPreviewDialog::onReportTitleChanged);
    connect(m_overviewCheck, &QCheckBox::toggled, this, &HtmlPreviewDialog::onContentSelectionChanged);
    connect(m_languageStatsCheck, &QCheckBox::toggled, this, &HtmlPreviewDialog::onContentSelectionChanged);
    connect(m_fileListCheck, &QCheckBox::toggled, this, &HtmlPreviewDialog::onContentSelectionChanged);
    connect(m_detailedTableCheck, &QCheckBox::toggled, this, &HtmlPreviewDialog::onContentSelectionChanged);
    connect(m_chartsCheck, &QCheckBox::toggled, this, &HtmlPreviewDialog::onContentSelectionChanged);
    
    connect(m_pieChartCheck, &QCheckBox::toggled, this, &HtmlPreviewDialog::onChartSelectionChanged);
    connect(m_barChartCheck, &QCheckBox::toggled, this, &HtmlPreviewDialog::onChartSelectionChanged);
    connect(m_lineChartCheck, &QCheckBox::toggled, this, &HtmlPreviewDialog::onChartSelectionChanged);
    connect(m_areaChartCheck, &QCheckBox::toggled, this, &HtmlPreviewDialog::onChartSelectionChanged);
    
    // 图表复选框状态改变时启用/禁用图表类型选择
    connect(m_chartsCheck, &QCheckBox::toggled, m_chartGroup, &QGroupBox::setEnabled);
    
    return panel;
}

QWidget* HtmlPreviewDialog::createPreviewPanel()
{
    QWidget *panel = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    QLabel *previewLabel = new QLabel(tr("预览效果:"), panel);
    previewLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    
    m_webView = new QTextBrowser(panel);
    m_webView->setOpenExternalLinks(true);
    
    layout->addWidget(previewLabel);
    layout->addWidget(m_webView);
    
    return panel;
}

void HtmlPreviewDialog::updatePreview()
{
    QString htmlContent = generateHTMLContent();
    m_webView->setHtml(htmlContent);
}

void HtmlPreviewDialog::onContentSelectionChanged()
{
    m_settings.includeOverview = m_overviewCheck->isChecked();
    m_settings.includeLanguageStats = m_languageStatsCheck->isChecked();
    m_settings.includeFileList = m_fileListCheck->isChecked();
    m_settings.includeDetailedTable = m_detailedTableCheck->isChecked();
    m_settings.includeCharts = m_chartsCheck->isChecked();
    
    updatePreview();
}

void HtmlPreviewDialog::onChartSelectionChanged()
{
    m_settings.includePieChart = m_pieChartCheck->isChecked();
    m_settings.includeBarChart = m_barChartCheck->isChecked();
    m_settings.includeLineChart = m_lineChartCheck->isChecked();
    m_settings.includeAreaChart = m_areaChartCheck->isChecked();
    
    updatePreview();
}

void HtmlPreviewDialog::onReportTitleChanged()
{
    m_settings.reportTitle = m_titleEdit->text();
    updatePreview();
}

void HtmlPreviewDialog::onExportClicked()
{
    accept();
}

void HtmlPreviewDialog::onCancelClicked()
{
    reject();
}

HtmlPreviewDialog::ExportSettings HtmlPreviewDialog::getExportSettings() const
{
    return m_settings;
}

QString HtmlPreviewDialog::generateHTMLContent() const
{
    QString html = R"(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>%1</title>
    <style>
        body {
            font-family: 'Microsoft YaHei', Arial, sans-serif;
            line-height: 1.6;
            margin: 0;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background-color: white;
            padding: 30px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #2c3e50;
            text-align: center;
            border-bottom: 3px solid #3498db;
            padding-bottom: 10px;
            margin-bottom: 30px;
        }
        h2 {
            color: #34495e;
            border-left: 4px solid #3498db;
            padding-left: 15px;
            margin-top: 30px;
        }
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin: 20px 0;
        }
        .stat-card {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 20px;
            border-radius: 8px;
            text-align: center;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        .stat-value {
            font-size: 2em;
            font-weight: bold;
            margin-bottom: 5px;
        }
        .stat-label {
            font-size: 0.9em;
            opacity: 0.9;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
            background-color: white;
        }
        th, td {
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #ddd;
        }
        th {
            background-color: #3498db;
            color: white;
            font-weight: bold;
        }
        tr:nth-child(even) {
            background-color: #f8f9fa;
        }
        tr:hover {
            background-color: #e8f4f8;
        }
        .charts-section {
            margin: 30px 0;
        }
        .chart-container {
            margin: 20px 0;
            text-align: center;
            background-color: #fafafa;
            padding: 20px;
            border-radius: 8px;
            border: 1px solid #e0e0e0;
        }
        .chart-title {
            font-size: 1.2em;
            font-weight: bold;
            color: #2c3e50;
            margin-bottom: 15px;
        }
        .chart-image {
            max-width: 100%;
            height: auto;
            border-radius: 4px;
        }
        .info-section {
            background-color: #ecf0f1;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
        }
        .file-tree {
            font-family: 'Courier New', monospace;
            background-color: #2c3e50;
            color: #ecf0f1;
            padding: 20px;
            border-radius: 5px;
            overflow-x: auto;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>%2</h1>
        <div class="info-section">
            <p><strong>项目路径:</strong> %3</p>
            <p><strong>生成时间:</strong> %4</p>
        </div>
)";
    
    html = html.arg(m_settings.reportTitle)
              .arg(m_settings.reportTitle)
              .arg(m_analysisResult.getProjectPath())
              .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    
    // 添加概览统计
    if (m_settings.includeOverview) {
        html += R"(
        <h2>概览统计</h2>
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
    }
    
    // 添加图表部分
    if (m_settings.includeCharts && !m_chartData.isEmpty()) {
        html += R"(
        <div class="charts-section">
            <h2>数据图表</h2>
)";
        
        for (int i = 0; i < m_chartData.size(); ++i) {
            QChart* chart = m_chartData[i];
            if (chart) {
                QString chartTitle = chart->title();
                bool shouldInclude = false;
                
                // 根据图表标题判断是否应该包含
                if (chartTitle.contains("饼图") || chartTitle.contains("分布")) {
                    shouldInclude = m_settings.includePieChart;
                } else if (chartTitle.contains("柱状图") || chartTitle.contains("条形图")) {
                    shouldInclude = m_settings.includeBarChart;
                } else if (chartTitle.contains("折线图") || chartTitle.contains("趋势")) {
                    shouldInclude = m_settings.includeLineChart;
                } else if (chartTitle.contains("面积图") || chartTitle.contains("累积")) {
                    shouldInclude = m_settings.includeAreaChart;
                } else {
                    // 默认包含未分类的图表
                    shouldInclude = true;
                }
                
                if (shouldInclude) {
                    QString base64Image = chartToBase64(chart);
                    if (!base64Image.isEmpty()) {
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
        }
        
        html += R"(
        </div>
)";
    }
    
    // 添加语言统计表格
    if (m_settings.includeLanguageStats) {
        html += R"(
        <h2>编程语言统计</h2>
        <table>
            <tr>
                <th>语言</th>
                <th>文件数</th>
                <th>总行数</th>
                <th>代码行数</th>
                <th>百分比</th>
            </tr>
)";
        
        const QMap<QString, int> &langStats = m_analysisResult.getLanguageStatistics();
        for (auto it = langStats.begin(); it != langStats.end(); ++it) {
            int lineCount = it.value();
            double percentage = (double)lineCount / m_analysisResult.getTotalLines() * 100;
            
            html += QString(R"(
            <tr>
                <td>%1</td>
                <td>%2</td>
                <td>%3</td>
                <td>%4</td>
                <td>%5%</td>
            </tr>
)")
            .arg(it.key())
            .arg("-")  // File count not available in this data structure
            .arg(formatNumber(lineCount))
            .arg("-")  // Code lines not available in this data structure
            .arg(QString::number(percentage, 'f', 1));
        }
        
        html += R"(
        </table>
)";
    }
    
    // 添加详细表格
    if (m_settings.includeDetailedTable) {
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
            .arg(QFileInfo(stats.filePath).fileName())
            .arg(stats.language)
            .arg(formatNumber(stats.totalLines))
            .arg(formatNumber(stats.codeLines))
            .arg(formatNumber(stats.commentLines))
            .arg(formatNumber(stats.blankLines))
            .arg(formatFileSize(stats.fileSize));
        }
        
        html += R"(
        </table>
)";
    }
    
    // 添加文件列表
    if (m_settings.includeFileList) {
        html += R"(
        <h2>项目文件结构</h2>
        <div class="file-tree">
)";
        
        // 这里可以添加文件树的生成逻辑
        html += QString("项目根目录: %1<br/>").arg(m_analysisResult.getProjectPath());
        html += QString("总计 %1 个文件").arg(m_analysisResult.getTotalFiles());
        
        html += R"(
        </div>
)";
    }
    
    html += R"(
    </div>
</body>
</html>
)";
    
    return html;
}

QString HtmlPreviewDialog::chartToBase64(QChart* chart) const
{
    if (!chart) {
        return QString();
    }
    
    // 创建图表视图
    QChartView chartView(chart);
    chartView.setRenderHint(QPainter::Antialiasing);
    chartView.resize(800, 600);
    
    // 渲染到图片
    QPixmap pixmap(800, 600);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    chartView.render(&painter);
    
    // 转换为Base64
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    
    return byteArray.toBase64();
}

QString HtmlPreviewDialog::formatNumber(int number) const
{
    return QLocale().toString(number);
}

QString HtmlPreviewDialog::formatFileSize(qint64 bytes) const
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
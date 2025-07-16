#include "chart_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>
#include <QApplication>
#include <QEventLoop>
#include <QStandardPaths>
#include <QtCharts/QChartView>
#include <QtSvg/QSvgGenerator>
#include <QtGui/QPainter>
#include <QtCore/QDir>

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_contentLayout(nullptr)
    , m_controlPanel(nullptr)
    , m_controlLayout(nullptr)
    , m_chartTypeGroup(nullptr)
    , m_chartTypeCombo(nullptr)
    , m_exportGroup(nullptr)
    , m_exportPNGButton(nullptr)
    , m_exportSVGButton(nullptr)
    , m_chartInfoLabel(nullptr)
    , m_chartContainer(nullptr)
    , m_chartLayout(nullptr)
    , m_chartView(nullptr)
    , m_currentChart(nullptr)
    , m_languagePieSeries(nullptr)
    , m_fileTypeBarSeries(nullptr)
    , m_codeLinesBarSeries(nullptr)
    , m_fileSizeScatterSeries(nullptr)
    , m_complexityLineSeries(nullptr)
    , m_languageAreaSeries(nullptr)
    , m_currentChartType(LanguagePieChart)
    , m_isRefreshing(false)
{
    initializeUI();
    
    // 初始化语言颜色映射
    m_languageColors["C++"] = QColor("#f34b7d");
    m_languageColors["C"] = QColor("#555555");
    m_languageColors["Java"] = QColor("#b07219");
    m_languageColors["Python"] = QColor("#3572A5");
    m_languageColors["JavaScript"] = QColor("#f1e05a");
    m_languageColors["C#"] = QColor("#239120");
    m_languageColors["PHP"] = QColor("#4F5D95");
    m_languageColors["TypeScript"] = QColor("#2b7489");
    m_languageColors["Go"] = QColor("#00ADD8");
    m_languageColors["Rust"] = QColor("#dea584");
    m_languageColors["Swift"] = QColor("#ffac45");
    m_languageColors["Kotlin"] = QColor("#F18E33");
    m_languageColors["Other"] = QColor("#cccccc");
}

ChartWidget::~ChartWidget()
{
    // 直接删除图表对象，避免调用setChart(nullptr)
    if (m_currentChart) {
        delete m_currentChart;
        m_currentChart = nullptr;
    }
    
    // Qt会自动清理子组件和系列对象，包括m_chartView
}

void ChartWidget::initializeUI()
{
    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
    // 创建内容布局
    m_contentLayout = new QHBoxLayout();
    m_contentLayout->setSpacing(10);
    
    // 创建控制面板
    m_controlPanel = createControlPanel();
    m_controlPanel->setFixedWidth(250);
    
    // 创建图表视图
    m_chartContainer = createChartView();
    
    // 添加到内容布局
    m_contentLayout->addWidget(m_controlPanel);
    m_contentLayout->addWidget(m_chartContainer, 1);
    
    // 添加到主布局
    m_mainLayout->addLayout(m_contentLayout);
    
    setLayout(m_mainLayout);
}

QWidget* ChartWidget::createControlPanel()
{
    QWidget *panel = new QWidget();
    m_controlLayout = new QVBoxLayout(panel);
    m_controlLayout->setContentsMargins(10, 10, 10, 10);
    m_controlLayout->setSpacing(15);
    
    // 图表类型选择组
    m_chartTypeGroup = new QGroupBox(tr("图表类型"));
    QVBoxLayout *typeLayout = new QVBoxLayout(m_chartTypeGroup);
    
    m_chartTypeCombo = new QComboBox();
    m_chartTypeCombo->addItem(tr("语言分布饼图"), static_cast<int>(LanguagePieChart));
    m_chartTypeCombo->addItem(tr("文件类型柱状图"), static_cast<int>(FileTypeBarChart));
    m_chartTypeCombo->addItem(tr("代码行数柱状图"), static_cast<int>(CodeLinesBarChart));
    m_chartTypeCombo->addItem(tr("文件大小散点图"), static_cast<int>(FileSizeScatterChart));
    m_chartTypeCombo->addItem(tr("复杂度趋势图"), static_cast<int>(ComplexityLineChart));
    m_chartTypeCombo->addItem(tr("语言占比面积图"), static_cast<int>(LanguageAreaChart));
    
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartWidget::onChartTypeChanged);
    
    typeLayout->addWidget(m_chartTypeCombo);
    
    // 导出组
    m_exportGroup = new QGroupBox(tr("导出图表"));
    QVBoxLayout *exportLayout = new QVBoxLayout(m_exportGroup);
    
    m_exportPNGButton = new QPushButton(tr("导出为PNG"));
    m_exportSVGButton = new QPushButton(tr("导出为SVG"));
    
    connect(m_exportPNGButton, &QPushButton::clicked, this, &ChartWidget::onExportPNGClicked);
    connect(m_exportSVGButton, &QPushButton::clicked, this, &ChartWidget::onExportSVGClicked);
    
    exportLayout->addWidget(m_exportPNGButton);
    exportLayout->addWidget(m_exportSVGButton);
    
    // 图表信息标签
    m_chartInfoLabel = new QLabel(tr("请选择项目进行分析"));
    m_chartInfoLabel->setWordWrap(true);
    m_chartInfoLabel->setStyleSheet("QLabel { color: #666; font-size: 12px; padding: 10px; background-color: #f5f5f5; border-radius: 5px; }");
    
    // 添加到控制布局
    m_controlLayout->addWidget(m_chartTypeGroup);
    m_controlLayout->addWidget(m_exportGroup);
    m_controlLayout->addWidget(m_chartInfoLabel);
    m_controlLayout->addStretch();
    
    return panel;
}

QWidget* ChartWidget::createChartView()
{
    QWidget *container = new QWidget();
    m_chartLayout = new QVBoxLayout(container);
    m_chartLayout->setContentsMargins(0, 0, 0, 0);
    
    // 创建图表视图
    m_chartView = new QChartView();
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumSize(600, 400);
    
    // 创建一个初始的空图表
    m_currentChart = new QChart();
    m_currentChart->setTitle(tr("请选择项目进行分析"));
    m_currentChart->setTitleFont(QFont("Arial", 16));
    m_chartView->setChart(m_currentChart);
    
    m_chartLayout->addWidget(m_chartView);
    
    return container;
}

void ChartWidget::setAnalysisResult(const AnalysisResult &result)
{
    m_analysisResult = result;
    refreshCharts();
    
    // 更新信息标签
    QString info = tr("项目: %1\n总文件数: %2\n总行数: %3\n语言数: %4")
                   .arg(result.getProjectPath())
                   .arg(result.getTotalFiles())
                   .arg(result.getTotalLines())
                   .arg(result.getLanguageStatistics().size());
    m_chartInfoLabel->setText(info);
}

void ChartWidget::clearCharts()
{
    if (m_currentChart) {
        m_chartView->setChart(nullptr);
        delete m_currentChart;
        m_currentChart = nullptr;
    }
    
    m_analysisResult = AnalysisResult();
    m_chartInfoLabel->setText(tr("请选择项目进行分析"));
}

ChartWidget::ChartType ChartWidget::getCurrentChartType() const
{
    return m_currentChartType;
}

void ChartWidget::setChartType(ChartType type)
{
    if (m_currentChartType != type) {
        m_currentChartType = type;
        m_chartTypeCombo->setCurrentIndex(static_cast<int>(type));
        refreshCharts();
    }
}

bool ChartWidget::exportChart(const QString &filePath, const QString &format) const
{
    if (!m_chartView || !m_currentChart) {
        return false;
    }
    
    try {
        if (format.toUpper() == "SVG") {
            QSvgGenerator generator;
            generator.setFileName(filePath);
            generator.setSize(m_chartView->size());
            generator.setViewBox(m_chartView->rect());
            generator.setTitle(tr("代码分析图表"));
            generator.setDescription(tr("由CodeVisualization生成"));
            
            QPainter painter(&generator);
            m_chartView->render(&painter);
            return true;
        } else {
            // PNG/JPG格式
            QPixmap pixmap = m_chartView->grab();
            return pixmap.save(filePath, format.toUpper().toLocal8Bit().data());
        }
    } catch (...) {
        return false;
    }
}

const AnalysisResult& ChartWidget::getAnalysisResult() const
{
    return m_analysisResult;
}

void ChartWidget::refreshCharts()
{
    if (m_analysisResult.getProjectPath().isEmpty()) {
        return;
    }
    
    // 防抖：如果正在刷新，则忽略新的请求
    if (m_isRefreshing) {
        return;
    }
    
    m_isRefreshing = true;
    
    // 禁用控件防止重复操作
    if (m_chartTypeCombo) {
        m_chartTypeCombo->setEnabled(false);
    }
    
    // 彻底重新创建QChartView来避免setChart(nullptr)的崩溃问题
    if (m_currentChart) {
        // 删除旧的图表对象
        delete m_currentChart;
        m_currentChart = nullptr;
        
        // 从布局中移除旧的QChartView
        if (m_chartView) {
            m_chartLayout->removeWidget(m_chartView);
            delete m_chartView;
            m_chartView = nullptr;
        }
        
        // 创建新的QChartView
        m_chartView = new QChartView();
        m_chartView->setRenderHint(QPainter::Antialiasing);
        m_chartView->setMinimumSize(600, 400);
        
        // 添加到布局中
        m_chartLayout->addWidget(m_chartView);
        
        // 强制处理事件
        QApplication::processEvents();
    }
    
    // 清理所有系列指针
    m_languagePieSeries = nullptr;
    m_fileTypeBarSeries = nullptr;
    m_codeLinesBarSeries = nullptr;
    m_fileSizeScatterSeries = nullptr;
    m_complexityLineSeries = nullptr;
    m_languageAreaSeries = nullptr;
    
    // 再次强制处理待处理的事件
    QApplication::processEvents();
    
    // 根据类型创建新图表
    switch (m_currentChartType) {
        case LanguagePieChart:
            createLanguagePieChart();
            break;
        case FileTypeBarChart:
            createFileTypeBarChart();
            break;
        case CodeLinesBarChart:
            createCodeLinesBarChart();
            break;
        case FileSizeScatterChart:
            createFileSizeScatterChart();
            break;
        case ComplexityLineChart:
            createComplexityLineChart();
            break;
        case LanguageAreaChart:
            createLanguageAreaChart();
            break;
    }
    
    // 重新启用控件
    if (m_chartTypeCombo) {
        m_chartTypeCombo->setEnabled(true);
    }
    
    m_isRefreshing = false;
}

void ChartWidget::updateChartTheme(const QString &theme)
{
    if (!m_currentChart) {
        return;
    }
    
    if (theme == "Light") {
        m_currentChart->setTheme(QChart::ChartThemeLight);
    } else if (theme == "Dark") {
        m_currentChart->setTheme(QChart::ChartThemeDark);
    } else if (theme == "Blue") {
        m_currentChart->setTheme(QChart::ChartThemeBlueIcy);
    }
}

void ChartWidget::onChartTypeChanged(int index)
{
    if (index < 0 || index >= m_chartTypeCombo->count()) {
        return;
    }
    
    ChartType newType = static_cast<ChartType>(m_chartTypeCombo->itemData(index).toInt());
    setChartType(newType);
}

void ChartWidget::onPieSliceClicked(QPieSlice *slice)
{
    if (slice) {
        emit chartClicked(slice->label(), slice->value());
        
        // 切换切片的突出显示
        slice->setExploded(!slice->isExploded());
    }
}

void ChartWidget::onPieSliceHovered(QPieSlice *slice, bool state)
{
    if (slice) {
        emit chartHovered(slice->label(), slice->value(), state);
        
        // 悬停时改变切片颜色
        if (state) {
            slice->setBrush(slice->brush().color().lighter(120));
        } else {
            QString language = slice->label().split(" ").first();
            slice->setBrush(getLanguageColor(language));
        }
    }
}

void ChartWidget::onBarClicked(int index, QBarSet *barset)
{
    if (barset && index >= 0) {
        QString category = "";
        double value = barset->at(index);
        emit chartClicked(category, value);
    }
}

void ChartWidget::onExportPNGClicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("导出PNG图片"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/chart.png",
        tr("PNG图片 (*.png)"));
    
    if (!fileName.isEmpty()) {
        if (exportChart(fileName, "PNG")) {
            QMessageBox::information(this, tr("导出成功"), tr("图表已成功导出为PNG格式"));
        } else {
            QMessageBox::warning(this, tr("导出失败"), tr("无法导出图表"));
        }
    }
}

void ChartWidget::onExportSVGClicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("导出SVG图片"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/chart.svg",
        tr("SVG图片 (*.svg)"));
    
    if (!fileName.isEmpty()) {
        if (exportChart(fileName, "SVG")) {
            QMessageBox::information(this, tr("导出成功"), tr("图表已成功导出为SVG格式"));
        } else {
            QMessageBox::warning(this, tr("导出失败"), tr("无法导出图表"));
        }
    }
}

void ChartWidget::createLanguagePieChart()
{
    if (m_analysisResult.getProjectPath().isEmpty()) {
        return;
    }
    
    m_currentChart = new QChart();
    m_languagePieSeries = new QPieSeries();
    
    // 获取语言统计数据
    const auto& languageStats = m_analysisResult.getLanguageStatistics();
    
    for (auto it = languageStats.begin(); it != languageStats.end(); ++it) {
        const QString& language = it.key();
        int lines = it.value();
        
        if (lines > 0) {
            QPieSlice *slice = m_languagePieSeries->append(QString("%1 (%2行)").arg(language).arg(lines), lines);
            slice->setBrush(getLanguageColor(language));
            slice->setLabelVisible(true);
            
            // 连接信号
            connect(slice, &QPieSlice::clicked, this, [this, slice]() {
                onPieSliceClicked(slice);
            });
            connect(slice, &QPieSlice::hovered, this, [this, slice](bool state) {
                onPieSliceHovered(slice, state);
            });
        }
    }
    
    m_currentChart->addSeries(m_languagePieSeries);
    m_currentChart->setTitle(tr("编程语言分布"));
    m_currentChart->legend()->setAlignment(Qt::AlignRight);
    
    setupChartStyle(m_currentChart);
    m_chartView->setChart(m_currentChart);
}

void ChartWidget::createFileTypeBarChart()
{
    if (m_analysisResult.getProjectPath().isEmpty()) {
        return;
    }
    
    m_currentChart = new QChart();
    m_fileTypeBarSeries = new QBarSeries();
    
    // 统计文件类型
    QMap<QString, int> fileTypeCount;
    const auto& fileStats = m_analysisResult.getFileStatistics();
    
    for (const auto& fileStat : fileStats) {
        QString extension = QFileInfo(fileStat.filePath).suffix().toLower();
        if (extension.isEmpty()) {
            extension = tr("无扩展名");
        }
        fileTypeCount[extension]++;
    }
    
    QBarSet *barSet = new QBarSet(tr("文件数量"));
    QStringList categories;
    
    for (auto it = fileTypeCount.begin(); it != fileTypeCount.end(); ++it) {
        categories << it.key();
        *barSet << it.value();
    }
    
    m_fileTypeBarSeries->append(barSet);
    m_currentChart->addSeries(m_fileTypeBarSeries);
    
    // 设置坐标轴
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setTitleText(tr("编程语言类型"));
    m_currentChart->addAxis(axisX, Qt::AlignBottom);
    m_fileTypeBarSeries->attachAxis(axisX);
    
    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, *std::max_element(fileTypeCount.begin(), fileTypeCount.end()) * 1.1);
    axisY->setTitleText(tr("文件数量 (个)"));
    m_currentChart->addAxis(axisY, Qt::AlignLeft);
    m_fileTypeBarSeries->attachAxis(axisY);
    
    m_currentChart->setTitle(tr("文件类型分布 - 按编程语言统计文件数量"));
    setupChartStyle(m_currentChart);
    m_chartView->setChart(m_currentChart);
    
    // 为柱状图添加数据标签说明
    if (m_currentChart->legend()) {
        m_currentChart->legend()->setVisible(true);
    }
}

void ChartWidget::createCodeLinesBarChart()
{
    if (m_analysisResult.getProjectPath().isEmpty()) {
        return;
    }
    
    m_currentChart = new QChart();
    m_codeLinesBarSeries = new QBarSeries();
    
    QBarSet *codeSet = new QBarSet(tr("代码行"));
    QBarSet *commentSet = new QBarSet(tr("注释行"));
    QBarSet *blankSet = new QBarSet(tr("空白行"));
    
    QStringList categories;
    const auto& languageStats = m_analysisResult.getLanguageStatistics();
    
    // 优化：预分配容器大小并使用更高效的数据结构
    const auto& fileStats = m_analysisResult.getFileStatistics();
    
    // 预分配容器大小
    QMap<QString, int> languageCodeLines;
    QMap<QString, int> languageCommentLines;
    QMap<QString, int> languageBlankLines;
    
    // 限制处理的文件数量，避免大项目卡顿
    const int maxFiles = qMin(fileStats.size(), 10000);
    
    for (int i = 0; i < maxFiles; ++i) {
        const auto& fileStat = fileStats[i];
        const QString& language = fileStat.language;
        languageCodeLines[language] += fileStat.codeLines;
        languageCommentLines[language] += fileStat.commentLines;
        languageBlankLines[language] += fileStat.blankLines;
        
        // 每处理1000个文件就处理一次事件，保持界面响应
        if (i % 1000 == 0) {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    }
    
    // 限制显示的语言数量，避免图表过于复杂
    int processedLanguages = 0;
    const int maxLanguages = 15;
    
    for (auto it = languageStats.begin(); it != languageStats.end() && processedLanguages < maxLanguages; ++it, ++processedLanguages) {
        const QString& language = it.key();
        
        categories << language;
        *codeSet << languageCodeLines.value(language, 0);
        *commentSet << languageCommentLines.value(language, 0);
        *blankSet << languageBlankLines.value(language, 0);
    }
    
    // 设置颜色
    codeSet->setColor(QColor("#3498db"));
    commentSet->setColor(QColor("#2ecc71"));
    blankSet->setColor(QColor("#95a5a6"));
    
    m_codeLinesBarSeries->append(codeSet);
    m_codeLinesBarSeries->append(commentSet);
    m_codeLinesBarSeries->append(blankSet);
    
    m_currentChart->addSeries(m_codeLinesBarSeries);
    
    // 设置坐标轴
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setTitleText(tr("编程语言类型"));
    m_currentChart->addAxis(axisX, Qt::AlignBottom);
    m_codeLinesBarSeries->attachAxis(axisX);
    
    QValueAxis *axisY = new QValueAxis();
    int maxLines = 0;
    for (auto it = languageStats.begin(); it != languageStats.end(); ++it) {
        maxLines = qMax(maxLines, it.value());
    }
    axisY->setRange(0, maxLines * 1.1);
    axisY->setTitleText(tr("代码行数 (行)"));
    m_currentChart->addAxis(axisY, Qt::AlignLeft);
    m_codeLinesBarSeries->attachAxis(axisY);
    
    m_currentChart->setTitle(tr("各语言代码行数统计 - 包含代码、注释、空白行"));
    setupChartStyle(m_currentChart);
    m_chartView->setChart(m_currentChart);
    
    // 为代码行数图表添加详细说明
    if (m_currentChart->legend()) {
        m_currentChart->legend()->setVisible(true);
        codeSet->setLabel(tr("有效代码行"));
        commentSet->setLabel(tr("注释说明行"));
        blankSet->setLabel(tr("空白分隔行"));
    }
}

void ChartWidget::createFileSizeScatterChart()
{
    if (m_analysisResult.getProjectPath().isEmpty()) {
        return;
    }
    
    m_currentChart = new QChart();
    m_fileSizeScatterSeries = new QScatterSeries();
    
    const auto& fileStats = m_analysisResult.getFileStatistics();
    
    // 限制散点数量，避免图表过于密集和性能问题
    const int maxPoints = qMin(fileStats.size(), 5000);
    int addedPoints = 0;
    
    for (int i = 0; i < fileStats.size() && addedPoints < maxPoints; ++i) {
        const auto& fileStat = fileStats[i];
        qint64 size = fileStat.fileSize;
        int lines = fileStat.totalLines;
        
        if (size > 0 && lines > 0) {
            m_fileSizeScatterSeries->append(size / 1024.0, lines); // 转换为KB
            addedPoints++;
        }
        
        // 每处理1000个文件就处理一次事件
        if (i % 1000 == 0) {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    }
    
    m_fileSizeScatterSeries->setName(tr("文件大小 vs 行数"));
    m_fileSizeScatterSeries->setMarkerSize(8);
    m_fileSizeScatterSeries->setColor(QColor("#e74c3c"));
    
    m_currentChart->addSeries(m_fileSizeScatterSeries);
    
    // 设置坐标轴
    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText(tr("文件大小 (KB)"));
    axisX->setLabelFormat("%.1f");
    m_currentChart->addAxis(axisX, Qt::AlignBottom);
    m_fileSizeScatterSeries->attachAxis(axisX);
    
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(tr("代码行数 (行)"));
    axisY->setLabelFormat("%d");
    m_currentChart->addAxis(axisY, Qt::AlignLeft);
    m_fileSizeScatterSeries->attachAxis(axisY);
    
    m_currentChart->setTitle(tr("文件大小与行数关系 - 分析代码文件的规模分布"));
    setupChartStyle(m_currentChart);
    m_chartView->setChart(m_currentChart);
    
    // 为散点图添加说明
    if (m_currentChart->legend()) {
        m_currentChart->legend()->setVisible(true);
        m_fileSizeScatterSeries->setName(tr("文件规模分布点"));
    }
}

void ChartWidget::createComplexityLineChart()
{
    if (m_analysisResult.getProjectPath().isEmpty()) {
        return;
    }
    
    m_currentChart = new QChart();
    m_complexityLineSeries = new QLineSeries();
    
    // 优化复杂度数据计算
    const auto& fileStats = m_analysisResult.getFileStatistics();
    const int maxFiles = qMin(fileStats.size(), 5000);
    
    QMap<QString, double> languageComplexity;
    QMap<QString, int> languageFileCount;
    
    for (int i = 0; i < maxFiles; ++i) {
        const auto& fileStat = fileStats[i];
        QString extension = QFileInfo(fileStat.filePath).suffix().toLower();
        if (!extension.isEmpty() && fileStat.totalLines > 0) {
            // 简化的复杂度计算：基于行数密度
            double complexity = (double)fileStat.totalLines / qMax(1.0, (double)fileStat.fileSize / 1024.0);
            languageComplexity[extension] += complexity;
            languageFileCount[extension]++;
        }
        
        // 每处理1000个文件就处理一次事件
        if (i % 1000 == 0) {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    }
    
    // 计算平均复杂度并限制显示数量
    int index = 0;
    const int maxLanguages = 10;
    for (auto it = languageComplexity.begin(); it != languageComplexity.end() && index < maxLanguages; ++it, ++index) {
        double avgComplexity = it.value() / qMax(1, languageFileCount[it.key()]);
        m_complexityLineSeries->append(index, avgComplexity);
    }
    
    m_complexityLineSeries->setName(tr("代码复杂度"));
    m_complexityLineSeries->setColor(QColor("#9b59b6"));
    
    m_currentChart->addSeries(m_complexityLineSeries);
    
    // 设置坐标轴
    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText(tr("文件类型序号 (按复杂度排序)"));
    axisX->setLabelFormat("%d");
    axisX->setTickCount(qMin(index + 1, 6)); // 限制刻度数量
    m_currentChart->addAxis(axisX, Qt::AlignBottom);
    m_complexityLineSeries->attachAxis(axisX);
    
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(tr("代码密度 (行数/KB)"));
    axisY->setLabelFormat("%.2f");
    m_currentChart->addAxis(axisY, Qt::AlignLeft);
    m_complexityLineSeries->attachAxis(axisY);
    
    m_currentChart->setTitle(tr("代码复杂度趋势 - 数值越高表示代码越密集"));
    setupChartStyle(m_currentChart);
    m_chartView->setChart(m_currentChart);
    
    // 添加图表说明
    if (m_currentChart->legend()) {
        m_currentChart->legend()->setVisible(true);
        // 为复杂度图表添加说明文本
        m_complexityLineSeries->setName(tr("代码密度 (行数/文件大小)"));
    }
}

void ChartWidget::createLanguageAreaChart()
{
    if (m_analysisResult.getProjectPath().isEmpty()) {
        return;
    }
    
    m_currentChart = new QChart();
    
    QLineSeries *upperSeries = new QLineSeries();
    QLineSeries *lowerSeries = new QLineSeries();
    
    const auto& languageStats = m_analysisResult.getLanguageStatistics();
    
    int index = 0;
    int totalLines = m_analysisResult.getTotalLines();
    double cumulative = 0;
    
    // 限制显示的语言数量，避免图表过于复杂
    const int maxLanguages = 12;
    int processedLanguages = 0;
    
    for (auto it = languageStats.begin(); it != languageStats.end() && processedLanguages < maxLanguages; ++it, ++index, ++processedLanguages) {
        double percentage = (double)it.value() / totalLines * 100;
        
        lowerSeries->append(index, cumulative);
        cumulative += percentage;
        upperSeries->append(index, cumulative);
        
        // 每处理几个语言就处理一次事件
        if (processedLanguages % 5 == 0) {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    }
    
    m_languageAreaSeries = new QAreaSeries(upperSeries, lowerSeries);
    m_languageAreaSeries->setName(tr("语言占比累积"));
    m_languageAreaSeries->setColor(QColor("#3498db"));
    
    m_currentChart->addSeries(m_languageAreaSeries);
    
    // 设置坐标轴
    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText(tr("编程语言序号 (按使用量排序)"));
    axisX->setLabelFormat("%d");
    axisX->setTickCount(qMin(processedLanguages + 1, 6)); // 限制刻度数量
    m_currentChart->addAxis(axisX, Qt::AlignBottom);
    m_languageAreaSeries->attachAxis(axisX);
    
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(tr("累积占比 (%)"));
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%.0f%%");
    m_currentChart->addAxis(axisY, Qt::AlignLeft);
    m_languageAreaSeries->attachAxis(axisY);
    
    m_currentChart->setTitle(tr("编程语言占比面积图 - 显示各语言累积使用情况"));
    setupChartStyle(m_currentChart);
    m_chartView->setChart(m_currentChart);
    
    // 添加图表说明
    if (m_currentChart->legend()) {
        m_currentChart->legend()->setVisible(true);
        // 为面积图添加说明文本
        m_languageAreaSeries->setName(tr("语言占比累积 (按代码行数)"));
    }
}

void ChartWidget::setupChartStyle(QChart *chart)
{
    if (!chart) {
        return;
    }
    
    // 设置图例样式和位置
    QLegend *legend = chart->legend();
    if (legend) {
        legend->setVisible(true);
        legend->setAlignment(Qt::AlignBottom);
        legend->setFont(QFont("Arial", 10));
        legend->setLabelColor(QColor("#2c3e50"));
        legend->setBackgroundVisible(false);
        legend->setBorderColor(Qt::transparent);
    }
    
    // 设置图表背景和边框
    chart->setBackgroundBrush(QBrush(QColor("#ffffff")));
    chart->setPlotAreaBackgroundBrush(QBrush(QColor("#fafafa")));
    chart->setPlotAreaBackgroundVisible(true);
    
    // 设置标题样式
    chart->setTitleFont(QFont("Arial", 14, QFont::Bold));
    chart->setTitleBrush(QBrush(QColor("#2c3e50")));
    
    // 设置图表边距
    chart->setMargins(QMargins(10, 10, 10, 10));
    
    // 启用动画效果
    chart->setAnimationOptions(QChart::SeriesAnimations);
}

QColor ChartWidget::getLanguageColor(const QString &language) const
{
    return m_languageColors.value(language, QColor("#cccccc"));
}

QString ChartWidget::formatValue(double value) const
{
    if (value >= 1000000) {
        return QString("%1M").arg(value / 1000000.0, 0, 'f', 1);
    } else if (value >= 1000) {
        return QString("%1K").arg(value / 1000.0, 0, 'f', 1);
    } else {
        return QString::number(value, 'f', 0);
    }
}

QString ChartWidget::formatPercentage(double value) const
{
    return QString("%1%").arg(value, 0, 'f', 1);
}

QChart* ChartWidget::getCurrentChart() const
{
    return m_currentChart;
}

QList<QChart*> ChartWidget::getAllCharts() const
{
    QList<QChart*> charts;
    
    // 添加当前图表
    if (m_currentChart) {
        charts.append(m_currentChart);
    }
    
    // 这里可以根据需要添加其他图表
    // 例如：如果有多个图表类型，可以都添加到列表中
    
    return charts;
}
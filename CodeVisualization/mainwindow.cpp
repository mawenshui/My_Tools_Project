#include "mainwindow.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QAction>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtGui/QCloseEvent>
#include <QtGui/QIcon>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_rightSplitter(nullptr)
    , m_projectWidget(nullptr)
    , m_statisticsWidget(nullptr)
    , m_chartWidget(nullptr)
    , m_menuBar(nullptr)
    , m_fileMenu(nullptr)
    , m_viewMenu(nullptr)
    , m_toolsMenu(nullptr)
    , m_helpMenu(nullptr)
    , m_newProjectAction(nullptr)
    , m_openProjectAction(nullptr)
    , m_saveResultsAction(nullptr)
    , m_exportResultsAction(nullptr)
    , m_exitAction(nullptr)
    , m_startAnalysisAction(nullptr)
    , m_cancelAnalysisAction(nullptr)
    , m_aboutAction(nullptr)
    , m_aboutQtAction(nullptr)
    , m_helpAction(nullptr)
    , m_advancedConfigAction(nullptr)
    , m_statusLabel(nullptr)
    , m_progressBar(nullptr)
    , m_fileCountLabel(nullptr)
    , m_lineCountLabel(nullptr)
    , m_analyzer(nullptr)
    , m_analysisInProgress(false)
{
    // 初始化代码分析器
    m_analyzer = new CodeAnalyzer(this);
    // 初始化UI
    initializeUI();
    // 连接信号槽
    connectSignals();
    // 读取设置
    readSettings();
    // 更新UI状态
    updateUIState();
    // 设置窗口标题
    setWindowTitle();
}

MainWindow::~MainWindow()
{
    // 如果分析正在进行，取消分析
    if(m_analysisInProgress && m_analyzer)
    {
        m_analyzer->cancelAnalysis();
    }
    // 写入设置
    writeSettings();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(m_analysisInProgress)
    {
        QMessageBox::StandardButton reply = QMessageBox::question(
                                                this,
                                                tr("退出确认"),
                                                tr("代码分析正在进行中，确定要退出吗？"),
                                                QMessageBox::Yes | QMessageBox::No,
                                                QMessageBox::No
                                            );
        if(reply == QMessageBox::Yes)
        {
            if(m_analyzer)
            {
                m_analyzer->cancelAnalysis();
            }
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }
    else
    {
        event->accept();
    }
}

void MainWindow::startAnalysis(const QString &projectPath)
{
    if(m_analysisInProgress || !m_analyzer)
    {
        return;
    }
    
    // 显示图表选择对话框
    ChartSelectionDialog chartDialog(this);
    chartDialog.loadUserPreferences();
    
    if (chartDialog.exec() != QDialog::Accepted) {
        return; // 用户取消了分析
    }
    
    // 获取用户选择的图表选项
    ChartSelectionDialog::ChartSelectionOptions chartOptions = chartDialog.getChartSelectionOptions();
    chartDialog.saveUserPreferences();
    
    // 将图表选项传递给统计组件
    ChartGenerationOptions generationOptions;
    generationOptions.generatePieChart = chartOptions.generateLanguagePieChart;
    generationOptions.generateBarChart = chartOptions.generateFileTypeBarChart || chartOptions.generateCodeLinesBarChart;
    generationOptions.generateLineChart = chartOptions.generateComplexityLineChart;
    generationOptions.generateAreaChart = chartOptions.generateLanguageAreaChart;
    
    m_statisticsWidget->setChartGenerationOptions(generationOptions);
    
    m_currentProjectPath = projectPath;
    m_analysisInProgress = true;
    // 更新UI状态
    updateUIState();
    // 设置窗口标题
    setWindowTitle(projectPath);
    // 清空之前的结果
    m_statisticsWidget->clearContent();
    // 开始分析
    m_analyzer->analyzeProject(projectPath);
}

void MainWindow::cancelAnalysis()
{
    if(!m_analysisInProgress || !m_analyzer)
    {
        return;
    }
    m_analyzer->cancelAnalysis();
}

void MainWindow::onAnalysisStatusChanged(CodeAnalyzer::AnalysisStatus status)
{
    QString statusText;
    switch(status)
    {
        case CodeAnalyzer::Idle:
            statusText = tr("就绪");
            m_analysisInProgress = false;
            break;
        case CodeAnalyzer::Scanning:
            statusText = tr("扫描文件中...");
            break;
        case CodeAnalyzer::Analyzing:
            statusText = tr("分析代码中...");
            break;
        case CodeAnalyzer::Completed:
            statusText = tr("分析完成");
            m_analysisInProgress = false;
            break;
        case CodeAnalyzer::Cancelled:
            statusText = tr("分析已取消");
            m_analysisInProgress = false;
            break;
        case CodeAnalyzer::Error:
            statusText = tr("分析出错");
            m_analysisInProgress = false;
            break;
    }
    m_statusLabel->setText(statusText);
    updateUIState();
}

void MainWindow::onAnalysisProgressUpdated(int current, int total, const QString &message)
{
    if(total > 0)
    {
        m_progressBar->setMaximum(total);
        m_progressBar->setValue(current);
        m_progressBar->setVisible(true);
    }
    else
    {
        m_progressBar->setVisible(false);
    }
    if(!message.isEmpty())
    {
        m_statusLabel->setText(message);
    }
}

void MainWindow::onAnalysisCompleted(const AnalysisResult &result)
{
    m_analysisInProgress = false;
    
    // 显示结果
    m_statisticsWidget->setAnalysisResult(result);
    
    // 更新图表显示
    m_chartWidget->setAnalysisResult(result);
    
    // 更新状态栏
    m_fileCountLabel->setText(tr("文件: %1").arg(result.getTotalFiles()));
    m_lineCountLabel->setText(tr("总行数: %1").arg(result.getTotalLines()));
    
    // 隐藏进度条
    m_progressBar->setVisible(false);
    
    // 更新UI状态
    updateUIState();
    
    // 显示完成消息
    m_statusLabel->setText(tr("分析完成 - 共分析 %1 个文件，%2 行代码")
                           .arg(result.getTotalFiles())
                           .arg(result.getTotalLines()));
}

void MainWindow::onAnalysisError(const QString &error)
{
    m_analysisInProgress = false;
    // 隐藏进度条
    m_progressBar->setVisible(false);
    // 更新UI状态
    updateUIState();
    // 显示错误消息
    QMessageBox::critical(this, tr("分析错误"), error);
    m_statusLabel->setText(tr("分析失败: %1").arg(error));
}

void MainWindow::onFileSelected(const QString &filePath)
{
    // 可以在这里添加文件预览功能
    m_statusLabel->setText(tr("选中文件: %1").arg(QDir::toNativeSeparators(filePath)));
}

void MainWindow::newProject()
{
    if(m_analysisInProgress)
    {
        QMessageBox::information(this, tr("提示"), tr("请等待当前分析完成后再创建新项目。"));
        return;
    }
    // 清空当前结果
    m_statisticsWidget->clearContent();
    m_projectWidget->resetToDefaults();
    m_currentProjectPath.clear();
    // 更新状态
    m_statusLabel->setText(tr("就绪"));
    m_fileCountLabel->setText(tr("文件: 0"));
    m_lineCountLabel->setText(tr("总行数: 0"));
    // 设置窗口标题
    setWindowTitle();
    updateUIState();
}

void MainWindow::openProject()
{
    if(m_analysisInProgress)
    {
        QMessageBox::information(this, tr("提示"), tr("请等待当前分析完成后再打开项目。"));
        return;
    }
    QString projectPath = QFileDialog::getExistingDirectory(
                              this,
                              tr("选择项目目录"),
                              QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                          );
    if(!projectPath.isEmpty())
    {
        m_projectWidget->setProjectPath(projectPath);
    }
}

void MainWindow::saveResults()
{
    if(!m_statisticsWidget || m_statisticsWidget->getAnalysisResult().isEmpty())
    {
        QMessageBox::information(this, tr("提示"), tr("没有可保存的分析结果。"));
        return;
    }
    QString fileName = QFileDialog::getSaveFileName(
                           this,
                           tr("保存分析结果"),
                           QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/analysis_result.csv",
                           tr("CSV文件 (*.csv);;HTML文件 (*.html)")
                       );
    if(!fileName.isEmpty())
    {
        if(fileName.endsWith(".csv", Qt::CaseInsensitive))
        {
            m_statisticsWidget->exportToCSV(fileName);
        }
        else if(fileName.endsWith(".html", Qt::CaseInsensitive))
        {
            m_statisticsWidget->exportToHTML(fileName);
        }
        m_statusLabel->setText(tr("结果已保存到: %1").arg(QDir::toNativeSeparators(fileName)));
    }
}

void MainWindow::exportResults()
{
    saveResults();
}

void MainWindow::exitApplication()
{
    close();
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("关于 CodeVisualization"),
                       tr("<h3>CodeVisualization 1.0.0</h3>"
       "<p>一个强大的代码行数统计和可视化工具</p>"
       "<p>支持多种编程语言，提供详细的代码统计信息</p>"
       "<p><b>功能特性:</b></p>"
       "<ul>"
       "<li>多语言代码分析</li>"
       "<li>实时进度显示</li>"
       "<li>详细统计报告</li>"
       "<li>数据导出功能</li>"
       "<li>可视化图表</li>"
       "</ul>"
       "<p>© 2025 CodeVisualization Team</p>"
       "<p>@ Mawenshui</p>"));
}

void MainWindow::showAboutQt()
{
    QMessageBox::aboutQt(this, tr("关于 Qt"));
}

void MainWindow::showHelp()
{
    QMessageBox::information(this, tr("帮助"),
                             tr("<h3>使用说明</h3>"
       "<p><b>1. 选择项目:</b> 点击'浏览'按钮选择要分析的项目目录</p>"
       "<p><b>2. 配置选项:</b> 设置排除规则和分析选项</p>"
       "<p><b>3. 开始分析:</b> 点击'开始分析'按钮开始代码分析</p>"
       "<p><b>4. 查看结果:</b> 在右侧面板查看详细的分析结果</p>"
       "<p><b>5. 导出数据:</b> 使用'文件'菜单导出分析结果</p>"
       "<br>"
       "<p><b>支持的文件类型:</b></p>"
       "<p>C/C++, Java, Python, JavaScript, TypeScript, C#, Go, Rust, PHP, Ruby, Swift, Kotlin, Scala, HTML, CSS, XML, JSON, YAML, Markdown 等</p>"));
}

void MainWindow::showAdvancedConfig()
{
    AdvancedConfigDialog dialog(this);
    
    // 从分析器获取当前配置并设置到对话框
    if (m_analyzer) {
        dialog.setExcludeRules(m_analyzer->getExcludePatterns());
        dialog.setSupportedLanguages(m_analyzer->getSupportedLanguages());
        
        // 设置分析选项
        QMap<QString, QVariant> currentOptions;
        currentOptions["includeComments"] = true; // 默认值，可以从分析器获取
        currentOptions["includeBlankLines"] = true; // 默认值，可以从分析器获取
        currentOptions["recursiveAnalysis"] = true;
        currentOptions["followSymlinks"] = false;
        currentOptions["maxFileSize"] = 10;
        currentOptions["maxDepth"] = 10;
        currentOptions["encoding"] = "UTF-8";
        dialog.setAnalysisOptions(currentOptions);
    }
    
    if (dialog.exec() == QDialog::Accepted) {
        // 获取对话框的配置并应用到分析器
        if (m_analyzer) {
            m_analyzer->setExcludePatterns(dialog.getExcludeRules());
            m_analyzer->setSupportedLanguages(dialog.getSupportedLanguages());
            m_analyzer->setAnalysisOptions(dialog.getAnalysisOptions());
        }
        
        // 显示配置已更新的状态信息
        m_statusLabel->setText(tr("高级配置已更新"));
        QTimer::singleShot(3000, [this]() {
            m_statusLabel->setText(tr("就绪"));
        });
    }
}

void MainWindow::setLightTheme()
{
    ThemeManager::instance()->setTheme(ThemeManager::LightTheme);
}

void MainWindow::setDarkTheme()
{
    ThemeManager::instance()->setTheme(ThemeManager::DarkTheme);
}

void MainWindow::setSystemTheme()
{
    ThemeManager::instance()->setTheme(ThemeManager::SystemTheme);
}

void MainWindow::onThemeChanged(ThemeManager::ThemeType theme)
{
    // 更新菜单选中状态
    switch (theme) {
    case ThemeManager::LightTheme:
        m_lightThemeAction->setChecked(true);
        m_statusLabel->setText(tr("已切换到亮色主题"));
        break;
    case ThemeManager::DarkTheme:
        m_darkThemeAction->setChecked(true);
        m_statusLabel->setText(tr("已切换到暗色主题"));
        break;
    case ThemeManager::SystemTheme:
        m_systemThemeAction->setChecked(true);
        m_statusLabel->setText(tr("已切换到系统主题"));
        break;
    }
    
    // 3秒后恢复状态栏文本
    QTimer::singleShot(3000, [this]() {
        if (!m_analysisInProgress) {
            m_statusLabel->setText(tr("就绪"));
        }
    });
}

void MainWindow::initializeUI()
{
    // 设置窗口属性
    setMinimumSize(1200, 800);
    resize(1400, 900);
    // 创建UI组件
    createMenuBar();
    createToolBar();
    createStatusBar();
    createCentralWidget();
}

void MainWindow::createMenuBar()
{
    m_menuBar = menuBar();
    // 文件菜单
    m_fileMenu = m_menuBar->addMenu(tr("文件(&F)"));
    m_newProjectAction = new QAction(tr("新建项目(&N)"), this);
    m_newProjectAction->setShortcut(QKeySequence::New);
    m_newProjectAction->setStatusTip(tr("创建新的分析项目"));
    m_fileMenu->addAction(m_newProjectAction);
    m_openProjectAction = new QAction(tr("打开项目(&O)"), this);
    m_openProjectAction->setShortcut(QKeySequence::Open);
    m_openProjectAction->setStatusTip(tr("打开现有项目进行分析"));
    m_fileMenu->addAction(m_openProjectAction);
    m_fileMenu->addSeparator();
    m_saveResultsAction = new QAction(tr("保存结果(&S)"), this);
    m_saveResultsAction->setShortcut(QKeySequence::Save);
    m_saveResultsAction->setStatusTip(tr("保存分析结果到文件"));
    m_fileMenu->addAction(m_saveResultsAction);
    m_exportResultsAction = new QAction(tr("导出结果(&E)"), this);
    m_exportResultsAction->setShortcut(QKeySequence("Ctrl+E"));
    m_exportResultsAction->setStatusTip(tr("导出分析结果"));
    m_fileMenu->addAction(m_exportResultsAction);
    m_fileMenu->addSeparator();
    m_exitAction = new QAction(tr("退出(&X)"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("退出应用程序"));
    m_fileMenu->addAction(m_exitAction);
    // 工具菜单
    m_toolsMenu = m_menuBar->addMenu(tr("工具(&T)"));
    m_startAnalysisAction = new QAction(tr("开始分析(&S)"), this);
    m_startAnalysisAction->setShortcut(QKeySequence("F5"));
    m_startAnalysisAction->setStatusTip(tr("开始代码分析"));
    m_toolsMenu->addAction(m_startAnalysisAction);
    m_cancelAnalysisAction = new QAction(tr("取消分析(&C)"), this);
    m_cancelAnalysisAction->setShortcut(QKeySequence("Escape"));
    m_cancelAnalysisAction->setStatusTip(tr("取消当前分析"));
    m_toolsMenu->addAction(m_cancelAnalysisAction);
    m_toolsMenu->addSeparator();
    m_advancedConfigAction = new QAction(tr("高级配置(&A)"), this);
    m_advancedConfigAction->setShortcut(QKeySequence("Ctrl+Alt+S"));
    m_advancedConfigAction->setStatusTip(tr("打开高级配置对话框"));
    m_toolsMenu->addAction(m_advancedConfigAction);
    
    // 视图菜单
    m_viewMenu = m_menuBar->addMenu(tr("视图(&V)"));
    
    // 主题子菜单
    m_themeMenu = m_viewMenu->addMenu(tr("主题(&T)"));
    
    // 创建主题动作组
    m_themeActionGroup = new QActionGroup(this);
    
    m_lightThemeAction = new QAction(tr("亮色主题(&L)"), this);
    m_lightThemeAction->setCheckable(true);
    m_lightThemeAction->setStatusTip(tr("切换到亮色主题"));
    m_themeActionGroup->addAction(m_lightThemeAction);
    m_themeMenu->addAction(m_lightThemeAction);
    
    m_darkThemeAction = new QAction(tr("暗色主题(&D)"), this);
    m_darkThemeAction->setCheckable(true);
    m_darkThemeAction->setStatusTip(tr("切换到暗色主题"));
    m_themeActionGroup->addAction(m_darkThemeAction);
    m_themeMenu->addAction(m_darkThemeAction);
    
    m_systemThemeAction = new QAction(tr("跟随系统(&S)"), this);
    m_systemThemeAction->setCheckable(true);
    m_systemThemeAction->setStatusTip(tr("跟随系统主题设置"));
    m_themeActionGroup->addAction(m_systemThemeAction);
    m_themeMenu->addAction(m_systemThemeAction);
    
    // 设置当前主题
    ThemeManager::ThemeType currentTheme = ThemeManager::instance()->getCurrentTheme();
    switch (currentTheme) {
    case ThemeManager::LightTheme:
        m_lightThemeAction->setChecked(true);
        break;
    case ThemeManager::DarkTheme:
        m_darkThemeAction->setChecked(true);
        break;
    case ThemeManager::SystemTheme:
        m_systemThemeAction->setChecked(true);
        break;
    }
    
    // 帮助菜单
    m_helpMenu = m_menuBar->addMenu(tr("帮助(&H)"));
    m_helpAction = new QAction(tr("帮助(&H)"), this);
    m_helpAction->setShortcut(QKeySequence::HelpContents);
    m_helpAction->setStatusTip(tr("显示帮助信息"));
    m_helpMenu->addAction(m_helpAction);
    m_helpMenu->addSeparator();
    m_aboutAction = new QAction(tr("关于(&A)"), this);
    m_aboutAction->setStatusTip(tr("显示关于信息"));
    m_helpMenu->addAction(m_aboutAction);
    m_aboutQtAction = new QAction(tr("关于 Qt(&Q)"), this);
    m_aboutQtAction->setStatusTip(tr("显示关于Qt的信息"));
    m_helpMenu->addAction(m_aboutQtAction);
}

void MainWindow::createToolBar()
{
    QToolBar *mainToolBar = addToolBar(tr("主工具栏"));
    mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mainToolBar->addAction(m_newProjectAction);
    mainToolBar->addAction(m_openProjectAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_startAnalysisAction);
    mainToolBar->addAction(m_cancelAnalysisAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_saveResultsAction);
}

void MainWindow::createStatusBar()
{
    QStatusBar *statusBar = this->statusBar();
    // 状态标签
    m_statusLabel = new QLabel(tr("就绪"));
    statusBar->addWidget(m_statusLabel, 1);
    // 进度条
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(200);
    statusBar->addWidget(m_progressBar);
    // 文件数量标签
    m_fileCountLabel = new QLabel(tr("文件: 0"));
    m_fileCountLabel->setMinimumWidth(80);
    statusBar->addPermanentWidget(m_fileCountLabel);
    // 行数标签
    m_lineCountLabel = new QLabel(tr("总行数: 0"));
    m_lineCountLabel->setMinimumWidth(100);
    statusBar->addPermanentWidget(m_lineCountLabel);
}

void MainWindow::createCentralWidget()
{
    m_centralWidget = new QWidget();
    setCentralWidget(m_centralWidget);
    
    // 创建主分割器（水平）
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    
    // 创建项目选择组件
    m_projectWidget = new ProjectSelectionWidget();
    m_projectWidget->setMaximumWidth(400);
    m_projectWidget->setMinimumWidth(300);
    
    // 创建右侧分割器（垂直）
    m_rightSplitter = new QSplitter(Qt::Vertical);
    
    // 创建统计显示组件
    m_statisticsWidget = new StatisticsWidget();
    
    // 创建图表显示组件
    m_chartWidget = new ChartWidget();
    
    // 添加到右侧分割器
    m_rightSplitter->addWidget(m_statisticsWidget);
    m_rightSplitter->addWidget(m_chartWidget);
    
    // 设置右侧分割器比例（统计组件:图表组件 = 1:1）
    m_rightSplitter->setStretchFactor(0, 1);
    m_rightSplitter->setStretchFactor(1, 1);
    
    // 添加到主分割器
    m_mainSplitter->addWidget(m_projectWidget);
    m_mainSplitter->addWidget(m_rightSplitter);
    
    // 设置主分割器比例（项目选择:右侧内容 = 0:1）
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    
    // 设置布局
    QHBoxLayout *layout = new QHBoxLayout(m_centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_mainSplitter);
}

void MainWindow::connectSignals()
{
    // 菜单动作连接
    connect(m_newProjectAction, &QAction::triggered, this, &MainWindow::newProject);
    connect(m_openProjectAction, &QAction::triggered, this, &MainWindow::openProject);
    connect(m_saveResultsAction, &QAction::triggered, this, &MainWindow::saveResults);
    connect(m_exportResultsAction, &QAction::triggered, this, &MainWindow::exportResults);
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::exitApplication);
    connect(m_startAnalysisAction, &QAction::triggered, [this]()
    {
        if(!m_projectWidget->getProjectPath().isEmpty())
        {
            startAnalysis(m_projectWidget->getProjectPath());
        }
    });
    connect(m_cancelAnalysisAction, &QAction::triggered, this, &MainWindow::cancelAnalysis);
    connect(m_advancedConfigAction, &QAction::triggered, this, &MainWindow::showAdvancedConfig);
    connect(m_helpAction, &QAction::triggered, this, &MainWindow::showHelp);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    connect(m_aboutQtAction, &QAction::triggered, this, &MainWindow::showAboutQt);
    
    // 主题相关信号连接
    connect(m_lightThemeAction, &QAction::triggered, this, &MainWindow::setLightTheme);
    connect(m_darkThemeAction, &QAction::triggered, this, &MainWindow::setDarkTheme);
    connect(m_systemThemeAction, &QAction::triggered, this, &MainWindow::setSystemTheme);
    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, &MainWindow::onThemeChanged);
    // 项目组件信号连接
    connect(m_projectWidget, &ProjectSelectionWidget::startAnalysis,
            this, &MainWindow::startAnalysis);
    // 统计组件信号连接
    connect(m_statisticsWidget, &StatisticsWidget::fileSelected,
            this, &MainWindow::onFileSelected);
    
    // 图表组件信号连接
    connect(m_chartWidget, &ChartWidget::chartClicked,
            [this](const QString &category, double value) {
                m_statusLabel->setText(tr("图表点击: %1 = %2").arg(category).arg(value));
            });
    connect(m_chartWidget, &ChartWidget::chartHovered,
            [this](const QString &category, double value, bool hovered) {
                if (hovered) {
                    m_statusLabel->setText(tr("图表悬停: %1 = %2").arg(category).arg(value));
                } else {
                    m_statusLabel->setText(tr("就绪"));
                }
            });
    
    // PDF导出相关信号连接
    connect(m_statisticsWidget, &StatisticsWidget::chartDataRequested,
            [this]() {
                QList<QChart*> charts = m_chartWidget->getAllCharts();
                m_statisticsWidget->receiveChartData(charts);
            });
    
    // 分析器信号连接
    connect(m_analyzer, &CodeAnalyzer::statusChanged,
            this, &MainWindow::onAnalysisStatusChanged);
    connect(m_analyzer, &CodeAnalyzer::progressUpdated,
            this, &MainWindow::onAnalysisProgressUpdated);
    connect(m_analyzer, &CodeAnalyzer::analysisCompleted,
            this, &MainWindow::onAnalysisCompleted);
    connect(m_analyzer, &CodeAnalyzer::analysisError,
            this, &MainWindow::onAnalysisError);
}

void MainWindow::readSettings()
{
    QSettings settings;
    // 窗口几何
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    // 分割器状态
    if(m_mainSplitter)
    {
        m_mainSplitter->restoreState(settings.value("mainSplitterState").toByteArray());
    }
    if(m_rightSplitter)
    {
        m_rightSplitter->restoreState(settings.value("rightSplitterState").toByteArray());
    }
}

void MainWindow::writeSettings()
{
    QSettings settings;
    // 窗口几何
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    // 分割器状态
    if(m_mainSplitter)
    {
        settings.setValue("mainSplitterState", m_mainSplitter->saveState());
    }
    if(m_rightSplitter)
    {
        settings.setValue("rightSplitterState", m_rightSplitter->saveState());
    }
}

void MainWindow::updateUIState()
{
    bool hasResults = m_statisticsWidget && !m_statisticsWidget->getAnalysisResult().isEmpty();
    bool canStartAnalysis = !m_analysisInProgress &&
                            m_projectWidget &&
                            !m_projectWidget->getProjectPath().isEmpty();
    // 更新动作状态
    if(m_newProjectAction)
    {
        m_newProjectAction->setEnabled(!m_analysisInProgress);
    }
    if(m_openProjectAction)
    {
        m_openProjectAction->setEnabled(!m_analysisInProgress);
    }
    if(m_saveResultsAction)
    {
        m_saveResultsAction->setEnabled(hasResults && !m_analysisInProgress);
    }
    if(m_exportResultsAction)
    {
        m_exportResultsAction->setEnabled(hasResults && !m_analysisInProgress);
    }
    if(m_startAnalysisAction)
    {
        m_startAnalysisAction->setEnabled(canStartAnalysis);
    }
    if(m_cancelAnalysisAction)
    {
        m_cancelAnalysisAction->setEnabled(m_analysisInProgress);
    }
}

void MainWindow::setWindowTitle(const QString &projectPath)
{
    QString title = tr("CodeVisualization");
    if(!projectPath.isEmpty())
    {
        QDir dir(projectPath);
        title += tr(" - %1").arg(dir.dirName());
    }
    QMainWindow::setWindowTitle(title);
}


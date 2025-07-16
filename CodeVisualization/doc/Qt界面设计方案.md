# Qt代码行数统计工具 - 界面设计方案

## 概述

本文档详细描述了基于Qt Creator + MinGW技术栈的代码行数统计工具的界面设计方案。采用Qt Widgets框架，充分利用Qt原生组件和数据类型，实现现代化、高效的桌面应用程序界面。

## 技术架构

### 主要Qt模块
- **Qt Widgets**：主要UI框架
- **Qt Charts**：图表和数据可视化
- **Qt Core**：核心功能和数据类型
- **Qt GUI**：图形界面基础
- **Qt Concurrent**：多线程处理

### 核心Qt组件
- **QMainWindow**：主窗口框架
- **QStackedWidget**：多页面切换
- **QSplitter**：可调整布局
- **QTreeView/QTableView**：数据展示
- **QProgressBar**：进度指示
- **QChart系列**：图表组件

## 主窗口设计 (MainWindow)

### 窗口结构
```cpp
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAnalysisStarted();
    void onAnalysisFinished();
    void onPageChanged(int index);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    
    // UI组件
    QStackedWidget *m_stackedWidget;
    QMenuBar *m_menuBar;
    QToolBar *m_toolBar;
    QStatusBar *m_statusBar;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
};
```

### 菜单栏设计
- **文件菜单**
  - 打开项目 (Ctrl+O)
  - 最近项目
  - 导出报告 (Ctrl+E)
  - 退出 (Ctrl+Q)

- **编辑菜单**
  - 复制结果 (Ctrl+C)
  - 全选 (Ctrl+A)
  - 查找 (Ctrl+F)

- **视图菜单**
  - 切换主题
  - 显示/隐藏工具栏
  - 全屏模式 (F11)

- **工具菜单**
  - 设置 (Ctrl+,)
  - 语言设置
  - 重新分析 (F5)

- **帮助菜单**
  - 关于
  - 用户手册
  - 检查更新

### 工具栏设计
```cpp
void MainWindow::setupToolBar()
{
    m_toolBar = addToolBar(tr("主工具栏"));
    
    // 添加常用操作按钮
    QAction *openAction = m_toolBar->addAction(QIcon(":/icons/open.png"), tr("打开项目"));
    QAction *analyzeAction = m_toolBar->addAction(QIcon(":/icons/analyze.png"), tr("开始分析"));
    QAction *exportAction = m_toolBar->addAction(QIcon(":/icons/export.png"), tr("导出报告"));
    
    m_toolBar->addSeparator();
    
    QAction *settingsAction = m_toolBar->addAction(QIcon(":/icons/settings.png"), tr("设置"));
    
    // 连接信号槽
    connect(openAction, &QAction::triggered, this, &MainWindow::openProject);
    connect(analyzeAction, &QAction::triggered, this, &MainWindow::startAnalysis);
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportReport);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
}
```

## 页面设计

### 1. 项目选择页面 (ProjectSelectionWidget)

```cpp
class ProjectSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectSelectionWidget(QWidget *parent = nullptr);

signals:
    void projectSelected(const QString &path);
    void analysisRequested();

private slots:
    void onBrowseClicked();
    void onRecentProjectClicked(const QModelIndex &index);
    void onDragEnterEvent(QDragEnterEvent *event);
    void onDropEvent(QDropEvent *event);

private:
    void setupUI();
    void updateRecentProjects();
    
    // UI组件
    QVBoxLayout *m_mainLayout;
    QGroupBox *m_projectGroup;
    QLineEdit *m_projectPathEdit;
    QPushButton *m_browseButton;
    QListWidget *m_recentProjectsList;
    
    QGroupBox *m_configGroup;
    QListWidget *m_languageList;
    QTextEdit *m_excludeRulesEdit;
    QCheckBox *m_includeCommentsCheck;
    QCheckBox *m_includeBlankLinesCheck;
    
    QPushButton *m_analyzeButton;
};
```

#### 界面布局
- **项目选择区域**
  - 路径输入框 (QLineEdit)
  - 浏览按钮 (QPushButton)
  - 拖拽提示区域 (支持拖拽文件夹)
  - 最近项目列表 (QListWidget)

- **配置选项区域**
  - 语言选择列表 (QListWidget，多选)
  - 排除规则文本框 (QTextEdit)
  - 分析选项复选框 (QCheckBox)

- **操作按钮**
  - 开始分析按钮 (QPushButton)

### 2. 统计结果页面 (StatisticsWidget)

```cpp
class StatisticsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatisticsWidget(QWidget *parent = nullptr);
    void updateStatistics(const AnalysisResult &result);

private:
    void setupUI();
    void createSummaryCards();
    void createDetailTable();
    void createFileTree();
    
    // UI组件
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_summaryLayout;
    
    // 概览卡片
    QVector<QFrame*> m_summaryCards;
    QLabel *m_totalLinesLabel;
    QLabel *m_codeLinesLabel;
    QLabel *m_commentLinesLabel;
    QLabel *m_blankLinesLabel;
    QLabel *m_filesCountLabel;
    QLabel *m_languagesCountLabel;
    
    // 详细表格
    QTableView *m_statisticsTable;
    QStandardItemModel *m_tableModel;
    
    // 文件树
    QTreeView *m_fileTree;
    QStandardItemModel *m_treeModel;
    
    // 搜索和过滤
    QLineEdit *m_searchEdit;
    QComboBox *m_filterCombo;
};
```

#### 界面布局
- **概览卡片区域**
  - 6个统计卡片 (QFrame + QLabel)
  - 使用QHBoxLayout水平排列
  - 每个卡片显示关键指标

- **详细统计表格**
  - QTableView + QStandardItemModel
  - 支持排序和筛选
  - 自定义委托显示进度条

- **文件结构树**
  - QTreeView + QStandardItemModel
  - 分层显示文件和目录
  - 显示每个文件的行数信息

### 3. 数据可视化页面 (VisualizationWidget)

```cpp
class VisualizationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VisualizationWidget(QWidget *parent = nullptr);
    void updateCharts(const AnalysisResult &result);

private:
    void setupUI();
    void createLanguagePieChart();
    void createCodeLinesBarChart();
    void createTrendChart();
    void createHeatMap();
    
    // Qt Charts组件
    QChartView *m_pieChartView;
    QPieChart *m_pieChart;
    
    QChartView *m_barChartView;
    QBarChart *m_barChart;
    
    QChartView *m_lineChartView;
    QLineChart *m_lineChart;
    
    // 自定义图表
    QWidget *m_heatMapWidget;
    
    // 布局
    QGridLayout *m_chartsLayout;
    QComboBox *m_chartTypeCombo;
};
```

#### 图表类型
- **语言分布饼图** (QPieChart)
  - 显示各语言代码行数占比
  - 支持切片选择和高亮
  - 动态标签显示

- **代码行数柱状图** (QBarChart)
  - 对比各语言的代码行数
  - 支持缩放和平移
  - 工具提示显示详细信息

- **趋势折线图** (QLineChart)
  - 显示项目演化趋势
  - 时间轴显示
  - 多系列数据对比

- **文件大小热力图** (自定义QWidget)
  - 使用QPainter绘制
  - 颜色映射文件大小
  - 交互式缩放

### 4. 版本对比页面 (ComparisonWidget)

```cpp
class ComparisonWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ComparisonWidget(QWidget *parent = nullptr);
    void setComparisonData(const AnalysisResult &baseline, const AnalysisResult &target);

private slots:
    void onBaselineChanged(const QString &path);
    void onTargetChanged(const QString &path);
    void onCompareClicked();

private:
    void setupUI();
    void updateComparisonTable();
    void updateDifferenceCharts();
    
    // 版本选择
    QComboBox *m_baselineCombo;
    QComboBox *m_targetCombo;
    QPushButton *m_compareButton;
    
    // 差异统计
    QTableView *m_comparisonTable;
    QStandardItemModel *m_comparisonModel;
    
    // 差异图表
    QChartView *m_differenceChartView;
    QBarChart *m_differenceChart;
    
    // 变化概览
    QLabel *m_addedLinesLabel;
    QLabel *m_deletedLinesLabel;
    QLabel *m_modifiedLinesLabel;
};
```

### 5. 设置页面 (SettingsWidget)

```cpp
class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    void loadSettings();
    void saveSettings();

private slots:
    void onThemeChanged(int index);
    void onLanguageChanged(int index);
    void onFontSizeChanged(int value);
    void onResetClicked();

private:
    void setupUI();
    void createAppearanceGroup();
    void createAnalysisGroup();
    void createAdvancedGroup();
    
    // 外观设置
    QGroupBox *m_appearanceGroup;
    QComboBox *m_themeCombo;
    QComboBox *m_languageCombo;
    QSpinBox *m_fontSizeSpinBox;
    
    // 分析设置
    QGroupBox *m_analysisGroup;
    QListWidget *m_defaultExcludesList;
    QSpinBox *m_maxFileSizeSpinBox;
    QSpinBox *m_threadCountSpinBox;
    
    // 高级设置
    QGroupBox *m_advancedGroup;
    QCheckBox *m_enableCacheCheck;
    QSpinBox *m_autoSaveIntervalSpinBox;
    QSpinBox *m_recentProjectsCountSpinBox;
    
    // 操作按钮
    QPushButton *m_saveButton;
    QPushButton *m_resetButton;
    QPushButton *m_applyButton;
};
```

### 6. 导出报告页面 (ExportWidget)

```cpp
class ExportWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ExportWidget(QWidget *parent = nullptr);
    void setAnalysisResult(const AnalysisResult &result);

private slots:
    void onTemplateChanged(int index);
    void onFormatChanged(int index);
    void onExportClicked();
    void onPreviewClicked();

private:
    void setupUI();
    void updatePreview();
    
    // 模板选择
    QComboBox *m_templateCombo;
    QListWidget *m_contentList;
    
    // 格式选择
    QButtonGroup *m_formatGroup;
    QRadioButton *m_pdfRadio;
    QRadioButton *m_excelRadio;
    QRadioButton *m_htmlRadio;
    QRadioButton *m_jsonRadio;
    
    // 预览区域
    QTextEdit *m_previewEdit;
    
    // 导出选项
    QCheckBox *m_includeChartsCheck;
    QCheckBox *m_includeFileListCheck;
    QLineEdit *m_outputPathEdit;
    QPushButton *m_browseOutputButton;
    
    // 操作按钮
    QPushButton *m_previewButton;
    QPushButton *m_exportButton;
};
```

## 样式设计 (QSS)

### 主题系统
```cpp
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum Theme {
        LightTheme,
        DarkTheme,
        BlueTheme,
        GreenTheme
    };
    
    static ThemeManager* instance();
    void setTheme(Theme theme);
    Theme currentTheme() const;
    
signals:
    void themeChanged(Theme theme);
    
private:
    void loadStyleSheet(const QString &fileName);
    
    Theme m_currentTheme;
    static ThemeManager *s_instance;
};
```

### 浅色主题样式
```css
/* 主窗口样式 */
QMainWindow {
    background-color: #f5f7fa;
    color: #333333;
}

/* 按钮样式 */
QPushButton {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #667eea, stop:1 #764ba2);
    color: white;
    border: none;
    border-radius: 8px;
    padding: 8px 16px;
    font-weight: bold;
}

QPushButton:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #5a6fd8, stop:1 #6a4190);
}

QPushButton:pressed {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4e60c6, stop:1 #5e377e);
}

/* 表格样式 */
QTableView {
    background-color: white;
    border: 1px solid #e9ecef;
    border-radius: 8px;
    gridline-color: #e9ecef;
}

QTableView::item {
    padding: 8px;
    border-bottom: 1px solid #e9ecef;
}

QTableView::item:selected {
    background-color: #667eea;
    color: white;
}

/* 卡片样式 */
QFrame.summary-card {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #667eea, stop:1 #764ba2);
    border-radius: 12px;
    padding: 16px;
    margin: 8px;
}

/* 输入框样式 */
QLineEdit, QTextEdit {
    border: 2px solid #e9ecef;
    border-radius: 8px;
    padding: 8px;
    background-color: white;
}

QLineEdit:focus, QTextEdit:focus {
    border-color: #667eea;
    outline: none;
}
```

## 数据模型设计

### 统计数据模型
```cpp
class StatisticsTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit StatisticsTableModel(QObject *parent = nullptr);
    
    // QAbstractTableModel接口
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    void setStatisticsData(const QVector<LanguageStatistics> &data);
    void sortByColumn(int column, Qt::SortOrder order);
    
private:
    QVector<LanguageStatistics> m_data;
    QStringList m_headers;
};
```

### 文件树模型
```cpp
class FileTreeModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit FileTreeModel(QObject *parent = nullptr);
    
    void setFileData(const FileAnalysisResult &result);
    void expandToFile(const QString &filePath);
    
private:
    void addFileItem(QStandardItem *parent, const FileInfo &fileInfo);
    QStandardItem* createFileItem(const FileInfo &fileInfo);
    
    QHash<QString, QStandardItem*> m_pathToItem;
};
```

## 国际化支持

### 翻译文件结构
```
resources/
├── translations/
│   ├── app_zh_CN.ts    # 简体中文
│   ├── app_zh_TW.ts    # 繁体中文
│   ├── app_en_US.ts    # 英文
│   └── app_ja_JP.ts    # 日文
└── icons/
    ├── light/
    └── dark/
```

### 翻译管理
```cpp
class TranslationManager : public QObject
{
    Q_OBJECT

public:
    static TranslationManager* instance();
    
    void setLanguage(const QString &language);
    QString currentLanguage() const;
    QStringList availableLanguages() const;
    
signals:
    void languageChanged(const QString &language);
    
private:
    void loadTranslation(const QString &language);
    
    QTranslator *m_translator;
    QString m_currentLanguage;
    static TranslationManager *s_instance;
};
```

## 性能优化策略

### 1. 大数据处理
- 使用QAbstractItemModel的延迟加载
- 实现虚拟滚动减少内存占用
- 分页显示大量文件列表

### 2. 多线程处理
```cpp
class AnalysisWorker : public QObject
{
    Q_OBJECT

public slots:
    void analyzeProject(const QString &projectPath, const AnalysisConfig &config);
    
signals:
    void progressUpdated(int percentage);
    void fileAnalyzed(const QString &filePath, const FileStatistics &stats);
    void analysisCompleted(const AnalysisResult &result);
    void analysisError(const QString &error);
    
private:
    void analyzeDirectory(const QString &dirPath);
    void analyzeFile(const QString &filePath);
};
```

### 3. 内存管理
- 合理使用Qt的父子对象机制
- 及时释放大型数据结构
- 使用QCache缓存计算结果

### 4. UI响应性
- 使用QTimer实现渐进式UI更新
- 异步加载图表数据
- 实现取消操作机制

## 总结

本Qt界面设计方案充分利用了Qt框架的优势，通过模块化设计实现了功能丰富、性能优异的代码统计工具。主要特点包括：

1. **原生性能**：使用Qt原生组件，确保应用程序的高性能和响应性
2. **现代化界面**：采用现代设计理念，提供直观的用户体验
3. **可扩展架构**：模块化设计便于功能扩展和维护
4. **跨平台支持**：一套代码支持多个操作系统
5. **国际化支持**：完整的多语言支持方案
6. **主题系统**：灵活的主题切换和自定义能力

该设计方案为开发高质量的代码统计分析工具提供了完整的技术基础和实现指导。
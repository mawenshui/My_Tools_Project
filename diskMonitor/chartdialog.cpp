#include "chartdialog.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QIcon>
#include <QStyle>
#include <QHeaderView>
#include <QSizePolicy>
#include <QDesktopWidget>
#include <QStorageInfo>

ChartDialog::ChartDialog(HistoryManager *historyManager, QWidget *parent)
    : QDialog(parent)
    , m_chartManager(nullptr)
    , m_historyManager(historyManager)
    , m_tabWidget(nullptr)
    , m_chartTab(nullptr)
    , m_settingsTab(nullptr)
    , m_controlGroup(nullptr)
    , m_chartTypeCombo(nullptr)
    , m_driveCombo(nullptr)
    , m_timeRangeCombo(nullptr)
    , m_startTimeEdit(nullptr)
    , m_endTimeEdit(nullptr)
    , m_customRangeCheck(nullptr)
    , m_refreshBtn(nullptr)
    , m_exportBtn(nullptr)
    , m_resetBtn(nullptr)
    , m_realTimeGroup(nullptr)
    , m_realTimeCheck(nullptr)
    , m_autoRefreshCheck(nullptr)
    , m_refreshIntervalSpin(nullptr)
    , m_statusLabel(nullptr)
    , m_refreshProgress(nullptr)
    , m_styleGroup(nullptr)
    , m_showGridCheck(nullptr)
    , m_showLegendCheck(nullptr)
    , m_enableAnimationCheck(nullptr)
    , m_opacitySlider(nullptr)
    , m_chartSplitter(nullptr)
    , m_chartContainer(nullptr)
    , m_autoRefreshTimer(new QTimer(this))
    , m_settings(new QSettings(this))
    , m_currentDriveLetter("C:")
    , m_currentChartType(DEFAULT_CHART_TYPE)
    , m_currentTimeRange(DEFAULT_TIME_RANGE)
    , m_isRealTimeEnabled(false)
    , m_refreshInterval(DEFAULT_REFRESH_INTERVAL)
{
    // 初始化图表管理器
    m_chartManager = new ChartManager(m_historyManager, this);
    
    // 设置窗口属性
    setWindowTitle("磁盘监控图表查看器");
    setWindowIcon(QIcon(":/icons/chart.png"));
    resize(1200, 800);
    setMinimumSize(800, 600);
    
    // 设置窗口标志
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    
    // 初始化UI
    setupUI();
    setupConnections();
    
    // 加载设置
    loadSettings();
    
    // 初始化状态
    updateControlsState();
    updateDriveList();
    
    qDebug() << "[ChartDialog] 图表对话框初始化完成";
}

ChartDialog::~ChartDialog()
{
    // 保存设置
    saveSettings();
    
    // 停止自动刷新
    stopAutoRefresh();
    
    qDebug() << "[ChartDialog] 图表对话框已销毁";
}

void ChartDialog::setupUI()
{
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    
    // 创建选项卡控件
    m_tabWidget = new QTabWidget(this);
    
    // 设置选项卡
    setupChartTab();
    setupSettingsTab();
    
    // 添加选项卡
    m_tabWidget->addTab(m_chartTab, "图表显示");
    m_tabWidget->addTab(m_settingsTab, "设置");
    
    // 创建状态栏
    setupStatusBar();
    
    // 添加到主布局
    mainLayout->addWidget(m_tabWidget);
    
    // 设置布局
    setLayout(mainLayout);
}

void ChartDialog::setupChartTab()
{
    m_chartTab = new QWidget();
    QVBoxLayout *chartLayout = new QVBoxLayout(m_chartTab);
    chartLayout->setContentsMargins(5, 5, 5, 5);
    chartLayout->setSpacing(5);
    
    // 设置控制面板
    setupControlPanel();
    chartLayout->addWidget(m_controlGroup);
    
    // 创建图表显示区域
    m_chartSplitter = new QSplitter(Qt::Vertical, m_chartTab);
    m_chartContainer = new QWidget();
    m_chartContainer->setMinimumHeight(400);
    m_chartSplitter->addWidget(m_chartContainer);
    
    chartLayout->addWidget(m_chartSplitter, 1);
}

void ChartDialog::setupSettingsTab()
{
    m_settingsTab = new QWidget();
    QVBoxLayout *settingsLayout = new QVBoxLayout(m_settingsTab);
    settingsLayout->setContentsMargins(10, 10, 10, 10);
    settingsLayout->setSpacing(10);
    
    // 设置实时更新控制
    setupRealTimeControls();
    settingsLayout->addWidget(m_realTimeGroup);
    
    // 设置样式控制
    setupStyleControls();
    settingsLayout->addWidget(m_styleGroup);
    
    // 添加弹性空间
    settingsLayout->addStretch();
}

void ChartDialog::setupControlPanel()
{
    m_controlGroup = new QGroupBox("📊 图表控制面板");
    m_controlGroup->setStyleSheet(
        "QGroupBox { "
        "    font-weight: bold; "
        "    border: 2px solid #cccccc; "
        "    border-radius: 8px; "
        "    margin-top: 1ex; "
        "    padding-top: 10px; "
        "} "
        "QGroupBox::title { "
        "    subcontrol-origin: margin; "
        "    left: 10px; "
        "    padding: 0 5px 0 5px; "
        "}"
    );
    
    QGridLayout *controlLayout = new QGridLayout(m_controlGroup);
    controlLayout->setSpacing(12);
    controlLayout->setContentsMargins(15, 20, 15, 15);
    
    int row = 0;
    
    // 图表类型选择 - 增强样式
    QLabel *chartTypeLabel = new QLabel("📈 图表类型:");
    chartTypeLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    controlLayout->addWidget(chartTypeLabel, row, 0);
    
    m_chartTypeCombo = new QComboBox();
    m_chartTypeCombo->setStyleSheet(
        "QComboBox { "
        "    border: 2px solid #bdc3c7; "
        "    border-radius: 6px; "
        "    padding: 5px; "
        "    min-width: 120px; "
        "} "
        "QComboBox:hover { "
        "    border-color: #3498db; "
        "} "
        "QComboBox::drop-down { "
        "    border: none; "
        "} "
        "QComboBox::down-arrow { "
        "    image: url(:/icons/dropdown.png); "
        "    width: 12px; "
        "    height: 12px; "
        "}"
    );
    populateChartTypes();
    controlLayout->addWidget(m_chartTypeCombo, row, 1);
    
    // 驱动器选择 - 增强样式
    QLabel *driveLabel = new QLabel("💾 驱动器:");
    driveLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    controlLayout->addWidget(driveLabel, row, 2);
    
    m_driveCombo = new QComboBox();
    m_driveCombo->setStyleSheet(m_chartTypeCombo->styleSheet());
    populateDriveList();
    controlLayout->addWidget(m_driveCombo, row, 3);
    
    row++;
    
    // 时间范围选择 - 增强样式
    QLabel *timeRangeLabel = new QLabel("⏰ 时间范围:");
    timeRangeLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    controlLayout->addWidget(timeRangeLabel, row, 0);
    
    m_timeRangeCombo = new QComboBox();
    m_timeRangeCombo->setStyleSheet(m_chartTypeCombo->styleSheet());
    populateTimeRanges();
    controlLayout->addWidget(m_timeRangeCombo, row, 1);
    
    // 自定义时间范围 - 增强样式
    m_customRangeCheck = new QCheckBox("🔧 自定义时间范围");
    m_customRangeCheck->setStyleSheet(
        "QCheckBox { "
        "    font-weight: bold; "
        "    color: #34495e; "
        "} "
        "QCheckBox::indicator { "
        "    width: 18px; "
        "    height: 18px; "
        "} "
        "QCheckBox::indicator:unchecked { "
        "    border: 2px solid #bdc3c7; "
        "    border-radius: 3px; "
        "    background-color: white; "
        "} "
        "QCheckBox::indicator:checked { "
        "    border: 2px solid #27ae60; "
        "    border-radius: 3px; "
        "    background-color: #27ae60; "
        "}"
    );
    controlLayout->addWidget(m_customRangeCheck, row, 2, 1, 2);
    
    row++;
    
    // 开始时间 - 增强样式
    QLabel *startTimeLabel = new QLabel("📅 开始时间:");
    startTimeLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    controlLayout->addWidget(startTimeLabel, row, 0);
    
    m_startTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-1));
    m_startTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
    m_startTimeEdit->setEnabled(false);
    m_startTimeEdit->setStyleSheet(
        "QDateTimeEdit { "
        "    border: 2px solid #bdc3c7; "
        "    border-radius: 6px; "
        "    padding: 5px; "
        "    background-color: #ecf0f1; "
        "} "
        "QDateTimeEdit:enabled { "
        "    background-color: white; "
        "    border-color: #3498db; "
        "} "
        "QDateTimeEdit:enabled:hover { "
        "    border-color: #2980b9; "
        "}"
    );
    controlLayout->addWidget(m_startTimeEdit, row, 1);
    
    // 结束时间 - 增强样式
    QLabel *endTimeLabel = new QLabel("📅 结束时间:");
    endTimeLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    controlLayout->addWidget(endTimeLabel, row, 2);
    
    m_endTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    m_endTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
    m_endTimeEdit->setEnabled(false);
    m_endTimeEdit->setStyleSheet(m_startTimeEdit->styleSheet());
    controlLayout->addWidget(m_endTimeEdit, row, 3);
    
    row++;
    
    // 操作按钮 - 增强样式和功能
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    
    // 定义按钮样式
    QString buttonStyle = 
        "QPushButton { "
        "    background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
        "                                      stop: 0 #f8f9fa, stop: 1 #e9ecef); "
        "    border: 2px solid #dee2e6; "
        "    border-radius: 8px; "
        "    padding: 8px 16px; "
        "    font-weight: bold; "
        "    min-width: 80px; "
        "} "
        "QPushButton:hover { "
        "    background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
        "                                      stop: 0 #e9ecef, stop: 1 #dee2e6); "
        "    border-color: #adb5bd; "
        "} "
        "QPushButton:pressed { "
        "    background-color: #dee2e6; "
        "    border-color: #6c757d; "
        "} "
        "QPushButton:disabled { "
        "    background-color: #f8f9fa; "
        "    color: #6c757d; "
        "    border-color: #dee2e6; "
        "}";
    
    m_refreshBtn = new QPushButton("🔄 刷新");
    m_refreshBtn->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_refreshBtn->setStyleSheet(buttonStyle);
    m_refreshBtn->setToolTip("刷新当前图表数据");
    buttonLayout->addWidget(m_refreshBtn);
    
    m_exportBtn = new QPushButton("💾 导出");
    m_exportBtn->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_exportBtn->setStyleSheet(buttonStyle);
    m_exportBtn->setToolTip("导出图表为图片或PDF文件");
    buttonLayout->addWidget(m_exportBtn);
    
    m_resetBtn = new QPushButton("🔄 重置");
    m_resetBtn->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    m_resetBtn->setStyleSheet(buttonStyle);
    m_resetBtn->setToolTip("重置所有设置为默认值");
    buttonLayout->addWidget(m_resetBtn);
    
    // 添加快速操作按钮
    m_fullScreenBtn = new QPushButton("🖥️ 全屏");
    m_fullScreenBtn->setStyleSheet(buttonStyle);
    m_fullScreenBtn->setToolTip("全屏显示图表");
    buttonLayout->addWidget(m_fullScreenBtn);
    
    buttonLayout->addStretch();
    
    // 添加状态指示器
    m_statusIndicator = new QLabel("●");
    m_statusIndicator->setStyleSheet("color: #27ae60; font-size: 16px;");
    m_statusIndicator->setToolTip("连接状态: 正常");
    buttonLayout->addWidget(m_statusIndicator);
    
    controlLayout->addLayout(buttonLayout, row, 0, 1, 4);
}

void ChartDialog::setupRealTimeControls()
{
    m_realTimeGroup = new QGroupBox("⚡ 实时更新设置");
    m_realTimeGroup->setStyleSheet(
        "QGroupBox { "
        "    font-weight: bold; "
        "    border: 2px solid #e74c3c; "
        "    border-radius: 8px; "
        "    margin-top: 1ex; "
        "    padding-top: 10px; "
        "    color: #2c3e50; "
        "} "
        "QGroupBox::title { "
        "    subcontrol-origin: margin; "
        "    left: 10px; "
        "    padding: 0 5px 0 5px; "
        "    color: #e74c3c; "
        "}"
    );
    
    QGridLayout *realTimeLayout = new QGridLayout(m_realTimeGroup);
    realTimeLayout->setSpacing(12);
    realTimeLayout->setContentsMargins(15, 20, 15, 15);
    
    int row = 0;
    
    // 实时图表开关 - 增强样式
    m_realTimeCheck = new QCheckBox("🔴 启用实时图表");
    m_realTimeCheck->setChecked(false);
    m_realTimeCheck->setStyleSheet(
        "QCheckBox { "
        "    font-weight: bold; "
        "    color: #e74c3c; "
        "    font-size: 14px; "
        "} "
        "QCheckBox::indicator { "
        "    width: 20px; "
        "    height: 20px; "
        "} "
        "QCheckBox::indicator:unchecked { "
        "    border: 2px solid #e74c3c; "
        "    border-radius: 4px; "
        "    background-color: white; "
        "} "
        "QCheckBox::indicator:checked { "
        "    border: 2px solid #e74c3c; "
        "    border-radius: 4px; "
        "    background-color: #e74c3c; "
        "    image: url(:/icons/check.png); "
        "}"
    );
    realTimeLayout->addWidget(m_realTimeCheck, row, 0, 1, 3);
    
    row++;
    
    // 自动刷新开关 - 增强样式
    m_autoRefreshCheck = new QCheckBox("🔄 启用自动刷新");
    m_autoRefreshCheck->setChecked(true);
    m_autoRefreshCheck->setStyleSheet(
        "QCheckBox { "
        "    font-weight: bold; "
        "    color: #3498db; "
        "} "
        "QCheckBox::indicator { "
        "    width: 18px; "
        "    height: 18px; "
        "} "
        "QCheckBox::indicator:unchecked { "
        "    border: 2px solid #3498db; "
        "    border-radius: 3px; "
        "    background-color: white; "
        "} "
        "QCheckBox::indicator:checked { "
        "    border: 2px solid #3498db; "
        "    border-radius: 3px; "
        "    background-color: #3498db; "
        "}"
    );
    realTimeLayout->addWidget(m_autoRefreshCheck, row, 0);
    
    // 刷新间隔 - 增强样式
    QLabel *intervalLabel = new QLabel("⏱️ 刷新间隔:");
    intervalLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    realTimeLayout->addWidget(intervalLabel, row, 1);
    
    m_refreshIntervalSpin = new QSpinBox();
    m_refreshIntervalSpin->setRange(5, 300);
    m_refreshIntervalSpin->setValue(DEFAULT_REFRESH_INTERVAL);
    m_refreshIntervalSpin->setSuffix(" 秒");
    m_refreshIntervalSpin->setStyleSheet(
        "QSpinBox { "
        "    border: 2px solid #bdc3c7; "
        "    border-radius: 6px; "
        "    padding: 5px; "
        "    background-color: white; "
        "    min-width: 80px; "
        "} "
        "QSpinBox:hover { "
        "    border-color: #3498db; "
        "} "
        "QSpinBox:focus { "
        "    border-color: #2980b9; "
        "    background-color: #ecf0f1; "
        "}"
    );
    realTimeLayout->addWidget(m_refreshIntervalSpin, row, 2);
    
    row++;
    
    // 状态显示 - 增强样式
    m_statusLabel = new QLabel("📊 状态: 就绪");
    m_statusLabel->setStyleSheet(
        "QLabel { "
        "    font-weight: bold; "
        "    color: #27ae60; "
        "    background-color: #d5f4e6; "
        "    border: 1px solid #27ae60; "
        "    border-radius: 4px; "
        "    padding: 5px; "
        "}"
    );
    realTimeLayout->addWidget(m_statusLabel, row, 0);
    
    m_refreshProgress = new QProgressBar();
    m_refreshProgress->setVisible(false);
    m_refreshProgress->setStyleSheet(
        "QProgressBar { "
        "    border: 2px solid #bdc3c7; "
        "    border-radius: 6px; "
        "    text-align: center; "
        "    background-color: #ecf0f1; "
        "} "
        "QProgressBar::chunk { "
        "    background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, "
        "                                      stop: 0 #3498db, stop: 1 #2980b9); "
        "    border-radius: 4px; "
        "}"
    );
    realTimeLayout->addWidget(m_refreshProgress, row, 1, 1, 2);
}

void ChartDialog::setupStyleControls()
{
    m_styleGroup = new QGroupBox("🎨 图表样式设置");
    m_styleGroup->setStyleSheet(
        "QGroupBox { "
        "    font-weight: bold; "
        "    border: 2px solid #9b59b6; "
        "    border-radius: 8px; "
        "    margin-top: 1ex; "
        "    padding-top: 10px; "
        "    color: #2c3e50; "
        "} "
        "QGroupBox::title { "
        "    subcontrol-origin: margin; "
        "    left: 10px; "
        "    padding: 0 5px 0 5px; "
        "    color: #9b59b6; "
        "}"
    );
    
    QGridLayout *styleLayout = new QGridLayout(m_styleGroup);
    styleLayout->setSpacing(12);
    styleLayout->setContentsMargins(15, 20, 15, 15);
    
    int row = 0;
    
    // 网格显示 - 增强样式
    m_showGridCheck = new QCheckBox("📊 显示网格");
    m_showGridCheck->setChecked(true);
    m_showGridCheck->setStyleSheet(
        "QCheckBox { "
        "    font-weight: bold; "
        "    color: #9b59b6; "
        "} "
        "QCheckBox::indicator { "
        "    width: 18px; "
        "    height: 18px; "
        "} "
        "QCheckBox::indicator:unchecked { "
        "    border: 2px solid #9b59b6; "
        "    border-radius: 3px; "
        "    background-color: white; "
        "} "
        "QCheckBox::indicator:checked { "
        "    border: 2px solid #9b59b6; "
        "    border-radius: 3px; "
        "    background-color: #9b59b6; "
        "}"
    );
    styleLayout->addWidget(m_showGridCheck, row, 0);
    
    // 图例显示 - 增强样式
    m_showLegendCheck = new QCheckBox("📋 显示图例");
    m_showLegendCheck->setChecked(true);
    m_showLegendCheck->setStyleSheet(m_showGridCheck->styleSheet());
    styleLayout->addWidget(m_showLegendCheck, row, 1);
    
    row++;
    
    // 动画效果 - 增强样式
    m_enableAnimationCheck = new QCheckBox("✨ 启用动画效果");
    m_enableAnimationCheck->setChecked(true);
    m_enableAnimationCheck->setStyleSheet(m_showGridCheck->styleSheet());
    styleLayout->addWidget(m_enableAnimationCheck, row, 0);
    
    // 添加主题选择
    QLabel *themeLabel = new QLabel("🌈 图表主题:");
    themeLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    styleLayout->addWidget(themeLabel, row, 1);
    
    row++;
    
    // 透明度控制 - 增强样式
    QLabel *opacityLabel = new QLabel("🔍 图表透明度:");
    opacityLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    styleLayout->addWidget(opacityLabel, row, 0);
    
    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(50, 100);
    m_opacitySlider->setValue(100);
    m_opacitySlider->setStyleSheet(
        "QSlider::groove:horizontal { "
        "    border: 1px solid #bdc3c7; "
        "    height: 8px; "
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                                stop:0 #ecf0f1, stop:1 #bdc3c7); "
        "    margin: 2px 0; "
        "    border-radius: 4px; "
        "} "
        "QSlider::handle:horizontal { "
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "                                stop:0 #9b59b6, stop:1 #8e44ad); "
        "    border: 1px solid #8e44ad; "
        "    width: 18px; "
        "    margin: -2px 0; "
        "    border-radius: 9px; "
        "} "
        "QSlider::handle:horizontal:hover { "
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "                                stop:0 #a569bd, stop:1 #9b59b6); "
        "}"
    );
    styleLayout->addWidget(m_opacitySlider, row, 1);
    
    // 添加透明度值显示标签
    m_opacityValueLabel = new QLabel("100%");
    m_opacityValueLabel->setStyleSheet(
        "QLabel { "
        "    font-weight: bold; "
        "    color: #9b59b6; "
        "    background-color: #f8f9fa; "
        "    border: 1px solid #9b59b6; "
        "    border-radius: 3px; "
        "    padding: 2px 6px; "
        "    min-width: 40px; "
        "}"
    );
    styleLayout->addWidget(m_opacityValueLabel, row, 2);
    
    // 连接透明度滑块和标签
    connect(m_opacitySlider, &QSlider::valueChanged, [this](int value) {
        m_opacityValueLabel->setText(QString("%1%").arg(value));
    });
    
    row++;
    
    // 添加主题选择下拉框
    m_themeCombo = new QComboBox();
    m_themeCombo->addItem("🌟 默认主题", "default");
    m_themeCombo->addItem("🌙 深色主题", "dark");
    m_themeCombo->addItem("🌈 彩色主题", "colorful");
    m_themeCombo->addItem("💼 商务主题", "business");
    m_themeCombo->addItem("🎨 艺术主题", "artistic");
    m_themeCombo->setStyleSheet(
        "QComboBox { "
        "    border: 2px solid #9b59b6; "
        "    border-radius: 6px; "
        "    padding: 5px; "
        "    min-width: 120px; "
        "    background-color: white; "
        "} "
        "QComboBox:hover { "
        "    border-color: #8e44ad; "
        "    background-color: #f8f9fa; "
        "} "
        "QComboBox::drop-down { "
        "    border: none; "
        "} "
        "QComboBox::down-arrow { "
        "    width: 12px; "
        "    height: 12px; "
        "}"
    );
    styleLayout->addWidget(m_themeCombo, row-1, 2);
    
    // 添加颜色方案选择
    QLabel *colorSchemeLabel = new QLabel("🎯 颜色方案:");
    colorSchemeLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    styleLayout->addWidget(colorSchemeLabel, row, 0);
    
    m_colorSchemeCombo = new QComboBox();
    m_colorSchemeCombo->addItem("🔵 蓝色系", "blue");
    m_colorSchemeCombo->addItem("🟢 绿色系", "green");
    m_colorSchemeCombo->addItem("🔴 红色系", "red");
    m_colorSchemeCombo->addItem("🟡 黄色系", "yellow");
    m_colorSchemeCombo->addItem("🟣 紫色系", "purple");
    m_colorSchemeCombo->addItem("🌈 渐变色", "gradient");
    m_colorSchemeCombo->setStyleSheet(m_themeCombo->styleSheet());
    styleLayout->addWidget(m_colorSchemeCombo, row, 1, 1, 2);
}

void ChartDialog::setupConnections()
{
    // 控制面板连接
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartDialog::onChartTypeChanged);
    
    connect(m_driveCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
            this, &ChartDialog::onDriveSelectionChanged);
    
    connect(m_timeRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartDialog::onTimeRangeChanged);
    
    connect(m_customRangeCheck, &QCheckBox::toggled,
            this, &ChartDialog::updateCustomTimeRangeState);
    
    connect(m_startTimeEdit, &QDateTimeEdit::dateTimeChanged,
            this, &ChartDialog::onCustomTimeRangeChanged);
    
    connect(m_endTimeEdit, &QDateTimeEdit::dateTimeChanged,
            this, &ChartDialog::onCustomTimeRangeChanged);
    
    // 按钮连接
    connect(m_refreshBtn, &QPushButton::clicked, this, &ChartDialog::onRefreshClicked);
    connect(m_exportBtn, &QPushButton::clicked, this, &ChartDialog::onExportClicked);
    connect(m_resetBtn, &QPushButton::clicked, this, &ChartDialog::onResetClicked);
    
    // 实时更新连接
    connect(m_realTimeCheck, &QCheckBox::toggled, this, &ChartDialog::onRealTimeToggled);
    connect(m_autoRefreshCheck, &QCheckBox::toggled, this, &ChartDialog::onAutoRefreshToggled);
    connect(m_refreshIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ChartDialog::onRefreshIntervalChanged);
    
    // 样式控制连接
    connect(m_showGridCheck, &QCheckBox::toggled, this, &ChartDialog::onStyleChanged);
    connect(m_showLegendCheck, &QCheckBox::toggled, this, &ChartDialog::onStyleChanged);
    connect(m_enableAnimationCheck, &QCheckBox::toggled, this, &ChartDialog::onStyleChanged);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &ChartDialog::onStyleChanged);
    
    // 自动刷新定时器
    connect(m_autoRefreshTimer, &QTimer::timeout, this, &ChartDialog::autoRefreshTimeout);
    
    // 图表管理器连接
    if (m_chartManager) {
        connect(m_chartManager, &ChartManager::chartCreated,
                this, &ChartDialog::onChartCreated);
        connect(m_chartManager, &ChartManager::chartUpdated,
                this, &ChartDialog::onChartUpdated);
        connect(m_chartManager, &ChartManager::chartExported,
                this, &ChartDialog::onChartExported);
        connect(m_chartManager, &ChartManager::errorOccurred,
                this, &ChartDialog::onChartError);
    }
}

void ChartDialog::setupStatusBar()
{
    // 这里可以添加状态栏，但QDialog默认没有状态栏
    // 如果需要，可以手动创建一个状态标签
}

void ChartDialog::populateChartTypes()
{
    if (!m_chartTypeCombo) return;
    
    m_chartTypeCombo->clear();
    m_chartTypeCombo->addItem(getChartTypeDisplayName(ChartType::RealTimeUsage), 
                             static_cast<int>(ChartType::RealTimeUsage));
    m_chartTypeCombo->addItem(getChartTypeDisplayName(ChartType::HistoryTrend), 
                             static_cast<int>(ChartType::HistoryTrend));
    m_chartTypeCombo->addItem(getChartTypeDisplayName(ChartType::MultiDriveCompare), 
                             static_cast<int>(ChartType::MultiDriveCompare));
    m_chartTypeCombo->addItem(getChartTypeDisplayName(ChartType::DataTransfer), 
                             static_cast<int>(ChartType::DataTransfer));
    m_chartTypeCombo->addItem(getChartTypeDisplayName(ChartType::ProcessActivity), 
                             static_cast<int>(ChartType::ProcessActivity));
}

void ChartDialog::populateDriveList()
{
    if (!m_driveCombo) return;
    
    m_driveCombo->clear();
    
    // 获取系统实际驱动器列表
    QList<QStorageInfo> storageList = QStorageInfo::mountedVolumes();
    QStringList drives;
    
    for (const QStorageInfo &storage : storageList) {
        if (storage.isValid() && storage.isReady()) {
            QString driveLetter = storage.rootPath();
            // 确保格式为 "C:" 形式
            if (driveLetter.endsWith("/") || driveLetter.endsWith("\\")) {
                driveLetter = driveLetter.left(driveLetter.length() - 1);
            }
            if (!driveLetter.isEmpty() && !drives.contains(driveLetter)) {
                drives.append(driveLetter);
            }
        }
    }
    
    // 排序驱动器列表
    drives.sort();
    
    for (const QString &drive : drives) {
        m_driveCombo->addItem(drive);
    }
    
    // 设置默认选择
    int index = m_driveCombo->findText(m_currentDriveLetter);
    if (index >= 0) {
        m_driveCombo->setCurrentIndex(index);
    }
}

void ChartDialog::populateTimeRanges()
{
    if (!m_timeRangeCombo) return;
    
    m_timeRangeCombo->clear();
    m_timeRangeCombo->addItem(getTimeRangeDisplayName(TimeRange::LastHour), 
                             static_cast<int>(TimeRange::LastHour));
    m_timeRangeCombo->addItem(getTimeRangeDisplayName(TimeRange::Last6Hours), 
                             static_cast<int>(TimeRange::Last6Hours));
    m_timeRangeCombo->addItem(getTimeRangeDisplayName(TimeRange::Last24Hours), 
                             static_cast<int>(TimeRange::Last24Hours));
    m_timeRangeCombo->addItem(getTimeRangeDisplayName(TimeRange::LastWeek), 
                             static_cast<int>(TimeRange::LastWeek));
    m_timeRangeCombo->addItem(getTimeRangeDisplayName(TimeRange::LastMonth), 
                             static_cast<int>(TimeRange::LastMonth));
}

void ChartDialog::showChart(ChartType type, const QString &driveLetter)
{
    m_currentChartType = type;
    if (!driveLetter.isEmpty()) {
        m_currentDriveLetter = driveLetter;
    }
    
    // 更新UI控件
    updateControlsState();
    
    // 更新图表显示
    updateChartDisplay();
    
    // 显示窗口
    show();
    raise();
    activateWindow();
    
    qDebug() << "[ChartDialog] 显示图表:" << static_cast<int>(type) << driveLetter;
}

void ChartDialog::refreshCharts()
{
    try {
        // 参数验证
        if (!m_chartManager) {
            qCritical() << "[ChartDialog] 刷新图表失败：图表管理器为空";
            return;
        }
        
        qDebug() << QString("[ChartDialog] 开始刷新所有图表 - 当前类型: %1, 驱动器: %2")
                    .arg(static_cast<int>(m_currentChartType)).arg(m_currentDriveLetter);
        
        // 更新状态栏显示刷新状态
        if (m_statusLabel) {
            m_statusLabel->setText("状态: 正在刷新图表...");
            m_statusLabel->setStyleSheet("color: blue;");
        }
        
        // 刷新所有图表
        m_chartManager->refreshAllCharts();
        
        // 更新状态栏
        updateStatusBar();
        
        qDebug() << "[ChartDialog] 图表刷新完成";
        
    } catch (const std::exception &e) {
        qCritical() << QString("[ChartDialog] 刷新图表失败：发生异常 - %1").arg(e.what());
        
        // 更新状态栏显示错误
        if (m_statusLabel) {
            m_statusLabel->setText(QString("状态: 刷新失败 - %1").arg(e.what()));
            m_statusLabel->setStyleSheet("color: red;");
        }
    } catch (...) {
        qCritical() << "[ChartDialog] 刷新图表失败：发生未知异常";
        
        // 更新状态栏显示错误
        if (m_statusLabel) {
            m_statusLabel->setText("状态: 刷新失败 - 未知错误");
            m_statusLabel->setStyleSheet("color: red;");
        }
    }
}

void ChartDialog::resetToDefaults()
{
    m_currentChartType = DEFAULT_CHART_TYPE;
    m_currentTimeRange = DEFAULT_TIME_RANGE;
    m_currentDriveLetter = "C:";
    m_refreshInterval = DEFAULT_REFRESH_INTERVAL;
    m_isRealTimeEnabled = false;
    
    updateControlsState();
    updateChartDisplay();
    
    qDebug() << "[ChartDialog] 重置为默认设置";
}

void ChartDialog::startAutoRefresh(int intervalSeconds)
{
    m_refreshInterval = intervalSeconds;
    m_autoRefreshTimer->start(intervalSeconds * 1000);
    
    if (m_statusLabel) {
        m_statusLabel->setText(QString("状态: 自动刷新中 (%1秒间隔)").arg(intervalSeconds));
    }
    
    qDebug() << "[ChartDialog] 开始自动刷新，间隔:" << intervalSeconds << "秒";
}

void ChartDialog::stopAutoRefresh()
{
    m_autoRefreshTimer->stop();
    
    if (m_statusLabel) {
        m_statusLabel->setText("状态: 就绪");
    }
    
    qDebug() << "[ChartDialog] 停止自动刷新";
}

bool ChartDialog::isAutoRefreshActive() const
{
    return m_autoRefreshTimer->isActive();
}

void ChartDialog::loadSettings()
{
    if (!m_settings) return;
    
    // 窗口几何
    restoreGeometry(m_settings->value("ChartDialog/geometry").toByteArray());
    
    // 图表设置
    m_currentChartType = static_cast<ChartType>(
        m_settings->value("ChartDialog/chartType", static_cast<int>(DEFAULT_CHART_TYPE)).toInt());
    m_currentDriveLetter = m_settings->value("ChartDialog/driveLetter", "C:").toString();
    m_currentTimeRange = static_cast<TimeRange>(
        m_settings->value("ChartDialog/timeRange", static_cast<int>(DEFAULT_TIME_RANGE)).toInt());
    
    // 实时更新设置
    m_isRealTimeEnabled = m_settings->value("ChartDialog/realTimeEnabled", false).toBool();
    m_refreshInterval = m_settings->value("ChartDialog/refreshInterval", DEFAULT_REFRESH_INTERVAL).toInt();
    bool autoRefreshEnabled = m_settings->value("ChartDialog/autoRefreshEnabled", true).toBool();
    
    // 样式设置
    bool showGrid = m_settings->value("ChartDialog/showGrid", true).toBool();
    bool showLegend = m_settings->value("ChartDialog/showLegend", true).toBool();
    bool enableAnimation = m_settings->value("ChartDialog/enableAnimation", true).toBool();
    int opacity = m_settings->value("ChartDialog/opacity", 100).toInt();
    
    // 应用设置到UI控件
    if (m_realTimeCheck) m_realTimeCheck->setChecked(m_isRealTimeEnabled);
    if (m_autoRefreshCheck) m_autoRefreshCheck->setChecked(autoRefreshEnabled);
    if (m_refreshIntervalSpin) m_refreshIntervalSpin->setValue(m_refreshInterval);
    if (m_showGridCheck) m_showGridCheck->setChecked(showGrid);
    if (m_showLegendCheck) m_showLegendCheck->setChecked(showLegend);
    if (m_enableAnimationCheck) m_enableAnimationCheck->setChecked(enableAnimation);
    if (m_opacitySlider) m_opacitySlider->setValue(opacity);
    
    qDebug() << "[ChartDialog] 加载设置完成";
}

void ChartDialog::saveSettings()
{
    if (!m_settings) return;
    
    // 窗口几何
    m_settings->setValue("ChartDialog/geometry", saveGeometry());
    
    // 图表设置
    m_settings->setValue("ChartDialog/chartType", static_cast<int>(m_currentChartType));
    m_settings->setValue("ChartDialog/driveLetter", m_currentDriveLetter);
    m_settings->setValue("ChartDialog/timeRange", static_cast<int>(m_currentTimeRange));
    
    // 实时更新设置
    m_settings->setValue("ChartDialog/realTimeEnabled", m_isRealTimeEnabled);
    m_settings->setValue("ChartDialog/refreshInterval", m_refreshInterval);
    m_settings->setValue("ChartDialog/autoRefreshEnabled", 
                        m_autoRefreshCheck ? m_autoRefreshCheck->isChecked() : true);
    
    // 样式设置
    m_settings->setValue("ChartDialog/showGrid", 
                        m_showGridCheck ? m_showGridCheck->isChecked() : true);
    m_settings->setValue("ChartDialog/showLegend", 
                        m_showLegendCheck ? m_showLegendCheck->isChecked() : true);
    m_settings->setValue("ChartDialog/enableAnimation", 
                        m_enableAnimationCheck ? m_enableAnimationCheck->isChecked() : true);
    m_settings->setValue("ChartDialog/opacity", 
                        m_opacitySlider ? m_opacitySlider->value() : 100);
    
    qDebug() << "[ChartDialog] 保存设置完成";
}

void ChartDialog::closeEvent(QCloseEvent *event)
{
    saveSettings();
    stopAutoRefresh();
    emit chartDialogClosed();
    event->accept();
}

void ChartDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    // 可以在这里处理窗口大小变化
}

void ChartDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    updateDriveList();
    updateChartDisplay();
}

// 槽函数实现
void ChartDialog::onRefreshClicked()
{
    refreshCharts();
}

void ChartDialog::onExportClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
                                                   "导出图表", 
                                                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                   "PNG图片 (*.png);;JPEG图片 (*.jpg);;PDF文档 (*.pdf)");
    
    if (!fileName.isEmpty() && m_chartManager) {
        bool success = m_chartManager->exportChart(m_currentChartType, fileName);
        if (success) {
            QMessageBox::information(this, "导出成功", "图表已成功导出到: " + fileName);
        } else {
            QMessageBox::warning(this, "导出失败", "图表导出失败，请检查文件路径和权限。");
        }
    }
}

void ChartDialog::onResetClicked()
{
    int ret = QMessageBox::question(this, "确认重置", 
                                   "确定要重置所有设置为默认值吗？",
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        resetToDefaults();
    }
}

void ChartDialog::onAutoRefreshToggled(bool enabled)
{
    if (enabled) {
        startAutoRefresh(m_refreshInterval);
    } else {
        stopAutoRefresh();
    }
}

void ChartDialog::onRefreshIntervalChanged(int seconds)
{
    m_refreshInterval = seconds;
    if (isAutoRefreshActive()) {
        startAutoRefresh(seconds);
    }
}

void ChartDialog::onChartTypeChanged(int index)
{
    if (m_chartTypeCombo) {
        ChartType type = static_cast<ChartType>(m_chartTypeCombo->itemData(index).toInt());
        m_currentChartType = type;
        updateChartDisplay();
        emit chartTypeSelected(type, m_currentDriveLetter);
    }
}

void ChartDialog::onDriveSelectionChanged(const QString &driveLetter)
{
    m_currentDriveLetter = driveLetter;
    updateChartDisplay();
}

void ChartDialog::onTimeRangeChanged(int index)
{
    if (m_timeRangeCombo) {
        TimeRange range = static_cast<TimeRange>(m_timeRangeCombo->itemData(index).toInt());
        m_currentTimeRange = range;
        updateChartDisplay();
    }
}

void ChartDialog::onCustomTimeRangeChanged()
{
    if (m_customRangeCheck && m_customRangeCheck->isChecked()) {
        validateTimeRange();
        updateChartDisplay();
    }
}

void ChartDialog::onRealTimeToggled(bool enabled)
{
    m_isRealTimeEnabled = enabled;
    updateRealTimeState();
    
    if (m_chartManager) {
        if (enabled) {
            m_chartManager->startRealTimeUpdate();
        } else {
            m_chartManager->stopRealTimeUpdate();
        }
    }
}

void ChartDialog::onStyleChanged()
{
    applyChartStyle();
}

void ChartDialog::autoRefreshTimeout()
{
    refreshCharts();
}

void ChartDialog::onChartCreated(ChartType type, QChartView *chartView)
{
    try {
        // 参数验证
        if (!chartView) {
            qCritical() << "[ChartDialog] 图表创建回调失败：图表视图为空";
            return;
        }
        
        if (!m_chartContainer) {
            qCritical() << "[ChartDialog] 图表创建回调失败：图表容器为空";
            return;
        }
        
        // 验证图表视图的有效性
        if (!chartView->chart()) {
            qCritical() << "[ChartDialog] 图表创建回调失败：图表对象为空";
            return;
        }
        
        qDebug() << QString("[ChartDialog] 开始处理图表创建回调 - 类型: %1").arg(static_cast<int>(type));
        
        // 获取或创建布局
        QLayout *layout = m_chartContainer->layout();
        if (!layout) {
            layout = new QVBoxLayout(m_chartContainer);
            if (!layout) {
                qCritical() << "[ChartDialog] 图表创建回调失败：无法创建布局";
                return;
            }
            m_chartContainer->setLayout(layout);
            qDebug() << "[ChartDialog] 已为图表容器创建新布局";
        }
        
        // 安全地清除旧的图表
        QList<QWidget*> oldWidgets;
        for (int i = 0; i < layout->count(); ++i) {
            QLayoutItem *item = layout->itemAt(i);
            if (item && item->widget()) {
                oldWidgets.append(item->widget());
            }
        }
        
        // 从布局中移除所有widget
        for (QWidget *widget : oldWidgets) {
            if (widget) {
                layout->removeWidget(widget);
                widget->setParent(nullptr);
                widget->deleteLater();
            }
        }
        
        if (!oldWidgets.isEmpty()) {
            qDebug() << QString("[ChartDialog] 已清理 %1 个旧图表组件").arg(oldWidgets.size());
        }
        
        // 添加新图表
        layout->addWidget(chartView);
        
        // 确保图表可见
        chartView->setVisible(true);
        m_chartContainer->setVisible(true);
        
        qDebug() << QString("[ChartDialog] 图表已成功添加到容器 - 类型: %1").arg(static_cast<int>(type));
        
        // 更新状态栏
        updateStatusBar();
        
    } catch (const std::bad_alloc &e) {
        qCritical() << QString("[ChartDialog] 图表创建回调失败：内存分配异常 - %1").arg(e.what());
    } catch (const std::exception &e) {
        qCritical() << QString("[ChartDialog] 图表创建回调失败：发生异常 - %1").arg(e.what());
    } catch (...) {
        qCritical() << "[ChartDialog] 图表创建回调失败：发生未知异常";
    }
}

void ChartDialog::onChartUpdated(ChartType type)
{
    updateStatusBar();
}

void ChartDialog::onChartExported(const QString &filePath)
{
    if (m_statusLabel) {
        m_statusLabel->setText(QString("状态: 图表已导出到 %1").arg(filePath));
    }
}

void ChartDialog::onChartError(const QString &error)
{
    try {
        // 记录错误信息
        qCritical() << QString("[ChartDialog] 图表错误回调：%1").arg(error);
        
        // 显示错误对话框
        if (this && this->isVisible()) {
            QMessageBox::warning(this, "图表错误", 
                QString("图表操作失败：\n%1\n\n请检查数据源或重试操作。").arg(error));
        }
        
        // 更新状态栏
        if (m_statusLabel) {
            m_statusLabel->setText(QString("状态: 错误 - %1").arg(error));
            m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
        }
        
        // 记录到调试输出
        qCritical() << "[ChartDialog] 图表错误:" << error;
        
    } catch (const std::exception &e) {
        qCritical() << "[ChartDialog] 处理图表错误时发生异常:" << e.what();
    } catch (...) {
        qCritical() << "[ChartDialog] 处理图表错误时发生未知异常";
    }
}

void ChartDialog::updateStatusBar()
{
    try {
        // 验证状态标签是否存在
        if (!m_statusLabel) {
            qDebug() << "[ChartDialog] 警告：状态标签为空，无法更新状态栏";
            return;
        }
        
        // 获取图表类型名称
        QString chartTypeName;
        try {
            chartTypeName = getChartTypeDisplayName(m_currentChartType);
        } catch (...) {
            chartTypeName = "未知类型";
            qDebug() << "[ChartDialog] 警告：获取图表类型名称失败，使用默认值";
        }
        
        // 构建状态文本
        QString statusText = QString("状态: 显示 %1 图表 - 驱动器 %2")
                            .arg(chartTypeName)
                            .arg(m_currentDriveLetter.isEmpty() ? "未选择" : m_currentDriveLetter);
        
        // 更新状态标签
        m_statusLabel->setText(statusText);
        m_statusLabel->setStyleSheet("color: green;");
        
        qDebug() << QString("[ChartDialog] 状态栏已更新：%1").arg(statusText);
        
    } catch (const std::exception &e) {
        qCritical() << QString("[ChartDialog] 更新状态栏失败：发生异常 - %1").arg(e.what());
        
        // 设置错误状态
        if (m_statusLabel) {
            m_statusLabel->setText("状态: 更新失败");
            m_statusLabel->setStyleSheet("color: red;");
        }
    } catch (...) {
        qCritical() << "[ChartDialog] 更新状态栏失败：发生未知异常";
        
        // 设置错误状态
        if (m_statusLabel) {
            m_statusLabel->setText("状态: 更新失败");
            m_statusLabel->setStyleSheet("color: red;");
        }
    }
}

void ChartDialog::updateDriveList()
{
    try {
        // 验证驱动器下拉框是否存在
        if (!m_driveCombo) {
            qDebug() << "[ChartDialog] 警告：驱动器下拉框为空，无法更新驱动器列表";
            return;
        }
        
        qDebug() << "[ChartDialog] 开始更新驱动器列表";
        
        // 使用现有的populateDriveList方法
        populateDriveList();
        
        qDebug() << "[ChartDialog] 驱动器列表更新完成";
        
    } catch (const std::exception &e) {
        qCritical() << QString("[ChartDialog] 更新驱动器列表失败：发生异常 - %1").arg(e.what());
        
        // 设置错误状态
        if (m_driveCombo) {
            m_driveCombo->clear();
            m_driveCombo->addItem("更新失败");
        }
    } catch (...) {
        qCritical() << "[ChartDialog] 更新驱动器列表失败：发生未知异常";
        
        // 设置错误状态
        if (m_driveCombo) {
            m_driveCombo->clear();
            m_driveCombo->addItem("更新失败");
        }
    }
}

void ChartDialog::updateControlsState()
{
    try {
        qDebug() << QString("[ChartDialog] 开始更新控件状态 - 图表类型: %1, 驱动器: %2")
                    .arg(static_cast<int>(m_currentChartType)).arg(m_currentDriveLetter);
        
        // 更新图表类型选择
        if (m_chartTypeCombo) {
            try {
                int index = -1;
                for (int i = 0; i < m_chartTypeCombo->count(); ++i) {
                    if (static_cast<ChartType>(m_chartTypeCombo->itemData(i).toInt()) == m_currentChartType) {
                        index = i;
                        break;
                    }
                }
                if (index >= 0) {
                    m_chartTypeCombo->setCurrentIndex(index);
                    qDebug() << QString("[ChartDialog] 图表类型下拉框已更新到索引 %1").arg(index);
                } else {
                    qDebug() << QString("[ChartDialog] 警告：图表类型 %1 在下拉框中未找到")
                                .arg(static_cast<int>(m_currentChartType));
                }
            } catch (const std::exception &e) {
                qCritical() << QString("[ChartDialog] 更新图表类型下拉框失败：%1").arg(e.what());
            }
        } else {
            qDebug() << "[ChartDialog] 警告：图表类型下拉框为空";
        }
        
        // 更新驱动器选择
        if (m_driveCombo) {
            try {
                int index = m_driveCombo->findText(m_currentDriveLetter);
                if (index >= 0) {
                    m_driveCombo->setCurrentIndex(index);
                    qDebug() << QString("[ChartDialog] 驱动器下拉框已更新到索引 %1").arg(index);
                } else {
                    qDebug() << QString("[ChartDialog] 警告：驱动器 %1 在下拉框中未找到")
                                .arg(m_currentDriveLetter);
                }
            } catch (const std::exception &e) {
                qCritical() << QString("[ChartDialog] 更新驱动器下拉框失败：%1").arg(e.what());
            }
        } else {
            qDebug() << "[ChartDialog] 警告：驱动器下拉框为空";
        }
        
        // 更新时间范围选择
        if (m_timeRangeCombo) {
            try {
                int index = -1;
                for (int i = 0; i < m_timeRangeCombo->count(); ++i) {
                    if (static_cast<TimeRange>(m_timeRangeCombo->itemData(i).toInt()) == m_currentTimeRange) {
                        index = i;
                        break;
                    }
                }
                if (index >= 0) {
                    m_timeRangeCombo->setCurrentIndex(index);
                    qDebug() << QString("[ChartDialog] 时间范围下拉框已更新到索引 %1").arg(index);
                } else {
                    qDebug() << QString("[ChartDialog] 警告：时间范围 %1 在下拉框中未找到")
                                .arg(static_cast<int>(m_currentTimeRange));
                }
            } catch (const std::exception &e) {
                qCritical() << QString("[ChartDialog] 更新时间范围下拉框失败：%1").arg(e.what());
            }
        } else {
            qDebug() << "[ChartDialog] 警告：时间范围下拉框为空";
        }
        
        updateCustomTimeRangeState();
        updateRealTimeState();
        
        qDebug() << "[ChartDialog] 控件状态更新完成";
        
    } catch (const std::exception &e) {
        qCritical() << QString("[ChartDialog] 更新控件状态失败：发生异常 - %1").arg(e.what());
    } catch (...) {
        qCritical() << "[ChartDialog] 更新控件状态失败：发生未知异常";
    }
}

void ChartDialog::updateCustomTimeRangeState()
{
    bool customEnabled = m_customRangeCheck && m_customRangeCheck->isChecked();
    
    if (m_startTimeEdit) m_startTimeEdit->setEnabled(customEnabled);
    if (m_endTimeEdit) m_endTimeEdit->setEnabled(customEnabled);
    if (m_timeRangeCombo) m_timeRangeCombo->setEnabled(!customEnabled);
}

void ChartDialog::updateRealTimeState()
{
    // 根据实时模式状态更新相关控件
    if (m_refreshIntervalSpin) {
        m_refreshIntervalSpin->setEnabled(m_isRealTimeEnabled);
    }
}

void ChartDialog::updateChartDisplay()
{
    if (!m_chartManager) {
        qDebug() << "[ChartDialog] 警告: ChartManager未初始化";
        return;
    }
    
    try {
        // 创建新图表
        QChartView* chartView = m_chartManager->createChart(m_currentChartType, m_currentDriveLetter);
        
        if (chartView) {
            // 触发onChartCreated来将图表添加到容器中
            onChartCreated(m_currentChartType, chartView);
            qDebug() << "[ChartDialog] 图表显示已更新";
        } else {
            qDebug() << "[ChartDialog] 警告: 图表创建失败";
            
            // 更新状态栏显示错误
            if (m_statusLabel) {
                m_statusLabel->setText("状态: 图表创建失败");
                m_statusLabel->setStyleSheet("color: red;");
            }
        }
    } catch (const std::exception &e) {
        qDebug() << "[ChartDialog] 更新图表显示时发生异常:" << e.what();
        
        // 更新状态栏显示错误
        if (m_statusLabel) {
            m_statusLabel->setText(QString("状态: 更新失败 - %1").arg(e.what()));
            m_statusLabel->setStyleSheet("color: red;");
        }
    }
}

void ChartDialog::applyChartStyle()
{
    if (!m_chartManager) return;
    
    // 创建样式配置
    ChartStyle style;
    style.showGrid = m_showGridCheck ? m_showGridCheck->isChecked() : true;
    style.showLegend = m_showLegendCheck ? m_showLegendCheck->isChecked() : true;
    style.enableAnimation = m_enableAnimationCheck ? m_enableAnimationCheck->isChecked() : true;
    
    // 应用样式
    m_chartManager->setChartStyle(style);
    
    // 刷新图表以应用新样式
    refreshCharts();
}

void ChartDialog::validateTimeRange()
{
    if (m_startTimeEdit && m_endTimeEdit) {
        QDateTime start = m_startTimeEdit->dateTime();
        QDateTime end = m_endTimeEdit->dateTime();
        
        if (start >= end) {
            // 自动调整结束时间
            m_endTimeEdit->setDateTime(start.addSecs(3600)); // 添加1小时（3600秒）
        }
    }
}

QString ChartDialog::getChartTypeDisplayName(ChartType type)
{
    switch (type) {
    case ChartType::RealTimeUsage:
        return "实时使用率";
    case ChartType::HistoryTrend:
        return "历史趋势";
    case ChartType::MultiDriveCompare:
        return "多驱动器对比";
    case ChartType::DataTransfer:
        return "数据传输量";
    case ChartType::ProcessActivity:
        return "进程活动";
    default:
        return "未知类型";
    }
}

QString ChartDialog::getTimeRangeDisplayName(TimeRange range)
{
    switch (range) {
    case TimeRange::LastHour:
        return "最近1小时";
    case TimeRange::Last6Hours:
        return "最近6小时";
    case TimeRange::Last24Hours:
        return "最近24小时";
    case TimeRange::LastWeek:
        return "最近一周";
    case TimeRange::LastMonth:
        return "最近一月";
    case TimeRange::Custom:
        return "自定义";
    default:
        return "未知范围";
    }
}

void ChartDialog::logError(const QString &message)
{
    try {
        // 输出到调试控制台
        qCritical() << "[ChartDialog] 错误:" << message;
        
        // 发出错误信号（如果需要的话）
        // emit errorOccurred(message);
        
        // TODO: 可以添加文件日志记录
        // 例如：写入到日志文件
        
    } catch (...) {
        // 避免在错误处理中再次抛出异常
        qCritical() << "[ChartDialog] logError函数内部发生异常";
    }
}

QIcon ChartDialog::getChartTypeIcon(ChartType type)
{
    Q_UNUSED(type); // 标记参数未使用，避免编译警告
    // 这里可以为不同的图表类型返回不同的图标
    // 目前返回默认图标
    return style()->standardIcon(QStyle::SP_ComputerIcon);
}
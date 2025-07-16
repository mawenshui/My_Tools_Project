#include "chart_selection_dialog.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtGui/QFont>
#include <QtGui/QPalette>

ChartSelectionDialog::ChartSelectionDialog(QWidget *parent)
    : QDialog(parent)
    , m_settings(nullptr)
{
    setWindowTitle(tr("选择要生成的图表"));
    setModal(true);
    resize(600, 500);
    
    // 初始化设置对象
    m_settings = new QSettings(this);
    
    initializeUI();
    loadUserPreferences();
    updateSelectionStatus();
}

ChartSelectionDialog::~ChartSelectionDialog()
{
}

void ChartSelectionDialog::initializeUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(15);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 创建标题和说明
    m_titleLabel = new QLabel(tr("图表生成选择"), this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("color: #2c3e50; margin: 10px 0;");
    
    m_descriptionLabel = new QLabel(
        tr("请选择在代码统计过程中要生成的图表类型。\n"
           "选中的图表将在统计完成后自动生成并缓存，提高后续访问速度。"), this);
    m_descriptionLabel->setWordWrap(true);
    m_descriptionLabel->setAlignment(Qt::AlignCenter);
    m_descriptionLabel->setStyleSheet("color: #7f8c8d; margin-bottom: 15px;");
    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #27ae60; font-weight: bold;");
    
    // 创建滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    
    m_contentWidget = new QWidget();
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setSpacing(10);
    
    // 创建图表选择区域
    QWidget *chartSelectionArea = createChartSelectionArea();
    m_contentLayout->addWidget(chartSelectionArea);
    
    m_scrollArea->setWidget(m_contentWidget);
    
    // 创建按钮区域
    QWidget *buttonArea = createButtonArea();
    
    // 添加到主布局
    m_mainLayout->addWidget(m_titleLabel);
    m_mainLayout->addWidget(m_descriptionLabel);
    m_mainLayout->addWidget(m_statusLabel);
    m_mainLayout->addWidget(m_scrollArea, 1);
    m_mainLayout->addWidget(buttonArea);
    
    // 连接信号
    connect(m_selectAllButton, &QPushButton::clicked, this, &ChartSelectionDialog::onSelectAllClicked);
    connect(m_selectNoneButton, &QPushButton::clicked, this, &ChartSelectionDialog::onSelectNoneClicked);
    connect(m_recommendedButton, &QPushButton::clicked, this, &ChartSelectionDialog::onRecommendedClicked);
    connect(m_okButton, &QPushButton::clicked, this, &ChartSelectionDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &ChartSelectionDialog::onCancelClicked);
    
    // 连接图表选择信号
    connect(m_languagePieChartCheck, &QCheckBox::toggled, this, &ChartSelectionDialog::onChartSelectionChanged);
    connect(m_fileTypeBarChartCheck, &QCheckBox::toggled, this, &ChartSelectionDialog::onChartSelectionChanged);
    connect(m_codeLinesBarChartCheck, &QCheckBox::toggled, this, &ChartSelectionDialog::onChartSelectionChanged);
    connect(m_fileSizeScatterChartCheck, &QCheckBox::toggled, this, &ChartSelectionDialog::onChartSelectionChanged);
    connect(m_complexityLineChartCheck, &QCheckBox::toggled, this, &ChartSelectionDialog::onChartSelectionChanged);
    connect(m_languageAreaChartCheck, &QCheckBox::toggled, this, &ChartSelectionDialog::onChartSelectionChanged);
}

QWidget* ChartSelectionDialog::createChartSelectionArea()
{
    m_chartSelectionGroup = new QGroupBox(tr("可用图表类型"), this);
    m_chartSelectionLayout = new QGridLayout(m_chartSelectionGroup);
    m_chartSelectionLayout->setSpacing(15);
    
    // 创建图表复选框
    m_languagePieChartCheck = new QCheckBox(tr("语言分布饼图"), m_chartSelectionGroup);
    m_fileTypeBarChartCheck = new QCheckBox(tr("文件类型柱状图"), m_chartSelectionGroup);
    m_codeLinesBarChartCheck = new QCheckBox(tr("代码行数柱状图"), m_chartSelectionGroup);
    m_fileSizeScatterChartCheck = new QCheckBox(tr("文件大小散点图"), m_chartSelectionGroup);
    m_complexityLineChartCheck = new QCheckBox(tr("复杂度趋势图"), m_chartSelectionGroup);
    m_languageAreaChartCheck = new QCheckBox(tr("语言占比面积图"), m_chartSelectionGroup);
    
    // 创建图表卡片
    QWidget *pieChartCard = createChartCard(
        tr("语言分布饼图"),
        tr("显示项目中各编程语言的代码行数分布，直观展示语言使用比例"),
        m_languagePieChartCheck,
        true
    );
    
    QWidget *fileTypeBarCard = createChartCard(
        tr("文件类型柱状图"),
        tr("按文件类型统计文件数量，帮助了解项目文件结构"),
        m_fileTypeBarChartCheck,
        true
    );
    
    QWidget *codeLinesBarCard = createChartCard(
        tr("代码行数柱状图"),
        tr("显示各编程语言的代码行数对比，便于分析代码规模"),
        m_codeLinesBarChartCheck,
        true
    );
    
    QWidget *fileSizeScatterCard = createChartCard(
        tr("文件大小散点图"),
        tr("展示文件大小与代码行数的关系，识别异常大小的文件"),
        m_fileSizeScatterChartCheck,
        false
    );
    
    QWidget *complexityLineCard = createChartCard(
        tr("复杂度趋势图"),
        tr("显示代码复杂度随时间的变化趋势（需要历史数据）"),
        m_complexityLineChartCheck,
        false
    );
    
    QWidget *languageAreaCard = createChartCard(
        tr("语言占比面积图"),
        tr("以面积图形式展示各语言的累积占比变化"),
        m_languageAreaChartCheck,
        false
    );
    
    // 添加到布局（2列布局）
    m_chartSelectionLayout->addWidget(pieChartCard, 0, 0);
    m_chartSelectionLayout->addWidget(fileTypeBarCard, 0, 1);
    m_chartSelectionLayout->addWidget(codeLinesBarCard, 1, 0);
    m_chartSelectionLayout->addWidget(fileSizeScatterCard, 1, 1);
    m_chartSelectionLayout->addWidget(complexityLineCard, 2, 0);
    m_chartSelectionLayout->addWidget(languageAreaCard, 2, 1);
    
    return m_chartSelectionGroup;
}

QWidget* ChartSelectionDialog::createButtonArea()
{
    QWidget *buttonArea = new QWidget(this);
    QVBoxLayout *buttonAreaLayout = new QVBoxLayout(buttonArea);
    buttonAreaLayout->setSpacing(10);
    
    // 快捷按钮
    m_quickButtonLayout = new QHBoxLayout();
    
    m_selectAllButton = new QPushButton(tr("全选"), buttonArea);
    m_selectNoneButton = new QPushButton(tr("全不选"), buttonArea);
    m_recommendedButton = new QPushButton(tr("推荐选择"), buttonArea);
    
    m_selectAllButton->setMinimumSize(80, 30);
    m_selectNoneButton->setMinimumSize(80, 30);
    m_recommendedButton->setMinimumSize(100, 30);
    
    m_recommendedButton->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #2980b9; }"
        "QPushButton:pressed { background-color: #21618c; }"
    );
    
    m_quickButtonLayout->addWidget(m_selectAllButton);
    m_quickButtonLayout->addWidget(m_selectNoneButton);
    m_quickButtonLayout->addStretch();
    m_quickButtonLayout->addWidget(m_recommendedButton);
    
    // 对话框按钮
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch();
    
    m_okButton = new QPushButton(tr("确定"), buttonArea);
    m_cancelButton = new QPushButton(tr("取消"), buttonArea);
    
    m_okButton->setDefault(true);
    m_okButton->setMinimumSize(100, 35);
    m_cancelButton->setMinimumSize(100, 35);
    
    m_okButton->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #229954; }"
        "QPushButton:pressed { background-color: #1e8449; }"
        "QPushButton:disabled { background-color: #95a5a6; }"
    );
    
    m_buttonLayout->addWidget(m_okButton);
    m_buttonLayout->addWidget(m_cancelButton);
    
    buttonAreaLayout->addLayout(m_quickButtonLayout);
    buttonAreaLayout->addLayout(m_buttonLayout);
    
    return buttonArea;
}

QWidget* ChartSelectionDialog::createChartCard(const QString &title, const QString &description, 
                                              QCheckBox *checkBox, bool recommended)
{
    QWidget *card = new QWidget(this);
    card->setFixedHeight(120);
    
    QString cardStyle = QString(
        "QWidget { "
        "    background-color: %1; "
        "    border: 2px solid %2; "
        "    border-radius: 8px; "
        "    padding: 10px; "
        "} "
        "QWidget:hover { "
        "    border-color: %3; "
        "    background-color: %4; "
        "}"
    ).arg(recommended ? "#f8f9fa" : "#ffffff")
     .arg(recommended ? "#3498db" : "#e0e0e0")
     .arg("#3498db")
     .arg(recommended ? "#e8f4f8" : "#f8f9fa");
    
    card->setStyleSheet(cardStyle);
    
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(5);
    cardLayout->setContentsMargins(10, 10, 10, 10);
    
    // 标题行
    QHBoxLayout *titleLayout = new QHBoxLayout();
    
    checkBox->setStyleSheet("QCheckBox { font-weight: bold; color: #2c3e50; }");
    titleLayout->addWidget(checkBox);
    
    if (recommended) {
        QLabel *recommendedLabel = new QLabel(tr("推荐"), card);
        recommendedLabel->setStyleSheet(
            "QLabel { "
            "    background-color: #e74c3c; "
            "    color: white; "
            "    padding: 2px 6px; "
            "    border-radius: 3px; "
            "    font-size: 10px; "
            "    font-weight: bold; "
            "}"
        );
        recommendedLabel->setFixedSize(40, 20);
        recommendedLabel->setAlignment(Qt::AlignCenter);
        titleLayout->addWidget(recommendedLabel);
    }
    
    titleLayout->addStretch();
    
    // 描述
    QLabel *descLabel = new QLabel(description, card);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("QLabel { color: #7f8c8d; font-size: 11px; }");
    
    cardLayout->addLayout(titleLayout);
    cardLayout->addWidget(descLabel);
    cardLayout->addStretch();
    
    return card;
}

void ChartSelectionDialog::updateSelectionStatus()
{
    int selectedCount = 0;
    if (m_languagePieChartCheck->isChecked()) selectedCount++;
    if (m_fileTypeBarChartCheck->isChecked()) selectedCount++;
    if (m_codeLinesBarChartCheck->isChecked()) selectedCount++;
    if (m_fileSizeScatterChartCheck->isChecked()) selectedCount++;
    if (m_complexityLineChartCheck->isChecked()) selectedCount++;
    if (m_languageAreaChartCheck->isChecked()) selectedCount++;
    
    if (selectedCount == 0) {
        m_statusLabel->setText(tr("⚠️ 请至少选择一种图表类型"));
        m_statusLabel->setStyleSheet("color: #e74c3c; font-weight: bold;");
        m_okButton->setEnabled(false);
    } else {
        m_statusLabel->setText(tr("✓ 已选择 %1 种图表类型").arg(selectedCount));
        m_statusLabel->setStyleSheet("color: #27ae60; font-weight: bold;");
        m_okButton->setEnabled(true);
    }
}

bool ChartSelectionDialog::validateSelection()
{
    ChartSelectionOptions options = getChartSelectionOptions();
    return options.hasAnyChartSelected();
}

ChartSelectionDialog::ChartSelectionOptions ChartSelectionDialog::getChartSelectionOptions() const
{
    ChartSelectionOptions options;
    options.generateLanguagePieChart = m_languagePieChartCheck->isChecked();
    options.generateFileTypeBarChart = m_fileTypeBarChartCheck->isChecked();
    options.generateCodeLinesBarChart = m_codeLinesBarChartCheck->isChecked();
    options.generateFileSizeScatterChart = m_fileSizeScatterChartCheck->isChecked();
    options.generateComplexityLineChart = m_complexityLineChartCheck->isChecked();
    options.generateLanguageAreaChart = m_languageAreaChartCheck->isChecked();
    return options;
}

void ChartSelectionDialog::setChartSelectionOptions(const ChartSelectionOptions &options)
{
    m_options = options;
    m_languagePieChartCheck->setChecked(options.generateLanguagePieChart);
    m_fileTypeBarChartCheck->setChecked(options.generateFileTypeBarChart);
    m_codeLinesBarChartCheck->setChecked(options.generateCodeLinesBarChart);
    m_fileSizeScatterChartCheck->setChecked(options.generateFileSizeScatterChart);
    m_complexityLineChartCheck->setChecked(options.generateComplexityLineChart);
    m_languageAreaChartCheck->setChecked(options.generateLanguageAreaChart);
    updateSelectionStatus();
}

void ChartSelectionDialog::loadUserPreferences()
{
    if (!m_settings) return;
    
    m_settings->beginGroup("ChartSelection");
    
    ChartSelectionOptions options;
    options.generateLanguagePieChart = m_settings->value("LanguagePieChart", true).toBool();
    options.generateFileTypeBarChart = m_settings->value("FileTypeBarChart", true).toBool();
    options.generateCodeLinesBarChart = m_settings->value("CodeLinesBarChart", true).toBool();
    options.generateFileSizeScatterChart = m_settings->value("FileSizeScatterChart", false).toBool();
    options.generateComplexityLineChart = m_settings->value("ComplexityLineChart", false).toBool();
    options.generateLanguageAreaChart = m_settings->value("LanguageAreaChart", false).toBool();
    
    m_settings->endGroup();
    
    setChartSelectionOptions(options);
}

void ChartSelectionDialog::saveUserPreferences()
{
    if (!m_settings) return;
    
    ChartSelectionOptions options = getChartSelectionOptions();
    
    m_settings->beginGroup("ChartSelection");
    
    m_settings->setValue("LanguagePieChart", options.generateLanguagePieChart);
    m_settings->setValue("FileTypeBarChart", options.generateFileTypeBarChart);
    m_settings->setValue("CodeLinesBarChart", options.generateCodeLinesBarChart);
    m_settings->setValue("FileSizeScatterChart", options.generateFileSizeScatterChart);
    m_settings->setValue("ComplexityLineChart", options.generateComplexityLineChart);
    m_settings->setValue("LanguageAreaChart", options.generateLanguageAreaChart);
    
    m_settings->endGroup();
    m_settings->sync();
}

void ChartSelectionDialog::accept()
{
    if (validateSelection()) {
        saveUserPreferences();
        QDialog::accept();
    } else {
        QMessageBox::warning(this, tr("选择无效"), 
                           tr("请至少选择一种图表类型。"));
    }
}

void ChartSelectionDialog::reject()
{
    QDialog::reject();
}

void ChartSelectionDialog::onChartSelectionChanged()
{
    updateSelectionStatus();
}

void ChartSelectionDialog::onSelectAllClicked()
{
    m_languagePieChartCheck->setChecked(true);
    m_fileTypeBarChartCheck->setChecked(true);
    m_codeLinesBarChartCheck->setChecked(true);
    m_fileSizeScatterChartCheck->setChecked(true);
    m_complexityLineChartCheck->setChecked(true);
    m_languageAreaChartCheck->setChecked(true);
}

void ChartSelectionDialog::onSelectNoneClicked()
{
    m_languagePieChartCheck->setChecked(false);
    m_fileTypeBarChartCheck->setChecked(false);
    m_codeLinesBarChartCheck->setChecked(false);
    m_fileSizeScatterChartCheck->setChecked(false);
    m_complexityLineChartCheck->setChecked(false);
    m_languageAreaChartCheck->setChecked(false);
}

void ChartSelectionDialog::onRecommendedClicked()
{
    m_languagePieChartCheck->setChecked(true);
    m_fileTypeBarChartCheck->setChecked(true);
    m_codeLinesBarChartCheck->setChecked(true);
    m_fileSizeScatterChartCheck->setChecked(false);
    m_complexityLineChartCheck->setChecked(false);
    m_languageAreaChartCheck->setChecked(false);
}

void ChartSelectionDialog::onOkClicked()
{
    accept();
}

void ChartSelectionDialog::onCancelClicked()
{
    reject();
}
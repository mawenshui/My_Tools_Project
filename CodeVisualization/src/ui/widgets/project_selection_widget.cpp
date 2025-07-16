#include "project_selection_widget.h"
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>

ProjectSelectionWidget::ProjectSelectionWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_projectGroup(nullptr)
    , m_projectPathEdit(nullptr)
    , m_browseButton(nullptr)
    , m_recentProjectsCombo(nullptr)
    , m_configGroup(nullptr)
    , m_includeCommentsCheck(nullptr)
    , m_includeBlankLinesCheck(nullptr)
    , m_excludeGroup(nullptr)
    , m_excludeList(nullptr)
    , m_excludePatternEdit(nullptr)
    , m_addPatternButton(nullptr)
    , m_removePatternButton(nullptr)
    , m_resetPatternsButton(nullptr)
    , m_startAnalysisButton(nullptr)
    , m_resetConfigButton(nullptr)
    , m_statusLabel(nullptr)
    , m_configManager(ConfigManager::instance())
{
    initializeUI();
    loadRecentProjects();
    loadDefaultExcludePatterns();
    updateUIState();
    
    // 连接配置管理器信号
    connect(m_configManager, &ConfigManager::recentProjectsChanged,
            this, &ProjectSelectionWidget::loadRecentProjects);
}

ProjectSelectionWidget::~ProjectSelectionWidget()
{
    // ConfigManager会自动保存配置，无需手动调用
}

void ProjectSelectionWidget::initializeUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // 创建各个区域
    m_mainLayout->addWidget(createProjectSelectionArea());
    m_mainLayout->addWidget(createAnalysisConfigArea());
    m_mainLayout->addWidget(createExcludeRulesArea());
    m_mainLayout->addWidget(createActionButtonsArea());
    
    // 状态标签
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    m_statusLabel->setText(tr("准备分析项目"));
    m_mainLayout->addWidget(m_statusLabel);
    
    m_mainLayout->addStretch();
    
    setLayout(m_mainLayout);
}

QWidget* ProjectSelectionWidget::createProjectSelectionArea()
{
    m_projectGroup = new QGroupBox(tr("项目选择"), this);
    QVBoxLayout *groupLayout = new QVBoxLayout(m_projectGroup);
    
    // 项目路径输入
    QHBoxLayout *pathLayout = new QHBoxLayout();
    QLabel *pathLabel = new QLabel(tr("项目路径:"), this);
    m_projectPathEdit = new QLineEdit(this);
    m_projectPathEdit->setPlaceholderText(tr("选择或输入项目目录路径..."));
    m_browseButton = new QPushButton(tr("浏览..."), this);
    m_browseButton->setFixedWidth(80);
    
    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(m_projectPathEdit, 1);
    pathLayout->addWidget(m_browseButton);
    
    // 最近项目
    QHBoxLayout *recentLayout = new QHBoxLayout();
    QLabel *recentLabel = new QLabel(tr("最近项目:"), this);
    m_recentProjectsCombo = new QComboBox(this);
    m_recentProjectsCombo->setEditable(false);
    m_recentProjectsCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_recentProjectsCombo->setToolTip(tr("选择最近使用的项目"));
    
    // 清空最近项目按钮
    QPushButton *clearRecentButton = new QPushButton(tr("清空"), this);
    clearRecentButton->setToolTip(tr("清空最近项目列表"));
    clearRecentButton->setMaximumWidth(60);
    
    recentLayout->addWidget(recentLabel);
    recentLayout->addWidget(m_recentProjectsCombo, 1);
    recentLayout->addWidget(clearRecentButton);
    
    // 连接清空按钮信号
    connect(clearRecentButton, &QPushButton::clicked, [this]() {
        if (QMessageBox::question(this, tr("确认清空"), 
                                tr("确定要清空最近项目列表吗？\n这将清空配置文件中的最近项目记录。"),
                                QMessageBox::Yes | QMessageBox::No,
                                QMessageBox::No) == QMessageBox::Yes) {
            m_configManager->clearRecentProjects();
        }
    });
    
    groupLayout->addLayout(pathLayout);
    groupLayout->addLayout(recentLayout);
    
    // 连接信号
    connect(m_browseButton, &QPushButton::clicked, this, &ProjectSelectionWidget::browseProjectFolder);
    connect(m_projectPathEdit, &QLineEdit::textChanged, this, &ProjectSelectionWidget::onProjectPathChanged);
    connect(m_recentProjectsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                QStringList recentProjects = m_configManager->getRecentProjects();
                if (index >= 0 && index < recentProjects.size()) {
                    QString selectedPath = recentProjects[index];
                    m_projectPathEdit->setText(selectedPath);
                    updateUIState();
                    
                    // 显示项目信息
                    QFileInfo info(selectedPath);
                    if (info.exists()) {
                        m_statusLabel->setText(tr("已选择项目: %1").arg(info.baseName()));
                    } else {
                        m_statusLabel->setText(tr("警告: 项目路径不存在"));
                        m_statusLabel->setStyleSheet("color: orange;");
                        QTimer::singleShot(3000, [this]() {
                            m_statusLabel->setText(tr("请选择项目目录"));
                            m_statusLabel->setStyleSheet("");
                        });
                    }
                }
            });
    
    // 双击最近项目快速开始分析
    connect(m_recentProjectsCombo, QOverload<int>::of(&QComboBox::activated),
            [this](int index) {
                QStringList recentProjects = m_configManager->getRecentProjects();
                if (index >= 0 && index < recentProjects.size()) {
                    QString path = recentProjects[index];
                    if (QDir(path).exists()) {
                        m_projectPathEdit->setText(path);
                        updateUIState();
                        // 自动开始分析
                        QTimer::singleShot(100, [this, path]() {
                            if (m_startAnalysisButton->isEnabled()) {
                                emit startAnalysis(path);
                            }
                        });
                    }
                }
            });
    
    return m_projectGroup;
}

QWidget* ProjectSelectionWidget::createAnalysisConfigArea()
{
    m_configGroup = new QGroupBox(tr("分析配置"), this);
    QVBoxLayout *groupLayout = new QVBoxLayout(m_configGroup);
    
    // 配置选项
    m_includeCommentsCheck = new QCheckBox(tr("在统计中包含注释行"), this);
    m_includeCommentsCheck->setChecked(true);
    
    m_includeBlankLinesCheck = new QCheckBox(tr("在统计中包含空白行"), this);
    m_includeBlankLinesCheck->setChecked(true);
    
    groupLayout->addWidget(m_includeCommentsCheck);
    groupLayout->addWidget(m_includeBlankLinesCheck);
    
    // 连接信号
    connect(m_includeCommentsCheck, &QCheckBox::toggled, this, &ProjectSelectionWidget::onConfigurationOptionChanged);
    connect(m_includeBlankLinesCheck, &QCheckBox::toggled, this, &ProjectSelectionWidget::onConfigurationOptionChanged);
    
    return m_configGroup;
}

QWidget* ProjectSelectionWidget::createExcludeRulesArea()
{
    m_excludeGroup = new QGroupBox(tr("排除规则"), this);
    QVBoxLayout *groupLayout = new QVBoxLayout(m_excludeGroup);
    
    // 排除规则列表
    m_excludeList = new QListWidget(this);
    m_excludeList->setMaximumHeight(120);
    m_excludeList->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // 添加规则输入
    QHBoxLayout *addLayout = new QHBoxLayout();
    m_excludePatternEdit = new QLineEdit(this);
    m_excludePatternEdit->setPlaceholderText(tr("输入排除模式 (例如: *.tmp, *build*, 等)"));
    m_addPatternButton = new QPushButton(tr("添加"), this);
    m_addPatternButton->setFixedWidth(60);
    
    addLayout->addWidget(m_excludePatternEdit, 1);
    addLayout->addWidget(m_addPatternButton);
    
    // 操作按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_removePatternButton = new QPushButton(tr("删除选中"), this);
    m_resetPatternsButton = new QPushButton(tr("重置为默认"), this);
    
    buttonLayout->addWidget(m_removePatternButton);
    buttonLayout->addWidget(m_resetPatternsButton);
    buttonLayout->addStretch();
    
    groupLayout->addWidget(m_excludeList);
    groupLayout->addLayout(addLayout);
    groupLayout->addLayout(buttonLayout);
    
    // 连接信号
    connect(m_addPatternButton, &QPushButton::clicked, this, &ProjectSelectionWidget::addExcludePattern);
    connect(m_removePatternButton, &QPushButton::clicked, this, &ProjectSelectionWidget::removeExcludePattern);
    connect(m_resetPatternsButton, &QPushButton::clicked, this, &ProjectSelectionWidget::resetExcludePatterns);
    connect(m_excludePatternEdit, &QLineEdit::returnPressed, this, &ProjectSelectionWidget::addExcludePattern);
    
    return m_excludeGroup;
}

QWidget* ProjectSelectionWidget::createActionButtonsArea()
{
    QWidget *buttonWidget = new QWidget(this);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
    
    m_startAnalysisButton = new QPushButton(tr("开始分析"), this);
    m_startAnalysisButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #3d8b40;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #cccccc;"
        "    color: #666666;"
        "}"
    );
    
    m_resetConfigButton = new QPushButton(tr("重置配置"), this);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_resetConfigButton);
    buttonLayout->addWidget(m_startAnalysisButton);
    
    // 连接信号
    connect(m_startAnalysisButton, &QPushButton::clicked, this, &ProjectSelectionWidget::onStartAnalysisClicked);
    connect(m_resetConfigButton, &QPushButton::clicked, this, &ProjectSelectionWidget::resetToDefaults);
    
    return buttonWidget;
}

QString ProjectSelectionWidget::getProjectPath() const
{
    return m_projectPathEdit->text().trimmed();
}

void ProjectSelectionWidget::setProjectPath(const QString &path)
{
    m_projectPathEdit->setText(path);
}

QStringList ProjectSelectionWidget::getExcludePatterns() const
{
    QStringList patterns;
    for (int i = 0; i < m_excludeList->count(); ++i) {
        patterns.append(m_excludeList->item(i)->text());
    }
    return patterns;
}

void ProjectSelectionWidget::setExcludePatterns(const QStringList &patterns)
{
    m_excludeList->clear();
    m_excludeList->addItems(patterns);
}

bool ProjectSelectionWidget::getIncludeComments() const
{
    return m_includeCommentsCheck->isChecked();
}

void ProjectSelectionWidget::setIncludeComments(bool include)
{
    m_includeCommentsCheck->setChecked(include);
}

bool ProjectSelectionWidget::getIncludeBlankLines() const
{
    return m_includeBlankLinesCheck->isChecked();
}

void ProjectSelectionWidget::setIncludeBlankLines(bool include)
{
    m_includeBlankLinesCheck->setChecked(include);
}

void ProjectSelectionWidget::resetToDefaults()
{
    m_projectPathEdit->clear();
    m_includeCommentsCheck->setChecked(true);
    m_includeBlankLinesCheck->setChecked(true);
    resetExcludePatterns();
    updateUIState();
    emit configurationChanged();
}

bool ProjectSelectionWidget::validateConfiguration() const
{
    return getConfigurationErrors().isEmpty();
}

QStringList ProjectSelectionWidget::getConfigurationErrors() const
{
    QStringList errors;
    
    QString projectPath = getProjectPath();
    if (projectPath.isEmpty()) {
        errors.append(tr("项目路径是必需的"));
    } else if (!isValidProjectPath(projectPath)) {
        errors.append(tr("无效的项目路径: %1").arg(projectPath));
    }
    
    return errors;
}

void ProjectSelectionWidget::browseProjectFolder()
{
    QString currentPath = getProjectPath();
    if (currentPath.isEmpty()) {
        currentPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    
    QString selectedPath = QFileDialog::getExistingDirectory(
        this,
        tr("选择项目目录"),
        currentPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!selectedPath.isEmpty()) {
        setProjectPath(selectedPath);
        addToRecentProjects(selectedPath);
    }
}

void ProjectSelectionWidget::onProjectPathChanged()
{
    updateUIState();
    emit projectPathChanged(getProjectPath());
    emit configurationChanged();
}

void ProjectSelectionWidget::onRecentProjectChanged()
{
    int index = m_recentProjectsCombo->currentIndex();
    QStringList recentProjects = m_configManager->getRecentProjects();
    if (index >= 0 && index < recentProjects.size()) {
        QString selectedPath = recentProjects[index];
        if (!selectedPath.isEmpty() && selectedPath != getProjectPath()) {
            setProjectPath(selectedPath);
        }
    }
}

void ProjectSelectionWidget::addExcludePattern()
{
    QString pattern = m_excludePatternEdit->text().trimmed();
    if (pattern.isEmpty()) {
        return;
    }
    
    // 检查是否已存在
    for (int i = 0; i < m_excludeList->count(); ++i) {
        if (m_excludeList->item(i)->text() == pattern) {
            m_excludePatternEdit->clear();
            return;
        }
    }
    
    m_excludeList->addItem(pattern);
    m_excludePatternEdit->clear();
    emit configurationChanged();
}

void ProjectSelectionWidget::removeExcludePattern()
{
    int currentRow = m_excludeList->currentRow();
    if (currentRow >= 0) {
        delete m_excludeList->takeItem(currentRow);
        emit configurationChanged();
    }
}

void ProjectSelectionWidget::resetExcludePatterns()
{
    setExcludePatterns(m_defaultExcludePatterns);
    emit configurationChanged();
}

void ProjectSelectionWidget::onStartAnalysisClicked()
{
    if (!validateConfiguration()) {
        QStringList errors = getConfigurationErrors();
        QMessageBox::warning(this, tr("配置错误"),
                            tr("请修复以下问题:\n\n%1").arg(errors.join("\n")));
        return;
    }
    
    QString projectPath = getProjectPath();
    addToRecentProjects(projectPath);
    emit startAnalysis(projectPath);
}

void ProjectSelectionWidget::onConfigurationOptionChanged()
{
    emit configurationChanged();
}

void ProjectSelectionWidget::loadRecentProjects()
{
    QStringList recentProjects = m_configManager->getRecentProjects();
    
    // 更新下拉框
    m_recentProjectsCombo->clear();
    if (recentProjects.isEmpty()) {
        m_recentProjectsCombo->addItem(tr("无最近项目"));
        m_recentProjectsCombo->setEnabled(false);
        m_statusLabel->setText(tr("准备分析项目 - 无最近项目记录"));
    } else {
        for (const QString &path : recentProjects) {
            QFileInfo info(path);
            m_recentProjectsCombo->addItem(QString("%1 (%2)").arg(info.baseName(), path));
        }
        m_recentProjectsCombo->setEnabled(true);
        m_statusLabel->setText(tr("准备分析项目 - 已加载 %1 个最近项目").arg(recentProjects.size()));
    }
}

void ProjectSelectionWidget::saveRecentProjects()
{
    // 配置现在由ConfigManager自动管理，无需手动保存
    // 此方法保留以维持接口兼容性
}

void ProjectSelectionWidget::addToRecentProjects(const QString &projectPath)
{
    if (projectPath.isEmpty() || !QDir(projectPath).exists()) {
        return;
    }
    
    // 使用ConfigManager添加最近项目
    m_configManager->addRecentProject(projectPath);
    
    // UI会通过信号自动更新，无需手动调用loadRecentProjects()
}

void ProjectSelectionWidget::loadDefaultExcludePatterns()
{
    m_defaultExcludePatterns << "*.git*" << "*.svn*" << "*.hg*"
                            << "*.vs*" << "*.vscode*" << "*.idea*"
                            << "*build*" << "*debug*" << "*release*"
                            << "*bin*" << "*obj*" << "*target*"
                            << "*node_modules*" << "*vendor*"
                            << "*.tmp" << "*.temp" << "*.log";
    
    if (m_excludeList->count() == 0) {
        setExcludePatterns(m_defaultExcludePatterns);
    }
}

void ProjectSelectionWidget::updateUIState()
{
    QString projectPath = getProjectPath();
    bool isValidPath = isValidProjectPath(projectPath);
    
    m_startAnalysisButton->setEnabled(isValidPath);
    
    if (projectPath.isEmpty()) {
        m_statusLabel->setText(tr("请选择项目目录"));
        m_statusLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    } else if (!isValidPath) {
        m_statusLabel->setText(tr("无效的项目路径"));
        m_statusLabel->setStyleSheet("QLabel { color: #d32f2f; font-style: italic; }");
    } else {
        m_statusLabel->setText(tr("准备分析项目: %1").arg(QFileInfo(projectPath).baseName()));
        m_statusLabel->setStyleSheet("QLabel { color: #388e3c; font-style: italic; }");
    }
    
    m_removePatternButton->setEnabled(m_excludeList->currentRow() >= 0);
}

bool ProjectSelectionWidget::isValidProjectPath(const QString &path) const
{
    if (path.isEmpty()) {
        return false;
    }
    
    QFileInfo info(path);
    return info.exists() && info.isDir() && info.isReadable();
}
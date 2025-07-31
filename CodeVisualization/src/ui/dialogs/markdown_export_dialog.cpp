#include "markdown_export_dialog.h"

/**
 * @brief 构造函数
 * @param parent 父组件指针
 */
MarkdownExportDialog::MarkdownExportDialog(QWidget *parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_contentGroup(nullptr)
    , m_overviewCheckBox(nullptr)
    , m_detailedTableCheckBox(nullptr)
    , m_fileListCheckBox(nullptr)
    , m_languageStatsCheckBox(nullptr)
    , m_selectAllButton(nullptr)
    , m_selectNoneButton(nullptr)
    , m_modeGroup(nullptr)
    , m_singleFileRadio(nullptr)
    , m_multipleFilesRadio(nullptr)
    , m_modeButtonGroup(nullptr)
    , m_pathGroup(nullptr)
    , m_pathEdit(nullptr)
    , m_browseButton(nullptr)
    , m_pathLabel(nullptr)
    , m_prefixEdit(nullptr)
    , m_prefixLabel(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
{
    setWindowTitle(tr("Markdown导出配置"));
    setModal(true);
    resize(500, 350);
    
    initializeUI();
    updateUIState();
}

/**
 * @brief 析构函数
 */
MarkdownExportDialog::~MarkdownExportDialog()
{
}

/**
 * @brief 初始化UI界面
 */
void MarkdownExportDialog::initializeUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(15);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 创建各个区域
    m_mainLayout->addWidget(createContentSelectionGroup());
    m_mainLayout->addWidget(createExportModeGroup());
    m_mainLayout->addWidget(createPathSelectionGroup());
    m_mainLayout->addStretch();
    m_mainLayout->addWidget(createButtonArea());
    
    // 连接信号槽
    connect(m_browseButton, &QPushButton::clicked, this, &MarkdownExportDialog::onBrowseClicked);
    connect(m_singleFileRadio, &QRadioButton::toggled, this, &MarkdownExportDialog::onExportModeChanged);
    connect(m_multipleFilesRadio, &QRadioButton::toggled, this, &MarkdownExportDialog::onExportModeChanged);
    connect(m_okButton, &QPushButton::clicked, this, &MarkdownExportDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &MarkdownExportDialog::onCancelClicked);
    connect(m_selectAllButton, &QPushButton::clicked, this, &MarkdownExportDialog::onSelectAllClicked);
    connect(m_selectNoneButton, &QPushButton::clicked, this, &MarkdownExportDialog::onSelectNoneClicked);
}

/**
 * @brief 创建内容选择区域
 * @return 内容选择组件
 */
QGroupBox* MarkdownExportDialog::createContentSelectionGroup()
{
    m_contentGroup = new QGroupBox(tr("选择导出内容"), this);
    QVBoxLayout *layout = new QVBoxLayout(m_contentGroup);
    
    // 创建复选框
    m_overviewCheckBox = new QCheckBox(tr("概览信息（项目统计、总体数据）"), this);
    m_detailedTableCheckBox = new QCheckBox(tr("详细表格（按语言分组的统计数据）"), this);
    m_fileListCheckBox = new QCheckBox(tr("文件列表（每个文件的详细信息）"), this);
    m_languageStatsCheckBox = new QCheckBox(tr("语言统计（各编程语言的统计信息）"), this);
    
    // 默认选中所有选项
    m_overviewCheckBox->setChecked(true);
    m_detailedTableCheckBox->setChecked(true);
    m_fileListCheckBox->setChecked(true);
    m_languageStatsCheckBox->setChecked(true);
    
    layout->addWidget(m_overviewCheckBox);
    layout->addWidget(m_detailedTableCheckBox);
    layout->addWidget(m_fileListCheckBox);
    layout->addWidget(m_languageStatsCheckBox);
    
    // 添加快捷选择按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_selectAllButton = new QPushButton(tr("全选"), this);
    m_selectNoneButton = new QPushButton(tr("全不选"), this);
    
    m_selectAllButton->setMaximumWidth(80);
    m_selectNoneButton->setMaximumWidth(80);
    
    buttonLayout->addWidget(m_selectAllButton);
    buttonLayout->addWidget(m_selectNoneButton);
    buttonLayout->addStretch();
    
    layout->addLayout(buttonLayout);
    
    return m_contentGroup;
}

/**
 * @brief 创建导出模式选择区域
 * @return 导出模式选择组件
 */
QGroupBox* MarkdownExportDialog::createExportModeGroup()
{
    m_modeGroup = new QGroupBox(tr("导出模式"), this);
    QVBoxLayout *layout = new QVBoxLayout(m_modeGroup);
    
    m_singleFileRadio = new QRadioButton(tr("单文件模式（所有内容导出到一个Markdown文件）"), this);
    m_multipleFilesRadio = new QRadioButton(tr("多文件模式（每种内容导出到单独的Markdown文件）"), this);
    
    // 默认选择单文件模式
    m_singleFileRadio->setChecked(true);
    
    m_modeButtonGroup = new QButtonGroup(this);
    m_modeButtonGroup->addButton(m_singleFileRadio, static_cast<int>(SingleFile));
    m_modeButtonGroup->addButton(m_multipleFilesRadio, static_cast<int>(MultipleFiles));
    
    layout->addWidget(m_singleFileRadio);
    layout->addWidget(m_multipleFilesRadio);
    
    return m_modeGroup;
}

/**
 * @brief 创建路径选择区域
 * @return 路径选择组件
 */
QGroupBox* MarkdownExportDialog::createPathSelectionGroup()
{
    m_pathGroup = new QGroupBox(tr("导出路径"), this);
    QGridLayout *layout = new QGridLayout(m_pathGroup);
    
    // 路径选择
    m_pathLabel = new QLabel(tr("保存位置:"), this);
    m_pathEdit = new QLineEdit(this);
    m_browseButton = new QPushButton(tr("浏览..."), this);
    m_browseButton->setMaximumWidth(80);
    
    // 设置默认路径
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    m_pathEdit->setText(defaultPath);
    
    layout->addWidget(m_pathLabel, 0, 0);
    layout->addWidget(m_pathEdit, 0, 1);
    layout->addWidget(m_browseButton, 0, 2);
    
    // 文件前缀（多文件模式时使用）
    m_prefixLabel = new QLabel(tr("文件前缀:"), this);
    m_prefixEdit = new QLineEdit(this);
    m_prefixEdit->setText("analysis");
    m_prefixEdit->setPlaceholderText(tr("多文件模式时的文件名前缀"));
    
    layout->addWidget(m_prefixLabel, 1, 0);
    layout->addWidget(m_prefixEdit, 1, 1, 1, 2);
    
    return m_pathGroup;
}

/**
 * @brief 创建按钮区域
 * @return 按钮区域组件
 */
QWidget* MarkdownExportDialog::createButtonArea()
{
    QWidget *buttonWidget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(buttonWidget);
    
    layout->addStretch();
    
    m_okButton = new QPushButton(tr("导出"), this);
    m_cancelButton = new QPushButton(tr("取消"), this);
    
    m_okButton->setDefault(true);
    m_okButton->setMinimumWidth(80);
    m_cancelButton->setMinimumWidth(80);
    
    layout->addWidget(m_okButton);
    layout->addWidget(m_cancelButton);
    
    return buttonWidget;
}

/**
 * @brief 获取选择的导出内容
 * @return 导出内容标志
 */
MarkdownExportDialog::ExportContents MarkdownExportDialog::getSelectedContents() const
{
    ExportContents contents = static_cast<ExportContents>(0);
    
    if(m_overviewCheckBox->isChecked())
        contents |= Overview;
    if(m_detailedTableCheckBox->isChecked())
        contents |= DetailedTable;
    if(m_fileListCheckBox->isChecked())
        contents |= FileList;
    if(m_languageStatsCheckBox->isChecked())
        contents |= LanguageStats;
    
    return contents;
}

/**
 * @brief 获取导出模式
 * @return 导出模式
 */
MarkdownExportDialog::ExportMode MarkdownExportDialog::getExportMode() const
{
    return m_singleFileRadio->isChecked() ? SingleFile : MultipleFiles;
}

/**
 * @brief 获取导出路径
 * @return 导出路径
 */
QString MarkdownExportDialog::getExportPath() const
{
    return m_pathEdit->text();
}

/**
 * @brief 获取文件名前缀
 * @return 文件名前缀
 */
QString MarkdownExportDialog::getFilePrefix() const
{
    return m_prefixEdit->text();
}

/**
 * @brief 设置默认导出路径
 * @param path 默认路径
 */
void MarkdownExportDialog::setDefaultPath(const QString &path)
{
    if(!path.isEmpty())
        m_pathEdit->setText(path);
}

/**
 * @brief 浏览按钮点击槽函数
 */
void MarkdownExportDialog::onBrowseClicked()
{
    QString currentPath = m_pathEdit->text();
    if(currentPath.isEmpty())
        currentPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    
    QString selectedPath;
    if(getExportMode() == SingleFile)
    {
        // 单文件模式：选择保存文件
        selectedPath = QFileDialog::getSaveFileName(
            this,
            tr("选择Markdown文件保存位置"),
            QDir(currentPath).absoluteFilePath("analysis.md"),
            tr("Markdown文件 (*.md);;所有文件 (*.*)"));
    }
    else
    {
        // 多文件模式：选择保存目录
        selectedPath = QFileDialog::getExistingDirectory(
            this,
            tr("选择Markdown文件保存目录"),
            currentPath);
    }
    
    if(!selectedPath.isEmpty())
    {
        m_pathEdit->setText(selectedPath);
    }
}

/**
 * @brief 导出模式改变槽函数
 */
void MarkdownExportDialog::onExportModeChanged()
{
    updateUIState();
}

/**
 * @brief 确定按钮点击槽函数
 */
void MarkdownExportDialog::onOkClicked()
{
    if(validateInput())
        accept();
}

/**
 * @brief 取消按钮点击槽函数
 */
void MarkdownExportDialog::onCancelClicked()
{
    reject();
}

/**
 * @brief 全选按钮点击槽函数
 */
void MarkdownExportDialog::onSelectAllClicked()
{
    m_overviewCheckBox->setChecked(true);
    m_detailedTableCheckBox->setChecked(true);
    m_fileListCheckBox->setChecked(true);
    m_languageStatsCheckBox->setChecked(true);
}

/**
 * @brief 全不选按钮点击槽函数
 */
void MarkdownExportDialog::onSelectNoneClicked()
{
    m_overviewCheckBox->setChecked(false);
    m_detailedTableCheckBox->setChecked(false);
    m_fileListCheckBox->setChecked(false);
    m_languageStatsCheckBox->setChecked(false);
}

/**
 * @brief 验证输入
 * @return 输入是否有效
 */
bool MarkdownExportDialog::validateInput()
{
    // 检查是否选择了至少一种导出内容
    if(getSelectedContents() == 0)
    {
        QMessageBox::warning(this, tr("警告"), tr("请至少选择一种要导出的内容。"));
        return false;
    }
    
    // 检查路径是否为空
    QString path = m_pathEdit->text().trimmed();
    if(path.isEmpty())
    {
        QMessageBox::warning(this, tr("警告"), tr("请选择导出路径。"));
        return false;
    }
    
    // 检查多文件模式下的文件前缀
    if(getExportMode() == MultipleFiles)
    {
        QString prefix = m_prefixEdit->text().trimmed();
        if(prefix.isEmpty())
        {
            QMessageBox::warning(this, tr("警告"), tr("多文件模式下请输入文件前缀。"));
            return false;
        }
        
        // 检查目录是否存在或可创建
        QDir dir(path);
        if(!dir.exists())
        {
            int ret = QMessageBox::question(this, tr("确认"), 
                tr("目录 '%1' 不存在，是否创建？").arg(path),
                QMessageBox::Yes | QMessageBox::No);
            if(ret != QMessageBox::Yes)
                return false;
        }
    }
    else
    {
        // 单文件模式：检查目录是否存在
        QFileInfo fileInfo(path);
        QDir dir = fileInfo.absoluteDir();
        if(!dir.exists())
        {
            int ret = QMessageBox::question(this, tr("确认"), 
                tr("目录 '%1' 不存在，是否创建？").arg(dir.absolutePath()),
                QMessageBox::Yes | QMessageBox::No);
            if(ret != QMessageBox::Yes)
                return false;
        }
    }
    
    return true;
}

/**
 * @brief 更新UI状态
 */
void MarkdownExportDialog::updateUIState()
{
    bool isMultipleFiles = (getExportMode() == MultipleFiles);
    
    // 根据导出模式更新UI状态
    m_prefixLabel->setVisible(isMultipleFiles);
    m_prefixEdit->setVisible(isMultipleFiles);
    
    // 更新路径标签文本
    if(isMultipleFiles)
    {
        m_pathLabel->setText(tr("保存目录:"));
        m_pathEdit->setPlaceholderText(tr("选择保存Markdown文件的目录"));
    }
    else
    {
        m_pathLabel->setText(tr("保存位置:"));
        m_pathEdit->setPlaceholderText(tr("选择Markdown文件的保存位置"));
    }
}
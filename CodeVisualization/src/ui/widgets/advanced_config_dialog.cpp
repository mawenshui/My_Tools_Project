#include "advanced_config_dialog.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QInputDialog>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

AdvancedConfigDialog::AdvancedConfigDialog(QWidget *parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_excludeRulesPage(nullptr)
    , m_excludeRulesList(nullptr)
    , m_addExcludeRuleBtn(nullptr)
    , m_removeExcludeRuleBtn(nullptr)
    , m_editExcludeRuleBtn(nullptr)
    , m_excludeRuleEdit(nullptr)
    , m_languageSupportPage(nullptr)
    , m_languageTable(nullptr)
    , m_addLanguageBtn(nullptr)
    , m_removeLanguageBtn(nullptr)
    , m_editLanguageBtn(nullptr)
    , m_analysisOptionsPage(nullptr)
    , m_includeCommentsCheck(nullptr)
    , m_includeBlankLinesCheck(nullptr)
    , m_recursiveAnalysisCheck(nullptr)
    , m_followSymlinksCheck(nullptr)
    , m_maxFileSizeSpin(nullptr)
    , m_maxDepthSpin(nullptr)
    , m_encodingCombo(nullptr)
    , m_outputFormatCombo(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_applyButton(nullptr)
    , m_resetButton(nullptr)
{
    setWindowTitle(tr("高级配置"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);
    resize(600, 500);
    initializeDefaults();
    initializeUI();
    connectSignals();
    loadSettings();
}

AdvancedConfigDialog::~AdvancedConfigDialog()
{
}

void AdvancedConfigDialog::initializeUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // 创建标签页组件
    m_tabWidget = new QTabWidget(this);
    // 添加各个页面
    m_tabWidget->addTab(createExcludeRulesPage(), tr("排除规则"));
    m_tabWidget->addTab(createLanguageSupportPage(), tr("语言支持"));
    m_tabWidget->addTab(createAnalysisOptionsPage(), tr("分析选项"));
    mainLayout->addWidget(m_tabWidget);
    // 创建按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_resetButton = new QPushButton(tr("重置默认"), this);
    m_resetButton->setToolTip(tr("重置所有设置为默认值"));
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addStretch();
    m_okButton = new QPushButton(tr("确定"), this);
    m_okButton->setDefault(true);
    m_cancelButton = new QPushButton(tr("取消"), this);
    m_applyButton = new QPushButton(tr("应用"), this);
    m_applyButton->setToolTip(tr("应用设置但不关闭对话框"));
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_applyButton);
    mainLayout->addLayout(buttonLayout);
}

QWidget* AdvancedConfigDialog::createExcludeRulesPage()
{
    m_excludeRulesPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_excludeRulesPage);
    // 说明标签
    QLabel *descLabel = new QLabel(tr("配置需要排除的文件和目录规则（支持通配符）:"));
    layout->addWidget(descLabel);
    // 排除规则列表
    m_excludeRulesList = new QListWidget();
    m_excludeRulesList->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_excludeRulesList);
    // 输入框和按钮布局
    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_excludeRuleEdit = new QLineEdit();
    m_excludeRuleEdit->setPlaceholderText(tr("输入排除规则，如: *.tmp, build/*, .git"));
    inputLayout->addWidget(m_excludeRuleEdit);
    m_addExcludeRuleBtn = new QPushButton(tr("添加"));
    inputLayout->addWidget(m_addExcludeRuleBtn);
    layout->addLayout(inputLayout);
    // 操作按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_editExcludeRuleBtn = new QPushButton(tr("编辑"));
    m_removeExcludeRuleBtn = new QPushButton(tr("删除"));
    buttonLayout->addWidget(m_editExcludeRuleBtn);
    buttonLayout->addWidget(m_removeExcludeRuleBtn);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    // 示例说明
    QLabel *exampleLabel = new QLabel(tr("示例规则:\n"
                                         "*.tmp - 排除所有.tmp文件\n"
                                         "build/* - 排除build目录下所有内容\n"
                                         ".git - 排除.git目录\n"
                                         "**/node_modules - 排除所有node_modules目录"));
    exampleLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    layout->addWidget(exampleLabel);
    return m_excludeRulesPage;
}

QWidget* AdvancedConfigDialog::createLanguageSupportPage()
{
    m_languageSupportPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_languageSupportPage);
    // 说明标签
    QLabel *descLabel = new QLabel(tr("配置支持的编程语言和对应的文件扩展名:"));
    layout->addWidget(descLabel);
    // 语言支持表格
    m_languageTable = new QTableWidget();
    m_languageTable->setColumnCount(2);
    QStringList headers;
    headers << tr("语言名称") << tr("文件扩展名");
    m_languageTable->setHorizontalHeaderLabels(headers);
    m_languageTable->horizontalHeader()->setStretchLastSection(true);
    m_languageTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_languageTable);
    // 操作按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_addLanguageBtn = new QPushButton(tr("添加语言"));
    m_editLanguageBtn = new QPushButton(tr("编辑"));
    m_removeLanguageBtn = new QPushButton(tr("删除"));
    buttonLayout->addWidget(m_addLanguageBtn);
    buttonLayout->addWidget(m_editLanguageBtn);
    buttonLayout->addWidget(m_removeLanguageBtn);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    // 示例说明
    QLabel *exampleLabel = new QLabel(tr("示例:\n"
                                         "C++ : .cpp, .cxx, .cc, .h, .hpp\n"
                                         "Python : .py, .pyw\n"
                                         "JavaScript : .js, .jsx, .ts, .tsx"));
    exampleLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    layout->addWidget(exampleLabel);
    return m_languageSupportPage;
}

QWidget* AdvancedConfigDialog::createAnalysisOptionsPage()
{
    m_analysisOptionsPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_analysisOptionsPage);
    // 分析内容选项组
    QGroupBox *contentGroup = new QGroupBox(tr("分析内容"));
    QVBoxLayout *contentLayout = new QVBoxLayout(contentGroup);
    m_includeCommentsCheck = new QCheckBox(tr("包含注释行"));
    m_includeCommentsCheck->setToolTip(tr("统计时包含代码注释行"));
    contentLayout->addWidget(m_includeCommentsCheck);
    m_includeBlankLinesCheck = new QCheckBox(tr("包含空白行"));
    m_includeBlankLinesCheck->setToolTip(tr("统计时包含空白行"));
    contentLayout->addWidget(m_includeBlankLinesCheck);
    layout->addWidget(contentGroup);
    // 分析行为选项组
    QGroupBox *behaviorGroup = new QGroupBox(tr("分析行为"));
    QGridLayout *behaviorLayout = new QGridLayout(behaviorGroup);
    m_recursiveAnalysisCheck = new QCheckBox(tr("递归分析子目录"));
    m_recursiveAnalysisCheck->setToolTip(tr("分析所有子目录中的文件"));
    behaviorLayout->addWidget(m_recursiveAnalysisCheck, 0, 0, 1, 2);
    m_followSymlinksCheck = new QCheckBox(tr("跟随符号链接"));
    m_followSymlinksCheck->setToolTip(tr("分析符号链接指向的文件"));
    behaviorLayout->addWidget(m_followSymlinksCheck, 1, 0, 1, 2);
    // 最大文件大小
    QLabel *maxSizeLabel = new QLabel(tr("最大文件大小 (MB):"));
    behaviorLayout->addWidget(maxSizeLabel, 2, 0);
    m_maxFileSizeSpin = new QSpinBox();
    m_maxFileSizeSpin->setRange(1, 1000);
    m_maxFileSizeSpin->setValue(10);
    m_maxFileSizeSpin->setToolTip(tr("超过此大小的文件将被跳过"));
    behaviorLayout->addWidget(m_maxFileSizeSpin, 2, 1);
    // 最大目录深度
    QLabel *maxDepthLabel = new QLabel(tr("最大目录深度:"));
    behaviorLayout->addWidget(maxDepthLabel, 3, 0);
    m_maxDepthSpin = new QSpinBox();
    m_maxDepthSpin->setRange(1, 50);
    m_maxDepthSpin->setValue(10);
    m_maxDepthSpin->setToolTip(tr("限制分析的目录深度"));
    behaviorLayout->addWidget(m_maxDepthSpin, 3, 1);
    layout->addWidget(behaviorGroup);
    // 输出选项组
    QGroupBox *outputGroup = new QGroupBox(tr("输出选项"));
    QGridLayout *outputLayout = new QGridLayout(outputGroup);
    // 文件编码
    QLabel *encodingLabel = new QLabel(tr("文件编码:"));
    outputLayout->addWidget(encodingLabel, 0, 0);
    m_encodingCombo = new QComboBox();
    m_encodingCombo->addItems({"UTF-8", "GBK", "GB2312", "ASCII", "UTF-16"});
    m_encodingCombo->setCurrentText("UTF-8");
    outputLayout->addWidget(m_encodingCombo, 0, 1);
    // 输出格式
    QLabel *formatLabel = new QLabel(tr("默认输出格式:"));
    outputLayout->addWidget(formatLabel, 1, 0);
    m_outputFormatCombo = new QComboBox();
    m_outputFormatCombo->addItems({"HTML", "CSV", "JSON", "XML"});
    m_outputFormatCombo->setCurrentText("HTML");
    outputLayout->addWidget(m_outputFormatCombo, 1, 1);
    layout->addWidget(outputGroup);
    layout->addStretch();
    return m_analysisOptionsPage;
}

void AdvancedConfigDialog::connectSignals()
{
    // 排除规则相关信号
    connect(m_addExcludeRuleBtn, &QPushButton::clicked, this, &AdvancedConfigDialog::addExcludeRule);
    connect(m_removeExcludeRuleBtn, &QPushButton::clicked, this, &AdvancedConfigDialog::removeExcludeRule);
    connect(m_editExcludeRuleBtn, &QPushButton::clicked, this, &AdvancedConfigDialog::editExcludeRule);
    connect(m_excludeRuleEdit, &QLineEdit::returnPressed, this, &AdvancedConfigDialog::addExcludeRule);
    // 语言支持相关信号
    connect(m_addLanguageBtn, &QPushButton::clicked, this, &AdvancedConfigDialog::addLanguageSupport);
    connect(m_removeLanguageBtn, &QPushButton::clicked, this, &AdvancedConfigDialog::removeLanguageSupport);
    connect(m_editLanguageBtn, &QPushButton::clicked, this, &AdvancedConfigDialog::editLanguageSupport);
    // 按钮信号
    connect(m_okButton, &QPushButton::clicked, this, &AdvancedConfigDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &AdvancedConfigDialog::onCancelClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &AdvancedConfigDialog::applySettings);
    connect(m_resetButton, &QPushButton::clicked, this, &AdvancedConfigDialog::resetToDefaults);
    // 列表选择变化信号
    connect(m_excludeRulesList, &QListWidget::itemSelectionChanged, this, &AdvancedConfigDialog::updateUIState);
    connect(m_languageTable, &QTableWidget::itemSelectionChanged, this, &AdvancedConfigDialog::updateUIState);
}

void AdvancedConfigDialog::initializeDefaults()
{
    // 默认排除规则
    m_excludeRules << "*.tmp" << "*.log" << "*.bak" << "*.swp" << "*.swo"
                   << ".git" << ".svn" << ".hg" << ".bzr"
                   << "build" << "bin" << "obj" << "debug" << "release"
                   << "node_modules" << "__pycache__" << ".pytest_cache"
                   << "*.o" << "*.obj" << "*.exe" << "*.dll" << "*.so";
    // 默认支持的语言
    m_supportedLanguages["C++"] = QStringList() << ".cpp" << ".cxx" << ".cc" << ".c" << ".h" << ".hpp" << ".hxx";
    m_supportedLanguages["C"] = QStringList() << ".c" << ".h";
    m_supportedLanguages["Python"] = QStringList() << ".py" << ".pyw";
    m_supportedLanguages["Java"] = QStringList() << ".java";
    m_supportedLanguages["JavaScript"] = QStringList() << ".js" << ".jsx";
    m_supportedLanguages["TypeScript"] = QStringList() << ".ts" << ".tsx";
    m_supportedLanguages["C#"] = QStringList() << ".cs";
    m_supportedLanguages["Go"] = QStringList() << ".go";
    m_supportedLanguages["Rust"] = QStringList() << ".rs";
    m_supportedLanguages["PHP"] = QStringList() << ".php";
    m_supportedLanguages["Ruby"] = QStringList() << ".rb";
    m_supportedLanguages["Swift"] = QStringList() << ".swift";
    m_supportedLanguages["Kotlin"] = QStringList() << ".kt" << ".kts";
    m_supportedLanguages["Scala"] = QStringList() << ".scala";
    m_supportedLanguages["HTML"] = QStringList() << ".html" << ".htm";
    m_supportedLanguages["CSS"] = QStringList() << ".css";
    m_supportedLanguages["XML"] = QStringList() << ".xml";
    m_supportedLanguages["JSON"] = QStringList() << ".json";
    m_supportedLanguages["YAML"] = QStringList() << ".yml" << ".yaml";
    m_supportedLanguages["Markdown"] = QStringList() << ".md" << ".markdown";
    // 默认分析选项
    m_analysisOptions["includeComments"] = true;
    m_analysisOptions["includeBlankLines"] = true;
    m_analysisOptions["recursiveAnalysis"] = true;
    m_analysisOptions["followSymlinks"] = false;
    m_analysisOptions["maxFileSize"] = 10;
    m_analysisOptions["maxDepth"] = 10;
    m_analysisOptions["encoding"] = "UTF-8";
    m_analysisOptions["outputFormat"] = "HTML";
}

void AdvancedConfigDialog::addExcludeRule()
{
    QString rule = m_excludeRuleEdit->text().trimmed();
    if(!rule.isEmpty() && !m_excludeRules.contains(rule))
    {
        m_excludeRules.append(rule);
        m_excludeRulesList->addItem(rule);
        m_excludeRuleEdit->clear();
    }
}

void AdvancedConfigDialog::removeExcludeRule()
{
    int currentRow = m_excludeRulesList->currentRow();
    if(currentRow >= 0)
    {
        m_excludeRules.removeAt(currentRow);
        delete m_excludeRulesList->takeItem(currentRow);
    }
}

void AdvancedConfigDialog::editExcludeRule()
{
    int currentRow = m_excludeRulesList->currentRow();
    if(currentRow >= 0)
    {
        QString currentRule = m_excludeRules.at(currentRow);
        bool ok;
        QString newRule = QInputDialog::getText(this, tr("编辑排除规则"),
                                                tr("排除规则:"), QLineEdit::Normal,
                                                currentRule, &ok);
        if(ok && !newRule.trimmed().isEmpty() && newRule != currentRule)
        {
            m_excludeRules[currentRow] = newRule.trimmed();
            m_excludeRulesList->item(currentRow)->setText(newRule.trimmed());
        }
    }
}

void AdvancedConfigDialog::addLanguageSupport()
{
    bool ok;
    QString language = QInputDialog::getText(this, tr("添加语言支持"),
                       tr("语言名称:"), QLineEdit::Normal,
                       QString(), &ok);
    if(!ok || language.trimmed().isEmpty())
    {
        return;
    }
    QString extensions = QInputDialog::getText(this, tr("添加语言支持"),
                         tr("文件扩展名 (用逗号分隔):"), QLineEdit::Normal,
                         QString(), &ok);
    if(!ok || extensions.trimmed().isEmpty())
    {
        return;
    }
    QStringList extList;
    foreach(const QString &ext, extensions.split(',', QString::SkipEmptyParts))
    {
        QString trimmedExt = ext.trimmed();
        if(!trimmedExt.startsWith('.'))
        {
            trimmedExt = '.' + trimmedExt;
        }
        extList.append(trimmedExt);
    }
    m_supportedLanguages[language.trimmed()] = extList;
    updateLanguageTable();
}

void AdvancedConfigDialog::removeLanguageSupport()
{
    int currentRow = m_languageTable->currentRow();
    if(currentRow >= 0)
    {
        QString language = m_languageTable->item(currentRow, 0)->text();
        m_supportedLanguages.remove(language);
        updateLanguageTable();
    }
}

void AdvancedConfigDialog::editLanguageSupport()
{
    int currentRow = m_languageTable->currentRow();
    if(currentRow >= 0)
    {
        QString language = m_languageTable->item(currentRow, 0)->text();
        QStringList extensions = m_supportedLanguages[language];
        bool ok;
        QString newLanguage = QInputDialog::getText(this, tr("编辑语言支持"),
                              tr("语言名称:"), QLineEdit::Normal,
                              language, &ok);
        if(!ok)
        {
            return;
        }
        QString extString = extensions.join(", ");
        QString newExtensions = QInputDialog::getText(this, tr("编辑语言支持"),
                                tr("文件扩展名 (用逗号分隔):"), QLineEdit::Normal,
                                extString, &ok);
        if(!ok)
        {
            return;
        }
        QStringList extList;
        foreach(const QString &ext, newExtensions.split(',', QString::SkipEmptyParts))
        {
            QString trimmedExt = ext.trimmed();
            if(!trimmedExt.startsWith('.'))
            {
                trimmedExt = '.' + trimmedExt;
            }
            extList.append(trimmedExt);
        }
        if(newLanguage.trimmed() != language)
        {
            m_supportedLanguages.remove(language);
        }
        m_supportedLanguages[newLanguage.trimmed()] = extList;
        updateLanguageTable();
    }
}

void AdvancedConfigDialog::updateLanguageTable()
{
    m_languageTable->setRowCount(m_supportedLanguages.size());
    int row = 0;
    for(auto it = m_supportedLanguages.begin(); it != m_supportedLanguages.end(); ++it, ++row)
    {
        QTableWidgetItem *languageItem = new QTableWidgetItem(it.key());
        QTableWidgetItem *extensionsItem = new QTableWidgetItem(it.value().join(", "));
        m_languageTable->setItem(row, 0, languageItem);
        m_languageTable->setItem(row, 1, extensionsItem);
    }
}

void AdvancedConfigDialog::resetToDefaults()
{
    int ret = QMessageBox::question(this, tr("重置设置"),
                                    tr("确定要重置所有设置为默认值吗？"),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
    if(ret == QMessageBox::Yes)
    {
        initializeDefaults();
        updateUI();
    }
}

void AdvancedConfigDialog::updateUI()
{
    // 更新排除规则列表
    m_excludeRulesList->clear();
    m_excludeRulesList->addItems(m_excludeRules);
    // 更新语言支持表格
    updateLanguageTable();
    // 更新分析选项
    m_includeCommentsCheck->setChecked(m_analysisOptions["includeComments"].toBool());
    m_includeBlankLinesCheck->setChecked(m_analysisOptions["includeBlankLines"].toBool());
    m_recursiveAnalysisCheck->setChecked(m_analysisOptions["recursiveAnalysis"].toBool());
    m_followSymlinksCheck->setChecked(m_analysisOptions["followSymlinks"].toBool());
    m_maxFileSizeSpin->setValue(m_analysisOptions["maxFileSize"].toInt());
    m_maxDepthSpin->setValue(m_analysisOptions["maxDepth"].toInt());
    m_encodingCombo->setCurrentText(m_analysisOptions["encoding"].toString());
    m_outputFormatCombo->setCurrentText(m_analysisOptions["outputFormat"].toString());
}

void AdvancedConfigDialog::updateUIState()
{
    // 更新排除规则按钮状态
    bool hasExcludeSelection = m_excludeRulesList->currentRow() >= 0;
    m_removeExcludeRuleBtn->setEnabled(hasExcludeSelection);
    m_editExcludeRuleBtn->setEnabled(hasExcludeSelection);
    // 更新语言支持按钮状态
    bool hasLanguageSelection = m_languageTable->currentRow() >= 0;
    m_removeLanguageBtn->setEnabled(hasLanguageSelection);
    m_editLanguageBtn->setEnabled(hasLanguageSelection);
}

void AdvancedConfigDialog::applySettings()
{
    // 收集当前UI中的设置
    m_analysisOptions["includeComments"] = m_includeCommentsCheck->isChecked();
    m_analysisOptions["includeBlankLines"] = m_includeBlankLinesCheck->isChecked();
    m_analysisOptions["recursiveAnalysis"] = m_recursiveAnalysisCheck->isChecked();
    m_analysisOptions["followSymlinks"] = m_followSymlinksCheck->isChecked();
    m_analysisOptions["maxFileSize"] = m_maxFileSizeSpin->value();
    m_analysisOptions["maxDepth"] = m_maxDepthSpin->value();
    m_analysisOptions["encoding"] = m_encodingCombo->currentText();
    m_analysisOptions["outputFormat"] = m_outputFormatCombo->currentText();
    saveSettings();
}

void AdvancedConfigDialog::onOkClicked()
{
    applySettings();
    accept();
}

void AdvancedConfigDialog::onCancelClicked()
{
    reject();
}

void AdvancedConfigDialog::loadSettings()
{
    QSettings settings;
    // 加载排除规则
    settings.beginGroup("ExcludeRules");
    int size = settings.beginReadArray("rules");
    if(size > 0)
    {
        m_excludeRules.clear();
        for(int i = 0; i < size; ++i)
        {
            settings.setArrayIndex(i);
            m_excludeRules.append(settings.value("rule").toString());
        }
    }
    settings.endArray();
    settings.endGroup();
    // 加载语言支持
    settings.beginGroup("LanguageSupport");
    QStringList languages = settings.childGroups();
    if(!languages.isEmpty())
    {
        m_supportedLanguages.clear();
        foreach(const QString &language, languages)
        {
            settings.beginGroup(language);
            QStringList extensions = settings.value("extensions").toStringList();
            m_supportedLanguages[language] = extensions;
            settings.endGroup();
        }
    }
    settings.endGroup();
    // 加载分析选项
    settings.beginGroup("AnalysisOptions");
    m_analysisOptions["includeComments"] = settings.value("includeComments", true).toBool();
    m_analysisOptions["includeBlankLines"] = settings.value("includeBlankLines", true).toBool();
    m_analysisOptions["recursiveAnalysis"] = settings.value("recursiveAnalysis", true).toBool();
    m_analysisOptions["followSymlinks"] = settings.value("followSymlinks", false).toBool();
    m_analysisOptions["maxFileSize"] = settings.value("maxFileSize", 10).toInt();
    m_analysisOptions["maxDepth"] = settings.value("maxDepth", 10).toInt();
    m_analysisOptions["encoding"] = settings.value("encoding", "UTF-8").toString();
    m_analysisOptions["outputFormat"] = settings.value("outputFormat", "HTML").toString();
    settings.endGroup();
    updateUI();
}

void AdvancedConfigDialog::saveSettings()
{
    QSettings settings;
    // 保存排除规则
    settings.beginGroup("ExcludeRules");
    settings.beginWriteArray("rules");
    for(int i = 0; i < m_excludeRules.size(); ++i)
    {
        settings.setArrayIndex(i);
        settings.setValue("rule", m_excludeRules.at(i));
    }
    settings.endArray();
    settings.endGroup();
    // 保存语言支持
    settings.beginGroup("LanguageSupport");
    settings.remove(""); // 清除旧设置
    for(auto it = m_supportedLanguages.begin(); it != m_supportedLanguages.end(); ++it)
    {
        settings.beginGroup(it.key());
        settings.setValue("extensions", it.value());
        settings.endGroup();
    }
    settings.endGroup();
    // 保存分析选项
    settings.beginGroup("AnalysisOptions");
    for(auto it = m_analysisOptions.begin(); it != m_analysisOptions.end(); ++it)
    {
        settings.setValue(it.key(), it.value());
    }
    settings.endGroup();
}

QStringList AdvancedConfigDialog::getExcludeRules() const
{
    return m_excludeRules;
}

void AdvancedConfigDialog::setExcludeRules(const QStringList &rules)
{
    m_excludeRules = rules;
    if(m_excludeRulesList)
    {
        m_excludeRulesList->clear();
        m_excludeRulesList->addItems(m_excludeRules);
    }
}

QMap<QString, QStringList> AdvancedConfigDialog::getSupportedLanguages() const
{
    return m_supportedLanguages;
}

void AdvancedConfigDialog::setSupportedLanguages(const QMap<QString, QStringList>& languages)
{
    m_supportedLanguages = languages;
    if(m_languageTable)
    {
        updateLanguageTable();
    }
}

QMap<QString, QVariant> AdvancedConfigDialog::getAnalysisOptions() const
{
    return m_analysisOptions;
}

void AdvancedConfigDialog::setAnalysisOptions(const QMap<QString, QVariant>& options)
{
    m_analysisOptions = options;
    updateUI();
}

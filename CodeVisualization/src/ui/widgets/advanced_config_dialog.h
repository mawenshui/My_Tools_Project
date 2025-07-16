#ifndef ADVANCED_CONFIG_DIALOG_H
#define ADVANCED_CONFIG_DIALOG_H

#include <QtWidgets/QDialog>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include <QtCore/QSettings>
#include <QtCore/QStringList>

/**
 * @brief 高级配置对话框类
 * 
 * 提供代码分析工具的高级配置功能，包括：
 * - 自定义排除规则
 * - 更多语言支持
 * - 分析选项配置
 * 
 * @author CodeVisualization Team
 * @date 2024-01-01
 * @version 1.1.0
 */
class AdvancedConfigDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父组件指针
     */
    explicit AdvancedConfigDialog(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~AdvancedConfigDialog();
    
    /**
     * @brief 获取排除规则列表
     * @return 排除规则字符串列表
     */
    QStringList getExcludeRules() const;
    
    /**
     * @brief 设置排除规则列表
     * @param rules 排除规则字符串列表
     */
    void setExcludeRules(const QStringList &rules);
    
    /**
     * @brief 获取支持的语言列表
     * @return 语言扩展名映射
     */
    QMap<QString, QStringList> getSupportedLanguages() const;
    
    /**
     * @brief 设置支持的语言列表
     * @param languages 语言扩展名映射
     */
    void setSupportedLanguages(const QMap<QString, QStringList> &languages);
    
    /**
     * @brief 获取分析选项
     * @return 分析选项映射
     */
    QMap<QString, QVariant> getAnalysisOptions() const;
    
    /**
     * @brief 设置分析选项
     * @param options 分析选项映射
     */
    void setAnalysisOptions(const QMap<QString, QVariant> &options);
    
    /**
     * @brief 加载配置
     */
    void loadSettings();
    
    /**
     * @brief 保存配置
     */
    void saveSettings();

private slots:
    /**
     * @brief 添加排除规则
     */
    void addExcludeRule();
    
    /**
     * @brief 删除排除规则
     */
    void removeExcludeRule();
    
    /**
     * @brief 编辑排除规则
     */
    void editExcludeRule();
    
    /**
     * @brief 添加语言支持
     */
    void addLanguageSupport();
    
    /**
     * @brief 删除语言支持
     */
    void removeLanguageSupport();
    
    /**
     * @brief 编辑语言支持
     */
    void editLanguageSupport();
    
    /**
     * @brief 重置为默认设置
     */
    void resetToDefaults();
    
    /**
     * @brief 应用设置
     */
    void applySettings();
    
    /**
     * @brief 确定按钮点击
     */
    void onOkClicked();
    
    /**
     * @brief 取消按钮点击
     */
    void onCancelClicked();

private:
    /**
     * @brief 初始化UI
     */
    void initializeUI();
    
    /**
     * @brief 创建排除规则页面
     * @return 排除规则页面组件
     */
    QWidget* createExcludeRulesPage();
    
    /**
     * @brief 创建语言支持页面
     * @return 语言支持页面组件
     */
    QWidget* createLanguageSupportPage();
    
    /**
     * @brief 创建分析选项页面
     * @return 分析选项页面组件
     */
    QWidget* createAnalysisOptionsPage();
    
    /**
     * @brief 连接信号槽
     */
    void connectSignals();
    
    /**
     * @brief 初始化默认设置
     */
    void initializeDefaults();
    
    /**
     * @brief 更新UI显示
     */
    void updateUI();
    
    /**
     * @brief 更新语言表格
     */
    void updateLanguageTable();
    
    /**
     * @brief 更新UI状态
     */
    void updateUIState();
    
    // UI组件
    QTabWidget *m_tabWidget;                    ///< 标签页组件
    
    // 排除规则页面
    QWidget *m_excludeRulesPage;                ///< 排除规则页面
    QListWidget *m_excludeRulesList;            ///< 排除规则列表
    QPushButton *m_addExcludeRuleBtn;           ///< 添加排除规则按钮
    QPushButton *m_removeExcludeRuleBtn;        ///< 删除排除规则按钮
    QPushButton *m_editExcludeRuleBtn;          ///< 编辑排除规则按钮
    QLineEdit *m_excludeRuleEdit;               ///< 排除规则输入框
    
    // 语言支持页面
    QWidget *m_languageSupportPage;             ///< 语言支持页面
    QTableWidget *m_languageTable;              ///< 语言支持表格
    QPushButton *m_addLanguageBtn;              ///< 添加语言按钮
    QPushButton *m_removeLanguageBtn;           ///< 删除语言按钮
    QPushButton *m_editLanguageBtn;             ///< 编辑语言按钮
    
    // 分析选项页面
    QWidget *m_analysisOptionsPage;             ///< 分析选项页面
    QCheckBox *m_includeCommentsCheck;          ///< 包含注释复选框
    QCheckBox *m_includeBlankLinesCheck;        ///< 包含空行复选框
    QCheckBox *m_recursiveAnalysisCheck;        ///< 递归分析复选框
    QCheckBox *m_followSymlinksCheck;           ///< 跟随符号链接复选框
    QSpinBox *m_maxFileSizeSpin;                ///< 最大文件大小
    QSpinBox *m_maxDepthSpin;                   ///< 最大深度
    QComboBox *m_encodingCombo;                 ///< 编码选择
    QComboBox *m_outputFormatCombo;             ///< 输出格式选择
    
    // 按钮
    QPushButton *m_okButton;                    ///< 确定按钮
    QPushButton *m_cancelButton;                ///< 取消按钮
    QPushButton *m_applyButton;                 ///< 应用按钮
    QPushButton *m_resetButton;                 ///< 重置按钮
    
    // 数据
    QStringList m_excludeRules;                 ///< 排除规则列表
    QMap<QString, QStringList> m_supportedLanguages; ///< 支持的语言
    QMap<QString, QVariant> m_analysisOptions;  ///< 分析选项
};

#endif // ADVANCED_CONFIG_DIALOG_H
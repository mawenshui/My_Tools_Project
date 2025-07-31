#ifndef PROJECT_SELECTION_WIDGET_H
#define PROJECT_SELECTION_WIDGET_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QButtonGroup>
#include <QtCore/QStringList>
#include "../../core/config/config_manager.h"

/**
 * @brief 项目选择和配置组件
 * 
 * 提供项目路径选择、分析配置等功能
 * 支持最近项目历史记录
 * 支持排除规则配置
 * 
 * @author CodeVisualization Team
 * @date 2024-01-01
 * @version 1.0.0
 */
class ProjectSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 文件过滤模式枚举
     */
    enum FilterMode {
        ExcludeMode,    ///< 排除模式：不统计指定文件类型
        IncludeMode     ///< 包含模式：只统计指定文件类型
    };
    /**
     * @brief 构造函数
     * @param parent 父组件指针
     */
    explicit ProjectSelectionWidget(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~ProjectSelectionWidget();
    
    /**
     * @brief 获取选择的项目路径
     * @return 项目路径
     */
    QString getProjectPath() const;
    
    /**
     * @brief 设置项目路径
     * @param path 项目路径
     */
    void setProjectPath(const QString &path);
    
    /**
     * @brief 获取排除规则列表
     * @return 排除规则
     */
    QStringList getExcludePatterns() const;
    
    /**
     * @brief 设置排除规则列表
     * @param patterns 排除规则
     */
    void setExcludePatterns(const QStringList &patterns);
    
    /**
     * @brief 获取是否包含注释行
     * @return 是否包含注释行
     */
    bool getIncludeComments() const;
    
    /**
     * @brief 设置是否包含注释行
     * @param include 是否包含
     */
    void setIncludeComments(bool include);
    
    /**
     * @brief 获取是否包含空白行
     * @return 是否包含空白行
     */
    bool getIncludeBlankLines() const;
    
    /**
     * @brief 设置是否包含空白行
     * @param include 是否包含
     */
    void setIncludeBlankLines(bool include);
    
    /**
     * @brief 获取当前过滤模式
     * @return 过滤模式
     */
    FilterMode getFilterMode() const;
    
    /**
     * @brief 设置过滤模式
     * @param mode 过滤模式
     */
    void setFilterMode(FilterMode mode);
    
    /**
     * @brief 获取文件类型过滤列表
     * @return 文件类型列表
     */
    QStringList getFileTypeFilters() const;
    
    /**
     * @brief 设置文件类型过滤列表
     * @param types 文件类型列表
     */
    void setFileTypeFilters(const QStringList &types);
    
    /**
     * @brief 重置为默认配置
     */
    void resetToDefaults();
    
    /**
     * @brief 验证当前配置
     * @return 配置是否有效
     */
    bool validateConfiguration() const;
    
    /**
     * @brief 获取配置错误信息
     * @return 错误信息列表
     */
    QStringList getConfigurationErrors() const;

signals:
    /**
     * @brief 项目路径变化信号
     * @param path 新的项目路径
     */
    void projectPathChanged(const QString &path);
    
    /**
     * @brief 配置变化信号
     */
    void configurationChanged();
    
    /**
     * @brief 开始分析信号
     * @param projectPath 项目路径
     */
    void startAnalysis(const QString &projectPath);

private slots:
    /**
     * @brief 浏览项目文件夹
     */
    void browseProjectFolder();
    
    /**
     * @brief 项目路径文本变化
     */
    void onProjectPathChanged();
    
    /**
     * @brief 最近项目选择变化
     */
    void onRecentProjectChanged();
    
    /**
     * @brief 添加排除规则
     */
    void addExcludePattern();
    
    /**
     * @brief 移除规则按钮点击槽
     */
    void removeExcludePattern();
    
    /**
     * @brief 过滤模式改变槽
     */
    void onFilterModeChanged();
    
    /**
     * @brief 添加文件类型过滤
     */
    void addFileTypeFilter();
    
    /**
     * @brief 移除文件类型过滤
     */
    void removeFileTypeFilter();
    
    /**
     * @brief 重置文件类型过滤为预设
     */
    void resetFileTypeFilters();
    
    /**
     * @brief 重置排除规则为默认
     */
    void resetExcludePatterns();
    
    /**
     * @brief 开始分析按钮点击
     */
    void onStartAnalysisClicked();
    
    /**
     * @brief 配置选项变化
     */
    void onConfigurationOptionChanged();

private:
    /**
     * @brief 初始化UI
     */
    void initializeUI();
    
    /**
     * @brief 创建项目选择区域
     * @return 项目选择组件
     */
    QWidget* createProjectSelectionArea();
    
    /**
     * @brief 创建分析配置区域
     * @return 分析配置组件
     */
    QWidget* createAnalysisConfigArea();
    
    /**
     * @brief 创建排除规则区域
     * @return 排除规则区域组件
     */
    QWidget* createExcludeRulesArea();
    
    /**
     * @brief 创建文件类型过滤区域
     * @return 文件类型过滤区域组件
     */
    QWidget* createFileTypeFilterArea();
    
    /**
     * @brief 创建操作按钮区域
     * @return 操作按钮组件
     */
    QWidget* createActionButtonsArea();
    
    /**
     * @brief 加载最近项目列表
     */
    void loadRecentProjects();
    
    /**
     * @brief 保存最近项目列表
     */
    void saveRecentProjects();
    
    /**
     * @brief 添加到最近项目
     * @param projectPath 项目路径
     */
    void addToRecentProjects(const QString &projectPath);
    
    /**
     * @brief 加载默认排除规则
     */
    void loadDefaultExcludePatterns();
    
    /**
     * @brief 更新UI状态
     */
    void updateUIState();
    
    /**
     * @brief 验证项目路径
     * @param path 项目路径
     * @return 是否有效
     */
    bool isValidProjectPath(const QString &path) const;
    
    // UI组件
    QVBoxLayout *m_mainLayout;              ///< 主布局
    
    // 项目选择区域
    QGroupBox *m_projectGroup;              ///< 项目选择组
    QLineEdit *m_projectPathEdit;           ///< 项目路径输入框
    QPushButton *m_browseButton;            ///< 浏览按钮
    QComboBox *m_recentProjectsCombo;       ///< 最近项目下拉框
    
    // 分析配置区域
    QGroupBox *m_configGroup;               ///< 配置选项组
    QCheckBox *m_includeCommentsCheck;      ///< 包含注释行复选框
    QCheckBox *m_includeBlankLinesCheck;    ///< 包含空白行复选框
    
    // 排除规则区域
    QGroupBox *m_excludeGroup;              ///< 排除规则组
    QListWidget *m_excludeList;             ///< 排除规则列表
    QLineEdit *m_excludePatternEdit;        ///< 排除规则输入框
    QPushButton *m_addPatternButton;        ///< 添加规则按钮
    QPushButton *m_removePatternButton;     ///< 移除规则按钮
    QPushButton *m_resetPatternsButton;     ///< 重置规则按钮
    
    // 文件类型过滤相关组件
    QGroupBox *m_filterGroup;               ///< 过滤模式组
    QRadioButton *m_excludeModeRadio;       ///< 排除模式单选按钮
    QRadioButton *m_includeModeRadio;       ///< 包含模式单选按钮
    QButtonGroup *m_filterModeGroup;        ///< 过滤模式按钮组
    QListWidget *m_fileTypeList;            ///< 文件类型列表
    QLineEdit *m_fileTypeEdit;              ///< 文件类型输入框
    QPushButton *m_addFileTypeButton;       ///< 添加文件类型按钮
    QPushButton *m_removeFileTypeButton;    ///< 移除文件类型按钮
    QPushButton *m_resetFileTypeButton;     ///< 重置文件类型按钮
    QPushButton *m_presetCodeButton;        ///< 代码文件预设按钮
    QPushButton *m_presetDocButton;         ///< 文档文件预设按钮
    QPushButton *m_presetConfigButton;      ///< 配置文件预设按钮
    
    // 操作按钮区域
    QPushButton *m_startAnalysisButton;     ///< 开始分析按钮
    QPushButton *m_resetConfigButton;       ///< 重置配置按钮
    
    // 状态标签
    QLabel *m_statusLabel;                  ///< 状态标签
    
    // 数据成员
    ConfigManager *m_configManager;         ///< 配置管理器
    QStringList m_defaultExcludePatterns;   ///< 默认排除规则
    FilterMode m_currentFilterMode;         ///< 当前过滤模式
    QStringList m_defaultCodeTypes;         ///< 默认代码文件类型
    QStringList m_defaultDocTypes;          ///< 默认文档文件类型
    QStringList m_defaultConfigTypes;       ///< 默认配置文件类型
};

#endif // PROJECT_SELECTION_WIDGET_H
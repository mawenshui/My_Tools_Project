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
     * @brief 移除排除规则
     */
    void removeExcludePattern();
    
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
     * @return 排除规则组件
     */
    QWidget* createExcludeRulesArea();
    
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
    
    // 操作按钮区域
    QPushButton *m_startAnalysisButton;     ///< 开始分析按钮
    QPushButton *m_resetConfigButton;       ///< 重置配置按钮
    
    // 状态标签
    QLabel *m_statusLabel;                  ///< 状态标签
    
    // 数据成员
    ConfigManager *m_configManager;         ///< 配置管理器
    QStringList m_defaultExcludePatterns;   ///< 默认排除规则
};

#endif // PROJECT_SELECTION_WIDGET_H
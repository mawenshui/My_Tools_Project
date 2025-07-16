#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QAction>
#include <QtWidgets/QMenu>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QLabel>
#include <QtCore/QTimer>
#include "src/ui/widgets/project_selection_widget.h"
#include "src/ui/widgets/statistics_widget.h"
#include "src/ui/widgets/chart_widget.h"
#include "src/ui/widgets/advanced_config_dialog.h"
#include "src/ui/widgets/chart_selection_dialog.h"
#include "src/core/analyzer/code_analyzer.h"
#include "src/ui/theme/theme_manager.h"
#include "src/core/models/analysis_result.h"

/**
 * @brief 主窗口类
 * 
 * 代码行数统计工具的主界面
 * 集成项目选择、代码分析、结果显示等功能
 * 
 * @author CodeVisualization Team
 * @date 2024-01-01
 * @version 1.1.0
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父组件指针
     */
    explicit MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~MainWindow();

protected:
    /**
     * @brief 关闭事件处理
     * @param event 关闭事件
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    /**
     * @brief 开始分析
     * @param projectPath 项目路径
     */
    void startAnalysis(const QString &projectPath);
    
    /**
     * @brief 取消分析
     */
    void cancelAnalysis();
    
    /**
     * @brief 分析状态变化
     * @param status 分析状态
     */
    void onAnalysisStatusChanged(CodeAnalyzer::AnalysisStatus status);
    
    /**
     * @brief 分析进度更新
     * @param current 当前进度
     * @param total 总进度
     * @param message 进度消息
     */
    void onAnalysisProgressUpdated(int current, int total, const QString &message);
    
    /**
     * @brief 分析完成
     * @param result 分析结果
     */
    void onAnalysisCompleted(const AnalysisResult &result);
    
    /**
     * @brief 分析错误
     * @param error 错误信息
     */
    void onAnalysisError(const QString &error);
    
    /**
     * @brief 文件选择
     * @param filePath 文件路径
     */
    void onFileSelected(const QString &filePath);
    
    /**
     * @brief 新建项目
     */
    void newProject();
    
    /**
     * @brief 打开项目
     */
    void openProject();
    
    /**
     * @brief 保存结果
     */
    void saveResults();
    
    /**
     * @brief 导出结果
     */
    void exportResults();
    
    /**
     * @brief 退出应用
     */
    void exitApplication();
    
    /**
     * @brief 显示关于对话框
     */
    void showAbout();
    
    /**
     * @brief 显示关于Qt对话框
     */
    void showAboutQt();
    
    /**
     * @brief 显示帮助
     */
    void showHelp();
    
    /**
     * @brief 显示高级配置对话框
     */
    void showAdvancedConfig();
    
    /**
     * @brief 切换到亮色主题
     */
    void setLightTheme();
    
    /**
     * @brief 切换到暗色主题
     */
    void setDarkTheme();
    
    /**
     * @brief 切换到系统主题
     */
    void setSystemTheme();
    
    /**
     * @brief 主题变化处理
     * @param theme 新主题
     */
    void onThemeChanged(ThemeManager::ThemeType theme);

private:
    /**
     * @brief 初始化UI
     */
    void initializeUI();
    
    /**
     * @brief 创建菜单栏
     */
    void createMenuBar();
    
    /**
     * @brief 创建工具栏
     */
    void createToolBar();
    
    /**
     * @brief 创建状态栏
     */
    void createStatusBar();
    
    /**
     * @brief 创建中央组件
     */
    void createCentralWidget();
    
    /**
     * @brief 连接信号槽
     */
    void connectSignals();
    
    /**
     * @brief 读取设置
     */
    void readSettings();
    
    /**
     * @brief 写入设置
     */
    void writeSettings();
    
    /**
     * @brief 更新UI状态
     */
    void updateUIState();
    
    /**
     * @brief 设置窗口标题
     * @param projectPath 项目路径
     */
    void setWindowTitle(const QString &projectPath = QString());
    
    // UI组件
    QWidget *m_centralWidget;                   ///< 中央组件
    QSplitter *m_mainSplitter;                  ///< 主分割器
    QSplitter *m_rightSplitter;                 ///< 右侧分割器
    ProjectSelectionWidget *m_projectWidget;    ///< 项目选择组件
    StatisticsWidget *m_statisticsWidget;       ///< 统计显示组件
    ChartWidget *m_chartWidget;                 ///< 图表显示组件
    
    // 菜单和动作
    QMenuBar *m_menuBar;                        ///< 菜单栏
    QMenu *m_fileMenu;                          ///< 文件菜单
    QMenu *m_viewMenu;                          ///< 视图菜单
    QMenu *m_toolsMenu;                         ///< 工具菜单
    QMenu *m_themeMenu;                         ///< 主题菜单
    QMenu *m_helpMenu;                          ///< 帮助菜单
    
    QAction *m_newProjectAction;                ///< 新建项目动作
    QAction *m_openProjectAction;               ///< 打开项目动作
    QAction *m_saveResultsAction;               ///< 保存结果动作
    QAction *m_exportResultsAction;             ///< 导出结果动作
    QAction *m_exitAction;                      ///< 退出动作
    
    QAction *m_startAnalysisAction;             ///< 开始分析动作
    QAction *m_cancelAnalysisAction;            ///< 取消分析动作
    
    QAction *m_aboutAction;                     ///< 关于动作
    QAction *m_aboutQtAction;                   ///< 关于Qt动作
    QAction *m_helpAction;                      ///< 帮助动作
    QAction *m_advancedConfigAction;            ///< 高级配置动作
    
    QAction *m_lightThemeAction;                ///< 亮色主题动作
    QAction *m_darkThemeAction;                 ///< 暗色主题动作
    QAction *m_systemThemeAction;               ///< 系统主题动作
    QActionGroup *m_themeActionGroup;           ///< 主题动作组
    
    // 状态栏组件
    QLabel *m_statusLabel;                      ///< 状态标签
    QProgressBar *m_progressBar;                ///< 进度条
    QLabel *m_fileCountLabel;                   ///< 文件数量标签
    QLabel *m_lineCountLabel;                   ///< 行数标签
    
    // 核心组件
    CodeAnalyzer *m_analyzer;                   ///< 代码分析器
    
    // 状态变量
    bool m_analysisInProgress;                  ///< 分析是否进行中
    QString m_currentProjectPath;               ///< 当前项目路径
};

#endif // MAINWINDOW_H

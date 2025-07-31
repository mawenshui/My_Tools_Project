#ifndef STATISTICS_WIDGET_H
#define STATISTICS_WIDGET_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QHeaderView>
#include <QtCore/QTimer>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>
#include <QtCharts/QChart>
#include "../../core/models/analysis_result.h"
#include "../../core/models/file_statistics.h"
#include "../../core/export/csv_exporter.h"
#include "../../core/export/markdown_exporter.h"
#include "../dialogs/csv_export_dialog.h"
#include "../dialogs/markdown_export_dialog.h"
#include "html_preview_dialog.h"

QT_CHARTS_USE_NAMESPACE

/**
 * @brief 统计结果显示组件
 * 
 * 显示代码分析结果的各种统计信息
 * 包括概览、详细表格、文件列表等视图
 * 支持数据导出功能
 * 
 * @author CodeVisualization Team
 * @date 2024-01-01
 * @version 1.0.0
 */
/**
 * @brief 图表生成选项结构
 */
struct ChartGenerationOptions {
    bool generatePieChart = true;      ///< 生成饼图
    bool generateBarChart = true;      ///< 生成柱状图
    bool generateLineChart = false;    ///< 生成折线图
    bool generateAreaChart = false;    ///< 生成面积图
};

class StatisticsWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 显示模式枚举
     */
    enum DisplayMode {
        Overview,       ///< 概览模式
        DetailedTable,  ///< 详细表格模式
        FileList,       ///< 文件列表模式
        LanguageStats   ///< 语言统计模式
    };
    
    /**
     * @brief 构造函数
     * @param parent 父组件指针
     */
    explicit StatisticsWidget(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~StatisticsWidget();
    
    /**
     * @brief 设置分析结果
     * @param result 分析结果
     */
    void setAnalysisResult(const AnalysisResult &result);
    
    /**
     * @brief 清空显示内容
     */
    void clearContent();
    
    /**
     * @brief 获取当前显示模式
     * @return 显示模式
     */
    DisplayMode getCurrentDisplayMode() const;
    
    /**
     * @brief 设置显示模式
     * @param mode 显示模式
     */
    void setDisplayMode(DisplayMode mode);
    
    /**
     * @brief 导出数据到CSV（新版本）
     * @return 导出是否成功
     */
    bool exportToCSV();
    
    /**
     * @brief 导出数据到CSV（兼容旧版本）
     * @param filePath 文件路径
     * @return 导出是否成功
     */
    bool exportToCSV(const QString &filePath) const;
    
    /**
     * @brief 导出为HTML格式
     * @param filePath 文件路径
     * @return 导出是否成功
     */
    bool exportToHTML(const QString &filePath) const;
    
    /**
     * @brief 导出为Markdown格式（使用对话框）
     * @return 导出是否成功
     */
    bool exportToMarkdown();
    
    /**
     * @brief 导出为Markdown格式
     * @param filePath 文件路径
     * @return 导出是否成功
     */
    bool exportToMarkdown(const QString &filePath) const;


    
    /**
     * @brief 获取分析结果
     * @return 分析结果
     */
    const AnalysisResult& getAnalysisResult() const;

signals:
    /**
     * @brief 文件选择信号
     * @param filePath 文件路径
     */
    void fileSelected(const QString &filePath);
    
    /**
     * @brief 导出请求信号
     * @param format 导出格式
     */
    void exportRequested(const QString &format);

    /**
     * @brief 请求图表数据信号
     */
    void chartDataRequested();

public slots:
    /**
     * @brief 接收图表数据
     * @param charts 图表列表
     */
    void receiveChartData(const QList<QChart*> &charts);
    
    /**
     * @brief 设置图表生成选项
     * @param options 图表选项
     */
    void setChartGenerationOptions(const ChartGenerationOptions &options);
    
    /**
     * @brief 获取图表生成选项
     * @return 图表选项
     */
    ChartGenerationOptions getChartGenerationOptions() const;

public slots:
    /**
     * @brief 刷新显示
     */
    void refreshDisplay();
    
    /**
     * @brief 更新进度
     * @param current 当前进度
     * @param total 总进度
     * @param message 进度消息
     */
    void updateProgress(int current, int total, const QString &message);

private slots:
    /**
     * @brief 标签页切换
     * @param index 标签页索引
     */
    void onTabChanged(int index);
    
    /**
     * @brief 文件表格项目选择
     */
    void onFileTableItemSelectionChanged();
    
    /**
     * @brief 文件树项目选择
     */
    void onFileTreeItemSelectionChanged();
    
    /**
     * @brief 导出CSV按钮点击槽函数
     */
    void onExportCSVClicked();
    
    /**
     * @brief CSV导出进度更新槽函数
     * @param current 当前进度
     * @param total 总进度
     * @param message 进度消息
     */
    void onCSVExportProgress(int current, int total, const QString &message);
    
    /**
     * @brief CSV导出完成槽函数
     * @param success 是否成功
     * @param message 完成消息
     */
    void onCSVExportCompleted(bool success, const QString &message);
    
    /**
     * @brief 导出HTML按钮点击槽函数
     */
    void onExportHTMLClicked();
    
    /**
     * @brief 显示HTML预览对话框
     */
    void showHTMLPreviewDialog();
    
    /**
     * @brief 导出Markdown按钮点击槽函数
     */
    void onExportMarkdownClicked();
    
    /**
     * @brief Markdown导出进度更新槽函数
     * @param current 当前进度
     * @param total 总进度
     * @param message 进度消息
     */
    void onMarkdownExportProgress(int current, int total, const QString &message);
    
    /**
     * @brief Markdown导出完成槽函数
     * @param success 是否成功
     * @param message 结果消息
     */
    void onMarkdownExportCompleted(bool success, const QString &message);


    
    /**
     * @brief 刷新按钮点击
     */
    void onRefreshClicked();

private:
    /**
     * @brief 初始化UI
     */
    void initializeUI();
    
    /**
     * @brief 创建概览标签页
     * @return 概览组件
     */
    QWidget* createOverviewTab();
    
    /**
     * @brief 创建详细表格标签页
     * @return 详细表格组件
     */
    QWidget* createDetailedTableTab();
    
    /**
     * @brief 创建文件列表标签页
     * @return 文件列表组件
     */
    QWidget* createFileListTab();
    
    /**
     * @brief 创建语言统计标签页
     * @return 语言统计组件
     */
    QWidget* createLanguageStatsTab();
    
    /**
     * @brief 创建工具栏
     * @return 工具栏组件
     */
    QWidget* createToolBar();
    
    /**
     * @brief 更新概览显示
     */
    void updateOverviewDisplay();
    
    /**
     * @brief 更新详细表格显示
     */
    void updateDetailedTableDisplay();
    
    /**
     * @brief 更新文件列表显示
     */
    void updateFileListDisplay();
    
    /**
     * @brief 更新语言统计显示
     */
    void updateLanguageStatsDisplay();
    
    /**
     * @brief 创建概览信息卡片
     * @param title 标题
     * @param value 值
     * @param description 描述
     * @return 信息卡片组件
     */
    QWidget* createInfoCard(const QString &title, const QString &value, const QString &description = QString());
    
    /**
     * @brief 格式化文件大小
     * @param bytes 字节数
     * @return 格式化后的大小字符串
     */
    QString formatFileSize(qint64 bytes) const;
    
    /**
     * @brief 格式化数字
     * @param number 数字
     * @return 格式化后的数字字符串
     */
    QString formatNumber(int number) const;
    
    /**
     * @brief 格式化百分比
     * @param value 值
     * @return 格式化后的百分比字符串
     */
    QString formatPercentage(double value) const;
    
    /**
     * @brief 格式化持续时间
     * @param milliseconds 毫秒数
     * @return 格式化后的时间字符串
     */
    QString formatDuration(qint64 milliseconds) const;
    
    /**
     * @brief 获取语言图标
     * @param language 编程语言
     * @return 图标路径
     */
    QString getLanguageIcon(const QString &language) const;
    
    /**
     * @brief 生成HTML报告内容
     * @return HTML内容
     */
    QString generateHTMLReport() const;
    QString chartToBase64(QChart* chart) const;
    
    /**
     * @brief 生成CSV内容
     * @return CSV内容
     */
    QString generateCSVContent() const;
    
    // UI组件
    QVBoxLayout *m_mainLayout;              ///< 主布局
    QTabWidget *m_tabWidget;                ///< 标签页组件
    
    // 概览标签页
    QWidget *m_overviewTab;                 ///< 概览标签页
    QGridLayout *m_overviewLayout;          ///< 概览布局
    QLabel *m_projectPathLabel;             ///< 项目路径标签
    QLabel *m_analysisTimeLabel;            ///< 分析时间标签
    QLabel *m_totalFilesLabel;              ///< 总文件数标签
    QLabel *m_totalLinesLabel;              ///< 总行数标签
    QLabel *m_codeLinesLabel;               ///< 代码行数标签
    QLabel *m_commentLinesLabel;            ///< 注释行数标签
    QLabel *m_blankLinesLabel;              ///< 空白行数标签
    QLabel *m_languageCountLabel;           ///< 语言数量标签
    
    // 详细表格标签页
    QWidget *m_detailedTableTab;            ///< 详细表格标签页
    QTableWidget *m_detailedTable;          ///< 详细表格
    
    // 文件列表标签页
    QWidget *m_fileListTab;                 ///< 文件列表标签页
    QTreeWidget *m_fileTree;                ///< 文件树
    
    // 语言统计标签页
    QWidget *m_languageStatsTab;            ///< 语言统计标签页
    QTableWidget *m_languageTable;          ///< 语言统计表格
    
    // 工具栏
    QWidget *m_toolBar;                     ///< 工具栏
    QPushButton *m_exportCSVButton;         ///< 导出CSV按钮
    QPushButton *m_exportHTMLButton;        ///< 导出HTML按钮
    QPushButton *m_exportMarkdownButton;    ///< 导出Markdown按钮

    QList<QChart*> m_chartData;             ///< 图表数据缓存
    QPushButton *m_refreshButton;           ///< 刷新按钮
    QProgressBar *m_progressBar;            ///< 进度条
    QLabel *m_progressLabel;                ///< 进度标签
    
    // 数据成员
    AnalysisResult m_analysisResult;        ///< 分析结果
    DisplayMode m_currentDisplayMode;       ///< 当前显示模式
    ChartGenerationOptions m_chartOptions;  ///< 图表生成选项
    CSVExporter *m_csvExporter;             ///< CSV导出器
    MarkdownExporter *m_markdownExporter;   ///< Markdown导出器
};

#endif // STATISTICS_WIDGET_H
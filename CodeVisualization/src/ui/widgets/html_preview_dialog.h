#ifndef HTML_PREVIEW_DIALOG_H
#define HTML_PREVIEW_DIALOG_H

#include <QtWidgets/QDialog>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QTextBrowser>
#include <QtCharts/QChart>
#include <QtCore/QMap>
#include "../../core/models/analysis_result.h"

QT_CHARTS_USE_NAMESPACE

/**
 * @brief HTML导出预览对话框
 * 
 * 提供HTML导出前的预览功能，用户可以选择要导出的内容
 * 包括统计数据、表格、详细信息、各类图表等
 * 
 * @author CodeVisualization Team
 * @date 2024-12-01
 * @version 1.0.0
 */
class HtmlPreviewDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 导出设置结构体
     */
    struct ExportSettings {
        bool includeOverview = true;        ///< 包含概览统计
        bool includeLanguageStats = true;   ///< 包含语言统计
        bool includeFileList = true;        ///< 包含文件列表
        bool includeDetailedTable = true;   ///< 包含详细表格
        bool includeCharts = true;          ///< 包含图表
        
        // 图表选择
        bool includePieChart = true;        ///< 包含饼图
        bool includeBarChart = true;        ///< 包含柱状图
        bool includeLineChart = true;       ///< 包含折线图
        bool includeAreaChart = true;       ///< 包含面积图
        
        QString reportTitle = "代码统计分析报告";  ///< 报告标题
        QString customCSS;                  ///< 自定义CSS样式
    };

    /**
     * @brief 构造函数
     * @param analysisResult 分析结果
     * @param chartData 图表数据
     * @param parent 父窗口
     */
    explicit HtmlPreviewDialog(const AnalysisResult &analysisResult, 
                              const QList<QChart*> &chartData,
                              QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~HtmlPreviewDialog();

    /**
     * @brief 获取导出设置
     * @return 导出设置
     */
    ExportSettings getExportSettings() const;
    
    /**
     * @brief 生成HTML内容
     * @return HTML内容字符串
     */
    QString generateHTMLContent() const;

public slots:
    /**
     * @brief 更新预览
     */
    void updatePreview();

private slots:
    /**
     * @brief 内容选择改变
     */
    void onContentSelectionChanged();
    
    /**
     * @brief 图表选择改变
     */
    void onChartSelectionChanged();
    
    /**
     * @brief 报告标题改变
     */
    void onReportTitleChanged();
    
    /**
     * @brief 确认导出
     */
    void onExportClicked();
    
    /**
     * @brief 取消导出
     */
    void onCancelClicked();

private:
    /**
     * @brief 初始化UI
     */
    void initializeUI();
    
    /**
     * @brief 创建设置面板
     * @return 设置面板
     */
    QWidget* createSettingsPanel();
    
    /**
     * @brief 创建预览面板
     * @return 预览面板
     */
    QWidget* createPreviewPanel();
    
    /**
     * @brief 图表转Base64
     * @param chart 图表对象
     * @return Base64编码的图片
     */
    QString chartToBase64(QChart* chart) const;
    
    /**
     * @brief 格式化数字
     * @param number 数字
     * @return 格式化后的数字字符串
     */
    QString formatNumber(int number) const;
    
    /**
     * @brief 格式化文件大小
     * @param bytes 字节数
     * @return 格式化后的大小字符串
     */
    QString formatFileSize(qint64 bytes) const;

private:
    // 数据成员
    AnalysisResult m_analysisResult;        ///< 分析结果
    QList<QChart*> m_chartData;             ///< 图表数据
    ExportSettings m_settings;              ///< 导出设置
    
    // UI组件
    QVBoxLayout *m_mainLayout;              ///< 主布局
    QSplitter *m_splitter;                  ///< 分割器
    
    // 设置面板
    QWidget *m_settingsPanel;               ///< 设置面板
    QGroupBox *m_contentGroup;              ///< 内容选择组
    QCheckBox *m_overviewCheck;             ///< 概览复选框
    QCheckBox *m_languageStatsCheck;        ///< 语言统计复选框
    QCheckBox *m_fileListCheck;             ///< 文件列表复选框
    QCheckBox *m_detailedTableCheck;        ///< 详细表格复选框
    QCheckBox *m_chartsCheck;               ///< 图表复选框
    
    QGroupBox *m_chartGroup;                ///< 图表选择组
    QCheckBox *m_pieChartCheck;             ///< 饼图复选框
    QCheckBox *m_barChartCheck;             ///< 柱状图复选框
    QCheckBox *m_lineChartCheck;            ///< 折线图复选框
    QCheckBox *m_areaChartCheck;            ///< 面积图复选框
    
    QGroupBox *m_titleGroup;                ///< 标题设置组
    QLineEdit *m_titleEdit;                 ///< 标题编辑框
    
    // 预览面板
    QWidget *m_previewPanel;                ///< 预览面板
    QTextBrowser *m_webView;                ///< 网页预览
    
    // 按钮
    QHBoxLayout *m_buttonLayout;            ///< 按钮布局
    QPushButton *m_exportButton;            ///< 导出按钮
    QPushButton *m_cancelButton;            ///< 取消按钮
};

#endif // HTML_PREVIEW_DIALOG_H
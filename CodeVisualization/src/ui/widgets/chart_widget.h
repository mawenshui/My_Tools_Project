#ifndef CHART_WIDGET_H
#define CHART_WIDGET_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QSplitter>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QLegend>
#include <QtCore/QTimer>
#include "../../core/models/analysis_result.h"
#include "../../core/models/file_statistics.h"

QT_CHARTS_USE_NAMESPACE

/**
 * @brief 图表显示组件
 * 
 * 提供多种图表类型来可视化代码分析结果
 * 包括饼图、柱状图、折线图、散点图等
 * 支持交互式图表操作和数据导出
 * 
 * @author CodeVisualization Team
 * @date 2024-01-01
 * @version 1.1.0
 */
class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 图表类型枚举
     */
    enum ChartType {
        LanguagePieChart,       ///< 语言分布饼图
        FileTypeBarChart,       ///< 文件类型柱状图
        CodeLinesBarChart,      ///< 代码行数柱状图
        FileSizeScatterChart,   ///< 文件大小散点图
        ComplexityLineChart,    ///< 复杂度趋势图
        LanguageAreaChart       ///< 语言占比面积图
    };
    
    /**
     * @brief 构造函数
     * @param parent 父组件指针
     */
    explicit ChartWidget(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~ChartWidget();
    
    /**
     * @brief 设置分析结果
     * @param result 分析结果
     */
    void setAnalysisResult(const AnalysisResult &result);
    
    /**
     * @brief 清空图表内容
     */
    void clearCharts();
    
    /**
     * @brief 获取当前图表类型
     * @return 图表类型
     */
    ChartType getCurrentChartType() const;
    
    /**
     * @brief 设置图表类型
     * @param type 图表类型
     */
    void setChartType(ChartType type);
    
    /**
     * @brief 导出图表为图片
     * @param filePath 文件路径
     * @param format 图片格式 (PNG, SVG等)
     * @return 是否导出成功
     */
    bool exportChart(const QString &filePath, const QString &format = "PNG") const;

    /**
     * @brief 获取当前显示的图表
     * @return 当前图表对象
     */
    QChart* getCurrentChart() const;

    /**
     * @brief 获取所有可用的图表
     * @return 图表列表
     */
    QList<QChart*> getAllCharts() const;
    
    /**
     * @brief 获取分析结果
     * @return 分析结果
     */
    const AnalysisResult& getAnalysisResult() const;

signals:
    /**
     * @brief 图表点击信号
     * @param category 分类
     * @param value 值
     */
    void chartClicked(const QString &category, double value);
    
    /**
     * @brief 图表悬停信号
     * @param category 分类
     * @param value 值
     * @param hovered 是否悬停
     */
    void chartHovered(const QString &category, double value, bool hovered);
    
    /**
     * @brief 导出请求信号
     * @param format 导出格式
     */
    void exportRequested(const QString &format);

public slots:
    /**
     * @brief 刷新图表
     */
    void refreshCharts();
    
    /**
     * @brief 更新图表主题
     * @param theme 主题名称
     */
    void updateChartTheme(const QString &theme);

private slots:
    /**
     * @brief 图表类型选择变化
     * @param index 选择索引
     */
    void onChartTypeChanged(int index);
    
    /**
     * @brief 饼图切片点击
     * @param slice 饼图切片
     */
    void onPieSliceClicked(QPieSlice *slice);
    
    /**
     * @brief 饼图切片悬停
     * @param slice 饼图切片
     * @param state 悬停状态
     */
    void onPieSliceHovered(QPieSlice *slice, bool state);
    
    /**
     * @brief 柱状图点击
     * @param index 索引
     * @param barset 柱状图集合
     */
    void onBarClicked(int index, QBarSet *barset);
    
    /**
     * @brief 导出PNG按钮点击
     */
    void onExportPNGClicked();
    
    /**
     * @brief 导出SVG按钮点击
     */
    void onExportSVGClicked();

private:
    /**
     * @brief 初始化UI
     */
    void initializeUI();
    
    /**
     * @brief 创建控制面板
     * @return 控制面板组件
     */
    QWidget* createControlPanel();
    
    /**
     * @brief 创建图表视图
     * @return 图表视图组件
     */
    QWidget* createChartView();
    
    /**
     * @brief 创建语言分布饼图
     */
    void createLanguagePieChart();
    
    /**
     * @brief 创建文件类型柱状图
     */
    void createFileTypeBarChart();
    
    /**
     * @brief 创建代码行数柱状图
     */
    void createCodeLinesBarChart();
    
    /**
     * @brief 创建文件大小散点图
     */
    void createFileSizeScatterChart();
    
    /**
     * @brief 创建复杂度趋势图
     */
    void createComplexityLineChart();
    
    /**
     * @brief 创建语言占比面积图
     */
    void createLanguageAreaChart();
    
    /**
     * @brief 设置图表样式
     * @param chart 图表对象
     */
    void setupChartStyle(QChart *chart);
    
    /**
     * @brief 获取语言颜色
     * @param language 编程语言
     * @return 颜色
     */
    QColor getLanguageColor(const QString &language) const;
    
    /**
     * @brief 格式化数值
     * @param value 数值
     * @return 格式化后的字符串
     */
    QString formatValue(double value) const;
    
    /**
     * @brief 格式化百分比
     * @param value 数值
     * @return 格式化后的百分比字符串
     */
    QString formatPercentage(double value) const;
    
    // UI组件
    QVBoxLayout *m_mainLayout;              ///< 主布局
    QHBoxLayout *m_contentLayout;           ///< 内容布局
    
    // 控制面板
    QWidget *m_controlPanel;                ///< 控制面板
    QVBoxLayout *m_controlLayout;           ///< 控制布局
    QGroupBox *m_chartTypeGroup;            ///< 图表类型分组
    QComboBox *m_chartTypeCombo;            ///< 图表类型选择
    QGroupBox *m_exportGroup;               ///< 导出分组
    QPushButton *m_exportPNGButton;         ///< 导出PNG按钮
    QPushButton *m_exportSVGButton;         ///< 导出SVG按钮
    QLabel *m_chartInfoLabel;               ///< 图表信息标签
    
    // 图表视图
    QWidget *m_chartContainer;              ///< 图表容器
    QVBoxLayout *m_chartLayout;             ///< 图表布局
    QChartView *m_chartView;                ///< 图表视图
    QChart *m_currentChart;                 ///< 当前图表
    
    // 图表对象
    QPieSeries *m_languagePieSeries;        ///< 语言饼图系列
    QBarSeries *m_fileTypeBarSeries;        ///< 文件类型柱状图系列
    QBarSeries *m_codeLinesBarSeries;       ///< 代码行数柱状图系列
    QScatterSeries *m_fileSizeScatterSeries; ///< 文件大小散点图系列
    QLineSeries *m_complexityLineSeries;    ///< 复杂度折线图系列
    QAreaSeries *m_languageAreaSeries;      ///< 语言面积图系列
    
    // 数据成员
    AnalysisResult m_analysisResult;        ///< 分析结果
    ChartType m_currentChartType;           ///< 当前图表类型
    QMap<QString, QColor> m_languageColors; ///< 语言颜色映射
    bool m_isRefreshing;                    ///< 是否正在刷新图表
};

#endif // CHART_WIDGET_H
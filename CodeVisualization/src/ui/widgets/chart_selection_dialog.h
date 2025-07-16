#ifndef CHART_SELECTION_DIALOG_H
#define CHART_SELECTION_DIALOG_H

#include <QtWidgets/QDialog>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFrame>
#include <QtWidgets/QScrollArea>
#include <QtCore/QSettings>

/**
 * @brief 图表选择对话框
 * 
 * 在代码统计开始前，让用户选择要生成的图表类型
 * 支持保存用户的选择偏好
 * 
 * @author CodeVisualization Team
 * @date 2024-01-01
 * @version 1.0.0
 */
class ChartSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 图表选择选项结构
     */
    struct ChartSelectionOptions {
        bool generateLanguagePieChart = true;       ///< 生成语言分布饼图
        bool generateFileTypeBarChart = true;       ///< 生成文件类型柱状图
        bool generateCodeLinesBarChart = true;      ///< 生成代码行数柱状图
        bool generateFileSizeScatterChart = false;  ///< 生成文件大小散点图
        bool generateComplexityLineChart = false;   ///< 生成复杂度趋势图
        bool generateLanguageAreaChart = false;     ///< 生成语言占比面积图
        
        /**
         * @brief 检查是否有任何图表被选中
         * @return 是否有图表被选中
         */
        bool hasAnyChartSelected() const {
            return generateLanguagePieChart || generateFileTypeBarChart ||
                   generateCodeLinesBarChart || generateFileSizeScatterChart ||
                   generateComplexityLineChart || generateLanguageAreaChart;
        }
        
        /**
         * @brief 获取选中的图表数量
         * @return 选中的图表数量
         */
        int getSelectedChartCount() const {
            int count = 0;
            if (generateLanguagePieChart) count++;
            if (generateFileTypeBarChart) count++;
            if (generateCodeLinesBarChart) count++;
            if (generateFileSizeScatterChart) count++;
            if (generateComplexityLineChart) count++;
            if (generateLanguageAreaChart) count++;
            return count;
        }
    };
    
    /**
     * @brief 构造函数
     * @param parent 父组件指针
     */
    explicit ChartSelectionDialog(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~ChartSelectionDialog();
    
    /**
     * @brief 获取图表选择选项
     * @return 图表选择选项
     */
    ChartSelectionOptions getChartSelectionOptions() const;
    
    /**
     * @brief 设置图表选择选项
     * @param options 图表选择选项
     */
    void setChartSelectionOptions(const ChartSelectionOptions &options);
    
    /**
     * @brief 加载用户偏好设置
     */
    void loadUserPreferences();
    
    /**
     * @brief 保存用户偏好设置
     */
    void saveUserPreferences();

public slots:
    /**
     * @brief 接受对话框
     */
    void accept() override;
    
    /**
     * @brief 拒绝对话框
     */
    void reject() override;

private slots:
    /**
     * @brief 图表选择状态改变
     */
    void onChartSelectionChanged();
    
    /**
     * @brief 全选按钮点击
     */
    void onSelectAllClicked();
    
    /**
     * @brief 全不选按钮点击
     */
    void onSelectNoneClicked();
    
    /**
     * @brief 推荐选择按钮点击
     */
    void onRecommendedClicked();
    
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
     * @brief 创建图表选择区域
     * @return 图表选择组件
     */
    QWidget* createChartSelectionArea();
    
    /**
     * @brief 创建按钮区域
     * @return 按钮区域组件
     */
    QWidget* createButtonArea();
    
    /**
     * @brief 创建图表选项卡片
     * @param title 标题
     * @param description 描述
     * @param checkBox 复选框
     * @param recommended 是否推荐
     * @return 卡片组件
     */
    QWidget* createChartCard(const QString &title, const QString &description, 
                            QCheckBox *checkBox, bool recommended = false);
    
    /**
     * @brief 更新选择状态显示
     */
    void updateSelectionStatus();
    
    /**
     * @brief 验证选择
     * @return 是否有效
     */
    bool validateSelection();
    
    // UI组件
    QVBoxLayout *m_mainLayout;                      ///< 主布局
    QScrollArea *m_scrollArea;                      ///< 滚动区域
    QWidget *m_contentWidget;                       ///< 内容组件
    QVBoxLayout *m_contentLayout;                   ///< 内容布局
    
    // 标题和说明
    QLabel *m_titleLabel;                           ///< 标题标签
    QLabel *m_descriptionLabel;                     ///< 描述标签
    QLabel *m_statusLabel;                          ///< 状态标签
    
    // 图表选择区域
    QGroupBox *m_chartSelectionGroup;               ///< 图表选择分组
    QGridLayout *m_chartSelectionLayout;            ///< 图表选择布局
    
    // 图表复选框
    QCheckBox *m_languagePieChartCheck;             ///< 语言分布饼图复选框
    QCheckBox *m_fileTypeBarChartCheck;             ///< 文件类型柱状图复选框
    QCheckBox *m_codeLinesBarChartCheck;            ///< 代码行数柱状图复选框
    QCheckBox *m_fileSizeScatterChartCheck;         ///< 文件大小散点图复选框
    QCheckBox *m_complexityLineChartCheck;          ///< 复杂度趋势图复选框
    QCheckBox *m_languageAreaChartCheck;            ///< 语言占比面积图复选框
    
    // 快捷按钮
    QHBoxLayout *m_quickButtonLayout;               ///< 快捷按钮布局
    QPushButton *m_selectAllButton;                 ///< 全选按钮
    QPushButton *m_selectNoneButton;                ///< 全不选按钮
    QPushButton *m_recommendedButton;               ///< 推荐选择按钮
    
    // 对话框按钮
    QHBoxLayout *m_buttonLayout;                    ///< 按钮布局
    QPushButton *m_okButton;                        ///< 确定按钮
    QPushButton *m_cancelButton;                    ///< 取消按钮
    
    // 数据成员
    ChartSelectionOptions m_options;                ///< 图表选择选项
    QSettings *m_settings;                          ///< 设置对象
};

#endif // CHART_SELECTION_DIALOG_H